#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/narrowphase.h>

bool impl::obbObb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( OBB, OBB );

	auto tA = impl::getTransform( a );
	auto tB = impl::getTransform( b );

	pod::Vector3f cA = uf::transform::apply( tA, (a.collider.obb.max + a.collider.obb.min) * 0.5f );
	pod::Vector3f cB = uf::transform::apply( tB, (b.collider.obb.max + b.collider.obb.min) * 0.5f );
	pod::Vector3f eA = (a.collider.obb.max - a.collider.obb.min) * 0.5f;
	pod::Vector3f eB = (b.collider.obb.max - b.collider.obb.min) * 0.5f;

	pod::Vector3f axesA[3] = {
		uf::quaternion::rotate(tA.orientation, pod::Vector3f{1,0,0}),
		uf::quaternion::rotate(tA.orientation, pod::Vector3f{0,1,0}),
		uf::quaternion::rotate(tA.orientation, pod::Vector3f{0,0,1})
	};
	pod::Vector3f axesB[3] = {
		uf::quaternion::rotate(tB.orientation, pod::Vector3f{1,0,0}),
		uf::quaternion::rotate(tB.orientation, pod::Vector3f{0,1,0}),
		uf::quaternion::rotate(tB.orientation, pod::Vector3f{0,0,1})
	};

	float minOverlap = FLT_MAX;
	pod::Vector3f bestAxis;

	auto testAxis = [&](const pod::Vector3f& axis) -> bool {
		float mag = uf::vector::magnitude(axis);
		if (mag < EPS) return true;
		pod::Vector3f n = axis / mag;

		float pA = uf::vector::dot(cA, n);
		float rA = eA.x * std::fabs(uf::vector::dot(axesA[0], n)) +
				   eA.y * std::fabs(uf::vector::dot(axesA[1], n)) +
				   eA.z * std::fabs(uf::vector::dot(axesA[2], n));

		float pB = uf::vector::dot(cB, n);
		float rB = eB.x * std::fabs(uf::vector::dot(axesB[0], n)) +
				   eB.y * std::fabs(uf::vector::dot(axesB[1], n)) +
				   eB.z * std::fabs(uf::vector::dot(axesB[2], n));

		float dist = std::fabs(pB - pA);
		float overlap = (rA + rB) - dist;

		if ( overlap < 0) return false;

		if ( overlap < minOverlap ) {
			minOverlap = overlap;
			bestAxis = n;
		}
		return true;
	};

	for ( auto i = 0; i < 3; ++i ) if ( !testAxis(axesA[i]) ) return false;
	for ( auto i = 0; i < 3; ++i ) if ( !testAxis(axesB[i]) ) return false;
	for ( auto i = 0; i < 3; ++i ) {
		for ( auto  j = 0; j < 3; j++ ) if ( !testAxis(uf::vector::cross(axesA[i], axesB[j])) ) return false;
	};

	if ( uf::vector::dot(bestAxis, cB - cA) < 0.0f ) bestAxis = -bestAxis;

	// to-do: generate contact face
	pod::Vector3f contactPoint = cA + bestAxis * (eA.x * std::fabs(uf::vector::dot(axesA[0], bestAxis)) +
												  eA.y * std::fabs(uf::vector::dot(axesA[1], bestAxis)) +
												  eA.z * std::fabs(uf::vector::dot(axesA[2], bestAxis)));

	manifold.points.emplace_back( pod::Contact{ contactPoint, bestAxis, minOverlap } );
	return true;
}


