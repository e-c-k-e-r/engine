#if UF_USE_BULLET
#include <uf/ext/bullet/bullet.h>

#include <uf/utils/math/physics.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/engine/scene/scene.h>
#include <uf/engine/graph/graph.h>
#include <BulletCollision/CollisionShapes/btTriangleShape.h>

namespace {
	struct VertexLine {
		pod::Vector3f position;
		pod::Vector3f color;

		static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
	};

	UF_VERTEX_DESCRIPTOR(VertexLine,
		UF_VERTEX_DESCRIPTION(VertexLine, R32G32B32_SFLOAT, position)
		UF_VERTEX_DESCRIPTION(VertexLine, R32G32B32_SFLOAT, color)
	);

	class BulletDebugDrawer : public btIDebugDraw {
	protected:
		int m;
		uf::Mesh mesh;
	public:
		virtual void drawLine( const btVector3& from, const btVector3& to, const btVector3& color ) {
			drawLine( from, to, color, color );
		}
		virtual void drawLine( const btVector3& from, const btVector3& to, const btVector3& fromColor, const btVector3& toColor ) {
			uf::stl::vector<VertexLine> vertices;
			{
				auto& vertex = vertices.emplace_back();
				vertex.position = { from.getX(), from.getY(), from.getZ() };
				vertex.color = { fromColor.getX(), fromColor.getY(), fromColor.getZ() };
			}
			{
				auto& vertex = vertices.emplace_back();
				vertex.position = { to.getX(), to.getY(), to.getZ() };
				vertex.color = { toColor.getX(), toColor.getY(), toColor.getZ() };
			}
			if ( !mesh.hasVertex<VertexLine>() ) mesh.bind<VertexLine>();
			mesh.insertVertices(vertices);
		}
		virtual void drawContactPoint(const btVector3&, const btVector3&, btScalar, int, const btVector3&) {

		}
		virtual void reportErrorWarning(const char* str ) {
			UF_MSG_WARNING("[Bullet] " << str);
		}
		virtual void draw3dText(const btVector3& , const char* str ) {

		}
		virtual void setDebugMode(int p) {
			m = p;
		}
		virtual void clearLines() {
			for ( auto& buffer : mesh.buffers ) buffer.clear();
			mesh.vertex.count = 0;
			mesh.index.count = 0;
		}
		virtual void flushLines() {
			auto& scene = uf::scene::getCurrentScene();
			ext::bullet::debugDraw( scene );
		}
		int getDebugMode(void) const { return m; }

		uf::Mesh& getMesh() { return mesh; }
		const uf::Mesh& getMesh() const { return mesh; }
	};

	void UF_API ext::bullet::debugDraw( uf::Object& object ) {
		auto& mesh = ext::bullet::debugDrawer.getMesh();
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

			graphic.initialize(ext::bullet::debugDrawLayer);
			graphic.initializeMesh( mesh );

			graphic.descriptor.topology = uf::renderer::enums::PrimitiveTopology::LINE_LIST;
			graphic.descriptor.fill = uf::renderer::enums::PolygonMode::LINE;
			graphic.descriptor.lineWidth = ext::bullet::debugDrawLineWidth;
		} else {
			if ( graphic.updateMesh( mesh ) ) {
				graphic.getPipeline().update( graphic );
			}
		}
		graphic.process = true;
	}
}

size_t ext::bullet::iterations = 1;
size_t ext::bullet::substeps = 12;
float ext::bullet::timescale = 1.0f;

size_t ext::bullet::defaultMaxCollisionAlgorithmPoolSize = 65535;
size_t ext::bullet::defaultMaxPersistentManifoldPoolSize = 65535;

bool ext::bullet::debugDrawEnabled = false;
float ext::bullet::debugDrawRate = 1.0f;
uf::stl::string ext::bullet::debugDrawLayer = "";
float ext::bullet::debugDrawLineWidth = 1.0f;

