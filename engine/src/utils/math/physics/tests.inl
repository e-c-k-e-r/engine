#include <uf/utils/tests/tests.h>

#define FRAMERATE 60
#define INV_FRAMERATE 1.0f / FRAMERATE

#define PHYSICS_STEP(time) for ( int i = 0; i < time * FRAMERATE; i++ ) uf::physics::impl::step(world, INV_FRAMERATE);

namespace {
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

		return mesh;
	}
}

// list of unit tests to "standardly" verify the system works, but honestly this is a mess
#if 0
TEST(SphereSphere_Collision, {
	pod::World world;
	uf::Object objA, objB;
	auto& bodyA = uf::physics::impl::create(world, objA, pod::Sphere{1.0f}, 1.0f);
	auto& bodyB = uf::physics::impl::create(world, objB, pod::Sphere{1.0f}, 1.0f);

	bodyA.transform->position = {0,0,0};
	bodyB.transform->position = {1.5f,0,0}; // closer than sum of radii (2.0)

	pod::Manifold m;
	bool collided = sphereSphere(bodyA, bodyB, m);
	EXPECT_TRUE(collided);
	EXPECT_TRUE(!m.points.empty());
	EXPECT_NEAR(m.points[0].penetration, 0.5f, EPS(1e-4f));
})

TEST(AabbAabb_Collision, {
	pod::World world;
	uf::Object objA, objB;
	pod::AABB box = { {-1,-1,-1}, {1,1,1} };

	auto& bodyA = uf::physics::impl::create(world, objA, box, 1.0f);
	auto& bodyB = uf::physics::impl::create(world, objB, box, 1.0f);

	bodyA.transform->position = {0,0,0};
	bodyB.transform->position = {1.5f,0,0}; // overlap in x-axis

	pod::Manifold m;
	bool collided = aabbAabb(bodyA, bodyB, m);
	EXPECT_TRUE(collided);
	EXPECT_TRUE(!m.points.empty());
})

TEST(RaySphere_Hit, {
	pod::World world;
	uf::Object obj;
	auto& body = uf::physics::impl::create(world, obj, pod::Sphere{1.0f}, 1.0f);
	body.transform->position = {0,0,0};

	pod::Ray ray{ {0,0,-5}, uf::vector::normalize(pod::Vector3f{0,0,1}) };
	pod::RayQuery hit = uf::physics::impl::rayCast(ray, world, 100.0f);

	EXPECT_TRUE(hit.hit);
	EXPECT_NEAR(hit.contact.penetration, 4.0f, EPS(1e-4f));
})

TEST(SphereSphere_NoCollision, {
	pod::World world;
	uf::Object objA, objB;
	auto& bodyA = uf::physics::impl::create(world, objA, pod::Sphere{1.0f}, 1.0f);
	auto& bodyB = uf::physics::impl::create(world, objB, pod::Sphere{1.0f}, 1.0f);

	bodyA.transform->position = {0,0,0};
	bodyB.transform->position = {5.0f,0,0}; // too far

	pod::Manifold m;
	bool collided = sphereSphere(bodyA, bodyB, m);
	EXPECT_TRUE(!collided);
})

TEST(SphereAabb_Collision, {
	pod::World world;
	uf::Object objSphere, objBox;

	auto& sphere = uf::physics::impl::create(world, objSphere, pod::Sphere{1.0f}, 1.0f);
	auto& box = uf::physics::impl::create(world, objBox, pod::AABB{{-1,-1,-1}, {1,1,1}}, 1.0f);

	sphere.transform->position = {0.5f, 0.0f, 0.0f}; // overlapping inside box
	box.transform->position = {0,0,0};

	pod::Manifold m;
	bool collided = sphereAabb(sphere, box, m);
	EXPECT_TRUE(collided);
	EXPECT_TRUE(!m.points.empty());
	EXPECT_TRUE(m.points[0].penetration > 0.0f);
})

TEST(SpherePlane_Collision, {
	pod::World world;
	uf::Object objSphere, objPlane;

	auto& sphere = uf::physics::impl::create(world, objSphere, pod::Sphere{1.0f}, 1.0f);
	auto& plane = uf::physics::impl::create(world, objPlane, pod::Plane{{0,1,0},0.0f}, 0.0f);

	// Place sphere so it's intersecting the plane
	sphere.transform->position = {0,0.5f,0};

	pod::Manifold m;
	bool collided = planeSphere(plane, sphere, m);
	EXPECT_TRUE(collided);
	EXPECT_TRUE(!m.points.empty());
	EXPECT_NEAR(m.points[0].penetration, 0.5f, EPS(1e-4f));
})

TEST(SpherePlane_NoCollision, {
	pod::World world;
	uf::Object objSphere, objPlane;

	auto& sphere = uf::physics::impl::create(world, objSphere, pod::Sphere{1.0f}, 1.0f);
	auto& plane = uf::physics::impl::create(world, objPlane, pod::Plane{{0,1,0},0.0f}, 0.0f);

	sphere.transform->position = {0, 5.0f, 0}; // clearly above
	pod::Manifold m;
	bool collided = planeSphere(plane, sphere, m);
	EXPECT_TRUE(!collided);
})

