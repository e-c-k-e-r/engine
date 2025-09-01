#define REORIENT_NORMALS_ON_FETCH 0
#define REORIENT_NORMALS_ON_CONTACT 1

// mesh BVH
namespace {
	// to-do: clean this up
	uint32_t getIndex( const void* indexData, size_t indexSize, size_t idx ) {
		if ( indexSize == sizeof(uint8_t) ) {
			auto* ptr = reinterpret_cast<const uint8_t*>(indexData);
			return static_cast<uint32_t>(ptr[idx]);
		} else if ( indexSize == sizeof(uint16_t) ) {
			auto* ptr = reinterpret_cast<const uint16_t*>(indexData);
			return static_cast<uint32_t>(ptr[idx]);
		} else if ( indexSize == sizeof(uint32_t) ) {
			auto* ptr = reinterpret_cast<const uint32_t*>(indexData);
			return ptr[idx];
		}
		UF_EXCEPTION("Unsupported index type of size {}", indexSize);
	}

	pod::Vector3f triangleCenter( const pod::Triangle& tri ) {
		return ( tri.points[0] + tri.points[1] + tri.points[2] ) / 3.0f;
	}
	pod::Vector3f triangleNormal( const pod::Triangle& tri ) {
		return uf::vector::normalize(uf::vector::cross(tri.points[1] - tri.points[0], tri.points[2] - tri.points[0]));
	}
	pod::Vector3f triangleNormal( const pod::TriangleWithNormal& tri ) {
		return uf::vector::normalize( tri.normals[0] + tri.normals[1] + tri.normals[2] );
	}

	pod::AABB computeTriangleAABB( const void* vertices, size_t vertexStride, const void* indexData, size_t indexSize, size_t triID ) {
		auto triIndexID = triID * 3;

		uint32_t i0 = ::getIndex( indexData, indexSize, triIndexID + 0 );
		uint32_t i1 = ::getIndex( indexData, indexSize, triIndexID + 1 );
		uint32_t i2 = ::getIndex( indexData, indexSize, triIndexID + 2 );

		auto& v0 = *reinterpret_cast<const pod::Vector3f*>(reinterpret_cast<const uint8_t*>(vertices) + i0 * vertexStride);
		auto& v1 = *reinterpret_cast<const pod::Vector3f*>(reinterpret_cast<const uint8_t*>(vertices) + i1 * vertexStride);
		auto& v2 = *reinterpret_cast<const pod::Vector3f*>(reinterpret_cast<const uint8_t*>(vertices) + i2 * vertexStride);

		return {
			{	
				std::min({v0.x, v1.x, v2.x}),
				std::min({v0.y, v1.y, v2.y}),
				std::min({v0.z, v1.z, v2.z}),
			},
			{
				std::max({v0.x, v1.x, v2.x}),
				std::max({v0.y, v1.y, v2.y}),
				std::max({v0.z, v1.z, v2.z}),
			}
		};
	}

	pod::TriangleWithNormal fetchTriangle( const uf::Mesh& mesh, size_t triID ) {
		auto views = mesh.makeViews({"position", "normal"});
		UF_ASSERT(!views.empty());

		uint32_t triIndexID = triID * 3; // remap triangle ID to index ID
		// find which view contains this triangle index.
		const uf::Mesh::View* found = nullptr;
		for ( auto& v : views ) {
			if ( v.index.first <= triIndexID && triIndexID < v.index.first + v.index.count ) {
				found = &v;
				break;
			}
		}
		UF_ASSERT( found );

		pod::TriangleWithNormal tri;

		auto& positions = (*found)["position"];
		auto& normals   = (*found)["normal"];
		auto& indices   = (*found)["index"];

		const void* indexBase = indices.data(found->index.first);
		size_t indexSize	  = mesh.index.size;

		// reset back to local indices range
		triIndexID -= found->index.first;

		uint32_t idxs[3];
		// to-do: just make this a macro that could have a parallel hint
		for ( auto i = 0; i < 3; ++i ) idxs[i] = getIndex(indexBase, indexSize, triIndexID + i);

		{
			auto* base = reinterpret_cast<const uint8_t*>(positions.data(found->vertex.first));
			size_t stride = positions.stride();

			for ( auto i = 0; i < 3; ++i ) tri.points[i] = *reinterpret_cast<const pod::Vector3f*>(base + idxs[i] * stride);
		}

		if ( normals.valid() ) {
			auto* base = reinterpret_cast<const uint8_t*>(normals.data(found->vertex.first));
			size_t stride = normals.stride();
			for ( auto i = 0; i < 3; ++i ) tri.normals[i] = *reinterpret_cast<const pod::Vector3f*>(base + idxs[i] * stride);
		} else {
			auto normal = ::triangleNormal( (pod::Triangle&) tri );
			for ( auto i = 0; i < 3; ++i ) tri.normals[i] = normal;
		}

		return tri;
	}

