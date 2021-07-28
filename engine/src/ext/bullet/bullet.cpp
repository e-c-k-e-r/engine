#include <uf/ext/bullet/bullet.h>
#if UF_USE_BULLET

#include <uf/utils/math/physics.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/engine/scene/scene.h>
#include <uf/engine/graph/graph.h>
#include <BulletCollision/CollisionShapes/btTriangleShape.h>

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

size_t ext::bullet::iterations = 1;
size_t ext::bullet::substeps = 12;
float ext::bullet::timescale = 1.0f;

size_t ext::bullet::defaultMaxCollisionAlgorithmPoolSize = 65535;
size_t ext::bullet::defaultMaxPersistentManifoldPoolSize = 65535;

bool ext::bullet::debugDrawEnabled = false;
float ext::bullet::debugDrawRate = 1.0f;
uf::stl::string ext::bullet::debugDrawLayer = "";

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

	auto& mesh = ext::bullet::debugDrawer.getMesh();
	mesh.bind<VertexLine>();

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
				
		if ( !collider.shared ) continue;
		{
			auto& physics = entity->getComponent<pod::Physics>();
			body->setLinearVelocity( btVector3( physics.linear.velocity.x, physics.linear.velocity.y, physics.linear.velocity.z ) );
		}
		{
			auto model = uf::transform::model( collider.transform );
			
			btTransform t;
			t = body->getWorldTransform();
			t.setFromOpenGLMatrix(&model[0]);

		//	t.setOrigin( btVector3( transform.position.x, transform.position.y, transform.position.z ) );
		//	t.setRotation( btQuaternion( transform.orientation.x, transform.orientation.y, transform.orientation.z, transform.orientation.w ) );

			body->setWorldTransform(t);
			body->setCenterOfMassTransform(t);
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

pod::Bullet& ext::bullet::create( uf::Object& object, const uf::Mesh& mesh, bool dynamic ) {
	btTriangleIndexVertexArray* bMesh = new btTriangleIndexVertexArray();

	if ( mesh.index.count ) {
		uf::Mesh::Attribute vertexAttribute;
		for ( auto& attribute : mesh.vertex.attributes ) if ( attribute.descriptor.name == "position" ) { vertexAttribute = attribute; break; }
		UF_ASSERT( vertexAttribute.descriptor.name == "position" );

		auto& indexAttribute = mesh.index.attributes.front();
		PHY_ScalarType indexType = PHY_INTEGER;
		PHY_ScalarType vertexType = PHY_FLOAT;
		switch ( mesh.index.stride ) {
			case sizeof(uint8_t): indexType = PHY_UCHAR; break;
			case sizeof(uint16_t): indexType = PHY_SHORT; break;
			case sizeof(uint32_t): indexType = PHY_INTEGER; break;
			default: UF_EXCEPTION("unsupported index type"); break;
		}

		btIndexedMesh iMesh;
		if ( mesh.indirect.count ) {
			uf::Mesh::Attribute remappedVertexAttribute;
			uf::Mesh::Attribute remappedIndexAttribute;
			for ( auto i = 0; i < mesh.indirect.count; ++i ) {
				remappedVertexAttribute = mesh.remapVertexAttribute( vertexAttribute, i );
				remappedIndexAttribute = mesh.remapIndexAttribute( indexAttribute, i );

				iMesh.m_numTriangles        = remappedIndexAttribute.length / 3;
				iMesh.m_triangleIndexBase   = (const uint8_t*) remappedIndexAttribute.pointer;
				iMesh.m_triangleIndexStride = remappedIndexAttribute.stride * 3;

				iMesh.m_numVertices         = remappedVertexAttribute.length;
				iMesh.m_vertexBase          = (const uint8_t*) remappedVertexAttribute.pointer;
				iMesh.m_vertexStride        = remappedVertexAttribute.stride;
				iMesh.m_indexType 			= indexType;
				iMesh.m_vertexType 			= vertexType;

				bMesh->addIndexedMesh( iMesh, indexType );
			}
		} else {
			iMesh.m_numTriangles        = indexAttribute.length / 3;
			iMesh.m_triangleIndexBase   = (const uint8_t*) indexAttribute.pointer;
			iMesh.m_triangleIndexStride = indexAttribute.stride * 3;

			iMesh.m_numVertices         = vertexAttribute.length;
			iMesh.m_vertexBase          = (const uint8_t*) vertexAttribute.pointer;
			iMesh.m_vertexStride        = vertexAttribute.stride;
			iMesh.m_indexType 			= indexType;
			iMesh.m_vertexType 			= vertexType;
			bMesh->addIndexedMesh( iMesh, indexType );
		}
	} else UF_EXCEPTION("to-do: not require indices for meshes");

	auto& collider = ext::bullet::create( object );
	collider.shape = new btBvhTriangleMeshShape( bMesh, true, true );
	ext::bullet::attach( collider );

	auto& transform = object.getComponent<pod::Transform<>>();
	auto model = uf::transform::model( collider.transform );
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
/*
pod::Bullet& ext::bullet::create( uf::Object& object, const void* verticesPointer, size_t verticesCount, size_t verticesStride, const void* indicesPointer, size_t indicesCount, size_t indicesStride, bool dynamic ) {
	btTriangleIndexVertexArray* bMesh = new btTriangleIndexVertexArray();

	if ( indicesCount ) {
		PHY_ScalarType indexType = PHY_INTEGER;
		PHY_ScalarType vertexType = PHY_FLOAT;
		switch ( indicesStride ) {
			case 1: indexType = PHY_UCHAR; break;
			case 2: indexType = PHY_SHORT; break;
			case 4: indexType = PHY_INTEGER; break;
		}

		const uint8_t* indicesBuffer = (const uint8_t*) indicesPointer;
		const uint8_t* verticesBuffer = (const uint8_t*) verticesPointer;

		btIndexedMesh iMesh;
		iMesh.m_numTriangles        = indicesCount / 3;
		iMesh.m_triangleIndexBase   = indicesBuffer;
		iMesh.m_triangleIndexStride = 3 * indicesStride;
		iMesh.m_numVertices         = verticesCount;
		iMesh.m_vertexBase          = verticesBuffer;
		iMesh.m_vertexStride        = verticesStride;
		iMesh.m_indexType 			= indexType;
		iMesh.m_vertexType 			= vertexType;
		bMesh->addIndexedMesh( iMesh, indexType );
	} else UF_EXCEPTION("to-do: not require indices for meshes");


	auto& collider = ext::bullet::create( object );
	collider.shape = new btBvhTriangleMeshShape( bMesh, true, true );
	ext::bullet::attach( collider );

	auto& transform = object.getComponent<pod::Transform<>>();
	auto model = uf::transform::model( collider.transform );
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
*/
pod::Bullet& ext::bullet::create( uf::Object& object, const pod::Vector3f& corner, float mass ) {
	auto& collider = ext::bullet::create( object );
	collider.mass = mass;
	collider.shape = new btBoxShape(btVector3(corner.x, corner.y, corner.z));
	ext::bullet::attach( collider );

	auto& transform = object.getComponent<pod::Transform<>>();
	auto model = uf::transform::model( collider.transform );

	collider.body->setContactProcessingThreshold(0.0);
	collider.body->setAngularFactor(0.0);
	collider.body->setCcdMotionThreshold(1e-7);
	collider.body->setCcdSweptSphereRadius(0.25 * 0.2);
	return collider;
}
uf::stl::vector<pod::Bullet>& ext::bullet::create( uf::Object& object, const uf::stl::vector<pod::Instance::Bounds>& bounds, float mass ) {
	auto& colliders = object.getComponent<uf::stl::vector<pod::Bullet>>();
	colliders.reserve(colliders.size() + bounds.size());

	auto& transform = object.getComponent<pod::Transform<>>();
	auto flatten = uf::transform::flatten( transform );
	auto model = uf::transform::model( transform );
	for ( auto bound : bounds ) {
	/*
		pod::Vector3f corners[8] = {
			uf::matrix::multiply<float>( model, pod::Vector3f{ -bound.min.x, bound.min.y, bound.min.z }, 1.0f ),
			uf::matrix::multiply<float>( model, pod::Vector3f{ -bound.max.x, bound.min.y, bound.min.z }, 1.0f ),
			uf::matrix::multiply<float>( model, pod::Vector3f{ -bound.max.x, bound.max.y, bound.min.z }, 1.0f ),
			uf::matrix::multiply<float>( model, pod::Vector3f{ -bound.min.x, bound.max.y, bound.min.z }, 1.0f ),

			uf::matrix::multiply<float>( model, pod::Vector3f{ -bound.min.x, bound.min.y, bound.max.z }, 1.0f ),
			uf::matrix::multiply<float>( model, pod::Vector3f{ -bound.max.x, bound.min.y, bound.max.z }, 1.0f ),
			uf::matrix::multiply<float>( model, pod::Vector3f{ -bound.max.x, bound.max.y, bound.max.z }, 1.0f ),
			uf::matrix::multiply<float>( model, pod::Vector3f{ -bound.min.x, bound.max.y, bound.max.z }, 1.0f ),
		};
		bound = {};

		FOR_ARRAY( corners ) {
			bound.min = uf::vector::min( bound.min, corners[i] );
			bound.max = uf::vector::max( bound.max, corners[i] );
		}
	*/
		pod::Vector3f center = (bound.max + bound.min) * 0.5f;
		pod::Vector3f corner = (bound.max - bound.min) * 0.5f;
		center.x = -center.x;

		center = uf::matrix::multiply<float>( model, center, 1.0f );
		corner = uf::matrix::multiply<float>( model, corner, 0.0f );

		auto& collider = colliders.emplace_back();
		collider.transform = flatten;
		collider.transform.position = center;
		collider.transform.reference = NULL;
		collider.mass = mass;
		collider.shape = new btBoxShape(btVector3(corner.x, corner.y, corner.z));
		ext::bullet::attach( collider );

		collider.body->setContactProcessingThreshold(0.0);
		collider.body->setAngularFactor(0.0);
		collider.body->setCcdMotionThreshold(1e-7);
		collider.body->setCcdSweptSphereRadius(0.25 * 0.2);
	}
	return colliders;
}

pod::Bullet& ext::bullet::create( uf::Object& object, float radius, float height, float mass ) {
	auto& collider = ext::bullet::create( object );
	collider.mass = mass;
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

void UF_API ext::bullet::setVelocity( pod::Bullet& collider, const pod::Vector3f& v ) {
	if ( !collider.body ) return;
	collider.body->setLinearVelocity( btVector3( v.x, v.y, v.z ) );
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
		graphic.descriptor.lineWidth = 8.0f;
	} else {
		if ( graphic.updateMesh( mesh ) ) {
			graphic.getPipeline().update( graphic );
		}
	}
	graphic.process = true;
}
#endif