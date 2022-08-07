#if UF_USE_UNUSED
#include <uf/utils/math/collision/gjk.h>

/*
pod::Simplex::Simplex( const pod::Vector3& b, const pod::Vector3& c, const pod::Vector3& d ) {
	this->set(b, c, d);
}
void pod::Simplex::add( const pod::Vector3& a ) {
	this->d = this->c;
	this->c = this->b;
	this->b = a;
}
void pod::Simplex::set( const pod::Vector3& b, const pod::Vector3& c, const pod::Vector3& d ) {
	this->b = b;
	this->c = c;
	this->d = d;
	if ( b != pod::Vector3{} ) this->size++;
	if ( c != pod::Vector3{} ) this->size++;
	if ( d != pod::Vector3{} ) this->size++;
}
*/
pod::Collider::~Collider(){}
uf::stl::string pod::Collider::type() const { return ""; }
pod::Collider::Manifold pod::Collider::intersects( const pod::Collider& y ) const {
	const pod::Collider& x = *this;
	
	pod::Simplex::SupportPoint a;
	pod::Simplex simplex;
	pod::Vector3 direction = pod::Vector3{ 1.0f, 0.0f, 0.0f };
	
	pod::Collider::Manifold manifold(x, y);
	float dot = -0.1;
	uint16_t iterations = 0;
	uint16_t iterations_cap = 25;
	while ( iterations++ < iterations_cap ) {
		direction = uf::vector::normalize(direction);

		a = pod::Simplex::SupportPoint{};
		a.a = x.support(direction);
		a.b = y.support(-direction);
		a.v = a.a - a.b;

		if ( (dot = a.dot(direction)) < 0 ) {
			return manifold;
		}
		if( simplex.size == 0 ) {	
			simplex.b = a;

			direction = uf::vector::multiply(direction, -1);
			simplex.size = 1;
			continue;
		}
		if ( simplex.size == 1 ) {
			direction = uf::vector::cross(a.cross(simplex.b), a.v);
			simplex.c = simplex.b;
			simplex.b = a;
			simplex.size = 2;
			continue;
		}
		if ( simplex.size == 2 ) {
			pod::Vector3 ao = a * -1;
			pod::Vector3 ab = simplex.b - a;
			pod::Vector3 ac = simplex.c - a;
			pod::Vector3 abc = uf::vector::cross(ab, ac);
			pod::Vector3 abP = uf::vector::cross(ab, abc);
			if ( uf::vector::dot(abP, ao) > 0 ) {
				simplex.c = simplex.b;
				simplex.b = a;

				direction = uf::vector::cross(ab, ao);
				continue;
			}
			pod::Vector3 acP = uf::vector::cross(abc, ac);
			if ( uf::vector::dot(acP, ao) > 0 ) {
				simplex.b = a;

				direction = uf::vector::cross(ac, ao);
				continue;
			}
			if ( uf::vector::dot(abc, ao) > 0 ) {
				simplex.d = simplex.c;
				simplex.c = simplex.b;
				simplex.b = a;

				direction = abc;
				continue;
			}
			simplex.d = simplex.b;
			simplex.b = a;

			direction = uf::vector::multiply(abc, -1);
			simplex.size = 3;
			continue;
		}
		if ( simplex.size == 3 ) {
			pod::Vector3 ao = a * -1;
			pod::Vector3 ab = simplex.b - a;
			pod::Vector3 ac = simplex.c - a;
			pod::Vector3 abc = uf::vector::cross(ab, ac);
			pod::Vector3 ad, acd, adb;
			if ( uf::vector::dot(abc, ao) > 0 ) {
				goto check_face;
			}
			ad = simplex.d - a;
			acd = uf::vector::cross(ac, ad);
			if ( uf::vector::dot(acd, ao) > 0 ) {
				simplex.b = simplex.c;
				simplex.c = simplex.d;

				ab = ac;
				ac = ad;
				abc = acd;

				goto check_face;
			}
			goto EPA;

		check_face:
			pod::Vector3 abP = uf::vector::cross(ab, abc);
			if ( uf::vector::dot(abP, ao) > 0 ) {
				simplex.c = simplex.b;
				simplex.b = a;

				direction = uf::vector::cross(uf::vector::cross(ab, ao), ab);
				simplex.size = 2;
				continue;
			}
			pod::Vector3 acP = uf::vector::cross(abc, ac);
			if ( uf::vector::dot(acP, ao) > 0 ) {
				simplex.b = a;
				direction = uf::vector::cross(uf::vector::cross(ac, ao), ac);
				simplex.size = 2;
				continue;
			}
			simplex.d = simplex.c;
			simplex.c = simplex.b;
			simplex.b = a;
			direction = abc;
			simplex.size = 3;
			continue;
		}
	}
	return manifold;

EPA:
	struct Triangle {
		pod::Simplex::SupportPoint points[3];
		pod::Vector3f normal;
		Triangle( const pod::Simplex::SupportPoint& a, const pod::Simplex::SupportPoint& b, const pod::Simplex::SupportPoint& c ) {
			points[0] = a;
			points[1] = b;
			points[2] = c;
			normal = uf::vector::normalize( uf::vector::cross( b.v - a.v, c.v - a.v ) );
		}
	};
	struct Edge {
		pod::Simplex::SupportPoint points[2];
		Edge( const pod::Simplex::SupportPoint& a, const pod::Simplex::SupportPoint& b ) {
			points[0] = a;
			points[1] = b;
		}
	};

	iterations = 0;
	manifold.colliding = true;
	manifold.depth = 0.0f;
	manifold.normal = { 0, 0, 0 };

	uf::stl::vector<Triangle> triangles;
	uf::stl::vector<Edge> edges;

	triangles.emplace_back(         a, simplex.b, simplex.c );
	triangles.emplace_back(         a, simplex.b, simplex.d );
	triangles.emplace_back(         a, simplex.c, simplex.d );
	triangles.emplace_back( simplex.b, simplex.c, simplex.d );

	auto addEdge = [&]( const pod::Simplex::SupportPoint& a, const pod::Simplex::SupportPoint& b ) {
		for ( auto it = edges.begin(); it != edges.end(); ++it ) {
			if( it->points[0]== b && it->points[1]== a ) {
				edges.erase(it);
				return;
			}
		}
		edges.emplace_back( a, b );
	};

	while ( iterations++ < iterations_cap ) {
		// find closest triangle to origin
		struct {
			uf::stl::vector<Triangle>::iterator it;
			float distance = 9E9;
			pod::Simplex::SupportPoint support;
		} closest = { triangles.begin() };

		for(auto it = triangles.begin(); it != triangles.end(); it++) {
			float distance = fabs( uf::vector::dot(it->normal, it->points[0].v) );
			if( distance < closest.distance ) {
				closest.distance = distance;
				closest.it = it;
			}
		}
		{
			closest.support.a = x.support(closest.it->normal);
			closest.support.b = y.support(-closest.it->normal);
			closest.support.v = closest.support.a - closest.support.b;
		}

		manifold.normal = -closest.it->normal;
		manifold.depth = uf::vector::dot( closest.it->normal, closest.support.v );
		if( manifold.depth - closest.distance < 0.00001f ) {
			manifold.depth = uf::vector::dot( closest.it->normal, closest.it->points[0].v );
			return manifold;
		}

		for(auto it = triangles.begin(); it != triangles.end();) {
			// can this face be 'seen' by closest.support?
			if( uf::vector::dot( it->normal, (closest.support.v - it->points[0].v) ) > 0) {
				addEdge( it->points[0], it->points[1] );
				addEdge( it->points[1], it->points[2] );
				addEdge( it->points[2], it->points[0] );
				it = triangles.erase(it);
				continue;
			}
			it++;
		}

		// create new triangles from the edges in the edge list
		for(auto it = edges.begin(); it != edges.end(); it++)
			triangles.emplace_back( closest.support, it->points[0], it->points[1] );

		edges.clear();
	}
	return manifold;
}

pod::Vector3f pod::Collider::getPosition() const {
	return this->m_transform.reference ? this->m_transform.reference->position : this->m_transform.position;
//	return uf::transform::flatten( this->m_transform.reference ? *this->m_transform.reference : this->m_transform ).position;
}
pod::Transform<>& pod::Collider::getTransform() {
	return this->m_transform;
}
const pod::Transform<>& pod::Collider::getTransform() const {
	return this->m_transform;
}
void pod::Collider::setTransform( const pod::Transform<>& transform ) {
	this->m_transform = transform;
}
#endif