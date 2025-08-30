#if !UF_USE_REACTPHYSICS
#include <uf/utils/math/physics/impl.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/utils/memory/stack.h>

namespace {
	bool warmupSolver = false;
	bool blockContactSolver = false;
	bool useGjk = false;
	bool fixedStep = true;
	int substeps = 0;

	int solverIterations = 20;
	float baumgarteCorrectionPercent = 0.4f;
	float baumgarteCorrectionSlop = 0.01f;
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
#include "mesh.inl"
#include "ray.inl"
#include "bvh.inl"
#include "gjk.inl"
#include "epa.inl"
#include "integration.inl"

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
	::buildBroadphaseBVH(bvh, bodies);
	// query for overlaps
	::queryOverlaps( bvh, pairs );
	// iterate overlaps
	for ( auto& [ia, ib] : pairs ) {
		auto& a = *bodies[ia];
		auto& b = *bodies[ib];

		pod::Manifold manifold;
		if ( ::generateContacts( a, b, manifold, dt ) ) {
			// bodies with meshes already reorient the normal to the triangle's center
			// do not do it for meshes because it'll reorient to the mesh's origin
			if ( a.collider.type != pod::ShapeType::MESH && b.collider.type != pod::ShapeType::MESH ) {
				for ( auto& c : manifold.points ) c.normal = ::orientNormalToAB( a, b, c.normal );
			}
			// retrieve accumulated impulses
			if ( ::warmupSolver )  ::retrieveContacts( manifold, ::manifoldsCache[::makePairKey( a, b )] );
			// keep four most important contacts
			::reduceContacts( manifold );
		#if 0
			UF_MSG_DEBUG("body a={}, body b={}, manifold size={}", uf::vector::toString(a.transform->position), uf::vector::toString(b.transform->position), manifold.points.size());
			for ( auto& contact : manifold.points ) {
				UF_MSG_DEBUG("contact={}, normal={}, depth={}", uf::vector::toString( contact.point ), uf::vector::toString( contact.normal ), contact.penetration );
			}
		#endif

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

void uf::physics::impl::setMass( pod::RigidBody& body, float mass ) {
	body.mass = mass;
	body.inverseMass = 1.0f / mass;
	setInertia( body );
}
void uf::physics::impl::setInertia( pod::RigidBody& body ) {
	if ( body.isStatic || body.mass <= 0 ) {
		body.inverseInertiaTensor = {};
		return;
	}

	// to-do: make this a flag or something
	if ( body.collider.type == pod::ShapeType::CAPSULE ) {
		body.inertiaTensor = { FLT_MAX, FLT_MAX, FLT_MAX };
		body.inverseInertiaTensor = { 0.0f, 0.0f, 0.0f };

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
void uf::physics::impl::applyForce( pod::RigidBody& body, const pod::Vector3f& force ) {
	if ( body.isStatic ) return;
	body.forceAccumulator += force;
}
void uf::physics::impl::applyForceAtPoint( pod::RigidBody body, const pod::Vector3f& force, const pod::Vector3f& point ) {
	if ( body.isStatic ) return;
	// linear force
	body.forceAccumulator += force;
	// angular force
	pod::Vector3f r = point - body.transform->position;
	body.torqueAccumulator += uf::vector::cross( r, force );
}
void uf::physics::impl::applyImpulse( pod::RigidBody& body, const pod::Vector3f& impulse ) {
	if ( body.isStatic ) return;
	body.velocity += impulse * body.inverseMass;
}
void uf::physics::impl::applyTorque( pod::RigidBody& body, const pod::Vector3f& torque ) {
	if ( body.isStatic ) return;
	body.torqueAccumulator += torque;
}
void uf::physics::impl::setVelocity( pod::RigidBody& body, const pod::Vector3f& v ) {
	body.velocity = v;
}
void uf::physics::impl::applyRotation( pod::RigidBody& body, const pod::Quaternion<>& q ) {
	uf::transform::rotate( *body.transform, q );
}
void uf::physics::impl::applyRotation( pod::RigidBody& body, const pod::Vector3f& axis, float angle ) {
	applyRotation( body, uf::quaternion::axisAngle( axis, angle ) );
}

// body creation
pod::RigidBody& uf::physics::impl::create( pod::World& world, uf::Object& object, float mass ) {
	// bind to component
	pod::RigidBody& body = object.getComponent<pod::RigidBody>();
	// initial initialization
	body.world = &world;
	body.object = &object;
	body.transform = &object.getComponent<pod::Transform<>>();
	body.mass = mass;
	body.inverseMass = 1.0f / mass;
	body.isStatic = mass == 0.0f;

	// insert into world
	world.bodies.emplace_back(&body);

	return body;
}
pod::RigidBody& uf::physics::impl::create( pod::World& world, uf::Object& object, const pod::AABB& aabb, float mass ) {
	auto& body = create( world, object, mass );
	body.collider.type = pod::ShapeType::AABB;
	body.collider.u.aabb = aabb;
	body.bounds = ::computeAABB( body );
	setInertia( body );
	UF_MSG_DEBUG("Creating body of type: {} | mass: {} | min: {} | max: {}", "AABB", mass, uf::vector::toString(aabb.min), uf::vector::toString(aabb.max) );
	return body;
}
pod::RigidBody& uf::physics::impl::create( pod::World& world, uf::Object& object, const pod::Sphere& sphere, float mass ) {
	auto& body = create( world, object, mass );
	body.collider.type = pod::ShapeType::SPHERE;
	body.collider.u.sphere = sphere;
	body.bounds = ::computeAABB( body );
	setInertia( body );
	UF_MSG_DEBUG("Creating body of type: {} | mass: {} | radius: {}", "SPHERE", mass, sphere.radius );
	return body;
}
pod::RigidBody& uf::physics::impl::create( pod::World& world, uf::Object& object, const pod::Plane& plane, float mass ) {
	auto& body = create( world, object, mass );
	body.collider.type = pod::ShapeType::PLANE;
	body.collider.u.plane = plane;
	body.bounds = ::computeAABB( body );
	setInertia( body );
	UF_MSG_DEBUG("Creating body of type: {} | mass: {} | normal: {} | offset: {}", "PLANE", mass, uf::vector::toString( plane.normal ), plane.offset );
	return body;
}
pod::RigidBody& uf::physics::impl::create( pod::World& world, uf::Object& object, const pod::Capsule& capsule, float mass ) {
	auto& body = create( world, object, mass );
	body.collider.type = pod::ShapeType::CAPSULE;
	body.collider.u.capsule = capsule;
	body.material.restitution = 0.001f;
	body.bounds = ::computeAABB( body );
	setInertia( body );
	UF_MSG_DEBUG("Creating body of type: {} | mass: {} | radius: {} | height: {}", "CAPSULE", mass, capsule.radius, capsule.halfHeight * 2.0f );
	return body;
}
pod::RigidBody& uf::physics::impl::create( pod::World& world, uf::Object& object, const uf::Mesh& mesh, float mass ) {
	auto& body = create( world, object, mass );
	body.collider.type = pod::ShapeType::MESH;
	body.collider.u.mesh.mesh = &mesh;
	body.collider.u.mesh.bvh = new pod::BVH;

	::buildMeshBVH( *body.collider.u.mesh.bvh, mesh );

	UF_MSG_DEBUG("Built mesh BVH: nodes={} indices={}", body.collider.u.mesh.bvh->nodes.size(), body.collider.u.mesh.bvh->indices.size());

	body.bounds = ::computeAABB( body );
	setInertia( body );
	UF_MSG_DEBUG("Creating body of type: {} | mass: {} ", "MESH", mass );
	return body;
}

pod::RigidBody& uf::physics::impl::create( uf::Object& object, float mass ) {
	// bind to scene
	// auto& root = object.getRootParent<>(); // in the event a scene is being initialized that is not the root scene, use the root parent instead
	// auto& world = root.getComponent<pod::World>();
	auto& scene = uf::scene::getCurrentScene();
	auto& world = scene.getComponent<pod::World>();
	return create( world, object, mass );
	
}
pod::RigidBody& uf::physics::impl::create( uf::Object& object, const pod::AABB& aabb, float mass ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& world = scene.getComponent<pod::World>();
	return create( world, object, aabb, mass );
}
pod::RigidBody& uf::physics::impl::create( uf::Object& object, const pod::Sphere& sphere, float mass ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& world = scene.getComponent<pod::World>();
	return create( world, object, sphere, mass );
}
pod::RigidBody& uf::physics::impl::create( uf::Object& object, const pod::Plane& plane, float mass ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& world = scene.getComponent<pod::World>();
	return create( world, object, plane, mass );
}
pod::RigidBody& uf::physics::impl::create( uf::Object& object, const pod::Capsule& capsule, float mass ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& world = scene.getComponent<pod::World>();
	return create( world, object, capsule, mass );
}
pod::RigidBody& uf::physics::impl::create( uf::Object& object, const uf::Mesh& mesh, float mass ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& world = scene.getComponent<pod::World>();
	return create( world, object, mesh, mass );
}

void uf::physics::impl::destroy( uf::Object& object ) {
	if ( !object.hasComponent<pod::RigidBody>() ) return;
	
	return destroy( object.getComponent<pod::RigidBody>() );
}
void uf::physics::impl::destroy( pod::RigidBody& body ) {
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

pod::RayQuery uf::physics::impl::rayCast( const pod::Ray& ray, const pod::RigidBody& body, float maxDistance ) {
	return rayCast( ray, *body.world, &body, maxDistance );
}
pod::RayQuery uf::physics::impl::rayCast( const pod::Ray& ray, const pod::World& world, float maxDistance ) {
	return rayCast( ray, world, NULL, maxDistance );
}
pod::RayQuery uf::physics::impl::rayCast( const pod::Ray& ray, const pod::World& world, const pod::RigidBody* body, float maxDistance ) {
	pod::RayQuery rayHit;
	rayHit.contact.penetration = maxDistance;

	auto& bvh = world.bvh;
	auto& bodies = world.bodies;

#if 1
	uf::stl::vector<int> candidates;
	::queryBVH( bvh, ray, candidates );

	for ( auto i : candidates ) {
#else
	for ( auto i = 0; i < bodies.size(); ++i ) {
#endif
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