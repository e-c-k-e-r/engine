#include "BulletCollision/CollisionDispatch/btInternalEdgeUtility.h"
#if 0
template<typename T, typename U>
pod::Bullet& ext::bullet::create( uf::Object& object, const uf::BaseMesh<T, U>& mesh, bool dynamic ) {
	auto& transform = object.getComponent<pod::Transform<>>();
	auto model = uf::transform::model( transform );

	auto& collider = ext::bullet::create( object );
	
	btTriangleMesh* bMesh = new btTriangleMesh( true, false );
	bMesh->preallocateVertices( mesh.vertices.size() );
	for ( auto& vertex : mesh.vertices ) {
		bMesh->findOrAddVertex( btVector3(vertex.position.x, vertex.position.y, vertex.position.z), false );
	}

	if ( mesh.attributes.index.pointer ) {
		bMesh->preallocateIndices( mesh.indices.size() );
		for ( auto& index : mesh.indices ) bMesh->addIndex( index );
		bMesh->getIndexedMeshArray()[0].m_numTriangles = mesh.indices.size() / 3;
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
#if 0
	auto& collider = ext::bullet::create( object );
	btTriangleMesh* bMesh = new btTriangleMesh();
	auto& transform = object.getComponent<pod::Transform<>>();
	auto model = uf::transform::model( transform );
	auto expanded = mesh;
	expanded.expand();
	for ( size_t i = 0; i < expanded.vertices.size() / 3; ++i ) {
		if ( i * 3 + 2 >= expanded.vertices.size() ) break;
		auto a = expanded.vertices[i*3+0].position;
		auto b = expanded.vertices[i*3+1].position;
		auto c = expanded.vertices[i*3+2].position;
		if ( applyTransform ) {
			a = uf::matrix::multiply<float>( model, a );
			b = uf::matrix::multiply<float>( model, b );
			c = uf::matrix::multiply<float>( model, c );
		}

		if ( windingOrder == -1 ) {
			bMesh->addTriangle(btVector3(a.x, a.y, a.z), btVector3(b.x, b.y, b.z), btVector3(c.x, c.y, c.z));
		} else if ( windingOrder == 0 ) {
			bMesh->addTriangle(btVector3(a.x, a.y, a.z), btVector3(b.x, b.y, b.z), btVector3(c.x, c.y, c.z));
			bMesh->addTriangle(btVector3(a.x, a.y, a.z), btVector3(c.x, c.y, c.z), btVector3(b.x, b.y, b.z));
		} else if ( windingOrder == 1 ) {
			bMesh->addTriangle(btVector3(a.x, a.y, a.z), btVector3(c.x, c.y, c.z), btVector3(b.x, b.y, b.z));
		}
	}
	collider.shape = new btBvhTriangleMeshShape(bMesh, true);
	ext::bullet::attach( collider );
	{	
		btTransform t = collider.body->getWorldTransform();
		t.setFromOpenGLMatrix(&model[0]);
		collider.body->setWorldTransform(t);
		collider.body->setCenterOfMassTransform(t);
	}
	return collider;
	/*
		if ( applyTransform ) {
			auto transformed = uf::matrix::multiply<float>( model, vertex.position );
			queue.emplace_back( transformed.x, transformed.y, transformed.z );
		} else {
			queue.emplace_back( vertex.position.x, vertex.position.y, vertex.position.z );
		}
	*/
#endif
}
#endif