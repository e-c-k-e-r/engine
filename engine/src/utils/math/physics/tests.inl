#include <uf/utils/tests/tests.h>

#define FRAMERATE 60
#define INV_FRAMERATE 1.0f / FRAMERATE

#define PHYSICS_STEP(time) for ( int i = 0; i < time * FRAMERATE; i++ ) uf::physics::step(world, INV_FRAMERATE);

namespace impl {
	uf::Mesh generateMesh( float size = 1 ) {
		uf::Mesh mesh;
		mesh.bind<pod::Vertex_3F2F, uint16_t>();
		mesh.insertVertices<pod::Vertex_3F2F>({
			{ pod::Vector3f{-1.0f * size, 0.0f,  1.0f * size}, pod::Vector2f{0.0f, 0.0f} },
			{ pod::Vector3f{-1.0f * size, 0.0f, -1.0f * size}, pod::Vector2f{0.0f, 1.0f} },
			{ pod::Vector3f{ 1.0f * size, 0.0f, -1.0f * size}, pod::Vector2f{1.0f, 1.0f} },
		
			{ pod::Vector3f{ 1.0f * size, 0.0f, -1.0f * size}, pod::Vector2f{1.0f, 1.0f} },
			{ pod::Vector3f{ 1.0f * size, 0.0f,  1.0f * size}, pod::Vector2f{1.0f, 0.0f} },
			{ pod::Vector3f{-1.0f * size, 0.0f,  1.0f * size}, pod::Vector2f{0.0f, 0.0f} },
		});
		mesh.insertIndices<uint16_t>({
			//0, 1, 2, 3, 4, 5
			0, 2, 1, 3, 5, 4 
		});
		mesh.updateDescriptor();

		return mesh;
	}
}

// cringe wrapper because i changed the api again
namespace uf {
	namespace physics {
		template<typename T> pod::PhysicsBody& create( pod::World& world, uf::Object& object, const T& shape, float mass, const pod::Vector3f& center = {} ) {
			auto& body = uf::physics::create( world, object, mass, center );
			return uf::physics::initialize( body, shape );
		}
	}
}

// list of unit tests to "standardly" verify the system works, but honestly this is a mess
// to-do: clean up all of this
TEST(SphereSphere_Collision, {
	pod::World world;
	uf::Object objA, objB;
	
	auto& bodyA = uf::physics::create(world, objA, pod::Sphere{1.0f}, 1.0f);
	auto& bodyB = uf::physics::create(world, objB, pod::Sphere{1.0f}, 1.0f);

	bodyA.transform->position = {};
	bodyB.transform->position.x = 1.5f;

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::sphereSphere(bodyA, bodyB, m);
	EXPECT_TRUE(collided);
	EXPECT_TRUE(!m.points.empty());
	EXPECT_NEAR(m.points[0].penetration, 0.5f, EPS);
})

TEST(AabbAabb_Collision, {
	pod::World world;
	uf::Object objA, objB;
	pod::AABB box = { {-1,-1,-1}, {1,1,1} };

	auto& bodyA = uf::physics::create(world, objA, box, 1.0f);
	auto& bodyB = uf::physics::create(world, objB, box, 1.0f);

	bodyA.transform->position = {};
	bodyB.transform->position.x = 1.5f;

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::aabbAabb(bodyA, bodyB, m);
	EXPECT_TRUE(collided);
	EXPECT_TRUE(!m.points.empty());
})

TEST(RaySphere_Hit, {
	pod::World world;
	uf::Object obj;
	auto& body = uf::physics::create(world, obj, pod::Sphere{1.0f}, 1.0f);
	body.transform->position = {0,0,0};
	body.bounds = impl::computeAABB( body );

	impl::buildBroadphaseBVH( world.dynamicBvh, world.bodies );

	pod::Ray ray{ {0,0,-5}, uf::vector::normalize(pod::Vector3f{0,0,1}) };
	pod::RayQuery hit = uf::physics::rayCast(ray, world, 100.0f);

	EXPECT_TRUE(hit.hit);
	EXPECT_NEAR(hit.contact.penetration, 4.0f, EPS);
})

TEST(SphereSphere_NoCollision, {
	pod::World world;
	uf::Object objA, objB;
	auto& bodyA = uf::physics::create(world, objA, pod::Sphere{1.0f}, 1.0f);
	auto& bodyB = uf::physics::create(world, objB, pod::Sphere{1.0f}, 1.0f);

	bodyA.transform->position = {0,0,0};
	bodyB.transform->position = {5.0f,0,0}; // too far

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::sphereSphere(bodyA, bodyB, m);
	EXPECT_TRUE(!collided);
})

TEST(SphereAabb_Collision, {
	pod::World world;
	uf::Object objA, objB;

	auto& bodyA = uf::physics::create(world, objA, pod::Sphere{1.0f}, 1.0f);
	auto& bodyB = uf::physics::create(world, objB, pod::AABB{{-1,-1,-1}, {1,1,1}}, 1.0f);

	bodyA.transform->position = {0.5f, 0.0f, 0.0f}; // overlapping inside box
	bodyB.transform->position = {0,0,0};

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::sphereAabb(bodyA, bodyB, m);
	EXPECT_TRUE(collided);
	EXPECT_TRUE(!m.points.empty());
	EXPECT_GT(m.points[0].penetration, 0.0f);
})

TEST(SpherePlane_Collision, {
	pod::World world;
	uf::Object objA, objB;

	auto& bodyA = uf::physics::create(world, objA, pod::Sphere{1.0f}, 1.0f);
	auto& bodyB = uf::physics::create(world, objB, pod::Plane{{0,1,0},0.0f}, 0.0f);

	// Place sphere so it's intersecting the plane
	bodyA.transform->position = {0,0.5f,0};

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::planeSphere(bodyB, bodyA, m);
	EXPECT_TRUE(collided);
	EXPECT_TRUE(!m.points.empty());
	EXPECT_NEAR(m.points[0].penetration, 0.5f, EPS);
})

TEST(SpherePlane_NoCollision, {
	pod::World world;
	uf::Object objA, objB;

	auto& bodyA = uf::physics::create(world, objA, pod::Sphere{1.0f}, 1.0f);
	auto& bodyB = uf::physics::create(world, objB, pod::Plane{{0,1,0},0.0f}, 0.0f);

	bodyA.transform->position = {0, 5.0f, 0}; // clearly above
	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::planeSphere(bodyB, bodyA, m);
	EXPECT_TRUE(!collided);
})

TEST(CapsuleCapsule_Collision, {
	pod::World world;
	uf::Object objA, objB;
	auto& bodyA = uf::physics::create(world, objA, pod::Capsule{0.5f, pod::Vector3f{0,1,0}}, 1.0f);
	auto& bodyB = uf::physics::create(world, objB, pod::Capsule{0.5f, pod::Vector3f{0,1,0}}, 1.0f);

	bodyA.transform->position = {0,0,0};
	bodyB.transform->position = {0.8f,0,0}; // slight overlap

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::capsuleCapsule(bodyA, bodyB, m);
	EXPECT_TRUE(collided);
	EXPECT_TRUE(!m.points.empty());
	EXPECT_GT(m.points[0].penetration, 0.0f);
})

TEST(RayAabb_Miss, {
	pod::World world;
	uf::Object obj;
	auto& box = uf::physics::create(world, obj, pod::AABB{{-1,-1,-1},{1,1,1}}, 1.0f);
	box.transform->position = {0,0,0};
	box.bounds = impl::computeAABB( box );

	impl::buildBroadphaseBVH( world.dynamicBvh, world.bodies );

	pod::Ray ray{{5,5,5}, uf::vector::normalize(pod::Vector3f{1,0,0})};
	auto hit = uf::physics::rayCast(ray, world, 100.0f);
	EXPECT_TRUE(!hit.hit);
})