namespace ext {
	namespace bullet {
		btDefaultCollisionConstructionInfo constructionInfo;
		btDefaultCollisionConfiguration* collisionConfiguration = NULL;
		btCollisionDispatcher* dispatcher = NULL;
		btBroadphaseInterface* overlappingPairCache = NULL;
		btSequentialImpulseConstraintSolver* solver = NULL;
		btDiscreteDynamicsWorld* dynamicsWorld = NULL;
		btAlignedObjectArray<btCollisionShape*> collisionShapes;

		uf::stl::unordered_map<btRigidBody*, size_t> rigidBodyMap;

		BulletDebugDrawer debugDrawer;
	}
}

#if !UF_ENV_DREAMCAST
static bool contactCallback(btManifoldPoint &ManifoldPoint, const btCollisionObjectWrapper *Object0, int PartID0, int Index0, const btCollisionObjectWrapper *Object1, int PartID1, int Index1) {
	if( Object1->getCollisionShape()->getShapeType() != TRIANGLE_SHAPE_PROXYTYPE ) return false;
/*
	pod::Vector3f Before = {
		ManifoldPoint.m_normalWorldOnB.getX(),
		ManifoldPoint.m_normalWorldOnB.getY(),
		ManifoldPoint.m_normalWorldOnB.getZ(),
	};
*/
	btAdjustInternalEdgeContacts(ManifoldPoint, Object1, Object0, PartID1, Index1);
	btAdjustInternalEdgeContacts(ManifoldPoint, Object0, Object1, PartID0, Index0);
/*
	pod::Vector3f After = {
		ManifoldPoint.m_normalWorldOnB.getX(),
		ManifoldPoint.m_normalWorldOnB.getY(),
		ManifoldPoint.m_normalWorldOnB.getZ(),
	};
*/
	return false;
}
#endif


