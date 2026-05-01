namespace {
	bool planeAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( PLANE, AABB );

		const auto& plane = a;
		const auto& aabb = b;

		auto normal = uf::vector::normalize( plane.collider.plane.normal );
		float offset = plane.collider.plane.offset;

		auto center = ::aabbCenter( aabb.bounds ); // center
		auto extent = ::aabbExtent( aabb.bounds ); // half extents
		float r = fabs(extent.x * normal.x) + fabs(extent.y * normal.y) + fabs(extent.z * normal.z); // effective projection radius of box onto plane normal

		float dist = uf::vector::dot( normal, center ) - offset;
		if ( dist > r ) return false;

		pod::Vector3f contact = center - normal * dist;
		float penetration = r - dist;

		manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
		return true;
	}
	bool planeSphere(const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps) {
		ASSERT_COLLIDER_TYPES(PLANE, SPHERE);

		const auto& plane = a;
		const auto& sphere = b;

		auto& normal = plane.collider.plane.normal;
		float offset = plane.collider.plane.offset;
		
		auto center = ::getPosition( sphere );
		float r = sphere.collider.sphere.radius;

		float dist = uf::vector::dot( normal, center ) - offset;
		if ( dist > r ) return false;

		float penetration = r - dist;
		auto contact = center - normal * r;

		manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
		return true;
	}
	bool planePlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( PLANE, PLANE );
		return false;
	}
	bool planeCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( PLANE, CAPSULE );
		REVERSE_COLLIDER( a, b, capsulePlane );
	}
	bool planeMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( PLANE, MESH );
		REVERSE_COLLIDER( a, b, meshPlane );
	}
	bool planeHull( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( PLANE, CONVEX_HULL );
		REVERSE_COLLIDER( a, b, hullPlane );
	}
}