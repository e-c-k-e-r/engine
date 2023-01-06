#if UF_USE_REACTPHYSICS
#include <uf/utils/math/physics.h>
#include <uf/ext/reactphysics/reactphysics.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/engine/scene/scene.h>
#include <uf/engine/graph/graph.h>

namespace {
	rp3d::PhysicsCommon common;
	rp3d::PhysicsWorld* world;

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
		static size_t oldCount = 0;
		uf::Mesh mesh;
		rp3d::DebugRenderer& debugRenderer = ::world->getDebugRenderer(); 
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
		graphic.process = false;

		if ( create ) {
			graphic.device = &uf::renderer::device;
			graphic.material.device = &uf::renderer::device;
			graphic.descriptor.cullMode = uf::renderer::enums::CullMode::NONE;

			graphic.material.metadata.autoInitializeUniformBuffers = false;
			graphic.material.attachShader(uf::io::root + "/shaders/base/line/vert.spv", uf::renderer::enums::Shader::VERTEX);
			graphic.material.attachShader(uf::io::root + "/shaders/base/line/frag.spv", uf::renderer::enums::Shader::FRAGMENT);
			graphic.material.metadata.autoInitializeUniformBuffers = true;

			graphic.material.getShader("vertex").buffers.emplace_back( uf::graph::storage.buffers.camera.alias() );

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

ext::reactphysics::gravity::Mode ext::reactphysics::gravity::mode = ext::reactphysics::gravity::Mode::UNIVERSAL;
float ext::reactphysics::gravity::constant = 6.67408e-11;

bool ext::reactphysics::debugDraw::enabled = false;
float ext::reactphysics::debugDraw::rate = 1.0f;
uf::stl::string ext::reactphysics::debugDraw::layer = "";
float ext::reactphysics::debugDraw::lineWidth = 1.0f;

void ext::reactphysics::initialize() {
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

	::world = ::common.createPhysicsWorld(settings);
//	::world->setEventListener(&::listener);

	if ( ext::reactphysics::debugDraw::enabled ) {
		::world->setIsDebugRenderingEnabled(true);
		rp3d::DebugRenderer& debugRenderer = ::world->getDebugRenderer(); 
		// Select the contact points and contact normals to be displayed 
		debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLIDER_AABB, true); 
		debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLISION_SHAPE, true); 
	//	debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::CONTACT_POINT, true);
	//	debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::CONTACT_NORMAL, true);
	}
}
void ext::reactphysics::tick( float delta ) {
	ext::reactphysics::syncTo();

	static float accumulator = 0;
	accumulator += uf::physics::time::delta; 
	while ( accumulator >= ext::reactphysics::timescale ) { 
		::world->update(ext::reactphysics::timescale); 
		accumulator -= ext::reactphysics::timescale; 
	}

	TIMER( ext::reactphysics::debugDraw::rate, ext::reactphysics::debugDraw::enabled ) {
		auto& scene = uf::scene::getCurrentScene();
		::debugDraw( scene );
	}

	ext::reactphysics::syncFrom( accumulator / ext::reactphysics::timescale );
}
void ext::reactphysics::terminate() {
	if ( !::world ) return;

	size_t count = ::world->getNbRigidBodies();
	for ( size_t i = 0; i < count; ++i ) {
		auto* body = ::world->getRigidBody(i); if ( !body ) continue;
		uf::Object* object = (uf::Object*) body->getUserData(); if ( !object || !object->isValid() ) continue;
		auto& state = object->getComponent<pod::PhysicsState>(); // if ( !state.shared ) continue;
		state.body = NULL;
	}

	::common.destroyPhysicsWorld(::world);
	::world = NULL;
}

// base collider creation
pod::PhysicsState& ext::reactphysics::create( uf::Object& object ) {
	auto& state = object.getComponent<pod::PhysicsState>();

	state.uid = object.getUid();
	state.object = &object;
	state.transform.reference = &object.getComponent<pod::Transform<>>();
	state.shared = ext::reactphysics::shared;

	return state;
}

