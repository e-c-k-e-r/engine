namespace {
	bool rayTriangleIntersect( const pod::Ray& ray, const pod::Triangle& tri, float& t, float& u, float& v, float eps = EPS(1.0e-6f) ) {
		auto edge1 = tri.points[1] - tri.points[0];
		auto edge2 = tri.points[2] - tri.points[0];
		auto h = uf::vector::cross( ray.direction, edge2 );
		float a = uf::vector::dot( edge1, h );
		
		if ( fabs(a) < eps ) return false;
		
		float f = 1.0f / a;
		auto s = ray.origin - tri.points[0];
		u = f * uf::vector::dot( s, h );
		if ( u < 0.0f || u > 1.0f ) return false;
		
		auto q = uf::vector::cross( s, edge1 );
		v = f * uf::vector::dot( ray.direction, q );
		if (v < 0.0f || u + v > 1.0f) return false;
		
		t = f * uf::vector::dot( edge2, q );
		return ( t > eps );
	}
	
	bool rayAabbIntersect( const pod::Ray& ray, const pod::AABB& box, float& tMin, float& tMax, float eps = EPS(1.0e-6f) ) {
		tMin = 0.0f;
		tMax = FLT_MAX;

		for ( auto i = 0; i < 3; ++i ) {
			float minB = box.min[i];
			float maxB = box.max[i];

			// inflate degenerate slab
			if ( fabs(maxB - minB) < 1e-6f) {
				maxB += 1e-6f;
				minB -= 1e-6f;
			}

			float invD = 1.0f / ray.direction[i];
			float t0 = ( minB - ray.origin[i] ) * invD;
			float t1 = ( maxB - ray.origin[i] ) * invD;

			if ( invD < 0.0f ) std::swap(t0, t1);

			tMin = std::max(tMin, t0);
			tMax = std::min(tMax, t1);

			if ( tMax <= tMin ) return false;
		}
		return true;
	}

	bool rayAabb( const pod::Ray& ray, const pod::RigidBody& body, pod::RayQuery& rayHit ) {
		float tMin = 0.0f;
		float tMax = FLT_MAX;

		if ( !::rayAabbIntersect( ray, body.bounds, tMin, tMax ) ) return false;

		if ( tMin < rayHit.contact.penetration ) {
			rayHit.hit = true;
			rayHit.body = &body;
			rayHit.contact.point = ray.origin + ray.direction * tMin;
			rayHit.contact.penetration = tMin;

			auto local = rayHit.contact.point - (body.bounds.min + body.bounds.max) * 0.5f;
			auto extents = (body.bounds.max - body.bounds.min) * 0.5f;
			auto absLocal = pod::Vector3f{fabs(local.x), fabs(local.y), fabs(local.z)};

			if ( absLocal.x > absLocal.y && absLocal.x > absLocal.z ) {
				rayHit.contact.normal = { (local.x > 0 ? 1.0f : -1.0f), 0, 0 };
			} else if ( absLocal.y > absLocal.z ) {
				rayHit.contact.normal = { 0, (local.y > 0 ? 1.0f : -1.0f), 0 };
			} else {
				rayHit.contact.normal = { 0, 0, (local.z > 0 ? 1.0f : -1.0f) };
			}
		}
		return true;
	}
	bool raySphere( const pod::Ray& ray, const pod::RigidBody& body, pod::RayQuery& rayHit ) {
		auto center = ::getPosition(body);
		float r = body.collider.u.sphere.radius;

		// vector from sphere center to ray origin
		auto oc = ray.origin - center;

		float a = uf::vector::dot( ray.direction, ray.direction );
		float b = 2.0f * uf::vector::dot( oc, ray.direction );
		float c = uf::vector::dot( oc, oc ) - r*r;

		float disc = b*b - 4*a*c;

		UF_MSG_DEBUG( "center={}, r={}, oc={}, a, b, c, disc", uf::vector::toString( center ), r, uf::vector::toString( oc ), a, b, c, disc );

		if ( disc < 0 ) return false;

		float sqrtDisc = std::sqrt(disc);
		float t0 = (-b - sqrtDisc) / (2*a);
		float t1 = (-b + sqrtDisc) / (2*a);

		float t = ( t0 >= 0 ) ? t0 : t1;

		UF_MSG_DEBUG( "sqrtDisc={}, t0={}, t1={}, t={}, rayHit.contact.penetration={}", sqrtDisc, t0, t1, t, rayHit.contact.penetration );

		if ( t < 0 ) return false; // both behind ray

		// compare against current best hit
		if ( t >= rayHit.contact.penetration ) return false;

		// record hit
		rayHit.hit = true;
		rayHit.body = &body;
		rayHit.contact.point = ray.origin + ray.direction*t;
		rayHit.contact.normal = uf::vector::normalize( rayHit.contact.point - center );
		rayHit.contact.penetration = t;
		return true;
	}
	bool rayPlane( const pod::Ray& ray, const pod::RigidBody& body, pod::RayQuery& rayHit, float eps = EPS(1e-6f) ) {
		auto& normal = body.collider.u.plane.normal;
		float offset = body.collider.u.plane.offset;

		float denom = uf::vector::dot( normal, ray.direction );
		if ( fabs(denom) < eps ) return false; // parallel

		float t = (offset - uf::vector::dot( normal, ray.origin )) / denom;
		if ( t < 0.0f ) return false; // behind ray start
		if ( t >= rayHit.contact.penetration ) return false; // existing hit is closer

		// record hit
		rayHit.hit = true;
		rayHit.body = &body;
		rayHit.contact.point = ray.origin + ray.direction * t;
		rayHit.contact.normal = normal * ( denom < 0.0f ? 1.0f : -1.0f ); // face ray
		rayHit.contact.penetration = t;
		return true;
	}
	bool rayCapsule( const pod::Ray& ray, const pod::RigidBody& body, pod::RayQuery& rayHit ) {
		auto [ p1, p2 ] = ::getCapsuleSegment( body );
		float r = body.collider.u.capsule.radius;

		auto d = p2 - p1;			// segment direction
		auto m = ray.origin - p1;
		auto n = ray.direction;

		float md = uf::vector::dot(m, d);
		float nd = uf::vector::dot(n, d);
		float dd = uf::vector::dot(d, d);

		// coeffs for intersection with infinite cylinder
		float a = dd - nd*nd;
		float k = uf::vector::dot(m, m) - r*r;
		float c = dd - md*md;

		float b = dd * uf::vector::dot(m,n) - nd*md;
		float discr = b*b - a*c;
		if ( discr < 0.0f ) return false;

		float t = (-b - std::sqrt(discr)) / a;  // nearer hit

		if ( t < 0 || t > rayHit.contact.penetration ) return false; // clipped by range

		// check hit actually lies within segment caps
		float y = md + t*nd;
		if ( y < 0.0f || y > dd ) {
			// check against spherical caps instead
			float tSphere = FLT_MAX;
			bool hit = false;

			for ( auto center : {p1, p2} ) {
				auto oc = ray.origin - center;
				float b = uf::vector::dot(oc, n);
				float c = uf::vector::dot(oc, oc) - r*r;
				float disc = b*b - c;
				if ( disc >= 0 ) {
					float tTmp = -b - std::sqrt(disc);
					if ( tTmp >= 0 && tTmp < tSphere ) {
						tSphere = tTmp;
						hit = true;
					}
				}
			}
			if ( !hit || tSphere > rayHit.contact.penetration ) return false;

			t = tSphere;
		}

		// register hit
		if ( t < rayHit.contact.penetration ) {
			rayHit.hit = true;
			rayHit.body = &body;
			rayHit.contact.point = ray.origin + n * t;
			rayHit.contact.penetration = t;

			// normal: from capsule axis to hit point
			auto closest = ::closestPointOnSegment( rayHit.contact.point, p1, p2 );
			auto normal = uf::vector::normalize( rayHit.contact.point - closest );
			rayHit.contact.normal = normal;
		}
		return true;
	}
	
	bool rayMesh( const pod::Ray& r, const pod::RigidBody& body, pod::RayQuery& rayHit ) {
		const uf::Mesh& meshData = *body.collider.u.mesh.mesh;
		const pod::BVH& bvh  = *body.collider.u.mesh.bvh;

		const auto transform = ::getTransform( body );

		pod::Ray ray;
		ray.origin	= uf::transform::applyInverse( transform, r.origin );
		ray.direction = uf::quaternion::rotate( uf::quaternion::inverse( transform.orientation ), r.direction );

		uf::stl::vector<int> indices;
		::queryBVH( bvh, ray, indices );

		for ( auto triID : indices ) {
			auto tri = ::fetchTriangle( meshData, triID );

			float t, u, v;
			if ( !::rayTriangleIntersect( ray, tri, t, u, v ) ) continue;
			if ( t >= rayHit.contact.penetration ) continue;

			auto l = ray.origin + ray.direction * t;
			auto bary = ::computeBarycentric( l, tri );
			auto n = uf::vector::normalize( ::interpolateWithBarycentric( bary, tri.normals ) );

			// push back to world
			auto p = uf::transform::apply( transform, l);
			n = uf::quaternion::rotate( transform.orientation, n );

			rayHit.hit = true;
			rayHit.body = &body;
			rayHit.contact.point = p;
			rayHit.contact.normal = n;
			rayHit.contact.penetration = t;
		}

		return rayHit.hit;
	}
}