#if UF_USE_REACTPHYSICS
#include <uf/utils/math/physics.h>
#include <uf/ext/reactphysics/reactphysics.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/engine/scene/scene.h>
#include <uf/engine/graph/graph.h>


#if 0 && UF_ENV_DREAMCAST
	#define RP3D_OLD 1
#endif

namespace {
	rp3d::PhysicsCommon common;
	rp3d::PhysicsWorld* world;

	// i was wrong to assume that RP3D handles deleting these per the documentation
	// these should be userdatas / allocated using our mempool instead to avoid fragmentation
	uf::stl::unordered_map<size_t, uf::stl::vector<rp3d::TriangleVertexArray*>> triangleParts;

	reactphysics3d::TriangleMesh* createTriangleMesh( const uf::Mesh& mesh, const uf::Object& object ) {
		auto* rMesh = ::common.createTriangleMesh();
		auto& parts = ::triangleParts[object.getUid()];

		auto views = mesh.makeViews({"position", "normal"});

		if ( views.empty() ) return rMesh;

		for ( auto& view : views ) {
			if ( !view.vertex.count || !view.index.count ) continue;

			rp3d::TriangleVertexArray* part = NULL;

			auto& indices = view["index"];
			auto& positions = view["position"];
			auto& normals = view["normal"];

			UF_ASSERT( positions.valid() );

			// deduce types
			rp3d::TriangleVertexArray::IndexDataType indexType = rp3d::TriangleVertexArray::IndexDataType::INDEX_INTEGER_TYPE;
			rp3d::TriangleVertexArray::VertexDataType vertexType = rp3d::TriangleVertexArray::VertexDataType::VERTEX_FLOAT_TYPE;
			rp3d::TriangleVertexArray::NormalDataType normalType = rp3d::TriangleVertexArray::NormalDataType::NORMAL_FLOAT_TYPE;
			switch ( mesh.index.size ) {
				case sizeof(uint16_t): indexType = rp3d::TriangleVertexArray::IndexDataType::INDEX_SHORT_TYPE; break;
				case sizeof(uint32_t): indexType = rp3d::TriangleVertexArray::IndexDataType::INDEX_INTEGER_TYPE; break;
				default: UF_EXCEPTION("unsupported index type: {}", mesh.index.size); break;
			}
			switch ( positions.attribute.descriptor.type ) {
				case uf::renderer::enums::Type::USHORT:
				case uf::renderer::enums::Type::SHORT: vertexType = rp3d::TriangleVertexArray::VertexDataType::VERTEX_SHORT_TYPE; break;
				case uf::renderer::enums::Type::FLOAT: vertexType = rp3d::TriangleVertexArray::VertexDataType::VERTEX_FLOAT_TYPE; break;
			#if UF_USE_FLOAT16
				case uf::renderer::enums::Type::HALF: vertexType = rp3d::TriangleVertexArray::VertexDataType::VERTEX_FLOAT16_TYPE; break;
			#endif
			#if UF_USE_BFLOAT16
				case uf::renderer::enums::Type::BFLOAT: vertexType = rp3d::TriangleVertexArray::VertexDataType::VERTEX_BFLOAT16_TYPE; break;
			#endif
				default: UF_EXCEPTION("unsupported vertex type: {}", positions.attribute.descriptor.type); break;
			}
			switch ( normals.attribute.descriptor.type ) {
				case uf::renderer::enums::Type::USHORT:
				case uf::renderer::enums::Type::SHORT: normalType = rp3d::TriangleVertexArray::NormalDataType::NORMAL_SHORT_TYPE; break;
				case uf::renderer::enums::Type::FLOAT: normalType = rp3d::TriangleVertexArray::NormalDataType::NORMAL_FLOAT_TYPE; break;
			#if UF_USE_FLOAT16
				case uf::renderer::enums::Type::HALF: normalType = rp3d::TriangleVertexArray::NormalDataType::NORMAL_FLOAT16_TYPE; break;
			#endif
			#if UF_USE_BFLOAT16
				case uf::renderer::enums::Type::BFLOAT: normalType = rp3d::TriangleVertexArray::NormalDataType::NORMAL_BFLOAT16_TYPE; break;
			#endif
				default: UF_EXCEPTION("unsupported normal type: {}", normals.attribute.descriptor.type); break;
			}

			// has normals
			if ( normals.valid() ) {
				part = new rp3d::TriangleVertexArray(
					view.vertex.count,
					positions.data(view.vertex.first),
					positions.stride(),
					
					normals.data(view.vertex.first),
					normals.stride(),
					
					view.index.count / 3,
					indices.data(view.index.first),
					indices.stride() * 3,

					vertexType,
					normalType,
					indexType
				);
			} else {
				part = new rp3d::TriangleVertexArray(
					view.vertex.count,
					positions.data(view.vertex.first),
					positions.stride(),
										
					view.index.count / 3,
					indices.data(view.index.first),
					indices.stride() * 3,

					vertexType,
					indexType
				);
			}

			parts.emplace_back(part);
			rMesh->addSubpart(part);
		}

		return rMesh;
	}

