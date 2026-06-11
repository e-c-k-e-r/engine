#pragma once

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

#include <cfloat>

#define ASSERT_COLLIDER_TYPES( A, B ) UF_ASSERT( a.collider.type == pod::ShapeType::A && b.collider.type == pod::ShapeType::B );

#define REORIENT_NORMALS_ON_FETCH 0
#define REORIENT_NORMALS_ON_CONTACT 1

#define REVERSE_COLLIDER( a, b, fun )\
	auto start = manifold.points.size();\
	if ( !::fun( b, a, manifold ) ) return false;\
	for ( auto i = start; i < manifold.points.size(); ++i ) manifold.points[i].normal = -manifold.points[i].normal;\
	return true;

// Forward declates
namespace pod {
	struct World;
	struct PhysicsBody; 
}

// GJK
namespace pod {
	struct SupportPoint {
		pod::Vector3f p;
		pod::Vector3f pA;
		pod::Vector3f pB;
	};

	struct Simplex {
		uf::stl::vector<pod::SupportPoint> pts;
	};

	struct Face {
		pod::SupportPoint a, b, c;
		pod::Vector3f normal;
		float distance;
	};
}

// Constraints
namespace pod {
	enum class ConstraintType {
		BALL_AND_SOCKET,
		HINGE,
		CONE_TWIST,
		SLIDER,
		DISTANCE,
		WELD,
		SPRING,
		//CONTACT
	};

	// cannot provide default initialization values as that makes them non-trivial for unions
	struct BallSocket {
		pod::Vector3f localAnchorA;
		pod::Vector3f localAnchorB;

		pod::Vector3f accumulatedImpulse;
	};

	struct Hinge {
		pod::Vector3f localAnchorA;
		pod::Vector3f localAnchorB;

		pod::Vector3f accumulatedImpulse;
		pod::Vector2f accumulatedAngularImpulse;

		pod::Vector3f localAxisA;
		pod::Vector3f localAxisB;
	};
	
	struct ConeTwist {
		pod::Vector3f localAnchorA;
		pod::Vector3f localAnchorB;

		pod::Vector3f accumulatedImpulse;
		pod::Vector3f accumulatedAngularImpulse;

		pod::Vector3f localTwistAxisA;
		pod::Vector3f localTwistAxisB;

		pod::Vector3f localReferenceAxisA;
		pod::Vector3f localReferenceAxisB;

		float swingLimit; // M_PI / 4.0f;
		float twistLimit; // M_PI / 8.0f;
	};

	struct Slider {
		pod::Vector3f localAnchorA;
		pod::Vector3f localAnchorB;

		pod::Vector2f accumulatedLinearImpulse;
		pod::Vector3f accumulatedAngularImpulse;

		pod::Vector3f localAxisA;
		pod::Vector3f localAxisB;

		pod::Vector3f localReferenceAxisA;
		pod::Vector3f localReferenceAxisB;

		float lowerLimit;
		float upperLimit;
		float accumulatedLimitImpulse;
	};

	struct Distance {
		pod::Vector3f localAnchorA;
		pod::Vector3f localAnchorB;

		float accumulatedImpulse;
		float targetDistance;
		bool isRope; // to-do: rename to something more generic
	};

	struct Weld {
		pod::Vector3f localAnchorA;
		pod::Vector3f localAnchorB;

		pod::Vector3f accumulatedLinearImpulse;
		pod::Vector3f accumulatedAngularImpulse;

		pod::Vector3f localAxisA;
		pod::Vector3f localAxisB;

		pod::Vector3f localReferenceAxisA;
		pod::Vector3f localReferenceAxisB;
	};

	struct Spring {
		pod::Vector3f localAnchorA;
		pod::Vector3f localAnchorB;
		
		float accumulatedImpulse;

		float restLength;
		float stiffness;
		float damping;
	};

	struct Motor {
		bool enabled = false;
		float targetVelocity = 0.0f;
		union {
			float maxMotorForce;
			float maxMotorTorque;
		};
		float accumulatedMotorImpulse = 0.0f;
	};

	// this /could/ mirror the above joint-based constraints
	// but a lot of the code initializes contact structs as Contact{ point, normal, depth }, which would not easily align with the above structs
	struct Contact {
		pod::Vector3f point;
		pod::Vector3f normal;
		float penetration;

		pod::Vector3f localA;
		pod::Vector3f localB;

		// warm-start cached values
		int lifetime = 0;
		pod::Vector3f tangent;
		float accumulatedNormalImpulse = 0.0f;
		float accumulatedTangentImpulse = 0.0f;
		float accumulatedPseudoImpulse = 0.0f;
	};

	struct Manifold {
		pod::PhysicsBody* a = NULL;
		pod::PhysicsBody* b = NULL;
		uf::stl::vector<pod::Contact> points;
	};

	struct Constraint {
		pod::ConstraintType type = {};
		pod::PhysicsBody* a = NULL;
		pod::PhysicsBody* b = NULL;

		union {
			pod::BallSocket ballSocket;
			pod::Hinge hinge;
			pod::ConeTwist coneTwist;
			pod::Slider slider;
			pod::Distance distance;
			pod::Weld weld;
			pod::Spring spring;
		};
		pod::Motor motor;
	};

	struct SolverBodyContext {
		float invM = 0;
		pod::Matrix3f invI = {};
	};

	struct JacobianRow {
		pod::Vector3f rA;
		pod::Vector3f rB;
		pod::Vector3f n;
	};
}