TEST(CapsuleCapsule_Collision, {
	pod::World world;
	uf::Object objA, objB;
	auto& capA = uf::physics::impl::create(world, objA, pod::Capsule{0.5f, 1.0f}, 1.0f);
	auto& capB = uf::physics::impl::create(world, objB, pod::Capsule{0.5f, 1.0f}, 1.0f);

	capA.transform->position = {0,0,0};
	capB.transform->position = {0.8f,0,0}; // slight overlap

	pod::Manifold m;
	bool collided = capsuleCapsule(capA, capB, m);
	EXPECT_TRUE(collided);
	EXPECT_TRUE(!m.points.empty());
	EXPECT_TRUE(m.points[0].penetration > 0.0f);
})

TEST(RayAabb_Miss, {
	pod::World world;
	uf::Object obj;
	auto& box = uf::physics::impl::create(world, obj, pod::AABB{{-1,-1,-1},{1,1,1}}, 1.0f);
	box.transform->position = {0,0,0};

	pod::Ray ray{{5,5,5}, uf::vector::normalize(pod::Vector3f{1,0,0})};
	auto hit = uf::physics::impl::rayCast(ray, world, 100.0f);
	EXPECT_TRUE(!hit.hit);
})

// GJK shouldn't be used for sphere sphere
#if 0
TEST(Gjk_SphereSphereOverlap, {
	pod::World world;
	uf::Object objA, objB;
	auto& a = uf::physics::impl::create(world, objA, pod::Sphere{1.0f}, 1.0f);
	auto& b = uf::physics::impl::create(world, objB, pod::Sphere{1.0f}, 1.0f);

	a.transform->position = {0,0,0};
	b.transform->position = {1.5f,0,0};

	pod::Simplex simplex;
	bool inside = gjk(a, b, simplex);
	EXPECT_TRUE(inside);
	auto contact = epa(a, b, simplex);
	EXPECT_TRUE(contact.penetration > 0.0f);
})
#endif

TEST(PhysicsStep_Gravity, {
	pod::World world;
	uf::Object obj;
	auto& body = uf::physics::impl::create(world, obj, pod::Sphere{1.0f}, 1.0f);
	body.transform->position = {0, 10, 0};
	body.velocity = {0,0,0};

	PHYSICS_STEP(1)

	EXPECT_NEAR(body.transform->position.y, 10.0f - world.gravity.y, 0.05f);
})

TEST(PhysicsStep_SpherePlane_Bounce, {
	pod::World world;
	uf::Object objSphere, objPlane;

	auto& sphere = uf::physics::impl::create(world, objSphere, pod::Sphere{1.0f}, 1.0f);
	auto& plane  = uf::physics::impl::create(world, objPlane, pod::Plane{{0,1,0}, 0.0f}, 0.0f);

	sphere.transform->position = {0, 2, 0};
	sphere.material.restitution = 1.0f;

	PHYSICS_STEP(1)

	// After bouncing, sphere should be near plane surface, not sinking below
	EXPECT_TRUE(sphere.transform->position.y >= 0.9f);
	EXPECT_TRUE(fabs(sphere.velocity.y) < 10.0f); // should have reversed sign at least once
})

TEST(PhysicsStep_AabbStacking, {
	pod::World world;
	uf::Object bottomObj, fallingObj;

	auto& bottom = uf::physics::impl::create(world, bottomObj, pod::AABB{{-1,-1,-1},{1,1,1}}, 0.0f);
	bottom.transform->position = {0,0,0};

	auto& falling = uf::physics::impl::create(world, fallingObj, pod::AABB{{-1,-1,-1},{1,1,1}}, 1.0f);
	falling.transform->position = {0,5,0};

	PHYSICS_STEP(5);

	// After time, falling cube should rest on top of static one
	EXPECT_TRUE(falling.transform->position.y > 1.9f);
	EXPECT_TRUE(fabs(falling.velocity.y) < 0.1f);
})

TEST(PhysicsStep_SphereSphere_HeadOn, {
	pod::World world;
	uf::Object objA, objB;

	auto& A = uf::physics::impl::create(world, objA, pod::Sphere{1.0f}, 1.0f);
	auto& B = uf::physics::impl::create(world, objB, pod::Sphere{1.0f}, 1.0f);

	A.transform->position = {-5,0,0};
	B.transform->position = { 5,0,0};
	A.velocity = { 5,0,0};
	B.velocity = {-5,0,0};

	PHYSICS_STEP(5);

	// Expect velocities swapped (perfect elastic bounce with equal masses)
	EXPECT_TRUE(A.velocity.x < 0.0f);
	EXPECT_TRUE(B.velocity.x > 0.0f);
})

TEST(PhysicsStep_RaycastDynamic, {
	pod::World world;
	uf::Object obj;
	world.gravity = {};
	auto& body = uf::physics::impl::create(world, obj, pod::Sphere{1.0f}, 1.0f);
	body.transform->position = {0,0,0};
	body.velocity = {0,0,10};

	PHYSICS_STEP(1);

	pod::Ray ray{ {0,0,-5}, {0,0,1} };
	pod::RayQuery q = uf::physics::impl::rayCast(ray, world, 100.0f);
	EXPECT_TRUE(q.hit);
	EXPECT_TRUE(fabs(q.contact.point.z - 10.0f) <= 1.0f); // near where it moved
})