void ext::bullet::initialize() {
	ext::bullet::constructionInfo = btDefaultCollisionConstructionInfo();
	ext::bullet::constructionInfo.m_defaultMaxCollisionAlgorithmPoolSize = ext::bullet::defaultMaxCollisionAlgorithmPoolSize;
	ext::bullet::constructionInfo.m_defaultMaxPersistentManifoldPoolSize = ext::bullet::defaultMaxPersistentManifoldPoolSize;
	ext::bullet::collisionConfiguration = new btDefaultCollisionConfiguration(ext::bullet::constructionInfo);
	ext::bullet::dispatcher = new btCollisionDispatcher(ext::bullet::collisionConfiguration);
	ext::bullet::overlappingPairCache = new btDbvtBroadphase(); // new btAxis3Sweep();
	ext::bullet::solver = new btSequentialImpulseConstraintSolver;
	ext::bullet::dynamicsWorld = new btDiscreteDynamicsWorld(ext::bullet::dispatcher, ext::bullet::overlappingPairCache, ext::bullet::solver, ext::bullet::collisionConfiguration);
	ext::bullet::dynamicsWorld->setGravity(btVector3(0, -9.81, 0));

	ext::bullet::debugDrawer.setDebugMode(btIDebugDraw::DBG_DrawWireframe);
 	ext::bullet::dynamicsWorld->setDebugDrawer(&ext::bullet::debugDrawer);

	auto& mesh = ext::bullet::debugDrawer.getMesh();
	mesh.bind<VertexLine>();

#if !UF_ENV_DREAMCAST
	gContactAddedCallback = contactCallback;
#endif
}
void ext::bullet::tick( float delta ) { if ( delta == 0.0f ) delta = uf::physics::time::delta;
	ext::bullet::syncTo();

	static float accumulator = 0;
	static const float timeStep = 1.0f / 60.0f; 

	accumulator += uf::physics::time::delta; 

	while ( accumulator >= timeStep ) { 
		ext::bullet::dynamicsWorld->stepSimulation( delta, ext::bullet::substeps, timeStep );
		accumulator -= timeStep; 
	}
/*
	delta = delta * ext::bullet::timescale / ext::bullet::iterations;
	for ( size_t i = 0; i < ext::bullet::iterations; ++i ) {
		ext::bullet::dynamicsWorld->stepSimulation(delta, ext::bullet::substeps, delta / ext::bullet::substeps);
	}
*/
	ext::bullet::syncFrom();
	TIMER(ext::bullet::debugDrawRate, ext::bullet::debugDrawEnabled && ) {
		ext::bullet::dynamicsWorld->debugDrawWorld();
	}
}
void ext::bullet::terminate() {
	for ( int i = ext::bullet::dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
		btCollisionObject* obj = ext::bullet::dynamicsWorld->getCollisionObjectArray()[i];
		btRigidBody* body = btRigidBody::upcast(obj);
		if (body && body->getMotionState()) delete body->getMotionState();
		ext::bullet::dynamicsWorld->removeCollisionObject(obj);
		delete obj;
	}
	for ( size_t i = 0; i < ext::bullet::collisionShapes.size(); ++i ) {
		btCollisionShape* shape = ext::bullet::collisionShapes[i];
		ext::bullet::collisionShapes[i] = NULL;
		delete shape;
	}

	if ( ext::bullet::dynamicsWorld ) delete ext::bullet::dynamicsWorld;
	if ( ext::bullet::solver ) delete ext::bullet::solver;
	if ( ext::bullet::overlappingPairCache ) delete ext::bullet::overlappingPairCache;
	if ( ext::bullet::dispatcher ) delete ext::bullet::dispatcher;
	if ( ext::bullet::collisionConfiguration ) delete ext::bullet::collisionConfiguration;
	ext::bullet::collisionShapes.clear();
}
void ext::bullet::syncTo() {
	// update bullet transforms
	for ( int i = ext::bullet::dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; --i) {
		btCollisionObject* obj = ext::bullet::dynamicsWorld->getCollisionObjectArray()[i];
		
		btRigidBody* body = btRigidBody::upcast(obj);
		if ( !body || !body->getMotionState() ) return;
	
		uf::Object* entity = (uf::Object*) body->getUserPointer();
		if ( !entity || !entity->isValid() || !entity->hasComponent<pod::PhysicsState>() ) continue;

		auto& state = entity->getComponent<pod::PhysicsState>();
		if ( !state.shared ) continue;
				
		auto& physics = entity->getComponent<pod::Physics>();
		auto model = uf::transform::model( state.transform );
		
		btTransform t = body->getWorldTransform();
		t.setFromOpenGLMatrix(&model[0]);
	//	t.setOrigin( btVector3( transform.position.x, transform.position.y, transform.position.z ) );
	//	t.setRotation( btQuaternion( transform.orientation.x, transform.orientation.y, transform.orientation.z, transform.orientation.w ) );

		body->setWorldTransform(t);
		body->setCenterOfMassTransform(t);
		body->setLinearVelocity( btVector3( physics.linear.velocity.x, physics.linear.velocity.y, physics.linear.velocity.z ) );
	}
}
void ext::bullet::syncFrom() {
	for ( int i = ext::bullet::dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; --i) {
		btCollisionObject* obj = ext::bullet::dynamicsWorld->getCollisionObjectArray()[i];
		btRigidBody* body = btRigidBody::upcast(obj);
		if ( !body || !body->getMotionState() ) return;
		btTransform t = body->getWorldTransform();
	//	body->getMotionState()->getWorldTransform(t);

		uf::Object* object = (uf::Object*) body->getUserPointer();
		if ( !object || !object->isValid() || !object->hasComponent<pod::PhysicsState>() ) continue;

		auto& state = object->getComponent<pod::PhysicsState>();;
		auto& transform = object->getComponent<pod::Transform<>>();
		auto& physics = object->getComponent<pod::Physics>();

		/*state.*/transform.position.x = t.getOrigin().getX();
		/*state.*/transform.position.y = t.getOrigin().getY();
		/*state.*/transform.position.z = t.getOrigin().getZ();

		// state transform is an offset, un-offset
		if ( state.transform.reference ) {
			transform.position -= state.transform.position;
		}
		
		/*state.*/transform.orientation.x = t.getRotation().getX();
		/*state.*/transform.orientation.y = t.getRotation().getY();
		/*state.*/transform.orientation.z = t.getRotation().getZ();
		/*state.*/transform.orientation.w = t.getRotation().getW();

		physics.linear.velocity.x = body->getLinearVelocity().getX();
		physics.linear.velocity.y = body->getLinearVelocity().getY();
		physics.linear.velocity.z = body->getLinearVelocity().getZ();

		physics.rotational.velocity.x = body->getAngularVelocity().getX();
		physics.rotational.velocity.y = body->getAngularVelocity().getY();
		physics.rotational.velocity.z = body->getAngularVelocity().getZ();

		transform = uf::transform::reorient( transform );
	}
}
pod::PhysicsState& ext::bullet::create( uf::Object& object ) {
	auto& collider = object.getComponent<pod::PhysicsState>();

	collider.uid = object.getUid();
	collider.pointer = &object;
	collider.transform.reference = &object.getComponent<pod::Transform<>>();
	collider.shared = true;
	return collider;
}
void ext::bullet::attach( pod::PhysicsState& collider ) {
	if ( !collider.shape ) return;
	
	auto model = uf::transform::model( collider.transform );
	btTransform t;
	t.setFromOpenGLMatrix(&model[0]);
//	t.setIdentity();
//	t.setOrigin(btVector3(collider.transform->position.x, collider.transform->position.y, collider.transform->position.z));
//	t.setRotation(btQuaternion(collider.transform->orientation.x, collider.transform->orientation.y, collider.transform->orientation.z, collider.transform->orientation.w));

	btVector3 inertia(collider.stats.inertia.x, collider.stats.inertia.y, collider.stats.inertia.z);
	btDefaultMotionState* motion = new btDefaultMotionState(t);
	if ( collider.stats.mass != 0.0f ) collider.shape->calculateLocalInertia(collider.stats.mass, inertia);
	btRigidBody::btRigidBodyConstructionInfo info(collider.stats.mass, motion, collider.shape, inertia);
	
	collider.body = new btRigidBody(info);
	collider.body->setUserPointer((void*) collider.pointer);
	collider.body->setRestitution(collider.stats.restitution);
	collider.body->setFriction(collider.stats.friction);
	collider.body->setGravity( btVector3( collider.stats.gravity.x, collider.stats.gravity.y, collider.stats.gravity.z ) );

/*
	collider.body->setCollisionFlags(collider.body->getCollisionFlags() | collider.stats.flags);
	if ( collider.stats.mass != 0.0f ) {
		collider.body->setCollisionFlags(collider.body->getCollisionFlags() | ~btCollisionObject::CF_STATIC_OBJECT);
	} else {
		collider.body->setCollisionFlags(collider.body->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
	}
	collider.body->activate(true);
	collider.body->setActivationState(DISABLE_DEACTIVATION);
*/
	ext::bullet::dynamicsWorld->addRigidBody(collider.body);
	ext::bullet::collisionShapes.push_back(collider.shape);
}

