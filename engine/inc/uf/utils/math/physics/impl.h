#pragma once

#include <uf/config.h>
#include <uf/utils/time/time.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/math/quaternion.h>
#include <uf/utils/math/matrix.h>
#include <uf/utils/math/shapes.h>
#include <uf/utils/math/transform.h>

#include <uf/utils/memory/vector.h>
#include <uf/utils/memory/unordered_map.h>
#include <uf/utils/memory/unordered_set.h>
#include <uf/utils/memory/stack.h>

#include <uf/engine/object/object.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/utils/math/quant.h>

#include <cfloat>

#define EPS 1.0e-6f
#define EPS2 (EPS * EPS)
#define ASSERT_COLLIDER_TYPES( A, B ) UF_ASSERT( a.collider.type == pod::ShapeType::A && b.collider.type == pod::ShapeType::B );

#define REORIENT_NORMALS_ON_FETCH 0
#define REORIENT_NORMALS_ON_CONTACT 1

#define REVERSE_COLLIDER( a, b, fun )\
	auto start = manifold.points.size();\
	if ( !::fun( b, a, manifold ) ) return false;\
	for ( auto i = start; i < manifold.points.size(); ++i ) manifold.points[i].normal = -manifold.points[i].normal;\
	return true;

namespace pod {
	enum class ShapeType {
		AABB,
		OBB,
		SPHERE,
		PLANE,
		CAPSULE,
		TRIANGLE,
		MESH,
		CONVEX_HULL,
	};

	struct SupportPoint {
		/*alignas(16)*/ pod::Vector3f p;
		/*alignas(16)*/ pod::Vector3f pA;
		/*alignas(16)*/ pod::Vector3f pB;
	};

	struct Simplex {
		uf::stl::vector<pod::SupportPoint> pts;
	};

	struct Face {
		pod::SupportPoint a, b, c;
		/*alignas(16)*/ pod::Vector3f normal;
		float distance;
	};

	struct BVH {
		typedef uint32_t index_t;
		typedef std::pair<index_t,index_t> pair_t;
		
		struct PairHash {
			size_t operator()( const pair_t& p ) const noexcept {
				uint64_t a = (uint64_t) std::min(p.first, p.second);
				uint64_t b = (uint64_t) std::max(p.first, p.second);
				return (a << 32) ^ b;
			}
		};
		struct PairEq {
			bool operator()( const pair_t& a, const pair_t& b ) const noexcept {
				return (a.first == b.first && a.second == b.second) || (a.first == b.second && a.second == b.first);
			}
		};

	//	typedef uf::stl::unordered_set<pair_t, PairHash, PairEq> pairs_t;
		typedef uf::stl::vector<pair_t> pairs_t;
		
		struct Node {
			BVH::index_t left = 0;
			BVH::index_t right = 0;
			BVH::index_t start = 0;
			BVH::index_t flags = 0;

			BVH::index_t getCount() const { return flags & 0x7FFFFFFF; }
			bool isAsleep() const { return (flags & 0x80000000u) != 0; }
			void setCount(BVH::index_t c) { flags = (flags & 0x80000000u) | (c & 0x7FFFFFFF); }
			void setAsleep(bool a) { flags = (flags & 0x7FFFFFFF) | (a ? 0x80000000u : 0); }
		};
		struct FlatNode {
			BVH::index_t start = 0;
			BVH::index_t skipIndex = 0;
			BVH::index_t flags = 0;

			BVH::index_t getCount() const { return flags & 0x7FFFFFFF; }
			bool isAsleep() const { return (flags & 0x80000000u) != 0; }
			void setCount(BVH::index_t c) { flags = (flags & 0x80000000u) | (c & 0x7FFFFFFF); }
			void setAsleep(bool a) { flags = (flags & 0x7FFFFFFF) | (a ? 0x80000000u : 0); }
		};
		struct UpdatePolicy {
			enum class Decision {
				NONE,	// do nothing
				REFIT,   // refit bounds
				REBUILD  // rebuild from scratch
			};
			float displacementThreshold = 0.25f; // 25% of AABB size
			float overlapThreshold = 2.0f;	   // 2x growth in root surface area
			float dirtyRatioThreshold = 0.3f;	// 30% dirty bodies
			uint16_t maxFramesBeforeRebuild = 600;   // force rebuild every 600 frames
		};

		bool dirty = false;
		uf::stl::vector<pod::BVH::index_t> indices;
		uf::stl::vector<pod::BVH::Node> nodes;
		uf::stl::vector<pod::BVH::FlatNode> flattened;
		
		uf::stl::vector<pod::AABB> bounds;
		uf::stl::vector<pod::AABB> flatBounds;
	};

