#include <uf/utils/math/physics/common.h>

namespace impl {
	void updateStaticBody( pod::PhysicsBody& body ) {
		if ( !body.isStatic ) return;

		body.bounds = impl::computeAABB( body );
		if ( body.world ) body.world->staticBvh.dirty = true;
	}
}

// create ID from pointers
uint64_t impl::makePairKey( const pod::PhysicsBody& a, const pod::PhysicsBody& b ) {
	uint64_t lhs = reinterpret_cast<uint64_t>(&a);
	uint64_t rhs = reinterpret_cast<uint64_t>(&b);
	if (lhs > rhs) std::swap(lhs, rhs);
	size_t seed = 0;
	seed ^= std::hash<uint64_t>{}(lhs) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	seed ^= std::hash<uint64_t>{}(rhs) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	return seed;
}

// marks a body as asleep
void impl::wakeBody( pod::PhysicsBody& body ) {
	bool wasAwake = body.activity.awake;
	if ( !wasAwake ) {
		body.activity.sleepTimer = 0.0f;
	}

	body.activity.awake = true;
	if ( body.isStatic ) impl::updateStaticBody( body );
}
void impl::sleepBody( pod::PhysicsBody& body ) {
	bool wasAsleep = !body.activity.awake;

	body.activity.awake = false;
	body.velocity = pod::Vector3f{};
	body.angularVelocity = pod::Vector3f{};
}
void impl::updateActivity( pod::PhysicsBody& body, float dt ) {
	// reset grounded state
	bool wasGrounded = body.activity.grounded;
	body.activity.grounded = false;

	// update bounds
	body.bounds = impl::computeAABB( body );

	// already asleep
	if ( !body.activity.awake ) return;

	// check if body is moving
	float linSpeedSq = uf::vector::magnitude( body.velocity );
	float angSpeedSq = uf::vector::magnitude( body.angularVelocity );

	// body is nearly still
	if ( linSpeedSq < pod::Activity::linearSleepEpsilon && angSpeedSq < pod::Activity::angularSleepEpsilon ) {
		body.activity.sleepTimer += dt;
		float threshold = pod::Activity::sleepThreshold;

		if ( wasGrounded ) threshold *= 0.25f;
		if ( body.activity.sleepTimer > threshold ) impl::sleepBody( body );
	}
	// body is moving, reset timer
	else impl::wakeBody( body );
}

// returns an absolute transform while also allowing offsetting the collision body
// to-do: find a succinct way to explain this madness
pod::Transform<> impl::getTransform( const pod::PhysicsBody& body ) {
	pod::Transform<> t;
	t.position = body.offset;
	t.reference = body.transform;
	return uf::transform::flatten( t );
}

pod::Vector3f impl::getPosition( const pod::PhysicsBody& body, bool useTransform ) {
	useTransform = true; // guh

	if ( !useTransform ) return impl::aabbCenter( body.bounds );
	return impl::getTransform( body ).position;
}

pod::PhysicsBody impl::physicsBodyHullView( const pod::PhysicsBody& body, int32_t index ) {
	pod::PhysicsBody view = body;
	view.viewIndex = index;
	return view;
}

pod::PhysicsBody impl::physicsBodyTriView( const pod::PhysicsBody& body, const pod::TriangleWithNormal triangle ) {
	pod::PhysicsBody view = body;
	view.collider.type = pod::ShapeType::TRIANGLE;
	view.collider.triangle = triangle;
	// assume triangle is already transformed
	view.offset = {};
	view.transform = NULL;
	return view;
}
pod::PhysicsBody impl::physicsBodyTriView( const pod::PhysicsBody& body, size_t triID ) {
	auto tri = impl::fetchTriangle( *body.collider.mesh.mesh, triID, body );
	return impl::physicsBodyTriView( body, tri );
}

bool impl::shouldCollide( const pod::Collider& a, const pod::Collider& b ) {
	return ( a.category & b.mask ) && ( b.category & a.mask );
}
bool impl::shouldCollide( const pod::PhysicsBody& a, const pod::PhysicsBody& b ) {
	if ( a.isStatic && b.isStatic ) return false; // this shouldn't ever happen if we're segregating static bodies from dynamic bodies in the broadphase
	return impl::shouldCollide( a.collider, b.collider );
}

pod::Matrix3f impl::computeWorldInverseInertia( const pod::PhysicsBody& b ) {
	if ( b.isStatic || b.inverseMass == 0.0f ) return pod::Matrix3f{};

	pod::Matrix3f invI_local = uf::matrix::diagonal( b.inverseInertiaTensor );
	pod::Matrix3f R = uf::quaternion::matrix3(b.transform->orientation);

#if 1
	return R * invI_local * uf::matrix::transpose(R);
#else
	return uf::matrix::transpose(R) * invI_local * R;
#endif
}

