namespace {
	bool aabbOverlap( const pod::AABB& a, const pod::AABB& b, float eps ) {
		return  (a.min.x < b.max.x - eps && a.max.x > b.min.x + eps) &&
				(a.min.y < b.max.y - eps && a.max.y > b.min.y + eps) &&
				(a.min.z < b.max.z - eps && a.max.z > b.min.z + eps);
	}

	pod::AABB computeSegmentAABB( const pod::Vector3f& p1, const pod::Vector3f p2, float r ) {
		return { 
			uf::vector::min( p1, p2 ) - r,
			uf::vector::max( p1, p2 ) + r,
		};
	}

	pod::Vector3f closestPointOnAABB(const pod::Vector3f& p, const pod::AABB& box) {
		return {
			std::max(box.min.x, std::min(p.x, box.max.x)),
			std::max(box.min.y, std::min(p.y, box.max.y)),
			std::max(box.min.z, std::min(p.z, box.max.z))
		};
	}

	std::pair<pod::Vector3f, pod::Vector3f> getCapsuleSegment( const pod::RigidBody& body ) {
		const auto transform = ::getTransform( body );
		const auto& capsule = body.collider.u.capsule;
		const pod::Vector3f up = uf::quaternion::rotate( transform.orientation, pod::Vector3f{0,1,0} );

		// segment defines the cylinder axis only (ignore spherical ends)
		auto p1 = transform.position + up * capsule.halfHeight;
		auto p2 = transform.position - up * capsule.halfHeight;
		return { p1, p2 };
	}

	pod::AABB computeAABB( const pod::RigidBody& body ) {
		const auto transform = ::getTransform( body );
		switch ( body.collider.type ) {
			case pod::ShapeType::AABB: {
				return {
					transform.position + body.collider.u.aabb.min,
					transform.position + body.collider.u.aabb.max,
				};
			} break;
			case pod::ShapeType::SPHERE: {
				return {
					transform.position - body.collider.u.sphere.radius,
					transform.position + body.collider.u.sphere.radius,
				};
			} break;
			case pod::ShapeType::CAPSULE: {
				auto [ p1, p2 ] = ::getCapsuleSegment( body );
				return ::computeSegmentAABB( p1, p2, body.collider.u.capsule.radius );
			} break;
			case pod::ShapeType::MESH: {
				if ( body.collider.u.mesh.bvh && !body.collider.u.mesh.bvh->nodes.empty() )
					return {
						transform.position + body.collider.u.mesh.bvh->nodes[0].bounds.min,
						transform.position + body.collider.u.mesh.bvh->nodes[0].bounds.max,
					};
			} break;
			default: {
			} break;
		}

		return {};
	}

	pod::AABB computeTriangleAABB( const pod::Triangle& tri ) {
		return {
			{
				std::min({tri.points[0].x, tri.points[1].x, tri.points[2].x}),
				std::min({tri.points[0].y, tri.points[1].y, tri.points[2].y}),
				std::min({tri.points[0].z, tri.points[1].z, tri.points[2].z}),
			},
			{
				std::max({tri.points[0].x, tri.points[1].x, tri.points[2].x}),
				std::max({tri.points[0].y, tri.points[1].y, tri.points[2].y}),
				std::max({tri.points[0].z, tri.points[1].z, tri.points[2].z}),
			},
		};
	}

	float triAabbDistanceSq( const pod::Triangle& tri, const pod::AABB& box ) {
		float minDistSq = FLT_MAX;
		for ( auto i = 0; i < 3; ++i ) {
			auto cp = ::closestPointOnAABB( tri.points[i], box );
			auto d  = tri.points[i] - cp;
			minDistSq = std::min( minDistSq, uf::vector::dot( d, d ) );
		}
		return minDistSq;
	}