	struct MeshBVH {
		pod::BVH* bvh;
		const uf::Mesh* mesh;
	};
	typedef MeshBVH ConvexHullBVH;

	typedef uint32_t CollisionMask;
	struct Collider {
		// what it is
		enum CategoryMask : uint32_t {
			CATEGORY_NONE			= 0,
			CATEGORY_STATIC			= 1 << 0,
			CATEGORY_DYNAMIC		= 1 << 1,
			CATEGORY_PLAYER	  		= 1 << 2,
			CATEGORY_NPC			= 1 << 3,
			CATEGORY_TRIGGER		= 1 << 4,
			CATEGORY_PROJECTILE		= 1 << 5,
			CATEGORY_CHARACTER		= CATEGORY_PLAYER | CATEGORY_NPC,
			CATEGORY_ALL			= 0xFFFFFFFF
		};
		// what it collides with
		enum CollisionMask : uint32_t {
			MASK_NONE				= 0,
			MASK_STATIC				= CATEGORY_DYNAMIC | CATEGORY_PLAYER | CATEGORY_NPC | CATEGORY_PROJECTILE,
			MASK_DYNAMIC			= CATEGORY_STATIC | CATEGORY_DYNAMIC | CATEGORY_PLAYER | CATEGORY_NPC,
			MASK_PLAYER				= CATEGORY_STATIC | CATEGORY_DYNAMIC | CATEGORY_NPC | CATEGORY_PROJECTILE,
			MASK_NPC				= CATEGORY_STATIC | CATEGORY_DYNAMIC | CATEGORY_PLAYER | CATEGORY_PROJECTILE,
			MASK_TRIGGER			= CATEGORY_PLAYER | CATEGORY_NPC,
			MASK_PROJECTILE			= CATEGORY_STATIC | CATEGORY_DYNAMIC | CATEGORY_PLAYER | CATEGORY_NPC,
			MASK_CHARACTER			= MASK_PLAYER | MASK_NPC,
			MASK_ALL				= 0xFFFFFFFF
		};

		pod::ShapeType type;
		pod::CollisionMask category = Collider::CATEGORY_ALL;
		pod::CollisionMask mask = Collider::MASK_ALL;

		union {
			pod::AABB aabb;
			pod::OBB obb;
			pod::Sphere sphere;
			pod::Plane plane;
			pod::Capsule capsule;
			pod::TriangleWithNormal triangle;
			pod::MeshBVH mesh;
			pod::ConvexHullBVH convexHull;
		};
	};

	struct PhysicsMaterial {
		float restitution = 0.2f;
		float staticFriction = 0.5f;
		float dynamicFriction = 0.3f;
	};

	struct Activity {
		bool awake = true;
		bool grounded = false;
		float sleepTimer = 0.0f;
		int32_t islandID = -1;
		static constexpr float sleepThreshold = 0.5f; // seconds
		static constexpr float linearSleepEpsilon = 0.2f; // m/s
		static constexpr float angularSleepEpsilon = 0.2f; // rad/s		
	};

	struct World; // forward declare

	struct PhysicsBody {
		pod::World* world = NULL;
		uf::Object* object = NULL;
		pod::Transform<>* transform = NULL;

		bool isStatic = false;

		float mass = 1.0f;
		float inverseMass = 1.0f; // for fast division
		int32_t viewIndex = -1; // -1 means it's not an aliased view

		/*alignas(16)*/ pod::Vector3f offset = {};

		/*alignas(16)*/ pod::Vector3f velocity = {};
		/*alignas(16)*/ pod::Vector3f pseudoVelocity = {};
		/*alignas(16)*/ pod::Vector3f forceAccumulator = {};

		/*alignas(16)*/ pod::Vector3f angularVelocity = {};
		/*alignas(16)*/ pod::Vector3f pseudoAngularVelocity = {};
		/*alignas(16)*/ pod::Vector3f torqueAccumulator = {};

		/*alignas(16)*/ pod::Vector3f inertiaTensor = { 1, 1, 1 };
		/*alignas(16)*/ pod::Vector3f inverseInertiaTensor = { 1, 1, 1 };

		/*alignas(16)*/ pod::Vector3f gravity = { NAN, NAN, NAN }; // an invalid gravity will fallback to world gravity

		/*alignas(16)*/ pod::AABB bounds;
		/*alignas(16)*/ pod::Collider collider;
		/*alignas(16)*/ pod::PhysicsMaterial material;
		/*alignas(16)*/ pod::Activity activity;
	};

	struct Contact {
		pod::Vector3f point;
		pod::Vector3f normal;
		float penetration;