	// if body is a mesh, apply its transform to the triangles, else reorient the normal with respect to the body
	pod::TriangleWithNormal fetchTriangle( const uf::Mesh& mesh, size_t triID, const pod::RigidBody& body ) {
		auto tri = ::fetchTriangle( mesh, triID );
		auto transform = ::getTransform( body );

		if ( body.collider.type == pod::ShapeType::MESH ) {
			for ( auto i = 0; i < 3; ++i ) {
				tri.points[i]  = uf::transform::apply( transform, tri.points[i] );
				tri.normals[i] = uf::quaternion::rotate( transform.orientation, tri.normals[i] );
			}
		}
		else {
		#if REORIENT_NORMALS_ON_FETCH
			auto triCenter = ::triangleCenter( tri );
			auto delta = ::getPosition( body ) - triCenter;
			for ( auto i = 0; i < 3; ++i ) {
				if ( uf::vector::dot(tri.normals[i], delta) < 0.0f ) tri.normals[i] = -tri.normals[i];
			}
		#endif
		}

		return tri;
	}

	bool computeTriangleTriangleSegment( const pod::TriangleWithNormal& A, const pod::TriangleWithNormal& B, pod::Vector3f& p0, pod::Vector3f& p1, float eps = EPS(1e-6f) ) {
		uf::stl::vector<pod::Vector3f> intersections;

		auto checkAndPush = [&]( const pod::Vector3f& pt ) {
			// avoid duplicates
			for ( auto& v : intersections ) {
				if ( uf::vector::distanceSquared( v, pt ) < eps*eps ) return;
			}
			intersections.emplace_back(pt);
		};

		// segment-plane intersection
		auto intersectSegmentPlane = [&](const pod::Vector3f& a, const pod::Vector3f& b, const pod::Vector3f& n, float d, pod::Vector3f& out)->bool {
			pod::Vector3f ab = b - a;
			float denom = uf::vector::dot( n, ab );
			if (fabs(denom) < eps) return false; // parallel

			float t = (d - uf::vector::dot( n, a )) / denom;
			if ( t < -eps || t > 1.0f + eps ) return false;
			out = a + ab * t;
			return true;
		};

		// planes
		auto nA = ::triangleNormal( A );
		auto nB = ::triangleNormal( B );
		float dA = uf::vector::dot( nA, A.points[0] );
		float dB = uf::vector::dot( nB, B.points[0] );

		// clip edges of A against plane of B
		const pod::Vector3f At[3] = { A.points[0], A.points[1], A.points[2] };
		for ( auto i = 0; i < 3; ++i ) {
			int j = ( i + 1 ) % 3;
			pod::Vector3f p;
			if ( intersectSegmentPlane( At[i], At[j], nB, dB, p ) ) {
				// check if intersection lies inside triangle B
				if ( ::pointInTriangle( p, B ) ) checkAndPush(p);
			}
		}

		// clip edges of B against plane of A
		const pod::Vector3f Bt[3] = { B.points[0], B.points[1], B.points[2] };
		for ( auto i = 0; i < 3; ++i ) {
			int j = ( i + 1 ) % 3;
			pod::Vector3f p;
			if ( intersectSegmentPlane( Bt[i], Bt[j], nA, dA, p ) ) {
				if ( ::pointInTriangle( p, A ) ) checkAndPush(p);
			}
		}

		if ( intersections.empty() ) return false;

		// degenerate intersection
		if ( intersections.size() == 1 ) {
			p0 = p1 = intersections[0];
			return true;
		}

		// find two furthest apart points for intersection segment
		float maxDist2 = -1.0f;
		for ( auto i = 0 ; i < intersections.size(); i++ ) {
			for ( auto j = i + 1 ; j<intersections.size(); j++ ) {
				float d2 = uf::vector::distanceSquared( intersections[i], intersections[j] );
				if ( d2 > maxDist2 ) {
					maxDist2 = d2;
					p0 = intersections[i];
					p1 = intersections[j];
				}
			}
		}

		return maxDist2 >= 0.0f;
	}