	bool triAabbOverlap( const pod::Triangle& tri, const pod::AABB& box, float eps ) {
		// compute box center and half extents
		auto c = ( box.min + box.max ) * 0.5f;
		auto e = ( box.max - box.min ) * 0.5f;

		// move triangle into box's local space
		auto v0 = tri.points[0] - c;
		auto v1 = tri.points[1] - c;
		auto v2 = tri.points[2] - c;

		// triangle edges
		auto f0 = v1 - v0;
		auto f1 = v2 - v1;
		auto f2 = v0 - v2;

		// sAT: test the 9 edge cross axes
		auto axisTest = [&]( const pod::Vector3f& axis ) {
			if ( uf::vector::magnitude(axis) < eps ) return true;
			
			auto a = uf::vector::normalize( axis );
			
			float p0 = uf::vector::dot( v0, a );
			float p1 = uf::vector::dot( v1, a );
			float p2 = uf::vector::dot( v2, a );

			float r = e.x * fabs(a.x) + e.y * fabs(a.y) + e.z * fabs(a.z);
			
			float minP = std::min({p0, p1, p2});
			float maxP = std::max({p0, p1, p2});
			
			return !(minP > r || maxP < -r);
		};

		if ( !axisTest( {0, -f0.z, f0.y} ) ) return false;
		if ( !axisTest( {0, -f1.z, f1.y} ) ) return false;
		if ( !axisTest( {0, -f2.z, f2.y} ) ) return false;
		if ( !axisTest( {f0.z, 0, -f0.x} ) ) return false;
		if ( !axisTest( {f1.z, 0, -f1.x} ) ) return false;
		if ( !axisTest( {f2.z, 0, -f2.x} ) ) return false;
		if ( !axisTest( {-f0.y, f0.x, 0} ) ) return false;
		if ( !axisTest( {-f1.y, f1.x, 0} ) ) return false;
		if ( !axisTest( {-f2.y, f2.x, 0} ) ) return false;

		// test AABB face axes
		for ( auto i = 0; i < 3; ++i ) {
			float minVal = std::min({v0[i], v1[i], v2[i]});
			float maxVal = std::max({v0[i], v1[i], v2[i]});
			if ( minVal > e[i] || maxVal < -e[i] ) return false;
		}

		// test triangle normal axis
		auto n = uf::vector::cross( f0, f1 );
		float d0 = uf::vector::dot( v0, n );
		float r  = e.x * fabs(n.x) + e.y * fabs(n.y) + e.z * fabs(n.z);
		if ( fabs(d0) > r ) return false;

		return true;
	}

	pod::AABB mergeAabb( const pod::AABB& a, const pod::AABB& b ) {
		return {
			uf::vector::min( a.min, b.min ),
			uf::vector::max( a.max, b.max ),
		};
	}

	pod::AABB transformAabbToLocal( const pod::AABB& box, const pod::Transform<>& transform ) {
		auto inv = uf::transform::inverse( transform );

		// transform all 8 corners
		pod::Vector3f corners[8] = {
			{ box.min.x, box.min.y, box.min.z },
			{ box.max.x, box.min.y, box.min.z },
			{ box.min.x, box.max.y, box.min.z },
			{ box.max.x, box.max.y, box.min.z },
			{ box.min.x, box.min.y, box.max.z },
			{ box.max.x, box.min.y, box.max.z },
			{ box.min.x, box.max.y, box.max.z },
			{ box.max.x, box.max.y, box.max.z },
		};

		pod::AABB out = {
			{  FLT_MAX,  FLT_MAX,  FLT_MAX },
			{ -FLT_MAX, -FLT_MAX, -FLT_MAX },
		};

		for ( auto i = 0; i < 8; ++i ) {
			auto local = uf::transform::apply( inv, corners[i] );
			out.min = uf::vector::min( out.min, local );
			out.max = uf::vector::max( out.max, local );
		}
		return out;
	}

	pod::Vector3f aabbCenter( const pod::AABB& aabb ) {
		return ( aabb.max + aabb.min ) * 0.5f;
	}
	pod::Vector3f aabbExtent( const pod::AABB& aabb ) {
		return ( aabb.max - aabb.min ) * 0.5f;
	}

