namespace {
	bool planeAabb( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( PLANE, AABB );

		const auto& plane = a;
		const auto& aabb = b;

		auto normal = uf::vector::normalize( plane.collider.u.plane.normal );
		float offset = plane.collider.u.plane.offset;

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
	bool planeSphere(const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps) {
		ASSERT_COLLIDER_TYPES(PLANE, SPHERE);

		const auto& plane = a;
		const auto& sphere = b;

		auto& normal = plane.collider.u.plane.normal;
		float offset = plane.collider.u.plane.offset;
		
		auto center = ::getPosition( a );
		float r = sphere.collider.u.sphere.radius;

		float dist = uf::vector::dot( normal, center ) - offset;
		if ( dist > r ) return false;

		auto contact = center - normal * dist - normal * r;
		float penetration = r - dist;

		manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
		return true;
	}
	bool planePlane( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( PLANE, PLANE );
		return false;
	}
	bool planeCapsule( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( PLANE, CAPSULE );
		return capsulePlane( b, a, manifold, eps );
	}
	bool planeMesh( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( PLANE, MESH );
		return meshPlane( b, a, manifold, eps );
	}
}