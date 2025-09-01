// forward declare
namespace {
	bool aabbAabb( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool aabbSphere( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool aabbPlane( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool aabbCapsule( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool aabbMesh( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	
	bool sphereSphere( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool sphereAabb( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool spherePlane( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool sphereCapsule( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool sphereMesh( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	
	bool planeAabb( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool planeSphere( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool planePlane( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool planeCapsule( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool planeMesh( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	
	bool capsuleCapsule( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool capsuleAabb( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool capsulePlane( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool capsuleSphere( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool capsuleMesh( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );

	bool meshAabb( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool meshSphere( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool meshPlane( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool meshCapsule( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool meshMesh( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );

	bool triangleTriangle( const pod::TriangleWithNormal& a, const pod::TriangleWithNormal& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool triangleAabb( const pod::TriangleWithNormal& tri, const pod::RigidBody& body, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool triangleSphere( const pod::TriangleWithNormal& tri, const pod::RigidBody& body, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool trianglePlane( const pod::TriangleWithNormal& tri, const pod::RigidBody& body, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool triangleCapsule( const pod::TriangleWithNormal& tri, const pod::RigidBody& body, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );

	pod::Vector3f aabbCenter( const pod::AABB& aabb );
	bool aabbOverlap( const pod::AABB& a, const pod::AABB& b, float eps = EPS(1.0e-6f) );
	pod::AABB computeTriangleAABB( const pod::Triangle& tri );

	void solveContacts( uf::stl::vector<pod::Manifold>& manifolds, float dt );

	void traverseNodePair( const pod::BVH& bvh, int leftID, int rightID, pod::BVH::pair_t& pairs );
	void traverseNodePair( const pod::BVH& a, int nodeA, const pod::BVH& b, int nodeB, pod::BVH::pair_t& out );
	
	void queryBVH( const pod::BVH& bvh, const pod::AABB& bounds, uf::stl::vector<int>& indices );
	void queryBVH( const pod::BVH& bvh, const pod::AABB& bounds, uf::stl::vector<int>& indices, int nodeID );
	void queryBVH( const pod::BVH& bvh, const pod::Ray& ray, uf::stl::vector<int>& indices, int nodeID, float maxDist = FLT_MAX );
	void queryBVH( const pod::BVH& bvh, const pod::Ray& ray, uf::stl::vector<int>& indices, float maxDist = FLT_MAX );
}

namespace {
	// create ID from pointers
	uint64_t makePairKey( const pod::RigidBody& a, const pod::RigidBody& b ) {
		auto idA = reinterpret_cast<uint64_t>(&a);
		auto idB = reinterpret_cast<uint64_t>(&b);
		if ( idA > idB ) std::swap(idA, idB); // ensure consistent order
		return (idA << 32) ^ idB;
	}

	// returns an absolute transform while also allowing offsetting the collision body
	// to-do: find a succint way to explain this madness
	pod::Transform<> getTransform( const pod::RigidBody& body ) {
		pod::Transform<> t;
		t.position = body.offset;
		t.reference = body.transform;
		return uf::transform::flatten( t );
	}

	pod::Vector3f getPosition( const pod::RigidBody& body, bool useTransform = false ) {
		if ( !useTransform ) return ::aabbCenter( body.bounds );
		return ::getTransform( body ).position;
	}

	// normalizes the delta between two bodies / contacts by the distance (as it was already computed) if non-zero
	// a lot of collider v colliders use this semantic
	pod::Vector3f normalizeDelta( const pod::Vector3f& delta, float dist, float eps = EPS(1.0e-6), const pod::Vector3f& fallback = pod::Vector3f{0,1,0} ) {
		return ( dist > eps ) ? delta / dist : fallback;
	}

	pod::Vector3f computeTangent( const pod::Vector3f& normal ) {
		pod::Vector3f up = ( std::fabs(normal.y) < 0.999f ) ? pod::Vector3f{0,1,0} : pod::Vector3f{1,0,0}; // pick a vector not parallel to normal
		pod::Vector3f tangent = uf::vector::normalize( uf::vector::cross( up, normal ) );
		return tangent;
	}
	pod::Vector3f closestPointOnSegment( const pod::Vector3f& p, const pod::Vector3f& a, const pod::Vector3f& b ) {
		pod::Vector3f ab = b - a;
		float t = uf::vector::dot(p - a, ab) / uf::vector::dot(ab, ab);
		t = std::max(0.0f, std::min(1.0f, t)); // clamp( t, 0, 1 )
		return a + ab * t;
	}

	std::pair<pod::Vector3f, pod::Vector3f> closestSegmentSegment( const pod::Vector3f& A, const pod::Vector3f& B, const pod::Vector3f& C, const pod::Vector3f& D, float eps = EPS(1e-6f) ) {
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

		if ( Dd < eps ) {
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

		sc = ( fabs(sN) < eps ) ? 0 : ( sN / sD );
		tc = ( fabs(tN) < eps ) ? 0 : ( tN / tD );

		return { A + u * sc, C + v * tc };
	}

	pod::Vector3f closestPointSegmentAabb( const pod::Vector3f& p1, const pod::Vector3f& p2, const pod::AABB& box, float eps = EPS(1e-6f) ) {
		// AABB center and half extents
		auto c = ( box.min + box.max ) * 0.5f;
		auto e = ( box.max - box.min ) * 0.5f;

		// direction of line segment
		auto d = p2 - p1;
		float len2 = uf::vector::dot( d, d );
		float t = 0.0f;

		if ( len2 > eps ) {
			// parametric closest t from box center
			t = uf::vector::dot( c - p1, d ) / len2;
			t = std::max(0.0f, std::min(1.0f, t));
		}

		// closest point on segment to box center
		auto segClosest = p1 + d * t;

		// clamp this point into AABB
		return {
			std::max(box.min.x, std::min(segClosest.x, box.max.x)),
			std::max(box.min.y, std::min(segClosest.y, box.max.y)),
			std::max(box.min.z, std::min(segClosest.z, box.max.z)),
		};
	}

	pod::Vector3f computeBarycentric( const pod::Vector3f& p, const pod::Vector3f& a, const pod::Vector3f& b, const pod::Vector3f& c, bool clamps = false ) {
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
			float distAB = uf::vector::dot( pAB, pAB );

			// check against AC
			float t2 = std::clamp(uf::vector::dot( ao, ac ) / d11, 0.0f, 1.0f);
			auto pAC = a + (ac * t2);
			float distAC = uf::vector::dot( pAC, pAC );

			// check against BC
			auto bc = c - b;
			float d22 = uf::vector::dot( bc, bc );
			float t3 = std::clamp(uf::vector::dot( -b, bc ) / d22, 0.0f, 1.0f);
			auto pBC = b + ( bc * t3 );
			float distBC = uf::vector::dot( pBC, pBC );

			// pick closest edge/vertex
			if ( distAB <= distAC && distAB <= distBC ) return { 1.0f - t, t, 0.0f };
			if ( distAC <= distBC ) return { 1.0f - t2, 0.0f, t2 };
			return { 0.0f, 1.0f - t3, t3 };
		}
		
		return { u, v, w };
	}
	// made a huge mess of probably mixing up arguments................
	pod::Vector3f computeBarycentric(const pod::Vector3f& p, const pod::Triangle& tri, bool clamps = false ) {
		return ::computeBarycentric( p, tri.points[0], tri.points[1], tri.points[2], clamps );
	}
	pod::Vector3f interpolateWithBarycentric( const pod::Vector3f& bary, const pod::Vector3f a, const pod::Vector3f b, const pod::Vector3f c ) {
		return a * bary.x + b * bary.y + c * bary.z;
	}
	pod::Vector3f interpolateWithBarycentric( const pod::Vector3f& bary, const pod::Vector3f points[3] ) {
		return ::interpolateWithBarycentric( bary, points[0], points[1], points[2] );
	}

	pod::Vector3f closestPointOnTriangle( const pod::Vector3f& p, const pod::Vector3f& a, const pod::Vector3f& b, const pod::Vector3f& c ) {
		auto bary = ::computeBarycentric( p, a, b, c );
		return ::interpolateWithBarycentric( bary, a, b, c );
	}
	pod::Vector3f closestPointOnTriangle( const pod::Vector3f& p, const pod::Triangle& tri ) {
		return ::closestPointOnTriangle( p, tri.points[0], tri.points[1], tri.points[2] );
	}


	pod::Vector3f orientNormalToAB( const pod::RigidBody& a, const pod::RigidBody& b, pod::Vector3f n ) {
		return uf::vector::normalize( uf::vector::dot( n, ::getPosition( b ) - ::getPosition( a ) ) < 0.0f ? -n : n );
	}

	float segmentTriangleDistanceSq( const pod::Vector3f& p0, const pod::Vector3f& p1, const pod::Triangle& tri, pod::Vector3f& outSeg, pod::Vector3f& outTri ) {
		// segment vs plane
		auto n = uf::vector::normalize( uf::vector::cross( tri.points[1] - tri.points[0], tri.points[2]-tri.points[0] ) );
		float d0 = uf::vector::dot( p0 - tri.points[0], n );
		float d1 = uf::vector::dot( p1 - tri.points[0], n );
		
		// intersects plane
		if ( ( d0 * d1 ) <= 0 ) {
			float t = d0 / ( d0-d1 );
			auto p = p0 + ( p1 - p0 ) * t;
			auto q = ::closestPointOnTriangle( p, tri );
			outSeg = p; outTri = q;
			return uf::vector::magnitude( p - q );
		}

		// otherwise check endpoints against triangle
		auto q0 = ::closestPointOnTriangle( p0, tri );
		auto q1 = ::closestPointOnTriangle( p1, tri );
		float d0sq = uf::vector::magnitude( p0 - q0 );
		float d1sq = uf::vector::magnitude( p1 - q1 );

		if ( d0sq < d1sq ) { outSeg = p0; outTri = q0; return d0sq; }
		else { outSeg = p1; outTri = q1; return d1sq; }
	}

	bool triangleTriangleIntersect( const pod::Triangle& a, const pod::Triangle& b, float eps = EPS(1e-6f) ) {
		auto boxA = ::computeTriangleAABB( a );
		auto boxB = ::computeTriangleAABB( b );

		if ( !aabbOverlap( boxA, boxB ) ) return false;

		// check vertices of a inside b or vice versa
		for ( auto i = 0; i < 3; i++ ) {
			auto q = ::closestPointOnTriangle( a.points[i], b );
			if ( uf::vector::magnitude( q - a.points[i] ) < eps ) return true;
		}
		for ( auto i = 0; i < 3; i++ ) {
			auto q = ::closestPointOnTriangle( b.points[i], a );
			if ( uf::vector::magnitude( q - b.points[i] ) < eps ) return true;
		}
		return false;
	}
}