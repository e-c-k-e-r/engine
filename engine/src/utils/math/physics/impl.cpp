#if !UF_USE_REACTPHYSICS
#include <uf/utils/math/physics/impl.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/utils/memory/stack.h>

namespace {
	bool warmupSolver = true; // cache manifold data to warm up the solver
	bool blockContactSolver = true; // use BlockNxN solvers (where N = number of contacts for a manifold)
	bool psgContactSolver = true; // use PSG contact solver
	bool useGjk = false; // currently don't have a way to broadphase mesh => narrowphase tri via GJK
	bool fixedStep = false; // run physics simulation with a fixed delta time (with accumulation), rather than rely on actual engine deltatime
	uint32_t substeps = 4; // number of substeps per frame tick
	uint32_t reserveCount = 32; // amount of elements to reserve for vectors used in this system, to-do: have it tie to a memory pool allocator

	// increasing these make things lag for reasons I can imagine why
	uint32_t broadphaseBvhCapacity = 4; // number of bodies per leaf node
	uint32_t meshBvhCapacity = 4; // number of triangles per leaf node

	// additionally flattens a BVH for linear iteration, rather than a recursive / stack-based traversal
	bool flattenBvhBodies = true;
	bool flattenBvhMeshes = true;
	
	// use surface area heuristics for building the BVH, rather than naive splits
	bool useBvhSahBodies = true; // it actually seems slower to use these......
	bool useBvhSahMeshes = true;

	bool useSplitBvhs = true; // creates separate BVHs for static / dynamic objects

	// to-do: find possibly better values for this
	uint32_t solverIterations = 10;
	float baumgarteCorrectionPercent = 0.2f;
	float baumgarteCorrectionSlop = 0.01f;
	
	uf::stl::unordered_map<size_t, pod::Manifold> manifoldsCache;
	uint32_t manifoldCacheLifetime = 6; // to-do: find a good value for this

	uint32_t frameCounter = 0;

