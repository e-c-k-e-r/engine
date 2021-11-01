#if UF_USE_REACTPHYSICS
#include <uf/utils/math/physics.h>
#include <uf/ext/reactphysics/reactphysics.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/engine/scene/scene.h>
#include <uf/engine/graph/graph.h>

#define UF_PHYSICS_SHARED 1

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
		rp3d::RaycastInfo raycastInfo;

		virtual rp3d::decimal notifyRaycastHit(const rp3d::RaycastInfo& info) override {
			if ( !isHit || raycastInfo.hitFraction > info.hitFraction ) {
				raycastInfo.body = info.body;
				raycastInfo.hitFraction = info.hitFraction;
				raycastInfo.collider = info.collider;
				raycastInfo.worldNormal = info.worldNormal;
				raycastInfo.worldPoint = info.worldPoint;
				isHit = true;
			}
		//	return rp3d::decimal(1.0);
			return raycastInfo.hitFraction;
		}
	};


	rp3d::DefaultLogger* logger = NULL;
}

float ext::reactphysics::timescale = 1.0f / 60.0f;
bool ext::reactphysics::debugDrawEnabled = false;
float ext::reactphysics::debugDrawRate = 1.0f;
uf::stl::string ext::reactphysics::debugDrawLayer = "";
float ext::reactphysics::debugDrawLineWidth = 1.0f;

void ext::reactphysics::initialize() {
	rp3d::PhysicsWorld::WorldSettings settings;
	settings.gravity = rp3d::Vector3( 0, -9.81, 0 );

//	::logger = ::common.createDefaultLogger();
//	size_t logLevel = static_cast<uint>(rp3d::Logger::Level::Warning) | static_cast<uint>(rp3d::Logger::Level::Error); // | static_cast<uint>(rp3d::Logger::Level::Information);
//	logger->addFileDestination("./data/logs/rp3d_log_.html", logLevel, rp3d::DefaultLogger::Format::HTML); 
//	::common.setLogger(::logger);

	::world = ::common.createPhysicsWorld(settings);
//	::world->setEventListener(&::listener);

	if ( ext::reactphysics::debugDrawEnabled ) {
		::world->setIsDebugRenderingEnabled(true);
		rp3d::DebugRenderer& debugRenderer = ::world->getDebugRenderer(); 
		// Select the contact points and contact normals to be displayed 
		debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLIDER_AABB, true); 
		debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLISION_SHAPE, true); 
		debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::CONTACT_POINT, true);
		debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::CONTACT_NORMAL, true);
	}
}
void ext::reactphysics::tick( float delta ) {
#if UF_PHYSICS_SHARED
	ext::reactphysics::syncTo();
#endif

	static float accumulator = 0;

	accumulator += uf::physics::time::delta; 

	while ( accumulator >= ext::reactphysics::timescale ) { 
		::world->update(ext::reactphysics::timescale); 
		accumulator -= ext::reactphysics::timescale; 
	}

	TIMER(ext::reactphysics::debugDrawRate, ext::reactphysics::debugDrawEnabled && ) {
		auto& scene = uf::scene::getCurrentScene();
		ext::reactphysics::debugDraw( scene );
	}

	ext::reactphysics::syncFrom();
}
void ext::reactphysics::terminate() {
	::common.destroyPhysicsWorld(::world);
	::world = NULL;
}