TEST(SphereSphere_TouchingButNotOverlapping, {
	pod::World world;
	uf::Object objA, objB;
	auto& a = uf::physics::impl::create(world, objA, pod::Sphere{1.0f}, 1.0f);
	auto& b = uf::physics::impl::create(world, objB, pod::Sphere{1.0f}, 1.0f);
	a.transform->position = {0,0,0};
	b.transform->position = {2.0f,0,0}; // exactly touching

	pod::Manifold m;
	bool collided = sphereSphere(a, b, m);

	EXPECT_TRUE(collided);	   // should count as a collision…
	EXPECT_NEAR(m.points[0].penetration, 0.0f, EPS(1e-6f));
})

TEST(RaySphere_OriginInside, {
	pod::World world;
	uf::Object obj;
	auto& body = uf::physics::impl::create(world, obj, pod::Sphere{2.0f}, 1.0f);
	body.transform->position = {0,0,0};

	pod::Ray ray{ {0,0,0}, {1,0,0} }; // starts inside
	auto q = uf::physics::impl::rayCast(ray, world, 100.0f);

	EXPECT_TRUE(q.hit);
	EXPECT_NEAR(q.contact.penetration, 0.0f, EPS(1e-6f));
})

TEST(PhysicsStep_StaticFriction_Holds, {
	pod::World world;
	uf::Object objSphere, objPlane;

	auto& sphere = uf::physics::impl::create(world, objSphere, pod::Sphere{1.0f}, 1.0f);
	auto& plane  = uf::physics::impl::create(world, objPlane, pod::Plane{{0,1,0}, 0.0f}, 0.0f);

	world.gravity = {0,-9.81f,0};
	sphere.transform->position = {0,1.0f,0};
	sphere.material.staticFriction = 2.0f; // stronger grip to cover solver slop

	// Apply smaller force (well below μ_s * N)
	uf::physics::impl::applyForce(sphere, {2,0,0});

	PHYSICS_STEP(1);

	EXPECT_NEAR(sphere.transform->position.x, 0.0f, 0.05f); // allow tiny error
})

TEST(PhysicsStep_StaticFriction_Slips, {
	pod::World world;
	uf::Object objSphere, objPlane;

	auto& sphere = uf::physics::impl::create(world, objSphere, pod::Sphere{1.0f}, 1.0f);
	auto& plane  = uf::physics::impl::create(world, objPlane, pod::Plane{{0,1,0}, 0.0f}, 0.0f);

	world.gravity = {0,-9.81f,0};
	sphere.transform->position = {0,1.0f,0};
	sphere.material.staticFriction = 1.0f;

	uf::physics::impl::applyForce(sphere, {15,0,0}); // Above limit

	PHYSICS_STEP(1);

	EXPECT_TRUE(fabs(sphere.transform->position.x) > 0.1f); // It should slide
})

TEST(CapsulePlane_Slope_StaticHold, {
	pod::World world;
	uf::Object objCap, objPlane;
	auto& cap = uf::physics::impl::create(world, objCap, pod::Capsule{0.5f,1.0f}, 1.0f);
	auto& plane = uf::physics::impl::create(world, objPlane, pod::Plane{{0,1,1},0.0f}, 0.0f);

	// Place capsule on slope
	cap.transform->position = {0,2,0};
	cap.material.staticFriction = 2.0f;   // More than tan(45) = 1
	cap.material.dynamicFriction = 1.0f;

	PHYSICS_STEP(5);

	EXPECT_NEAR(cap.transform->position.z, 0.0f, 0.2f); // Held in place
	EXPECT_NEAR(cap.velocity.z, 0.0f, 0.05f);
})

TEST(CapsulePlane_Slope_Slip, {
	pod::World world;
	uf::Object objCap, objPlane;
	auto& cap = uf::physics::impl::create(world, objCap, pod::Capsule{0.5f,1.0f}, 1.0f);
	auto& plane = uf::physics::impl::create(world, objPlane, pod::Plane{{0,1,1},0.0f}, 0.0f);

	cap.transform->position = {0,2,0};
	cap.material.staticFriction = 0.5f;   // Less than tan(45) => must slip
	cap.material.dynamicFriction = 0.5f;

	PHYSICS_STEP(5);

	EXPECT_TRUE(fabs(cap.transform->position.z) > 1.0f); // Should have slid downhill
})

TEST(CapsulePlane_RestingContact, {
	pod::World world;
	uf::Object objCap, objPlane;
	auto& cap = uf::physics::impl::create(world, objCap, pod::Capsule{0.5f, 1.0f}, 1.0f);
	auto& plane = uf::physics::impl::create(world, objPlane, pod::Plane{{0,1,0}, 0.0f}, 0.0f);

	cap.transform->position = {0, 1.5f, 0}; // halfHeight=1, radius=0.5, so "foot" at y=0
	cap.velocity = {0,0,0};

	PHYSICS_STEP(1);

	// Capsule should rest on the floor at y=1.5
	EXPECT_NEAR(cap.transform->position.y, 1.5f, 0.05f);
	EXPECT_NEAR(cap.velocity.y, 0.0f, 0.05f); // no jitter
})