	class EventListener : public rp3d::EventListener {
	public:
		virtual void onContact( const rp3d::CollisionCallback::CallbackData& callbackData ) override { 
		//	UF_MSG_DEBUG("Contact");
		}
	} listener;

	class RaycastCallback : public rp3d::RaycastCallback {
	public:
		bool isHit = false;
		uf::Object* source = NULL;
		rp3d::RaycastInfo raycastInfo;

		virtual rp3d::decimal notifyRaycastHit(const rp3d::RaycastInfo& info) override {
			if ( !isHit || raycastInfo.hitFraction > info.hitFraction ) {
				if ( info.body->getUserData() == source ) {

				} else {
					raycastInfo.body = info.body;
					raycastInfo.hitFraction = info.hitFraction;
					raycastInfo.collider = info.collider;
					raycastInfo.worldNormal = info.worldNormal;
					raycastInfo.worldPoint = info.worldPoint;
					isHit = true;
				}
			}
		//	return rp3d::decimal(1.0);
			return raycastInfo.hitFraction;
		}
	};


	rp3d::DefaultLogger* logger = NULL;

	pod::Vector3f convert( const rp3d::Vector3& v ) { return pod::Vector3f{ v.x, v.y, v.z }; }
	rp3d::Vector3 convert( const pod::Vector3f& v ) { return rp3d::Vector3( v.x, v.y, v.z ); }

	pod::Quaternion<> convert( const rp3d::Quaternion& q ) { return pod::Quaternion<>{ q.x, q.y, q.z, q.w }; }
	rp3d::Quaternion convert( const pod::Quaternion<>& q ) { return rp3d::Quaternion( q.x, q.y, q.z, q.w ); }

	pod::Transform<> convert( const rp3d::Transform& t ) {
		pod::Transform<> transform;

		/*state.*/transform.position = ::convert(t.getPosition());
		/*state.*/transform.orientation = ::convert(t.getOrientation());
		return uf::transform::reorient(/*state.*/transform);
	}
	rp3d::Transform convert( const pod::Transform<>& t ) {
		auto model = uf::transform::model( t );

		rp3d::Transform transform;
		transform.setFromOpenGL(&model[0]);

		return transform;
	}

	pod::Quaternion<> convertQ( const rp3d::Vector3& _v ) {
		pod::Quaternion<> q = uf::quaternion::identity();
		pod::Vector3f v = ::convert( _v );
		q.w = uf::vector::norm( v );
		if ( q.w > 0 ) q = { v.x / q.w, v.y / q.w, v.z / q.w, q.w };
		return q;
	}
	rp3d::Vector3 convertQ( const pod::Quaternion<>& q ) {
		rp3d::Vector3 v;
		v.x = q.x * q.w;
		v.y = q.y * q.w;
		v.z = q.z * q.w;
		return v;
	}

