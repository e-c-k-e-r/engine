#if !UF_USE_REACTPHYSICS
#include <uf/utils/math/physics/impl.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/utils/memory/stack.h>

namespace {
	bool warmupSolver = true;
	bool blockContactSolver = false; // blockNxN solver is flawed
	bool psgContactSolver = true; // iterative solver is flawed
	bool useGjk = false; // currently don't have a way to broadphase mesh => narrowphase tri via GJK
	bool fixedStep = true;
	int substeps = 4;

	// increasing these make things lag
	int broadphaseBvhCapacity = 1;
	int meshBvhCapacity = 1;

	int solverIterations = 5;
	float baumgarteCorrectionPercent = 0.005f; // needs to be very small or the correction is too large
	float baumgarteCorrectionSlop = 0.001f;
	uf::stl::unordered_map<size_t, pod::Manifold> manifoldsCache;
}

#define EPS(x) x // 1.0e-6f
#define ASSERT_COLLIDER_TYPES( A, B ) UF_ASSERT( a.collider.type == pod::ShapeType::A && b.collider.type == pod::ShapeType::B );
#define UF_PHYSICS_TEST 0

#include "helpers.inl"
#include "aabb.inl"
#include "sphere.inl"
#include "plane.inl"
#include "capsule.inl"
#include "triangle.inl"
#include "mesh.inl"
#include "ray.inl"
#include "bvh.inl"
#include "gjk.inl"
#include "epa.inl"
#include "integration.inl"
#include "solvers.inl"

// unused
float uf::physics::impl::timescale = 1.0f / 60.0f;
bool uf::physics::impl::interpolate = false;
bool uf::physics::impl::shared = false;
bool uf::physics::impl::globalStorage = false;

// Bindings
void uf::physics::impl::initialize() {
	uf::physics::impl::initialize( uf::scene::getCurrentScene() );
}
void uf::physics::impl::initialize( uf::Object& object ) {
	uf::physics::impl::initialize( object.getComponent<pod::World>() );
}
void uf::physics::impl::initialize( pod::World& world ) {
	// to-do: any additional initialization (since it's not needed right now)
}
void uf::physics::impl::tick( float dt ) {
	uf::physics::impl::tick( uf::scene::getCurrentScene(), dt );
}
void uf::physics::impl::tick( uf::Object& object, float dt ) {
	uf::physics::impl::tick( object.getComponent<pod::World>(), dt );
}
void uf::physics::impl::tick( pod::World& world, float dt ) {
	if ( !::fixedStep ) {

		if ( ::substeps > 0 ) uf::physics::impl::substep( world, dt, ::substeps );
		else uf::physics::impl::step( world, dt );

		return;
	}

	static float accumulator = 0;
	accumulator += dt; 
	while ( accumulator >= uf::physics::impl::timescale ) { 
		if ( ::substeps > 0 ) uf::physics::impl::substep( world, uf::physics::impl::timescale, ::substeps ); 
		else uf::physics::impl::step( world, uf::physics::impl::timescale ); 
		accumulator -= uf::physics::impl::timescale; 
	}
}
void uf::physics::impl::terminate() {
	uf::physics::impl::terminate( uf::scene::getCurrentScene() );
}
void uf::physics::impl::terminate( uf::Object& object ) {
	uf::physics::impl::terminate( object.getComponent<pod::World>() );
}
void uf::physics::impl::terminate( pod::World& world ) {
	world.bodies.clear();
}