TEST(CapsuleAabb_RestingContact, {
	pod::World world;
	uf::Object objCap, objBox;

	auto& cap = uf::physics::impl::create(world, objCap, pod::Capsule{0.5f, 1.0f}, 1.0f);
	auto& floor = uf::physics::impl::create(world, objBox, pod::AABB{{-5, -1, -5},{5, 0, 5}}, 0.0f);

	cap.transform->position = {0, 1.5f, 0};
	cap.velocity = {0,0,0};

	PHYSICS_STEP(2);

	EXPECT_NEAR(cap.transform->position.y, 1.5f, 0.05f);
	EXPECT_NEAR(cap.velocity.y, 0.0f, 0.05f);
})

TEST(CapsulePlane_Settling, {
	pod::World world;
	uf::Object objCap, objPlane;

	auto& cap = uf::physics::impl::create(world, objCap, pod::Capsule{0.5f, 1.0f}, 1.0f);
	auto& plane = uf::physics::impl::create(world, objPlane, pod::Plane{{0,1,0},0.0f}, 0.0f);

	cap.transform->position = {0, 2.0f, 0}; // slightly above
	cap.velocity = {0,0,0};

	PHYSICS_STEP(3);

	EXPECT_NEAR(cap.transform->position.y, 1.5f, 0.05f);
})

TEST(CapsulePlane_SlopeStaticFriction, {
	pod::World world;
	uf::Object objCap, objPlane;

	auto& cap = uf::physics::impl::create(world, objCap, pod::Capsule{0.5f, 1.0f}, 1.0f);
	auto& slope = uf::physics::impl::create(world, objPlane, pod::Plane{{0,1,1},0.0f}, 0.0f); // 45° slope

	cap.transform->position = {0, 3.0f, 0};
	cap.material.staticFriction = 2.0f;
	cap.material.dynamicFriction = 1.0f;

	PHYSICS_STEP(4);

	// Should not slide much if static friction is strong
	EXPECT_NEAR(cap.transform->position.z, 0.0f, 0.1f);
})

TEST(CapsuleAabb_StepEdge, {
	pod::World world;
	uf::Object objCap, objBox;

	auto& cap = uf::physics::impl::create(world, objCap, pod::Capsule{0.5f, 1.0f}, 1.0f);
	auto& floor = uf::physics::impl::create(world, objBox, pod::AABB{{0,-1,-5},{5,0,5}}, 0.0f);

	cap.transform->position = {0.25f, 1.5f, 0}; // Capsule foot half on, half off

	PHYSICS_STEP(4);

	// Should not spaz out or fall through
	EXPECT_NEAR(cap.transform->position.y, 1.5f, 0.1f);
})

TEST(Diagnostic_CapsuleGrounding, {
	pod::World world;
	uf::Object objCap, objFloor;

	// Capsule: radius 0.5, half-height 1.0 (total height 2.0 + end caps)
	auto& cap = uf::physics::impl::create(world, objCap, pod::Capsule{0.5f, 1.0f}, 1.0f);

	// Test toggle: try both AABB floors and Plane floors.
	bool usePlane = true;
	auto& floor = usePlane ? uf::physics::impl::create(world, objFloor, pod::Plane{{0,1,0}, 0.0f}, 0.0f) : uf::physics::impl::create(world, objFloor, pod::AABB{{-5,-1,-5},{5,0,5}}, 0.0f);

	cap.transform->position = {0, 3, 0}; // start a little above floor
	cap.velocity = {0,0,0};

	PHYSICS_STEP(2);

	// Final resting state: should be near Y=1.5 (halfHeight + radius)
	EXPECT_NEAR(cap.transform->position.y, 1.5f, 0.1f);
})

TEST(CapsulePlane_ContactNormal, {
	pod::World world;
	uf::Object objCap, objPlane;

	auto& cap = uf::physics::impl::create(world, objCap, pod::Capsule{0.5f, 1.0f}, 1.0f);
	auto& plane = uf::physics::impl::create(world, objPlane, pod::Plane{{0,1,0},0.0f}, 0.0f);

	cap.transform->position = {0,1.5f,0};
	pod::Manifold m;
	bool collided = capsulePlane(cap, plane, m);

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
	uf::Object playerObj, groundObj;

	auto& player = uf::physics::impl::create(world, playerObj, pod::AABB{{-0.5f,-1.0f,-0.5f},{0.5f,0.0f,0.5f}}, 1.0f);
	auto& ground = uf::physics::impl::create(world, groundObj, pod::Plane{{0,1,0},0}, 0.0f);

	player.transform->position = {0, 2.0f, 0};
	
	PHYSICS_STEP(5);		

	// Expect the player to remain on ground at y=0.0 without sinking further
	EXPECT_NEAR(player.transform->position.y, 0.0f, 0.05f);
	EXPECT_NEAR(player.velocity.y, 0.0f, 0.05f);
})