// BVH
namespace pod {
	struct BVH {
		typedef uint32_t index_t;
		typedef std::pair<index_t,index_t> pair_t;
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
		uf::stl::vector<pod::Constraint*> constraints;
	};
}

// Colliders
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
			MASK_DYNAMIC			= CATEGORY_STATIC | CATEGORY_DYNAMIC | CATEGORY_PLAYER | CATEGORY_NPC | CATEGORY_TRIGGER,
			MASK_PLAYER             = CATEGORY_STATIC | CATEGORY_DYNAMIC | CATEGORY_NPC | CATEGORY_PROJECTILE | CATEGORY_TRIGGER,
            MASK_NPC                = CATEGORY_STATIC | CATEGORY_DYNAMIC | CATEGORY_PLAYER | CATEGORY_PROJECTILE | CATEGORY_TRIGGER,
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

	enum class CollisionState {
		ENTER,
		SUSTAIN,
		EXIT
	};

	struct CollisionEvent {
		typedef std::pair<pod::PhysicsBody*, pod::PhysicsBody*> pairs_t;
		typedef uf::stl::vector<pod::CollisionEvent> events_t;
		typedef uf::stl::unordered_map<uint64_t, pod::CollisionEvent::pairs_t> map_t;

		pod::CollisionState state = {};
		pod::PhysicsBody* a = NULL;
		pod::PhysicsBody* b = NULL;
		pod::Vector3f point = {};
		pod::Vector3f normal = {};
		float impulse = 0;
	};
}
	
namespace pod {
	struct PhysicsBody {
		pod::World* world = NULL;
		uf::Object* object = NULL;
		pod::Transform<>* transform = NULL;
		pod::PhysicsBody* next = NULL;

		float inverseMass = 0.0f;
		int32_t viewIndex = -1; // -1 means it's not an aliased view

		pod::Vector3f offsetPosition = {};
		pod::Quaternion<> offsetOrientation = { 0, 0, 0, 1 };

		pod::Vector3f velocity = {};
		pod::Vector3f forceAccumulator = {};

		pod::Vector3f angularVelocity = {};
		pod::Vector3f torqueAccumulator = {};

		pod::Vector3f pseudoVelocity = {};
		pod::Vector3f pseudoAngularVelocity = {};

		pod::Vector3f inverseInertiaTensor = { 1, 1, 1 };

		pod::Vector3f gravity = { NAN, NAN, NAN }; // an invalid gravity will fallback to world gravity

		pod::AABB bounds;
		pod::Collider collider;
		pod::PhysicsMaterial material;
		pod::Activity activity;
	};
}

// Settings
namespace pod {
	struct PhysicsSettings {
		bool warmupSolver = true; // cache manifold data to warm up the solver
		bool blockContactSolver = true; // use BlockNxN solvers (where N = number of contacts for a manifold)
		bool resolveBlockContact = true; // attempts to resolve an invalid BlockNxN solve with an BlockN-1xN-1 solve
		bool useGjk = false; // currently don't have a way to broadphase mesh => narrowphase tri via GJK
		
		bool async = false; // dedicated thread for physics sim
		float timestep = 1.0f / 60.0f; // timestep for fixed step ticks
		bool fixedStep = true; // run physics simulation with a fixed delta time (with accumulation), rather than rely on actual engine deltatime
		uint32_t substeps = 4; // number of substeps per frame tick
		uint32_t reserveCount = 32; // amount of elements to reserve for vectors used in this system, to-do: have it tie to a memory pool allocator

		uint32_t broadphaseBvhCapacity = 1; // number of bodies per leaf node
		uint32_t meshBvhCapacity = 1; // number of triangles per leaf node

		bool flattenTransforms = true; // flatten transforms before a step, then unflattens it at end of step (to solve a problem involving hierarchies)

		// additionally flattens a BVH for linear iteration, rather than a recursive / stack-based traversal
		bool flattenBvhBodies = true;
		bool flattenBvhMeshes = true;
		
		// use surface area heuristics for building the BVH, rather than naive splits
		bool useBvhSahBodies = true;
		bool useBvhSahMeshes = true;

		bool useSplitBvhs = true; // creates separate BVHs for static / dynamic objects

		// to-do: find possibly better values for this
		uint32_t solverIterations = 10;
		bool ngsPositionSolver = false;
		float baumgarteCorrectionPercent = 0.2f;
		float baumgarteCorrectionSlop = 0.005f;
		float maxLinearCorrection = 0.2f;
		
		float contactCFM = 1e-3f;
		float jointCFM = 1e-4f;

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

		struct {
			pod::CollisionMask mask = pod::Collider::CATEGORY_NONE; // draws wireframe of collision bodies
			bool contacts = false;
			bool constraints = false;
			bool rays = false;
			bool depthTest = true;
		} debugDraw;
	};

	struct World {
		uf::stl::vector<pod::PhysicsBody*> bodies;
		uf::stl::vector<pod::Constraint*> constraints;
	
		pod::Vector3f gravity = { 0, -9.81f, 0 };
		pod::BVH dynamicBvh;
		pod::BVH staticBvh;

		pod::CollisionEvent::events_t collisionEvents;
		pod::CollisionEvent::map_t activeCollisions;
	};
}

namespace uf {
	namespace physics {
		typedef pod::Math::num_t num_t;
		namespace time = uf::time; // to-do: have separate values from the physics system
		
		//extern UF_API pod::World world;
		extern UF_API pod::PhysicsSettings settings;
	}
}