// normalizes the delta between two bodies / contacts by the distance (as it was already computed) if non-zero
// a lot of collider v colliders use this semantic
pod::Vector3f impl::normalizeDelta( const pod::Vector3f& delta, float dist, const pod::Vector3f& fallback ) {
	return ( dist > EPS ) ? delta / dist : fallback;
}

pod::Vector3f impl::computeTangent( const pod::Vector3f& normal ) {
	pod::Vector3f up = ( std::fabs(normal.y) < 0.999f ) ? pod::Vector3f{0,1,0} : pod::Vector3f{1,0,0}; // pick a vector not parallel to normal
	pod::Vector3f tangent = uf::vector::normalize( uf::vector::cross( up, normal ) );
	return tangent;
}
pod::Vector3f impl::closestPointOnSegment( const pod::Vector3f& p, const pod::Vector3f& a, const pod::Vector3f& b ) {
	pod::Vector3f ab = b - a;
	float t = uf::vector::dot(p - a, ab) / uf::vector::dot(ab, ab);
	t = std::clamp( t, 0.0f, 1.0f );
	return a + ab * t;
}

std::pair<pod::Vector3f, pod::Vector3f> impl::closestSegmentSegment( const pod::Vector3f& A, const pod::Vector3f& B, const pod::Vector3f& C, const pod::Vector3f& D ) {
	auto u = B - A;
	auto v = D - C;
	auto w = A - C;

	float a = uf::vector::dot( u, u );
	float b = uf::vector::dot( u, v );
	float c = uf::vector::dot( v, v );
	float d = uf::vector::dot( u, w );
	float e = uf::vector::dot( v, w );
	
	float Dd = a*c - b*b;

	float sc, sN, sD = Dd;
	float tc, tN, tD = Dd;

	if ( Dd < EPS ) {
		sN = 0.0f;
		sD = 1.0f;
		tN = e;
		tD = c;
	} else {
		sN = (b*e - c*d);
		tN = (a*e - b*d);
		if (sN < 0) { sN = 0; tN = e; tD = c; }
		else if (sN > sD) { sN = sD; tN = e + b; tD = c; }
	}

	if ( tN < 0 ) {
		tN = 0;
		if (-d < 0) sN = 0;
		else if (-d > a) sN = sD;
		else { sN = -d; sD = a; }
	} else if ( tN > tD ) {
		tN = tD;
		if ((-d + b) < 0) sN = 0;
		else if ((-d + b) > a) sN = sD;
		else { sN = (-d + b); sD = a; }
	}

	sc = ( fabs(sN) < EPS ) ? 0 : ( sN / sD );
	tc = ( fabs(tN) < EPS ) ? 0 : ( tN / tD );

	return { A + u * sc, C + v * tc };
}

pod::Vector3f impl::closestPointSegmentAabb( const pod::Vector3f& p1, const pod::Vector3f& p2, const pod::AABB& box ) {
	// AABB center and half extents
	auto c = ( box.min + box.max ) * 0.5f;
	auto e = ( box.max - box.min ) * 0.5f;

	// direction of line segment
	auto d = p2 - p1;
	float len2 = uf::vector::magnitude( d );
	float t = 0.0f;

	if ( len2 > EPS2 ) {
		// parametric closest t from box center
		t = uf::vector::dot( c - p1, d ) / len2;
		t = std::clamp( t, 0.0f, 1.0f );
	}

	// closest point on segment to box center
	auto segClosest = p1 + d * t;

	// clamp this point into AABB
	return uf::vector::clamp( segClosest, box.min, box.max );
}