// Implementation
void uf::physics::impl::substep( pod::World& world, float dt, int substeps ) {
	float h = dt / substeps;
	for ( auto i=0; i < substeps; ++i) {
		uf::physics::impl::step( world, h );
	}
}
void uf::physics::impl::step( pod::World& world, float dt ) {
	auto& bodies = world.bodies;
	auto& bvh = world.bvh;

	if ( bodies.empty() ) return;

	for ( auto* body : bodies ) {
		::integrate( *body, dt );
	}
	
	uf::stl::vector<pod::Manifold> manifolds;
	uf::stl::vector<std::pair<int,int>> pairs;

	// create BVH
	::buildBroadphaseBVH( bvh, bodies, ::broadphaseBvhCapacity );
	// query for overlaps
	::queryOverlaps( bvh, pairs );
	// iterate overlaps
	for ( auto& [ia, ib] : pairs ) {
		auto& a = *bodies[ia];
		auto& b = *bodies[ib];

		// could be also pruned in the broadphase, but traversal needs to be agnostic between a BVH for bodies or a BVH for triangles
		if ( !::shouldCollide( a, b ) ) continue;

		pod::Manifold manifold;
		if ( ::generateContacts( a, b, manifold, dt ) ) {
			// bodies with meshes already reorient the normal to the triangle's center
			// do not do it for meshes because it'll reorient to the mesh's origin
			if ( a.collider.type != pod::ShapeType::MESH && b.collider.type != pod::ShapeType::MESH ) {
				for ( auto& c : manifold.points ) c.normal = ::orientNormalToAB( a, b, c.normal );
			}
			// retrieve accumulated impulses
			if ( ::warmupSolver )  ::retrieveContacts( manifold, ::manifoldsCache[::makePairKey( a, b )] );
			// merge similar contacts from a mesh to ensure continuity
			if ( a.collider.type == pod::ShapeType::MESH || b.collider.type == pod::ShapeType::MESH ) {
				::mergeContacts( manifold );
			}
			// keep four most important contacts
			::reduceContacts( manifold );
			// no points remained, skip
			if ( manifold.points.empty() ) continue;

			// store manifold
			manifolds.emplace_back(manifold);
		}
	}

	// pass manifolds to solver
	::solveContacts( manifolds, dt );

	if ( ::warmupSolver ) {
		// update cache
		for ( auto& manifold : manifolds ) {
			::manifoldsCache[::makePairKey( *manifold.a, *manifold.b )] = manifold;
		}

		// prune if too old / empty
		for ( auto& [ key, manifold ] : manifoldsCache ) {
			// prune manifolds that are X frames old
			for ( auto it = manifold.points.begin(); it != manifold.points.end(); ) {
				if ( it->lifetime > 3 ) manifold.points.erase(it);
				else ++it;
			}
			// empty manifold, kill it
			if ( manifold.points.empty() ) manifoldsCache.erase(key);
		}
	}

	// recompute bounds for further queries
	for ( auto* body : bodies ) {
		body->bounds = ::computeAABB( *body );
	}
}

void uf::physics::impl::setMass( pod::PhysicsBody& body, float mass ) {
	body.mass = mass;
	body.inverseMass = 1.0f / mass;
	uf::physics::impl::updateInertia( body );
}

void uf::physics::impl::setColliderCategory( pod::PhysicsBody& body, uint32_t category ) {
	body.collider.category = category;
}
void uf::physics::impl::setColliderCategory( pod::PhysicsBody& body, const uf::stl::string& category ) {
	auto c = uf::string::uppercase( category );
	if ( c == "NONE" ) return uf::physics::impl::setColliderCategory( body, pod::Collider::CATEGORY_NONE );
	if ( c == "STATIC" ) return uf::physics::impl::setColliderCategory( body, pod::Collider::CATEGORY_STATIC );
	if ( c == "DYNAMIC" ) return uf::physics::impl::setColliderCategory( body, pod::Collider::CATEGORY_DYNAMIC );
	if ( c == "PLAYER" ) return uf::physics::impl::setColliderCategory( body, pod::Collider::CATEGORY_PLAYER );
	if ( c == "NPC" ) return uf::physics::impl::setColliderCategory( body, pod::Collider::CATEGORY_NPC );
	if ( c == "TRIGGER" ) return uf::physics::impl::setColliderCategory( body, pod::Collider::CATEGORY_TRIGGER );
	if ( c == "PROJECTILE" ) return uf::physics::impl::setColliderCategory( body, pod::Collider::CATEGORY_PROJECTILE );
	if ( c == "CHARACTER" ) return uf::physics::impl::setColliderCategory( body, pod::Collider::CATEGORY_CHARACTER );
	if ( c == "ALL" ) return uf::physics::impl::setColliderCategory( body, pod::Collider::CATEGORY_ALL );
}
void uf::physics::impl::setColliderMask( pod::PhysicsBody& body, uint32_t mask ) {
	body.collider.mask = mask;
}
void uf::physics::impl::setColliderMask( pod::PhysicsBody& body, const uf::stl::string& mask ) {
	auto m = uf::string::uppercase( mask );
	if ( m == "NONE" ) return uf::physics::impl::setColliderMask( body, pod::Collider::MASK_NONE );
	if ( m == "STATIC" ) return uf::physics::impl::setColliderMask( body, pod::Collider::MASK_STATIC );
	if ( m == "DYNAMIC" ) return uf::physics::impl::setColliderMask( body, pod::Collider::MASK_DYNAMIC );
	if ( m == "PLAYER" ) return uf::physics::impl::setColliderMask( body, pod::Collider::MASK_PLAYER );
	if ( m == "NPC" ) return uf::physics::impl::setColliderMask( body, pod::Collider::MASK_NPC );
	if ( m == "TRIGGER" ) return uf::physics::impl::setColliderMask( body, pod::Collider::MASK_TRIGGER );
	if ( m == "PROJECTILE" ) return uf::physics::impl::setColliderMask( body, pod::Collider::MASK_PROJECTILE );
	if ( m == "CHARACTER" ) return uf::physics::impl::setColliderCategory( body, pod::Collider::MASK_CHARACTER );
	if ( m == "ALL" ) return uf::physics::impl::setColliderMask( body, pod::Collider::MASK_ALL );
}
void uf::physics::impl::setGravity( pod::PhysicsBody& body, const pod::Vector3f& gravity ) {
	body.gravity = gravity;
}
pod::Vector3f uf::physics::impl::getGravity( pod::PhysicsBody& body ) {
	return uf::vector::isValid( body.gravity ) ? body.gravity : body.world->gravity;
}