// GJK shouldn't be used for sphere sphere
#if 0
TEST(Gjk_SphereSphereOverlap, {
	pod::World world;
	uf::Object objA, objB;
	auto& a = uf::physics::create(world, objA, pod::Sphere{1.0f}, 1.0f);
	auto& b = uf::physics::create(world, objB, pod::Sphere{1.0f}, 1.0f);

	a.transform->position = {0,0,0};
	b.transform->position = {1.5f,0,0};

	pod::Simplex simplex;
	bool inside = gjk(a, b, simplex);
	EXPECT_TRUE(inside);
	auto contact = epa(a, b, simplex);
	EXPECT_GT(contact.penetration, 0.0f);
})
#endif

// only works when stepping for dt=1.0f
#if 0
TEST(PhysicsStep_Gravity, {
	pod::World world;
	uf::Object obj;
	auto& body = uf::physics::create(world, obj, pod::Sphere{1.0f}, 1.0f);
	body.transform->position = {0, 10, 0};
	body.velocity = {0,0,0};

	PHYSICS_STEP(1)

	EXPECT_NEAR(body.transform->position.y, 10.0f - world.gravity.y, 0.05f);
})
#endif

TEST(PhysicsStep_SpherePlane_Bounce, {
	pod::World world;
	uf::Object objSphere, objPlane;

	auto& sphere = uf::physics::create(world, objSphere, pod::Sphere{1.0f}, 1.0f);
	auto& plane  = uf::physics::create(world, objPlane, pod::Plane{{0,1,0}, 0.0f}, 0.0f);

	sphere.transform->position = {0, 2, 0};
	sphere.material.restitution = 1.0f;

	PHYSICS_STEP(1)

	// After bouncing, sphere should be near plane surface, not sinking below
	EXPECT_GE(sphere.transform->position.y, 0.9f);
	EXPECT_LT(fabs(sphere.velocity.y), 10.0f); // should have reversed sign at least once
})

TEST(PhysicsStep_AabbStacking, {
	pod::World world;
	uf::Object bottomObj, fallingObj;

	auto& bottom = uf::physics::create(world, bottomObj, pod::AABB{{-1,-1,-1},{1,1,1}}, 0.0f);
	bottom.transform->position = {0,0,0};

	auto& falling = uf::physics::create(world, fallingObj, pod::AABB{{-1,-1,-1},{1,1,1}}, 1.0f);
	falling.transform->position = {0,5,0};

	PHYSICS_STEP(5);

	// After time, falling cube should rest on top of static one
	EXPECT_GT(falling.transform->position.y, 1.9f);
	EXPECT_LT(fabs(falling.velocity.y),0.1f);
})

TEST(PhysicsStep_SphereSphere_HeadOn, {
	pod::World world;
	uf::Object objA, objB;

	auto& A = uf::physics::create(world, objA, pod::Sphere{1.0f}, 1.0f);
	auto& B = uf::physics::create(world, objB, pod::Sphere{1.0f}, 1.0f);

	A.transform->position = {-5,0,0};
	B.transform->position = { 5,0,0};
	A.velocity = { 5,0,0};
	B.velocity = {-5,0,0};

	PHYSICS_STEP(5);

	// Expect velocities swapped (perfect elastic bounce with equal masses)
	EXPECT_LT(A.velocity.x,0.0f);
	EXPECT_GT(B.velocity.x, 0.0f);
})

TEST(PhysicsStep_RaycastDynamic, {
	pod::World world;
	uf::Object obj;
	world.gravity = {};
	auto& body = uf::physics::create(world, obj, pod::Sphere{1.0f}, 1.0f);
	body.transform->position = {0,0,0};
	body.velocity = {0,0,10};

	PHYSICS_STEP(1);

	pod::Ray ray{ {0,0,-5}, {0,0,1} };
	pod::RayQuery q = uf::physics::rayCast(ray, world, 100.0f);
	EXPECT_TRUE(q.hit);
	EXPECT_LE(fabs(q.contact.point.z - 10.0f), 1.0f); // near where it moved
})

TEST(SphereSphere_TouchingButNotOverlapping, {
	pod::World world;
	uf::Object objA, objB;
	auto& bodyA = uf::physics::create(world, objA, pod::Sphere{1.0f}, 1.0f);
	auto& bodyB = uf::physics::create(world, objB, pod::Sphere{1.0f}, 1.0f);
	bodyA.transform->position = {0,0,0};
	bodyB.transform->position = {2.0f,0,0}; // exactly touching

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::sphereSphere(bodyA, bodyB, m);

	EXPECT_TRUE(collided);	   // should count as a collision
	EXPECT_NEAR(m.points[0].penetration, 0.0f, EPS);
})

// expects being inside the body will set the hit to where the origin is
#if 0
TEST(RaySphere_OriginInside, {
	pod::World world;
	uf::Object obj;
	auto& body = uf::physics::create(world, obj, pod::Sphere{2.0f}, 1.0f);
	body.transform->position = {0,0,0};
	body.bounds = impl::computeAABB( body );

	impl::buildBroadphaseBVH( world.dynamicBvh, world.bodies );

	pod::Ray ray{ {0,0,0}, {1,0,0} }; // starts inside
	auto q = uf::physics::rayCast(ray, world, 100.0f);

	EXPECT_TRUE(q.hit);
	EXPECT_NEAR(q.contact.penetration, 0.0f, EPS);
})
#endif

TEST(PhysicsStep_StaticFriction_Holds, {
	pod::World world;
	uf::Object objA, objB;

	auto& bodyA = uf::physics::create(world, objA, pod::Sphere{1.0f}, 1.0f);
	auto& bodyB  = uf::physics::create(world, objB, pod::Plane{{0,1,0}, 0.0f}, 0.0f);

	world.gravity = {0,-9.81f,0};
	bodyA.transform->position = {0,1.0f,0};
	bodyA.material.staticFriction = 2.0f; // stronger grip to cover solver slop

	// Apply smaller force (well below μ_s * N)
	uf::physics::applyForce(bodyA, {2,0,0});

	PHYSICS_STEP(1);

	EXPECT_NEAR(bodyA.transform->position.x, 0.0f, 0.05f); // allow tiny error
})

TEST(PhysicsStep_StaticFriction_Slips, {
	pod::World world;
	uf::Object objA, objB;

	auto& bodyA = uf::physics::create(world, objA, pod::Sphere{1.0f}, 1.0f);
	auto& bodyB  = uf::physics::create(world, objB, pod::Plane{{0,1,0}, 0.0f}, 0.0f);

	world.gravity = {0,-9.81f,0};
	bodyA.transform->position = {0,1.0f,0};
	bodyA.material.staticFriction = 1.0f;

	uf::physics::applyForce(bodyA, {15,0,0}); // Above limit

	PHYSICS_STEP(1);

	EXPECT_GT(fabs(bodyA.transform->position.x), 0.1f); // It should slide
})

// not really a good way to check as these are solver-dependent
#if 0
TEST(CapsulePlane_Slope_StaticHold, {
	pod::World world;
	uf::Object objA, objB;
	auto& bodyA = uf::physics::create(world, objA, pod::Capsule{0.5f, pod::Vector3f{0, 1, 0}}, 1.0f);
	auto& bodyB = uf::physics::create(world, objB, pod::Plane{{0,1,1},0.0f}, 0.0f);

	// Place capsule on slope
	bodyA.transform->position = {0,2,0};
	bodyA.material.staticFriction = 2.0f;   // More than tan(45) = 1
	bodyA.material.dynamicFriction = 1.0f;

	PHYSICS_STEP(5);

	EXPECT_NEAR(bodyA.transform->position.z, 0.0f, 0.2f); // Held in place
	EXPECT_NEAR(bodyA.velocity.z, 0.0f, 0.05f);
})
#endif