TEST(CapsuleSphere_Collision, {
	pod::World world;
	uf::Object capObj, sphObj;
	auto& cap = uf::physics::impl::create(world, capObj, pod::Capsule{0.5f,1.0f}, 1.0f);
	auto& sph = uf::physics::impl::create(world, sphObj, pod::Sphere{0.5f}, 1.0f);

	cap.transform->position = {0,0,0};
	sph.transform->position = {0,0.5f,0}; // overlap

	pod::Manifold m;
	bool collided = capsuleSphere(cap, sph, m);
	EXPECT_TRUE(collided);
	EXPECT_TRUE(!m.points.empty());
})

TEST(CapsuleSphere_NoCollision, {
	pod::World world;
	uf::Object capObj, sphObj;
	auto& cap = uf::physics::impl::create(world, capObj, pod::Capsule{0.5f,1.0f}, 1.0f);
	auto& sph = uf::physics::impl::create(world, sphObj, pod::Sphere{0.5f}, 1.0f);

	cap.transform->position = {0,0,0};
	sph.transform->position = {0,5,0}; // too far

	pod::Manifold m;
	bool collided = capsuleSphere(cap, sph, m);
	EXPECT_TRUE(!collided);
})


TEST(AabbSphere_Collision, {
	pod::World world;
	uf::Object objA, objB;
	auto& box   = uf::physics::impl::create(world, objA, pod::AABB{{-1,-1,-1},{1,1,1}}, 1.0f);
	auto& sphere= uf::physics::impl::create(world, objB, pod::Sphere{0.5f}, 1.0f);

	box.transform->position	= {0,0,0};
	sphere.transform->position = {0.75f,0,0}; // Intersecting into box

	pod::Manifold m;
	bool collided = aabbSphere(box,sphere,m);
	EXPECT_TRUE(collided);
	EXPECT_TRUE(!m.points.empty());
})

TEST(AabbSphere_NoCollision, {
	pod::World world;
	uf::Object objA, objB;
	auto& box   = uf::physics::impl::create(world, objA, pod::AABB{{-1,-1,-1},{1,1,1}}, 1.0f);
	auto& sphere= uf::physics::impl::create(world, objB, pod::Sphere{0.5f}, 1.0f);

	box.transform->position	= {0,0,0};
	sphere.transform->position = {5,0,0}; // too far away

	pod::Manifold m;
	bool collided = aabbSphere(box,sphere,m);
	EXPECT_TRUE(!collided);
})


TEST(AabbPlane_Collision, {
	pod::World world;
	uf::Object objA, objB;
	auto& box   = uf::physics::impl::create(world, objA, pod::AABB{{-1,-1,-1},{1,1,1}}, 1.0f);
	auto& plane = uf::physics::impl::create(world, objB, pod::Plane{{0,1,0},0.0f}, 0.0f);

	box.transform->position = {0,0.5f,0}; // half interpenetrating plane

	pod::Manifold m;
	bool collided = aabbPlane(box,plane,m);
	EXPECT_TRUE(collided);
	EXPECT_TRUE(!m.points.empty());
})

TEST(AabbPlane_NoCollision, {
	pod::World world;
	uf::Object objA, objB;
	auto& box   = uf::physics::impl::create(world, objA, pod::AABB{{-1,-1,-1},{1,1,1}}, 1.0f);
	auto& plane = uf::physics::impl::create(world, objB, pod::Plane{{0,1,0},0.0f}, 0.0f);

	box.transform->position = {0,5,0}; // clearly above

	pod::Manifold m;
	bool collided = aabbPlane(box,plane,m);
	EXPECT_TRUE(!collided);
})


TEST(AabbCapsule_Collision, {
	pod::World world;
	uf::Object objA, objB;
	auto& box	 = uf::physics::impl::create(world, objA, pod::AABB{{-1,-1,-1},{1,1,1}}, 1.0f);
	auto& cap	 = uf::physics::impl::create(world, objB, pod::Capsule{0.5f,1.0f}, 1.0f);

	box.transform->position = {0,0,0};
	cap.transform->position = {0,0.5f,0}; // partially overlapping

	pod::Manifold m;
	bool collided = aabbCapsule(box,cap,m);
	EXPECT_TRUE(collided);
	EXPECT_TRUE(!m.points.empty());
})

TEST(AabbCapsule_NoCollision, {
	pod::World world;
	uf::Object objA, objB;
	auto& box	 = uf::physics::impl::create(world, objA, pod::AABB{{-1,-1,-1},{1,1,1}}, 1.0f);
	auto& cap	 = uf::physics::impl::create(world, objB, pod::Capsule{0.5f,1.0f}, 1.0f);

	box.transform->position = {0,0,0};
	cap.transform->position = {0,5,0};

	pod::Manifold m;
	bool collided = aabbCapsule(box,cap,m);
	EXPECT_TRUE(!collided);
})


TEST(SphereCapsule_Collision, {
	pod::World world;
	uf::Object objA, objB;
	auto& sphere = uf::physics::impl::create(world, objA, pod::Sphere{0.5f}, 1.0f);
	auto& cap	= uf::physics::impl::create(world, objB, pod::Capsule{0.5f,1.0f}, 1.0f);

	sphere.transform->position = {0,0.0f,0};
	cap.transform->position	= {0,0.25f,0};

	pod::Manifold m;
	bool collided = sphereCapsule(sphere,cap,m);
	EXPECT_TRUE(collided);
	EXPECT_TRUE(!m.points.empty());
})