void uf::physics::impl::updateInertia( pod::PhysicsBody& body ) {
	if ( body.isStatic || body.mass <= 0 ) {
		body.inverseInertiaTensor = {};
		return;
	}

	switch ( body.collider.type ) {
		case pod::ShapeType::AABB: {
			pod::Vector3f extents = (body.collider.u.aabb.max - body.collider.u.aabb.min);
			extents *= extents; // square it;

			body.inertiaTensor = extents * (body.mass / 12.0f);
			// to-do: add overloaded inverse order arithmetic operators
			//body.inverseInertiaTensor = 1.0f / body.inertiaTensor;
			body.inverseInertiaTensor = { 1.0f / body.inertiaTensor.x, 1.0f / body.inertiaTensor.y, 1.0f / body.inertiaTensor.z };
		} break;
		case pod::ShapeType::SPHERE: {
			float I = 0.4f * body.mass * body.collider.u.sphere.radius * body.collider.u.sphere.radius;
			float invI = 1.0f / I;
			body.inertiaTensor = { I, I, I };
			body.inverseInertiaTensor = { invI, invI, invI };
		} break;
		case pod::ShapeType::CAPSULE: {
			float r = body.collider.u.capsule.radius;
			float h = body.collider.u.capsule.halfHeight * 2.0f; // full cyl height
			float m = body.mass;

			float Ixx = 0.25f * m * r * r + (1.0f/12.0f) * m * h * h;
			float Iyy = 0.5f * m * r * r;
			float Izz = Ixx;

			body.inertiaTensor = { Ixx, Iyy, Izz };
			body.inverseInertiaTensor = { 1.0f/Ixx, 1.0f/Iyy, 1.0f/Izz };
		} break;

		// to-do: add others
		default: {
		} break;
	}
}
void uf::physics::impl::applyForce( pod::PhysicsBody& body, const pod::Vector3f& force ) {
	if ( body.isStatic ) return;
	body.forceAccumulator += force;
}
void uf::physics::impl::applyForceAtPoint( pod::PhysicsBody body, const pod::Vector3f& force, const pod::Vector3f& point ) {
	if ( body.isStatic ) return;
	// linear force
	body.forceAccumulator += force;
	// angular force
	pod::Vector3f r = point - ::getPosition( body );
	body.torqueAccumulator += uf::vector::cross( r, force );
}
void uf::physics::impl::applyImpulse( pod::PhysicsBody& body, const pod::Vector3f& impulse ) {
	if ( body.isStatic ) return;
	body.velocity += impulse * body.inverseMass;
}
void uf::physics::impl::applyTorque( pod::PhysicsBody& body, const pod::Vector3f& torque ) {
	if ( body.isStatic ) return;
	body.torqueAccumulator += torque;
}
void uf::physics::impl::setVelocity( pod::PhysicsBody& body, const pod::Vector3f& v ) {
	body.velocity = v;
}
void uf::physics::impl::applyRotation( pod::PhysicsBody& body, const pod::Quaternion<>& q ) {
	uf::transform::rotate( *body.transform/*.reference*/, q );
}
void uf::physics::impl::applyRotation( pod::PhysicsBody& body, const pod::Vector3f& axis, float angle ) {
	applyRotation( body, uf::quaternion::axisAngle( axis, angle ) );
}