	pod::Vector3f uintToVector( uint32_t u ) {
		switch ( u ) {
			case (uint) rp3d::DebugRenderer::DebugColor::RED: return pod::Vector3f{ 1.0f, 0.0f, 0.0f }; break;
			case (uint) rp3d::DebugRenderer::DebugColor::GREEN: return pod::Vector3f{ 0.0f, 1.0f, 0.0f }; break;
			case (uint) rp3d::DebugRenderer::DebugColor::BLUE: return pod::Vector3f{ 0.0f, 0.0f, 1.0f }; break;
			case (uint) rp3d::DebugRenderer::DebugColor::BLACK: return pod::Vector3f{ 0.0f, 0.0f, 0.0f }; break;
			case (uint) rp3d::DebugRenderer::DebugColor::WHITE: return pod::Vector3f{ 1.0f, 1.0f, 1.0f }; break;
			case (uint) rp3d::DebugRenderer::DebugColor::YELLOW: return pod::Vector3f{ 1.0f, 1.0f, 0.0f }; break;
			case (uint) rp3d::DebugRenderer::DebugColor::MAGENTA: return pod::Vector3f{ 1.0f, 0.0f, 1.0f }; break;
			case (uint) rp3d::DebugRenderer::DebugColor::CYAN: return pod::Vector3f{ 0.0f, 1.0f, 1.0f }; break;
			default: return pod::Vector3f{};
		}
	}

	struct VertexLine {
		pod::Vector3f position;
		pod::Vector3f color;

		static uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
	};

	UF_VERTEX_DESCRIPTOR(VertexLine,
		UF_VERTEX_DESCRIPTION(VertexLine, R32G32B32_SFLOAT, position)
		UF_VERTEX_DESCRIPTION(VertexLine, R32G32B32_SFLOAT, color)
	);

	// allows showing collision models
	void debugDraw( uf::Object& object ) {
		auto& scene = uf::scene::getCurrentScene();
		auto& world = /*ext::reactphysics::globalStorage ? ::world :*/ scene.getComponent<ext::reactphysics::WorldState>();

		static size_t oldCount = 0;
		uf::Mesh mesh;
		rp3d::DebugRenderer& debugRenderer = world->getDebugRenderer(); 
		if ( !mesh.hasVertex<VertexLine>() ) mesh.bind<VertexLine>();

		size_t lineCount = debugRenderer.getNbLines();
		size_t triCount = debugRenderer.getNbTriangles();

		if ( !lineCount || !triCount ) return;
	//	if ( oldCount == lineCount * 2 + triCount * 3 ) return;
		oldCount = lineCount * 2 + triCount * 3;

		auto* lines = debugRenderer.getLinesArray();
		auto* tris = debugRenderer.getTrianglesArray();

		uf::stl::vector<VertexLine> vertices;
		vertices.reserve( lineCount * 2 + triCount * 3 );
		for ( size_t i = 0; i < lineCount; ++i ) {
			auto& line = lines[i];

			auto& vertex1 = vertices.emplace_back();
			vertex1.position = ::convert( line.point1 );
			vertex1.color = ::uintToVector( line.color1 );

			auto& vertex2 = vertices.emplace_back();
			vertex2.position = ::convert( line.point2 );
			vertex2.color = ::uintToVector( line.color2 );
		}
		for ( size_t i = 0; i < triCount; ++i ) {
			auto& tri = tris[i];

			auto& vertex1 = vertices.emplace_back();
			vertex1.position = ::convert( tri.point1 );
			vertex1.color = ::uintToVector( tri.color1 );

			auto& vertex2 = vertices.emplace_back();
			vertex2.position = ::convert( tri.point2 );
			vertex2.color = ::uintToVector( tri.color2 );

			auto& vertex3 = vertices.emplace_back();
			vertex3.position = ::convert( tri.point3 );
			vertex3.color = ::uintToVector( tri.color3 );
		}
		mesh.insertVertices(vertices);

		if ( !mesh.vertex.count ) return;

		bool create = !object.hasComponent<uf::Graphic>();
		auto& graphic = object.getComponent<uf::Graphic>();
		auto& storage = uf::graph::globalStorage ? uf::graph::storage : scene.getComponent<pod::Graph::Storage>();
		graphic.process = false;

		if ( create ) {
			graphic.device = &uf::renderer::device;
			graphic.material.device = &uf::renderer::device;
			graphic.descriptor.cullMode = uf::renderer::enums::CullMode::NONE;

			graphic.material.metadata.autoInitializeUniformBuffers = false;
			graphic.material.attachShader(uf::io::root + "/shaders/base/line/vert.spv", uf::renderer::enums::Shader::VERTEX);
			graphic.material.attachShader(uf::io::root + "/shaders/base/line/frag.spv", uf::renderer::enums::Shader::FRAGMENT);
			graphic.material.metadata.autoInitializeUniformBuffers = true;

			graphic.material.getShader("vertex").buffers.emplace_back( storage.buffers.camera.alias() );

			graphic.initialize(ext::reactphysics::debugDraw::layer);
			graphic.initializeMesh( mesh );

			graphic.descriptor.topology = uf::renderer::enums::PrimitiveTopology::LINE_LIST;
			graphic.descriptor.fill = uf::renderer::enums::PolygonMode::LINE;
			graphic.descriptor.lineWidth = ext::reactphysics::debugDraw::lineWidth;
		} else {
			if ( graphic.updateMesh( mesh ) ) {
				graphic.getPipeline().update( graphic );
			}
		}
		graphic.process = true;
	}
}

