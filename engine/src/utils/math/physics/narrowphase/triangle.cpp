#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/narrowphase.h>

bool impl::triangleTriangle( const pod::TriangleWithNormal& a, const pod::TriangleWithNormal& b, pod::Manifold& manifold ) {
	size_t axes = 0;
	pod::Vector3f axesBuffer[12];
	axesBuffer[axes++] = impl::triangleNormal(a);
	axesBuffer[axes++] = impl::triangleNormal(b);

	for (int i = 0; i < 3; i++) {
		auto ea = a.points[(i+1)%3] - a.points[i];
		for (int j = 0; j < 3; j++) {
			auto eb = b.points[(j+1)%3] - b.points[j];
			auto axis = uf::vector::cross(ea, eb);
			if ( uf::vector::magnitude( axis ) > EPS2 ) axesBuffer[axes++] = uf::vector::normalize(axis);
		}
	}

	// SAT test
	float minOverlap = FLT_MAX;
	pod::Vector3f bestAxis;

	for ( auto& axis : axesBuffer ) {
		auto projA = impl::projectTriangleOntoAxis(a, axis);
		auto projB = impl::projectTriangleOntoAxis(b, axis);

		float overlap = std::min(projA.y, projB.y) - std::max(projA.x, projB.x);
		if (overlap < 0) return false; // separating axis

		if (overlap < minOverlap) {
			minOverlap = overlap;
			bestAxis = axis;
		}
	}


	// clip polygons
	int polyCount = 0;
	pod::Vector3f poly[8];
	poly[polyCount++] = b.points[0];
	poly[polyCount++] = b.points[1];
	poly[polyCount++] = b.points[2];

	auto clipAgainstPlane = [&](const pod::Vector3f& n, const pod::Vector3f& p) {
		int outCount = 0;
		pod::Vector3f out[8];

		for ( auto i = 0; i < polyCount; i++ ) {
			auto curr = poly[i];
			auto prev = poly[(i+polyCount-1)%polyCount];
			float dCurr = uf::vector::dot(n, curr - p);
			float dPrev = uf::vector::dot(n, prev - p);

			if ( dCurr >= 0 ) {
				if ( dPrev < 0 ) {
					float t = dPrev / (dPrev - dCurr);
					out[outCount++] = prev + (curr - prev) * t;
				}
				out[outCount++] = curr;
			} else if ( dPrev >= 0 ) {
				float t = dPrev / (dPrev - dCurr);
				out[outCount++] = prev + (curr - prev) * t;
			}
		}
		// copy back
		polyCount = outCount;
		for ( auto i = 0; i < outCount; i++ ) poly[i] = out[i];
	};
	
	if ( uf::vector::dot(bestAxis, impl::triangleCenter(b) - impl::triangleCenter(a)) < 0.0f ) bestAxis = -bestAxis;

	for ( auto i = 0; i < 3; i++ ) {
		auto p0 = a.points[i];
		auto p1 = a.points[(i+1)%3];
		auto edge = p1 - p0;
		auto edgeNormal = uf::vector::normalize(uf::vector::cross(bestAxis, edge));
		clipAgainstPlane(edgeNormal, p0);
		if ( polyCount == 0 ) return false;
	}

	// build manifold
	float penetration = std::max( minOverlap, 0.05f ); // slop
	for (int i = 0; i < polyCount; i++) {
		manifold.points.emplace_back(pod::Contact{ poly[i], bestAxis, penetration });
	}

	return ( polyCount > 0 );
}

