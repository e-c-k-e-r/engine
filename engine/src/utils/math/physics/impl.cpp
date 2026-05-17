#include <uf/utils/math/physics/impl.h>
#include <uf/engine/scene/scene.h>

#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/narrowphase.h>
#include <uf/utils/math/physics/broadphase.h>
#include <uf/utils/math/physics/integration.h>
#include <uf/utils/math/physics/solvers.h>

#define UF_PHYSICS_TEST 0

pod::PhysicsSettings uf::physics::settings;

float uf::physics::timescale = 1.0f / 60.0f;
bool uf::physics::async = false;

// unused, as these are from reactphysics
bool uf::physics::interpolate = false;
bool uf::physics::shared = false;
bool uf::physics::globalStorage = false;

// Bindings
void uf::physics::initialize() {
	uf::physics::initialize( uf::scene::getCurrentScene() );
}
void uf::physics::initialize( uf::Object& object ) {
	uf::physics::initialize( object.getComponent<pod::World>() );
}
void uf::physics::initialize( pod::World& world ) {
	// to-do: any additional initialization (since it's not needed right now)
}
void uf::physics::tick() {
	uf::physics::tick( uf::scene::getCurrentScene() );
}
void uf::physics::tick( uf::Object& scene ) {
	uf::physics::time::previous = uf::physics::time::current;
	uf::physics::time::current = uf::physics::time::timer.elapsed();

	uf::physics::time::delta = uf::physics::time::current - uf::physics::time::previous;
	if ( uf::physics::time::delta > uf::physics::time::clamp ) {
		uf::physics::time::delta = uf::physics::time::clamp;
	}

	if ( uf::physics::async ) {
		uf::thread::queue( "Physics", [&](){ uf::physics::tick( scene, uf::physics::time::delta ); });	
	} else {
		uf::physics::tick( scene, uf::physics::time::delta );
	}
}
void uf::physics::tick( float dt ) {
	uf::physics::tick( uf::scene::getCurrentScene(), dt );
}
void uf::physics::tick( uf::Object& object, float dt ) {
	uf::physics::tick( object.getComponent<pod::World>(), dt );
}
void uf::physics::tick( pod::World& world, float dt ) {
	if ( !uf::physics::settings.fixedStep ) {
		if ( uf::physics::settings.substeps > 0 ) uf::physics::substep( world, dt, uf::physics::settings.substeps );
		else uf::physics::step( world, dt );

		return;
	}

	static float accumulator = 0;
	accumulator += dt; 
	while ( accumulator >= uf::physics::timescale ) { 
		if ( uf::physics::settings.substeps > 0 ) uf::physics::substep( world, uf::physics::timescale, uf::physics::settings.substeps ); 
		else uf::physics::step( world, uf::physics::timescale ); 
		accumulator -= uf::physics::timescale; 
	}
}
void uf::physics::terminate() {
	uf::physics::terminate( uf::scene::getCurrentScene() );
}
void uf::physics::terminate( uf::Object& object ) {
	uf::physics::terminate( object.getComponent<pod::World>() );
}
void uf::physics::terminate( pod::World& world ) {
	world.bodies.clear();
}