pod::Vector3f impl::computeBarycentric( const pod::Vector3f& p, const pod::Vector3f& a, const pod::Vector3f& b, const pod::Vector3f& c, bool clamps ) {
	// edges
	auto ab = b - a;
	auto ac = c - a;
	auto ao = p - a;

	// compute barycentric coords wrt triangle
	float d1 = uf::vector::dot(ab, ao);
	float d2 = uf::vector::dot(ac, ao);
	float d00 = uf::vector::dot(ab, ab);
	float d01 = uf::vector::dot(ab, ac);
	float d11 = uf::vector::dot(ac, ac);

	float denom = d00 * d11 - d01 * d01;

	float v = (d11*d1 - d01*d2) / denom;
	float w = (d00*d2 - d01*d1) / denom;
	float u = 1.0f - v - w;

	// clamp to triangle
	if ( clamps && (u < 0 || v < 0 || w < 0) ) {
		// if projected point is outside, snap to edges or vertices
		// check against AB
		float t = std::clamp(uf::vector::dot( ao, ab ) / d00, 0.0f, 1.0f);
		auto pAB = a + (ab * t);
		float distAB = uf::vector::dot( p - pAB, p - pAB );
		//float distAB = uf::vector::dot( pAB, pAB );

		// check against AC
		float t2 = std::clamp(uf::vector::dot( ao, ac ) / d11, 0.0f, 1.0f);
		auto pAC = a + (ac * t2);
		float distAC = uf::vector::dot( p - pAC, p - pAC );
		//float distAC = uf::vector::dot( pAC, pAC );

		// check against BC
		auto bc = c - b;
		float d22 = uf::vector::dot( bc, bc );
		float t3 = std::clamp(uf::vector::dot( p - b, bc ) / d22, 0.0f, 1.0f); 
		//float t3 = std::clamp(uf::vector::dot( -b, bc ) / d22, 0.0f, 1.0f);

		auto pBC = b + ( bc * t3 );
		float distBC = uf::vector::dot( p - pBC, p - pBC );
		//float distBC = uf::vector::dot( pBC, pBC );

		// pick closest edge/vertex
		if ( distAB <= distAC && distAB <= distBC ) return { 1.0f - t, t, 0.0f };
		if ( distAC <= distBC ) return { 1.0f - t2, 0.0f, t2 };
		return { 0.0f, 1.0f - t3, t3 };
	}
	
	return { u, v, w };
}
// made a huge mess of probably mixing up arguments................
pod::Vector3f impl::computeBarycentric(const pod::Vector3f& p, const pod::Triangle& tri, bool clamps ) {
	return impl::computeBarycentric( p, tri.points[0], tri.points[1], tri.points[2], clamps );
}
pod::Vector3f impl::interpolateWithBarycentric( const pod::Vector3f& bary, const pod::Vector3f a, const pod::Vector3f b, const pod::Vector3f c ) {
	return a * bary.x + b * bary.y + c * bary.z;
}
pod::Vector3f impl::interpolateWithBarycentric( const pod::Vector3f& bary, const pod::Vector3f points[3] ) {
	return impl::interpolateWithBarycentric( bary, points[0], points[1], points[2] );
}

bool impl::pointInTriangle( const pod::Vector3f& p, const pod::Vector3f& a, const pod::Vector3f& b, const pod::Vector3f& c ) {
	auto bary = impl::computeBarycentric( p, a, b, c, false );
	return ( bary.x >= -EPS && bary.y >= -EPS && bary.z >= -EPS );
}
bool impl::pointInTriangle( const pod::Vector3f& p, const pod::Triangle& tri ) {
	auto bary = impl::computeBarycentric( p, tri, false );
	return ( bary.x >= -EPS && bary.y >= -EPS && bary.z >= -EPS );
}

pod::Vector3f impl::closestPointOnTriangle( const pod::Vector3f& p, const pod::Vector3f& a, const pod::Vector3f& b, const pod::Vector3f& c ) {
	// check if P in vertex region outside A
	pod::Vector3f ab = b - a;
	pod::Vector3f ac = c - a;
	pod::Vector3f ap = p - a;
	float d1 = uf::vector::dot(ab, ap);
	float d2 = uf::vector::dot(ac, ap);
	if (d1 <= 0 && d2 <= 0) return a;

	// check if P in vertex region outside B
	pod::Vector3f bp = p - b;
	float d3 = uf::vector::dot(ab, bp);
	float d4 = uf::vector::dot(ac, bp);
	if (d3 >= 0 && d4 <= d3) return b;

	// check if P in edge region of AB, if so return projection on AB
	float vc = d1 * d4 - d3 * d2;
	if (vc <= 0 && d1 >= 0 && d3 <= 0) {
		float v = d1 / (d1 - d3);
		return a + ab * v;
	}

	// check vertex region outside C
	pod::Vector3f cp = p - c;
	float d5 = uf::vector::dot(ab, cp);
	float d6 = uf::vector::dot(ac, cp);
	if (d6 >= 0 && d5 <= d6) return c;

	// check edge region of AC
	float vb = d5 * d2 - d1 * d6;
	if (vb <= 0 && d2 >= 0 && d6 <= 0) {
		float w = d2 / (d2 - d6);
		return a + ac * w;
	}

	// check edge region of BC
	float va = d3 * d6 - d5 * d4;
	if (va <= 0 && (d4 - d3) >= 0 && (d5 - d6) >= 0) {
		float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
		return b + (c - b) * w;
	}

	// p inside face region. Return projection onto face
	float denom = 1.0f / (va + vb + vc);
	float v = vb * denom;
	float w = vc * denom;
	return a + ab * v + ac * w;
}
pod::Vector3f impl::closestPointOnTriangle( const pod::Vector3f& p, const pod::Triangle& tri ) {
	return impl::closestPointOnTriangle( p, tri.points[0], tri.points[1], tri.points[2] );
}