float ext::reactphysics::timescale = 1.0f / 60.0f;
bool ext::reactphysics::shared = true;
bool ext::reactphysics::interpolate = true;
bool ext::reactphysics::globalStorage = true;

ext::reactphysics::gravity::Mode ext::reactphysics::gravity::mode = ext::reactphysics::gravity::Mode::UNIVERSAL;
float ext::reactphysics::gravity::constant = 6.67408e-11;

bool ext::reactphysics::debugDraw::enabled = false;
float ext::reactphysics::debugDraw::rate = 1.0f;
uf::stl::string ext::reactphysics::debugDraw::layer = "";
float ext::reactphysics::debugDraw::lineWidth = 1.0f;

void ext::reactphysics::initialize() {
	return ext::reactphysics::initialize( uf::scene::getCurrentScene() );
}
void ext::reactphysics::initialize( uf::Object& scene ) {
	rp3d::PhysicsWorld::WorldSettings settings;
	if ( ext::reactphysics::gravity::mode == ext::reactphysics::gravity::Mode::DEFAULT ) {
		settings.gravity = rp3d::Vector3( 0, -9.81, 0 );
	} else {
		settings.gravity = rp3d::Vector3( 0, 0, 0 );
	}

//	::logger = ::common.createDefaultLogger();
//	size_t logLevel = static_cast<uint>(rp3d::Logger::Level::Warning) | static_cast<uint>(rp3d::Logger::Level::Error); // | static_cast<uint>(rp3d::Logger::Level::Information);
//	logger->addFileDestination("./data/logs/rp3d_log_.html", logLevel, rp3d::DefaultLogger::Format::HTML); 
//	::common.setLogger(::logger);

	auto& world = /*ext::reactphysics::globalStorage ? ::world :*/ scene.getComponent<ext::reactphysics::WorldState>();
	world = ::common.createPhysicsWorld(settings);
//	world->setEventListener(&::listener);

	if ( ext::reactphysics::debugDraw::enabled ) {
		world->setIsDebugRenderingEnabled(true);
		rp3d::DebugRenderer& debugRenderer = world->getDebugRenderer(); 
		// Select the contact points and contact normals to be displayed 
		debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLIDER_AABB, true); 
		debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLISION_SHAPE, true); 
	//	debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::CONTACT_POINT, true);
	//	debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::CONTACT_NORMAL, true);
	}
}
void ext::reactphysics::tick( float delta ) {
	return ext::reactphysics::tick( uf::scene::getCurrentScene(), delta );
}
void ext::reactphysics::tick( uf::Object& scene, float delta ) {
	auto& world = /*ext::reactphysics::globalStorage ? ::world :*/ scene.getComponent<ext::reactphysics::WorldState>();
	ext::reactphysics::syncTo( world );

	static float accumulator = 0;
	accumulator += uf::physics::time::delta; 
	while ( accumulator >= ext::reactphysics::timescale ) { 
		world->update(ext::reactphysics::timescale); 
		accumulator -= ext::reactphysics::timescale; 
	}

	TIMER( ext::reactphysics::debugDraw::rate, ext::reactphysics::debugDraw::enabled ) {
		::debugDraw( scene );
	}

	ext::reactphysics::syncFrom( world, accumulator / ext::reactphysics::timescale );
}
void ext::reactphysics::terminate() {
	return ext::reactphysics::terminate( uf::scene::getCurrentScene() );
}
void ext::reactphysics::terminate( uf::Object& scene ) {
	auto& world = /*ext::reactphysics::globalStorage ? ::world :*/ scene.getComponent<ext::reactphysics::WorldState>();
	if ( !world ) return;

	size_t count = world->getNbRigidBodies();
	for ( size_t i = 0; i < count; ++i ) {
		auto* body = world->getRigidBody(i); if ( !body ) continue;
		uf::Object* object = (uf::Object*) body->getUserData(); if ( !object || !object->isValid() ) continue;
		auto& state = object->getComponent<pod::PhysicsBody>();
		state.collider.body = NULL;
	}

	::common.destroyPhysicsWorld(world);
	world = NULL;
}