void ext::bullet::detach( pod::PhysicsState& collider ) {
	if ( !collider.body || !collider.shape ) return;
	ext::bullet::dynamicsWorld->removeCollisionObject( collider.body );
}

pod::PhysicsState& ext::bullet::create( uf::Object& object, const uf::Mesh& mesh, bool dynamic ) {
	btTriangleIndexVertexArray* bMesh = new btTriangleIndexVertexArray();

	if ( mesh.index.count ) {
		uf::Mesh::Attribute vertexAttribute;
		for ( auto& attribute : mesh.vertex.attributes ) if ( attribute.descriptor.name == "position" ) { vertexAttribute = attribute; break; }
		UF_ASSERT( vertexAttribute.descriptor.name == "position" );

		auto& indexAttribute = mesh.index.attributes.front();
		PHY_ScalarType indexType = PHY_INTEGER;
		PHY_ScalarType vertexType = PHY_FLOAT;
		switch ( mesh.index.size ) {
			case sizeof(uint8_t): indexType = PHY_UCHAR; break;
			case sizeof(uint16_t): indexType = PHY_SHORT; break;
			case sizeof(uint32_t): indexType = PHY_INTEGER; break;
			default: UF_EXCEPTION("unsupported index type"); break;
		}

		btIndexedMesh iMesh;
		if ( mesh.indirect.count ) {
			uf::Mesh::Attribute remappedVertexAttribute = vertexAttribute;
			uf::Mesh::Attribute remappedIndexAttribute = indexAttribute;

			uf::Mesh::Input remappedVertexInput;
			uf::Mesh::Input remappedIndexInput;
			for ( auto i = 0; i < mesh.indirect.count; ++i ) {
				remappedVertexInput = mesh.remapVertexInput( i );
				remappedIndexInput = mesh.remapIndexInput( i );

			//	remappedVertexAttribute = mesh.remapVertexAttribute( vertexAttribute, i );
			//	remappedIndexAttribute = mesh.remapIndexAttribute( indexAttribute, i );

				iMesh.m_numTriangles        = remappedIndexInput.count / 3;
				iMesh.m_triangleIndexBase   = (const uint8_t*) remappedIndexAttribute.pointer + remappedIndexAttribute.stride * remappedIndexInput.first;
				iMesh.m_triangleIndexStride = remappedIndexAttribute.stride * 3;

				iMesh.m_numVertices         = remappedVertexInput.count;
				iMesh.m_vertexBase          = (const uint8_t*) remappedVertexAttribute.pointer + remappedVertexAttribute.stride * remappedVertexInput.first;
				iMesh.m_vertexStride        = remappedVertexAttribute.stride;
				iMesh.m_indexType 			= indexType;
				iMesh.m_vertexType 			= vertexType;

				bMesh->addIndexedMesh( iMesh, indexType );
			}
		} else {
			iMesh.m_numTriangles        = mesh.index.count / 3;
			iMesh.m_triangleIndexBase   = (const uint8_t*) indexAttribute.pointer + indexAttribute.stride * mesh.index.count;
			iMesh.m_triangleIndexStride = indexAttribute.stride * 3;

			iMesh.m_numVertices         = mesh.vertex.count;
			iMesh.m_vertexBase          = (const uint8_t*) vertexAttribute.pointer + vertexAttribute.stride * mesh.vertex.count;
			iMesh.m_vertexStride        = vertexAttribute.stride;
			iMesh.m_indexType 			= indexType;
			iMesh.m_vertexType 			= vertexType;
			bMesh->addIndexedMesh( iMesh, indexType );
		}
	} else UF_EXCEPTION("to-do: not require indices for meshes");

	auto& collider = ext::bullet::create( object );
	collider.shape = new btBvhTriangleMeshShape( bMesh, true, true );
	collider.stats.mass = 0;
	ext::bullet::attach( collider );

	auto& transform = object.getComponent<pod::Transform<>>();
	auto model = uf::transform::model( collider.transform );
	btTransform t = collider.body->getWorldTransform();
	t.setFromOpenGLMatrix(&model[0]);
	collider.body->setWorldTransform(t);
	collider.body->setCenterOfMassTransform(t);

/*
	btBvhTriangleMeshShape* triangleMeshShape = (btBvhTriangleMeshShape*) collider.shape;
	btTriangleInfoMap* triangleInfoMap = new btTriangleInfoMap();
	triangleInfoMap->m_edgeDistanceThreshold = 0.01f;
	triangleInfoMap->m_maxEdgeAngleThreshold = SIMD_HALF_PI*0.25;
	if ( !false ) {
		btGenerateInternalEdgeInfo( triangleMeshShape, triangleInfoMap );
		collider.body->setCollisionFlags(collider.body->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
	}
*/
	return collider;
}
pod::PhysicsState& ext::bullet::create( uf::Object& object, const pod::Vector3f& extent ) {
	auto& collider = ext::bullet::create( object );
	collider.shape = new btBoxShape(btVector3(abs(extent.x), abs(extent.y), abs(extent.z)));
	ext::bullet::attach( collider );

	auto& transform = object.getComponent<pod::Transform<>>();
	auto model = uf::transform::model( collider.transform );

//	collider.body->setContactProcessingThreshold(0.0);
//	collider.body->setAngularFactor(0.0);
//	collider.body->setCcdMotionThreshold(1e-7);
//	collider.body->setCcdSweptSphereRadius(0.25 * 0.2);
	return collider;
}
uf::stl::vector<pod::PhysicsState>& ext::bullet::create( uf::Object& object, const uf::stl::vector<pod::Instance::Bounds>& bounds ) {
	auto& colliders = object.getComponent<uf::stl::vector<pod::PhysicsState>>();
	colliders.reserve(colliders.size() + bounds.size());

	auto& transform = object.getComponent<pod::Transform<>>();
	auto flatten = uf::transform::flatten( transform );
	auto model = uf::transform::model( transform );
	for ( auto bound : bounds ) {
		pod::Vector3f center = (bound.max + bound.min) * 0.5f;
		pod::Vector3f corner = (bound.max - bound.min) * 0.5f;
		center.x = -center.x;

		center = uf::matrix::multiply<float>( model, center, 1.0f );
		corner = uf::matrix::multiply<float>( model, corner, 0.0f );

		auto& collider = colliders.emplace_back();
		collider.transform = flatten;
		collider.transform.position = center;
		collider.transform.reference = NULL;
		collider.shape = new btBoxShape(btVector3(corner.x, corner.y, corner.z));
		ext::bullet::attach( collider );

	//	collider.body->setContactProcessingThreshold(0.0);
	//	collider.body->setAngularFactor(0.0);
	//	collider.body->setCcdMotionThreshold(1e-7);
	//	collider.body->setCcdSweptSphereRadius(0.25 * 0.2);
	}
	return colliders;
}

