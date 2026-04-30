namespace {
	FORCE_INLINE bool aabbOverlap( const pod::AABB& a, const pod::AABB& b, float eps ) {
	#if UF_USE_SIMD
		return uf::simd::all( uf::simd::lessEquals( a.min, b.max ) ) && uf::simd::all( uf::simd::greaterEquals( a.max, b.min ) );
	#else
		return ( a.min - eps ) <= ( b.max + eps ) && ( a.max + eps ) >= ( b.min - eps );
	#endif
	}

	FORCE_INLINE float aabbSurfaceArea(const pod::AABB& aabb) {
		auto d = uf::vector::max( ( aabb.max - aabb.min ), pod::Vector3f{} );
		return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
	}

	FORCE_INLINE pod::AABB computeSegmentAABB( const pod::Vector3f& p1, const pod::Vector3f p2, float r ) {
		return { 
			uf::vector::min( p1, p2 ) - r,
			uf::vector::max( p1, p2 ) + r,
		};
	}

	FORCE_INLINE pod::Vector3f closestPointOnAABB(const pod::Vector3f& p, const pod::AABB& box) {
		return uf::vector::clamp( p, box.min, box.max );
	}

	FORCE_INLINE pod::AABB computeTriangleAABB( const pod::Triangle& tri ) {
		return {
			uf::vector::min( uf::vector::min( tri.points[0], tri.points[1] ), tri.points[2] ),
			uf::vector::max( uf::vector::max( tri.points[0], tri.points[1] ), tri.points[2] ),
		};
	}

	pod::AABB computeConvexHullAABB( const uf::Mesh::View& view, const uf::Mesh::AttributeView& positions, pod::AABB bounds = { {  FLT_MAX,  FLT_MAX,  FLT_MAX }, { -FLT_MAX, -FLT_MAX, -FLT_MAX } }  ) {
		for ( size_t i = 0; i < view.vertex.count; ++i ) {
			pod::Vector3f v = ::getVertex( view, positions, i );
			bounds.min = uf::vector::min( bounds.min, v );
			bounds.max = uf::vector::max( bounds.max, v );
		}

		return bounds;
	}

	FORCE_INLINE pod::AABB mergeAabb( const pod::AABB& a, const pod::AABB& b ) {
		return {
			uf::vector::min( a.min, b.min ),
			uf::vector::max( a.max, b.max ),
		};
	}

	FORCE_INLINE pod::Vector3f aabbCenter( const pod::AABB& aabb ) {
		return ( aabb.max + aabb.min ) * 0.5f;
	}
	FORCE_INLINE pod::Vector3f aabbExtent( const pod::AABB& aabb ) {
		return ( aabb.max - aabb.min ) * 0.5f;
	}

	pod::AABB transformAabbToWorld( const pod::AABB& localBox, const pod::Transform<>& transform ) {
		const auto& q = transform.orientation;
		const auto& p = transform.position;

		pod::Vector3f center  = (localBox.min + localBox.max) * 0.5f;
		pod::Vector3f extents = (localBox.max - localBox.min) * 0.5f;

		pod::Vector3f axisX = uf::quaternion::rotate(q, pod::Vector3f{1,0,0});
		pod::Vector3f axisY = uf::quaternion::rotate(q, pod::Vector3f{0,1,0});
		pod::Vector3f axisZ = uf::quaternion::rotate(q, pod::Vector3f{0,0,1});

		pod::Vector3f worldCenter = uf::quaternion::rotate(q, center) + p;

		pod::Vector3f worldExtents = {
			fabs(axisX.x) * extents.x + fabs(axisY.x) * extents.y + fabs(axisZ.x) * extents.z,
			fabs(axisX.y) * extents.x + fabs(axisY.y) * extents.y + fabs(axisZ.y) * extents.z,
			fabs(axisX.z) * extents.x + fabs(axisY.z) * extents.y + fabs(axisZ.z) * extents.z
		};

		return {
			worldCenter - worldExtents,
			worldCenter + worldExtents
		};
	}

	std::pair<pod::Vector3f, pod::Vector3f> getCapsuleSegment( const pod::PhysicsBody& body ) {
		const auto transform = ::getTransform( body );
		const auto& capsule = body.collider.capsule;
		const pod::Vector3f up = uf::quaternion::rotate( transform.orientation, pod::Vector3f{0,1,0} );

		// segment defines the cylinder axis only (ignore spherical ends)
		auto p1 = transform.position + up * capsule.halfHeight;
		auto p2 = transform.position - up * capsule.halfHeight;
		return { p1, p2 };
	}

	pod::AABB computeAABB( const pod::PhysicsBody& body ) {
		const auto transform = ::getTransform( body );
		switch ( body.collider.type ) {
			case pod::ShapeType::AABB: {
			//	return ::transformAabbToWorld( body.collider.aabb, *body.transform );
				return {
					transform.position + body.collider.aabb.min,
					transform.position + body.collider.aabb.max,
				};
			} break;
			case pod::ShapeType::SPHERE: {
				return {
					transform.position - body.collider.sphere.radius,
					transform.position + body.collider.sphere.radius,
				};
			} break;
			case pod::ShapeType::CAPSULE: {
				auto [ p1, p2 ] = ::getCapsuleSegment( body );
				return ::computeSegmentAABB( p1, p2, body.collider.capsule.radius );
			} break;
			case pod::ShapeType::MESH: {
				if ( body.collider.mesh.bvh && !body.collider.mesh.bvh->bounds.empty() )
					return {
						transform.position + body.collider.mesh.bvh->bounds[0].min,
						transform.position + body.collider.mesh.bvh->bounds[0].max,
					};
				const auto& meshData = *body.collider.mesh.mesh;
				pod::AABB bounds = { {  FLT_MAX,  FLT_MAX,  FLT_MAX }, { -FLT_MAX, -FLT_MAX, -FLT_MAX } };
				for ( const auto& view : meshData.buffer_views ) ::computeConvexHullAABB( view, view["position"], bounds );
				return ::transformAabbToWorld( bounds, transform );
			} break;
			case pod::ShapeType::CONVEX_HULL: {
				if ( body.collider.convexHull.bvh && !body.collider.convexHull.bvh->bounds.empty() )
					return {
						transform.position + body.collider.convexHull.bvh->bounds[0].min,
						transform.position + body.collider.convexHull.bvh->bounds[0].max,
					};
				const auto& meshData = *body.collider.convexHull.mesh;
				pod::AABB bounds = { {  FLT_MAX,  FLT_MAX,  FLT_MAX }, { -FLT_MAX, -FLT_MAX, -FLT_MAX } };
				for ( const auto& view : meshData.buffer_views ) ::computeConvexHullAABB( view, view["position"], bounds );
				return ::transformAabbToWorld( bounds, transform );
			} break;
			default: {
			} break;
		}

		return {};
	}

	float triAabbDistanceSq( const pod::Triangle& tri, const pod::AABB& box ) {
		float minDistSq = FLT_MAX;
		FOR_EACH(3, {
			auto cp = ::closestPointOnAABB( tri.points[i], box );
			auto d  = tri.points[i] - cp;
			minDistSq = std::min( minDistSq, uf::vector::dot( d, d ) );
		});
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

		// SAT: test the 9 edge cross axes
		auto axisTest = [&]( const pod::Vector3f& axis ) {
			float norm = uf::vector::norm( axis );
			if ( norm < eps ) return true;
			
			auto a = axis / norm;
			
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
		FOR_EACH(3, {
			float minVal = std::min({v0[i], v1[i], v2[i]});
			float maxVal = std::max({v0[i], v1[i], v2[i]});
			if ( minVal > e[i] || maxVal < -e[i] ) return false;
		});

		// test triangle normal axis
		auto n = uf::vector::cross( f0, f1 );
		float d0 = uf::vector::dot( v0, n );
		float r  = e.x * fabs(n.x) + e.y * fabs(n.y) + e.z * fabs(n.z);
		if ( fabs(d0) > r ) return false;

		return true;
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

		FOR_EACH(8, {
			auto local = uf::transform::apply( inv, corners[i] );
			out.min = uf::vector::min( out.min, local );
			out.max = uf::vector::max( out.max, local );
		});
		return out;
	}

	bool aabbAabb( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES(AABB, AABB);

		const auto& A = a.bounds;
		const auto& B = b.bounds;

		if ( !::aabbOverlap( A, B ) ) return false;

		// calculate overlap extents
		auto overlaps = uf::vector::min( A.max, B.max ) - uf::vector::max( A.min, B.min );

		// determine collision axis = smallest overlap
		auto axis = -1;
		float minOverlap = FLT_MAX;
		FOR_EACH(3, {
			if ( overlaps[i] < minOverlap ) {
				minOverlap = overlaps[i];
				axis = i;
			}
		});

		pod::Vector3f delta = ::getPosition( b ) - ::getPosition( a );
		pod::Vector3f normal{0,0,0};
		normal[axis] = (delta[axis] < 0 ? -1.0f : 1.0f);

		// build manifold contacts: overlap region corners on the separating axis
		auto Min = uf::vector::max( A.min, B.min );
		auto Max = uf::vector::min( A.max, B.max );

		// on chosen axis, clamp to overlapped rectangle -> 4 potential points
		if ( axis == 0 ) { // x-axis separation, so face-on overlap in YZ plane
			manifold.points.emplace_back(pod::Contact{ { (normal.x > 0 ? A.max.x : A.min.x), Min.y, Min.z }, normal, minOverlap });
			manifold.points.emplace_back(pod::Contact{ { (normal.x > 0 ? A.max.x : A.min.x), Min.y, Max.z }, normal, minOverlap });
			manifold.points.emplace_back(pod::Contact{ { (normal.x > 0 ? A.max.x : A.min.x), Max.y, Min.z }, normal, minOverlap });
			manifold.points.emplace_back(pod::Contact{ { (normal.x > 0 ? A.max.x : A.min.x), Max.y, Max.z }, normal, minOverlap });
		}
		else if ( axis == 1 ) { // y-axis separation, overlap in XZ plane
			manifold.points.emplace_back(pod::Contact{ { Min.x, (normal.y > 0 ? A.max.y : A.min.y), Min.z }, normal, minOverlap });
			manifold.points.emplace_back(pod::Contact{ { Min.x, (normal.y > 0 ? A.max.y : A.min.y), Max.z }, normal, minOverlap });
			manifold.points.emplace_back(pod::Contact{ { Max.x, (normal.y > 0 ? A.max.y : A.min.y), Min.z }, normal, minOverlap });
			manifold.points.emplace_back(pod::Contact{ { Max.x, (normal.y > 0 ? A.max.y : A.min.y), Max.z }, normal, minOverlap });
		}
		else if (axis == 2) { // z-axis separation, overlap in XY plane
			manifold.points.emplace_back(pod::Contact{ { Min.x, Min.y, (normal.z > 0 ? A.max.z : A.min.z) }, normal, minOverlap });
			manifold.points.emplace_back(pod::Contact{ { Min.x, Max.y, (normal.z > 0 ? A.max.z : A.min.z) }, normal, minOverlap });
			manifold.points.emplace_back(pod::Contact{ { Max.x, Min.y, (normal.z > 0 ? A.max.z : A.min.z) }, normal, minOverlap });
			manifold.points.emplace_back(pod::Contact{ { Max.x, Max.y, (normal.z > 0 ? A.max.z : A.min.z) }, normal, minOverlap });
		}

		return true;
	}

	bool aabbSphere( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( AABB, SPHERE );
		REVERSE_COLLIDER( a, b, sphereAabb );
	}
	bool aabbPlane( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( AABB, PLANE );
		REVERSE_COLLIDER( a, b, planeAabb );
	}
	bool aabbCapsule( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( AABB, CAPSULE );
		REVERSE_COLLIDER( a, b, capsuleAabb );
	}
	bool aabbMesh( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( AABB, MESH );
		REVERSE_COLLIDER( a, b, meshAabb );
	}
	bool aabbHull( const pod::PhysicsBody& a, const pod::PhysicsBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( AABB, CONVEX_HULL );
		REVERSE_COLLIDER( a, b, hullAabb );
	}
}