// base collider creation
pod::PhysicsBody& ext::reactphysics::create( uf::Object& object, float mass, const pod::Vector3f& offset ) {
	auto& state = object.getComponent<pod::PhysicsBody>();

	state.world = ext::reactphysics::globalStorage ? ::world : uf::scene::getCurrentScene().getComponent<ext::reactphysics::WorldState>();
	state.object = &object;
	state.transform.position = offset;
	state.transform.reference = &object.getComponent<pod::Transform<>>();
	state.mass = mass;
	state.isStatic = mass != 0.0f;
	state.inverseMass = mass == 0.0f ? 0.0f : 1.0f / mass;

	return state;
}

void ext::reactphysics::destroy( uf::Object& object ) {
	auto& state = object.getComponent<pod::PhysicsBody>();
	ext::reactphysics::destroy( state );

	auto uid = object.getUid();
	if ( ::triangleParts.count( uid ) > 0 ) {
		auto& parts = ::triangleParts[uid];
		for ( auto* part : parts ) {
			delete part;
		}
		::triangleParts.erase( uid );
	}
}
void ext::reactphysics::destroy( pod::PhysicsBody& state ) {
	ext::reactphysics::detach( state );
}

void ext::reactphysics::attach( pod::PhysicsBody& state ) {
	if ( !state.collider.shape || !state.world ) return;
	// auto& scene = uf::scene::getCurrentScene();
	// auto& world = /*ext::reactphysics::globalStorage ? ::world :*/ scene.getComponent<ext::reactphysics::WorldState>();
	
	rp3d::Transform colliderTransform = rp3d::Transform::identity();
	colliderTransform.setPosition( ::convert( state.transform.position ) );
	colliderTransform.setOrientation( ::convert( state.transform.orientation ) );

	state.transform.position = {};
	state.transform.orientation = {};

	state.collider.body = state.world->createRigidBody( ::convert( *state.transform.reference ) );
	
	auto* collider = state.collider.body->addCollider(state.collider.shape, colliderTransform);
	collider->setCollisionCategoryBits(0xFF);
	collider->setCollideWithMaskBits(0xFF);
	
	state.collider.body->setUserData(state.object);
	state.collider.body->setMass(state.mass);

	if ( state.mass != 0.0f ) {
		state.collider.body->setType(rp3d::BodyType::DYNAMIC);
		state.collider.body->updateLocalCenterOfMassFromColliders();
		state.collider.body->updateMassPropertiesFromColliders();
	} else {
		state.collider.body->setType(rp3d::BodyType::STATIC);
	}

	state.collider.body->enableGravity(state.gravity != pod::Vector3f{0,0,0});

	// affects air speed, bad
//	state.collider.body->setLinearDamping(state.material.staticFriction);

	auto& material = collider->getMaterial();
	material.setBounciness(0);

	state.collider.body->setLocalInertiaTensor( ::convert( state.inertiaTensor ) );
}
void ext::reactphysics::detach( pod::PhysicsBody& state ) {
	if ( !state.collider.body || !state.world ) return;
	// auto& scene = uf::scene::getCurrentScene();
	// auto& world = /*ext::reactphysics::globalStorage ? ::world :*/ scene.getComponent<ext::reactphysics::WorldState>();

	state.world->destroyRigidBody(state.collider.body);
	state.collider.body = NULL;

	state = {}; // necessary if it gets reused
}

