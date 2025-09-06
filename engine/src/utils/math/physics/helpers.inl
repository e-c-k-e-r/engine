// forward declare
namespace {
	bool aabbAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool aabbSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool aabbPlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool aabbCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool aabbMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	
	bool sphereSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool sphereAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool spherePlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool sphereCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool sphereMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	
	bool planeAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool planeSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool planePlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool planeCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool planeMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	
	bool capsuleCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool capsuleAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool capsulePlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool capsuleSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool capsuleMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );

	bool meshAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool meshSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool meshPlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool meshCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool meshMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );

	bool triangleTriangle( const pod::TriangleWithNormal& a, const pod::TriangleWithNormal& b, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool triangleAabb( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool triangleSphere( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool trianglePlane( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );
	bool triangleCapsule( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold, float eps = EPS(1.0e-6f) );

	// ugh
	pod::Vector3f aabbCenter( const pod::AABB& aabb );
	bool aabbOverlap( const pod::AABB& a, const pod::AABB& b, float eps = EPS(1.0e-6f) );
	pod::AABB computeTriangleAABB( const pod::Triangle& tri );

	void solveContacts( uf::stl::vector<pod::Manifold>& manifolds, float dt );

	int32_t flattenBVH( pod::BVH& bvh, int32_t nodeID );

	void traverseNodePair( const pod::BVH& bvh, int32_t leftID, int32_t rightID, pod::BVH::pairs_t& pairs );
	void traverseNodePair( const pod::BVH& a, int32_t nodeA, const pod::BVH& b, int32_t nodeB, pod::BVH::pairs_t& out );
	
	void queryBVH( const pod::BVH& bvh, const pod::AABB& bounds, uf::stl::vector<int32_t>& indices );
	void queryBVH( const pod::BVH& bvh, const pod::AABB& bounds, uf::stl::vector<int32_t>& indices, int32_t nodeID );

	void queryBVH( const pod::BVH& bvh, const pod::Ray& ray, uf::stl::vector<int32_t>& indices, float maxDist = FLT_MAX );
	void queryBVH( const pod::BVH& bvh, const pod::Ray& ray, uf::stl::vector<int32_t>& indices, int32_t nodeID, float maxDist = FLT_MAX );

	void queryFlatBVH( const pod::BVH&, const pod::AABB& bounds, uf::stl::vector<int32_t>& out );
	void queryFlatBVH( const pod::BVH&, const pod::Ray& ray, uf::stl::vector<int32_t>& out, float maxDist = FLT_MAX );

	void queryOverlaps( const pod::BVH& bvh, pod::BVH::pairs_t& outPairs );
	void queryOverlaps( const pod::BVH& bvhA, const pod::BVH& bvhB, pod::BVH::pairs_t& outPairs );
	
	void queryFlatOverlaps( const pod::BVH& bvh, pod::BVH::pairs_t& outPairs );
	void queryFlatOverlaps( const pod::BVH& bvhA, const pod::BVH& bvhB, pod::BVH::pairs_t& outPairs );
}

namespace {
	// create ID from pointers
	uint64_t makePairKey( const pod::PhysicsBody& a, const pod::PhysicsBody& b ) {
		auto idA = reinterpret_cast<uint64_t>(&a);
		auto idB = reinterpret_cast<uint64_t>(&b);
		if ( idA > idB ) std::swap(idA, idB); // ensure consistent order
		return (idA << 32) ^ idB;
	}

	// marks a body as asleep
	void wakeBody( pod::PhysicsBody& body ) {
		body.activity.awake = true;
		body.activity.sleepTimer = 0.0f;
	}
	void sleepBody( pod::PhysicsBody& body ) {
		body.activity.awake = false;
		body.velocity = pod::Vector3f{};
		body.angularVelocity = pod::Vector3f{};
	}
	void updateActivity( pod::PhysicsBody& body, float dt ) {
		// already asleep
		if ( !body.activity.awake ) return;

		// check if body is moving
		float linSpeed = uf::vector::norm( body.velocity );
		float angSpeed = uf::vector::norm( body.angularVelocity );

		// body is nearly still
		if ( linSpeed < pod::Activity::linearSleepEpsilon && angSpeed < pod::Activity::angularSleepEpsilon ) {
			body.activity.sleepTimer += dt;
			if ( body.activity.sleepTimer > pod::Activity::sleepThreshold ) ::sleepBody( body );
		}
		// body is moving, reset timer
		else ::wakeBody( body );
	}

	// returns an absolute transform while also allowing offsetting the collision body
	// to-do: find a succint way to explain this madness
	pod::Transform<> getTransform( const pod::PhysicsBody& body ) {
		pod::Transform<> t;
		t.position = body.offset;
		t.reference = body.transform;
		return uf::transform::flatten( t );
	}

	pod::Vector3f getPosition( const pod::PhysicsBody& body, bool useTransform = false ) {
		if ( !useTransform ) return ::aabbCenter( body.bounds );
		return ::getTransform( body ).position;
	}

	bool shouldCollide( const pod::Collider& a, const pod::Collider& b ) {
		return ( a.category & b.mask ) && ( b.category & a.mask );
	}
	bool shouldCollide( const pod::PhysicsBody& a, const pod::PhysicsBody& b ) {
	//	if ( a.isStatic && b.isStatic ) return false;
		return ::shouldCollide( a.collider, b.collider );
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

	bool pointInTriangle( const pod::Vector3f& p, const pod::Vector3f& a, const pod::Vector3f& b, const pod::Vector3f& c, float eps = EPS(1.0e-6f) ) {
		auto bary = ::computeBarycentric( p, a, b, c, false );
		return ( bary.x >= -eps && bary.y >= -eps && bary.z >= -eps );
	}
	bool pointInTriangle( const pod::Vector3f& p, const pod::Triangle& tri, float eps = EPS(1.0e-6f) ) {
		auto bary = ::computeBarycentric( p, tri, false );
		return ( bary.x >= -eps && bary.y >= -eps && bary.z >= -eps );
	}

	pod::Vector3f closestPointOnTriangle( const pod::Vector3f& p, const pod::Vector3f& a, const pod::Vector3f& b, const pod::Vector3f& c ) {
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
	pod::Vector3f closestPointOnTriangle( const pod::Vector3f& p, const pod::Triangle& tri ) {
		return ::closestPointOnTriangle( p, tri.points[0], tri.points[1], tri.points[2] );
	}

	pod::Vector3f orientNormalToAB( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Vector3f n ) {
		return uf::vector::normalize( uf::vector::dot( n, ::getPosition( b ) - ::getPosition( a ) ) < 0.0f ? -n : n );
	}

	float segmentTriangleDistanceSq( const pod::Vector3f& p0, const pod::Vector3f& p1, const pod::Triangle& tri, pod::Vector3f& outSeg, pod::Vector3f& outTri, float eps = EPS(1.0e-6f) ) {
		float best = std::numeric_limits<float>::max();

		auto n = uf::vector::cross( tri.points[1]-tri.points[0], tri.points[2]-tri.points[0] );
		float denom = uf::vector::dot( n, n );
		if ( denom > eps ) {
			n /= std::sqrt( denom );
			float d0 = uf::vector::dot( p0 - tri.points[0], n );
			float d1 = uf::vector::dot( p1 - tri.points[0], n );
			if ( (d0 * d1) <= 0.0f ) {
				float t = d0 / (d0 - d1);
				auto q = p0 + (p1 - p0) * t;
				if ( ::pointInTriangle( q, tri ) ) {
					outSeg = q;
					outTri = q;
					return 0.0f;
				}
			}
		}

		// segment endpoints to triangle
		for ( auto p : { p0, p1 } ) {
			auto q = ::closestPointOnTriangle( p, tri );
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
			auto [s,e] = ::closestSegmentSegment( p0, p1, tri.points[i], tri.points[j] );
			float d = uf::vector::distanceSquared( s, e );
			if ( d < best ) {
				best = d;
				outSeg = s;
				outTri = e;
			}
		}

		return best;
	}

	bool triangleTriangleIntersect( const pod::Triangle& a, const pod::Triangle& b, float eps = EPS(1e-6f) ) {
		auto boxA = ::computeTriangleAABB( a );
		auto boxB = ::computeTriangleAABB( b );

		if ( !::aabbOverlap( boxA, boxB ) ) return false;

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