	bool aabbAabb( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES(AABB, AABB);

		const auto& A = a.bounds;
		const auto& B = b.bounds;

		if ( !::aabbOverlap( A, B ) ) return false;

		// Calculate overlap extents
		float overlaps[3] = {
			std::min(A.max.x, B.max.x) - std::max(A.min.x, B.min.x),
			std::min(A.max.y, B.max.y) - std::max(A.min.y, B.min.y),
			std::min(A.max.z, B.max.z) - std::max(A.min.z, B.min.z)
		};

		// Determine collision axis = smallest overlap
		int axis = -1;
		float minOverlap = FLT_MAX;
		for (int i = 0; i < 3; ++i) {
			if (overlaps[i] < minOverlap) {
				minOverlap = overlaps[i];
				axis = i;
			}
		}

		pod::Vector3f delta = ::getPosition( b ) - ::getPosition( a );
		pod::Vector3f normal{0,0,0};
		normal[axis] = (delta[axis] < 0 ? -1.0f : 1.0f);

		// Build manifold contacts: overlap region corners on the separating axis
		float xMin = std::max(A.min.x, B.min.x);
		float xMax = std::min(A.max.x, B.max.x);
		float yMin = std::max(A.min.y, B.min.y);
		float yMax = std::min(A.max.y, B.max.y);
		float zMin = std::max(A.min.z, B.min.z);
		float zMax = std::min(A.max.z, B.max.z);

		// On chosen axis, clamp to overlapped rectangle -> 4 potential points
		if (axis == 0) { // X-axis separation, so face-on overlap in YZ plane
			manifold.points.emplace_back(pod::Contact{ { (normal.x > 0 ? A.max.x : A.min.x), yMin, zMin }, normal, minOverlap });
			manifold.points.emplace_back(pod::Contact{ { (normal.x > 0 ? A.max.x : A.min.x), yMin, zMax }, normal, minOverlap });
			manifold.points.emplace_back(pod::Contact{ { (normal.x > 0 ? A.max.x : A.min.x), yMax, zMin }, normal, minOverlap });
			manifold.points.emplace_back(pod::Contact{ { (normal.x > 0 ? A.max.x : A.min.x), yMax, zMax }, normal, minOverlap });
		}
		else if (axis == 1) { // Y-axis separation, overlap in XZ plane
			manifold.points.emplace_back(pod::Contact{ { xMin, (normal.y > 0 ? A.max.y : A.min.y), zMin }, normal, minOverlap });
			manifold.points.emplace_back(pod::Contact{ { xMin, (normal.y > 0 ? A.max.y : A.min.y), zMax }, normal, minOverlap });
			manifold.points.emplace_back(pod::Contact{ { xMax, (normal.y > 0 ? A.max.y : A.min.y), zMin }, normal, minOverlap });
			manifold.points.emplace_back(pod::Contact{ { xMax, (normal.y > 0 ? A.max.y : A.min.y), zMax }, normal, minOverlap });
		}
		else if (axis == 2) { // Z-axis separation, overlap in XY plane
			manifold.points.emplace_back(pod::Contact{ { xMin, yMin, (normal.z > 0 ? A.max.z : A.min.z) }, normal, minOverlap });
			manifold.points.emplace_back(pod::Contact{ { xMin, yMax, (normal.z > 0 ? A.max.z : A.min.z) }, normal, minOverlap });
			manifold.points.emplace_back(pod::Contact{ { xMax, yMin, (normal.z > 0 ? A.max.z : A.min.z) }, normal, minOverlap });
			manifold.points.emplace_back(pod::Contact{ { xMax, yMax, (normal.z > 0 ? A.max.z : A.min.z) }, normal, minOverlap });
		}

		return true;
	}

	bool aabbSphere( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( AABB, SPHERE );
		return ::sphereAabb( b, a, manifold, eps );
	}
	bool aabbPlane( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( AABB, PLANE );
		return ::planeAabb( b, a, manifold, eps );
	}
	bool aabbCapsule( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( AABB, CAPSULE );
		return ::capsuleAabb( b, a, manifold, eps );
	}
	bool aabbMesh( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( AABB, MESH );
		return ::meshAabb( b, a, manifold, eps );
	}
}