// base collider creation
pod::PhysicsState& ext::reactphysics::create( uf::Object& object ) {
	auto& state = object.getComponent<pod::PhysicsState>();

	state.uid = object.getUid();
	state.object = &object;
	state.transform.reference = &object.getComponent<pod::Transform<>>();
#if UF_PHYSICS_SHARED
	state.shared = true;
#else
	state.shared = false;
#endif
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

	rp3d::Transform bodyTransform = rp3d::Transform::identity();
	rp3d::Transform colliderTransform = rp3d::Transform::identity();
	
#if !UF_SPOOKY_JANK
	colliderTransform.setPosition( rp3d::Vector3( state.transform.position.x, state.transform.position.y, state.transform.position.z ) );
	colliderTransform.setOrientation( rp3d::Quaternion( state.transform.orientation.x, state.transform.orientation.y, state.transform.orientation.z, state.transform.orientation.w ) );

	state.transform.position = {};
	state.transform.orientation = {};

	auto model = uf::transform::model( *state.transform.reference );
	bodyTransform.setFromOpenGL(&model[0]);
#else
	// has a parent, separate main transform to collider, and parent transform to body
	if ( state.transform.reference ) {
		auto model = uf::transform::model( *state.transform.reference );
		bodyTransform.setFromOpenGL(&model[0]);

		colliderTransform.setPosition( rp3d::Vector3( state.transform.position.x, state.transform.position.y, state.transform.position.z ) );
		colliderTransform.setOrientation( rp3d::Quaternion( state.transform.orientation.x, state.transform.orientation.y, state.transform.orientation.z, state.transform.orientation.w ) );
	// no parent, use for both
	} else {
		auto model = uf::transform::model( state.transform );
		bodyTransform.setFromOpenGL(&model[0]);
	}
#endif

	state.body = ::world->createRigidBody(bodyTransform);
	auto* collider = state.body->addCollider(state.shape, colliderTransform);
	state.body->setUserData(state.object);
	state.body->setMass(state.stats.mass);
	if ( state.stats.mass != 0.0f ) {
		state.body->setType(rp3d::BodyType::DYNAMIC);
		state.body->updateLocalCenterOfMassFromColliders();
		state.body->updateMassPropertiesFromColliders();
	} else {
		state.body->setType(rp3d::BodyType::STATIC);
	}

	auto& material = collider->getMaterial();
	material.setBounciness(0);

	state.body->setLocalInertiaTensor( rp3d::Vector3( state.stats.inertia.x, state.stats.inertia.y, state.stats.inertia.z ) );
}
void ext::reactphysics::detach( pod::PhysicsState& state ) {
	if ( !state.body ) return;
	::world->destroyRigidBody(state.body);
	state.body = NULL;
}