TEST(CapsulePlane_Slope_Slip, {
	pod::World world;
	uf::Object objA, objB;
	auto& bodyA = uf::physics::create(world, objA, pod::Capsule{0.5f, pod::Vector3f{0, 1, 0}}, 1.0f);
	auto& bodyB = uf::physics::create(world, objB, pod::Plane{{0,1,1},0.0f}, 0.0f);

	bodyA.transform->position = {0,2,0};
	bodyA.material.staticFriction = 0.5f;   // Less than tan(45) => must slip
	bodyA.material.dynamicFriction = 0.5f;

	PHYSICS_STEP(5);

	EXPECT_GT(fabs(bodyA.transform->position.z), 1.0f); // Should have slid downhill
})

TEST(CapsulePlane_RestingContact, {
	pod::World world;
	uf::Object objA, objB;
	auto& bodyA = uf::physics::create(world, objA, pod::Capsule{0.5f, pod::Vector3f{0,1,0}}, 1.0f);
	auto& bodyB = uf::physics::create(world, objB, pod::Plane{{0,1,0}, 0.0f}, 0.0f);

	bodyA.transform->position = {0, 1.5f, 0}; // halfHeight=1, radius=0.5, so "foot" at y=0
	bodyA.velocity = {0,0,0};

	PHYSICS_STEP(1);

	// Capsule should rest on the floor at y=1.5
	EXPECT_NEAR(bodyA.transform->position.y, 1.5f, 0.05f);
	EXPECT_NEAR(bodyA.velocity.y, 0.0f, 0.05f); // no jitter
})

TEST(CapsuleAabb_RestingContact, {
	pod::World world;
	uf::Object objA, objB;

	auto& bodyA = uf::physics::create(world, objA, pod::Capsule{0.5f, pod::Vector3f{0,1,0}}, 1.0f);
	auto& bodyB = uf::physics::create(world, objB, pod::AABB{{-5, -1, -5},{5, 0, 5}}, 0.0f);

	bodyA.transform->position = {0, 1.5f, 0};
	bodyA.velocity = {0,0,0};

	PHYSICS_STEP(2);

	EXPECT_NEAR(bodyA.transform->position.y, 1.5f, 0.05f);
	EXPECT_NEAR(bodyA.velocity.y, 0.0f, 0.05f);
})

TEST(CapsulePlane_Settling, {
	pod::World world;
	uf::Object objA, objB;

	auto& bodyA = uf::physics::create(world, objA, pod::Capsule{0.5f, pod::Vector3f{0,1,0}}, 1.0f);
	auto& bodyB = uf::physics::create(world, objB, pod::Plane{{0,1,0},0.0f}, 0.0f);

	bodyA.transform->position = {0, 2.0f, 0}; // slightly above
	bodyA.velocity = {0,0,0};

	PHYSICS_STEP(3);

	EXPECT_NEAR(bodyA.transform->position.y, 1.5f, 0.05f);
})

#if 0
TEST(CapsulePlane_SlopeStaticFriction, {
	pod::World world;
	uf::Object objA, objB;

	auto& bodyA = uf::physics::create(world, objA, pod::Capsule{0.5f, pod::Vector3f{0,1,0}}, 1.0f);
	auto& bodyB = uf::physics::create(world, objB, pod::Plane{{0,1,1},0.0f}, 0.0f); // 45° slope

	bodyA.transform->position = {0, 3.0f, 0};
	bodyA.material.staticFriction = 2.0f;
	bodyA.material.dynamicFriction = 1.0f;

	PHYSICS_STEP(4);

	// Should not slide much if static friction is strong
	EXPECT_NEAR(bodyA.transform->position.z, 0.0f, 0.1f);
})
#endif

TEST(CapsuleAabb_StepEdge, {
	pod::World world;
	uf::Object objA, objB;

	auto& bodyA = uf::physics::create(world, objA, pod::Capsule{0.5f, pod::Vector3f{0,1,0}}, 1.0f);
	auto& bodyB = uf::physics::create(world, objB, pod::AABB{{0,-1,-5},{5,0,5}}, 0.0f);

	bodyA.transform->position = {0.25f, 1.5f, 0}; // Capsule foot half on, half off

	PHYSICS_STEP(4);

	// Should not spaz out or fall through
	EXPECT_NEAR(bodyA.transform->position.y, 1.5f, 0.1f);
})

TEST(Diagnostic_CapsuleGrounding, {
	pod::World world;
	uf::Object objA, objFloor;

	// Capsule: radius 0.5, half-height 1.0 (total height 2.0 + end caps)
	auto& bodyA = uf::physics::create(world, objA, pod::Capsule{0.5f, pod::Vector3f{0,1,0}}, 1.0f);

	// Test toggle: try both AABB floors and Plane floors.
	bool usePlane = true;
	auto& bodyB = usePlane ? uf::physics::create(world, objFloor, pod::Plane{{0,1,0}, 0.0f}, 0.0f) : uf::physics::create(world, objFloor, pod::AABB{{-5,-1,-5},{5,0,5}}, 0.0f);

	bodyA.transform->position = {0, 3, 0}; // start a little above floor
	bodyA.velocity = {0,0,0};

	PHYSICS_STEP(2);

	// Final resting state: should be near Y=1.5 (halfHeight + radius)
	EXPECT_NEAR(bodyA.transform->position.y, 1.5f, 0.1f);
})

TEST(CapsulePlane_ContactNormal, {
	pod::World world;
	uf::Object objA, objB;

	auto& bodyA = uf::physics::create(world, objA, pod::Capsule{0.5f, pod::Vector3f{0,1,0}}, 1.0f);
	auto& bodyB = uf::physics::create(world, objB, pod::Plane{{0,1,0},0.0f}, 0.0f);

	bodyA.transform->position = {0,1.5f,0};
	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::capsulePlane(bodyA, bodyB, m);

	EXPECT_TRUE(collided);
	EXPECT_TRUE(!m.points.empty());
	if ( !m.points.empty() ) {
		EXPECT_NEAR(m.points[0].normal.y, 1.0f, 1e-3f); // Normal should point UP
	} else {
		EXPECT_NEAR(1.0f, 0.0f, 0.0f);
	}
})

TEST(AabbPlane_RestingNoSink, {
	pod::World world;
	uf::Object objA, objB;

	auto& bodyA = uf::physics::create(world, objA, pod::AABB{{-0.5f,-1.0f,-0.5f},{0.5f,0.0f,0.5f}}, 1.0f);
	auto& bodyB = uf::physics::create(world, objB, pod::Plane{{0,1,0},0}, 0.0f);

	bodyA.transform->position = {0, 2.0f, 0};
	
	PHYSICS_STEP(5);		

	// Expect the box to remain on plane at y=0.0 without sinking further
	EXPECT_NEAR(bodyA.transform->position.y, 0.0f, 0.05f);
	EXPECT_NEAR(bodyA.velocity.y, 0.0f, 0.05f);
})

TEST(CapsuleSphere_Collision, {
	pod::World world;
	uf::Object objA, objB;
	auto& bodyA = uf::physics::create(world, objA, pod::Capsule{0.5f, pod::Vector3f{0, 1, 0}}, 1.0f);
	auto& bodyB = uf::physics::create(world, objB, pod::Sphere{0.5f}, 1.0f);

	bodyA.transform->position = {0,0,0};
	bodyB.transform->position = {0,0.5f,0}; // overlap

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::capsuleSphere(bodyA, bodyB, m);
	EXPECT_TRUE(collided);
	EXPECT_TRUE(!m.points.empty());
})

