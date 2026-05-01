// to-do: clean this mess
#define REVERSE_COLLIDER( a, b, fun )\
	auto start = manifold.points.size();\
	if ( !::fun( b, a, manifold, eps ) ) return false;\
	for ( auto i = start; i < manifold.points.size(); ++i ) manifold.points[i].normal = -manifold.points[i].normal;\
	return true;

// forward declare (to-do: properly handle this)
namespace {
	bool aabbAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool aabbSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool aabbPlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool aabbCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool aabbMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool aabbHull( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	
	bool sphereSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool sphereAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool spherePlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool sphereCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool sphereMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool sphereHull( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	
	bool planeAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool planeSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool planePlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool planeCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool planeMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool planeHull( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	
	bool capsuleCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool capsuleAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool capsulePlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool capsuleSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool capsuleMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool capsuleHull( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );

	bool meshAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool meshSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool meshPlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool meshCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool meshMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool meshHull( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );

	bool triangleTriangle( const pod::TriangleWithNormal& a, const pod::TriangleWithNormal& b, pod::Manifold& manifold, float eps = EPS );
	bool triangleAabb( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold, float eps = EPS );
	bool triangleSphere( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold, float eps = EPS );
	bool trianglePlane( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold, float eps = EPS );
	bool triangleCapsule( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold, float eps = EPS );

	bool hullAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool hullSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool hullPlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool hullCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool hullMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );
	bool hullHull( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps = EPS );

	// ugh
	pod::AABB computeAABB( const pod::PhysicsBody& body );
	FORCE_INLINE bool aabbOverlap( const pod::AABB& a, const pod::AABB& b, float eps = EPS );
	pod::Vector3f aabbCenter( const pod::AABB& aabb );
	pod::Vector3f getVertex( const uf::Mesh::View& view, const uf::Mesh::AttributeView& positions, size_t index );
	pod::TriangleWithNormal fetchTriangle( const uf::Mesh& mesh, size_t triID, const pod::PhysicsBody& body );

	// to-do: define maxIterations as a setting
	bool gjk( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Simplex& simplex, int maxIterations = 20, float eps = EPS );
	bool gjk( const pod::Ray& ray, const pod::PhysicsBody& body, float maxDist, float& outT, pod::Vector3f& outNormal, float eps = EPS );
	pod::Contact epa( const pod::PhysicsBody& a, const pod::PhysicsBody& b, const pod::Simplex& simplex, uint32_t maxIterations = 64, float eps = EPS );
	bool generateClippingManifold( const pod::PhysicsBody& a, const pod::PhysicsBody& b, const pod::Contact& contact, pod::Manifold& manifold );

	void queryBVH( const pod::BVH& bvh, const pod::AABB& bounds, uf::stl::vector<pod::BVH::index_t>& indices );
	void queryBVH( const pod::BVH& bvh, const pod::AABB& bounds, uf::stl::vector<pod::BVH::index_t>& indices, pod::BVH::index_t nodeID );
	void queryBVH( const pod::BVH& bvh, const pod::Ray& ray, uf::stl::vector<pod::BVH::index_t>& indices, float maxDist = FLT_MAX );
	void queryBVH( const pod::BVH& bvh, const pod::Ray& ray, uf::stl::vector<pod::BVH::index_t>& indices, pod::BVH::index_t nodeID, float maxDist = FLT_MAX );
	void queryOverlaps( const pod::BVH& bvh, pod::BVH::pairs_t& outPairs );
	void queryOverlaps( const pod::BVH& bvhA, const pod::BVH& bvhB, pod::BVH::pairs_t& outPairs );
	void queryOverlaps( const pod::BVH& bvhA, const pod::BVH& bvhB, const pod::Transform<>& relTransform, pod::BVH::pairs_t& outPairs );
}

namespace {
	// create ID from pointers
	uint64_t makePairKey( const pod::PhysicsBody& a, const pod::PhysicsBody& b ) {
		uint64_t lhs = reinterpret_cast<uint64_t>(&a);
		uint64_t rhs = reinterpret_cast<uint64_t>(&b);
		if (lhs > rhs) std::swap(lhs, rhs);
		size_t seed = 0;
		seed ^= std::hash<uint64_t>{}(lhs) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		seed ^= std::hash<uint64_t>{}(rhs) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}

	// marks a body as asleep
	void wakeBody( pod::PhysicsBody& body ) {
		bool wasAwake = body.activity.awake;
		if ( !wasAwake ) {
			body.activity.sleepTimer = 0.0f;
		}

		body.activity.awake = true;
	}
	void sleepBody( pod::PhysicsBody& body ) {
		bool wasAsleep = !body.activity.awake;

		body.activity.awake = false;
		body.velocity = pod::Vector3f{};
		body.angularVelocity = pod::Vector3f{};
	}
	void updateActivity( pod::PhysicsBody& body, float dt ) {
		// reset grounded state
		bool wasGrounded = body.activity.grounded;
		body.activity.grounded = false;

		// update bounds
		body.bounds = ::computeAABB( body );

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
			if ( body.activity.sleepTimer > threshold ) ::sleepBody( body );
		}
		// body is moving, reset timer
		else ::wakeBody( body );
	}

	// returns an absolute transform while also allowing offsetting the collision body
	// to-do: find a succinct way to explain this madness
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

	pod::PhysicsBody physicsBodyHullView( const pod::PhysicsBody& body, int32_t index = -1 ) {
		pod::PhysicsBody view = body;
		view.viewIndex = index;
		return view;
	}

	pod::PhysicsBody physicsBodyTriView( const pod::PhysicsBody& body, const pod::TriangleWithNormal triangle ) {
		pod::PhysicsBody view = body;
		view.collider.type = pod::ShapeType::TRIANGLE;
		view.collider.triangle = triangle;
		// assume triangle is already transformed
		view.offset = {};
		view.transform = NULL;
		return view;
	}
	pod::PhysicsBody physicsBodyTriView( const pod::PhysicsBody& body, size_t triID ) {
		auto tri = ::fetchTriangle( *body.collider.mesh.mesh, triID, body );
		return ::physicsBodyTriView( body, tri );
	}

	bool shouldCollide( const pod::Collider& a, const pod::Collider& b ) {
		return ( a.category & b.mask ) && ( b.category & a.mask );
	}
	bool shouldCollide( const pod::PhysicsBody& a, const pod::PhysicsBody& b ) {
		if ( a.isStatic && b.isStatic ) return false; // this shouldn't ever happen if we're segregating static bodies from dynamic bodies in the broadphase
		return ::shouldCollide( a.collider, b.collider );
	}

	pod::Matrix3f computeWorldInverseInertia( const pod::PhysicsBody& b ) {
		if ( b.isStatic || b.inverseMass == 0.0f ) return pod::Matrix3f{};

		pod::Matrix3f invI_local = uf::matrix::diagonal( b.inverseInertiaTensor );
		pod::Matrix3f R = uf::quaternion::matrix3(b.transform->orientation);

		return R * invI_local * uf::matrix::transpose(R);
	}

	// normalizes the delta between two bodies / contacts by the distance (as it was already computed) if non-zero
	// a lot of collider v colliders use this semantic
	pod::Vector3f normalizeDelta( const pod::Vector3f& delta, float dist, float eps = EPS, const pod::Vector3f& fallback = pod::Vector3f{0,1,0} ) {
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
		t = std::clamp( t, 0.0f, 1.0f );
		return a + ab * t;
	}

	std::pair<pod::Vector3f, pod::Vector3f> closestSegmentSegment( const pod::Vector3f& A, const pod::Vector3f& B, const pod::Vector3f& C, const pod::Vector3f& D, float eps = EPS ) {
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

	pod::Vector3f closestPointSegmentAabb( const pod::Vector3f& p1, const pod::Vector3f& p2, const pod::AABB& box, float eps = EPS ) {
		const float eps2 = eps * eps;
		// AABB center and half extents
		auto c = ( box.min + box.max ) * 0.5f;
		auto e = ( box.max - box.min ) * 0.5f;

		// direction of line segment
		auto d = p2 - p1;
		float len2 = uf::vector::magnitude( d );
		float t = 0.0f;

		if ( len2 > eps2 ) {
			// parametric closest t from box center
			t = uf::vector::dot( c - p1, d ) / len2;
			t = std::clamp( t, 0.0f, 1.0f );
		}

		// closest point on segment to box center
		auto segClosest = p1 + d * t;

		// clamp this point into AABB
		return uf::vector::clamp( segClosest, box.min, box.max );
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
	pod::Vector3f computeBarycentric(const pod::Vector3f& p, const pod::Triangle& tri, bool clamps = false ) {
		return ::computeBarycentric( p, tri.points[0], tri.points[1], tri.points[2], clamps );
	}
	pod::Vector3f interpolateWithBarycentric( const pod::Vector3f& bary, const pod::Vector3f a, const pod::Vector3f b, const pod::Vector3f c ) {
		return a * bary.x + b * bary.y + c * bary.z;
	}
	pod::Vector3f interpolateWithBarycentric( const pod::Vector3f& bary, const pod::Vector3f points[3] ) {
		return ::interpolateWithBarycentric( bary, points[0], points[1], points[2] );
	}

	bool pointInTriangle( const pod::Vector3f& p, const pod::Vector3f& a, const pod::Vector3f& b, const pod::Vector3f& c, float eps = EPS ) {
		auto bary = ::computeBarycentric( p, a, b, c, false );
		return ( bary.x >= -eps && bary.y >= -eps && bary.z >= -eps );
	}
	bool pointInTriangle( const pod::Vector3f& p, const pod::Triangle& tri, float eps = EPS ) {
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

	float segmentTriangleDistanceSq( const pod::Vector3f& p0, const pod::Vector3f& p1, const pod::Triangle& tri, pod::Vector3f& outSeg, pod::Vector3f& outTri, float eps = EPS ) {
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

	int clipPolygonAgainstPlane( const pod::Vector3f* inPoly, int inCount, const pod::Vector3f& planeNormal, float planeOffset, pod::Vector3f* outPoly ) {
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
}
