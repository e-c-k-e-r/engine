#include <uf/ext/bullet/bullet.h>

#include <uf/utils/math/physics.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/engine/scene/scene.h>
#include <BulletCollision/CollisionShapes/btTriangleShape.h>

class BulletDebugDrawer : public btIDebugDraw {
protected:
	int m;
	uf::BaseMesh<pod::Vertex_3F2F3F4F> mesh;
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
	//	std::cout << "EMPTIED: " << mesh.vertices.size() << std::endl;
		mesh.destroy();
	//	mesh.indices.clear();
	//	mesh.vertices.clear();
	}
	virtual void flushLines() {
	//	std::cout << "FLUSHED: " << mesh.vertices.size() << std::endl;
		auto& scene = uf::scene::getCurrentScene();
		ext::bullet::debugDraw( scene );
	}
	int getDebugMode(void) const { return m; }

	uf::BaseMesh<pod::Vertex_3F2F3F4F>& getMesh() { return mesh; }
	const uf::BaseMesh<pod::Vertex_3F2F3F4F>& getMesh() const { return mesh; }
};

bool ext::bullet::debugDrawEnabled = false;
float ext::bullet::debugDrawRate = 1.0f;
size_t ext::bullet::iterations = 1;
size_t ext::bullet::substeps = 12;
float ext::bullet::timescale = 1.0f;

namespace ext {
	namespace bullet {
		btDefaultCollisionConfiguration* collisionConfiguration = NULL;
		btCollisionDispatcher* dispatcher = NULL;
		btBroadphaseInterface* overlappingPairCache = NULL;
		btSequentialImpulseConstraintSolver* solver = NULL;
		btDiscreteDynamicsWorld* dynamicsWorld = NULL;
		btAlignedObjectArray<btCollisionShape*> collisionShapes;

		std::unordered_map<btRigidBody*, size_t> rigidBodyMap;

		BulletDebugDrawer debugDrawer;
	}
}


/*
static bool fixMeshNormal( btManifoldPoint& cp, const btCollisionObjectWrapper* colObj0Wrap, int partId0, int index0 ) {
	const btTriangleShape* tri_shape = static_cast<const btTriangleShape*>(colObj0Wrap->getCollisionShape());
	btVector3 tri_normal;
	tri_shape->calcNormal(cp.m_normalWorldOnB);
	cp.m_normalWorldOnB = colObj0Wrap->getWorldTransform().getBasis() * cp.m_normalWorldOnB;

	// Reproject collision point along normal.
	cp.m_positionWorldOnB = cp.m_positionWorldOnA - cp.m_normalWorldOnB * cp.m_distance1;
	cp.m_localPointB = colObj0Wrap->getWorldTransform().invXform(cp.m_positionWorldOnB);
	return true;
}

btScalar SegmentSqrDistance(const btVector3& from, const btVector3& to,const btVector3 &p) {
	btVector3 diff = p - from;
	btVector3 v = to - from;
	btScalar t = v.dot(diff);
	if (t > 0) {
		btScalar dotVV = v.dot(v);
		if (t < dotVV)  diff -= (t / dotVV) * v;
		else diff -= v;
	}
	return diff.dot(diff);	
}

bool ProcessHeightfieldCollision(btManifoldPoint& cp, btVector3& cppos, btHeightfieldTerrainShape* heightfield_shape, btTriangleShape* triangle_shape, int part_id, int index) {
	// This can be classified as an edge collision if the contact normal differs from the triangle normal
	btVector3 triangle_normal;
	triangle_shape->calcNormal(triangle_normal);
	if (cp.m_normalWorldOnB.dot(triangle_normal) < 1 - SIMD_EPSILON) {
		// Need to find the edge that matches this contact normal
		int ev0 = -1, ev1 = -1;
		btVector3* vertices = &triangle_shape->getVertexPtr(0);
		for (int v0 = 2, v1 = 0; v1 < 3; v0 = v1, v1++) {
			float d = SegmentSqrDistance(vertices[v0], vertices[v1], cppos);
			if (d < SIMD_EPSILON) {
				ev0 = v0;
				ev1 = v1;
			}
		}

		// Leave contact point alone if the edge can't be found
		if (ev0 == -1) return true;

		// Get the triangle that shares this edge. If this is a boundary edge, the contact normal is
		// perfectly valid
		btVector3 neighbour_vertices[3];
		if (!heightfield_shape->getNeighbourTriangle(part_id, index, ev0, neighbour_vertices)) return true;

		// If the two faces have similar normals, use the face normal and ignore the edge normal
		btVector3 other_normal = (neighbour_vertices[1]-neighbour_vertices[0]).cross(neighbour_vertices[2]-neighbour_vertices[0]);
		other_normal.normalize();
		if (triangle_normal.dot(other_normal) > 1 - SIMD_EPSILON) cp.m_normalWorldOnB = triangle_normal;
	}

	return true;
}
*/
static bool contactCallback(btManifoldPoint &ManifoldPoint, const btCollisionObjectWrapper *Object0, int PartID0, int Index0, const btCollisionObjectWrapper *Object1, int PartID1, int Index1) {
/*
	if ( colObj0Wrap->getCollisionShape()->getShapeType() == TRIANGLE_SHAPE_PROXYTYPE ) {
		fixMeshNormal( cp, colObj0Wrap, partId0, index0 );
	}
	if ( colObj1Wrap->getCollisionShape()->getShapeType() == TRIANGLE_SHAPE_PROXYTYPE ) {
		fixMeshNormal( cp, colObj1Wrap, partId1, index1 );
	}
	btAdjustInternalEdgeContacts(cp, colObj1Wrap, colObj0Wrap, partId1,index1);
	//btAdjustInternalEdgeContacts(cp,colObj1Wrap,colObj0Wrap, partId1,index1, BT_TRIANGLE_CONVEX_BACKFACE_MODE);
	//btAdjustInternalEdgeContacts(cp,colObj1Wrap,colObj0Wrap, partId1,index1, BT_TRIANGLE_CONVEX_DOUBLE_SIDED+BT_TRIANGLE_CONCAVE_DOUBLE_SIDED);
*/

	if( Object1->getCollisionShape()->getShapeType() != TRIANGLE_SHAPE_PROXYTYPE ) return false;

	pod::Vector3f Before = {
		ManifoldPoint.m_normalWorldOnB.getX(),
		ManifoldPoint.m_normalWorldOnB.getY(),
		ManifoldPoint.m_normalWorldOnB.getZ(),
	};
	btAdjustInternalEdgeContacts(ManifoldPoint, Object1, Object0, PartID1, Index1);
	btAdjustInternalEdgeContacts(ManifoldPoint, Object0, Object1, PartID0, Index0);
/*	
	if ( Object0->getCollisionShape()->getShapeType() == TRIANGLE_SHAPE_PROXYTYPE ) {
		fixMeshNormal( ManifoldPoint, Object0, PartID0, Index0 );
	}
	if ( Object1->getCollisionShape()->getShapeType() == TRIANGLE_SHAPE_PROXYTYPE ) {
		fixMeshNormal( ManifoldPoint, Object1, PartID1, Index1 );
	}
*/
	pod::Vector3f After = {
		ManifoldPoint.m_normalWorldOnB.getX(),
		ManifoldPoint.m_normalWorldOnB.getY(),
		ManifoldPoint.m_normalWorldOnB.getZ(),
	};

//	if ( Before != After ) std::cout << "Before: " << uf::string::toString(Before) << "\tafter: " << uf::string::toString(After) << std::endl;

	return false;
}