	// to-do: tweak this to not be annoying
	pod::BVH::UpdatePolicy bvhUpdatePolicy = {
		.displacementThreshold = 0.25f,
		.overlapThreshold = 2.0f,
		.dirtyRatioThreshold = 0.3f,
		.maxFramesBeforeRebuild = 60 * 10, // 10 seconds
	};
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

// unused, as these are from reactphysics
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
void uf::physics::impl::substep( pod::World& world, float dt, int32_t substeps ) {
	float h = dt / substeps;
	for ( auto i = 0; i < substeps; ++i ) {
		uf::physics::impl::step( world, h );
	}
}
void uf::physics::impl::step( pod::World& world, float dt ) {
	auto& bodies = world.bodies;
	auto& dynamicBvh = world.dynamicBvh;
	auto& staticBvh = world.staticBvh;

	if ( bodies.empty() ) return;

	++::frameCounter;

	for ( auto* body : bodies ) {
		if ( !body->activity.awake ) continue;
		::integrate( *body, dt );
	}

	// rebuild static bvh if dirty
	if ( staticBvh.dirty && ::useSplitBvhs ) {
		::buildBroadphaseBVH( staticBvh, bodies, ::broadphaseBvhCapacity, ::useSplitBvhs, true ); // (re)build
	}

	switch ( ::decideBVHUpdate( dynamicBvh, bodies, ::bvhUpdatePolicy, ::frameCounter ) ) {
		case pod::BVH::UpdatePolicy::Decision::REBUILD: {
			::buildBroadphaseBVH( dynamicBvh, bodies, ::broadphaseBvhCapacity, ::useSplitBvhs, false ); // (re)build
		} break;
		case pod::BVH::UpdatePolicy::Decision::REFIT: {
			::refitBVH( dynamicBvh, bodies ); // refit
		} break;
		case pod::BVH::UpdatePolicy::Decision::NONE:
		default: {
			// no-op
		} break;
	}

	// query for overlaps
	pod::BVH::pairs_t pairs;
	::queryOverlaps( dynamicBvh, pairs );
	if ( ::useSplitBvhs ) {
		::queryOverlaps( dynamicBvh, staticBvh, pairs );
	}

	// build islands from overlaps
	uf::stl::vector<pod::Island> islands;
	::buildIslands( pairs, bodies, islands );

	// iterate islands
	#pragma omp parallel for schedule(dynamic)
	for ( auto& island : islands ) {
		uf::stl::vector<pod::Manifold> manifolds;
		manifolds.reserve(::reserveCount);

		// sleeping island, skip
		if ( !::updateIsland( island, bodies, dt ) ) continue;
		// iterate overlap pairs
		for ( auto& [ ia, ib ] : island.pairs ) {
			auto& a = *bodies[ia];
			auto& b = *bodies[ib];

			pod::Manifold manifold;
			// did not collide
			if ( !::generateContacts( a, b, manifold, dt ) ) continue;

			// bodies with meshes already reorient the normal to the triangle's center
			// do not do it for meshes because it'll reorient to the mesh's origin
			if ( a.collider.type != pod::ShapeType::MESH && b.collider.type != pod::ShapeType::MESH ) {
				for ( auto& c : manifold.points ) c.normal = ::orientNormalToAB( a, b, c.normal );
			}
			// retrieve accumulated impulses
			if ( ::warmupSolver ) ::retrieveContacts( manifold, ::manifoldsCache[::makePairKey( a, b )] );
			// merge similar contacts from a mesh to ensure continuity
			if ( a.collider.type == pod::ShapeType::MESH || b.collider.type == pod::ShapeType::MESH ) {
				::mergeContacts( manifold );
			}
			// keep four most important contacts
			::reduceContacts( manifold );
			// no points remained, skip
			if ( manifold.points.empty() ) continue;
			// wake up bodies
			if ( a.activity.awake && !b.activity.awake ) ::wakeBody( b );
			if ( b.activity.awake && !a.activity.awake ) ::wakeBody( a );

			// store manifold
			manifolds.emplace_back(manifold);
		}

		// pass manifolds to solver
		::solveContacts( manifolds, dt );
		// do position correction
		::solvePositions( manifolds, dt );
		// cache manifold positions
		if ( ::warmupSolver ) ::storeManifolds( manifolds, ::manifoldsCache );
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
			pod::Vector3f extents = (body.collider.aabb.max - body.collider.aabb.min);
			extents *= extents; // square it;

			body.inertiaTensor = extents * (body.mass / 12.0f);
			body.inverseInertiaTensor = 1.0f / body.inertiaTensor;
		} break;
		case pod::ShapeType::SPHERE: {
			float I = 0.4f * body.mass * body.collider.sphere.radius * body.collider.sphere.radius;
			float invI = 1.0f / I;
			body.inertiaTensor = { I, I, I };
			body.inverseInertiaTensor = { invI, invI, invI };
		} break;
		case pod::ShapeType::CAPSULE: {
			float r = body.collider.capsule.radius;
			float h = body.collider.capsule.halfHeight * 2.0f; // full cyl height
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
	if ( body.isStatic ) return; ::wakeBody( body );
	body.forceAccumulator += force;
}
void uf::physics::impl::applyForceAtPoint( pod::PhysicsBody body, const pod::Vector3f& force, const pod::Vector3f& point ) {
	if ( body.isStatic ) return; ::wakeBody( body );
	// linear force
	body.forceAccumulator += force;
	// angular force
	pod::Vector3f r = point - ::getPosition( body );
	body.torqueAccumulator += uf::vector::cross( r, force );
}
void uf::physics::impl::applyImpulse( pod::PhysicsBody& body, const pod::Vector3f& impulse ) {
	if ( body.isStatic ) return; ::wakeBody( body );
	body.velocity += impulse * body.inverseMass;
}
void uf::physics::impl::applyTorque( pod::PhysicsBody& body, const pod::Vector3f& torque ) {
	if ( body.isStatic ) return; ::wakeBody( body );
	body.torqueAccumulator += torque;
}
void uf::physics::impl::setVelocity( pod::PhysicsBody& body, const pod::Vector3f& v ) {
	::wakeBody( body );
	body.velocity = v;
}
void uf::physics::impl::applyRotation( pod::PhysicsBody& body, const pod::Quaternion<>& q ) {
	::wakeBody( body );
	uf::transform::rotate( *body.transform/*.reference*/, q );
}
void uf::physics::impl::applyRotation( pod::PhysicsBody& body, const pod::Vector3f& axis, float angle ) {
	uf::physics::impl::applyRotation( body, uf::quaternion::axisAngle( axis, angle ) );
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
		world.staticBvh.dirty = true; // mark as dirty
	} else {
		uf::physics::impl::setColliderCategory(body, "DYNAMIC");
		uf::physics::impl::setColliderMask(body, "DYNAMIC");
		world.dynamicBvh.dirty = true; // mark as dirty
	}

	world.bodies.emplace_back(&body); // insert into world

	return body;
}
pod::PhysicsBody& uf::physics::impl::create( pod::World& world, uf::Object& object, const pod::AABB& aabb, float mass, const pod::Vector3f& offset ) {
	auto& body = uf::physics::impl::create( world, object, mass, offset );
	body.collider.type = pod::ShapeType::AABB;
	body.collider.aabb = aabb;
	body.bounds = ::computeAABB( body );
	uf::physics::impl::updateInertia( body );
	return body;
}
pod::PhysicsBody& uf::physics::impl::create( pod::World& world, uf::Object& object, const pod::Sphere& sphere, float mass, const pod::Vector3f& offset ) {
	auto& body = uf::physics::impl::create( world, object, mass, offset );
	body.collider.type = pod::ShapeType::SPHERE;
	body.collider.sphere = sphere;
	body.bounds = ::computeAABB( body );
	uf::physics::impl::updateInertia( body );
	return body;
}
pod::PhysicsBody& uf::physics::impl::create( pod::World& world, uf::Object& object, const pod::Plane& plane, float mass, const pod::Vector3f& offset ) {
	auto& body = uf::physics::impl::create( world, object, mass, offset );
	body.collider.type = pod::ShapeType::PLANE;
	body.collider.plane = plane;
	body.bounds = ::computeAABB( body );
	uf::physics::impl::updateInertia( body );
	return body;
}
pod::PhysicsBody& uf::physics::impl::create( pod::World& world, uf::Object& object, const pod::Capsule& capsule, float mass, const pod::Vector3f& offset ) {
	auto& body = uf::physics::impl::create( world, object, mass, offset );
	body.collider.type = pod::ShapeType::CAPSULE;
	body.collider.capsule = capsule;
	body.bounds = ::computeAABB( body );
	uf::physics::impl::updateInertia( body );
	return body;
}
pod::PhysicsBody& uf::physics::impl::create( pod::World& world, uf::Object& object, const pod::TriangleWithNormal& tri, float mass, const pod::Vector3f& offset ) {
	auto& body = uf::physics::impl::create( world, object, mass, offset );
	body.collider.type = pod::ShapeType::TRIANGLE;
	body.collider.triangle = tri;
	body.bounds = ::computeAABB( body );
	uf::physics::impl::updateInertia( body );
	return body;
}
pod::PhysicsBody& uf::physics::impl::create( pod::World& world, uf::Object& object, const uf::Mesh& mesh, float mass, const pod::Vector3f& offset ) {
	auto& body = uf::physics::impl::create( world, object, mass, offset );
	body.collider.type = pod::ShapeType::MESH;
	body.collider.mesh.mesh = &mesh;

	body.collider.mesh.bvh = new pod::BVH;
	auto& bvh = *body.collider.mesh.bvh;
	::buildMeshBVH( bvh, mesh, ::meshBvhCapacity );

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
		if ( body.collider.mesh.bvh ) delete body.collider.mesh.bvh;
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

	auto& dynamicBvh = world.dynamicBvh;
	auto& staticBvh = world.staticBvh;
	auto& bodies = world.bodies;

	thread_local uf::stl::vector<pod::BVH::index_t> candidates;
	candidates.clear();
	::queryBVH( dynamicBvh, ray, candidates );
	if ( ::useSplitBvhs ) ::queryBVH( staticBvh, ray, candidates );

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