bool impl::obbAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( OBB, AABB );

	auto tA = impl::getTransform( a );

	pod::Vector3f cA = uf::transform::apply( tA, (a.collider.obb.max + a.collider.obb.min) * 0.5f );
	pod::Vector3f eA = (a.collider.obb.max - a.collider.obb.min) * 0.5f;
	pod::Vector3f axesA[3] = {
		uf::quaternion::rotate(tA.orientation, pod::Vector3f{1,0,0}),
		uf::quaternion::rotate(tA.orientation, pod::Vector3f{0,1,0}),
		uf::quaternion::rotate(tA.orientation, pod::Vector3f{0,0,1})
	};

	pod::Vector3f cB = impl::aabbCenter( b.bounds );
	pod::Vector3f eB = impl::aabbExtent( b.bounds );
	pod::Vector3f axesB[3] = { {1,0,0}, {0,1,0}, {0,0,1} };

	float minOverlap = FLT_MAX;
	pod::Vector3f bestAxis;

	auto testAxis = [&](const pod::Vector3f& axis) -> bool {
		float mag = uf::vector::magnitude(axis);
		if ( mag < EPS ) return true;
		pod::Vector3f n = axis / mag;

		float pA = uf::vector::dot(cA, n);
		float rA = eA.x * std::fabs(uf::vector::dot(axesA[0], n)) +
				   eA.y * std::fabs(uf::vector::dot(axesA[1], n)) +
				   eA.z * std::fabs(uf::vector::dot(axesA[2], n));

		float pB = uf::vector::dot(cB, n);
		float rB = eB.x * std::fabs(n.x) + eB.y * std::fabs(n.y) + eB.z * std::fabs(n.z);

		float dist = std::fabs(pB - pA);
		float overlap = (rA + rB) - dist;
		if ( overlap < 0 ) return false;
		if ( overlap < minOverlap ) {
			minOverlap = overlap;
			bestAxis = n;
		}
		return true;
	};

	for ( auto i = 0; i < 3; ++i ) if ( !testAxis(axesA[i]) ) return false;
	for ( auto i = 0; i < 3; ++i ) if ( !testAxis(axesB[i]) ) return false;
	for ( auto i = 0; i < 3; ++i ) {
		for ( auto  j = 0; j < 3; j++ ) if ( !testAxis(uf::vector::cross(axesA[i], axesB[j])) ) return false;
	};

	if ( uf::vector::dot(bestAxis, cB - cA) < 0.0f ) bestAxis = -bestAxis;

	pod::Vector3f contactPoint = cA + bestAxis * (eA.x * std::fabs(uf::vector::dot(axesA[0], bestAxis)) +
												  eA.y * std::fabs(uf::vector::dot(axesA[1], bestAxis)) +
												  eA.z * std::fabs(uf::vector::dot(axesA[2], bestAxis)));

	manifold.points.emplace_back( pod::Contact{ contactPoint, bestAxis, minOverlap } );
	return true;
}