pod::Vector3f impl::orientNormalToAB( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Vector3f n ) {
	return uf::vector::normalize( uf::vector::dot( n, impl::getPosition( b ) - impl::getPosition( a ) ) < 0.0f ? -n : n );
}

float impl::segmentTriangleDistanceSq( const pod::Vector3f& p0, const pod::Vector3f& p1, const pod::Triangle& tri, pod::Vector3f& outSeg, pod::Vector3f& outTri ) {
	float best = std::numeric_limits<float>::max();

	auto n = uf::vector::cross( tri.points[1]-tri.points[0], tri.points[2]-tri.points[0] );
	float denom = uf::vector::dot( n, n );
	if ( denom > EPS ) {
		n /= std::sqrt( denom );
		float d0 = uf::vector::dot( p0 - tri.points[0], n );
		float d1 = uf::vector::dot( p1 - tri.points[0], n );
		if ( (d0 * d1) <= 0.0f ) {
			float t = d0 / (d0 - d1);
			auto q = p0 + (p1 - p0) * t;
			if ( impl::pointInTriangle( q, tri ) ) {
				outSeg = q;
				outTri = q;
				return 0.0f;
			}
		}
	}

	// segment endpoints to triangle
	for ( auto p : { p0, p1 } ) {
		auto q = impl::closestPointOnTriangle( p, tri );
		float d = uf::vector::distanceSquared( p, q );
		if ( d < best ) {
			best = d;
			outSeg = p;
			outTri = q;
		}
	}

	// segment edges to tri edges
	for ( auto i = 0; i < 3; i++ ) {
		auto j = ( i + 1 ) % 3;
		auto [s,e] = impl::closestSegmentSegment( p0, p1, tri.points[i], tri.points[j] );
		float d = uf::vector::distanceSquared( s, e );
		if ( d < best ) {
			best = d;
			outSeg = s;
			outTri = e;
		}
	}

	return best;
}

int impl::clipPolygonAgainstPlane( const pod::Vector3f* inPoly, int inCount, const pod::Vector3f& planeNormal, float planeOffset, pod::Vector3f* outPoly ) {
	if (inCount == 0) return 0;

	int outCount = 0;
	pod::Vector3f prevPoint = inPoly[inCount - 1];
	float prevDistance = uf::vector::dot(prevPoint, planeNormal) - planeOffset;

	for (int i = 0; i < inCount; ++i) {
		pod::Vector3f currPoint = inPoly[i];
		float currDistance = uf::vector::dot(currPoint, planeNormal) - planeOffset;

		// If they cross the plane, compute the intersection point
		if ((prevDistance * currDistance) < 0.0f) {
			float t = prevDistance / (prevDistance - currDistance);
			outPoly[outCount++] = prevPoint + (currPoint - prevPoint) * t;
		}

		// If the current point is 'inside' or on the plane (distance <= 0), keep it
		if (currDistance <= 0.0f) {
			outPoly[outCount++] = currPoint;
		}

		prevPoint = currPoint;
		prevDistance = currDistance;
	}

	return outCount;
}

pod::Vector3f impl::triangleCenter( const pod::Triangle& tri ) {
	return ( tri.points[0] + tri.points[1] + tri.points[2] ) / 3.0f;
}
pod::Vector3f impl::triangleNormal( const pod::Triangle& tri ) {
	return uf::vector::normalize(uf::vector::cross(tri.points[1] - tri.points[0], tri.points[2] - tri.points[0]));
}
pod::Vector3f impl::triangleNormal( const pod::TriangleWithNormal& tri ) {
	return tri.normal;
	//return uf::vector::normalize( tri.normals[0] + tri.normals[1] + tri.normals[2] );
}

bool impl::triangleTriangleIntersect( const pod::Triangle& a, const pod::Triangle& b ) {
	auto boxA = impl::computeTriangleAABB( a );
	auto boxB = impl::computeTriangleAABB( b );

	if ( !impl::aabbOverlap( boxA, boxB ) ) return false;

	// check vertices of a inside b or vice versa
	for ( auto i = 0; i < 3; ++i ) {
		auto q = impl::closestPointOnTriangle( a.points[i], b );
		if ( uf::vector::magnitude( q - a.points[i] ) < EPS2 ) return true;
	};
	for ( auto i = 0; i < 3; ++i ) {
		auto q = impl::closestPointOnTriangle( b.points[i], a );
		if ( uf::vector::magnitude( q - b.points[i] ) < EPS2 ) return true;
	};
	return false;
}

