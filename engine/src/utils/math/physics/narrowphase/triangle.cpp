#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/narrowphase.h>

namespace impl {
	bool triangleGeneric( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
		const auto& tri = a;
		const auto& body = b;

		pod::Simplex simplex;
		if ( !impl::gjk( tri, body, simplex ) ) return false;
		auto result = impl::epa( tri, body, simplex );
		if ( !impl::generateClippingManifold( tri, body, result, manifold ) ) return false;
		return true;
	}
	
	bool triangleGeneric( const pod::TriangleWithNormal& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
		auto tri = impl::physicsBodyTriView( a );
		const auto& body = b;

		return triangleGeneric( tri, body, manifold );
	}
}

bool impl::triangleTriangle( const pod::TriangleWithNormal& a, const pod::TriangleWithNormal& b, pod::Manifold& manifold ) {
	if ( uf::physics::settings.useGjk ) {
		auto A = impl::physicsBodyTriView( a );
		auto B = impl::physicsBodyTriView( b );

		return impl::triangleGeneric( A, B, manifold );
	}

	size_t axesCount = 0;
	pod::Vector3f axes[14]; // 2 normals + 9 edge cross product
	axes[axesCount++] = impl::triangleNormal(a);
	axes[axesCount++] = impl::triangleNormal(b);

	for ( auto i = 0; i < 3; i++ ) {
		auto ea = a.points[(i+1)%3] - a.points[i];
		for ( auto j = 0; j < 3; j++ ) {
			auto eb = b.points[(j+1)%3] - b.points[j];
			auto axis = uf::vector::cross(ea, eb);
			if ( uf::vector::magnitude( axis ) > EPS2 ) axes[axesCount++] = uf::vector::normalize(axis);
		}
	}

	// SAT test
	float minOverlap = FLT_MAX;
	pod::Vector3f bestAxis;

	for ( auto i = 0; i < axesCount; i++ ) {
		const auto& axis = axes[i];
		auto nA = uf::vector::normalize( axis );
		auto pA = pod::Vector3f{
			uf::vector::dot( a.points[0], nA ),
			uf::vector::dot( a.points[1], nA ),
			uf::vector::dot( a.points[2], nA )
		};

		auto pB = pod::Vector3f{
			uf::vector::dot( b.points[0], nA ),
			uf::vector::dot( b.points[1], nA ),
			uf::vector::dot( b.points[2], nA )
		};

		auto projA = pod::Vector2f{ uf::vector::min( pA ), uf::vector::max( pA ) };
		auto projB = pod::Vector2f{ uf::vector::min( pB ), uf::vector::max( pB ) };

		float overlap = std::min(projA.y, projB.y) - std::max(projA.x, projB.x);
		if ( overlap < 0 ) return false;

		if ( overlap < minOverlap ) {
			minOverlap = overlap;
			bestAxis = axis;
		}
	}

	if ( uf::vector::dot(bestAxis, impl::triangleCenter(b) - impl::triangleCenter(a)) < 0.0f ) bestAxis = -bestAxis;

	auto nA = impl::triangleNormal( a );
	auto nB = impl::triangleNormal( b );

	bool isAReference = std::abs( uf::vector::dot( bestAxis, nA ) ) >= std::abs( uf::vector::dot( bestAxis, nB ) );

	const auto& refTri = isAReference ? a : b;
	const auto& incTri = isAReference ? b : a;
	auto refNormal = impl::triangleNormal( refTri );

	int polyCount = 3;
	pod::Vector3f poly[8];
	poly[0] = incTri.points[0];
	poly[1] = incTri.points[1];
	poly[2] = incTri.points[2];

	for ( auto i = 0; i < 3; i++ ) {
		auto p0 = refTri.points[i];
		auto p1 = refTri.points[(i+1)%3];
		auto edge = p1 - p0;

		//auto edgeNormal = uf::vector::normalize( uf::vector::cross( refNormal, edge ) );
		auto edgeNormal = uf::vector::normalize( uf::vector::cross( edge, refNormal ) );
		impl::clipPolygon( poly, polyCount, pod::Plane{ edgeNormal, uf::vector::dot(edgeNormal, p0) } );
		if ( polyCount == 0 ) return false;
	}

	float refOffset = uf::vector::dot(bestAxis, refTri.points[0]);

	// build manifold
	for ( auto i = 0; i < polyCount; i++ ) {
	#if 1
		float pointProj = uf::vector::dot(bestAxis, poly[i]);
		float penetration = isAReference ? (pointProj - refOffset) : (refOffset - pointProj);
	#else
		float dist = uf::vector::dot(poly[i], refNormal) - uf::vector::dot(refNormal, refTri.points[0]);
		float penetration = -dist;
	#endif

		manifold.points.emplace_back(pod::Contact{ poly[i], bestAxis, penetration });
	}

	return ( polyCount > 0 );
}

