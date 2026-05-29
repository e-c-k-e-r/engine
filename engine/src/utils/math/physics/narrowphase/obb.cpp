#include <uf/utils/math/physics/impl.h>
#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/narrowphase.h>

namespace impl {
	void getIncidentFace( const pod::OBB& obb, const pod::Vector3f* axes, const pod::Vector3f& normal, pod::Vector3f* outPoly ) {
		pod::Vector3f n = -normal;
		pod::Vector3f absN = uf::vector::abs(pod::Vector3f{
			uf::vector::dot(n, axes[0]),
			uf::vector::dot(n, axes[1]),
			uf::vector::dot(n, axes[2])
		});

		int maxAxis = 0;
		if ( absN.y > absN.x ) maxAxis = 1;
		if ( absN.z > absN.x && absN.z > absN.y ) maxAxis = 2;

		pod::Vector3f axis = axes[maxAxis];
		if ( uf::vector::dot(n, axis) < 0.0f ) axis = -axis;

		pod::Vector3f center = obb.center + axis * ((maxAxis == 0) ? obb.extent.x : (maxAxis == 1) ? obb.extent.y : obb.extent.z);

		int a1 = (maxAxis + 1) % 3;
		int a2 = (maxAxis + 2) % 3;

		float ext1 = (a1 == 0) ? obb.extent.x : (a1 == 1) ? obb.extent.y : obb.extent.z;
		float ext2 = (a2 == 0) ? obb.extent.x : (a2 == 1) ? obb.extent.y : obb.extent.z;

		outPoly[0] = center + axis[a1] * ext1 + axis[a2] * ext2;
		outPoly[1] = center - axis[a1] * ext1 + axis[a2] * ext2;
		outPoly[2] = center - axis[a1] * ext1 - axis[a2] * ext2;
		outPoly[3] = center + axis[a1] * ext1 - axis[a2] * ext2;
	}

	bool boxBox( const pod::OBB& boxA, const pod::OBB& boxB, const pod::Vector3f* axesA, const pod::Vector3f* axesB, pod::Manifold& manifold ) {
		float minOverlap = FLT_MAX;
		pod::Vector3f bestAxis;

		for ( int i = 0; i < 3; ++i ) {
			if ( !impl::testSeparatingAxis(boxA, boxB, axesA, axesB, axesA[i], minOverlap, bestAxis) ) return false;
			if ( !impl::testSeparatingAxis(boxA, boxB, axesA, axesB, axesB[i], minOverlap, bestAxis) ) return false;
		}

		for ( int i = 0; i < 3; ++i ) {
			for ( int j = 0; j < 3; j++ ) {
				pod::Vector3f axis = uf::vector::cross(axesA[i], axesB[j]);
				if ( !impl::testSeparatingAxis(boxA, boxB, axesA, axesB, axis, minOverlap, bestAxis) ) return false;
			}
		}

		if ( uf::vector::dot(bestAxis, boxB.center - boxA.center) < 0.0f ) bestAxis = -bestAxis;
		
	#if 1
		pod::Vector3f contactPoint = boxA.center + bestAxis * impl::projectExtents( boxA, bestAxis, axesA );
		manifold.points.emplace_back( pod::Contact{ contactPoint, bestAxis, minOverlap } );
	#else
		auto refBox = boxA;
		auto incBox = boxB;
		auto* refAxes = axesA;
		auto* incAxes = axesB;
		bool isARef = false;

		float maxDot = -1.0f;
		for ( int i = 0; i < 3; i++ ) {
			float dotA = std::fabs(uf::vector::dot(axesA[i], bestAxis));
			float dotB = std::fabs(uf::vector::dot(axesB[i], bestAxis));
			if ( dotA > maxDot ) { maxDot = dotA; isARef = true; }
			if ( dotB > maxDot ) { maxDot = dotB; isARef = false; }
		}

		if ( !isARef ) {
			refBox = boxB;
			incBox = boxA;
			refAxes = axesB;
			incAxes = axesA;
		}

		int polyCount = 4;
		pod::Vector3f poly[8];
		pod::Vector3f incNormal = isARef ? bestAxis : -bestAxis;
		impl::getIncidentFace( incBox, incAxes, incNormal, poly );

		int refAxisIdx = 0;
		float maxRefDot = -1.0f;
		for ( int i = 0; i < 3; i++ ) {
			float d = std::fabs(uf::vector::dot(refAxes[i], bestAxis));
			if ( d > maxRefDot ) { maxRefDot = d; refAxisIdx = i; }
		}

		pod::Vector3f refFaceNormal = refAxes[refAxisIdx];
		if ( uf::vector::dot(refFaceNormal, isARef ? bestAxis : -bestAxis ) < 0.0f) {
			refFaceNormal = -refFaceNormal;
		}

		int a1 = ( refAxisIdx + 1 ) % 3;
		int a2 = ( refAxisIdx + 2 ) % 3;
		float ext1 = ( a1 == 0 ) ? refBox.extent.x : ( a1 == 1 ) ? refBox.extent.y : refBox.extent.z;
		float ext2 = ( a2 == 0 ) ? refBox.extent.x : ( a2 == 1 ) ? refBox.extent.y : refBox.extent.z;

		impl::clipPolygon( poly, polyCount, pod::Plane{  refAxes[a1], ext1 + uf::vector::dot(refAxes[a1], refBox.center) });
		impl::clipPolygon( poly, polyCount, pod::Plane{ -refAxes[a1], ext1 - uf::vector::dot(refAxes[a1], refBox.center) });
		impl::clipPolygon( poly, polyCount, pod::Plane{  refAxes[a2], ext2 + uf::vector::dot(refAxes[a2], refBox.center) });
		impl::clipPolygon( poly, polyCount, pod::Plane{ -refAxes[a2], ext2 - uf::vector::dot(refAxes[a2], refBox.center) });

		if ( polyCount == 0 ) return false;

		float refExt = (refAxisIdx == 0) ? refBox.extent.x : (refAxisIdx == 1) ? refBox.extent.y : refBox.extent.z;
		pod::Vector3f refFaceCenter = refBox.center + refFaceNormal * refExt;
		float referenceOffset = uf::vector::dot(refFaceNormal, refFaceCenter);

		for ( auto i = 0; i < polyCount; i++ ) {
			float depth = referenceOffset - uf::vector::dot(refFaceNormal, poly[i]);
			if ( depth >= 0.0f ) {
				manifold.points.emplace_back( pod::Contact{ poly[i], bestAxis, depth } );
			}
		}
	#endif

		return manifold.points.size() > 0;
	}
}