// collider for mesh (static or dynamic)
pod::PhysicsBody& ext::reactphysics::create( uf::Object& object, const uf::Mesh& mesh, float mass, const pod::Vector3f& offset ) {
	UF_ASSERT( mesh.index.count );
	
	auto* rMesh = ::createTriangleMesh( mesh, object );

	auto& state = ext::reactphysics::create( object, mass, offset );
	state.collider.shape = ::common.createConcaveMeshShape( rMesh );
	state.mass = 0;
	ext::reactphysics::attach( state );

	return state;
}
// collider for boundingbox
pod::PhysicsBody& ext::reactphysics::create( uf::Object& object, const pod::AABB& aabb, float mass, const pod::Vector3f& offset ) {
	pod::Vector3f extent = ( aabb.max - aabb.min ) * 0.5f;
	auto& state = ext::reactphysics::create( object, mass, offset );
	state.collider.shape = ::common.createBoxShape( rp3d::Vector3( abs(extent.x), abs(extent.y), abs(extent.z) ) );
	ext::reactphysics::attach( state );
	
	return state;
}
//
pod::PhysicsBody& ext::reactphysics::create( uf::Object& object, const pod::Sphere& aabb, float mass, const pod::Vector3f& offset ) {
	auto& state = ext::reactphysics::create( object, mass, offset );
	//state.collider.shape = ::common.createSphereShape( rp3d::Vector3( abs(extent.x), abs(extent.y), abs(extent.z) ) );
	ext::reactphysics::attach( state );
	
	return state;
}
//
pod::PhysicsBody& ext::reactphysics::create( uf::Object& object, const pod::Plane& aabb, float mass, const pod::Vector3f& offset ) {
	auto& state = ext::reactphysics::create( object, mass, offset );
	//state.collider.shape = ::common.createPlaneShape( rp3d::Vector3( abs(extent.x), abs(extent.y), abs(extent.z) ) );
	ext::reactphysics::attach( state );
	
	return state;
}
// collider for capsule
pod::PhysicsBody& ext::reactphysics::create( uf::Object& object, const pod::Capsule& capsule, float mass, const pod::Vector3f& offset ) {
	auto& state = ext::reactphysics::create( object, mass, offset );
	state.collider.shape = ::common.createCapsuleShape( capsule.radius, capsule.halfHeight * 2.0f );
	ext::reactphysics::attach( state );
	
	return state;
}