void ext::reactphysics::destroy( uf::Object& object ) {
	auto& state = object.getComponent<pod::PhysicsState>();
	ext::reactphysics::destroy( state );
}
void ext::reactphysics::destroy( pod::PhysicsState& state ) {
	ext::reactphysics::detach( state );
}

void ext::reactphysics::attach( pod::PhysicsState& state ) {
	if ( !state.shape ) return;
	
	rp3d::Transform colliderTransform = rp3d::Transform::identity();
	colliderTransform.setPosition( ::convert( state.transform.position ) );
	colliderTransform.setOrientation( ::convert( state.transform.orientation ) );

	state.transform.position = {};
	state.transform.orientation = {};

	state.body = ::world->createRigidBody( ::convert( *state.transform.reference ) );
	
	auto* collider = state.body->addCollider(state.shape, colliderTransform);
	collider->setCollisionCategoryBits(0xFF);
	collider->setCollideWithMaskBits(0xFF);
	
	state.body->setUserData(state.object);
	state.body->setMass(state.stats.mass);

	if ( state.stats.mass != 0.0f ) {
		state.body->setType(rp3d::BodyType::DYNAMIC);
		state.body->updateLocalCenterOfMassFromColliders();
		state.body->updateMassPropertiesFromColliders();
	} else {
		state.body->setType(rp3d::BodyType::STATIC);
	}

	state.body->enableGravity(state.stats.gravity != pod::Vector3f{0,0,0});

	// affects air speed, bad
//	state.body->setLinearDamping(state.stats.friction);

	auto& material = collider->getMaterial();
	material.setBounciness(0);

	state.body->setLocalInertiaTensor( ::convert( state.stats.inertia ) );
}
void ext::reactphysics::detach( pod::PhysicsState& state ) {
	if ( !state.body ) return;
	::world->destroyRigidBody(state.body);
	state.body = NULL;
}

