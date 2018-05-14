#include <uf/utils/math/collision.h>
#include <iostream>

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
uf::Collider::~Collider(){}
std::string UF_API uf::Collider::type() const { return ""; }
uf::Collider::Manifold UF_API uf::Collider::intersects( const uf::Collider& y ) const {
	const uf::Collider& x = *this;
	pod::Simplex simplex;
	pod::Vector3 direction = pod::Vector3{ 1.0f, 0.0f, 0.0f };
	
	uf::Collider::Manifold manifold(x, y);
	double dot = -0.1;
	uint iterations = 0;
	uint iterations_cap = 25;
	while ( iterations++ < iterations_cap ) {
		direction = uf::vector::normalize(direction);

		pod::Simplex::SupportPoint a;
		a.a = x.support(direction);
		a.b = y.support(-direction);
		a.v = a.a - a.b;

		if ( (dot = a.dot(direction)) < 0 ) {
			return manifold;
		}
		if( simplex.size == 0 ) {	
			simplex.b = a;

			direction = direction * -1;
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

			direction = abc * -1;
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
	goto EPA;

EPA:
	manifold.colliding = true;
	manifold.normal = direction;
	manifold.depth = dot;
	return manifold;
}
/*

*/
UF_API uf::AABBox::AABBox( const pod::Vector3& origin, const pod::Vector3& corner ) {
	this->m_origin = origin;
	this->m_corner = corner;
}
std::string UF_API uf::AABBox::type() const { return "AABBox"; }
pod::Vector3* UF_API uf::AABBox::expand() const {
	pod::Vector3* raw = new pod::Vector3[8];
	raw[0] = pod::Vector3{  this->m_corner.x,  this->m_corner.y,  this->m_corner.z};
	raw[1] = pod::Vector3{  this->m_corner.x,  this->m_corner.y, -this->m_corner.z};
	raw[2] = pod::Vector3{ -this->m_corner.x,  this->m_corner.y, -this->m_corner.z};
	raw[3] = pod::Vector3{ -this->m_corner.x,  this->m_corner.y,  this->m_corner.z};
	raw[4] = pod::Vector3{ -this->m_corner.x, -this->m_corner.y,  this->m_corner.z};
	raw[5] = pod::Vector3{  this->m_corner.x, -this->m_corner.y,  this->m_corner.z};
	raw[6] = pod::Vector3{  this->m_corner.x, -this->m_corner.y, -this->m_corner.z};
	raw[7] = pod::Vector3{ -this->m_corner.x, -this->m_corner.y,  this->m_corner.z};

	for ( uint i = 0; i < 8; i++ ) raw[0] += this->m_origin;

	return raw;
}
pod::Vector3 UF_API uf::AABBox::support( const pod::Vector3& direction ) const {
	pod::Vector3 res;

	res[0] = this->m_origin.x + this->m_corner.x * (direction.x >= 0 ? 1 : -1);
	res[1] = this->m_origin.y + this->m_corner.y * (direction.y >= 0 ? 1 : -1);
	res[2] = this->m_origin.z + this->m_corner.z * (direction.z >= 0 ? 1 : -1);

	return res;
}

uf::Collider::Manifold UF_API uf::AABBox::intersects( const uf::AABBox& b ) const {
	const uf::AABBox& a = *this;
	uf::Collider::Manifold manifold(a, b);

	float a_left 	= a.m_origin.x - a.m_corner.x;
	float a_right 	= a.m_origin.x + a.m_corner.x;
	float a_bottom 	= a.m_origin.y - a.m_corner.y;
	float a_top 	= a.m_origin.y + a.m_corner.y;
	float a_back 	= a.m_origin.z - a.m_corner.z;
	float a_front 	= a.m_origin.z + a.m_corner.z;


	float b_left 	= b.m_origin.x - b.m_corner.x;
	float b_right 	= b.m_origin.x + b.m_corner.x;
	float b_bottom 	= b.m_origin.y - b.m_corner.y;
	float b_top 	= b.m_origin.y + b.m_corner.y;
	float b_back 	= b.m_origin.z - b.m_corner.z;
	float b_front 	= b.m_origin.z + b.m_corner.z;

	manifold.depth = 9E9;
	auto test = [&]( const pod::Vector3& axis, float minA, float maxA, float minB, float maxB )->bool{
		float axisLSqr = uf::vector::dot(axis, axis);
		if ( axisLSqr < 1E-8f ) return false;

		float d0 = maxB - minA;
		float d1 = maxA - minB;
		if ( d0 < 0 || d1 < 0 ) return false;
		float overlap = d0 < d1 ? d0 : -d1;
		pod::Vector3 sep = axis * ( overlap / axisLSqr );
		float sepLSqr = uf::vector::dot( sep, sep );
		if ( sepLSqr < manifold.depth ) {
			manifold.normal = sep;
			manifold.depth = sepLSqr;
		}
		return true;
	};

	if ( !test( {1, 0, 0}, a_left, a_right, b_left, b_right ) ) return manifold;
	if ( !test( {0, 1, 0}, a_bottom, a_top, b_bottom, b_top ) ) return manifold;
	if ( !test( {0, 0, 1}, a_back, a_front, b_back, b_front ) ) return manifold;

	manifold.normal = uf::vector::normalize( manifold.normal );
	manifold.depth = sqrt( manifold.depth ); // * 1.001;
	manifold.colliding = true;

	return manifold;

/*
	if ( a_right < b_left ) return manifold;
	if ( a_left > b_right ) return manifold;

	if ( a_top < b_bottom ) return manifold;
	if ( a_bottom > b_top ) return manifold;

	if ( a_front < b_back ) return manifold;
	if ( a_back > b_front ) return manifold;

	pod::Vector3* a_points = a.expand();
	pod::Vector3* b_points = b.expand();

	float smallest = 9E9;
	for ( uint b = 0; b < 8; b++ ) {
	for ( uint a = 0; a < 8; a++ ) {
		float distance = uf::vector::distanceSquared(b_points[b], a_points[a]);
		smallest = fmin( smallest, distance );
	}
	}

	delete[] a_points;
	delete[] b_points;

	manifold.depth = sqrt(smallest);
	manifold.normal = b.m_origin - a.m_origin;
	manifold.colliding = true;

	return manifold;
*/
}
/*

*/
UF_API uf::SphereCollider::SphereCollider( float r, const pod::Vector3& origin ) {
	this->m_radius = r;
	this->m_origin = origin;
}
std::string UF_API uf::SphereCollider::type() const { return "Sphere"; }
float UF_API uf::SphereCollider::getRadius() const {
	return this->m_radius;
}
const pod::Vector3& UF_API uf::SphereCollider::getOrigin() const {
	return this->m_origin;
}

void UF_API uf::SphereCollider::setRadius( float r ) {
	this->m_radius = r;
}
void UF_API uf::SphereCollider::setOrigin( const pod::Vector3& origin ) {
	this->m_origin = origin;
}

pod::Vector3* UF_API uf::SphereCollider::expand() const {
	return NULL;
}
pod::Vector3 UF_API uf::SphereCollider::support( const pod::Vector3& direction ) const {
	return this->m_origin + direction * (this->m_radius/uf::vector::magnitude(direction));
}
uf::Collider::Manifold uf::SphereCollider::intersects( const uf::SphereCollider& b ) const {
	const uf::SphereCollider& a = *this;
	uf::Collider::Manifold manifold(a, b);
	float distance = uf::vector::distance(a.m_origin, b.m_origin);
	float sum = a.m_radius + b.m_radius;

	if ( distance >= sum ) return manifold;

	manifold.depth = fabs(b.m_radius - a.m_radius);
	manifold.normal = b.m_origin - a.m_origin;
	manifold.colliding = true;
	return manifold;
}
/*

*/
/*
uf::MeshCollider::MeshCollider( uf::Mesh& mesh ) : mesh(mesh) {
	const uf::Mesh::vertices_t verts = this->m_mesh.getVertices();
	this->m_raw = verts.get();
}
std::string UF_API uf::Collider::type() const { return "Mesh"; }
uf::Mesh& uf::MeshCollider::getMesh() {
	return this->m_mesh;
}
const uf::Mesh& uf::MeshCollider::getMesh() const {
	return this->m_mesh;
}
void uf::MeshCollider::setMesh( const uf::Mesh& mesh ) {
	this->m_mesh = mesh;
	const uf::Mesh::vertices_t verts = this->m_mesh.getVertices();
	this->m_raw = verts.get();
}

pod::Vector3* uf::MeshCollider::expand() const {
	return (pod::Vector3*) &this->m_raw[0];
}
pod::Vector3 uf::MeshCollider::support( const pod::Vector3& direction ) const {
	uint len = this->m_raw.size();
	pod::Vector3* points = this->m_expand();
	
	uint best = 0;
	float best_dot = points[0].dot(direction);

	for ( uint i = 1; i < len; i++ ) {
		float dot = points[i].dot(direction);
		if ( dot > best_dot ) {
			best = i;
			best_dot = dot;
		}
	}
	return points[best];
}
*/
/*

*/
UF_API uf::ModularCollider::ModularCollider( uint len, pod::Vector3* container, bool should_free, const uf::ModularCollider::function_expand_t& expand, const uf::ModularCollider::function_support_t& support  ) {
	this->m_len = len;
	this->m_container = container;
	this->m_should_free = should_free;
	this->m_function_expand = expand;
	this->m_function_support = support;
}
UF_API uf::ModularCollider::~ModularCollider() {
	if ( this->m_container != NULL && this->m_should_free ) delete[] this->m_container;
}
std::string UF_API uf::ModularCollider::type() const { return "Modular"; }
void UF_API uf::ModularCollider::setExpand( const uf::ModularCollider::function_expand_t& expand ) {
	this->m_function_expand = expand;
}
void UF_API uf::ModularCollider::setSupport( const uf::ModularCollider::function_support_t& support ) {
	this->m_function_support = support;
}

pod::Vector3* UF_API uf::ModularCollider::getContainer() {
	return this->m_container;
}
uint UF_API uf::ModularCollider::getSize() const {
	return this->m_len;
}
void UF_API uf::ModularCollider::setContainer( pod::Vector3* container, uint len ) {
	this->m_container = container;
	this->m_len = len;
}

pod::Vector3* UF_API uf::ModularCollider::expand() const {
	return this->m_function_expand ? this->m_function_expand() : this->m_container;
}
pod::Vector3 UF_API uf::ModularCollider::support( const pod::Vector3& direction ) const {
	if ( this->m_function_support ) return this->m_function_support(direction);
	uint len = this->m_len;
	pod::Vector3* points = this->expand();
	
	uint best = 0;
	float best_dot = uf::vector::dot(points[0], direction);

	for ( uint i = 1; i < len; i++ ) {
		float dot = uf::vector::dot(points[i], direction);
		if ( dot > best_dot ) {
			best = i;
			best_dot = dot;
		}
	}
	return points[best];
}

uf::CollisionBody::~CollisionBody() {
	this->clear();
}
void UF_API uf::CollisionBody::clear() {
	for ( uf::Collider* pointer : this->m_container ) delete pointer;
	this->m_container.clear();
}

void UF_API uf::CollisionBody::add( uf::Collider* pointer ) {
	this->m_container.push_back(pointer);
}
uf::CollisionBody::container_t& UF_API uf::CollisionBody::getContainer() {
	return this->m_container;
}
const uf::CollisionBody::container_t& UF_API uf::CollisionBody::getContainer() const {
	return this->m_container;
}

std::size_t UF_API uf::CollisionBody::getSize() const {
	return this->m_container.size();
}

std::vector<uf::Collider::Manifold> UF_API uf::CollisionBody::intersects( const uf::CollisionBody& body ) const {
	std::vector<uf::Collider::Manifold> manifolds;
	for ( const uf::Collider* pointer : body.m_container ) {
		std::vector<uf::Collider::Manifold> result = this->intersects( *pointer );
		manifolds.insert( manifolds.end(), result.begin(), result.end() );
	}
	return manifolds;
}

std::vector<uf::Collider::Manifold> UF_API uf::CollisionBody::intersects( const uf::Collider& body ) const {
	std::vector<uf::Collider::Manifold> manifolds;
	for ( const uf::Collider* pointer : this->m_container ) {
		uf::Collider::Manifold manifold;
		if ( pointer->type() == body.type() ) {
			if ( pointer->type() == "AABBox" ) {
				const uf::AABBox& a = *((const uf::AABBox*) pointer);
				const uf::AABBox& b = *((const uf::AABBox*) &body);
				manifold = a.intersects(b);
			} else if ( pointer->type() == "Sphere" ) {
				const uf::SphereCollider& a = *((const uf::SphereCollider*) pointer);
				const uf::SphereCollider& b = *((const uf::SphereCollider*) &body);
				manifold = a.intersects(b);
			}
			else manifold = pointer->intersects( body );
		} else manifold = pointer->intersects( body );
		manifolds.push_back( manifold );		
	}
	return manifolds;
}