size_t impl::getIndex( const void* pointer, size_t stride, size_t index ) { 
	#define CAST_INDEX(T) case sizeof(T): return ((T*) pointer)[index];
	switch ( stride ) {
		CAST_INDEX(uint8_t);
		CAST_INDEX(uint16_t);
		CAST_INDEX(uint32_t);
		default: {
			UF_EXCEPTION("invalid stride type: {}", stride);
		} break;
	}
}
size_t impl::getIndex( const uf::Mesh::View& view, const uf::Mesh::AttributeView& indices, size_t index ) { 
	return impl::getIndex( indices.data(view.index.first), indices.stride(), index );
}
pod::Vector3f impl::getVertex( const uf::Mesh::View& view, const uf::Mesh::AttributeView& positions, size_t index ) {
	const auto stride = positions.stride();
	#define CAST_VERTEX(T) {\
		const T* vertices = (T*) positions.data(view.vertex.first + index);\
		return { vertices[0], vertices[1], vertices[2], };\
	}
	#define DEQUANTIZE_VERTEX(T) {\
		const T* vertices = (T*) positions.data(view.vertex.first + index);\
		return { uf::quant::dequantize(vertices[0]), uf::quant::dequantize(vertices[1]), uf::quant::dequantize(vertices[2]), };\
	}

	switch ( positions.attribute.descriptor.type ) {
		// dequantize
		case uf::renderer::enums::Type::USHORT:
		case uf::renderer::enums::Type::SHORT: {
			DEQUANTIZE_VERTEX(uint16_t);
		} break;
		case uf::renderer::enums::Type::FLOAT: {
			CAST_VERTEX(float);
		} break;
	#if UF_USE_FLOAT16
		case uf::renderer::enums::Type::HALF: {
			CAST_VERTEX(std::float16_t);
		} break;
	#endif
	#if UF_USE_BFLOAT16
		case uf::renderer::enums::Type::BFLOAT: {
			CAST_VERTEX(std::bfloat16_t);
		} break;
	#endif
		default: UF_EXCEPTION("unsupported vertex type: {}", positions.attribute.descriptor.type); break;
	}
//	return impl::getVertex( positions.data(view.vertex.first), positions.stride(), index );
}

pod::Triangle impl::fetchTriangle( const uf::Mesh::View& view, const uf::Mesh::AttributeView& indices, const uf::Mesh::AttributeView& positions, size_t triID ) {
	auto index = triID * 3;
	pod::Triangle tri;
	FOR_EACH(3, {
		tri.points[i] = impl::getVertex( view, positions, impl::getIndex( view, indices, index + i ) );
	});
	return tri;
}

// for clean code, this would be preferable
// but this incurs two lookups every triangle fetch, and I doubt the optimizer will optimize that away, so explicitly passing attribute views is preferable
pod::Triangle impl::fetchTriangle( const uf::Mesh::View& view, size_t triID ) {
	return impl::fetchTriangle( view, view["index"], view["position"], triID );
}

pod::TriangleWithNormal impl::fetchTriangle( const uf::Mesh& mesh, size_t triID ) {
	const auto& views = mesh.buffer_views;
	UF_ASSERT(!views.empty());

	// find which view contains this triangle index.
	size_t triBase = 0;
	const uf::Mesh::View* view = nullptr;
	for ( auto& v : views ) {
		auto trisInView = v.index.count / 3;
		if (triID < triBase + trisInView) {
			view = &v;
			triID -= triBase; // local triangle index inside this view
			break;
		}
		triBase += trisInView;
	}
	UF_ASSERT( view );
	
	pod::TriangleWithNormal tri = { impl::fetchTriangle( *view, triID ) };
	tri.normal = uf::vector::normalize(uf::vector::cross(tri.points[1] - tri.points[0], tri.points[2] - tri.points[0]));

	return tri;
}

// if body is a mesh, apply its transform to the triangles, else reorient the normal with respect to the body
pod::TriangleWithNormal impl::fetchTriangle( const uf::Mesh& mesh, size_t triID, const pod::PhysicsBody& body ) {
	auto tri = impl::fetchTriangle( mesh, triID );

	auto transform = impl::getTransform( body );

	if ( body.collider.type == pod::ShapeType::MESH ) {
		FOR_EACH(3, {
			tri.points[i] = uf::transform::apply( transform, tri.points[i] );
		});
		tri.normal = uf::quaternion::rotate( transform.orientation, tri.normal );
	}
	else {
	#if REORIENT_NORMALS_ON_FETCH
		auto triCenter = impl::triangleCenter( tri );
		auto delta = impl::getPosition( body ) - triCenter;
		if ( uf::vector::dot(tri.normal, delta) < 0.0f ) tri.normal = -tri.normal;
	#endif
	}

	return tri;
}