// collider for mesh (static or dynamic)
pod::PhysicsState& ext::reactphysics::create( uf::Object& object, const uf::Mesh& mesh, bool dynamic ) {
	UF_ASSERT( mesh.index.count );
	
	auto* rMesh = ::common.createTriangleMesh();

	uf::Mesh::Input vertexInput = mesh.vertex;
	uf::Mesh::Input indexInput = mesh.index;

	uf::Mesh::Attribute vertexAttribute = mesh.vertex.attributes.front();
	uf::Mesh::Attribute normalAttribute = mesh.vertex.attributes.front();
	uf::Mesh::Attribute indexAttribute = mesh.index.attributes.front();

	for ( auto& attribute : mesh.vertex.attributes ) {
		if ( attribute.descriptor.name == "position" ) vertexAttribute = attribute;
		if ( attribute.descriptor.name == "normal" ) normalAttribute = attribute;
	}
	UF_ASSERT( vertexAttribute.descriptor.name == "position" );

	rp3d::TriangleVertexArray::IndexDataType indexType = rp3d::TriangleVertexArray::IndexDataType::INDEX_INTEGER_TYPE;
	rp3d::TriangleVertexArray::VertexDataType vertexType = rp3d::TriangleVertexArray::VertexDataType::VERTEX_FLOAT_TYPE;
	rp3d::TriangleVertexArray::NormalDataType normalType = rp3d::TriangleVertexArray::NormalDataType::NORMAL_FLOAT_TYPE;
	switch ( mesh.index.size ) {
		case sizeof(uint16_t): indexType = rp3d::TriangleVertexArray::IndexDataType::INDEX_SHORT_TYPE; break;
		case sizeof(uint32_t): indexType = rp3d::TriangleVertexArray::IndexDataType::INDEX_INTEGER_TYPE; break;
		default: UF_EXCEPTION("unsupported index type"); break;
	}

	if ( mesh.indirect.count ) {
		for ( auto i = 0; i < mesh.indirect.count; ++i ) {
			vertexInput = mesh.remapVertexInput( i );
			indexInput = mesh.remapIndexInput( i );

			if ( normalAttribute.descriptor.name == "normal" ) {
				rMesh->addSubpart(new rp3d::TriangleVertexArray(
					vertexInput.count,
					(const uint8_t*) (vertexAttribute.pointer) + vertexAttribute.stride * vertexInput.first,
					vertexAttribute.stride,
					
					(const uint8_t*) (normalAttribute.pointer) + normalAttribute.stride * vertexInput.first,
					normalAttribute.stride,

					indexInput.count / 3,
					(const uint8_t*) (indexAttribute.pointer) + indexAttribute.stride * indexInput.first,
					indexAttribute.stride * 3,

					vertexType,
					normalType,
					indexType
				));
			} else {
				rMesh->addSubpart(new rp3d::TriangleVertexArray(
					vertexInput.count,
					(const uint8_t*) (vertexAttribute.pointer) + vertexAttribute.stride * vertexInput.first,
					vertexAttribute.stride,

					indexInput.count / 3,
					(const uint8_t*) (indexAttribute.pointer) + indexAttribute.stride * indexInput.first,
					indexAttribute.stride * 3,

					vertexType,
					indexType
				));
			}
		}
	} else {
		if ( normalAttribute.descriptor.name == "normal" ) {
			rMesh->addSubpart(new rp3d::TriangleVertexArray(
				vertexInput.count,
				(const uint8_t*) (vertexAttribute.pointer) + vertexAttribute.stride * vertexInput.first,
				vertexAttribute.stride,
				
				(const uint8_t*) (normalAttribute.pointer) + normalAttribute.stride * vertexInput.first,
				normalAttribute.stride,

				indexInput.count / 3,
				(const uint8_t*) (indexAttribute.pointer) + indexAttribute.stride * indexInput.first,
				indexAttribute.stride * 3,

				vertexType,
				normalType,
				indexType
			));
		} else {
			rMesh->addSubpart(new rp3d::TriangleVertexArray(
				vertexInput.count,
				(const uint8_t*) (vertexAttribute.pointer) + vertexAttribute.stride * vertexInput.first,
				vertexAttribute.stride,

				indexInput.count / 3,
				(const uint8_t*) (indexAttribute.pointer) + indexAttribute.stride * indexInput.first,
				indexAttribute.stride * 3,

				vertexType,
				indexType
			));
		}
	}

	auto& state = ext::reactphysics::create( object );
	state.shape = ::common.createConcaveMeshShape( rMesh );
	state.stats.mass = 0;
	ext::reactphysics::attach( state );

	return state;
}
// collider for boundingbox
pod::PhysicsState& ext::reactphysics::create( uf::Object& object, const pod::Vector3f& extent ) {
	auto& state = ext::reactphysics::create( object );
	state.shape = ::common.createBoxShape( rp3d::Vector3( abs(extent.x), abs(extent.y), abs(extent.z) ) );
	ext::reactphysics::attach( state );

	return state;
}
// collider for capsule
pod::PhysicsState& ext::reactphysics::create( uf::Object& object, float radius, float height ) {
	auto& state = ext::reactphysics::create( object );
	state.shape = ::common.createCapsuleShape( radius, height );
	ext::reactphysics::attach( state );

	return state;
}

