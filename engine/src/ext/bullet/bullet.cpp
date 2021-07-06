#include <uf/ext/bullet/bullet.h>
#if UF_USE_BULLET

#include <uf/utils/math/physics.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/engine/scene/scene.h>
#include <BulletCollision/CollisionShapes/btTriangleShape.h>

class BulletDebugDrawer : public btIDebugDraw {
protected:
	int m;
	uf::Mesh<pod::Vertex_3F2F3F4F> mesh;
public:
	virtual void drawLine( const btVector3& from, const btVector3& to, const btVector3& color ) {
		drawLine( from, to, color, color );
	}
	virtual void drawLine( const btVector3& from, const btVector3& to, const btVector3& fromColor, const btVector3& toColor ) {
		auto& A = mesh.vertices.emplace_back();
		A.position = { from.getX(), from.getY(), from.getZ() };
		//{0.0f, 0.0f},
		//{0.0f, 0.0f, 0.0f},
		A.color = { fromColor.getX(), fromColor.getY(), fromColor.getZ(), 1.0f };
		auto& B = mesh.vertices.emplace_back();
		B.position = { to.getX(), to.getY(), to.getZ() };
		//{0.0f, 0.0f},
		//{0.0f, 0.0f, 0.0f},
		B.color = { toColor.getX(), toColor.getY(), toColor.getZ(), 1.0f };
	}
	virtual void drawContactPoint(const btVector3&, const btVector3&, btScalar, int, const btVector3&) {

	}
	virtual void reportErrorWarning(const char* str ) {
		uf::iostream << "[Bullet] " << str << "\n";
	}
	virtual void draw3dText(const btVector3& , const char* str ) {

	}
	virtual void setDebugMode(int p) {
		m = p;
	}
	virtual void clearLines() {
		mesh.destroy();
	}
	virtual void flushLines() {
		auto& scene = uf::scene::getCurrentScene();
		ext::bullet::debugDraw( scene );
	}
	int getDebugMode(void) const { return m; }

	uf::Mesh<pod::Vertex_3F2F3F4F>& getMesh() { return mesh; }
	const uf::Mesh<pod::Vertex_3F2F3F4F>& getMesh() const { return mesh; }
};

bool ext::bullet::debugDrawEnabled = false;
float ext::bullet::debugDrawRate = 1.0f;
size_t ext::bullet::iterations = 1;
size_t ext::bullet::substeps = 12;
float ext::bullet::timescale = 1.0f;

size_t ext::bullet::defaultMaxCollisionAlgorithmPoolSize = 65535;
size_t ext::bullet::defaultMaxPersistentManifoldPoolSize = 65535;

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

	pod::Vector3f Before = {
		ManifoldPoint.m_normalWorldOnB.getX(),
		ManifoldPoint.m_normalWorldOnB.getY(),
		ManifoldPoint.m_normalWorldOnB.getZ(),
	};
	btAdjustInternalEdgeContacts(ManifoldPoint, Object1, Object0, PartID1, Index1);
	btAdjustInternalEdgeContacts(ManifoldPoint, Object0, Object1, PartID0, Index0);

	pod::Vector3f After = {
		ManifoldPoint.m_normalWorldOnB.getX(),
		ManifoldPoint.m_normalWorldOnB.getY(),
		ManifoldPoint.m_normalWorldOnB.getZ(),
	};

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

#if !UF_ENV_DREAMCAST
	gContactAddedCallback = contactCallback;