TEST(SphereCapsule_NoCollision, {
	pod::World world;
	uf::Object objA, objB;
	auto& sphere = uf::physics::impl::create(world, objA, pod::Sphere{0.5f}, 1.0f);
	auto& cap	= uf::physics::impl::create(world, objB, pod::Capsule{0.5f,1.0f}, 1.0f);

	sphere.transform->position = {0,5,0};
	cap.transform->position	= {0,0,0};

	pod::Manifold m;
	bool collided = sphereCapsule(sphere,cap,m);
	EXPECT_TRUE(!collided);
})

TEST(PlanePlane_NoCollision, {
	pod::World world;
	uf::Object objA, objB;
	auto& planeA= uf::physics::impl::create(world, objA, pod::Plane{{0,1,0},0.0f}, 0.0f);
	auto& planeB= uf::physics::impl::create(world, objB, pod::Plane{{0,0,1},0.0f}, 0.0f);

	pod::Manifold m;
	bool collided = planePlane(planeA,planeB,m);
	EXPECT_TRUE(!collided); // always false in your engine
})


TEST(PlaneCapsule_Collision, {
	pod::World world;
	uf::Object objA, objB;
	auto& plane = uf::physics::impl::create(world, objA, pod::Plane{{0,1,0},0.0f}, 0.0f);
	auto& cap   = uf::physics::impl::create(world, objB, pod::Capsule{0.5f,1.0f}, 1.0f);

	cap.transform->position = {0,0.25f,0}; // foot intersecting

	pod::Manifold m;
	bool collided = planeCapsule(plane,cap,m);
	EXPECT_TRUE(collided);
	EXPECT_TRUE(!m.points.empty());
})

TEST(PlaneCapsule_NoCollision, {
	pod::World world;
	uf::Object objA, objB;
	auto& plane = uf::physics::impl::create(world, objA, pod::Plane{{0,1,0},0.0f}, 0.0f);
	auto& cap   = uf::physics::impl::create(world, objB, pod::Capsule{0.5f,1.0f}, 1.0f);

	cap.transform->position = {0,5,0}; // far above

	pod::Manifold m;
	bool collided = planeCapsule(plane,cap,m);
	EXPECT_TRUE(!collided);
})
TEST(MeshSphere_Collision, {
	pod::World world;
	uf::Object objMesh, objSphere;

	// Create mesh body (a plane on Y=0, size=1)
	auto mesh = ::generateMesh(1.0f);
	auto& meshBody = uf::physics::impl::create(world, objMesh, mesh, 0.0f); // static mesh
	meshBody.transform->position = {0,0,0};

	// Sphere just above plane, radius 1, intersects
	auto& sphereBody = uf::physics::impl::create(world, objSphere, pod::Sphere{2.0f}, 1.0f);
	sphereBody.transform->position = {0,0.5f,0}; // half below plane (since plane is at y=0)

	pod::Manifold m;
	bool collided = meshSphere(meshBody, sphereBody, m);
	EXPECT_TRUE(collided);
	EXPECT_TRUE(!m.points.empty());
	if ( !m.points.empty() ) EXPECT_TRUE(m.points[0].penetration > 0.0f);
})

TEST(MeshSphere_NoCollision, {
	pod::World world;
	uf::Object objMesh, objSphere;

	auto mesh = ::generateMesh(1.0f);
	auto& meshBody = uf::physics::impl::create(world, objMesh, mesh, 0.0f);
	meshBody.transform->position = {0,0,0};

	auto& sphereBody = uf::physics::impl::create(world, objSphere, pod::Sphere{0.5f}, 1.0f);
	sphereBody.transform->position = {0,5.0f,0}; // far above plane

	pod::Manifold m;
	bool collided = meshSphere(meshBody, sphereBody, m);
	EXPECT_FALSE(collided);
})

TEST(MeshAabb_Collision, {
	pod::World world;
	uf::Object objMesh, objBox;

	auto mesh = ::generateMesh(2.0f);
	auto& meshBody = uf::physics::impl::create(world, objMesh, mesh, 0.0f);
	meshBody.transform->position = {0,0,0};

	pod::AABB box = { {-0.5f,-0.5f,-0.5f}, {0.5f,0.5f,0.5f} };
	auto& boxBody = uf::physics::impl::create(world, objBox, box, 1.0f);
	boxBody.transform->position = {0,0.25f,0}; // overlaps plane

	pod::Manifold m;
	bool collided = meshAabb(meshBody, boxBody, m);
	EXPECT_TRUE(collided);
	EXPECT_TRUE(!m.points.empty());
})

TEST(MeshAabb_NoCollision, {
	pod::World world;
	uf::Object objMesh, objBox;

	auto mesh = ::generateMesh(2.0f);
	auto& meshBody = uf::physics::impl::create(world, objMesh, mesh, 0.0f);
	meshBody.transform->position = {0,0,0};

	pod::AABB box = { {-0.5f,-0.5f,-0.5f}, {0.5f,0.5f,0.5f} };
	auto& boxBody = uf::physics::impl::create(world, objBox, box, 1.0f);
	boxBody.transform->position = {0,5.0f,0}; // above plane, no overlap

	pod::Manifold m;
	bool collided = meshAabb(meshBody, boxBody, m);
	EXPECT_FALSE(collided);
})