// synchronize engine transforms to bullet transforms
void ext::reactphysics::syncTo( ext::reactphysics::WorldState& world ) {
	size_t count = world->getNbRigidBodies();
	
	struct Body {
		rp3d::RigidBody* body{};
		float mass{};
		pod::Vector3f position{};
	};
	uf::stl::vector<Body> bodies; bodies.reserve(count);
	bodies.emplace_back(Body{
		.body = NULL,
		.mass = 5.97219e24,
		.position = pod::Vector3f{ 0, -6.371e6, 0 },
	});

	for ( size_t i = 0; i < count; ++i ) {
		auto* body = world->getRigidBody(i); if ( !body ) continue;
		uf::Object* object = (uf::Object*) body->getUserData(); if ( !object || !object->isValid() ) continue;
		auto& state = object->getComponent<pod::PhysicsBody>();

		if ( true /*state\.shared*/ ) {
			if ( !ext::reactphysics::interpolate ) body->setTransform(::convert(state.transform));
			body->setLinearVelocity( ::convert(state.velocity) );
			body->setAngularVelocity( ::convertQ(state.angularVelocity) );
		}
		// apply per-object gravities
		float mass = body->getMass();
		switch ( ext::reactphysics::gravity::mode ) {
			case ext::reactphysics::gravity::Mode::PER_OBJECT: if ( body->isGravityEnabled() ) {
			#if RP3D_OLD
				body->applyForceToCenterOfMass( ::convert(state.gravity * mass) );
			#else
				body->applyLocalForceAtCenterOfMass( ::convert(state.gravity * mass) );
			#endif
			} break;
			case ext::reactphysics::gravity::Mode::UNIVERSAL: if ( mass > 0 ) {
				auto transform = ::convert( body->getTransform() );
				bodies.emplace_back(Body{
					.body = body,
					.mass = mass,
					.position = transform.position,
				});
			} break;
		}
		state.internal.previous = state.internal.current;
	}

	if ( ext::reactphysics::gravity::mode == ext::reactphysics::gravity::Mode::UNIVERSAL ) {
		for ( auto i1 = 0; i1 < bodies.size(); ++i1 ) {
			for ( auto i2 = 0; i2 < bodies.size(); ++i2 ) {
				if ( i1 == i2 ) continue;
				const auto& b1 = bodies[i1];
				const auto& b2 = bodies[i2];

				const float T = uf::physics::time::delta / ext::reactphysics::timescale;

				const auto direction = ::convert(uf::vector::normalize( b2.position - b1.position ));
				const float G = ext::reactphysics::gravity::constant;
				const float m1 = b1.mass;
				const float m2 = b2.mass;
				const float r2 = uf::vector::distanceSquared( b1.position, b2.position );
				const float F = T * G * m1 * m2 / r2;

			#if RP3D_OLD
				if ( b1.body ) b1.body->applyForceToCenterOfMass(direction *  F);
				if ( b2.body ) b2.body->applyForceToCenterOfMass(direction * -F);
			#else
				if ( b1.body ) b1.body->applyLocalForceAtCenterOfMass(direction *  F);
				if ( b2.body ) b2.body->applyLocalForceAtCenterOfMass(direction * -F);
			#endif
			}
		}
	}
}
// synchronize bullet transforms to engine transforms
void ext::reactphysics::syncFrom( ext::reactphysics::WorldState& world, float interp ) {
	size_t count = world->getNbRigidBodies();
	for ( size_t i = 0; i < count; ++i ) {
		auto* body = world->getRigidBody(i); if ( !body ) continue;
		uf::Object* object = (uf::Object*) body->getUserData(); if ( !object || !object->isValid() ) continue;

		auto& state = object->getComponent<pod::PhysicsBody>();
		if ( !state.object ) state.object = object;
		
		auto& transform = state.object->getComponent<pod::Transform<>>();
	
		state.internal.current.transform = ::convert( body->getTransform() );
		state.internal.current.velocity = ::convert( body->getLinearVelocity() );
		state.internal.current.angularVelocity = ::convertQ( body->getAngularVelocity() );

		state.velocity = state.internal.current.velocity;
		state.angularVelocity = state.internal.current.angularVelocity;

		if ( !ext::reactphysics::interpolate ) {
			transform.position = state.internal.current.transform.position;
			transform.orientation = state.internal.current.transform.orientation;
			// state transform is an offset, un-offset
			if ( state.transform.reference ) transform.position -= state.transform.position;
		} else {
			transform.position = state.internal.previous.transform.position * ( 1.0f - interp ) + state.internal.current.transform.position * interp;
			transform.orientation = uf::quaternion::slerp(  state.internal.previous.transform.orientation, state.internal.current.transform.orientation, interp);
			// state transform is an offset, un-offset
			if ( state.transform.reference ) transform.position -= state.transform.position;
		}
	}
}
// apply impulse
void ext::reactphysics::setImpulse( pod::PhysicsBody& state, const pod::Vector3f& v ) {
	if ( !state.collider.body ) return;
#if !RP3D_OLD
	state.collider.body->resetForce();
	state.collider.body->resetTorque();
#endif
	state.collider.body->setLinearVelocity( ::convert(pod::Vector3f{}) );
	state.collider.body->setAngularVelocity( ::convert(pod::Vector3f{}) );
//	ext::reactphysics::applyImpulse( state, v );
}
void ext::reactphysics::applyImpulse( pod::PhysicsBody& state, const pod::Vector3f& v ) {
	if ( !state.collider.body ) return;

#if RP3D_OLD
	state.collider.body->applyForceToCenterOfMass( ::convert(v) );
#else
	state.collider.body->applyLocalForceAtCenterOfMass( ::convert(v) );
#endif
}
// directly move a transform
void ext::reactphysics::applyMovement( pod::PhysicsBody& state, const pod::Vector3f& v ) {
	if ( !state.collider.body ) return;

	rp3d::Transform transform = state.collider.body->getTransform();
	transform.setPosition( transform.getPosition() + ::convert(v) * uf::physics::time::delta );
	state.collider.body->setTransform(transform);
}
// directly apply a velocity
void ext::reactphysics::setVelocity( pod::PhysicsBody& state, const pod::Vector3f& v ) {
	if ( !state.collider.body ) return;
	
	state.velocity = v;
	state.collider.body->setLinearVelocity( ::convert(v) );
}
void ext::reactphysics::applyVelocity( pod::PhysicsBody& state, const pod::Vector3f& v ) {
	if ( !state.collider.body ) return;

	state.velocity += v;
	state.collider.body->setLinearVelocity( state.collider.body->getLinearVelocity() + ::convert(v) );
}
// directly rotate a transform
void ext::reactphysics::applyRotation( pod::PhysicsBody& state, const pod::Quaternion<>& q ) {
	if ( !state.collider.body ) return;

	uf::transform::rotate( state.object->getComponent<pod::Transform<>>(), q );

	auto transform = state.collider.body->getTransform();
	transform.setOrientation( transform.getOrientation() * ::convert( q ) );
	state.collider.body->setTransform(transform);
}
void ext::reactphysics::applyRotation( pod::PhysicsBody& state, const pod::Vector3f& axis, float delta ) {
	ext::reactphysics::applyRotation( state, uf::quaternion::axisAngle( axis, delta ) );
}