		pod::Vector3f localA;
		pod::Vector3f localB;

		// warm-start cached values
		int lifetime = 0;
		pod::Vector3f tangent = {};
		float accumulatedNormalImpulse = 0.0f;
		float accumulatedTangentImpulse = 0.0f;
		float accumulatedPseudoImpulse = 0.0f;
	};

	struct Manifold {
		pod::PhysicsBody* a = NULL;
		pod::PhysicsBody* b = NULL;
		float dt = 0;
		uf::stl::vector<pod::Contact> points;
	};

	struct RayQuery {
		bool hit = false;
		const pod::PhysicsBody* body = NULL;
		const pod::PhysicsBody* invoker = NULL;
		pod::Contact contact = { pod::Vector3f{}, pod::Vector3f{}, FLT_MAX };
	};

	struct Island {
		bool awake = true;
		uf::stl::vector<uint32_t> indices;
		pod::BVH::pairs_t pairs;

		uf::stl::vector<pod::Manifold> manifolds;
	};

	struct PhysicsSettings {
		bool warmupSolver = true; // cache manifold data to warm up the solver
		bool blockContactSolver = true; // use BlockNxN solvers (where N = number of contacts for a manifold)
		bool resolveBlockContact = true; // attempts to resolve an invalid BlockNxN solve with an BlockN-1xN-1 solve
		bool useGjk = false; // currently don't have a way to broadphase mesh => narrowphase tri via GJK
		
		pod::CollisionMask debugDraw = pod::Collider::CATEGORY_NONE; // draws wireframe of collision bodies
		bool async = false; // dedicated thread for physics sim
		float timestep = 1.0f / 60.0f; // timestep for fixed step ticks
		bool fixedStep = true; // run physics simulation with a fixed delta time (with accumulation), rather than rely on actual engine deltatime
		uint32_t substeps = 4; // number of substeps per frame tick
		uint32_t reserveCount = 32; // amount of elements to reserve for vectors used in this system, to-do: have it tie to a memory pool allocator

		uint32_t broadphaseBvhCapacity = 1; // number of bodies per leaf node
		uint32_t meshBvhCapacity = 1; // number of triangles per leaf node

		// additionally flattens a BVH for linear iteration, rather than a recursive / stack-based traversal
		bool flattenBvhBodies = true;
		bool flattenBvhMeshes = true;
		
		// use surface area heuristics for building the BVH, rather than naive splits
		bool useBvhSahBodies = true;
		bool useBvhSahMeshes = true;

		bool useSplitBvhs = true; // creates separate BVHs for static / dynamic objects

		// to-do: find possibly better values for this
		uint32_t solverIterations = 10;
		float baumgarteCorrectionPercent = 0.2f;
		float baumgarteCorrectionSlop = 0.005f;
		
		uf::stl::unordered_map<size_t, pod::Manifold> manifoldsCache;
		uint32_t manifoldCacheLifetime = 0; // 0 = derive from current settings

		uint32_t frameCounter = 0;

		// to-do: tweak this to not be annoying
		pod::BVH::UpdatePolicy bvhUpdatePolicy = {
			// triggers a refit
			.displacementThreshold = 0.25f,
			// triggers a rebuild
			.overlapThreshold = 2.0f,
			.dirtyRatioThreshold = 0.3,
			.maxFramesBeforeRebuild = 60,
		};

		float groundedThreshold = 0.7f; // threshold before marking a body as grounded
	};

	struct World {
		uf::stl::vector<pod::PhysicsBody*> bodies;
	
		pod::Vector3f gravity = { 0, -9.81f, 0 };
		pod::BVH dynamicBvh;
		pod::BVH staticBvh;
	};
}

namespace uf {
	namespace physics {
		typedef pod::Math::num_t num_t;
		namespace time = uf::time; // to-do: have separate values from the physics system
		
		extern UF_API pod::World world;
		extern UF_API pod::PhysicsSettings settings;

		void UF_API initialize();
		void UF_API initialize( uf::Object& );
		void UF_API initialize( pod::World& );
		
		void UF_API tick();
		void UF_API tick( uf::Object& );

		void UF_API tick( float );
		void UF_API tick( uf::Object&, float );
		void UF_API tick( pod::World&, float );

		void UF_API terminate();
		void UF_API terminate( uf::Object& );
		void UF_API terminate( pod::World& );

		void UF_API step( pod::World&, float dt );
		void UF_API substep( pod::World&, float dt, int substeps );