TEST(CapsuleSphere_NoCollision, {
	pod::World world;
	uf::Object objA, objB;
	auto& bodyA = uf::physics::create(world, objA, pod::Capsule{0.5f, pod::Vector3f{0, 1, 0}}, 1.0f);
	auto& bodyB = uf::physics::create(world, objB, pod::Sphere{0.5f}, 1.0f);

	bodyA.transform->position = {0,0,0};
	bodyB.transform->position = {0,5,0}; // too far

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::capsuleSphere(bodyA, bodyB, m);
	EXPECT_TRUE(!collided);
})


TEST(AabbSphere_Collision, {
	pod::World world;
	uf::Object objA, objB;
	auto& bodyA = uf::physics::create(world, objA, pod::AABB{{-1,-1,-1},{1,1,1}}, 1.0f);
	auto& bodyB = uf::physics::create(world, objB, pod::Sphere{0.5f}, 1.0f);

	bodyA.transform->position = {0,0,0};
	bodyB.transform->position = {0.75f,0,0}; // Intersecting into box

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::aabbSphere( bodyA, bodyB, m );
	EXPECT_TRUE(collided);
	EXPECT_TRUE(!m.points.empty());
})

TEST(AabbSphere_NoCollision, {
	pod::World world;
	uf::Object objA, objB;
	auto& bodyA = uf::physics::create(world, objA, pod::AABB{{-1,-1,-1},{1,1,1}}, 1.0f);
	auto& bodyB = uf::physics::create(world, objB, pod::Sphere{0.5f}, 1.0f);

	bodyA.transform->position	= {0,0,0};
	bodyB.transform->position = {5,0,0}; // too far away

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::aabbSphere(bodyA,bodyB,m);
	EXPECT_TRUE(!collided);
})


TEST(AabbPlane_Collision, {
	pod::World world;
	uf::Object objA, objB;
	auto& bodyA   = uf::physics::create(world, objA, pod::AABB{{-1,-1,-1},{1,1,1}}, 1.0f);
	auto& bodyB = uf::physics::create(world, objB, pod::Plane{{0,1,0},0.0f}, 0.0f);

	bodyA.transform->position = {0,0.5f,0}; // half interpenetrating plane

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::aabbPlane(bodyA,bodyB,m);
	EXPECT_TRUE(collided);
	EXPECT_TRUE(!m.points.empty());
})

TEST(AabbPlane_NoCollision, {
	pod::World world;
	uf::Object objA, objB;
	auto& bodyA   = uf::physics::create(world, objA, pod::AABB{{-1,-1,-1},{1,1,1}}, 1.0f);
	auto& bodyB = uf::physics::create(world, objB, pod::Plane{{0,1,0},0.0f}, 0.0f);

	bodyA.transform->position = {0,5,0}; // clearly above

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::aabbPlane(bodyA,bodyB,m);
	EXPECT_TRUE(!collided);
})


TEST(AabbCapsule_Collision, {
	pod::World world;
	uf::Object objA, objB;
	auto& bodyA	 = uf::physics::create(world, objA, pod::AABB{{-1,-1,-1},{1,1,1}}, 1.0f);
	auto& bodyB	 = uf::physics::create(world, objB, pod::Capsule{0.5f, pod::Vector3f{0, 1, 0}}, 1.0f);

	bodyA.transform->position = {0,0,0};
	bodyB.transform->position = {0,0.5f,0}; // partially overlapping

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::aabbCapsule(bodyA,bodyB,m);
	EXPECT_TRUE(collided);
	EXPECT_TRUE(!m.points.empty());
})

TEST(AabbCapsule_NoCollision, {
	pod::World world;
	uf::Object objA, objB;
	auto& bodyA	 = uf::physics::create(world, objA, pod::AABB{{-1,-1,-1},{1,1,1}}, 1.0f);
	auto& bodyB	 = uf::physics::create(world, objB, pod::Capsule{0.5f, pod::Vector3f{0, 1, 0}}, 1.0f);

	bodyA.transform->position = {0,0,0};
	bodyB.transform->position = {0,5,0};

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::aabbCapsule(bodyA,bodyB,m);
	EXPECT_TRUE(!collided);
})


TEST(SphereCapsule_Collision, {
	pod::World world;
	uf::Object objA, objB;
	auto& bodyA = uf::physics::create(world, objA, pod::Sphere{0.5f}, 1.0f);
	auto& bodyB	= uf::physics::create(world, objB, pod::Capsule{0.5f, pod::Vector3f{0, 1, 0}}, 1.0f);

	bodyA.transform->position = {0,0.0f,0};
	bodyB.transform->position	= {0,0.25f,0};

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::sphereCapsule(bodyA,bodyB,m);
	EXPECT_TRUE(collided);
	EXPECT_TRUE(!m.points.empty());
})

TEST(SphereCapsule_NoCollision, {
	pod::World world;
	uf::Object objA, objB;
	auto& bodyA = uf::physics::create(world, objA, pod::Sphere{0.5f}, 1.0f);
	auto& bodyB	= uf::physics::create(world, objB, pod::Capsule{0.5f, pod::Vector3f{0, 1, 0}}, 1.0f);

	bodyA.transform->position = {0,5,0};
	bodyB.transform->position	= {0,0,0};

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::sphereCapsule(bodyA,bodyB,m);
	EXPECT_TRUE(!collided);
})

TEST(PlanePlane_NoCollision, {
	pod::World world;
	uf::Object objA, objB;
	auto& bodyA= uf::physics::create(world, objA, pod::Plane{{0,1,0},0.0f}, 0.0f);
	auto& bodyB= uf::physics::create(world, objB, pod::Plane{{0,0,1},0.0f}, 0.0f);

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::planePlane(bodyA,bodyB,m);
	EXPECT_TRUE(!collided); // always false in your engine
})


TEST(PlaneCapsule_Collision, {
	pod::World world;
	uf::Object objA, objB;
	auto& bodyA = uf::physics::create(world, objA, pod::Plane{{0,1,0},0.0f}, 0.0f);
	auto& bodyB   = uf::physics::create(world, objB, pod::Capsule{0.5f, pod::Vector3f{0, 1, 0}}, 1.0f);

	bodyB.transform->position = {0,0.25f,0}; // foot intersecting

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::planeCapsule(bodyA,bodyB,m);
	EXPECT_TRUE(collided);
	EXPECT_TRUE(!m.points.empty());
})

TEST(PlaneCapsule_NoCollision, {
	pod::World world;
	uf::Object objA, objB;
	auto& bodyA = uf::physics::create(world, objA, pod::Plane{{0,1,0},0.0f}, 0.0f);
	auto& bodyB   = uf::physics::create(world, objB, pod::Capsule{0.5f, pod::Vector3f{0, 1, 0}}, 1.0f);

	bodyB.transform->position = {0,5,0}; // far above

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::planeCapsule(bodyA,bodyB,m);
	EXPECT_TRUE(!collided);
})
TEST(MeshSphere_Collision, {
	pod::World world;
	uf::Object objMesh, objSphere;

	// Create mesh body (a plane on Y=0, size=1)
	auto mesh = impl::generateMesh(1.0f);
	auto& bodyA = uf::physics::create(world, objMesh, mesh, 0.0f); // static mesh
	bodyA.transform->position = {0,0,0};

	// Sphere just above plane, radius 1, intersects
	auto& bodyB = uf::physics::create(world, objSphere, pod::Sphere{2.0f}, 1.0f);
	bodyB.transform->position = {0,0.5f,0}; // half below plane (since plane is at y=0)

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::meshSphere(bodyA, bodyB, m);
	EXPECT_TRUE(collided);
	EXPECT_TRUE(!m.points.empty());
	if ( !m.points.empty() ) EXPECT_GT(m.points[0].penetration, 0.0f);
})