/*
void FixMeshNormal(btManifoldPoint& cp, const btCollisionObject* colObj0, int , int ) {
	const btTriangleMeshShape* tri_shape = static_cast<const btTriangleMeshShape*>(colObj0->getCollisionShape());
	btVector3 tri_normal;
	tri_shape->calcNormal(cp.m_normalWorldOnB);
	cp.m_normalWorldOnB = colObj0->getWorldTransform().getBasis() * cp.m_normalWorldOnB;

	// Reproject collision point along normal.
	cp.m_positionWorldOnB = cp.m_positionWorldOnA - cp.m_normalWorldOnB * cp.m_distance1;
	cp.m_localPointB = colObj0->getWorldTransform().invXform(cp.m_positionWorldOnB);
}
*/

void ext::bullet::initialize() {
	ext::bullet::collisionConfiguration = new btDefaultCollisionConfiguration();
	ext::bullet::dispatcher = new btCollisionDispatcher(ext::bullet::collisionConfiguration);
	ext::bullet::overlappingPairCache = new btDbvtBroadphase(); // new btAxis3Sweep();
	ext::bullet::solver = new btSequentialImpulseConstraintSolver;
	ext::bullet::dynamicsWorld = new btDiscreteDynamicsWorld(ext::bullet::dispatcher, ext::bullet::overlappingPairCache, ext::bullet::solver, ext::bullet::collisionConfiguration);
	ext::bullet::dynamicsWorld->setGravity(btVector3(0, -9.81, 0));

	gContactAddedCallback = contactCallback;

	ext::bullet::debugDrawer.setDebugMode(btIDebugDraw::DBG_DrawWireframe);
    ext::bullet::dynamicsWorld->setDebugDrawer(&ext::bullet::debugDrawer);
#if 0
	if ( false ) {
		auto& scene = uf::scene::getCurrentScene();
		btCollisionShape* shape = new btBoxShape(btVector3(100, 0, 100));
		btVector3 inertia(0.0f, 0.0f, 0.0f);
		btScalar mass(0.0f);
	
		btTransform t;
		t.setIdentity();
		t.setOrigin(btVector3(0, -5, 0));

		btDefaultMotionState* motion = new btDefaultMotionState(t);
		if ( mass != 0.0f ) shape->calculateLocalInertia(mass, inertia);
		btRigidBody::btRigidBodyConstructionInfo info(mass, motion, shape, inertia);
		btRigidBody* body = new btRigidBody(info);
		body->setUserPointer((void*) scene.getUid());
		ext::bullet::dynamicsWorld->addRigidBody(body);
		ext::bullet::collisionShapes.push_back(shape);
	}
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

	delete ext::bullet::dynamicsWorld;
	delete ext::bullet::solver;
	delete ext::bullet::overlappingPairCache;
	delete ext::bullet::dispatcher;
	delete ext::bullet::collisionConfiguration;
	ext::bullet::collisionShapes.clear();
}
void ext::bullet::syncToBullet() {
	auto& scene = uf::scene::getCurrentScene();
	// update bullet transforms
	for ( int i = ext::bullet::dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; --i) {
		btCollisionObject* obj = ext::bullet::dynamicsWorld->getCollisionObjectArray()[i];
		btRigidBody* body = btRigidBody::upcast(obj);
		if ( !body || !body->getMotionState() ) return;
	
	#if 0
		if ( rigidBodyMap.count(body) == 0 ) return;
		size_t uid = rigidBodyMap[body];
	#else
		size_t uid = (size_t) body->getUserPointer();
	#endif
		if ( !uid ) continue;
		auto* entity = scene.findByUid( uid );
		if ( !entity ) continue;

		auto& collider = entity->getComponent<pod::Bullet>();
		if ( !collider.shared ) continue;
		{
			auto& physics = entity->getComponent<pod::Physics>();
			body->setLinearVelocity( btVector3( physics.linear.velocity.x, physics.linear.velocity.y, physics.linear.velocity.z ) );
		}
		{
			auto& transform = entity->getComponent<pod::Transform<>>();
			auto modelMatrix = uf::transform::model( transform );
			
			btTransform t;
			t = body->getWorldTransform();
			t.setFromOpenGLMatrix(&modelMatrix[0]);

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
	#if 0
		if ( rigidBodyMap.count(body) == 0 ) return;
		size_t uid = rigidBodyMap[body];
	#else
		size_t uid = (size_t) body->getUserPointer();
	#endif
		if ( !uid ) continue;
		auto* entity = scene.findByUid( uid );
		if ( !entity ) continue;

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
	
	//	std::cout << entity->getName() << ": " << uid << " (" << i << ") @ " << uf::string::toString(transform.position) << std::endl;
	}
}
pod::Bullet& ext::bullet::create( uf::Object& object ) {
	auto& collider = object.getComponent<pod::Bullet>();

	collider.uid = object.getUid();
	collider.transform = &object.getComponent<pod::Transform<>>();
	collider.shared = false;
	return collider;
}
void ext::bullet::attach( pod::Bullet& collider ) {
	if ( !collider.shape ) return;
	
	btTransform t;
	t.setIdentity();
	t.setOrigin(btVector3(collider.transform->position.x, collider.transform->position.y, collider.transform->position.z));
	t.setRotation(btQuaternion(collider.transform->orientation.x, collider.transform->orientation.y, collider.transform->orientation.z, collider.transform->orientation.w));

	btVector3 inertia(collider.inertia.x, collider.inertia.y, collider.inertia.z);
	btDefaultMotionState* motion = new btDefaultMotionState(t);
	if ( collider.mass != 0.0f ) collider.shape->calculateLocalInertia(collider.mass, inertia);
	btRigidBody::btRigidBodyConstructionInfo info(collider.mass, motion, collider.shape, inertia);
	
	collider.body = new btRigidBody(info);
#if 0
	rigidBodyMap[collider.body] = collider.uid;
#else
	collider.body->setUserPointer((void*) collider.uid);
#endif

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

pod::Bullet& ext::bullet::create( uf::Object& object, const pod::Vector3f& corner, float mass ) {
	auto& collider = ext::bullet::create( object );
	collider.mass = mass;

	collider.shape = new btBoxShape(btVector3(corner.x, corner.y, corner.z));
	ext::bullet::attach( collider );

	collider.body->setContactProcessingThreshold(0.0);
	collider.body->setAngularFactor(0.0);
	collider.body->setCcdMotionThreshold(1e-7);
	collider.body->setCcdSweptSphereRadius(0.25 * 0.2);
	return collider;
}

pod::Bullet& ext::bullet::create( uf::Object& object, float radius, float height, float mass ) {
	auto& collider = ext::bullet::create( object );
	collider.mass = mass;

	collider.shape = new btCapsuleShape(radius, height); // 1, 1.5
	ext::bullet::attach( collider );

	collider.body->setContactProcessingThreshold(0.0);
	collider.body->setAngularFactor(0.0);
	collider.body->setCcdMotionThreshold(1e-7);
	collider.body->setCcdSweptSphereRadius(0.25 * 0.2);
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
		graphic.descriptor.cullMode = VK_CULL_MODE_NONE;

		graphic.material.attachShader("./data/shaders/base.colored.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		graphic.material.attachShader("./data/shaders/base.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		graphic.initialize();
		graphic.initializeGeometry( mesh, 0 );
		graphic.descriptor.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		graphic.descriptor.fill = VK_POLYGON_MODE_LINE;
		graphic.descriptor.lineWidth = 8.0f;
	} else {
		graphic.process = true;

		graphic.initializeGeometry( mesh, 0 );
		graphic.getPipeline().update( graphic );
	}
}