#endif
}
void ext::bullet::tick( float delta ) { if ( delta == 0.0f ) delta = uf::physics::time::delta;
	ext::bullet::syncToBullet();
	delta = delta * ext::bullet::timescale / ext::bullet::iterations;
	for ( size_t i = 0; i < ext::bullet::iterations; ++i ) {
		ext::bullet::dynamicsWorld->stepSimulation(delta, ext::bullet::substeps, delta / ext::bullet::substeps);
	}
	ext::bullet::syncFromBullet();
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
void ext::bullet::syncToBullet() {
	auto& scene = uf::scene::getCurrentScene();
	// update bullet transforms
	for ( int i = ext::bullet::dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; --i) {
		btCollisionObject* obj = ext::bullet::dynamicsWorld->getCollisionObjectArray()[i];
		
		btRigidBody* body = btRigidBody::upcast(obj);
		if ( !body || !body->getMotionState() ) return;
	
		uf::Object* entity = (uf::Object*) body->getUserPointer();
		if ( !entity || !entity->isValid() || !entity->hasComponent<pod::Bullet>() ) continue;

		auto& collider = entity->getComponent<pod::Bullet>();
	//	auto& transform = entity->getComponent<pod::Transform<>>();
				
		if ( !collider.shared ) continue;
		{
			auto& physics = entity->getComponent<pod::Physics>();
			body->setLinearVelocity( btVector3( physics.linear.velocity.x, physics.linear.velocity.y, physics.linear.velocity.z ) );
		}
		{
		//	auto model = uf::transform::model( transform );
			auto model = uf::transform::model( collider.transform );
			
			btTransform t;
			t = body->getWorldTransform();
			t.setFromOpenGLMatrix(&model[0]);

		//	t.setOrigin( btVector3( transform.position.x, transform.position.y, transform.position.z ) );
		//	t.setRotation( btQuaternion( transform.orientation.x, transform.orientation.y, transform.orientation.z, transform.orientation.w ) );

			body->setWorldTransform(t);
			body->setCenterOfMassTransform(t);

		//	auto T = uf::transform::flatten( transform );
		//	UF_MSG_DEBUG( entity->getName() << ": " << entity->getUid() << " " << uf::vector::toString( T.position ) );
		}
	}
}
void ext::bullet::syncFromBullet() {
	auto& scene = uf::scene::getCurrentScene();
	for ( int i = ext::bullet::dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; --i) {
		btCollisionObject* obj = ext::bullet::dynamicsWorld->getCollisionObjectArray()[i];
		btRigidBody* body = btRigidBody::upcast(obj);
		if ( !body || !body->getMotionState() ) return;
		btTransform t;
		body->getMotionState()->getWorldTransform(t);

		uf::Object* entity = (uf::Object*) body->getUserPointer();
		if ( !entity || !entity->isValid() || !entity->hasComponent<pod::Bullet>() ) continue;
		size_t uid = entity->getUid();

		auto& collider = entity->getComponent<pod::Bullet>();

		auto& transform = entity->getComponent<pod::Transform<>>();
		transform.position.x = t.getOrigin().getX();
		transform.position.y = t.getOrigin().getY();
		transform.position.z = t.getOrigin().getZ();

		transform.orientation.x = t.getRotation().getX();
		transform.orientation.y = t.getRotation().getY();
		transform.orientation.z = t.getRotation().getZ();
		transform.orientation.w = t.getRotation().getW();

		// unoffset by our transform
	//	transform.position -= collider.transform.position;
	//	transform.orientation = transform.orientation * uf::quaternion::inverse( collider.transform.orientation );

		{
			auto& physics = entity->getComponent<pod::Physics>();
			physics.linear.velocity.x = body->getLinearVelocity().getX();
			physics.linear.velocity.y = body->getLinearVelocity().getY();
			physics.linear.velocity.z = body->getLinearVelocity().getZ();

			physics.rotational.velocity.x = body->getAngularVelocity().getX();
			physics.rotational.velocity.y = body->getAngularVelocity().getY();
			physics.rotational.velocity.z = body->getAngularVelocity().getZ();
		}


		transform = uf::transform::reorient( transform );
	}
}
pod::Bullet& ext::bullet::create( uf::Object& object ) {
	auto& collider = object.getComponent<pod::Bullet>();

	collider.uid = object.getUid();
	collider.pointer = &object;
//	collider.transform = &object.getComponent<pod::Transform<>>();
	collider.transform.reference = &object.getComponent<pod::Transform<>>();
	collider.shared = false;
	return collider;
}
void ext::bullet::attach( pod::Bullet& collider ) {
	if ( !collider.shape ) return;
	
	auto model = uf::transform::model( collider.transform );
	btTransform t;
	t.setFromOpenGLMatrix(&model[0]);
//	t.setIdentity();
//	t.setOrigin(btVector3(collider.transform->position.x, collider.transform->position.y, collider.transform->position.z));
//	t.setRotation(btQuaternion(collider.transform->orientation.x, collider.transform->orientation.y, collider.transform->orientation.z, collider.transform->orientation.w));

	btVector3 inertia(collider.inertia.x, collider.inertia.y, collider.inertia.z);
	btDefaultMotionState* motion = new btDefaultMotionState(t);
	if ( collider.mass != 0.0f ) collider.shape->calculateLocalInertia(collider.mass, inertia);
	btRigidBody::btRigidBodyConstructionInfo info(collider.mass, motion, collider.shape, inertia);
	
	collider.body = new btRigidBody(info);
//	collider.body->setUserPointer((void*) collider.uid);
	collider.body->setUserPointer((void*) collider.pointer);

	if ( collider.mass > 0 ) {
		collider.body->activate(true);
		collider.body->setActivationState(DISABLE_DEACTIVATION);
	}

	ext::bullet::dynamicsWorld->addRigidBody(collider.body);
	ext::bullet::collisionShapes.push_back(collider.shape);
}

void ext::bullet::detach( pod::Bullet& collider ) {
	if ( !collider.body || !collider.shape ) return;
	ext::bullet::dynamicsWorld->removeCollisionObject( collider.body );
}

pod::Bullet& ext::bullet::create( uf::Object& object, const pod::Mesh& mesh, bool dynamic ) {
	auto& transform = object.getComponent<pod::Transform<>>();

	auto& collider = ext::bullet::create( object );
	auto model = uf::transform::model( collider.transform );
	
	size_t indices = mesh.attributes.index.length;
	size_t indexStride = mesh.attributes.index.size;
	uint8_t* indexPointer = (uint8_t*) mesh.attributes.index.pointer;

	size_t vertices = mesh.attributes.vertex.length;
	size_t vertexStride = mesh.attributes.vertex.size;
	uint8_t* vertexPointer = (uint8_t*) mesh.attributes.vertex.pointer;

	uf::renderer::VertexDescriptor vertexAttributePosition;
	for ( auto& attribute : mesh.attributes.descriptor ) {
		if ( attribute.name == "position" ) vertexAttributePosition = attribute;
	}
	if ( vertexAttributePosition.name == "" ) return collider;
	
	btTriangleMesh* bMesh = new btTriangleMesh( true, false );
	bMesh->preallocateVertices( vertices );
	for ( size_t currentIndex = 0; currentIndex < vertices; ++currentIndex ) {
		uint8_t* vertexSrc = vertexPointer + (currentIndex * vertexStride);
		const pod::Vector3f& position = *((pod::Vector3f*) (vertexSrc + vertexAttributePosition.offset));
		bMesh->findOrAddVertex( btVector3( position.x, position.y, position.z ), false );
	}
	if ( mesh.attributes.index.pointer ) {
		bMesh->preallocateIndices( indices );
		for ( size_t currentIndex = 0; currentIndex < indices; ++currentIndex ) {
			uint32_t index = 0;
			uint8_t* indexSrc = indexPointer + (currentIndex * indexStride);
			switch ( indexStride ) {
				case sizeof( uint8_t): index = *(( uint8_t*) indexSrc); break;
				case sizeof(uint16_t): index = *((uint16_t*) indexSrc); break;
				case sizeof(uint32_t): index = *((uint32_t*) indexSrc); break;
			}
			bMesh->addIndex( index );
		}
		bMesh->getIndexedMeshArray()[0].m_numTriangles = indices / 3;
	}

	collider.shape = new btBvhTriangleMeshShape(bMesh, true);
	ext::bullet::attach( collider );

	btTransform t = collider.body->getWorldTransform();
	t.setFromOpenGLMatrix(&model[0]);
	collider.body->setWorldTransform(t);
	collider.body->setCenterOfMassTransform(t);

	btBvhTriangleMeshShape* triangleMeshShape = (btBvhTriangleMeshShape*) collider.shape;
	btTriangleInfoMap* triangleInfoMap = new btTriangleInfoMap();
	triangleInfoMap->m_edgeDistanceThreshold = 0.01f;
	triangleInfoMap->m_maxEdgeAngleThreshold = SIMD_HALF_PI*0.25;
	if ( !false ) btGenerateInternalEdgeInfo( triangleMeshShape, triangleInfoMap );
	collider.body->setCollisionFlags(collider.body->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);

	return collider;
}
pod::Bullet& ext::bullet::create( uf::Object& object, const pod::Vector3f& corner, float mass ) {
	auto& collider = ext::bullet::create( object );
	collider.mass = mass;
	collider.shape = new btBoxShape(btVector3(corner.x, corner.y, corner.z));
	ext::bullet::attach( collider );

	auto& transform = object.getComponent<pod::Transform<>>();
	auto model = uf::transform::model( collider.transform );

/*
	btTransform t = collider.body->getWorldTransform();
	t.setFromOpenGLMatrix(&model[0]);
	collider.body->setWorldTransform(t);
	collider.body->setCenterOfMassTransform(t);
*/

	collider.body->setContactProcessingThreshold(0.0);
	collider.body->setAngularFactor(0.0);
	collider.body->setCcdMotionThreshold(1e-7);
	collider.body->setCcdSweptSphereRadius(0.25 * 0.2);
	return collider;
}

pod::Bullet& ext::bullet::create( uf::Object& object, float radius, float height, float mass ) {
	auto& collider = ext::bullet::create( object );
	collider.mass = mass;
	collider.shape = new btCapsuleShape(radius, height);
	ext::bullet::attach( collider );

	auto& transform = object.getComponent<pod::Transform<>>();
	auto model = uf::transform::model( collider.transform );
	
/*
	btTransform t = collider.body->getWorldTransform();
	t.setFromOpenGLMatrix(&model[0]);
	collider.body->setWorldTransform(t);
	collider.body->setCenterOfMassTransform(t);
*/

	collider.body->setContactProcessingThreshold(0.0);
	collider.body->setAngularFactor(0.0);
	collider.body->setCcdMotionThreshold(1e-7);
	collider.body->setCcdSweptSphereRadius(0.25 * 0.2);

//	UF_MSG_DEBUG( collider.body->getLinearDamping() << " " << collider.body->getAngularDamping() );
//	collider.body->setDamping( 0.8, 0.8 );

	return collider;
}

void UF_API ext::bullet::move( pod::Bullet& collider, const pod::Vector3f& v, bool override ) {
	if ( !collider.body ) return;
/*	
	float x = collider.body->getLinearVelocity().getX();
	float y = collider.body->getLinearVelocity().getY();
	float z = collider.body->getLinearVelocity().getZ();
	if ( fabs(x) < 0.001f ) x = v.x;
	if ( fabs(y) < 0.001f ) y = v.y;
	if ( fabs(z) < 0.001f ) z = v.z;
	if ( override ) {
		x = v.x;
		y = v.y;
		z = v.z;
	}
	collider.body->setLinearVelocity( btVector3( x, y, z ) );
*/
	collider.body->setLinearVelocity( btVector3( v.x, v.y, v.z ) );

//	btTransform transform = collider.body->getWorldTransform();
//	transform.setOrigin( transform.getOrigin() + btVector3( v.x, v.y, v.z ) * uf::physics::time::delta );
//	collider.body->setWorldTransform(transform);
//	collider.body->setCenterOfMassTransform(transform);
}
void UF_API ext::bullet::applyImpulse( pod::Bullet& collider, const pod::Vector3f& v ) {
	if ( !collider.body ) return;
	collider.body->applyCentralImpulse( btVector3( v.x, v.y, v.z ) /** uf::physics::time::delta*/ );
}
void UF_API ext::bullet::applyMovement( pod::Bullet& collider, const pod::Vector3f& v ) {
	if ( !collider.body ) return;
	btTransform transform;
	collider.body->getMotionState()->getWorldTransform(transform);

	transform.setOrigin( transform.getOrigin() + btVector3( v.x, v.y, v.z ) * uf::physics::time::delta );

	collider.body->getMotionState()->setWorldTransform(transform);
	collider.body->setCenterOfMassTransform(transform);
}
void UF_API ext::bullet::applyVelocity( pod::Bullet& collider, const pod::Vector3f& v ) {
	if ( !collider.body ) return;
	collider.body->setLinearVelocity( collider.body->getLinearVelocity() + btVector3( v.x, v.y, v.z ) );
}
void UF_API ext::bullet::applyRotation( pod::Bullet& collider, const pod::Vector3f& axis, float delta ) {
	if ( !collider.body ) return;
	ext::bullet::applyRotation( collider, uf::quaternion::axisAngle( axis, delta ) );
}
void UF_API ext::bullet::applyRotation( pod::Bullet& collider, const pod::Quaternion<>& q ) {
	if ( !collider.body ) return;
	btTransform transform;
	collider.body->getMotionState()->getWorldTransform(transform);

	pod::Quaternion<> orientation = uf::vector::normalize(uf::quaternion::multiply({
		transform.getRotation().getX(),
		transform.getRotation().getY(),
		transform.getRotation().getZ(),
		transform.getRotation().getW(),
	}, q));
	transform.setRotation( btQuaternion( orientation.x, orientation.y, orientation.z, orientation.w ) );

	collider.body->getMotionState()->setWorldTransform(transform);
	collider.body->setCenterOfMassTransform(transform);
}
void UF_API ext::bullet::activateCollision( pod::Bullet& collider, bool enabled ) {
	if ( !collider.body ) return;
	collider.body->activate(enabled);
}

void UF_API ext::bullet::debugDraw( uf::Object& object ) {
	auto& mesh = ext::bullet::debugDrawer.getMesh();
	if ( mesh.vertices.empty() ) return;
	bool create = !object.hasComponent<uf::Graphic>();
	auto& graphic = object.getComponent<uf::Graphic>();
	graphic.process = false;
	if ( create ) {
		graphic.process = true;

		graphic.device = &uf::renderer::device;
		graphic.material.device = &uf::renderer::device;
		graphic.descriptor.cullMode = uf::renderer::enums::CullMode::NONE;

		graphic.material.attachShader(uf::io::root + "/shaders/base/colored.vert.spv", uf::renderer::enums::Shader::VERTEX);
		graphic.material.attachShader(uf::io::root + "/shaders/base/base.frag.spv", uf::renderer::enums::Shader::FRAGMENT);

		graphic.initialize();
		graphic.initializeMesh( mesh, 0 );
		graphic.descriptor.topology = uf::renderer::enums::PrimitiveTopology::LINE_LIST;
		graphic.descriptor.fill = uf::renderer::enums::PolygonMode::LINE;
		graphic.descriptor.lineWidth = 8.0f;
	} else {
		graphic.process = true;

		graphic.initializeMesh( mesh, 0 );
		graphic.getPipeline().update( graphic );
	}
}
#endif