// Implementation
void uf::physics::substep( pod::World& world, float dt, int32_t substeps ) {
	float h = dt / substeps;
	for ( auto i = 0; i < substeps; ++i ) {
		uf::physics::step( world, h );
	}
}
void uf::physics::step( pod::World& world, float dt ) {
	auto& bodies = world.bodies;
	auto& dynamicBvh = world.dynamicBvh;
	auto& staticBvh = world.staticBvh;

	if ( bodies.empty() ) return;

	++uf::physics::settings.frameCounter;

	for ( auto* body : bodies ) {
		impl::integrate( *body, dt );
	}

	// rebuild static bvh if dirty
	if ( staticBvh.dirty && uf::physics::settings.useSplitBvhs ) {
		impl::buildBroadphaseBVH( staticBvh, bodies, uf::physics::settings.broadphaseBvhCapacity, uf::physics::settings.useSplitBvhs, true ); // (re)build
	}

	switch ( impl::decideBVHUpdate( dynamicBvh, bodies, uf::physics::settings.bvhUpdatePolicy, uf::physics::settings.frameCounter ) ) {
		case pod::BVH::UpdatePolicy::Decision::REBUILD: {
			impl::buildBroadphaseBVH( dynamicBvh, bodies, uf::physics::settings.broadphaseBvhCapacity, uf::physics::settings.useSplitBvhs, false ); // (re)build
		} break;
		case pod::BVH::UpdatePolicy::Decision::REFIT: {
			impl::refitBVH( dynamicBvh, bodies ); // refit
		} break;
		case pod::BVH::UpdatePolicy::Decision::NONE:
		default: {
			// no-op
		} break;
	}

	// query for overlaps
	pod::BVH::pairs_t pairs;
	impl::queryOverlaps( dynamicBvh, pairs );
	if ( uf::physics::settings.useSplitBvhs ) {
		impl::queryOverlaps( dynamicBvh, staticBvh, pairs );
	}

	// build islands from overlaps
	uf::stl::vector<pod::Island> islands;
	impl::buildIslands( pairs, bodies, islands );

	if ( uf::physics::settings.warmupSolver ) impl::prepareManifoldCache( uf::physics::settings.manifoldsCache, islands, bodies );

	// iterate islands
	//#pragma omp parallel for schedule(dynamic)
	auto tasks = uf::thread::schedule(true);
	for ( auto& island : islands ) tasks.queue([&]{
		STATIC_THREAD_LOCAL(uf::stl::vector<pod::Manifold>, manifolds);
		manifolds.reserve(uf::physics::settings.reserveCount);

		// sleeping island, skip (asleep islands shouldn't ever be in here)
		if ( !island.awake ) return;

		// iterate overlap pairs
		for ( auto& [ ia, ib ] : island.pairs ) {
			auto& a = *bodies[ia];
			auto& b = *bodies[ib];

			pod::Manifold manifold;
			// did not collide
			if ( !impl::generateContacts( a, b, manifold, dt ) ) continue;

			// compute local points (for reprojection)
			impl::computeLocalContacts( manifold );

			// bodies with meshes already reorient the normal to the triangle's center
			// do not do it for meshes because it'll reorient to the mesh's origin
			// do not do it for planes
			bool shouldReorient = true;
			if ( a.collider.type == pod::ShapeType::MESH || b.collider.type == pod::ShapeType::MESH ) shouldReorient = false;
			//if ( a.collider.type == pod::ShapeType::PLANE || b.collider.type == pod::ShapeType::PLANE ) shouldReorient = false;
			if ( shouldReorient ) {
				for ( auto& c : manifold.points ) c.normal = impl::orientNormalToAB( a, b, c.normal );
			}
			// retrieve accumulated impulses
			if ( uf::physics::settings.warmupSolver ) {
				auto it = uf::physics::settings.manifoldsCache.find( impl::makePairKey( a, b ) );
				if ( it != uf::physics::settings.manifoldsCache.end() ) impl::retrieveContacts( manifold, it->second );
			}
			// merge similar contacts from a mesh to ensure continuity
			if ( a.collider.type == pod::ShapeType::MESH || b.collider.type == pod::ShapeType::MESH ) {
				impl::mergeContacts( manifold );
			}
			// keep four most important contacts
			impl::reduceContacts( manifold );
			// no points remained, skip
			if ( manifold.points.empty() ) continue;
			// wake up bodies
			if ( a.activity.awake && !b.activity.awake ) impl::wakeBody( b );
			if ( b.activity.awake && !a.activity.awake ) impl::wakeBody( a );
			// mark as grounded
			for ( auto& c : manifold.points ) {
				if ( std::fabs(uf::vector::dot(c.normal, pod::Vector3f{0,1,0})) > uf::physics::settings.groundedThreshold ) {
					// only mark if contact point is below body
					if ( c.point.y < impl::getPosition(a).y ) a.activity.grounded = true;
					if ( c.point.y < impl::getPosition(b).y ) b.activity.grounded = true;
				}
			}

			// store manifold
			manifolds.emplace_back(manifold);
		}

		// pass manifolds to solver
		impl::solveContacts( manifolds, dt );
		// do position correction
		impl::solvePositions( manifolds, dt );
		// cache manifold positions
		if ( uf::physics::settings.warmupSolver ) {
			impl::updateManifoldCache( manifolds, uf::physics::settings.manifoldsCache );
		}
	});
	uf::thread::execute( tasks );

	if ( uf::physics::settings.warmupSolver ) impl::pruneManifoldCache( uf::physics::settings.manifoldsCache );

	for ( auto* b : bodies ) {
		if ( b->isStatic ) continue;
		impl::snapVelocity( *b, dt );
	}
}

