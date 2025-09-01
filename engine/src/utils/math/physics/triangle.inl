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
		return uf::vector::normalize(( tri.normals[0] + tri.normals[1] + tri.normals[2] ) / 3.0f);
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
}

/*

#if REORIENT_NORMALS_ON_CONTACT
	if ( uf::vector::dot(normal, delta) < 0.0f ) normal = -normal;
#endif


// uf::vector::normalize( ::interpolateWithBarycentric( ::computeBarycentric( contact, tri ), tri.normals ) );

*/

// triangle colliders
namespace {
	bool triangleTriangle( const pod::TriangleWithNormal& a, const pod::TriangleWithNormal& b, pod::Manifold& manifold, float eps ) {
		if ( !::triangleTriangleIntersect( a, b ) ) return false;

		// to-do: properly derive the contact information
		auto contact = ( a.points[0] + b.points[0] ) * 0.5f; // center point
		auto normal = ::triangleNormal( a );
		float penetration = 0.001f;
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
		auto contact = closest;
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
		auto closest = ::closestPointOnTriangle( center, tri );

		if ( !uf::vector::isValid( closest ) ) return false;

		// to-do: derive proper delta
		auto delta = center - closest;
		float dist2 = uf::vector::dot(delta, delta);
		if ( dist2 > r * r ) return false;
		float dist = std::sqrt(dist2);

		auto contact = closest;
		auto normal = ( dist > eps ) ? ( delta / dist ) : ::triangleNormal( tri );
		float penetration = r - dist;

	#if REORIENT_NORMALS_ON_CONTACT
		if ( uf::vector::dot(normal, delta) < 0.0f ) normal = -normal;
	#endif

		manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
		return true;
	}
	// to-do
	bool trianglePlane( const pod::TriangleWithNormal& tri, const pod::RigidBody& body, pod::Manifold& manifold, float eps ) {
		return false;
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
		auto contact = closest;
		auto normal = ( dist > eps ) ? ( delta / dist ) : ::triangleNormal( tri );
		float penetration = r - dist;

	#if REORIENT_NORMALS_ON_CONTACT
		if ( uf::vector::dot(normal, delta) < 0.0f ) normal = -normal;
	#endif

		manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
		return true;
	}
}