TEST(RayMesh_Hit, {
	pod::World world;
	uf::Object objMesh;

	auto mesh = ::generateMesh(1.0f);
	auto& meshBody = uf::physics::impl::create(world, objMesh, mesh, 0.0f);
	meshBody.transform->position = {0,0,0};

	pod::Ray ray{ {0,1,0}, {0,-1,0} }; // from above, pointing down
	pod::RayQuery hit = uf::physics::impl::rayCast(ray, world, 100.0f);

	EXPECT_TRUE(hit.hit);
	EXPECT_TRUE(hit.contact.penetration > 0.0f);
})

TEST(RayMesh_Miss, {
	pod::World world;
	uf::Object objMesh;

	auto mesh = ::generateMesh(1.0f);
	auto& meshBody = uf::physics::impl::create(world, objMesh, mesh, 0.0f);
	meshBody.transform->position = {0,0,0};

	pod::Ray ray{ {0,2,0}, {1,0,0} }; // parallel, goes sideways
	pod::RayQuery hit = uf::physics::impl::rayCast(ray, world, 100.0f);

	EXPECT_FALSE(hit.hit);
})

TEST(MeshMesh_Collision, {
	pod::World world;
	uf::Object objA, objB;

	auto mesh = ::generateMesh(1.0f);

	auto& meshA = uf::physics::impl::create(world, objA, mesh, 0.0f);
	auto& meshB = uf::physics::impl::create(world, objB, mesh, 0.0f);

	meshA.transform->position = {0,0,0};
	meshB.transform->position = {0,0,0}; // same location

	pod::Manifold m;
	bool collided = meshMesh(meshA, meshB, m);
	EXPECT_TRUE(collided);
})

TEST(MeshMesh_NoCollision, {
	pod::World world;
	uf::Object objA, objB;

	auto mesh = ::generateMesh(1.0f);

	auto& meshA = uf::physics::impl::create(world, objA, mesh, 0.0f);
	auto& meshB = uf::physics::impl::create(world, objB, mesh, 0.0f);

	meshA.transform->position = {0,0,0};
	meshB.transform->position = {0,10.0f,0}; // too far apart

	pod::Manifold m;
	bool collided = meshMesh(meshA, meshB, m);
	EXPECT_FALSE(collided);
})
#endif

#define EPS 1.0e-4
#if 0
TEST(TriangleTriangle_Collision_SimpleOverlap, {
	// Two identical triangles overlapping on XY plane
	pod::TriangleWithNormal triA {
		{ { {0,0,0}, {1,0,0}, {0,1,0} } },
		{ {0,0,1}, {0,0,1}, {0,0,1} },
	};
	pod::TriangleWithNormal triB {
		{ { {0.25f,0.25f,0}, {1.25f,0.25f,0},  {0.25f,1.25f,0} } },
		{ {0,0,1}, {0,0,1},  {0,0,1} },
	};

	pod::Manifold m;
	bool collided = triangleTriangle( triA, triB, m, EPS );

	EXPECT_TRUE( collided );
	if ( !m.points.empty() ) {
		EXPECT_NEAR(m.points[0].point.z, 0, EPS); // contact should be on z=0 plane
		EXPECT_NEAR(uf::vector::norm(m.points[0].normal), 1.0f, EPS);
		EXPECT_GE(m.points[0].penetration, 0.0f);
	}
})
#endif

TEST(TriangleAabb_Collision_CenterInside, {
	pod::TriangleWithNormal tri {
		{ { {0,0,0}, {1,0,0}, {0,1,0} } },
		{ {0,0,1}, {0,0,1}, {0,0,1} },
	};

	// Make cube overlapping
	pod::RigidBody box;
	box.collider.type = pod::ShapeType::AABB;
	box.collider.u.aabb.min = {0.25f, 0.25f, -0.1f};
	box.collider.u.aabb.max = {0.75f, 0.75f, +0.1f};
	box.transform->position = ::aabbCenter( box.collider.u.aabb );
	box.bounds = ::computeAABB( box );

	pod::Manifold m;
	bool collided = triangleAabb(tri, box, m, EPS);

	EXPECT_TRUE(collided);
	if ( !m.points.empty() ) {
		EXPECT_NEAR(uf::vector::norm(m.points[0].normal), 1.0f, EPS);
		EXPECT_GE(m.points[0].penetration, 0.0f);

		UF_MSG_DEBUG("contact={}, normal={}, depth={}", uf::vector::toString( m.points[0].point ), uf::vector::toString( m.points[0].normal ), m.points[0].penetration );
	}
})