void uf::physics::setMass( pod::PhysicsBody& body, float mass ) {
	body.mass = mass;
	body.inverseMass = 1.0f / mass;
	uf::physics::updateInertia( body );
}

void uf::physics::setColliderCategory( pod::PhysicsBody& body, uint32_t category ) {
	body.collider.category = category;
}
void uf::physics::setColliderCategory( pod::PhysicsBody& body, const uf::stl::string& category ) {
	auto c = uf::string::uppercase( category );
	if ( c == "NONE" ) return uf::physics::setColliderCategory( body, pod::Collider::CATEGORY_NONE );
	if ( c == "STATIC" ) return uf::physics::setColliderCategory( body, pod::Collider::CATEGORY_STATIC );
	if ( c == "DYNAMIC" ) return uf::physics::setColliderCategory( body, pod::Collider::CATEGORY_DYNAMIC );
	if ( c == "PLAYER" ) return uf::physics::setColliderCategory( body, pod::Collider::CATEGORY_PLAYER );
	if ( c == "NPC" ) return uf::physics::setColliderCategory( body, pod::Collider::CATEGORY_NPC );
	if ( c == "TRIGGER" ) return uf::physics::setColliderCategory( body, pod::Collider::CATEGORY_TRIGGER );
	if ( c == "PROJECTILE" ) return uf::physics::setColliderCategory( body, pod::Collider::CATEGORY_PROJECTILE );
	if ( c == "CHARACTER" ) return uf::physics::setColliderCategory( body, pod::Collider::CATEGORY_CHARACTER );
	if ( c == "ALL" ) return uf::physics::setColliderCategory( body, pod::Collider::CATEGORY_ALL );
}
void uf::physics::setColliderMask( pod::PhysicsBody& body, uint32_t mask ) {
	body.collider.mask = mask;
}
void uf::physics::setColliderMask( pod::PhysicsBody& body, const uf::stl::string& mask ) {
	auto m = uf::string::uppercase( mask );
	if ( m == "NONE" ) return uf::physics::setColliderMask( body, pod::Collider::MASK_NONE );
	if ( m == "STATIC" ) return uf::physics::setColliderMask( body, pod::Collider::MASK_STATIC );
	if ( m == "DYNAMIC" ) return uf::physics::setColliderMask( body, pod::Collider::MASK_DYNAMIC );
	if ( m == "PLAYER" ) return uf::physics::setColliderMask( body, pod::Collider::MASK_PLAYER );
	if ( m == "NPC" ) return uf::physics::setColliderMask( body, pod::Collider::MASK_NPC );
	if ( m == "TRIGGER" ) return uf::physics::setColliderMask( body, pod::Collider::MASK_TRIGGER );
	if ( m == "PROJECTILE" ) return uf::physics::setColliderMask( body, pod::Collider::MASK_PROJECTILE );
	if ( m == "CHARACTER" ) return uf::physics::setColliderCategory( body, pod::Collider::MASK_CHARACTER );
	if ( m == "ALL" ) return uf::physics::setColliderMask( body, pod::Collider::MASK_ALL );
}
void uf::physics::setGravity( pod::PhysicsBody& body, const pod::Vector3f& gravity ) {
	body.gravity = gravity;
}
pod::Vector3f uf::physics::getGravity( pod::PhysicsBody& body ) {
	return uf::vector::isValid( body.gravity ) ? body.gravity : body.world->gravity;
}

