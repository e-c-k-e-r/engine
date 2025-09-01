namespace {
	bool capsuleCapsule( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( CAPSULE, CAPSULE );

		auto [ A1, A2 ] = ::getCapsuleSegment( a );
		auto [ B1, B2 ] = ::getCapsuleSegment( b );

		auto [ pA, pB ] = ::closestSegmentSegment( A1, A2, B1, B2 );
		float r = a.collider.u.capsule.radius + b.collider.u.capsule.radius;
		
		auto delta = pB - pA;
		float dist2 = uf::vector::dot(delta, delta);
		if ( dist2 > r * r ) return false;
		float dist = std::sqrt( dist2 );
		
		auto contact = ( pA + pB ) * 0.5f;
		auto normal = ::normalizeDelta( delta, dist, eps );
		float penetration = r - dist;

		manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
		return true;
	}
	bool capsuleAabb( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( CAPSULE, AABB );
		const auto& capsule = a;
		const auto& box = b;

		auto [ p1, p2 ] = ::getCapsuleSegment( capsule );
		float r = capsule.collider.u.capsule.radius;

		auto closestPoint = ::closestPointSegmentAabb( p1, p2, box.bounds );
		auto closestSegment = ::closestPointOnSegment( closestPoint, p1, p2 );
		
		auto delta = closestPoint - closestSegment;
		float dist2 = uf::vector::magnitude( delta );
		if ( dist2 > r * r ) return false;
		float dist = std::sqrt( dist2 );

		float penetration = r - dist;
		auto normal = ( dist > eps ) ? ( delta / dist ) : pod::Vector3f{0,1,0};
		auto contact = closestSegment + normal * ( r - penetration * 0.5f );

		manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
		return true;
	}
	bool capsulePlane( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( CAPSULE, PLANE );
		const auto& capsule = a;
		const auto& plane = b;

		const auto& normal = plane.collider.u.plane.normal;
		float o = plane.collider.u.plane.offset;

		auto [ p1, p2 ] = ::getCapsuleSegment( capsule );
		float r = capsule.collider.u.capsule.radius;

		// the "foot" is just whichever end of the capsule is closest to the normal
		auto foot = ( uf::vector::dot( normal, p1 ) < uf::vector::dot( normal, p2 ) ) ? p1 : p2;
		float dist = uf::vector::dot( normal, foot ) - o;
		if ( dist > r ) return false;

		auto contact = foot - (normal * r);
		float penetration = r - dist;
		manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });

		// build up to 4 contact points sampled around foot circle
	#if 0
		auto tangent1 = uf::vector::normalize( uf::vector::cross( normal, pod::Vector3f{1,0,0} ) );
		if ( uf::vector::magnitude( tangent1 ) < EPS(1e-6f) ) tangent1 = uf::vector::normalize( uf::vector::cross( normal, pod::Vector3f{0,1,0} ) );
		auto tangent2 = uf::vector::cross( normal, tangent1 );

		// four directions around circle
		manifold.points.emplace_back(pod::Contact{ foot + tangent1 * r - normal * d, normal, penetration });
		manifold.points.emplace_back(pod::Contact{ foot - tangent1 * r - normal * d, normal, penetration });
		manifold.points.emplace_back(pod::Contact{ foot + tangent2 * r - normal * d, normal, penetration });
		manifold.points.emplace_back(pod::Contact{ foot - tangent2 * r - normal * d, normal, penetration });
	#endif
		return true;
	}
	bool capsuleSphere( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( CAPSULE, SPHERE );
		const auto& capsule = a;
		const auto& sphere = b;

		auto [ p1, p2 ] = ::getCapsuleSegment( capsule );

		auto sphereCenter = ::getPosition( sphere );
		float r = capsule.collider.u.capsule.radius + sphere.collider.u.sphere.radius;

		// closest point on capsule segment to sphere center
		auto closest = ::closestPointOnSegment( sphereCenter, p1, p2 );

		auto delta = sphereCenter - closest;
		float dist2 = uf::vector::magnitude( delta );
		if ( dist2 > r * r ) return false;
		float dist = std::sqrt( dist2 );

		float penetration = r - dist;
		auto normal = ::normalizeDelta( delta, dist, eps );
		auto contact = closest + normal * (capsule.collider.u.capsule.radius - penetration * 0.5f );

		manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
		return true;
	}
	bool capsuleMesh( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( CAPSULE, MESH );
		return meshCapsule( b, a, manifold, eps );
	}
}