// synchronize engine transforms to bullet transforms
void ext::reactphysics::syncTo() {
	size_t count = ::world->getNbRigidBodies();
	
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
		auto* body = ::world->getRigidBody(i); if ( !body ) continue;
		uf::Object* object = (uf::Object*) body->getUserData(); if ( !object || !object->isValid() ) continue;
		auto& state = object->getComponent<pod::PhysicsState>(); // if ( !state.shared ) continue;

		if ( state.shared ) {
			if ( !ext::reactphysics::interpolate ) body->setTransform(::convert(state.transform));
			body->setLinearVelocity( ::convert(state.linear.velocity) );
			body->setAngularVelocity( ::convertQ(state.rotational.velocity) );
		}
		// apply per-object gravities
		float mass = body->getMass();
		switch ( ext::reactphysics::gravity::mode ) {
			case ext::reactphysics::gravity::Mode::PER_OBJECT: if ( body->isGravityEnabled() ) {
			#if UF_ENV_DREAMCAST
				body->applyForceToCenterOfMass( ::convert(state.stats.gravity * mass) );
			#else
				body->applyLocalForceAtCenterOfMass( ::convert(state.stats.gravity * mass) );
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

			#if UF_ENV_DREAMCAST
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
void ext::reactphysics::syncFrom( float interp ) {
	size_t count = ::world->getNbRigidBodies();
	for ( size_t i = 0; i < count; ++i ) {
		auto* body = ::world->getRigidBody(i); if ( !body ) continue;
		uf::Object* object = (uf::Object*) body->getUserData(); if ( !object ) continue;

		auto& state = object->getComponent<pod::PhysicsState>();

		if ( !state.object ) {
			state.object = object;
			//continue;
		}
		
		auto& transform = state.object->getComponent<pod::Transform<>>();
		auto& physics = state.object->getComponent<pod::Physics>();

	/*
		transform.position = ::convert( body->getTransform().getPosition() );
		transform.orientation = ::convert( body->getTransform().getOrientation() );
	
		// state transform is an offset, un-offset
		if ( state.transform.reference ) transform.position -= state.transform.position;

	//	transform = uf::transform::reorient( transform );
	*/
	
		state.internal.current.transform = ::convert( body->getTransform() );
		state.internal.current.linear.velocity = ::convert( body->getLinearVelocity() );
		state.internal.current.rotational.velocity = ::convertQ( body->getAngularVelocity() );

		physics.linear.velocity = state.internal.current.linear.velocity;
		physics.rotational.velocity = state.internal.current.rotational.velocity;

		if ( !ext::reactphysics::interpolate ) {
			transform.position = state.internal.current.transform.position;
			transform.orientation = state.internal.current.transform.orientation;
	
			// state transform is an offset, un-offset
			if ( state.transform.reference ) transform.position -= state.transform.position;

		//	transform = uf::transform::reorient( transform );
		} else {
		//	transform = uf::transform::interpolate( state.internal.previous.transform, state.internal.current.transform, interp );

			transform.position = state.internal.previous.transform.position * ( 1.0f - interp ) + state.internal.current.transform.position * interp;
			transform.orientation = uf::quaternion::slerp(  state.internal.previous.transform.orientation, state.internal.current.transform.orientation, interp);
			if ( state.transform.reference ) transform.position -= state.transform.position;

		//	physics.linear.velocity = uf::vector::lerp( state.internal.previous.linear.velocity, state.internal.current.linear.velocity, interp );
		//	physics.rotational.velocity = uf::quaternion::slerp( state.internal.previous.rotational.velocity, state.internal.current.rotational.velocity, interp );
		}
	}
}
// apply impulse
void ext::reactphysics::setImpulse( pod::PhysicsState& state, const pod::Vector3f& v ) {
	if ( !state.body ) return;
#if !UF_ENV_DREAMCAST
	state.body->resetForce();
	state.body->resetTorque();
#endif
	state.body->setLinearVelocity( ::convert(pod::Vector3f{}) );
	state.body->setAngularVelocity( ::convert(pod::Vector3f{}) );
//	ext::reactphysics::applyImpulse( state, v );
}
void ext::reactphysics::applyImpulse( pod::PhysicsState& state, const pod::Vector3f& v ) {
	if ( !state.body ) return;

#if UF_ENV_DREAMCAST
	state.body->applyForceToCenterOfMass( ::convert(v) );
#else
	state.body->applyLocalForceAtCenterOfMass( ::convert(v) );
#endif
}
// directly move a transform
void ext::reactphysics::applyMovement( pod::PhysicsState& state, const pod::Vector3f& v ) {
	if ( !state.body ) return;

	rp3d::Transform transform = state.body->getTransform();
	transform.setPosition( transform.getPosition() + ::convert(v) * uf::physics::time::delta );
	state.body->setTransform(transform);
}
// directly apply a velocity
void ext::reactphysics::setVelocity( pod::PhysicsState& state, const pod::Vector3f& v ) {
	if ( !state.body ) return;
	if ( state.shared ) {
		auto& physics = state.object->getComponent<pod::Physics>();
		physics.linear.velocity = v;
	//	return;
	}
	state.body->setLinearVelocity( ::convert(v) );
}
void ext::reactphysics::applyVelocity( pod::PhysicsState& state, const pod::Vector3f& v ) {
	if ( !state.body ) return;

	if ( state.shared ) {
		auto& physics = state.object->getComponent<pod::Physics>();
		physics.linear.velocity += v;
	 //	return;
	}
	state.body->setLinearVelocity( state.body->getLinearVelocity() + ::convert(v) );
}
// directly rotate a transform
void ext::reactphysics::applyRotation( pod::PhysicsState& state, const pod::Quaternion<>& q ) {
	if ( !state.body ) return;

	if ( state.shared ) {
		auto& transform = state.object->getComponent<pod::Transform<>>();
		uf::transform::rotate( transform, q );
	//	return;
	}
	auto transform = state.body->getTransform();
	transform.setOrientation( transform.getOrientation() * ::convert( q ) );
	state.body->setTransform(transform);
}
void ext::reactphysics::applyRotation( pod::PhysicsState& state, const pod::Vector3f& axis, float delta ) {
	ext::reactphysics::applyRotation( state, uf::quaternion::axisAngle( axis, delta ) );
}

// ray casting

uf::Object* ext::reactphysics::rayCast( const pod::Vector3f& center, const pod::Vector3f& direction ) {	
	float depth = -1;
	return rayCast( center, direction, NULL, depth );
}

uf::Object* ext::reactphysics::rayCast( pod::PhysicsState& state, const pod::Vector3f& center, const pod::Vector3f& direction ) {	
	float depth = -1;
	return rayCast( center, direction, state.object, depth );
}
uf::Object* ext::reactphysics::rayCast( pod::PhysicsState& state, const pod::Vector3f& center, const pod::Vector3f& direction, float& depth ) {
	return rayCast( center, direction, state.object, depth );
}

uf::Object* ext::reactphysics::rayCast( const pod::Vector3f& center, const pod::Vector3f& direction, uf::Object* source, float& depth ) {
	depth = -1;
	
	if ( !::world ) return NULL;

	::RaycastCallback callback;
	callback.source = source;
	::world->raycast( rp3d::Ray( ::convert( center ), ::convert( center + direction ) ), &callback );
	if ( !callback.isHit ) return NULL;
	
	depth = callback.raycastInfo.hitFraction;

	return (uf::Object*) callback.raycastInfo.body->getUserData();
}

// allows noclip
void ext::reactphysics::activateCollision( pod::PhysicsState& state, bool s ) {
	if ( !state.body ) return;
//	state.body->setIsActive(s);
	auto colliders = state.body->getNbColliders();
	for ( auto i = 0; i < colliders; ++i ) {
		auto* collider = state.body->getCollider(i);
		collider->setCollisionCategoryBits(s ? 0xFF : 0x00);
		collider->setCollideWithMaskBits(s ? 0xFF : 0x00);
	}
}

float ext::reactphysics::getMass( pod::PhysicsState& state ) {
	if ( !state.body ) return state.stats.mass;

	return (state.stats.mass = state.body->getMass());
}
void ext::reactphysics::setMass( pod::PhysicsState& state, float mass ) {
	state.stats.mass = mass;
	state.body->setMass(mass);
}

#endif