TEST(TriangleSphere_Collision_SphereTouchingTriangle, {
	pod::TriangleWithNormal tri {
		{ { {0,-1,0}, {1,1,0}, {-1,1,0} } },
		{ {0,0,1}, {0,0,1}, {0,0,1} },
	};

	pod::RigidBody sphere;
	sphere.collider.type = pod::ShapeType::SPHERE;
	sphere.collider.u.sphere.radius = 1.0f;
	sphere.transform->position = {0,0,0.5f}; // Place sphere just above triangle so it penetrates slightly
	sphere.bounds = ::computeAABB( sphere );

	pod::Manifold m;
	bool collided = triangleSphere(tri, sphere, m, EPS);

	EXPECT_TRUE(collided);
	if ( !m.points.empty() ) {
		EXPECT_NEAR(m.points[0].point.z, 0.0f, 0.5f);
		EXPECT_NEAR(uf::vector::norm(m.points[0].normal), 1.0f, EPS);

		UF_MSG_DEBUG("contact={}, normal={}, depth={}", uf::vector::toString( m.points[0].point ), uf::vector::toString( m.points[0].normal ), m.points[0].penetration );
		EXPECT_GE(m.points[0].penetration, 0.0f);
	}
})

TEST(TriangleCapsule_Collision_CapsuleIntersectingEdge, {
	pod::TriangleWithNormal tri {
		{ { {0,0,0}, {2,0,0},  {1,1,0} } },
		{ {0,0,1}, {0,0,1},  {0,0,1} },
	};

	pod::RigidBody capsule;
	capsule.collider.type = pod::ShapeType::CAPSULE;
	capsule.collider.u.capsule.radius = 0.5f;
	auto [ p1, p2 ] = std::pair{ pod::Vector3f{1,0.5f,-1}, pod::Vector3f{1,0.5f,1} };
	capsule.bounds = computeSegmentAABB(p1, p2, capsule.collider.u.capsule.radius);

	pod::Manifold m;
	bool collided = triangleCapsule(tri, capsule, m, EPS);

	EXPECT_TRUE(collided);
	if ( !m.points.empty() ) {
		EXPECT_NEAR(uf::vector::norm(m.points[0].normal), 1.0f, EPS);
		EXPECT_GE(m.points[0].penetration, 0.0f);

		UF_MSG_DEBUG("contact={}, normal={}, depth={}", uf::vector::toString( m.points[0].point ), uf::vector::toString( m.points[0].normal ), m.points[0].penetration );
	}
})

TEST(TriangleSphere_Collision_SphereTangentFace, {
    // Triangle is in Z=0 plane
    pod::TriangleWithNormal tri {
        { { {0,-1,0}, {1,1,0}, {-1,1,0} } },
        { {0,0,1}, {0,0,1}, {0,0,1} },
    };

    pod::RigidBody sphere;
    sphere.collider.type = pod::ShapeType::SPHERE;
    sphere.collider.u.sphere.radius = 1.0f;
    sphere.transform->position = {0, 0, 1.0f}; // center exactly 1 unit above plane
    sphere.bounds = ::computeAABB(sphere);

    pod::Manifold m;
    bool collided = triangleSphere(tri, sphere, m, EPS);

    // Should either detect a grazing collision or at least not error
    EXPECT_TRUE(collided);
    if (!m.points.empty()) {
        EXPECT_NEAR(m.points[0].penetration, 0.0f, 1e-4f);
        EXPECT_NEAR(uf::vector::norm(m.points[0].normal), 1.0f, EPS);
    }
})

TEST(TriangleSphere_Collision_SphereTangentEdge, {
    // Triangle tilted in XY plane, edge from (0,0,0) to (1,0,0)
    pod::TriangleWithNormal tri {
        { { {0,0,0}, {1,0,0}, {0,1,0} } },
        { {0,0,1}, {0,0,1}, {0,0,1} },
    };

    pod::RigidBody sphere;
    sphere.collider.type = pod::ShapeType::SPHERE;
    sphere.collider.u.sphere.radius = 0.5f;

    // Place sphere center exactly 0.5 units away from edge line
    sphere.transform->position = {0.5f, -0.5f, 0.0f};
    sphere.bounds = ::computeAABB(sphere);

    pod::Manifold m;
    bool collided = triangleSphere(tri, sphere, m, EPS);

    EXPECT_TRUE(collided); // Tangential along edge
    if (!m.points.empty()) {
        EXPECT_NEAR(m.points[0].penetration, 0.0f, 1e-4f);
    }
})

TEST(TriangleCapsule_Collision_TangentVertex, {
    pod::TriangleWithNormal tri {
        { { {0,0,0}, {1,0,0}, {0,1,0} } },
        { {0,0,1}, {0,0,1}, {0,0,1} },
    };

    pod::RigidBody capsule;
    capsule.collider.type = pod::ShapeType::CAPSULE;
    capsule.collider.u.capsule.radius = 0.25f;

    // Align segment so it hovers exactly through the vertex (0,0,0)
    pod::Vector3f p1{0.0f, -0.25f, 0.0f};
    pod::Vector3f p2{0.0f, -0.25f, 1.0f};

    capsule.bounds = computeSegmentAABB(p1, p2, capsule.collider.u.capsule.radius);

    pod::Manifold m;
    bool collided = triangleCapsule(tri, capsule, m, EPS);

    EXPECT_TRUE(collided);
    if (!m.points.empty()) {
        EXPECT_NEAR(m.points[0].penetration, 0.0f, 1e-4f);
    }
})