bool impl::obbObb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( OBB, OBB );

	auto tA = impl::getTransform( a );
	auto tB = impl::getTransform( b );

	auto boxA = a.collider.obb;
	auto boxB = b.collider.obb;

	boxA.center = uf::transform::apply( tA, boxA.center );
	boxB.center = uf::transform::apply( tB, boxB.center );

	pod::Vector3f axesA[3];
	pod::Vector3f axesB[3];
	impl::boxAxes( axesA, tA );
	impl::boxAxes( axesB, tB );

	return impl::boxBox( boxA, boxB, axesA, axesB, manifold );
}


bool impl::obbAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( OBB, AABB );

	auto tA = impl::getTransform( a );
	auto tB = impl::getTransform( b );

	auto boxA = a.collider.obb;
	auto boxB = impl::aabbToObb( b.bounds );
	boxA.center = uf::transform::apply( tA, boxA.center );

	pod::Vector3f axesA[3];
	pod::Vector3f axesB[3];
	impl::boxAxes( axesA, tA );
	impl::boxAxes( axesB );

	return impl::boxBox( boxA, boxB, axesA, axesB, manifold );
}

bool impl::obbSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( OBB, SPHERE );

	auto tA = impl::getTransform( a );
	auto box = a.collider.obb;
	box.center = uf::transform::apply( tA, box.center );

	auto sphereCenter = impl::getPosition( b );
	float radius = b.collider.sphere.radius;

	auto localP = uf::transform::applyInverse( tA, sphereCenter ) - box.center;
	auto closestLocal = uf::vector::clamp( localP, -box.extent, box.extent );

	auto deltaLocal = localP - closestLocal;
	float distSq = uf::vector::dot( deltaLocal, deltaLocal );

	if ( distSq > radius * radius ) return false;

	auto closestWorld = uf::transform::apply( tA, closestLocal + box.center );
	float dist = std::sqrt( distSq );

	pod::Vector3f normal;
	float penetration;

	if ( dist < EPS ) {
		float minDist = FLT_MAX;
		int axis = 0;
		float sign = 1.0f;

		FOR_EACH(3, {
			float distToMax = box.extent[i] - localP[i];
			float distToMin = localP[i] - (-box.extent[i]);
			if (distToMax < minDist) { minDist = distToMax; axis = i; sign = 1.0f; }
			if (distToMin < minDist) { minDist = distToMin; axis = i; sign = -1.0f; }
		});

		pod::Vector3f localNormal = {0,0,0};
		localNormal[axis] = sign;
		normal = uf::quaternion::rotate( tA.orientation, localNormal );
		penetration = radius + minDist;
	} else {
		normal = uf::quaternion::rotate( tA.orientation, deltaLocal / dist );
		penetration = radius - dist;
	}

	manifold.points.emplace_back( pod::Contact{ closestWorld, normal, penetration } );
	return true;
}
bool impl::obbPlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( OBB, PLANE );

	auto tA = impl::getTransform( a );
	auto box = a.collider.obb;
	box.center = uf::transform::apply( tA, box.center );

	pod::Vector3f axesA[3];
	impl::boxAxes( axesA, tA );

	pod::Vector3f normal = b.collider.plane.normal;
	float offset = b.collider.plane.offset;

	float rA = box.extent.x * std::fabs(uf::vector::dot(axesA[0], normal)) +
			   box.extent.y * std::fabs(uf::vector::dot(axesA[1], normal)) +
			   box.extent.z * std::fabs(uf::vector::dot(axesA[2], normal));

	float dist = uf::vector::dot(box.center, normal) - offset;
	if ( dist > rA ) return false; // in front of plane

	pod::Vector3f deepestPoint = box.center
		- axesA[0] * box.extent.x * (uf::vector::dot(axesA[0], normal) > 0 ? 1.0f : -1.0f)
		- axesA[1] * box.extent.y * (uf::vector::dot(axesA[1], normal) > 0 ? 1.0f : -1.0f)
		- axesA[2] * box.extent.z * (uf::vector::dot(axesA[2], normal) > 0 ? 1.0f : -1.0f);

	float penetration = rA - dist;
	manifold.points.emplace_back( pod::Contact{ deepestPoint, normal, penetration } );
	return true;

}
bool impl::obbCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( OBB, CAPSULE );

	auto tA = impl::getTransform( a );
	auto box = a.collider.obb;
	box.center = uf::transform::apply( tA, box.center );

	pod::Vector3f axesA[3];
	impl::boxAxes( axesA, tA );

	auto [p1, p2] = impl::getCapsuleSegment( b );

	pod::Vector3f cB = (p1 + p2) * 0.5f;

	pod::Vector3f segmentHalf = (p2 - p1) * 0.5f;
	float halfHeight = uf::vector::norm(segmentHalf);

	pod::Vector3f capAxis = (halfHeight > EPS2) ? (segmentHalf / halfHeight) : pod::Vector3f{0, 1, 0};

	float radius = b.collider.capsule.radius;

	float minOverlap = FLT_MAX;
	pod::Vector3f bestAxis;

	auto testAxis = [&](const pod::Vector3f& axis) -> bool {
		float mag2 = uf::vector::magnitude(axis);
		if ( mag2 < EPS2 ) return true;
		pod::Vector3f n = axis / std::sqrt( mag2 );

		float pA = uf::vector::dot(box.center, n);
		float rA = impl::projectExtents( box, n, axesA );
		float pB = uf::vector::dot(cB, n);

		float rB = std::fabs(uf::vector::dot(segmentHalf, n)) + radius;

		float dist = std::fabs(pB - pA);
		float overlap = (rA + rB) - dist;

		if ( overlap < 0 ) return false;

		if ( overlap < minOverlap ) {
			minOverlap = overlap;
			bestAxis = n;
		}
		return true;
	};

	if ( !testAxis(capAxis) ) return false;
	for ( auto i = 0; i < 3; ++i ) if ( !testAxis(axesA[i]) ) return false;
	for ( auto i = 0; i < 3; ++i ) if ( !testAxis(uf::vector::cross(axesA[i], capAxis)) ) return false;

	if ( uf::vector::dot(bestAxis, cB - box.center) < 0.0f ) bestAxis = -bestAxis;

	pod::Vector3f contactPoint = cB - bestAxis * radius;

	manifold.points.emplace_back( pod::Contact{ contactPoint, bestAxis, minOverlap } );
	return true;
}
bool impl::obbMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( OBB, MESH );
	REVERSE_COLLIDER( a, b, impl::meshObb );
}
bool impl::obbHull( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( OBB, CONVEX_HULL );
	REVERSE_COLLIDER( a, b, impl::hullObb );
}