bool impl::computeTriangleTriangleSegment( const pod::TriangleWithNormal& A, const pod::TriangleWithNormal& B, pod::Vector3f& p0, pod::Vector3f& p1 ) {
	int intersections = 0;
	pod::Vector3f intersectionBuffers[6] = {};

	auto checkAndPush = [&]( const pod::Vector3f& pt ) {
		// avoid duplicates
		for ( auto& v : intersectionBuffers ) {
			if ( uf::vector::distanceSquared( v, pt ) < EPS*EPS ) return;
		}
		intersectionBuffers[intersections++] = pt;
	};

	// segment-plane intersection
	auto intersectSegmentPlane = [&](const pod::Vector3f& a, const pod::Vector3f& b, const pod::Vector3f& n, float d, pod::Vector3f& out)->bool {
		pod::Vector3f ab = b - a;
		float denom = uf::vector::dot( n, ab );
		if (fabs(denom) < EPS) return false; // parallel

		float t = (d - uf::vector::dot( n, a )) / denom;
		if ( t < -EPS || t > 1.0f + EPS ) return false;
		out = a + ab * t;
		return true;
	};

	// planes
	auto nA = impl::triangleNormal( A );
	auto nB = impl::triangleNormal( B );
	float dA = uf::vector::dot( nA, A.points[0] );
	float dB = uf::vector::dot( nB, B.points[0] );

	// clip edges of A against plane of B
	const pod::Vector3f At[3] = { A.points[0], A.points[1], A.points[2] };
	FOR_EACH(3, {
		auto j = ( i + 1 ) % 3;
		pod::Vector3f p;
		if ( intersectSegmentPlane( At[i], At[j], nB, dB, p ) ) {
			// check if intersection lies inside triangle B
			if ( impl::pointInTriangle( p, B ) ) checkAndPush(p);
		}
	});

	// clip edges of B against plane of A
	const pod::Vector3f Bt[3] = { B.points[0], B.points[1], B.points[2] };
	FOR_EACH(3, {
		auto j = ( i + 1 ) % 3;
		pod::Vector3f p;
		if ( intersectSegmentPlane( Bt[i], Bt[j], nA, dA, p ) ) {
			if ( impl::pointInTriangle( p, A ) ) checkAndPush(p);
		}
	});

	if ( intersections == 0 ) return false;

	// degenerate intersection
	if ( intersections == 1 ) {
		p0 = p1 = intersectionBuffers[0];
		return true;
	}

	// find two furthest apart points for intersection segment
	float maxDist2 = -1.0f;
	for ( auto i = 0 ; i < intersections; i++ ) {
		for ( auto j = i + 1; j < intersections; j++ ) {
			float d2 = uf::vector::distanceSquared( intersectionBuffers[i], intersectionBuffers[j] );
			if ( d2 > maxDist2 ) {
				maxDist2 = d2;
				p0 = intersectionBuffers[i];
				p1 = intersectionBuffers[j];
			}
		}
	}

	return maxDist2 >= 0.0f;
}

pod::Vector2f impl::projectTriangleOntoAxis( const pod::TriangleWithNormal& tri, const pod::Vector3f& axis ) {
	pod::Vector3f normal = uf::vector::normalize( axis );

	float p0 = uf::vector::dot( tri.points[0], normal );
	float p1 = uf::vector::dot( tri.points[1], normal );
	float p2 = uf::vector::dot( tri.points[2], normal );

	return { std::min({ p0, p1, p2 }), std::max({ p0, p1, p2 }) };
}

bool impl::aabbOverlap( const pod::AABB& a, const pod::AABB& b ) {
#if UF_USE_SIMD
	return uf::simd::all( uf::simd::lessEquals( a.min, b.max ) ) && uf::simd::all( uf::simd::greaterEquals( a.max, b.min ) );
#else
	return ( a.min - EPS ) <= ( b.max + EPS ) && ( a.max + EPS ) >= ( b.min - EPS );
#endif
}

float impl::aabbSurfaceArea(const pod::AABB& aabb) {
	auto d = uf::vector::max( ( aabb.max - aabb.min ), pod::Vector3f{} );
	return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
}

pod::AABB impl::computeSegmentAABB( const pod::Vector3f& p1, const pod::Vector3f p2, float r ) {
	return { 
		uf::vector::min( p1, p2 ) - r,
		uf::vector::max( p1, p2 ) + r,
	};
}