bool impl::triangleAabb( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold ) {
	if ( uf::physics::settings.useGjk ) return impl::triangleGeneric( tri, body, manifold );

	const auto& box = body.bounds;
	pod::Vector3f axes[3] = { {1,0,0}, {0,1,0}, {0,0,1} };

	float minOverlap = FLT_MAX;
	pod::Vector3f bestAxis;
	if ( !impl::testSeparatingAxis( tri, box, tri.normal, axes, minOverlap, bestAxis ) ) return false;
	if ( !impl::testSeparatingAxis( tri, box, axes[0], axes, minOverlap, bestAxis ) ) return false;
	if ( !impl::testSeparatingAxis( tri, box, axes[1], axes, minOverlap, bestAxis ) ) return false;
	if ( !impl::testSeparatingAxis( tri, box, axes[2], axes, minOverlap, bestAxis ) ) return false;

	// test edges (3 triangle edges crossed with 3 AABB axes)
	pod::Vector3f edges[3] = {
		tri.points[1] - tri.points[0],
		tri.points[2] - tri.points[1],
		tri.points[0] - tri.points[2]
	};
	for ( int i = 0; i < 3; i++ ) {
		for ( int j = 0; j < 3; j++ ) {
			auto axis = uf::vector::cross(edges[i], axes[j]);
			if ( !impl::testSeparatingAxis( tri, box, axis, axes, minOverlap, bestAxis ) ) return false;
		}
	}

	auto cB = impl::aabbCenter(box);
	auto eB = impl::aabbExtent(box);
	auto cT = impl::triangleCenter(tri);
	if ( uf::vector::dot( bestAxis, cB - cT ) < 0.0f ) bestAxis = -bestAxis;

	int polyCount = 3;
	pod::Vector3f poly[8];
	poly[0] = tri.points[0];
	poly[1] = tri.points[1];
	poly[2] = tri.points[2];

	impl::clipPolygon( poly, polyCount, pod::Plane{ { 1,  0,  0}, eB.x + cB.x} );
	impl::clipPolygon( poly, polyCount, pod::Plane{ {-1,  0,  0}, eB.x - cB.x} );
	impl::clipPolygon( poly, polyCount, pod::Plane{ { 0,  1,  0}, eB.y + cB.y} );
	impl::clipPolygon( poly, polyCount, pod::Plane{ { 0, -1,  0}, eB.y - cB.y} );
	impl::clipPolygon( poly, polyCount, pod::Plane{ { 0,  0,  1}, eB.z + cB.z} );
	impl::clipPolygon( poly, polyCount, pod::Plane{ { 0,  0, -1}, eB.z - cB.z} );

	if ( polyCount == 0 ) return false;

	pod::Vector3f boxSupport = cB;
	boxSupport.x -= std::copysign(eB.x, bestAxis.x);
	boxSupport.y -= std::copysign(eB.y, bestAxis.y);
	boxSupport.z -= std::copysign(eB.z, bestAxis.z);

	float referenceOffset = uf::vector::dot(bestAxis, boxSupport);

	for ( auto i = 0; i < polyCount; i++ ) {
		float pointProjection = uf::vector::dot(bestAxis, poly[i]);
		float penetration = pointProjection - referenceOffset;

		manifold.points.emplace_back( pod::Contact{ poly[i], bestAxis, penetration } );
	}

	return true;
}
bool impl::triangleObb( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold ) {
	if ( uf::physics::settings.useGjk ) return impl::triangleGeneric( tri, body, manifold );

	const auto& box = body.bounds;
	auto tB = impl::getTransform( body );
	pod::Vector3f axes[3] = {
		uf::quaternion::rotate(tB.orientation, pod::Vector3f{1,0,0}),
		uf::quaternion::rotate(tB.orientation, pod::Vector3f{0,1,0}),
		uf::quaternion::rotate(tB.orientation, pod::Vector3f{0,0,1})
	};

	float minOverlap = FLT_MAX;
	pod::Vector3f bestAxis;

	if ( !impl::testSeparatingAxis( tri, box, tri.normal, axes, minOverlap, bestAxis ) ) return false;
	if ( !impl::testSeparatingAxis( tri, box, axes[0], axes, minOverlap, bestAxis ) ) return false;
	if ( !impl::testSeparatingAxis( tri, box, axes[1], axes, minOverlap, bestAxis ) ) return false;
	if ( !impl::testSeparatingAxis( tri, box, axes[2], axes, minOverlap, bestAxis ) ) return false;

	pod::Vector3f edges[3] = {
		tri.points[1] - tri.points[0],
		tri.points[2] - tri.points[1],
		tri.points[0] - tri.points[2]
	};
	for ( auto i = 0; i < 3; i++ ) {
		for ( auto j = 0; j < 3; j++ ) {
			pod::Vector3f axis = uf::vector::cross( edges[i], axes[j] );
			if ( !impl::testSeparatingAxis( tri, box, axis, axes, minOverlap, bestAxis ) ) return false;
		}
	}

	auto cB = impl::aabbCenter(box);
	auto eB = impl::aabbExtent(box);
	auto cT = impl::triangleCenter(tri);
	if ( uf::vector::dot( bestAxis, cB - cT ) < 0.0f ) bestAxis = -bestAxis;

	int polyCount = 3;
	pod::Vector3f poly[8];
	poly[0] = tri.points[0];
	poly[1] = tri.points[1];
	poly[2] = tri.points[2];

	impl::clipPolygon( poly, polyCount, pod::Plane{  axes[0], eB.x + uf::vector::dot(axes[0], cB) });
	impl::clipPolygon( poly, polyCount, pod::Plane{ -axes[0], eB.x - uf::vector::dot(axes[0], cB) });
	impl::clipPolygon( poly, polyCount, pod::Plane{  axes[1], eB.y + uf::vector::dot(axes[1], cB) });
	impl::clipPolygon( poly, polyCount, pod::Plane{ -axes[1], eB.y - uf::vector::dot(axes[1], cB) });
	impl::clipPolygon( poly, polyCount, pod::Plane{  axes[2], eB.z + uf::vector::dot(axes[2], cB) });
	impl::clipPolygon( poly, polyCount, pod::Plane{ -axes[2], eB.z - uf::vector::dot(axes[2], cB) });

	if ( polyCount == 0 ) return false;

	pod::Vector3f boxSupport = cB;
	boxSupport.x -= std::copysign(eB.x, bestAxis.x);
	boxSupport.y -= std::copysign(eB.y, bestAxis.y);
	boxSupport.z -= std::copysign(eB.z, bestAxis.z);

	float referenceOffset = uf::vector::dot(bestAxis, boxSupport);

	for ( auto i = 0; i < polyCount; i++ ) {
		float pointProjection = uf::vector::dot(bestAxis, poly[i]);
		float penetration = pointProjection - referenceOffset;

		manifold.points.emplace_back( pod::Contact{ poly[i], bestAxis, penetration } );
	}

	return true;
}
bool impl::triangleSphere( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold ) {
	if ( uf::physics::settings.useGjk ) return impl::triangleGeneric( tri, body, manifold );

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
bool impl::trianglePlane( const pod::TriangleWithNormal& tri, const pod::PhysicsBody& body, pod::Manifold& manifold ) {
	if ( uf::physics::settings.useGjk ) return impl::triangleGeneric( tri, body, manifold );

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
	if ( uf::physics::settings.useGjk ) return impl::triangleGeneric( tri, body, manifold );

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
	auto triView = impl::physicsBodyTriView( tri );

	return impl::triangleHull( triView, body, manifold );
}

bool impl::triangleTriangle( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( TRIANGLE, TRIANGLE );
	if ( uf::physics::settings.useGjk ) return impl::triangleGeneric( a, b, manifold );
	return impl::triangleTriangle( a.collider.triangle, b.collider.triangle, manifold );
}
bool impl::triangleAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( TRIANGLE, AABB );
	if ( uf::physics::settings.useGjk ) return impl::triangleGeneric( a, b, manifold );
	return impl::triangleAabb( a.collider.triangle, b, manifold );
}
bool impl::triangleObb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( TRIANGLE, OBB );
	if ( uf::physics::settings.useGjk ) return impl::triangleGeneric( a, b, manifold );
	return impl::triangleObb( a.collider.triangle, b, manifold );
}
bool impl::triangleSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( TRIANGLE, SPHERE );
	if ( uf::physics::settings.useGjk ) return impl::triangleGeneric( a, b, manifold );
	return impl::triangleSphere( a.collider.triangle, b, manifold );
}
bool impl::trianglePlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( TRIANGLE, PLANE );
	if ( uf::physics::settings.useGjk ) return impl::triangleGeneric( a, b, manifold );
	return impl::trianglePlane( a.collider.triangle, b, manifold );
}
bool impl::triangleCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( TRIANGLE, CAPSULE );
	if ( uf::physics::settings.useGjk ) return impl::triangleGeneric( a, b, manifold );
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

void impl::drawTriangle( const pod::PhysicsBody& body ) {
	const auto& tri = body.collider.triangle;
	auto transform = impl::getTransform(body);

	pod::Vector3f v0 = uf::transform::apply(transform, tri.points[0]);
	pod::Vector3f v1 = uf::transform::apply(transform, tri.points[1]);
	pod::Vector3f v2 = uf::transform::apply(transform, tri.points[2]);

	impl::addLine( v0, v1 );
	impl::addLine( v1, v2 );
	impl::addLine( v2, v0 );
}