// collider for mesh (static or dynamic)
pod::PhysicsState& ext::reactphysics::create( uf::Object& object, const uf::Mesh& mesh, bool dynamic ) {
	auto* rMesh = ::common.createTriangleMesh();

	if ( mesh.index.count ) {
		uf::Mesh::Attribute vertexAttribute;
		for ( auto& attribute : mesh.vertex.attributes ) if ( attribute.descriptor.name == "position" ) { vertexAttribute = attribute; break; }
		UF_ASSERT( vertexAttribute.descriptor.name == "position" );

		auto& indexAttribute = mesh.index.attributes.front();
		rp3d::TriangleVertexArray::IndexDataType indexType = rp3d::TriangleVertexArray::IndexDataType::INDEX_INTEGER_TYPE;
		rp3d::TriangleVertexArray::VertexDataType vertexType = rp3d::TriangleVertexArray::VertexDataType::VERTEX_FLOAT_TYPE;
		switch ( mesh.index.stride ) {
			case sizeof(uint16_t): indexType = rp3d::TriangleVertexArray::IndexDataType::INDEX_SHORT_TYPE; break;
			case sizeof(uint32_t): indexType = rp3d::TriangleVertexArray::IndexDataType::INDEX_INTEGER_TYPE; break;
			default: UF_EXCEPTION("unsupported index type"); break;
		}

		rp3d::TriangleVertexArray* vArray;
		if ( mesh.indirect.count ) {
			uf::Mesh::Attribute remappedVertexAttribute;
			uf::Mesh::Attribute remappedIndexAttribute;
			for ( auto i = 0; i < mesh.indirect.count; ++i ) {
				remappedVertexAttribute = mesh.remapVertexAttribute( vertexAttribute, i );
				remappedIndexAttribute = mesh.remapIndexAttribute( indexAttribute, i );
				
				vArray = new rp3d::TriangleVertexArray(
					remappedVertexAttribute.length,
					(const uint8_t*) remappedVertexAttribute.pointer,
					remappedVertexAttribute.stride,

					remappedIndexAttribute.length / 3,
					(const uint8_t*) remappedIndexAttribute.pointer,
					remappedIndexAttribute.stride * 3,

					vertexType,
					indexType
				);

				rMesh->addSubpart(vArray);
			}
		} else {
			vArray = new rp3d::TriangleVertexArray(
				vertexAttribute.length,
				(const uint8_t*) vertexAttribute.pointer,
				vertexAttribute.stride,

				indexAttribute.length / 3,
				(const uint8_t*) indexAttribute.pointer,
				indexAttribute.stride * 3,

				vertexType,
				indexType
			);

			rMesh->addSubpart(vArray);
		}
	} else UF_EXCEPTION("to-do: not require indices for meshes");

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
	for ( size_t i = 0; i < count; ++i ) {
		auto* body = ::world->getRigidBody(i); if ( !body ) continue;
		uf::Object* object = (uf::Object*) body->getUserData(); if ( !object ) continue;
		auto& state = object->getComponent<pod::PhysicsState>(); if ( !state.shared ) continue;

		auto model = uf::transform::model( state.transform );

		rp3d::Transform transform;
		transform.setFromOpenGL(&model[0]);

		body->setTransform(transform);
		body->setLinearVelocity( rp3d::Vector3( state.linear.velocity.x, state.linear.velocity.y, state.linear.velocity.z ) );
	}
}
// synchronize bullet transforms to engine transforms
void ext::reactphysics::syncFrom() {
	size_t count = ::world->getNbRigidBodies();
	for ( size_t i = 0; i < count; ++i ) {
		auto* body = ::world->getRigidBody(i); if ( !body ) continue;
		uf::Object* object = (uf::Object*) body->getUserData(); if ( !object ) continue;

		auto position = body->getTransform().getPosition();
		auto orientation = body->getTransform().getOrientation();
		auto linearVelocity = body->getLinearVelocity();
		auto rotationalVelocity = body->getAngularVelocity();

		auto& state = object->getComponent<pod::PhysicsState>();;
		auto& transform = state.object->getComponent<pod::Transform<>>();
		auto& physics = state.object->getComponent<pod::Physics>();

		/*state.*/transform.position.x = position.x;
		/*state.*/transform.position.y = position.y;
		/*state.*/transform.position.z = position.z;

		// state transform is an offset, un-offset
		if ( state.transform.reference ) {
			transform.position -= state.transform.position;
		}
		
		/*state.*/transform.orientation.x = orientation.x;
		/*state.*/transform.orientation.y = orientation.y;
		/*state.*/transform.orientation.z = orientation.z;
		/*state.*/transform.orientation.w = orientation.w;

		physics.linear.velocity.x = linearVelocity.x;
		physics.linear.velocity.y = linearVelocity.y;
		physics.linear.velocity.z = linearVelocity.z;
		
		physics.rotational.velocity.x = rotationalVelocity.x;
		physics.rotational.velocity.y = rotationalVelocity.y;
		physics.rotational.velocity.z = rotationalVelocity.z;
	}
}
// apply impulse
void ext::reactphysics::applyImpulse( pod::PhysicsState& state, const pod::Vector3f& v ) {
}
// directly move a transform
void ext::reactphysics::applyMovement( pod::PhysicsState& state, const pod::Vector3f& v ) {
	if ( !state.body ) return;

	rp3d::Transform transform = state.body->getTransform();
	transform.setPosition( transform.getPosition() + rp3d::Vector3( v.x, v.y, v.z ) * uf::physics::time::delta );
	state.body->setTransform(transform);
}
// directly apply a velocity
void ext::reactphysics::setVelocity( pod::PhysicsState& state, const pod::Vector3f& v ) {
	if ( !state.body ) return;
	if ( state.shared ) {
		auto& physics = state.object->getComponent<pod::Physics>();
		physics.linear.velocity = v;
	} else {
		state.body->setLinearVelocity( rp3d::Vector3( v.x, v.y, v.z ) );
	}
}
void ext::reactphysics::applyVelocity( pod::PhysicsState& state, const pod::Vector3f& v ) {
	if ( !state.body ) return;

	if ( state.shared ) {
		auto& physics = state.object->getComponent<pod::Physics>();
		physics.linear.velocity += v;
	} else {
		state.body->setLinearVelocity( state.body->getLinearVelocity() + rp3d::Vector3( v.x, v.y, v.z ) );
	}
}
// directly rotate a transform
void ext::reactphysics::applyRotation( pod::PhysicsState& state, const pod::Quaternion<>& q ) {
	if ( !state.body ) return;

	if ( state.shared ) {
		auto& transform = state.object->getComponent<pod::Transform<>>();
		uf::transform::rotate( transform, q );
		return;
	}

	auto transform = state.body->getTransform();
	auto o = transform.getOrientation();
	pod::Quaternion<> orientation = uf::vector::normalize(uf::quaternion::multiply({ o.x, o.y, o.z, o.w, }, q));
	transform.setOrientation( rp3d::Quaternion( orientation.x, orientation.y, orientation.z, orientation.w ) );

	state.body->setTransform(transform);
}
void ext::reactphysics::applyRotation( pod::PhysicsState& state, const pod::Vector3f& axis, float delta ) {
	ext::reactphysics::applyRotation( state, uf::quaternion::axisAngle( axis, delta ) );
}

// ray casting

