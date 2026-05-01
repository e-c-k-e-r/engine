#if !UF_USE_REACTPHYSICS
#include <uf/utils/math/physics/impl.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/utils/memory/stack.h>

#define EPS 1.0e-6f
#define EPS2 (EPS * EPS)
#define ASSERT_COLLIDER_TYPES( A, B ) UF_ASSERT( a.collider.type == pod::ShapeType::A && b.collider.type == pod::ShapeType::B );
#define UF_PHYSICS_TEST 0

#include "helpers.inl"
#include "aabb.inl"
#include "sphere.inl"
#include "plane.inl"
#include "capsule.inl"
#include "triangle.inl"
#include "mesh.inl"
#include "convexHull.inl"
#include "ray.inl"
#include "bvh.inl"
#include "gjk.inl"
#include "epa.inl"
#include "integration.inl"
#include "solvers.inl"

pod::PhysicsSettings uf::physics::impl::settings;

float uf::physics::impl::timescale = 1.0f / 60.0f;
bool uf::physics::impl::async = false;

// unused, as these are from reactphysics
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
	if ( !uf::physics::impl::settings.fixedStep ) {
		if ( uf::physics::impl::settings.substeps > 0 ) uf::physics::impl::substep( world, dt, uf::physics::impl::settings.substeps );
		else uf::physics::impl::step( world, dt );

		return;
	}

	static float accumulator = 0;
	accumulator += dt; 
	while ( accumulator >= uf::physics::impl::timescale ) { 
		if ( uf::physics::impl::settings.substeps > 0 ) uf::physics::impl::substep( world, uf::physics::impl::timescale, uf::physics::impl::settings.substeps ); 
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

	++uf::physics::impl::settings.frameCounter;

	for ( auto* body : bodies ) {
		::integrate( *body, dt );
	}

	// rebuild static bvh if dirty
	if ( staticBvh.dirty && uf::physics::impl::settings.useSplitBvhs ) {
		::buildBroadphaseBVH( staticBvh, bodies, uf::physics::impl::settings.broadphaseBvhCapacity, uf::physics::impl::settings.useSplitBvhs, true ); // (re)build
	}

	switch ( ::decideBVHUpdate( dynamicBvh, bodies, uf::physics::impl::settings.bvhUpdatePolicy, uf::physics::impl::settings.frameCounter ) ) {
		case pod::BVH::UpdatePolicy::Decision::REBUILD: {
			::buildBroadphaseBVH( dynamicBvh, bodies, uf::physics::impl::settings.broadphaseBvhCapacity, uf::physics::impl::settings.useSplitBvhs, false ); // (re)build
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
	if ( uf::physics::impl::settings.useSplitBvhs ) {
		::queryOverlaps( dynamicBvh, staticBvh, pairs );
	}

	// build islands from overlaps
	uf::stl::vector<pod::Island> islands;
	::buildIslands( pairs, bodies, islands );

	if ( uf::physics::impl::settings.warmupSolver ) ::prepareManifoldCache( uf::physics::impl::settings.manifoldsCache, islands, bodies );

	// iterate islands
	#pragma omp parallel for schedule(dynamic)
	for ( auto& island : islands ) {
		static thread_local uf::stl::vector<pod::Manifold> manifolds;
		manifolds.clear();
		manifolds.reserve(uf::physics::impl::settings.reserveCount);

		// sleeping island, skip (asleep islands shouldn't ever be in here)
		if ( !island.awake ) continue;

		// iterate overlap pairs
		for ( auto& [ ia, ib ] : island.pairs ) {
			auto& a = *bodies[ia];
			auto& b = *bodies[ib];

			pod::Manifold manifold;
			// did not collide
			if ( !::generateContacts( a, b, manifold, dt ) ) continue;

			// compute local points (for reprojection)
			::computeLocalContacts( manifold );

			// bodies with meshes already reorient the normal to the triangle's center
			// do not do it for meshes because it'll reorient to the mesh's origin
			// do not do it for planes
			bool shouldReorient = true;
			if ( a.collider.type == pod::ShapeType::MESH || b.collider.type == pod::ShapeType::MESH ) shouldReorient = false;
			//if ( a.collider.type == pod::ShapeType::PLANE || b.collider.type == pod::ShapeType::PLANE ) shouldReorient = false;
			if ( shouldReorient ) {
				for ( auto& c : manifold.points ) c.normal = ::orientNormalToAB( a, b, c.normal );
			}
			// retrieve accumulated impulses
			if ( uf::physics::impl::settings.warmupSolver ) {
				auto it = uf::physics::impl::settings.manifoldsCache.find( ::makePairKey( a, b ) );
				if ( it != uf::physics::impl::settings.manifoldsCache.end() ) ::retrieveContacts( manifold, it->second );
			}
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
			// mark as grounded
			for ( auto& c : manifold.points ) {
				if ( std::fabs(uf::vector::dot(c.normal, pod::Vector3f{0,1,0})) > uf::physics::impl::settings.groundedThreshold ) {
					// only mark if contact point is below body
					if ( c.point.y < ::getPosition(a).y ) a.activity.grounded = true;
					if ( c.point.y < ::getPosition(b).y ) b.activity.grounded = true;
				}
			}

			// store manifold
			manifolds.emplace_back(manifold);
		}

		// pass manifolds to solver
		::solveContacts( manifolds, dt );
		// do position correction
		// ::solvePositions( manifolds, dt );
		// cache manifold positions
		if ( uf::physics::impl::settings.warmupSolver ) {
			::updateManifoldCache( manifolds, uf::physics::impl::settings.manifoldsCache );
		}
	}

	if ( uf::physics::impl::settings.warmupSolver ) ::pruneManifoldCache( uf::physics::impl::settings.manifoldsCache );

	for ( auto* b : bodies ) {
		if ( b->isStatic ) continue;
		::snapVelocity( *b, dt );
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
		body.inertiaTensor = { FLT_MAX, FLT_MAX, FLT_MAX };
		body.inverseInertiaTensor = { 0.0f, 0.0f, 0.0f };
		return;
	}

	switch ( body.collider.type ) {
		case pod::ShapeType::AABB: {
			pod::Vector3f dims = (body.collider.aabb.max - body.collider.aabb.min);
			pod::Vector3f dimsSq = dims * dims;
			body.inertiaTensor = pod::Vector3f{ dimsSq.y + dimsSq.z, dimsSq.x + dimsSq.z, dimsSq.x + dimsSq.y } * (body.mass / 12.0f);
			body.inertiaTensor = uf::vector::max( body.inertiaTensor, { EPS, EPS, EPS } );
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
		case pod::ShapeType::MESH:
		case pod::ShapeType::CONVEX_HULL: {
			const auto& bvh = *body.collider.mesh.bvh;

			pod::Matrix3f inertia = {};
			float totalVolume = 0.0f;

			// compute total volume
			for ( size_t i = 0; i < bvh.nodes.size(); ++i ) {
				if ( bvh.nodes[i].getCount() == 0 ) continue;
				const auto& box = bvh.bounds[i];

				auto extents = box.max - box.min;
				totalVolume += extents.x * extents.y * extents.z;
			}

			if ( totalVolume < EPS ) {
				body.inertiaTensor = { FLT_MAX, FLT_MAX, FLT_MAX };
				body.inverseInertiaTensor = { 0.0f, 0.0f, 0.0f };
			} else {
				// accumulate inertia
				for ( size_t i = 0; i < bvh.nodes.size(); ++i ) {
					if ( bvh.nodes[i].getCount() == 0 ) continue;
					const auto& box = bvh.bounds[i];

					auto extents = box.max - box.min;
					float mass = body.mass * extents.x * extents.y * extents.z / totalVolume;

					// inertia tensor of a box about its center
					float x2 = extents.x * extents.x;
					float y2 = extents.y * extents.y;
					float z2 = extents.z * extents.z;

					pod::Matrix3f Ibox;
					Ibox(0,0) = (1.0f/12.0f) * mass * (y2 + z2);
					Ibox(1,1) = (1.0f/12.0f) * mass * (x2 + z2);
					Ibox(2,2) = (1.0f/12.0f) * mass * (x2 + y2);

					// parallel axis theorem
					pod::Vector3f center = (box.min + box.max) * 0.5f;
					pod::Vector3f d = center; // relative to mesh COM (assume COM at origin for now)
					float dist2 = uf::vector::magnitude( d );

					pod::Matrix3f pat = uf::matrix::identityi<pod::Matrix3f>() * (mass * dist2);
					pat -= uf::matrix::outerProduct(d, d) * mass;

					inertia += Ibox + pat;
				}
				
				body.inertiaTensor = { inertia(0,0), inertia(1,1), inertia(2,2) };
				body.inverseInertiaTensor = 1.0f / body.inertiaTensor;
			}
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
void uf::physics::impl::applyForceAtPoint( pod::PhysicsBody& body, const pod::Vector3f& force, const pod::Vector3f& point ) {
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
pod::PhysicsBody& uf::physics::impl::create( pod::World& world, uf::Object& object, const uf::Mesh& mesh, float mass, const pod::Vector3f& offset, bool convex ) {
	auto& body = uf::physics::impl::create( world, object, mass, offset );
	if ( !convex ) {
		body.collider.type = pod::ShapeType::MESH;
		body.collider.mesh.mesh = &mesh;
		body.collider.mesh.bvh = new pod::BVH;
		
		auto& bvh = *body.collider.mesh.bvh;
		::buildMeshBVH( bvh, mesh, uf::physics::impl::settings.meshBvhCapacity );
	} else {
		body.collider.type = pod::ShapeType::CONVEX_HULL;
		body.collider.convexHull.mesh = &mesh;
		body.collider.convexHull.bvh = new pod::BVH;
		
		auto& bvh = *body.collider.convexHull.bvh;
		::buildConvexHullBVH( bvh, mesh/*, uf::physics::impl::settings.meshBvhCapacity*/ );
	}

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
pod::PhysicsBody& uf::physics::impl::create( uf::Object& object, const uf::Mesh& mesh, float mass, const pod::Vector3f& offset, bool convex ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& world = scene.getComponent<pod::World>();
	return create( world, object, mesh, mass, offset, convex );
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

	static thread_local uf::stl::vector<pod::BVH::index_t> candidates;
	candidates.clear();
	::queryBVH( dynamicBvh, ray, candidates );
	if ( uf::physics::impl::settings.useSplitBvhs ) ::queryBVH( staticBvh, ray, candidates );

	for ( auto i : candidates ) {
		auto* b = bodies[i];
		
		if ( body == b ) continue;

		switch ( b->collider.type ) {
			case pod::ShapeType::AABB: rayAabb( ray, *b, rayHit ); break;
			case pod::ShapeType::SPHERE: raySphere( ray, *b, rayHit ); break;
			case pod::ShapeType::PLANE: rayPlane( ray, *b, rayHit ); break;
			case pod::ShapeType::CAPSULE: rayCapsule( ray, *b, rayHit ); break;
			case pod::ShapeType::MESH: rayMesh( ray, *b, rayHit ); break;
			case pod::ShapeType::CONVEX_HULL: rayHull( ray, *b, rayHit ); break;
		}
	}

	return rayHit;
}

#if UF_PHYSICS_TEST
	#include "tests.inl"
#endif
#endif