TEST(MeshSphere_NoCollision, {
	pod::World world;
	uf::Object objMesh, objSphere;

	auto mesh = impl::generateMesh(1.0f);
	auto& bodyA = uf::physics::create(world, objMesh, mesh, 0.0f);
	bodyA.transform->position = {0,0,0};

	auto& bodyB = uf::physics::create(world, objSphere, pod::Sphere{0.5f}, 1.0f);
	bodyB.transform->position = {0,5.0f,0}; // far above plane

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::meshSphere(bodyA, bodyB, m);
	EXPECT_FALSE(collided);
})

TEST(MeshAabb_Collision, {
	pod::World world;
	uf::Object objMesh, objBox;

	auto mesh = impl::generateMesh(2.0f);
	auto& bodyA = uf::physics::create(world, objMesh, mesh, 0.0f);
	bodyA.transform->position = {0,0,0};

	pod::AABB box = { {-0.5f,-0.5f,-0.5f}, {0.5f,0.5f,0.5f} };
	auto& bodyB = uf::physics::create(world, objBox, box, 1.0f);
	bodyB.transform->position = {0,0.25f,0}; // overlaps plane

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::meshAabb(bodyA, bodyB, m);
	EXPECT_TRUE(collided);
	EXPECT_TRUE(!m.points.empty());
})

TEST(MeshAabb_NoCollision, {
	pod::World world;
	uf::Object objMesh, objBox;

	auto mesh = impl::generateMesh(2.0f);
	auto& bodyA = uf::physics::create(world, objMesh, mesh, 0.0f);
	bodyA.transform->position = {0,0,0};

	pod::AABB box = { {-0.5f,-0.5f,-0.5f}, {0.5f,0.5f,0.5f} };
	auto& bodyB = uf::physics::create(world, objBox, box, 1.0f);
	bodyB.transform->position = {0,5.0f,0}; // above plane, no overlap

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::meshAabb(bodyA, bodyB, m);
	EXPECT_FALSE(collided);
})

TEST(RayMesh_Hit, {
	pod::World world;
	uf::Object objMesh;

	auto mesh = impl::generateMesh(1.0f);
	auto& bodyA = uf::physics::create(world, objMesh, mesh, 0.0f);
	bodyA.transform->position = {0,0,0};

	impl::buildBroadphaseBVH( world.dynamicBvh, world.bodies );

	pod::Ray ray{ {0,1,0}, {0,-1,0} }; // from above, pointing down
	pod::RayQuery hit = uf::physics::rayCast(ray, world, 100.0f);

	EXPECT_TRUE(hit.hit);
	EXPECT_GT(hit.contact.penetration, 0.0f);
})

TEST(RayMesh_Miss, {
	pod::World world;
	uf::Object objMesh;

	auto mesh = impl::generateMesh(1.0f);
	auto& bodyA = uf::physics::create(world, objMesh, mesh, 0.0f);
	bodyA.transform->position = {0,0,0};

	impl::buildBroadphaseBVH( world.dynamicBvh, world.bodies );

	pod::Ray ray{ {0,2,0}, {1,0,0} }; // parallel, goes sideways
	pod::RayQuery hit = uf::physics::rayCast(ray, world, 100.0f);

	EXPECT_FALSE(hit.hit);
})

TEST(MeshMesh_Collision, {
	pod::World world;
	uf::Object objA, objB;

	auto mesh = impl::generateMesh(1.0f);

	auto& bodyA = uf::physics::create(world, objA, mesh, 0.0f);
	auto& bodyB = uf::physics::create(world, objB, mesh, 0.0f);

	bodyA.transform->position = {0,0,0};
	bodyB.transform->position = {0,0,0}; // same location

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::meshMesh(bodyA, bodyB, m);
	EXPECT_TRUE(collided);
})

TEST(MeshMesh_NoCollision, {
	pod::World world;
	uf::Object objA, objB;

	auto mesh = impl::generateMesh(1.0f);

	auto& bodyA = uf::physics::create(world, objA, mesh, 0.0f);
	auto& bodyB = uf::physics::create(world, objB, mesh, 0.0f);

	bodyA.transform->position = {0,0,0};
	bodyB.transform->position = {0,10.0f,0}; // too far apart

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::meshMesh(bodyA, bodyB, m);
	EXPECT_FALSE(collided);
})

TEST(TriangleTriangle_Collision_SimpleOverlap, {
	pod::World world;
	uf::Object objA, objB;

	pod::TriangleWithNormal triA{
		{ { {0,0,0}, {1,0,0}, {0,1,0} } },
		{0,0,1},
	};
	pod::TriangleWithNormal triB{
		{ { {0.2f,0.2f,0}, {0.8f,0.2f,0}, {0.2f,0.8f,0} } },
		{0,0,1},
	};

	auto& bodyA = uf::physics::create( world, objA, triA, 0.0f );
	auto& bodyB = uf::physics::create( world, objB, triB, 0.0f );

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::triangleTriangle(bodyA, bodyB, m);

	EXPECT_TRUE(collided);
	EXPECT_FALSE(m.points.empty());
	if ( !m.points.empty() ) {
		EXPECT_GE(m.points[0].penetration, 0.0f);
		EXPECT_NEAR(uf::vector::norm(m.points[0].normal), 1.0f, EPS);
	}
})

TEST(TriangleTriangle_Collision_CoplanarOverlap, {
	pod::World world;
	uf::Object objA, objB;

	pod::TriangleWithNormal triA{
		{ { {0,0,0}, {2,0,0}, {0,2,0} } },
		{0,0,1},
	};
	pod::TriangleWithNormal triB{
		{ { {1,1,0}, {2,1,0}, {1,2,0} } },
		{0,0,1},
	};

	auto& bodyA = uf::physics::create(world, objA, triA, 0.0f);
	auto& bodyB = uf::physics::create(world, objB, triB, 0.0f);

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::triangleTriangle(bodyA, bodyB, m);

	EXPECT_TRUE(collided);
	EXPECT_FALSE(m.points.empty());
})

TEST(TriangleTriangle_Collision_TouchingEdge, {
	pod::World world;
	uf::Object objA, objB;

	pod::TriangleWithNormal triA{
		{ { {0,0,0}, {1,0,0}, {0.5f,1,0} } },
		{0,0,1},
	};
	pod::TriangleWithNormal triB{
		{ { {0.5f,1,0}, {1.5f,0,0}, {1,1,0} } },
		{0,0,1},
	};

	auto& bodyA = uf::physics::create(world, objA, triA, 0.0f);
	auto& bodyB = uf::physics::create(world, objB, triB, 0.0f);

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::triangleTriangle(bodyA, bodyB, m);

	// Should still report as collision (tangent contact)
	EXPECT_TRUE(collided);
	EXPECT_FALSE(m.points.empty());
	if(!m.points.empty()) {
		EXPECT_NEAR(uf::vector::norm(m.points[0].normal), 1.0f, EPS);
	}
})

TEST(TriangleAabb_Collision_OverlapCenter, {
	pod::World world;
	uf::Object objA, objB;

	pod::TriangleWithNormal tri {
		{ { {0,0,0}, {1,0,0}, {0,1,0} } },
		{0,0,1},
	};

	pod::AABB box = {{0.25f, 0.25f, -0.25f}, {0.75f, 0.75f, +0.25f}};

	auto& bodyA = uf::physics::create( world, objA, tri, 0.0f );
	auto& bodyB = uf::physics::create( world, objB, box, 0.0f );

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::triangleAabb(bodyA, bodyB, m);

	EXPECT_TRUE(collided);
	EXPECT_FALSE(m.points.empty());
	if ( !m.points.empty() ) {
		EXPECT_GE(m.points[0].penetration, 0.0f);
	}
})