pod::Vector3f impl::closestPointOnAABB(const pod::Vector3f& p, const pod::AABB& box) {
	return uf::vector::clamp( p, box.min, box.max );
}

pod::AABB impl::computeTriangleAABB( const pod::Triangle& tri ) {
	return {
		uf::vector::min( uf::vector::min( tri.points[0], tri.points[1] ), tri.points[2] ),
		uf::vector::max( uf::vector::max( tri.points[0], tri.points[1] ), tri.points[2] ),
	};
}

pod::AABB impl::computeConvexHullAABB( const uf::Mesh::View& view, const uf::Mesh::AttributeView& positions, pod::AABB bounds ) {
	for ( size_t i = 0; i < view.vertex.count; ++i ) {
		pod::Vector3f v = impl::getVertex( view, positions, i );
		bounds.min = uf::vector::min( bounds.min, v );
		bounds.max = uf::vector::max( bounds.max, v );
	}

	return bounds;
}
pod::AABB impl::computeConvexHullAABB( const uf::Mesh::View& view, pod::AABB bounds ) {
	return impl::computeConvexHullAABB( view, view["position"], bounds );
}

pod::AABB impl::mergeAabb( const pod::AABB& a, const pod::AABB& b ) {
	return {
		uf::vector::min( a.min, b.min ),
		uf::vector::max( a.max, b.max ),
	};
}

pod::Vector3f impl::aabbCenter( const pod::AABB& aabb ) {
	return ( aabb.max + aabb.min ) * 0.5f;
}
pod::Vector3f impl::aabbExtent( const pod::AABB& aabb ) {
	return ( aabb.max - aabb.min ) * 0.5f;
}

pod::AABB impl::transformAabbToWorld( const pod::AABB& localBox, const pod::Transform<>& transform ) {
	const auto& q = transform.orientation;
	const auto& p = transform.position;

	pod::Vector3f center  = (localBox.min + localBox.max) * 0.5f;
	pod::Vector3f extents = (localBox.max - localBox.min) * 0.5f;

	pod::Vector3f axisX = uf::quaternion::rotate(q, pod::Vector3f{1,0,0});
	pod::Vector3f axisY = uf::quaternion::rotate(q, pod::Vector3f{0,1,0});
	pod::Vector3f axisZ = uf::quaternion::rotate(q, pod::Vector3f{0,0,1});

	pod::Vector3f worldCenter = uf::quaternion::rotate(q, center) + p;

	pod::Vector3f worldExtents = {
		fabs(axisX.x) * extents.x + fabs(axisY.x) * extents.y + fabs(axisZ.x) * extents.z,
		fabs(axisX.y) * extents.x + fabs(axisY.y) * extents.y + fabs(axisZ.y) * extents.z,
		fabs(axisX.z) * extents.x + fabs(axisY.z) * extents.y + fabs(axisZ.z) * extents.z
	};

	return {
		worldCenter - worldExtents,
		worldCenter + worldExtents
	};
}

std::pair<pod::Vector3f, pod::Vector3f> impl::getCapsuleSegment( const pod::PhysicsBody& body ) {
	const auto transform = impl::getTransform( body );
	const auto& capsule = body.collider.capsule;
	const pod::Vector3f up = uf::quaternion::rotate( transform.orientation, pod::Vector3f{0,1,0} );

	// segment defines the cylinder axis only (ignore spherical ends)
	auto p1 = transform.position + up * capsule.halfHeight;
	auto p2 = transform.position - up * capsule.halfHeight;
	return { p1, p2 };
}

pod::AABB impl::computeAABB( const pod::PhysicsBody& body ) {
	const auto transform = impl::getTransform( body );
	switch ( body.collider.type ) {
		case pod::ShapeType::AABB:
		case pod::ShapeType::OBB: {
			return impl::transformAabbToWorld( body.collider.aabb, transform );
		/*
			return {
				transform.position + body.collider.aabb.min,
				transform.position + body.collider.aabb.max,
			};
		*/
		} break;
		case pod::ShapeType::SPHERE: {
			return {
				transform.position - body.collider.sphere.radius,
				transform.position + body.collider.sphere.radius,
			};
		} break;
		case pod::ShapeType::CAPSULE: {
			auto [ p1, p2 ] = impl::getCapsuleSegment( body );
			return impl::computeSegmentAABB( p1, p2, body.collider.capsule.radius );
		} break;
		case pod::ShapeType::MESH:
		case pod::ShapeType::CONVEX_HULL: {
			if ( body.collider.mesh.bvh && !body.collider.mesh.bvh->bounds.empty() )
				return impl::transformAabbToWorld( body.collider.mesh.bvh->bounds[0], transform );
			const auto& meshData = *body.collider.mesh.mesh;
			pod::AABB bounds = { {  FLT_MAX,  FLT_MAX,  FLT_MAX }, { -FLT_MAX, -FLT_MAX, -FLT_MAX } };
			for ( const auto& view : meshData.buffer_views ) impl::computeConvexHullAABB( view, view["position"], bounds );
			return impl::transformAabbToWorld( bounds, transform );
		}
		default: {
		} break;
	}

	return {};
}

