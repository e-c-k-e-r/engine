#include "BulletCollision/CollisionDispatch/btInternalEdgeUtility.h"

template<typename T, typename U>
pod::Bullet& ext::bullet::create( uf::Object& object, const uf::BaseMesh<T, U>& mesh, bool applyTransform, int windingOrder ) {
	auto& collider = ext::bullet::create( object );
	btTriangleMesh* bMesh = new btTriangleMesh();
	auto& transform = object.getComponent<pod::Transform<>>();
	auto model = uf::transform::model( transform );
/*
	if ( !mesh.indices.empty() ) {
		std::cout << "INDEXED VERTICES" << std::endl;
		for ( size_t i = 0; i < mesh.indices.size() / 3; ++i ) {			
			size_t A = mesh.indices[i*3]+0;
			size_t B = mesh.indices[i*3]+1;
			size_t C = mesh.indices[i*3]+2;

			if ( A >= mesh.vertices.size() ) continue;
			if ( B >= mesh.vertices.size() ) continue;
			if ( C >= mesh.vertices.size() ) continue;

			auto a = mesh.vertices[A].position;
			auto b = mesh.vertices[B].position;
			auto c = mesh.vertices[C].position;
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
			} else {
				std::cout << windingOrder << std::endl;
			}
		}
	} else {
		std::cout << "NON-INDEX VERTICES" << std::endl;
		for ( size_t i = 0; i < mesh.vertices.size() / 3; ++i ) {
			if ( i * 3 + 2 >= mesh.vertices.size() ) break;
			auto a = mesh.vertices[i*3+0].position;
			auto b = mesh.vertices[i*3+1].position;
			auto c = mesh.vertices[i*3+2].position;
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
			} else {
				std::cout << windingOrder << std::endl;
			}
		}
	}
*/
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
}