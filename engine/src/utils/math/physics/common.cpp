#include <uf/utils/math/physics/common.h>

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

	if ( body.inverseMass == 0.0f ) {
		body.bounds = impl::computeAABB( body );
		if ( body.world ) body.world->staticBvh.dirty = true;
	}
}
// marks body as awake
void impl::sleepBody( pod::PhysicsBody& body ) {
	bool wasAsleep = !body.activity.awake;

	body.activity.awake = false;
	body.velocity = pod::Vector3f{};
	body.angularVelocity = pod::Vector3f{};
}
// update body's grounded / sleep states
void impl::updateActivity( pod::PhysicsBody& body, float dt ) {
	// reset grounded state
	bool wasGrounded = body.activity.grounded;
	body.activity.grounded = false;

	// update bounds
	body.bounds = impl::computeAABB( body );

	// already asleep
	if ( !body.activity.awake ) return;

	// check if body is moving
	float linSpeed2 = uf::vector::magnitude( body.velocity );
	float angSpeed2 = uf::vector::magnitude( body.angularVelocity );

	// body is nearly still
	if ( linSpeed2 < pod::Activity::linearSleepEpsilon && angSpeed2 < pod::Activity::angularSleepEpsilon ) {
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
	pod::Transform<> t = {
		.position = body.offsetPosition,
		.orientation = body.offsetOrientation,
		.scale = {1, 1, 1},
		.reference = body.transform,
	};
	return uf::transform::flatten( t );
}
// get position of a body, uses bounds center or transform's position
pod::Vector3f impl::getPosition( const pod::PhysicsBody& body, bool useTransform ) {
	if ( !useTransform ) return impl::aabbCenter( body.bounds );
	return impl::getTransform( body ).position;
}
// applies a transform
pod::Vector3f impl::apply( const pod::Transform<>& t, const pod::Vector3f& p ) {
	return uf::transform::apply( t, p );
//	return uf::quaternion::rotate( t.orientation, p * t.scale ) + t.position; // explicitly needed to copy or GCC breaks
}
// applies an inverse transform
pod::Vector3f impl::applyInverse( const pod::Transform<>& t, const pod::Vector3f& p ) {
	return uf::transform::applyInverse( t, p );
}
/*
// these isometrically applies a transform
pod::Vector3f impl::apply( const pod::Transform<>& t, const pod::Vector3f& p ) {
	return uf::quaternion::rotate( t.orientation, p + t.position );
}
pod::Vector3f impl::applyInverse( const pod::Transform<>& t, const pod::Vector3f& p ) {
	return uf::quaternion::rotate( uf::quaternion::inverse( t.orientation ), p - t.position );
}
*/

uf::stl::string impl::similarMass( const pod::PhysicsBody& body ) {
	static uf::stl::vector<std::pair<float, uf::stl::string>> masses = {
		{ 5e-6,		"snowflake" },
		{ 2.5e-3,	"ping-pong ball" },
		{ 5e-3,		"penny" },
		{ 0.05,		"golf ball" },
		{ 0.17,		"billard ball" },
		{ 2,		"bag of sugar" },
		{ 7,		"male cat" },
		{ 10,		"bowling ball" },
		{ 30,		"dog" },
		{ 60,		"cheetah" },
		{ 90,		"adult male human" },
		{ 250,		"refrigerator" },
		{ 600,		"race horse" },
		{ 1000,		"small car" },
		{ 1650,		"medium car" },
		{ 2500,		"large car" },
		{ 6000,		"t-rex" },
		{ 7200,		"elephant" },
		{ 8e4,		"space shuttle" },
		{ 7e5,		"locomotive" },
		{ 9.2e6,	"Eiffel tower" },
		{ 6e24,		"the Earth" },
		{ 7e24,		"really freaking heavy" },
	};


	uf::stl::string value = "?";
	if ( body.inverseMass == 0.0f ) return value;
	float mass = 1.0f / body.inverseMass;
	for ( auto& [ m, s ] : masses ) if ( mass < m ) return s;
	return value;
}

// creates a view of a hull body
pod::PhysicsBody impl::physicsBodyHullView( const pod::PhysicsBody& body, int32_t index ) {
	pod::PhysicsBody view = body;
	view.viewIndex = index;
	return view;
}
// creates a view of a triangle
pod::PhysicsBody impl::physicsBodyTriView( const pod::TriangleWithNormal triangle, const pod::PhysicsBody& body ) {
	pod::PhysicsBody view = body;
	view.collider.type = pod::ShapeType::TRIANGLE;
	view.collider.triangle = triangle;
	// calculate normal if needed
	if ( uf::vector::magnitude( view.collider.triangle.normal ) < 0.001f ) {
		view.collider.triangle.normal = impl::triangleNormal( (const pod::Triangle&) triangle );
	}
	// assume triangle is already transformed
	view.offsetPosition = {};
	view.offsetOrientation = {0,0,0,1};
	view.transform = NULL;
	return view;
}
// creates a view of a mesh's triangle by triID
pod::PhysicsBody impl::physicsBodyTriView( const pod::PhysicsBody& body, size_t triID ) {
	auto tri = impl::fetchTriangle( *body.collider.mesh.mesh, triID, body );
	return impl::physicsBodyTriView( tri, body );
}
// checks whether or not two bodies would collide by mask
bool impl::shouldCollide( const pod::Collider& a, const pod::Collider& b ) {
	return ( a.category & b.mask ) && ( b.category & a.mask );
}
bool impl::shouldCollide( const pod::PhysicsBody& a, const pod::PhysicsBody& b ) {
	if ( a.inverseMass == 0.0f && b.inverseMass == 0.0f ) return false; // this shouldn't ever happen if we're segregating static bodies from dynamic bodies in the broadphase
	return impl::shouldCollide( a.collider, b.collider );
}

// returns an inverse inertia matrix from an inertia tensor
pod::Matrix3f impl::computeWorldInverseInertia( const pod::PhysicsBody& b ) {
	if ( b.inverseMass == 0.0f ) return pod::Matrix3f{};

	auto t = impl::getTransform( b );
	pod::Matrix3f invI_local = uf::matrix::diagonal( b.inverseInertiaTensor );
	pod::Matrix3f R = uf::quaternion::matrix3( t.orientation );

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

// computes the tangent of a normal
pod::Vector3f impl::computeTangent( const pod::Vector3f& normal ) {
	pod::Vector3f up = ( std::fabs(normal.y) < 0.999f ) ? pod::Vector3f{0,1,0} : pod::Vector3f{1,0,0}; // pick a vector not parallel to normal
	pod::Vector3f tangent = uf::vector::normalize( uf::vector::cross( up, normal ) );
	return tangent;
}
// returns the closest point on an A->B segment
pod::Vector3f impl::closestPointOnSegment( const pod::Vector3f& p, const pod::Vector3f& a, const pod::Vector3f& b ) {
	pod::Vector3f ab = b - a;
	float t = uf::vector::dot(p - a, ab) / uf::vector::dot(ab, ab);
	t = std::clamp( t, 0.0f, 1.0f );
	return a + ab * t;
}
// 
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
// 
pod::Vector3f impl::closestPointSegmentAabb( const pod::Vector3f& p1, const pod::Vector3f& p2, const pod::AABB& box ) {
	// AABB center and half extents
	auto c = impl::aabbCenter( box );
	auto e = impl::aabbExtent( box );

	// direction of line segment
	auto d = p2 - p1;
	float len2 = uf::vector::magnitude( d );
	float t = 0.0f;

	if ( len2 > EPS2 ) {
		// parametric closest t from box center
		t = uf::vector::dot( c - p1, d ) / len2; // sqrt?
		t = std::clamp( t, 0.0f, 1.0f );
	}

	// closest point on segment to box center
	auto segClosest = p1 + d * t;

	// clamp this point into AABB
	return uf::vector::clamp( segClosest, box.min, box.max );
}
// returns the barycentric coordinates of a point on a triangle
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
// returns if a point is inside a triangle
bool impl::pointInTriangle( const pod::Vector3f& p, const pod::Vector3f& a, const pod::Vector3f& b, const pod::Vector3f& c ) {
	auto bary = impl::computeBarycentric( p, a, b, c, false );
	return ( bary.x >= -EPS && bary.y >= -EPS && bary.z >= -EPS );
}
bool impl::pointInTriangle( const pod::Vector3f& p, const pod::Triangle& tri ) {
	auto bary = impl::computeBarycentric( p, tri, false );
	return ( bary.x >= -EPS && bary.y >= -EPS && bary.z >= -EPS );
}
// returns the closest point on a triangle (possible duplicate of computeBarycentric)
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
// reorients a normal from body A to body B
pod::Vector3f impl::orientNormalToAB( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Vector3f n ) {
	return uf::vector::normalize( uf::vector::dot( n, impl::getPosition( b ) - impl::getPosition( a ) ) < 0.0f ? -n : n );
}
//
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

// Separating Axis Theorem test
bool impl::testSeparatingAxis( const pod::Triangle& triangle, const pod::OBB& box, const pod::Vector3f& axis, const pod::Vector3f axes[3], float& outMinOverlap, pod::Vector3f& outBestAxis ) {
	float mag2 = uf::vector::magnitude( axis );
	if ( mag2 < EPS2 ) return true;
	pod::Vector3f n = axis / std::sqrt( mag2 );

	// project triangle
	float p0 = uf::vector::dot( triangle.points[0], n );
	float p1 = uf::vector::dot( triangle.points[1], n );
	float p2 = uf::vector::dot( triangle.points[2], n );
	float minT = std::min( { p0, p1, p2 } );
	float maxT = std::max( { p0, p1, p2 } );

	// project box
	float pB = uf::vector::dot( box.center, n );
	float rB = impl::projectExtents( box, n, axes );

	float minB = pB - rB;
	float maxB = pB + rB;

	// check for separation
	if ( minT > maxB || maxT < minB ) return false;

	// calculate overlap depth
	float overlap = std::min(maxT, maxB) - std::max(minT, minB);
	if ( overlap < outMinOverlap ) {
		outMinOverlap = overlap;
		outBestAxis = n;
	}
	return true;
}
bool impl::testSeparatingAxis( const pod::OBB& boxA, const pod::OBB& boxB, const pod::Vector3f axesA[3], const pod::Vector3f axesB[3], const pod::Vector3f& axis, float& outMinOverlap, pod::Vector3f& outBestAxis ) {
	float mag2 = uf::vector::magnitude(axis);
	if ( mag2 < EPS2 ) return true;
	pod::Vector3f n = axis / std::sqrt( mag2 );

	float pA = uf::vector::dot( boxA.center, n );
	float rA = impl::projectExtents( boxA, n, axesA );

	float pB = uf::vector::dot( boxB.center, n );
	float rB = impl::projectExtents( boxB, n, axesB );

	float dist = std::fabs( pB - pA );
	float overlap = ( rA + rB ) - dist;

	if ( overlap < 0.0f ) return false;

	if ( overlap < outMinOverlap ) {
		outMinOverlap = overlap;
		outBestAxis = n;
	}
	return true;
}

// Sutherland-Hodgman polygon clipping
void impl::clipPolygon( pod::Vector3f* poly, int& polyCount, const pod::Plane& plane ) {
	if ( polyCount == 0 ) return;

	int outCount = 0;
	pod::Vector3f out[8];

	for ( auto i = 0; i < polyCount; i++ ) {
		auto curr = poly[i];
		auto prev = poly[(i + polyCount - 1) % polyCount];

		float dCurr = uf::vector::dot(plane.normal, curr) - plane.offset;
		float dPrev = uf::vector::dot(plane.normal, prev) - plane.offset;

		if ( dCurr <= 0.0f ) {
			if ( dPrev > 0.0f ) {
				float t = dPrev / (dPrev - dCurr);
				out[outCount++] = prev + (curr - prev) * t;
			}
			out[outCount++] = curr;
		}
		else if ( dPrev <= 0.0f ) {
			float t = dPrev / (dPrev - dCurr);
			out[outCount++] = prev + (curr - prev) * t;
		}
	}

	polyCount = outCount;
	for ( auto i = 0; i < outCount; i++ ) poly[i] = out[i];
}
void impl::clipPolygon( pod::Vector3f* poly, int& polyCount, const pod::AABB& aabb ) {
	pod::Plane planes[6] = {
		{ pod::Vector3f{-1, 0, 0}, -aabb.max.x },
		{ pod::Vector3f{ 1, 0, 0},  aabb.min.x },
		{ pod::Vector3f{ 0,-1, 0}, -aabb.max.y },
		{ pod::Vector3f{ 0, 1, 0},  aabb.min.y },
		{ pod::Vector3f{ 0, 0,-1}, -aabb.max.z },
		{ pod::Vector3f{ 0, 0, 1},  aabb.min.z }
	};

	for ( auto i = 0; i < 6; i++ ) {
		impl::clipPolygon( poly, polyCount, planes[i] );
		// degenerated
		if ( polyCount < 3 ) {
			polyCount = 0;
			break;
		}
	}
}
// returns the center of a triangle
pod::Vector3f impl::triangleCenter( const pod::Triangle& tri ) {
	return ( tri.points[0] + tri.points[1] + tri.points[2] ) / 3.0f;
}
// returns the normal of a triangle
pod::Vector3f impl::triangleNormal( const pod::Triangle& tri ) {
	return uf::vector::normalize(uf::vector::cross(tri.points[1] - tri.points[0], tri.points[2] - tri.points[0]));
}
pod::Vector3f impl::triangleNormal( const pod::TriangleWithNormal& tri ) {
	if ( uf::vector::magnitude( tri.normal ) < 0.001f )  return impl::triangleNormal( (const pod::Triangle&) tri );
	return tri.normal;
}
// if body is a mesh, apply its transform to the triangles, else reorient the normal with respect to the body
pod::TriangleWithNormal impl::fetchTriangle( const uf::Mesh& mesh, size_t triID, const pod::PhysicsBody& body ) {
	auto tri = uf::mesh::fetchTriangle( mesh, triID );

	auto transform = impl::getTransform( body );

	if ( body.collider.type == pod::ShapeType::MESH || body.collider.type == pod::ShapeType::CONVEX_HULL ) {
		FOR_EACH(3, {
			tri.points[i] = impl::apply( transform, tri.points[i] );
		});
		tri.normal = uf::quaternion::rotate( transform.orientation, tri.normal );
	}
	#if REORIENT_NORMALS_ON_FETCH
	else {
		auto triCenter = impl::triangleCenter( tri );
		auto delta = impl::getPosition( body ) - triCenter;
		if ( uf::vector::dot(tri.normal, delta) < 0.0f ) tri.normal = -tri.normal;
	}
	#endif

	return tri;
}

// returns whether or not two AABBs are overlapping (with SIMD speedup)
bool impl::aabbOverlap( const pod::AABB& a, const pod::AABB& b ) {
#if UF_USE_SIMD
	return uf::simd::all( uf::simd::lessEquals( a.min, b.max ) ) && uf::simd::all( uf::simd::greaterEquals( a.max, b.min ) );
#else
	return ( a.min - EPS ) <= ( b.max + EPS ) && ( a.max + EPS ) >= ( b.min - EPS );
#endif
}
// returns the surface area an AABB covers
float impl::aabbSurfaceArea(const pod::AABB& aabb) {
	auto d = uf::vector::max( ( aabb.max - aabb.min ), pod::Vector3f{} );
	return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
}
// returns the bounds a line segment covers with a radius (for capsules)
pod::AABB impl::computeSegmentAABB( const pod::Vector3f& p1, const pod::Vector3f p2, float r ) {
	return { 
		uf::vector::min( p1, p2 ) - r,
		uf::vector::max( p1, p2 ) + r,
	};
}
// returns the closest point on an AABB
pod::Vector3f impl::closestPointOnAABB(const pod::Vector3f& p, const pod::AABB& box) {
	return uf::vector::clamp( p, box.min, box.max );
}
// returns the AABB of a triangle
pod::AABB impl::computeTriangleAABB( const pod::Triangle& tri ) {
	return {
		uf::vector::min( uf::vector::min( tri.points[0], tri.points[1] ), tri.points[2] ),
		uf::vector::max( uf::vector::max( tri.points[0], tri.points[1] ), tri.points[2] ),
	};
}
// returns the AABB of a hull
pod::AABB impl::computeConvexHullAABB( const uf::Mesh::View& view, const uf::Mesh::AttributeView& positions, pod::AABB bounds ) {
	for ( size_t i = 0; i < view.vertex.count; ++i ) {
		pod::Vector3f v = uf::mesh::fetchVertex( view, positions, i );
		bounds.min = uf::vector::min( bounds.min, v );
		bounds.max = uf::vector::max( bounds.max, v );
	}

	return bounds;
}
pod::AABB impl::computeConvexHullAABB( const uf::Mesh::View& view, pod::AABB bounds ) {
	return impl::computeConvexHullAABB( view, view["position"_hash], bounds );
}
// combines two AABBs
pod::AABB impl::mergeAabb( const pod::AABB& a, const pod::AABB& b ) {
	return {
		uf::vector::min( a.min, b.min ),
		uf::vector::max( a.max, b.max ),
	};
}
// returns the center of an AABB
pod::Vector3f impl::aabbCenter( const pod::AABB& aabb ) {
	return ( aabb.max + aabb.min ) * 0.5f;
}
// returns the half extents of an AABB
pod::Vector3f impl::aabbExtent( const pod::AABB& aabb ) {
	return ( aabb.max - aabb.min ) * 0.5f;
}
// returns the min bound of an OBB
pod::Vector3f impl::obbMin( const pod::OBB& obb ) {
	return obb.center - obb.extent;
}
// returns the max bound of an OBB
pod::Vector3f impl::obbMax( const pod::OBB& obb ) {
	return obb.center + obb.extent;
}
// converts a min-max AABB to center-extents OBB
pod::OBB impl::aabbToObb( const pod::AABB& aabb ) {
#if OBB_EXTENT_CENTER
	return pod::OBB{
		.extent = impl::aabbExtent( aabb ),
		.center = impl::aabbCenter( aabb ),
	};
#else
	return pod::OBB{
		.center = impl::aabbCenter( aabb ),
		.extent = impl::aabbExtent( aabb ),
	};
#endif
}
// converts a center-extents OBB to min-max AABB
pod::AABB impl::obbToAabb( const pod::OBB& obb ) {
	return pod::AABB{
		.min = impl::obbMin( obb ),
		.max = impl::obbMax( obb ),
	};
}
// returns AABB axes
void impl::boxAxes( pod::Vector3f axes[3] ) {
	axes[0] = {1,0,0};
	axes[1] = {0,1,0};
	axes[2] = {0,0,1};
}
// returns OBB axes
void impl::boxAxes( pod::Vector3f axes[3], const pod::Transform<>& transform ) {
	axes[0] = uf::quaternion::rotate(transform.orientation, pod::Vector3f{1,0,0});
	axes[1] = uf::quaternion::rotate(transform.orientation, pod::Vector3f{0,1,0});
	axes[2] = uf::quaternion::rotate(transform.orientation, pod::Vector3f{0,0,1});
}
// computes a box's extents from given axes
pod::Vector3f impl::extentFromAxes( const pod::OBB& box, const pod::Vector3f axes[3] ) {
	return ( uf::vector::abs(axes[0]) * box.extent.x ) + ( uf::vector::abs(axes[1]) * box.extent.y ) + ( uf::vector::abs(axes[2]) * box.extent.z );
}
//
float impl::projectExtents( const pod::OBB& box, const pod::Vector3f& normal, const pod::Vector3f axes[3] ) {
	return uf::vector::dot(box.extent, uf::vector::abs( pod::Vector3f{
		uf::vector::dot(axes[0], normal),
		uf::vector::dot(axes[1], normal),
		uf::vector::dot(axes[2], normal)
	} ) );
//	return box.extent.x * std::fabs(uf::vector::dot(axes[0], normal)) + box.extent.y * std::fabs(uf::vector::dot(axes[1], normal)) + box.extent.z * std::fabs(uf::vector::dot(axes[2], normal));
}
// transforms an AABB into world-space
pod::AABB impl::transformAabbToWorld( const pod::AABB& aabb, const pod::Transform<>& transform ) {
	auto box = impl::aabbToObb( aabb );
	pod::Vector3f axes[3];
	impl::boxAxes( axes, transform );

	pod::Vector3f center = impl::apply( transform, box.center );
	pod::Vector3f extent = impl::extentFromAxes( box, axes );

	return { center - extent, center + extent };
}
// returns the line segment of a capsule
std::pair<pod::Vector3f, pod::Vector3f> impl::getCapsuleSegment( const pod::PhysicsBody& body ) {
	const auto transform = impl::getTransform( body );
	const auto& capsule = body.collider.capsule;
	const pod::Vector3f up = uf::quaternion::rotate( transform.orientation, capsule.up );

	// segment defines the cylinder axis only (ignore spherical ends)
	auto p1 = transform.position + up;
	auto p2 = transform.position - up;
	return { p1, p2 };
}
// computes the AABB for a given body
pod::AABB impl::computeAABB( const pod::PhysicsBody& body ) {
	const auto transform = impl::getTransform( body );
	switch ( body.collider.type ) {
		case pod::ShapeType::AABB: {
			return impl::transformAabbToWorld( body.collider.aabb, transform );
		} break;
		case pod::ShapeType::OBB: {
			return impl::transformAabbToWorld( impl::obbToAabb( body.collider.obb ), transform );
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
			for ( const auto& view : meshData.buffer_views ) impl::computeConvexHullAABB( view, view["position"_hash], bounds );
			return impl::transformAabbToWorld( bounds, transform );
		}
		default: {
		} break;
	}

	return {};
}
// gets the corners of an AABB
void impl::getCorners( const pod::AABB& aabb, pod::Vector3f corners[8] ) {
	corners[0] = {aabb.min.x, aabb.min.y, aabb.min.z};
	corners[1] = {aabb.max.x, aabb.min.y, aabb.min.z};
	corners[2] = {aabb.max.x, aabb.max.y, aabb.min.z};
	corners[3] = {aabb.min.x, aabb.max.y, aabb.min.z};
	corners[4] = {aabb.min.x, aabb.min.y, aabb.max.z};
	corners[5] = {aabb.max.x, aabb.min.y, aabb.max.z};
	corners[6] = {aabb.max.x, aabb.max.y, aabb.max.z};
	corners[7] = {aabb.min.x, aabb.max.y, aabb.max.z};
}
void impl::getCorners( const pod::AABB& aabb, const pod::Transform<>& transform, pod::Vector3f corners[8] ) {
	impl::getCorners( aabb, corners );
	FOR_EACH( 8, {
		corners[i] = impl::apply( transform, corners[i] );
	});
}
// transforms an AABB into local space
pod::AABB impl::transformAabbToLocal( const pod::AABB& box, const pod::Transform<>& transform ) {
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
		auto local = impl::applyInverse( transform, corners[i] );
		out.min = uf::vector::min( out.min, local );
		out.max = uf::vector::max( out.max, local );
	});
	return out;
}