pod::PhysicsState& ext::bullet::create( uf::Object& object, float radius, float height ) {
	auto& collider = ext::bullet::create( object );
	collider.shape = new btCapsuleShape(radius, height);
	ext::bullet::attach( collider );

	auto& transform = object.getComponent<pod::Transform<>>();
	auto model = uf::transform::model( collider.transform );

	collider.body->setContactProcessingThreshold(0.0);
	collider.body->setAngularFactor(0.0);
	collider.body->setCcdMotionThreshold(1e-7);
	collider.body->setCcdSweptSphereRadius(0.25 * 0.2);

	return collider;
}

void UF_API ext::bullet::setVelocity( pod::PhysicsState& collider, const pod::Vector3f& v ) {
	if ( !collider.body ) return;
	collider.body->activate(true);
	if ( collider.shared ) {
		auto& physics = collider.pointer->getComponent<pod::Physics>();
		physics.linear.velocity = v;
	} else {
		collider.body->setLinearVelocity( btVector3( v.x, v.y, v.z ) );
	}
}
void UF_API ext::bullet::applyImpulse( pod::PhysicsState& collider, const pod::Vector3f& v ) {
	if ( !collider.body ) return;
	collider.body->activate(true);
	collider.body->applyCentralImpulse( btVector3( v.x, v.y, v.z ) /** uf::physics::time::delta*/ );
}
void UF_API ext::bullet::applyMovement( pod::PhysicsState& collider, const pod::Vector3f& v ) {
	if ( !collider.body ) return;
	collider.body->activate(true);

	btTransform transform = collider.body->getWorldTransform();
//	collider.body->getMotionState()->getWorldTransform(transform);

	transform.setOrigin( transform.getOrigin() + btVector3( v.x, v.y, v.z ) * uf::physics::time::delta );

//	collider.body->getMotionState()->setWorldTransform(transform);
	collider.body->setWorldTransform(transform);

	collider.body->setCenterOfMassTransform(transform);
}
void UF_API ext::bullet::applyVelocity( pod::PhysicsState& collider, const pod::Vector3f& v ) {
	if ( !collider.body ) return;
	collider.body->activate(true);

	if ( collider.shared ) {
		auto& physics = collider.pointer->getComponent<pod::Physics>();
		physics.linear.velocity += v;
	} else {
		collider.body->setLinearVelocity( collider.body->getLinearVelocity() + btVector3( v.x, v.y, v.z ) );
	}
}
void UF_API ext::bullet::applyRotation( pod::PhysicsState& collider, const pod::Vector3f& axis, float delta ) {
	ext::bullet::applyRotation( collider, uf::quaternion::axisAngle( axis, delta ) );
}
void UF_API ext::bullet::applyRotation( pod::PhysicsState& collider, const pod::Quaternion<>& q ) {
	if ( !collider.body ) return;
	collider.body->activate(true);

	if ( collider.shared ) {
		auto& transform = collider.pointer->getComponent<pod::Transform<>>();
		uf::transform::rotate( transform, q );
		return;
	}

	btTransform transform = collider.body->getWorldTransform();
//	collider.body->getMotionState()->getWorldTransform(transform);

	pod::Quaternion<> orientation = uf::vector::normalize(uf::quaternion::multiply({
		transform.getRotation().getX(),
		transform.getRotation().getY(),
		transform.getRotation().getZ(),
		transform.getRotation().getW(),
	}, q));
	transform.setRotation( btQuaternion( orientation.x, orientation.y, orientation.z, orientation.w ) );

//	collider.body->getMotionState()->setWorldTransform(transform);
	collider.body->setWorldTransform(transform);
	collider.body->setCenterOfMassTransform(transform);
}
void UF_API ext::bullet::activateCollision( pod::PhysicsState& collider, bool enabled ) {
	if ( !collider.body ) return;
	collider.body->activate(enabled);
}