	pod::Vector2f projectTriangleOntoAxis( const pod::TriangleWithNormal& tri, const pod::Vector3f& axis ) {
		pod::Vector3f normal = uf::vector::normalize( axis );

		float p0 = uf::vector::dot( tri.points[0], normal );
		float p1 = uf::vector::dot( tri.points[1], normal );
		float p2 = uf::vector::dot( tri.points[2], normal );

		return { std::min({ p0, p1, p2 }), std::max({ p0, p1, p2 }) };
	}
}

// triangle colliders
namespace {
	bool triangleTriangle( const pod::TriangleWithNormal& a, const pod::TriangleWithNormal& b, pod::Manifold& manifold, float eps ) {
		// if ( !::triangleTriangleIntersect( a, b ) ) return false;

		uf::stl::vector<pod::Vector3f> axes = { ::triangleNormal( a ), ::triangleNormal( b ) };

		pod::Vector3f p0 = {}, p1 = {};
		if ( !::computeTriangleTriangleSegment(a, b, p0, p1) ) {
			auto contact = ( p0 + p1 ) * 0.5f;
			auto normal   = uf::vector::normalize( axes[0] + axes[1] );
			manifold.points.emplace_back(pod::Contact{ contact, normal, eps });
			return true;
		}

		auto contact = ( p0 + p1 ) * 0.5f;
		float penetration = std::numeric_limits<float>::max();
		pod::Vector3f normal;

		// check edge cross-products
		for ( auto i = 0; i < 3; i++ ) {
			auto ea = a.points[( i + 1 ) % 3] - a.points[i];
			for ( auto j = 0; j < 3; j++ ) {
				auto eb = b.points[( j + 1 ) % 3] - b.points[j];
				auto axis = uf::vector::cross(ea, eb);
				if ( uf::vector::magnitude( axis ) > eps*eps ) axes.emplace_back( axis );
			}
		}

		// project onto each axis
		for ( auto axis : axes ) {
			axis = uf::vector::normalize( axis );
			pod::Vector2f aP = ::projectTriangleOntoAxis( a, axis );
			pod::Vector2f bP = ::projectTriangleOntoAxis( b, axis );

			float overlap = std::min( aP.x, bP.x ) - std::max( aP.y, bP.y );
			if ( overlap < 0) return false; // separating axis
			if ( overlap < penetration ) {
				penetration = overlap;
				normal = axis;
			}
		}

		manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
		return true;
	}