float impl::triAabbDistanceSq( const pod::Triangle& tri, const pod::AABB& box ) {
	float minDistSq = FLT_MAX;
	FOR_EACH(3, {
		auto cp = impl::closestPointOnAABB( tri.points[i], box );
		auto d  = tri.points[i] - cp;
		minDistSq = std::min( minDistSq, uf::vector::dot( d, d ) );
	});
	return minDistSq;
}

bool impl::triAabbOverlap( const pod::Triangle& tri, const pod::AABB& box ) {
	// compute box center and half extents
	auto c = ( box.min + box.max ) * 0.5f;
	auto e = ( box.max - box.min ) * 0.5f;

	// move triangle into box's local space
	auto v0 = tri.points[0] - c;
	auto v1 = tri.points[1] - c;
	auto v2 = tri.points[2] - c;

	// triangle edges
	auto f0 = v1 - v0;
	auto f1 = v2 - v1;
	auto f2 = v0 - v2;

	// SAT: test the 9 edge cross axes
	auto axisTest = [&]( const pod::Vector3f& axis ) {
		float norm = uf::vector::norm( axis );
		if ( norm < EPS ) return true;
		
		auto a = axis / norm;
		
		float p0 = uf::vector::dot( v0, a );
		float p1 = uf::vector::dot( v1, a );
		float p2 = uf::vector::dot( v2, a );

		float r = e.x * fabs(a.x) + e.y * fabs(a.y) + e.z * fabs(a.z);
		
		float minP = std::min({p0, p1, p2});
		float maxP = std::max({p0, p1, p2});
		
		return !(minP > r || maxP < -r);
	};

	if ( !axisTest( {0, -f0.z, f0.y} ) ) return false;
	if ( !axisTest( {0, -f1.z, f1.y} ) ) return false;
	if ( !axisTest( {0, -f2.z, f2.y} ) ) return false;
	if ( !axisTest( {f0.z, 0, -f0.x} ) ) return false;
	if ( !axisTest( {f1.z, 0, -f1.x} ) ) return false;
	if ( !axisTest( {f2.z, 0, -f2.x} ) ) return false;
	if ( !axisTest( {-f0.y, f0.x, 0} ) ) return false;
	if ( !axisTest( {-f1.y, f1.x, 0} ) ) return false;
	if ( !axisTest( {-f2.y, f2.x, 0} ) ) return false;

	// test AABB face axes
	for ( auto i = 0; i < 3; ++i ) {
		float minVal = std::min({v0[i], v1[i], v2[i]});
		float maxVal = std::max({v0[i], v1[i], v2[i]});
		if ( minVal > e[i] || maxVal < -e[i] ) return false;
	};

	// test triangle normal axis
	auto n = uf::vector::cross( f0, f1 );
	float d0 = uf::vector::dot( v0, n );
	float r  = e.x * fabs(n.x) + e.y * fabs(n.y) + e.z * fabs(n.z);
	if ( fabs(d0) > r ) return false;

	return true;
}

pod::AABB impl::transformAabbToLocal( const pod::AABB& box, const pod::Transform<>& transform ) {
	auto inv = uf::transform::inverse( transform );

	// transform all 8 corners
	pod::Vector3f corners[8] = {
		{ box.min.x, box.min.y, box.min.z },
		{ box.max.x, box.min.y, box.min.z },
		{ box.min.x, box.max.y, box.min.z },
		{ box.max.x, box.max.y, box.min.z },
		{ box.min.x, box.min.y, box.max.z },
		{ box.max.x, box.min.y, box.max.z },
		{ box.min.x, box.max.y, box.max.z },
		{ box.max.x, box.max.y, box.max.z },
	};

	pod::AABB out = {
		{  FLT_MAX,  FLT_MAX,  FLT_MAX },
		{ -FLT_MAX, -FLT_MAX, -FLT_MAX },
	};

	FOR_EACH(8, {
		auto local = uf::transform::apply( inv, corners[i] );
		out.min = uf::vector::min( out.min, local );
		out.max = uf::vector::max( out.max, local );
	});
	return out;
}