// body creation
pod::PhysicsBody& uf::physics::impl::create( pod::World& world, uf::Object& object, float mass, const pod::Vector3f& offset ) {
	// bind to component
	pod::PhysicsBody& body = object.getComponent<pod::PhysicsBody>();
	// initial initialization
	body.world = &world;
	body.object = &object;
	body.transform/*.reference*/ = &object.getComponent<pod::Transform<>>();
	body.offset = offset;
	body.mass = mass;
	body.inverseMass = mass == 0.0f ? 0.0f : 1.0f / mass;
	body.isStatic = mass == 0.0f;

	if ( body.isStatic ) {
		uf::physics::impl::setColliderCategory(body, "STATIC");
		uf::physics::impl::setColliderMask(body, "STATIC");
	}

	// insert into world
	world.bodies.emplace_back(&body);

	return body;
}
pod::PhysicsBody& uf::physics::impl::create( pod::World& world, uf::Object& object, const pod::AABB& aabb, float mass, const pod::Vector3f& offset ) {
	auto& body = uf::physics::impl::create( world, object, mass, offset );
	body.collider.type = pod::ShapeType::AABB;
	body.collider.u.aabb = aabb;
	body.bounds = ::computeAABB( body );
	uf::physics::impl::updateInertia( body );
	return body;
}
pod::PhysicsBody& uf::physics::impl::create( pod::World& world, uf::Object& object, const pod::Sphere& sphere, float mass, const pod::Vector3f& offset ) {
	auto& body = uf::physics::impl::create( world, object, mass, offset );
	body.collider.type = pod::ShapeType::SPHERE;
	body.collider.u.sphere = sphere;
	body.bounds = ::computeAABB( body );
	uf::physics::impl::updateInertia( body );
	return body;
}
pod::PhysicsBody& uf::physics::impl::create( pod::World& world, uf::Object& object, const pod::Plane& plane, float mass, const pod::Vector3f& offset ) {
	auto& body = uf::physics::impl::create( world, object, mass, offset );
	body.collider.type = pod::ShapeType::PLANE;
	body.collider.u.plane = plane;
	body.bounds = ::computeAABB( body );
	uf::physics::impl::updateInertia( body );
	return body;
}
pod::PhysicsBody& uf::physics::impl::create( pod::World& world, uf::Object& object, const pod::Capsule& capsule, float mass, const pod::Vector3f& offset ) {
	auto& body = uf::physics::impl::create( world, object, mass, offset );
	body.collider.type = pod::ShapeType::CAPSULE;
	body.collider.u.capsule = capsule;
	body.bounds = ::computeAABB( body );
	uf::physics::impl::updateInertia( body );
	return body;
}
pod::PhysicsBody& uf::physics::impl::create( pod::World& world, uf::Object& object, const pod::TriangleWithNormal& tri, float mass, const pod::Vector3f& offset ) {
	auto& body = uf::physics::impl::create( world, object, mass, offset );
	body.collider.type = pod::ShapeType::TRIANGLE;
	body.collider.u.triangle = tri;
	body.bounds = ::computeAABB( body );
	uf::physics::impl::updateInertia( body );
	return body;
}
pod::PhysicsBody& uf::physics::impl::create( pod::World& world, uf::Object& object, const uf::Mesh& mesh, float mass, const pod::Vector3f& offset ) {
	auto& body = uf::physics::impl::create( world, object, mass, offset );
	body.collider.type = pod::ShapeType::MESH;
	body.collider.u.mesh.mesh = &mesh;
	body.collider.u.mesh.bvh = new pod::BVH;

	::buildMeshBVH( *body.collider.u.mesh.bvh, mesh, ::meshBvhCapacity );

	body.bounds = ::computeAABB( body );
	uf::physics::impl::updateInertia( body );
	return body;
}