	bool triangleAabb( const pod::TriangleWithNormal& tri, const pod::RigidBody& body, pod::Manifold& manifold, float eps ) {
		const auto& aabb = body;

		auto closest = ::closestPointOnTriangle( ::getPosition( aabb ), tri );
		auto closestAabb = ::closestPointOnAABB( closest, aabb.bounds );

		if ( !uf::vector::isValid( closest ) ) return false;
		
		// to-do: derive proper delta
		auto delta = closestAabb - closest;
		float dist2 = uf::vector::dot( delta, delta );
		float tolerance = 1.0e-3;
		if ( dist2 >= tolerance ) return false;
		float dist = std::sqrt( dist2 );

		// to-do: properly derive the contact information
		auto contact = closest; // ( closest + closestAabb ) * 0.5f;
		auto normal = ( dist > eps ) ? ( delta / dist ) : ::triangleNormal( tri );
		float penetration = tolerance - dist;

	#if REORIENT_NORMALS_ON_CONTACT
		if ( uf::vector::dot(normal, delta) < 0.0f ) normal = -normal;
	#endif

		manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
		return true;
	}
	bool triangleSphere( const pod::TriangleWithNormal& tri, const pod::RigidBody& body, pod::Manifold& manifold, float eps ) {
		const auto& sphere = body;

		float r = sphere.collider.u.sphere.radius;
		auto center = ::getPosition( sphere );
		auto closest = ::closestPointOnTriangle( center, tri.points[0], tri.points[1], tri.points[2] );

		if ( !uf::vector::isValid( closest ) ) return false;

		// to-do: derive proper delta
		auto delta = center - closest;
		float dist2 = uf::vector::dot( delta, delta );
		if ( dist2 > r * r ) return false;
		float dist = std::sqrt(dist2);

		auto contact = ( center + closest ) * 0.5f;
		auto normal = ( dist > eps ) ? ( delta / dist ) : ::triangleNormal( tri );
		float penetration = r - dist;

	#if REORIENT_NORMALS_ON_CONTACT
		if ( uf::vector::dot(normal, delta) < 0.0f ) normal = -normal;
	#endif

		manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
		return true;
	}
	// to-do: implement
	bool trianglePlane( const pod::TriangleWithNormal& tri, const pod::RigidBody& body, pod::Manifold& manifold, float eps ) {
		const auto& plane = body;
		auto normal = plane.collider.u.plane.normal;
		float d = plane.collider.u.plane.offset;

		bool hit = false;
		pod::Vector3f dist;
		for ( auto i = 0; i < 3; i++ ) dist[i] = uf::vector::dot(normal, tri.points[i] ) - d;

		// completely on one side
		bool allAbove = ( dist.x >  eps && dist.y >  eps && dist.z >  eps );
		bool allBelow = ( dist.x < -eps && dist.y < -eps && dist.z < -eps );
		if ( allAbove )
			return hit;

		if ( allBelow ) {
			hit = true;
			for ( auto i = 0; i < 3; i++ ) {
				float penetration = -dist[i];
				manifold.points.emplace_back(pod::Contact{tri.points[i], normal, penetration});
			}
			return hit;
		}

		// points touching plane
		for ( auto i = 0; i < 3; i++ )
			if ( fabs( dist[i] ) <= eps ) {
				hit = true;
				manifold.points.emplace_back(pod::Contact{ tri.points[i], normal, 0.0f });
			}

		// edges that cross plane
		for ( auto i = 0; i < 3; i++ ) {
			auto j = (i + 1) % 3;
			if ( ( dist[i] > 0 && dist[j] < 0 ) || ( dist[i] < 0 && dist[j] > 0 ) ) {
				hit = true;
				float t = dist[i] / ( dist[i] - dist[j] );
				auto contact = tri.points[i] + ( tri.points[j] - tri.points[i] ) * t;
				float penetration = -std::min( dist[i], dist[j] );
				manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
			}
		}
		return hit;
	}
	bool triangleCapsule( const pod::TriangleWithNormal& tri, const pod::RigidBody& body, pod::Manifold& manifold, float eps ) {
		const auto& capsule = body;

		float r = capsule.collider.u.capsule.radius;
		auto [ p1, p2 ] = ::getCapsuleSegment( capsule );
		auto bounds = ::computeSegmentAABB( p1, p2, r );

		// to-do: derive proper delta
		pod::Vector3f closestSeg, closest;
		float dist2 = ::segmentTriangleDistanceSq( p1, p2, tri, closestSeg, closest );

		if ( !uf::vector::isValid( closest ) ) return false;
		if ( dist2 > r * r ) return false;
		float dist = std::sqrt( dist2 );
		auto delta = ( closestSeg - closest );

		// to-do: properly derive the contact information
		auto contact = closest; // ( closestSeg + closest ) * 0.5f;
		auto normal = ( dist > eps ) ? ( delta / dist ) : ::triangleNormal( tri );
		float penetration = r - dist;

	#if REORIENT_NORMALS_ON_CONTACT
		if ( uf::vector::dot(normal, delta) < 0.0f ) normal = -normal;
	#endif

		manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
		return true;
	}

	bool triangleTriangle( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( TRIANGLE, TRIANGLE );
		return ::triangleTriangle( a.collider.u.triangle, b.collider.u.triangle, manifold, eps );
	}
	bool triangleAabb( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( TRIANGLE, AABB );
		return ::triangleAabb( a.collider.u.triangle, b, manifold, eps );
	}
	bool triangleSphere( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( TRIANGLE, SPHERE );
		return ::triangleSphere( a.collider.u.triangle, b, manifold, eps );
	}
	bool trianglePlane( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( TRIANGLE, PLANE );
		return ::trianglePlane( a.collider.u.triangle, b, manifold, eps );
	}
	bool triangleCapsule( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( TRIANGLE, CAPSULE );
		return ::triangleCapsule( a.collider.u.triangle, b, manifold, eps );
	}
}