float UF_API ext::bullet::rayCast( const pod::Vector3f& from, const pod::Vector3f& to ) {
	btVector3 _from(from.x, from.y, from.z);
	btVector3 _to(to.x, to.y, to.z);

	btCollisionWorld::ClosestRayResultCallback res(_from, _to);
	ext::bullet::dynamicsWorld->rayTest(_from, _to, res);
	if ( !res.hasHit() ) return -1.0;
/*
	float length = uf::vector::distanceSquared( from, to );
	float depth = uf::vector::distanceSquared( from, pod::Vector3f{ res.m_hitPointWorld.getX(), res.m_hitPointWorld.getY(), res.m_hitPointWorld.getZ() } );
	return depth / length;
*/
	return uf::vector::distance( from, pod::Vector3f{ res.m_hitPointWorld.getX(), res.m_hitPointWorld.getY(), res.m_hitPointWorld.getZ() } );
}
float UF_API ext::bullet::rayCast( const pod::Vector3f& from, const pod::Vector3f& to, uf::Object*& uid ) {
	float pen = -1.0;
	uid = 0;
	btVector3 _from(from.x, from.y, from.z);
	btVector3 _to(to.x, to.y, to.z);

	btCollisionWorld::ClosestRayResultCallback res(_from, _to);
	ext::bullet::dynamicsWorld->rayTest(_from, _to, res);
	if ( !res.hasHit() ) return pen;

	pen = uf::vector::distance( from, pod::Vector3f{ res.m_hitPointWorld.getX(), res.m_hitPointWorld.getY(), res.m_hitPointWorld.getZ() } );
	
	const btCollisionObject* obj = res.m_collisionObject;	
	const btRigidBody* body = btRigidBody::upcast(obj);
	if ( !body || !body->getMotionState() ) return pen;
	uid = (uf::Object*) body->getUserPointer();
	return pen;
}
#endif