TEST(TriangleAabb_Collision_NoOverlap, {
	pod::World world;
	uf::Object objA, objB;

	pod::TriangleWithNormal tri {
		{ { {0,0,0}, {1,0,0}, {0,1,0} } },
		{0,0,1},
	};

	pod::AABB box = {{2,2,2}, {3,3,3}};

	auto& bodyA = uf::physics::create( world, objA, tri, 0.0f );
	auto& bodyB = uf::physics::create( world, objB, box, 0.0f );

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::triangleAabb(bodyA, bodyB, m);
	EXPECT_FALSE(collided);
})

TEST(TrianglePlane_Collision_BelowPlane, {
	pod::World world;
	uf::Object objA, objB;

	// Plane: z=0 with upward normal
	pod::Plane plane = { {0,0,1}, 0.0f };

	pod::TriangleWithNormal tri{
		{ { {0,0,-0.1f}, {1,0,-0.1f}, {0,1,-0.1f} } },
		{0,0,1},
	};

	auto& bodyA = uf::physics::create(world, objA, tri, 0.0f);
	auto& bodyB = uf::physics::create(world, objB, plane, 0.0f);

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::trianglePlane(bodyA, bodyB, m);

	EXPECT_TRUE(collided);
	EXPECT_FALSE(m.points.empty());
	if (!m.points.empty()) {
		EXPECT_GE(m.points[0].penetration, 0.0f);
	}
})

TEST(TrianglePlane_NoCollision_AbovePlane, {
	pod::World world;
	uf::Object objA, objB;

	pod::Plane plane = { {0,0,1}, 0.0f };

	pod::TriangleWithNormal tri{
		{ { {0,0,1}, {1,0,1}, {0,1,1} } },
		{0,0,-1},
		//{ {0,0,-1}, {0,0,-1}, {0,0,-1} }, // facing down, but above plane
	};

	auto& bodyA = uf::physics::create(world, objA, tri, 0.0f);
	auto& bodyB = uf::physics::create(world, objB, plane, 0.0f);

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::trianglePlane(bodyA, bodyB, m);
	EXPECT_FALSE(collided);
})

TEST(TriangleSphere_Collision_Tangent, {
	pod::World world;
	uf::Object objA, objB;

	pod::TriangleWithNormal tri{
		{ { {0,0,0}, {1,0,0}, {0,1,0} } },
		{0,0,1},
	};

	pod::Sphere sphere = { 0.5f }; // radius

	auto& bodyA = uf::physics::create(world, objA, tri, 0.0f);
	auto& bodyB = uf::physics::create(world, objB, sphere, 0.0f);
	bodyB.transform->position = pod::Vector3f{0.25f, 0.25f, 0.5f}; // exactly tangent above

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::triangleSphere(bodyA, bodyB, m);

	// At tangency: considered collision
	EXPECT_TRUE(collided);
	EXPECT_FALSE(m.points.empty());
	if(!m.points.empty()) {
		EXPECT_NEAR(m.points[0].penetration, 0.0f, EPS);
	}
})

TEST(TriangleCapsule_Collision_Overlap, {
	pod::World world;
	uf::Object objA, objB;

	pod::TriangleWithNormal tri{
		{ { {0,0,0}, {1,0,0}, {0,1,0} } },
		{0,0,1},
	};

	// Capsule aligned along Z axis, radius 0.2
	pod::Capsule capsule;
	capsule.radius = 0.2f;
	capsule.up = pod::Vector3f{0,1,0} * 1.0f; // segment lengt * 0.5fh
	// placed so capsule overlaps the tri plane
	auto& bodyA = uf::physics::create(world, objA, tri, 0.0f);
	auto& bodyB = uf::physics::create(world, objB, capsule, 0.0f);
	bodyB.transform->position = {0.25f, 0.25f, 0.1f};

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::triangleCapsule(bodyA, bodyB, m);

	EXPECT_TRUE(collided);
	EXPECT_FALSE(m.points.empty());
	if(!m.points.empty()) {
		EXPECT_GE(m.points[0].penetration, 0.0f);
	}
})

TEST(TriangleCapsule_Collision_NoOverlap, {
	pod::World world;
	uf::Object objA, objB;

	pod::TriangleWithNormal tri{
		{ { {0,0,0}, {1,0,0}, {0,1,0} } },
		{0,0,1},
	};

	pod::Capsule capsule;
	capsule.radius = 0.2f;
	capsule.up = pod::Vector3f{0,1,0} * 1.0f * 0.5f;
	// place it well above the tri plane
	auto& bodyA = uf::physics::create(world, objA, tri, 0.0f);
	auto& bodyB = uf::physics::create(world, objB, capsule, 0.0f);
	bodyB.transform->position = {0.5f, 0.5f, 2.0f};

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::triangleCapsule(bodyA, bodyB, m);
	EXPECT_FALSE(collided);
})

TEST(TriangleCapsule_Collision_Tangent, {
	pod::World world;
	uf::Object objA, objB;

	pod::TriangleWithNormal tri{
		{ { {0,0,0}, {1,0,0}, {0,1,0} } },
		{0,0,1},
	};

	pod::Capsule capsule;
	capsule.radius = 0.5f;
	capsule.up = pod::Vector3f{0,1,0} * 1.0f * 0.5f;
	auto& bodyA = uf::physics::create(world, objA, tri, 0.0f);
	auto& bodyB = uf::physics::create(world, objB, capsule, 0.0f);
	// place the capsule so its sphere-bottom just kisses the triangle
	bodyB.transform->position = {0.2f, 0.2f, 0.5f};

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::triangleCapsule(bodyA, bodyB, m);

	// At tangency, should still count as collision (penetration ≈ 0)
	EXPECT_TRUE(collided);
	EXPECT_FALSE(m.points.empty());
	if(!m.points.empty()) {
		EXPECT_NEAR(m.points[0].penetration, 0.0f, EPS);
	}
})


TEST(TriangleCapsule_Collision_EdgeAlignment, {
	pod::World world;
	uf::Object objA, objB;

	pod::TriangleWithNormal tri{
		{ { {0,0,0}, {2,0,0}, {0,2,0} } },
		{0,0,1},
	};

	pod::Capsule capsule;
	capsule.radius = 0.1f;
	capsule.up = pod::Vector3f{0,1,0} * 2.0f; // segment tall and skinn * 0.5fy
	auto& bodyA = uf::physics::create(world, objA, tri, 0.0f);
	auto& bodyB = uf::physics::create(world, objB, capsule, 0.0f);
	// lay the capsule along edge (x-axis direction near tri’s base)
	bodyB.transform->position = {1.0f, -0.05f, 0.0f};

	bodyA.bounds = impl::computeAABB( bodyA );
	bodyB.bounds = impl::computeAABB( bodyB );

	pod::Manifold m;
	bool collided = impl::triangleCapsule(bodyA, bodyB, m);

	EXPECT_TRUE(collided);
})

TEST(BallSocketJoint_Constraint, {
	pod::World world;
	uf::Object objA, objB;

	auto& bodyA = uf::physics::create(world, objA, pod::AABB{{-1,-1,-1},{1,1,1}}, 0.0f);
	auto& bodyB = uf::physics::create(world, objB, pod::AABB{{-1,-1,-1},{1,1,1}}, 1.0f);

	bodyA.transform->position = {0, 0, 0};
	bodyB.transform->position = {5, 0, 0};

	pod::Constraint constraint;
	pod::BallSocket ballSocket = {};
	ballSocket.localAnchorA = {1, 0, 0};
	ballSocket.localAnchorB = {-1, 0, 0};

	constraint.type = pod::ConstraintType::HINGE;
	constraint.a = &bodyA;
	constraint.b = &bodyB;
	constraint.ballSocket = ballSocket;

	float dt = 1.0f / 60.0f;

	for (int i = 0; i < 10; ++i) {
		bodyB.velocity.y -= 9.8f * dt;

		for (int solverIters = 0; solverIters < 10; ++solverIters) {
			impl::solveBallSocketConstraint( constraint, dt );
		}

		bodyB.transform->position += bodyB.velocity * dt;
	}

	auto tA = impl::getTransform(bodyA);
	auto tB = impl::getTransform(bodyB);
	pod::Vector3f worldAnchorA = tA.position + uf::quaternion::rotate(tA.orientation, constraint.ballSocket.localAnchorA);
	pod::Vector3f worldAnchorB = tB.position + uf::quaternion::rotate(tB.orientation, constraint.ballSocket.localAnchorB);

	float errorSq = uf::vector::distanceSquared(worldAnchorA, worldAnchorB);
	EXPECT_LT( errorSq, 0.2f );
})