// ray casting
pod::RayQuery ext::reactphysics::rayCast( const pod::Ray& ray, const pod::PhysicsBody& body, float maxDistance ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& world = /*ext::reactphysics::globalStorage ? ::world :*/ scene.getComponent<ext::reactphysics::WorldState>();
	
	pod::RayQuery query;
	query.contact.penetration = maxDistance;
	
	if ( !world ) return query;

	::RaycastCallback callback;
	callback.source = body.object;
	world->raycast( rp3d::Ray( ::convert( ray.origin ), ::convert( ray.origin + ray.direction ) ), &callback );
	if ( !callback.isHit ) return query;
	uf::Object* object = (uf::Object*) callback.raycastInfo.body->getUserData();

	query.hit = callback.isHit;
	query.body = &object->getComponent<pod::PhysicsBody>();
	query.contact.contact = ray.origin + ray.direction * callback.raycastInfo.hitFraction;
	query.contact.normal = ray.direction;
	query.contact.penetration = callback.raycastInfo.hitFraction;
	
	return query;
}

// allows noclip
void ext::reactphysics::activateCollision( pod::PhysicsBody& state, bool s ) {
	if ( !state.collider.body ) return;
//	state.collider.body->setIsActive(s);
	auto colliders = state.collider.body->getNbColliders();
	for ( auto i = 0; i < colliders; ++i ) {
		auto* collider = state.collider.body->getCollider(i);
		collider->setCollisionCategoryBits(s ? 0xFF : 0x00);
		collider->setCollideWithMaskBits(s ? 0xFF : 0x00);
	}
}

float ext::reactphysics::getMass( pod::PhysicsBody& state ) {
	if ( !state.collider.body ) return state.mass;

	return (state.mass = state.collider.body->getMass());
}
void ext::reactphysics::setMass( pod::PhysicsBody& state, float mass ) {
	state.mass = mass;
	state.collider.body->setMass(mass);
}

#endif