		void UF_API setMass( pod::PhysicsBody& body, float mass = 0.0f );
		void UF_API setColliderCategory( pod::PhysicsBody&, uint32_t category = pod::Collider::CATEGORY_ALL );
		void UF_API setColliderCategory( pod::PhysicsBody&, const uf::stl::string& );
		void UF_API setColliderMask( pod::PhysicsBody&, uint32_t mask = pod::Collider::MASK_ALL );
		void UF_API setColliderMask( pod::PhysicsBody&, const uf::stl::string& );
		void UF_API setGravity( pod::PhysicsBody&, const pod::Vector3f& = { NAN, NAN, NAN } );
		pod::Vector3f UF_API getGravity( pod::PhysicsBody& );

		void UF_API updateInertia( pod::PhysicsBody& body );

		void UF_API applyForce( pod::PhysicsBody& body, const pod::Vector3f& force );
		void UF_API applyForceAtPoint( pod::PhysicsBody& body, const pod::Vector3f& force, const pod::Vector3f& point );
		void UF_API applyImpulse( pod::PhysicsBody& body, const pod::Vector3f& impulse );
		void UF_API applyTorque( pod::PhysicsBody& body, const pod::Vector3f& torque );

		void UF_API setVelocity( pod::PhysicsBody& body, const pod::Vector3f& velocity );
		void UF_API applyVelocity( pod::PhysicsBody& body, const pod::Vector3f& velocity );
		
		void UF_API setAngularVelocity( pod::PhysicsBody& body, const pod::Vector3f& velocity );
		void UF_API setAngularVelocity( pod::PhysicsBody& body, const pod::Quaternion<>& q, float dt = 0 );
		void UF_API applyAngularVelocity( pod::PhysicsBody& body, const pod::Vector3f& velocity );
		void UF_API applyAngularVelocity( pod::PhysicsBody& body, const pod::Quaternion<>& q, float dt = 0 );

		void UF_API applyRotation( pod::PhysicsBody& body, const pod::Quaternion<>& q );
		void UF_API applyRotation( pod::PhysicsBody& body, const pod::Vector3f& axis, float angle );
		
		pod::PhysicsBody& UF_API create( uf::Object&, float mass = 0.0f, const pod::Vector3f& = {} );
		pod::PhysicsBody& UF_API create( uf::Object& object, const pod::AABB& aabb, float mass = 0.0f, const pod::Vector3f& = {} );
		pod::PhysicsBody& UF_API create( uf::Object& object, const pod::Sphere& sphere, float mass = 0.0f, const pod::Vector3f& = {} );
		pod::PhysicsBody& UF_API create( uf::Object& object, const pod::Plane& plane, float mass = 0.0f, const pod::Vector3f& = {} );
		pod::PhysicsBody& UF_API create( uf::Object& object, const pod::Capsule& capsule, float mass = 0.0f, const pod::Vector3f& = {} );
		pod::PhysicsBody& UF_API create( uf::Object&, const uf::Mesh& mesh, float mass = 0.0f, const pod::Vector3f& = {}, bool convex = false );

		pod::PhysicsBody& UF_API create( pod::World&, uf::Object&, float mass = 0.0f, const pod::Vector3f& = {} );
		pod::PhysicsBody& UF_API create( pod::World&, uf::Object& object, const pod::AABB& aabb, float mass = 0.0f, const pod::Vector3f& = {} );
		pod::PhysicsBody& UF_API create( pod::World&, uf::Object& object, const pod::Sphere& sphere, float mass = 0.0f, const pod::Vector3f& = {} );
		pod::PhysicsBody& UF_API create( pod::World&, uf::Object& object, const pod::Plane& plane, float mass = 0.0f, const pod::Vector3f& = {} );
		pod::PhysicsBody& UF_API create( pod::World&, uf::Object& object, const pod::Capsule& capsule, float mass = 0.0f, const pod::Vector3f& = {} );
		pod::PhysicsBody& UF_API create( pod::World&, uf::Object& object, const pod::TriangleWithNormal& tri, float mass = 0.0f, const pod::Vector3f& = {} );
		pod::PhysicsBody& UF_API create( pod::World&, uf::Object&, const uf::Mesh& mesh, float mass = 0.0f, const pod::Vector3f& = {}, bool convex = false );

		void UF_API destroy( uf::Object& );
		void UF_API destroy( pod::PhysicsBody& );

		pod::RayQuery UF_API rayCast( const pod::Ray&, const pod::PhysicsBody&, float = FLT_MAX );
		pod::RayQuery UF_API rayCast( const pod::Ray&, const pod::World&, float = FLT_MAX );
		pod::RayQuery UF_API rayCast( const pod::Ray&, const pod::World&, const pod::PhysicsBody*, float = FLT_MAX );
	}
}