TEST(Hinge_Constraint, {
	pod::World world;
	uf::Object objA, objB;

	auto& bodyA = uf::physics::create(world, objA, pod::AABB{{-1,-1,-1},{1,1,1}}, 0.0f);
	auto& bodyB = uf::physics::create(world, objB, pod::AABB{{-1,-1,-1},{1,1,1}}, 1.0f);

	bodyA.transform->position = {0, 0, 0};
	bodyA.transform->orientation = {0, 0, 0, 1};

	bodyB.transform->position = {3, 1, 0}; // expected to be at {2, 0, 0}
	bodyB.transform->orientation = uf::quaternion::axisAngle({0, 0, 1}, M_PI / 4.0f);

	bodyB.angularVelocity = {0, 2.0f, 0};

	pod::Hinge hinge = {};
	hinge.localAnchorA = {1, 0, 0};
	hinge.localAnchorB = {-1, 0, 0};
	hinge.localAxisA = {0, 1, 0};
	hinge.localAxisB = {0, 1, 0};

	pod::Constraint constraint;
	constraint.type = pod::ConstraintType::HINGE;
	constraint.a = &bodyA;
	constraint.b = &bodyB;
	constraint.hinge = hinge;

	float dt = 1.0f / 60.0f;

	for (int i = 0; i < 10; ++i) {
		bodyB.velocity.y -= 9.81f * dt;

		for (int solverIters = 0; solverIters < 10; ++solverIters) {
			impl::solveHingeConstraint( constraint, dt );
		}

		bodyB.transform->position += bodyB.velocity * dt;

		float angularSpeed2 = uf::vector::magnitude( bodyB.angularVelocity );
		if ( angularSpeed2 > 0.0001f ) {
			float angularSpeed = std::sqrt( angularSpeed2 );
			pod::Quaternion<> dq = uf::quaternion::axisAngle( bodyB.angularVelocity / angularSpeed, angularSpeed * dt );
			bodyB.transform->orientation = uf::quaternion::multiply(dq, bodyB.transform->orientation);
			bodyB.transform->orientation = uf::quaternion::normalize(bodyB.transform->orientation);
		}
	}

	auto tA = impl::getTransform(bodyA);
	auto tB = impl::getTransform(bodyB);
	pod::Vector3f worldAnchorA = tA.position + uf::quaternion::rotate(tA.orientation, constraint.hinge.localAnchorA);
	pod::Vector3f worldAnchorB = tB.position + uf::quaternion::rotate(tB.orientation, constraint.hinge.localAnchorB);

	float posErrorSq = uf::vector::distanceSquared(worldAnchorA, worldAnchorB);
	EXPECT_LT( posErrorSq, 0.01f );

	pod::Vector3f worldAxisA = uf::quaternion::rotate(tA.orientation, constraint.hinge.localAxisA);
	pod::Vector3f worldAxisB = uf::quaternion::rotate(tB.orientation, constraint.hinge.localAxisB);

	float axisDot = uf::vector::dot(worldAxisA, worldAxisB);
	EXPECT_GT( axisDot, 0.99f );
})

TEST(ConeTwist_Constraint, {
    pod::World world;
    uf::Object objA, objB;

    auto& bodyA = uf::physics::create(world, objA, pod::AABB{{-1,-1,-1},{1,1,1}}, 0.0f);
    auto& bodyB = uf::physics::create(world, objB, pod::AABB{{-1,-1,-1},{1,1,1}}, 1.0f);

    bodyA.transform->position = {0, 0, 0};
    bodyA.transform->orientation = {0, 0, 0, 1};

    bodyB.transform->position = {3, 1, 0};
    bodyB.transform->orientation = {0, 0, 0, 1};

    bodyB.angularVelocity = {10.0f, 0.0f, 10.0f};

    pod::ConeTwist coneTwist = {};
    coneTwist.localAnchorA = {1, 0, 0};
    coneTwist.localAnchorB = {-1, 0, 0};

    coneTwist.localTwistAxisA = {1, 0, 0};
    coneTwist.localTwistAxisB = {1, 0, 0};

    coneTwist.localReferenceAxisA = {0, 1, 0};
    coneTwist.localReferenceAxisB = {0, 1, 0};

    coneTwist.swingLimit = M_PI / 4.0f; // 45 degrees
    coneTwist.twistLimit = M_PI / 8.0f; // 22.5 degrees

    pod::Constraint constraint;
    constraint.type = pod::ConstraintType::CONE_TWIST;
    constraint.a = &bodyA;
    constraint.b = &bodyB;
    constraint.coneTwist = coneTwist;

    float dt = 1.0f / 60.0f;

    for (int i = 0; i < 60; ++i) {
        bodyB.velocity.y -= 9.81f * dt;

        for (int solverIters = 0; solverIters < 10; ++solverIters) {
        	impl::solveConeTwistConstraint( constraint, dt );
        }

        bodyB.transform->position += bodyB.velocity * dt;

        float angularSpeedSq = uf::vector::dot(bodyB.angularVelocity, bodyB.angularVelocity);
        if ( angularSpeedSq > 0.0001f ) {
            float angularSpeed = std::sqrt( angularSpeedSq );
            pod::Quaternion<> dq = uf::quaternion::axisAngle( bodyB.angularVelocity / angularSpeed, angularSpeed * dt );
            bodyB.transform->orientation = uf::quaternion::multiply(dq, bodyB.transform->orientation);
            bodyB.transform->orientation = uf::quaternion::normalize(bodyB.transform->orientation);
        }
    }

    auto tA = impl::getTransform(bodyA);
    auto tB = impl::getTransform(bodyB);

    pod::Vector3f worldAnchorA = tA.position + uf::quaternion::rotate(tA.orientation, constraint.coneTwist.localAnchorA);
    pod::Vector3f worldAnchorB = tB.position + uf::quaternion::rotate(tB.orientation, constraint.coneTwist.localAnchorB);

    float posErrorSq = uf::vector::distanceSquared(worldAnchorA, worldAnchorB);
    EXPECT_LT( posErrorSq, 0.01f );

    pod::Vector3f worldTwistA = uf::quaternion::rotate(tA.orientation, constraint.coneTwist.localTwistAxisA);
    pod::Vector3f worldTwistB = uf::quaternion::rotate(tB.orientation, constraint.coneTwist.localTwistAxisB);

    float swingDot = uf::vector::dot(worldTwistA, worldTwistB);

    swingDot = std::clamp(swingDot, -1.0f, 1.0f);
    float swingAngle = std::acos(swingDot);

    EXPECT_LE( swingAngle, coneTwist.swingLimit + 0.05f );

    pod::Vector3f worldRefA = uf::quaternion::rotate(tA.orientation, constraint.coneTwist.localReferenceAxisA);
    pod::Vector3f worldRefB = uf::quaternion::rotate(tB.orientation, constraint.coneTwist.localReferenceAxisB);

    pod::Vector3f projectedRefB = worldRefB - (worldTwistA * uf::vector::dot(worldRefB, worldTwistA));
    float projectedLength2 = uf::vector::dot(projectedRefB, projectedRefB);
    if ( projectedLength2 > 0.0001f ) {
        projectedRefB = projectedRefB / std::sqrt(projectedLength2);

        pod::Vector3f crossRef = uf::vector::cross(worldRefA, projectedRefB);
        float sinTheta = uf::vector::dot(crossRef, worldTwistA);
        float cosTheta = uf::vector::dot(worldRefA, projectedRefB);
        float twistAngle = std::atan2(sinTheta, cosTheta);

        EXPECT_LE( std::fabs(twistAngle), coneTwist.twistLimit + 0.05f );
    }
})