void uf::physics::updateInertia( pod::PhysicsBody& body ) {
	if ( body.isStatic || body.mass <= 0 ) {
		body.inertiaTensor = { FLT_MAX, FLT_MAX, FLT_MAX };
		body.inverseInertiaTensor = { 0.0f, 0.0f, 0.0f };
		return;
	}

	switch ( body.collider.type ) {
		case pod::ShapeType::AABB: 
		case pod::ShapeType::OBB: {
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
void uf::physics::applyForce( pod::PhysicsBody& body, const pod::Vector3f& force ) {
	if ( body.isStatic ) return;
	impl::wakeBody( body );
	body.forceAccumulator += force;
}
void uf::physics::applyForceAtPoint( pod::PhysicsBody& body, const pod::Vector3f& force, const pod::Vector3f& point ) {
	if ( body.isStatic ) return;
	impl::wakeBody( body );
	// linear force
	body.forceAccumulator += force;
	// angular force
	pod::Vector3f r = point - impl::getPosition( body );
	body.torqueAccumulator += uf::vector::cross( r, force );
}
void uf::physics::applyImpulse( pod::PhysicsBody& body, const pod::Vector3f& impulse ) {
	if ( body.isStatic ) return; impl::wakeBody( body );
	body.velocity += impulse * body.inverseMass;
}
void uf::physics::applyTorque( pod::PhysicsBody& body, const pod::Vector3f& torque ) {
	if ( body.isStatic ) return; impl::wakeBody( body );
	body.torqueAccumulator += torque;
}
void uf::physics::setVelocity( pod::PhysicsBody& body, const pod::Vector3f& v ) {
	impl::wakeBody( body );
	body.velocity = v;
}
void uf::physics::applyVelocity( pod::PhysicsBody& body, const pod::Vector3f& v ) {
	impl::wakeBody( body );
	body.velocity += v;
}
void uf::physics::setAngularVelocity( pod::PhysicsBody& body, const pod::Vector3f& v ) {
	impl::wakeBody( body );
	body.angularVelocity = v;
}
void uf::physics::setAngularVelocity( pod::PhysicsBody& body, const pod::Quaternion<>& q, float dt ) {
	if ( !dt ) dt = uf::physics::time::delta;
	float angle = 2.0f * std::acos( q.w );
	float sinHalfAngle = std::sqrt( 1.0f - q.w * q.w );

	pod::Vector3f axis{ 0, 0, 0 };
	if ( sinHalfAngle > EPS ) {
		axis.x = q.x / sinHalfAngle;
		axis.y = q.y / sinHalfAngle;
		axis.z = q.z / sinHalfAngle;
	}

	impl::wakeBody( body );
	body.angularVelocity = axis * ( angle / dt );
	UF_MSG_DEBUG("axis={}, angle={}, dt={}", uf::vector::toString( axis ), angle, dt );
}
void uf::physics::applyAngularVelocity( pod::PhysicsBody& body, const pod::Vector3f& v ) {
	impl::wakeBody( body );
	body.angularVelocity += v;
}
void uf::physics::applyAngularVelocity( pod::PhysicsBody& body, const pod::Quaternion<>& q, float dt ) {
	if ( !dt ) dt = uf::physics::time::delta;
	float angle = 2.0f * std::acos( q.w );
	float sinHalfAngle = std::sqrt( 1.0f - q.w * q.w );

	pod::Vector3f axis{ 0, 0, 0 };
	if ( sinHalfAngle > EPS ) {
		axis.x = q.x / sinHalfAngle;
		axis.y = q.y / sinHalfAngle;
		axis.z = q.z / sinHalfAngle;
	}

	impl::wakeBody( body );
	body.angularVelocity += axis * ( angle / dt );
	UF_MSG_DEBUG("axis={}, angle={}, dt={}", uf::vector::toString( axis ), angle, dt );
}
void uf::physics::applyRotation( pod::PhysicsBody& body, const pod::Quaternion<>& q ) {
	impl::wakeBody( body );
	uf::transform::rotate( *body.transform/*.reference*/, q );
}
void uf::physics::applyRotation( pod::PhysicsBody& body, const pod::Vector3f& axis, float angle ) {
	uf::physics::applyRotation( body, uf::quaternion::axisAngle( axis, angle ) );
}

// body creation
pod::PhysicsBody& uf::physics::create( pod::World& world, uf::Object& object, float mass, const pod::Vector3f& offset ) {
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
		uf::physics::setColliderCategory(body, "STATIC");
		uf::physics::setColliderMask(body, "STATIC");
		world.staticBvh.dirty = true; // mark as dirty
	} else {
		uf::physics::setColliderCategory(body, "DYNAMIC");
		uf::physics::setColliderMask(body, "DYNAMIC");
		world.dynamicBvh.dirty = true; // mark as dirty
	}

	world.bodies.emplace_back(&body); // insert into world

	return body;
}
pod::PhysicsBody& uf::physics::create( pod::World& world, uf::Object& object, const pod::AABB& aabb, float mass, const pod::Vector3f& offset ) {
	auto& body = uf::physics::create( world, object, mass, offset );
	//body.collider.type = pod::ShapeType::AABB;
	body.collider.type = pod::ShapeType::OBB;
	body.collider.aabb = aabb;
	body.bounds = impl::computeAABB( body );
	uf::physics::updateInertia( body );
	return body;
}
pod::PhysicsBody& uf::physics::create( pod::World& world, uf::Object& object, const pod::Sphere& sphere, float mass, const pod::Vector3f& offset ) {
	auto& body = uf::physics::create( world, object, mass, offset );
	body.collider.type = pod::ShapeType::SPHERE;
	body.collider.sphere = sphere;
	body.bounds = impl::computeAABB( body );
	uf::physics::updateInertia( body );
	return body;
}
pod::PhysicsBody& uf::physics::create( pod::World& world, uf::Object& object, const pod::Plane& plane, float mass, const pod::Vector3f& offset ) {
	auto& body = uf::physics::create( world, object, mass, offset );
	body.collider.type = pod::ShapeType::PLANE;
	body.collider.plane = plane;
	body.bounds = impl::computeAABB( body );
	uf::physics::updateInertia( body );
	return body;
}
pod::PhysicsBody& uf::physics::create( pod::World& world, uf::Object& object, const pod::Capsule& capsule, float mass, const pod::Vector3f& offset ) {
	auto& body = uf::physics::create( world, object, mass, offset );
	body.collider.type = pod::ShapeType::CAPSULE;
	body.collider.capsule = capsule;
	body.bounds = impl::computeAABB( body );
	uf::physics::updateInertia( body );
	return body;
}
pod::PhysicsBody& uf::physics::create( pod::World& world, uf::Object& object, const pod::TriangleWithNormal& tri, float mass, const pod::Vector3f& offset ) {
	auto& body = uf::physics::create( world, object, mass, offset );
	body.collider.type = pod::ShapeType::TRIANGLE;
	body.collider.triangle = tri;
	body.bounds = impl::computeAABB( body );
	uf::physics::updateInertia( body );
	return body;
}
pod::PhysicsBody& uf::physics::create( pod::World& world, uf::Object& object, const uf::Mesh& mesh, float mass, const pod::Vector3f& offset, bool convex ) {
	auto& body = uf::physics::create( world, object, mass, offset );
	if ( !convex ) {
		body.collider.type = pod::ShapeType::MESH;
		body.collider.mesh.mesh = &mesh;
		body.collider.mesh.bvh = new pod::BVH;
		
		auto& bvh = *body.collider.mesh.bvh;
		impl::buildMeshBVH( bvh, mesh, uf::physics::settings.meshBvhCapacity );
	} else {
		body.collider.type = pod::ShapeType::CONVEX_HULL;
		body.collider.convexHull.mesh = &mesh;
		body.collider.convexHull.bvh = new pod::BVH;
		
		auto& bvh = *body.collider.convexHull.bvh;
		impl::buildConvexHullBVH( bvh, mesh/*, uf::physics::settings.meshBvhCapacity*/ );
	}

	body.bounds = impl::computeAABB( body );
	uf::physics::updateInertia( body );
	return body;
}

pod::PhysicsBody& uf::physics::create( uf::Object& object, float mass, const pod::Vector3f& offset ) {
	// bind to scene
	// auto& root = object.getRootParent<>(); // in the event a scene is being initialized that is not the root scene, use the root parent instead
	// auto& world = root.getComponent<pod::World>();
	auto& scene = uf::scene::getCurrentScene();
	auto& world = scene.getComponent<pod::World>();
	return create( world, object, mass, offset );
	
}
pod::PhysicsBody& uf::physics::create( uf::Object& object, const pod::AABB& aabb, float mass, const pod::Vector3f& offset ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& world = scene.getComponent<pod::World>();
	return create( world, object, aabb, mass, offset );
}
pod::PhysicsBody& uf::physics::create( uf::Object& object, const pod::Sphere& sphere, float mass, const pod::Vector3f& offset ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& world = scene.getComponent<pod::World>();
	return create( world, object, sphere, mass, offset );
}
pod::PhysicsBody& uf::physics::create( uf::Object& object, const pod::Plane& plane, float mass, const pod::Vector3f& offset ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& world = scene.getComponent<pod::World>();
	return create( world, object, plane, mass, offset );
}
pod::PhysicsBody& uf::physics::create( uf::Object& object, const pod::Capsule& capsule, float mass, const pod::Vector3f& offset ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& world = scene.getComponent<pod::World>();
	return create( world, object, capsule, mass, offset );
}
pod::PhysicsBody& uf::physics::create( uf::Object& object, const uf::Mesh& mesh, float mass, const pod::Vector3f& offset, bool convex ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& world = scene.getComponent<pod::World>();
	return create( world, object, mesh, mass, offset, convex );
}

void uf::physics::destroy( uf::Object& object ) {
	if ( !object.hasComponent<pod::PhysicsBody>() ) return;
	
	return destroy( object.getComponent<pod::PhysicsBody>() );
}
void uf::physics::destroy( pod::PhysicsBody& body ) {
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
	if ( body.collider.type == pod::ShapeType::CONVEX_HULL ) {
		if ( body.collider.convexHull.bvh ) delete body.collider.convexHull.bvh;
	}
}

pod::RayQuery uf::physics::rayCast( const pod::Ray& ray, const pod::PhysicsBody& body, float maxDistance ) {
	return rayCast( ray, *body.world, &body, maxDistance );
}
pod::RayQuery uf::physics::rayCast( const pod::Ray& ray, const pod::World& world, float maxDistance ) {
	return rayCast( ray, world, NULL, maxDistance );
}
pod::RayQuery uf::physics::rayCast( const pod::Ray& ray, const pod::World& world, const pod::PhysicsBody* body, float maxDistance ) {
	pod::RayQuery rayHit;
	rayHit.contact.penetration = maxDistance;

	auto& dynamicBvh = world.dynamicBvh;
	auto& staticBvh = world.staticBvh;
	auto& bodies = world.bodies;

	STATIC_THREAD_LOCAL(uf::stl::vector<pod::BVH::index_t>, candidates);
	impl::queryBVH( dynamicBvh, ray, candidates );
	if ( uf::physics::settings.useSplitBvhs ) impl::queryBVH( staticBvh, ray, candidates );

	for ( auto i : candidates ) {
		auto* b = bodies[i];
		
		if ( body == b ) continue;

		switch ( b->collider.type ) {
			case pod::ShapeType::AABB: impl::rayAabb( ray, *b, rayHit ); break;
			case pod::ShapeType::OBB: impl::rayObb( ray, *b, rayHit ); break;
			case pod::ShapeType::SPHERE: impl::raySphere( ray, *b, rayHit ); break;
			case pod::ShapeType::PLANE: impl::rayPlane( ray, *b, rayHit ); break;
			case pod::ShapeType::CAPSULE: impl::rayCapsule( ray, *b, rayHit ); break;
			case pod::ShapeType::MESH: impl::rayMesh( ray, *b, rayHit ); break;
			case pod::ShapeType::CONVEX_HULL: impl::rayHull( ray, *b, rayHit ); break;
		}
	}

	return rayHit;
}

#if UF_PHYSICS_TEST
	#include "tests.inl"
#endif