bool impl::obbSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( OBB, SPHERE );

	auto tA = impl::getTransform( a );
	auto localCenter = ( a.collider.obb.max + a.collider.obb.min ) * 0.5f;
	auto extents = ( a.collider.obb.max - a.collider.obb.min ) * 0.5f;

	auto sphereCenter = impl::getPosition( b );
	float radius = b.collider.sphere.radius;

	auto localP = uf::transform::applyInverse( tA, sphereCenter ) - localCenter;
	auto closestLocal = uf::vector::clamp( localP, -extents, extents );

	auto deltaLocal = localP - closestLocal;
	float distSq = uf::vector::dot( deltaLocal, deltaLocal );

	if ( distSq > radius * radius ) return false;

	auto closestWorld = uf::transform::apply( tA, closestLocal + localCenter );
	float dist = std::sqrt( distSq );

	pod::Vector3f normal;
	float penetration;

	if ( dist < EPS ) {
		float minDist = FLT_MAX;
		int axis = 0;
		float sign = 1.0f;

		FOR_EACH(3, {
			float distToMax = extents[i] - localP[i];
			float distToMin = localP[i] - (-extents[i]);
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
	pod::Vector3f cA = uf::transform::apply( tA, (a.collider.obb.max + a.collider.obb.min) * 0.5f );
	pod::Vector3f eA = (a.collider.obb.max - a.collider.obb.min) * 0.5f;
	pod::Vector3f axesA[3] = {
		uf::quaternion::rotate(tA.orientation, pod::Vector3f{1,0,0}),
		uf::quaternion::rotate(tA.orientation, pod::Vector3f{0,1,0}),
		uf::quaternion::rotate(tA.orientation, pod::Vector3f{0,0,1})
	};

	pod::Vector3f normal = b.collider.plane.normal;
	float offset = b.collider.plane.offset;

	float rA = eA.x * std::fabs(uf::vector::dot(axesA[0], normal)) +
			   eA.y * std::fabs(uf::vector::dot(axesA[1], normal)) +
			   eA.z * std::fabs(uf::vector::dot(axesA[2], normal));

	float dist = uf::vector::dot(cA, normal) - offset;
	if ( dist > rA ) return false; // in front of plane

	pod::Vector3f deepestPoint = cA
		- axesA[0] * eA.x * (uf::vector::dot(axesA[0], normal) > 0 ? 1.0f : -1.0f)
		- axesA[1] * eA.y * (uf::vector::dot(axesA[1], normal) > 0 ? 1.0f : -1.0f)
		- axesA[2] * eA.z * (uf::vector::dot(axesA[2], normal) > 0 ? 1.0f : -1.0f);

	float penetration = rA - dist;
	manifold.points.emplace_back( pod::Contact{ deepestPoint, normal, penetration } );
	return true;

}
bool impl::obbCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold ) {
	ASSERT_COLLIDER_TYPES( OBB, CAPSULE );

	auto tA = impl::getTransform( a );
	pod::Vector3f cA = uf::transform::apply( tA, (a.collider.obb.max + a.collider.obb.min) * 0.5f );
	pod::Vector3f eA = (a.collider.obb.max - a.collider.obb.min) * 0.5f;
	pod::Vector3f axesA[3] = {
		uf::quaternion::rotate(tA.orientation, pod::Vector3f{1,0,0}),
		uf::quaternion::rotate(tA.orientation, pod::Vector3f{0,1,0}),
		uf::quaternion::rotate(tA.orientation, pod::Vector3f{0,0,1})
	};

	auto [p1, p2] = impl::getCapsuleSegment( b );
	pod::Vector3f cB = (p1 + p2) * 0.5f;
	pod::Vector3f capAxis = uf::vector::normalize(p2 - p1);
	float halfHeight = b.collider.capsule.halfHeight;
	float radius = b.collider.capsule.radius;

	float minOverlap = FLT_MAX;
	pod::Vector3f bestAxis;

	auto testAxis = [&](const pod::Vector3f& axis) -> bool {
		float mag = uf::vector::magnitude(axis);
		if (mag < EPS) return true;
		pod::Vector3f n = axis / mag;

		float pA = uf::vector::dot(cA, n);
		float rA = eA.x * std::fabs(uf::vector::dot(axesA[0], n)) +
				   eA.y * std::fabs(uf::vector::dot(axesA[1], n)) +
				   eA.z * std::fabs(uf::vector::dot(axesA[2], n));

		float pB = uf::vector::dot(cB, n);
		float rB = halfHeight * std::fabs(uf::vector::dot(capAxis, n)) + radius;

		float dist = std::fabs(pB - pA);
		float overlap = (rA + rB) - dist;

		if (overlap < 0) return false;

		if (overlap < minOverlap) {
			minOverlap = overlap;
			bestAxis = n;
		}
		return true;
	};

	if ( !testAxis(capAxis) ) return false;
	for ( auto i = 0; i < 3; ++i ) if ( !testAxis(axesA[i]) ) return false;
	for ( auto i = 0; i < 3; ++i ) if ( !testAxis(uf::vector::cross(axesA[i], capAxis)) ) return false;

	if ( uf::vector::dot(bestAxis, cB - cA) < 0.0f ) bestAxis = -bestAxis;

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