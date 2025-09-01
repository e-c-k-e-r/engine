namespace {
	bool sphereSphere( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( SPHERE, SPHERE );

		auto delta = ::getPosition( b ) - ::getPosition( a );
		float r = a.collider.u.sphere.radius + b.collider.u.sphere.radius;

		float dist2 = uf::vector::dot(delta, delta);
		if ( dist2 > r * r ) return false;
		float dist = std::sqrt( dist2 );
		
		float penetration = r - dist;
		auto normal = ::normalizeDelta( delta, dist, eps );
		auto contact = ::getPosition( a ) + (normal * a.collider.u.sphere.radius);

		manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
		return true;
	}
	bool sphereAabb( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( SPHERE, AABB );

		const auto& sphere = a;
		const auto& aabb = b;

		auto center = ::getPosition( sphere );
		float r = sphere.collider.u.sphere.radius;

		auto& bounds = aabb.bounds;
		auto closest = pod::Vector3f{
			std::max(bounds.min.x, std::min(center.x, bounds.max.x)),
			std::max(bounds.min.y, std::min(center.y, bounds.max.y)),
			std::max(bounds.min.z, std::min(center.z, bounds.max.z)),
		};

		auto delta = center - closest;
		float dist2 = uf::vector::dot(delta, delta);
		if ( dist2 > r * r ) return false;
		float dist = std::sqrt( dist2 );

		float penetration = r - dist;
		auto normal = ::normalizeDelta( delta, dist, eps );
		auto contact = center - normal * r;

		#if 0
			pod::Vector3f dir = center - ::aabbCenter( bounds );
			if (std::fabs(dir.x) > std::fabs(dir.y) && std::fabs(dir.x) > std::fabs(dir.z)) normal = { (dir.x > 0 ? 1 : -1), 0, 0 };
			else if (std::fabs(dir.y) > std::fabs(dir.z)) normal = { 0, (dir.y > 0 ? 1 : -1), 0 };
			else normal = { 0, 0, (dir.z > 0 ? 1 : -1) };
		#endif
		
		manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
		return true;
	}
	bool spherePlane( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( SPHERE, PLANE );
		return planeSphere( b, a, manifold, eps );
	}
	bool sphereCapsule( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( SPHERE, CAPSULE );
		return capsuleSphere( b, a, manifold, eps );
	}
	bool sphereMesh( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( SPHERE, MESH );
		return meshSphere( b, a, manifold, eps );
	}
}