void impl::drawObb( const pod::PhysicsBody& body ) {
	const auto& obb = body.collider.obb;
	auto transform = impl::getTransform(body);	
#if 0
	uf::debug::drawObb( obb, transform );
#else
	auto aabb = pod::AABB{
		.min = obb.center - obb.extent,
		.max = obb.center + obb.extent,
	};
	pod::Vector3f corners[8] = {
		{aabb.min.x, aabb.min.y, aabb.min.z}, {aabb.max.x, aabb.min.y, aabb.min.z},
		{aabb.max.x, aabb.max.y, aabb.min.z}, {aabb.min.x, aabb.max.y, aabb.min.z},
		{aabb.min.x, aabb.min.y, aabb.max.z}, {aabb.max.x, aabb.min.y, aabb.max.z},
		{aabb.max.x, aabb.max.y, aabb.max.z}, {aabb.min.x, aabb.max.y, aabb.max.z}
	};

	FOR_EACH( 8, {
		corners[i] = uf::transform::apply(transform, corners[i]);
	});

	uf::debug::drawLine( corners[0], corners[1] ); uf::debug::drawLine( corners[1], corners[2] );
	uf::debug::drawLine( corners[2], corners[3] ); uf::debug::drawLine( corners[3], corners[0] );

	uf::debug::drawLine( corners[4], corners[5] ); uf::debug::drawLine( corners[5], corners[6] );
	uf::debug::drawLine( corners[6], corners[7] ); uf::debug::drawLine( corners[7], corners[4] );

	uf::debug::drawLine( corners[0], corners[4] ); uf::debug::drawLine( corners[1], corners[5] );
	uf::debug::drawLine( corners[2], corners[6] ); uf::debug::drawLine( corners[3], corners[7] );
#endif
}

pod::PhysicsBody& uf::physics::initialize( pod::PhysicsBody& body, const pod::OBB& obb ) {
	body.collider.type = pod::ShapeType::OBB;
	body.collider.obb = obb;
	body.bounds = impl::computeAABB( body );

	uf::physics::updateInertia( body );
	return body;
}