bool impl::triangleAabb( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold ) {
	const auto& aabb = body.bounds;

	// box center and half extents
	pod::Vector3f boxCenter = impl::aabbCenter( aabb );
	pod::Vector3f boxHalf   = impl::aabbExtent( aabb );

	// move triangle into box-local space
	pod::Vector3f v0 = tri.points[0] - boxCenter;
	pod::Vector3f v1 = tri.points[1] - boxCenter;
	pod::Vector3f v2 = tri.points[2] - boxCenter;

	// triangle edges
	pod::Vector3f e0 = v1 - v0;
	pod::Vector3f e1 = v2 - v1;
	pod::Vector3f e2 = v0 - v2;

	float minOverlap = FLT_MAX;
	pod::Vector3f bestAxis;

	auto testAxis = [&](const pod::Vector3f& axis) -> bool {
		if ( uf::vector::magnitude( axis ) < EPS2 ) return true; // skip degenerate

		pod::Vector3f n = uf::vector::normalize(axis);

		// project triangle
		float t0 = uf::vector::dot(v0, n);
		float t1 = uf::vector::dot(v1, n);
		float t2 = uf::vector::dot(v2, n);
		float triMin = std::min({t0, t1, t2});
		float triMax = std::max({t0, t1, t2});

		// project box (radius along axis)
		float r = boxHalf.x * fabs(n.x) + boxHalf.y * fabs(n.y) + boxHalf.z * fabs(n.z); // to-do: use boxHalf + uf::vector::abs( n ) or something

		// overlap test
		if ( triMin > r || triMax < -r ) return false; // separating axis

		// compute overlap depth
		float overlap = std::min(triMax + r, r - triMin);
		if ( overlap < minOverlap ) {
			minOverlap = overlap;
			bestAxis = n;
		}
		return true;
	};

	if ( !testAxis({1,0,0}) ) return false;
	if ( !testAxis({0,1,0}) ) return false;
	if ( !testAxis({0,0,1}) ) return false;
	if ( !testAxis(uf::vector::cross(e0, e1)) ) return false;

	pod::Vector3f boxAxes[3] = { {1,0,0}, {0,1,0}, {0,0,1} };
	pod::Vector3f edges[3]   = { e0, e1, e2 };
	for ( auto& edge : edges ) {
		for ( auto& axis : boxAxes ) if ( !testAxis(uf::vector::cross(edge, axis)) ) return false;
	}

	pod::Vector3f triNormal = uf::vector::normalize(uf::vector::cross(e0, e1));
	float planeDist = uf::vector::dot(triNormal, v0);
	if ( uf::vector::dot(bestAxis, triNormal) < 0.0f ) bestAxis = -bestAxis;
	pod::Vector3f contact = boxCenter - bestAxis * (boxHalf.x * fabs(bestAxis.x) + boxHalf.y * fabs(bestAxis.y) + boxHalf.z * fabs(bestAxis.z));

	manifold.points.emplace_back( pod::Contact{ contact, bestAxis, minOverlap } );
	return true;
}
bool impl::triangleSphere( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold ) {
	const auto& sphere = body;

	float r = sphere.collider.sphere.radius;
	auto center = impl::getPosition( sphere );
	auto closest = impl::closestPointOnTriangle( center, tri.points[0], tri.points[1], tri.points[2] );

	if ( !uf::vector::isValid( closest ) ) return false;

	// to-do: derive proper delta
	auto delta = center - closest;
	float dist2 = uf::vector::magnitude( delta );
	if ( dist2 > r * r ) return false;
	float dist = std::sqrt(dist2);

	auto triNormal = impl::triangleNormal( tri );
	auto contact = ( center + closest ) * 0.5f;
	auto normal = ( dist > EPS ) ? ( delta / dist ) : triNormal;
	float penetration = r - dist;
	if ( uf::vector::dot( normal, triNormal ) > 0.707f ) normal = triNormal;

#if REORIENT_NORMALS_ON_CONTACT
	if ( uf::vector::dot( normal, delta ) < 0.0f ) normal = -normal;
#endif

	manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
	return true;
}
// to-do: implement
bool impl::trianglePlane( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold ) {
	const auto& plane = body;
	auto normal = plane.collider.plane.normal;
	float d = plane.collider.plane.offset;

	bool hit = false;
	pod::Vector3f dist;
	FOR_EACH(3, {
		dist[i] = uf::vector::dot(normal, tri.points[i] ) - d;
	});

	// completely on one side
	bool allAbove = ( dist.x >  EPS && dist.y >  EPS && dist.z >  EPS );
	bool allBelow = ( dist.x < -EPS && dist.y < -EPS && dist.z < -EPS );
	if ( allAbove )
		return hit;

	if ( allBelow ) {
		hit = true;
		FOR_EACH(3, {
			float penetration = -dist[i];
			manifold.points.emplace_back(pod::Contact{tri.points[i], normal, -dist[i]});
		});
		return hit;
	}

	// points touching plane
	for ( auto i = 0; i < 3; i++ )
		if ( fabs( dist[i] ) <= EPS ) {
			hit = true;
			manifold.points.emplace_back(pod::Contact{ tri.points[i], normal, 0.0f });
		}

	// edges that cross plane
	for ( auto i = 0; i < 3; i++ ) {
		auto j = (i + 1) % 3;
		if ( ( dist[i] > 0 && dist[j] < 0 ) || ( dist[i] < 0 && dist[j] > 0 ) ) {
			hit = true;
			float t = dist[i] / ( dist[i] - dist[j] );
			auto contact = tri.points[i] + ( tri.points[j] - tri.points[i] ) * t;
			float penetration = -std::min( dist[i], dist[j] );
			manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
		}
	}
	return hit;
}
bool impl::triangleCapsule( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold ) {
	const auto& capsule = body;

	float r = capsule.collider.capsule.radius;
	auto [ p1, p2 ] = impl::getCapsuleSegment( capsule );
	auto bounds = impl::computeSegmentAABB( p1, p2, r );

	// to-do: derive proper delta
	pod::Vector3f closestSeg = {}, closest = {};
	float dist2 = impl::segmentTriangleDistanceSq( p1, p2, tri, closestSeg, closest );

	if ( !uf::vector::isValid( closest ) ) return false;
	if ( dist2 > r * r ) return false;
	float dist = std::sqrt( dist2 );
	auto delta = ( closestSeg - closest );

	// to-do: properly derive the contact information
	auto triNormal = impl::triangleNormal( tri );
	auto contact = closest; // ( closestSeg + closest ) * 0.5f;
	auto normal = ( dist > EPS ) ? ( delta / dist ) : triNormal;
	float penetration = r - dist;
	if ( uf::vector::dot( normal, triNormal ) > 0.707f ) normal = triNormal;

#if REORIENT_NORMALS_ON_CONTACT
	if ( uf::vector::dot( normal, delta ) < 0.0f ) normal = -normal;
#endif

	manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
	return true;
}
bool impl::triangleHull( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold ) {
	pod::PhysicsBody triView = {};
	triView.collider.type = pod::ShapeType::TRIANGLE;
	triView.collider.triangle = tri;

	return impl::triangleHull( triView, body, manifold );
}

bool impl::triangleTriangle( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( TRIANGLE, TRIANGLE );
	return impl::triangleTriangle( a.collider.triangle, b.collider.triangle, manifold );
}
bool impl::triangleAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( TRIANGLE, AABB );
	return impl::triangleAabb( a.collider.triangle, b, manifold );
}
bool impl::triangleSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( TRIANGLE, SPHERE );
	return impl::triangleSphere( a.collider.triangle, b, manifold );
}
bool impl::trianglePlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( TRIANGLE, PLANE );
	return impl::trianglePlane( a.collider.triangle, b, manifold );
}
bool impl::triangleCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( TRIANGLE, CAPSULE );
	return impl::triangleCapsule( a.collider.triangle, b, manifold );
}
bool impl::triangleHull( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( TRIANGLE, CONVEX_HULL );
	const auto& tri = a;
	const auto& hull = b;

	pod::Simplex simplex;
	if ( !impl::gjk( tri, hull, simplex ) ) return false;
	auto result = impl::epa( tri, hull, simplex );
	if ( !impl::generateClippingManifold( tri, hull, result, manifold ) ) return false;
	return true;
}