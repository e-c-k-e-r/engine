namespace {
	bool sphereSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( SPHERE, SPHERE );

		auto delta = ::getPosition( b ) - ::getPosition( a );
		float r = a.collider.sphere.radius + b.collider.sphere.radius;

		float dist2 = uf::vector::magnitude( delta );
		if ( dist2 > r * r ) return false;
		float dist = std::sqrt( dist2 );
		
		float penetration = r - dist;
		auto normal = ::normalizeDelta( delta, dist, eps );
		auto contact = ::getPosition( a ) + (normal * a.collider.sphere.radius);

		manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
		return true;
	}
	bool sphereAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( SPHERE, AABB );

		const auto& sphere = a;
		const auto& aabb = b;

		auto center = ::getPosition( sphere );
		float r = sphere.collider.sphere.radius;

		auto& bounds = aabb.bounds;
		auto closest = pod::Vector3f{
			std::max(bounds.min.x, std::min(center.x, bounds.max.x)),
			std::max(bounds.min.y, std::min(center.y, bounds.max.y)),
			std::max(bounds.min.z, std::min(center.z, bounds.max.z)),
		};

		auto delta = center - closest;
		float dist2 = uf::vector::magnitude( delta );
		if ( dist2 > r * r ) return false;
		float dist = std::sqrt( dist2 );

		float penetration = r - dist;
		auto normal = ::normalizeDelta( delta, dist, eps );
		auto contact = center - normal * r;
		
		manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
		return true;
	}
	bool spherePlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( SPHERE, PLANE );
		REVERSE_COLLIDER( a, b, planeSphere );
	}
	bool sphereCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( SPHERE, CAPSULE );
		REVERSE_COLLIDER( a, b, capsuleSphere );
	}
	bool sphereMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( SPHERE, MESH );
		REVERSE_COLLIDER( a, b, meshSphere );
	}
	bool sphereHull( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( SPHERE, CONVEX_HULL );
		REVERSE_COLLIDER( a, b, hullSphere );
	}
}