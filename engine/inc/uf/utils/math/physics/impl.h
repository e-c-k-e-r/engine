#pragma once

#include <uf/utils/math/vector.h>
#include <uf/utils/math/quaternion.h>
#include <uf/utils/math/matrix.h>
#include <uf/utils/math/shapes.h>
#include <uf/utils/math/transform.h>

#include <uf/utils/memory/vector.h>
#include <uf/utils/memory/unordered_map.h>

#include <uf/engine/object/object.h>
#include <cfloat>

namespace uf {
	class Mesh;
}

namespace pod {
	enum class ShapeType {
		AABB,
		SPHERE,
		PLANE,
		CAPSULE,
		TRIANGLE,
		MESH,
	};

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

	struct BVH {
		typedef uf::stl::vector<std::pair<int,int>> pair_t;
		struct Node {
			pod::AABB bounds = {};
			int left = -1;
			int right = -1;
			int start = 0;
			int count = 0;
		};
		uf::stl::vector<size_t> indices;
		uf::stl::vector<pod::BVH::Node> nodes;
	};

	struct MeshBVH {
		pod::BVH* bvh;
		const uf::Mesh* mesh;
	};

	typedef uint32_t CollisionMask;

	struct Collider {
		pod::ShapeType type;
		pod::CollisionMask mask = 0xFFFFFFFF;
		union {
			pod::Sphere sphere;
			pod::AABB aabb;
			pod::Plane plane;
			pod::Capsule capsule;
			pod::TriangleWithNormal triangle;
			pod::MeshBVH mesh;
		} u;
	};

	struct PhysicsMaterial {
		float restitution = 0.2f;
		float staticFriction = 0.5f;
		float dynamicFriction = 0.3f;
	};

	struct World; // forward declare

	struct RigidBody {
		pod::World* world = NULL;
		uf::Object* object = NULL;
		// pod::Transform<> transform = {};
		
		pod::Transform<>* transform = NULL;
		pod::Vector3f offset = {};

		bool isStatic = false;

		float mass = 1.0f;
		float inverseMass = 1.0f;

		pod::Vector3f velocity = {};
		pod::Vector3f forceAccumulator = {};

		pod::Vector3f angularVelocity = {};
		pod::Vector3f torqueAccumulator = {};

		pod::Vector3f inertiaTensor = { 1, 1, 1 };
		pod::Vector3f inverseInertiaTensor = { 1, 1, 1 };

		pod::AABB bounds;
		pod::Collider collider;
		pod::PhysicsMaterial material;
	};

	struct Contact {
		pod::Vector3f point;
		pod::Vector3f normal;
		float penetration;

		// Warm-start cached values
		int lifetime = 0;
		pod::Vector3f tangent = {};
		float accumulatedNormalImpulse = 0.0f;
		float accumulatedTangentImpulse = 0.0f;
	};

	struct Manifold {
		pod::RigidBody* a = NULL;
		pod::RigidBody* b = NULL;
		float dt = 0;
		uf::stl::vector<pod::Contact> points;
	};

	struct RayQuery {
		bool hit = false;
		const pod::RigidBody* body;
		pod::Contact contact = { pod::Vector3f{}, pod::Vector3f{}, FLT_MAX };
	};

	struct World {
		uf::stl::vector<pod::RigidBody*> bodies;
	
		pod::Vector3f gravity = { 0, -9.81f, 0 };
		pod::BVH bvh;
	};
}

namespace uf {
	namespace physics {
		namespace impl {
			extern UF_API float timescale;
			extern UF_API bool interpolate;
			extern UF_API bool shared;
			extern UF_API bool globalStorage;
			
			extern UF_API pod::World world;

			void UF_API initialize();
			void UF_API initialize( uf::Object& );
			void UF_API initialize( pod::World& );
			
			void UF_API tick( float = 0 );
			void UF_API tick( uf::Object&, float = 0 );
			void UF_API tick( pod::World&, float = 0 );

			void UF_API terminate();
			void UF_API terminate( uf::Object& );
			void UF_API terminate( pod::World& );

			void UF_API step( pod::World&, float dt );
			void UF_API substep( pod::World&, float dt, int substeps );

			void UF_API setMass( pod::RigidBody& body, float mass = 0.0f );
			void UF_API setInertia( pod::RigidBody& body );
			void UF_API applyForce( pod::RigidBody& body, const pod::Vector3f& force );
			void UF_API applyForceAtPoint( pod::RigidBody body, const pod::Vector3f& force, const pod::Vector3f& point );
			void UF_API applyImpulse( pod::RigidBody& body, const pod::Vector3f& impulse );
			void UF_API applyTorque( pod::RigidBody& body, const pod::Vector3f& torque );

			void UF_API setVelocity( pod::RigidBody& body, const pod::Vector3f& velocity );
			void UF_API applyRotation( pod::RigidBody& body, const pod::Quaternion<>& q );
			void UF_API applyRotation( pod::RigidBody& body, const pod::Vector3f& axis, float angle );
			
			pod::RigidBody& UF_API create( uf::Object&, float mass = 0.0f, const pod::Vector3f& = {} );
			pod::RigidBody& UF_API create( uf::Object& object, const pod::AABB& aabb, float mass = 0.0f, const pod::Vector3f& = {} );
			pod::RigidBody& UF_API create( uf::Object& object, const pod::Sphere& sphere, float mass = 0.0f, const pod::Vector3f& = {} );
			pod::RigidBody& UF_API create( uf::Object& object, const pod::Plane& plane, float mass = 0.0f, const pod::Vector3f& = {} );
			pod::RigidBody& UF_API create( uf::Object& object, const pod::Capsule& capsule, float mass = 0.0f, const pod::Vector3f& = {} );
			pod::RigidBody& UF_API create( uf::Object&, const uf::Mesh& mesh, float mass = 0.0f, const pod::Vector3f& = {} );

			pod::RigidBody& UF_API create( pod::World&, uf::Object&, float mass = 0.0f, const pod::Vector3f& = {} );
			pod::RigidBody& UF_API create( pod::World&, uf::Object& object, const pod::AABB& aabb, float mass = 0.0f, const pod::Vector3f& = {} );
			pod::RigidBody& UF_API create( pod::World&, uf::Object& object, const pod::Sphere& sphere, float mass = 0.0f, const pod::Vector3f& = {} );
			pod::RigidBody& UF_API create( pod::World&, uf::Object& object, const pod::Plane& plane, float mass = 0.0f, const pod::Vector3f& = {} );
			pod::RigidBody& UF_API create( pod::World&, uf::Object& object, const pod::Capsule& capsule, float mass = 0.0f, const pod::Vector3f& = {} );
			pod::RigidBody& UF_API create( pod::World&, uf::Object& object, const pod::TriangleWithNormal& tri, float mass = 0.0f, const pod::Vector3f& = {} );
			pod::RigidBody& UF_API create( pod::World&, uf::Object&, const uf::Mesh& mesh, float mass = 0.0f, const pod::Vector3f& = {} );

			void UF_API destroy( uf::Object& );
			void UF_API destroy( pod::RigidBody& );

			pod::RayQuery UF_API rayCast( const pod::Ray&, const pod::RigidBody&, float = FLT_MAX );
			pod::RayQuery UF_API rayCast( const pod::Ray&, const pod::World&, float = FLT_MAX );
			pod::RayQuery UF_API rayCast( const pod::Ray&, const pod::World&, const pod::RigidBody*, float = FLT_MAX );
		}
	}
}