pod::PhysicsBody& uf::physics::impl::create( uf::Object& object, float mass, const pod::Vector3f& offset ) {
	// bind to scene
	// auto& root = object.getRootParent<>(); // in the event a scene is being initialized that is not the root scene, use the root parent instead
	// auto& world = root.getComponent<pod::World>();
	auto& scene = uf::scene::getCurrentScene();
	auto& world = scene.getComponent<pod::World>();
	return create( world, object, mass, offset );
	
}
pod::PhysicsBody& uf::physics::impl::create( uf::Object& object, const pod::AABB& aabb, float mass, const pod::Vector3f& offset ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& world = scene.getComponent<pod::World>();
	return create( world, object, aabb, mass, offset );
}
pod::PhysicsBody& uf::physics::impl::create( uf::Object& object, const pod::Sphere& sphere, float mass, const pod::Vector3f& offset ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& world = scene.getComponent<pod::World>();
	return create( world, object, sphere, mass, offset );
}
pod::PhysicsBody& uf::physics::impl::create( uf::Object& object, const pod::Plane& plane, float mass, const pod::Vector3f& offset ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& world = scene.getComponent<pod::World>();
	return create( world, object, plane, mass, offset );
}
pod::PhysicsBody& uf::physics::impl::create( uf::Object& object, const pod::Capsule& capsule, float mass, const pod::Vector3f& offset ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& world = scene.getComponent<pod::World>();
	return create( world, object, capsule, mass, offset );
}
pod::PhysicsBody& uf::physics::impl::create( uf::Object& object, const uf::Mesh& mesh, float mass, const pod::Vector3f& offset ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& world = scene.getComponent<pod::World>();
	return create( world, object, mesh, mass, offset );
}

void uf::physics::impl::destroy( uf::Object& object ) {
	if ( !object.hasComponent<pod::PhysicsBody>() ) return;
	
	return destroy( object.getComponent<pod::PhysicsBody>() );
}
void uf::physics::impl::destroy( pod::PhysicsBody& body ) {
	auto& world = *body.world;
	// remove from world
	for ( auto it = world.bodies.begin(); it != world.bodies.end(); ++it ) {
		if ( (*it)->object != body.object ) continue;
		world.bodies.erase(it);
		break;
	}

	// remove any pointered collider data
	if ( body.collider.type == pod::ShapeType::MESH ) {
		if ( body.collider.u.mesh.bvh ) delete body.collider.u.mesh.bvh;
	}
}

pod::RayQuery uf::physics::impl::rayCast( const pod::Ray& ray, const pod::PhysicsBody& body, float maxDistance ) {
	return rayCast( ray, *body.world, &body, maxDistance );
}
pod::RayQuery uf::physics::impl::rayCast( const pod::Ray& ray, const pod::World& world, float maxDistance ) {
	return rayCast( ray, world, NULL, maxDistance );
}
pod::RayQuery uf::physics::impl::rayCast( const pod::Ray& ray, const pod::World& world, const pod::PhysicsBody* body, float maxDistance ) {
	pod::RayQuery rayHit;
	rayHit.contact.penetration = maxDistance;

	auto& bvh = world.bvh;
	auto& bodies = world.bodies;

	uf::stl::vector<int> candidates;
	::queryBVH( bvh, ray, candidates );

	for ( auto i : candidates ) {
		auto* b = bodies[i];
		if ( body == b ) continue;
		switch ( b->collider.type ) {
			case pod::ShapeType::AABB: rayAabb( ray, *b, rayHit ); break;
			case pod::ShapeType::SPHERE: raySphere( ray, *b, rayHit ); break;
			case pod::ShapeType::PLANE: rayPlane( ray, *b, rayHit ); break;
			case pod::ShapeType::CAPSULE: rayCapsule( ray, *b, rayHit ); break;
			case pod::ShapeType::MESH: rayMesh( ray, *b, rayHit ); break;
		}
	}

	return rayHit;
}

#if UF_PHYSICS_TEST
	#include "tests.inl"
#endif
#endif