float ext::reactphysics::rayCast( const pod::Vector3f& center, const pod::Vector3f& direction ) {	
	if ( !::world )
		return -1;

	::RaycastCallback callback;
	::world->raycast( rp3d::Ray( rp3d::Vector3( center.x, center.y, center.z ), rp3d::Vector3( direction.x, direction.y, direction.z ) ), &callback );
	if ( !callback.isHit ) return -1;
	return callback.raycastInfo.hitFraction;
}
float ext::reactphysics::rayCast( const pod::Vector3f& center, const pod::Vector3f& direction, size_t& uid ) {
	if ( !::world )
		return -1;

	::RaycastCallback callback;
	::world->raycast( rp3d::Ray( rp3d::Vector3( center.x, center.y, center.z ), rp3d::Vector3( direction.x, direction.y, direction.z ) ), &callback );
	uid = 0;
	if ( !callback.isHit ) {
		return -1;
	}
	auto* object = (uf::Object*) callback.raycastInfo.body->getUserData();
	uid = object->getUid();
	return callback.raycastInfo.hitFraction;
}
float ext::reactphysics::rayCast( const pod::Vector3f& center, const pod::Vector3f& direction, uf::Object*& object ) {
	if ( !::world )
		return -1;

	::RaycastCallback callback;
	::world->raycast( rp3d::Ray( rp3d::Vector3( center.x, center.y, center.z ), rp3d::Vector3( direction.x, direction.y, direction.z ) ), &callback );
	object = NULL;
	if ( !callback.isHit ) {
		return -1;
	}
	object = (uf::Object*) callback.raycastInfo.body->getUserData();
	return callback.raycastInfo.hitFraction;
}

// allows noclip
void ext::reactphysics::activateCollision( pod::PhysicsState& state, bool s ) {

}


struct VertexLine {
	pod::Vector3f position;
	pod::Vector3f color;

	static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
};

UF_VERTEX_DESCRIPTOR(VertexLine,
	UF_VERTEX_DESCRIPTION(VertexLine, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(VertexLine, R32G32B32_SFLOAT, color)
);

namespace {
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
}

// allows showing collision models
void ext::reactphysics::debugDraw( uf::Object& object ) {
	static size_t oldCount = 0;
	uf::Mesh mesh;
	rp3d::DebugRenderer& debugRenderer = ::world->getDebugRenderer(); 
	if ( !mesh.hasVertex<VertexLine>() ) mesh.bind<VertexLine>();

	size_t lineCount = debugRenderer.getNbLines();
	size_t triCount = debugRenderer.getNbTriangles();

	if ( !lineCount || !triCount ) return;
	if ( oldCount == lineCount * 2 + triCount * 3 ) return;
	oldCount = lineCount * 2 + triCount * 3;

	auto* lines = debugRenderer.getLinesArray();
	auto* tris = debugRenderer.getTrianglesArray();

	uf::stl::vector<VertexLine> vertices;
	vertices.reserve( lineCount * 2 + triCount * 3 );
	for ( size_t i = 0; i < lineCount; ++i ) {
		auto& line = lines[i];

		auto& vertex1 = vertices.emplace_back();
		vertex1.position = { line.point1.x, line.point1.y, line.point1.z };
		vertex1.color = uintToVector( line.color1 );

		auto& vertex2 = vertices.emplace_back();
		vertex2.position = { line.point2.x, line.point2.y, line.point2.z };
		vertex2.color = uintToVector( line.color2 );
	}
	for ( size_t i = 0; i < triCount; ++i ) {
		auto& tri = tris[i];

		auto& vertex1 = vertices.emplace_back();
		vertex1.position = { tri.point1.x, tri.point1.y, tri.point1.z };
		vertex1.color = uintToVector( tri.color1 );

		auto& vertex2 = vertices.emplace_back();
		vertex2.position = { tri.point2.x, tri.point2.y, tri.point2.z };
		vertex2.color = uintToVector( tri.color2 );

		auto& vertex3 = vertices.emplace_back();
		vertex3.position = { tri.point3.x, tri.point3.y, tri.point3.z };
		vertex3.color = uintToVector( tri.color3 );
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

		graphic.material.metadata.autoInitializeUniforms = false;
		graphic.material.attachShader(uf::io::root + "/shaders/bullet/base.vert.spv", uf::renderer::enums::Shader::VERTEX);
		graphic.material.attachShader(uf::io::root + "/shaders/bullet/base.frag.spv", uf::renderer::enums::Shader::FRAGMENT);
		graphic.material.metadata.autoInitializeUniforms = true;

		graphic.material.getShader("vertex").buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.camera );

		graphic.initialize(ext::reactphysics::debugDrawLayer);
		graphic.initializeMesh( mesh );

		graphic.descriptor.topology = uf::renderer::enums::PrimitiveTopology::LINE_LIST;
		graphic.descriptor.fill = uf::renderer::enums::PolygonMode::LINE;
		graphic.descriptor.lineWidth = ext::reactphysics::debugDrawLineWidth;
	} else {
		if ( graphic.updateMesh( mesh ) ) {
			graphic.getPipeline().update( graphic );
		}
	}
	graphic.process = true;
}
#endif