TEST(DistanceJoint_Constraint, {
    pod::World world;
    uf::Object objA, objB;

    auto& bodyA = uf::physics::create(world, objA, pod::AABB{{-1,-1,-1},{1,1,1}}, 0.0f);
    auto& bodyB = uf::physics::create(world, objB, pod::AABB{{-1,-1,-1},{1,1,1}}, 1.0f);

    bodyA.transform->position = {0, 10, 0};
    bodyB.transform->position = {0, 5, 0};

    pod::Constraint constraint;
    constraint.type = pod::ConstraintType::DISTANCE;
    constraint.a = &bodyA;
    constraint.b = &bodyB;

    constraint.distance.localAnchorA = {0, 0, 0};
    constraint.distance.localAnchorB = {0, 0, 0};
    constraint.distance.targetDistance = 5.0f;
    constraint.distance.isRope = false;
    constraint.distance.accumulatedImpulse = 0.0f;

    float dt = 1.0f / 60.0f;

    for (int i = 0; i < 60; ++i) {
        bodyB.velocity.y -= 9.8f * dt;

        for (int solverIters = 0; solverIters < 10; ++solverIters) {
            impl::solveDistanceConstraint( constraint, dt );
        }

        bodyB.transform->position += bodyB.velocity * dt;
    }

    auto tA = impl::getTransform(bodyA);
    auto tB = impl::getTransform(bodyB);
    pod::Vector3f worldAnchorA = tA.position + uf::quaternion::rotate(tA.orientation, constraint.distance.localAnchorA);
    pod::Vector3f worldAnchorB = tB.position + uf::quaternion::rotate(tB.orientation, constraint.distance.localAnchorB);

    float currentDistance = std::sqrt(uf::vector::distanceSquared(worldAnchorA, worldAnchorB));
    float error = std::abs(currentDistance - constraint.distance.targetDistance);

    EXPECT_LT( error, 0.1f );
})

TEST(WeldJoint_Constraint, {
    pod::World world;
    uf::Object objA, objB;

    auto& bodyA = uf::physics::create(world, objA, pod::AABB{{-1,-1,-1},{1,1,1}}, 0.0f);
    auto& bodyB = uf::physics::create(world, objB, pod::AABB{{-1,-1,-1},{1,1,1}}, 1.0f);

    bodyA.transform->position = {0, 0, 0};
    bodyB.transform->position = {2, 0, 0};

    pod::Constraint constraint;
    constraint.type = pod::ConstraintType::WELD;
    constraint.a = &bodyA;
    constraint.b = &bodyB;

    constraint.weld.localAnchorA = {1, 0, 0};
    constraint.weld.localAnchorB = {-1, 0, 0};
    constraint.weld.localAxisA = {1, 0, 0};
    constraint.weld.localAxisB = {1, 0, 0};
    constraint.weld.localReferenceAxisA = {0, 1, 0};
    constraint.weld.localReferenceAxisB = {0, 1, 0};

    float dt = 1.0f / 60.0f;

    for (int i = 0; i < 30; ++i) {
        bodyB.velocity.y -= 10.0f * dt;
        bodyB.angularVelocity.z += 5.0f * dt;

        for (int solverIters = 0; solverIters < 10; ++solverIters) {
            impl::solveWeldConstraint( constraint, dt );
        }

        bodyB.transform->position += bodyB.velocity * dt;
    }

    auto tA = impl::getTransform(bodyA);
    auto tB = impl::getTransform(bodyB);
    pod::Vector3f worldAnchorA = tA.position + uf::quaternion::rotate(tA.orientation, constraint.weld.localAnchorA);
    pod::Vector3f worldAnchorB = tB.position + uf::quaternion::rotate(tB.orientation, constraint.weld.localAnchorB);

    float posErrorSq = uf::vector::distanceSquared(worldAnchorA, worldAnchorB);
    EXPECT_LT( posErrorSq, 0.1f );
})

TEST(SliderJoint_Constraint, {
    pod::World world;
    uf::Object objA, objB;

    auto& bodyA = uf::physics::create(world, objA, pod::AABB{{-1,-1,-1},{1,1,1}}, 0.0f);
    auto& bodyB = uf::physics::create(world, objB, pod::AABB{{-1,-1,-1},{1,1,1}}, 1.0f);

    bodyA.transform->position = {0, 0, 0};
    bodyB.transform->position = {2, 0, 0};

    pod::Constraint constraint;
    constraint.type = pod::ConstraintType::SLIDER;
    constraint.a = &bodyA;
    constraint.b = &bodyB;

    constraint.slider.localAnchorA = {0, 0, 0};
    constraint.slider.localAnchorB = {0, 0, 0};
    constraint.slider.localAxisA = {1, 0, 0};
    constraint.slider.localAxisB = {1, 0, 0};
    constraint.slider.localReferenceAxisA = {0, 1, 0};
    constraint.slider.localReferenceAxisB = {0, 1, 0};
    constraint.slider.lowerLimit = -5.0f;
    constraint.slider.upperLimit = 5.0f;

    float dt = 1.0f / 60.0f;

    for (int i = 0; i < 60; ++i) {
        bodyB.velocity.x += 50.0f * dt;

        for (int solverIters = 0; solverIters < 10; ++solverIters) {
            impl::solveSliderConstraint( constraint, dt );
        }

        bodyB.transform->position += bodyB.velocity * dt;
    }

    auto tA = impl::getTransform(bodyA);
    auto tB = impl::getTransform(bodyB);
    pod::Vector3f relPos = tB.position - tA.position;
    float slideDistance = uf::vector::dot(relPos, constraint.slider.localAxisA);

    EXPECT_LT( slideDistance, constraint.slider.upperLimit + 0.2f );
})

TEST(SpringJoint_Constraint, {
    pod::World world;
    uf::Object objA, objB;

    auto& bodyA = uf::physics::create(world, objA, pod::AABB{{-1,-1,-1},{1,1,1}}, 0.0f);
    auto& bodyB = uf::physics::create(world, objB, pod::AABB{{-1,-1,-1},{1,1,1}}, 1.0f);

    bodyA.transform->position = {0, 10, 0};
    bodyB.transform->position = {0, 0, 0};

    pod::Constraint constraint;
    constraint.type = pod::ConstraintType::SPRING;
    constraint.a = &bodyA;
    constraint.b = &bodyB;

    constraint.spring.localAnchorA = {0, 0, 0};
    constraint.spring.localAnchorB = {0, 0, 0};
    constraint.spring.restLength = 5.0f;
    constraint.spring.stiffness = 50.0f;
    constraint.spring.damping = 5.0f;
    constraint.spring.accumulatedImpulse = 0.0f;

    float dt = 1.0f / 60.0f;

    for (int i = 0; i < 180; ++i) {
        bodyB.velocity.y -= 9.8f * dt;

        for (int solverIters = 0; solverIters < 10; ++solverIters) {
            impl::solveSpringConstraint( constraint, dt );
        }

        bodyB.transform->position += bodyB.velocity * dt;
    }

    auto tA = impl::getTransform(bodyA);
    auto tB = impl::getTransform(bodyB);
    float distance = std::sqrt(uf::vector::distanceSquared(tA.position, tB.position));

    EXPECT_LT( distance, constraint.spring.restLength + 2.0f );
    EXPECT_GT( distance, constraint.spring.restLength - 0.1f );
})