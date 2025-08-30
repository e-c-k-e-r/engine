#define REORIENT_NORMALS_ON_FETCH 0
#define REORIENT_NORMALS_ON_CONTACT 0

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

		if ( normals.valid() && false ) {
			auto* base = reinterpret_cast<const uint8_t*>(normals.data(found->vertex.first));
			size_t stride = normals.stride();
			for ( auto i = 0; i < 3; ++i ) tri.normals[i] = *reinterpret_cast<const pod::Vector3f*>(base + idxs[i] * stride);
		} else {
			auto normal = uf::vector::normalize(uf::vector::cross(tri.points[1]-tri.points[0], tri.points[2]-tri.points[0]));
			for ( auto i = 0; i < 3; ++i ) tri.normals[i] = normal;
		}

		return tri;
	}

	// if body is a mesh, apply its transform to the triangles, else reorient the normal with respect to the body
	pod::TriangleWithNormal fetchTriangle( const uf::Mesh& mesh, size_t triID, const pod::RigidBody& body ) {
		auto tri = ::fetchTriangle( mesh, triID );
		if ( body.collider.type == pod::ShapeType::MESH ) {
			if ( body.transform ) {
				for ( auto i = 0; i < 3; ++i ) {
					tri.points[i]  = uf::transform::apply(*body.transform, tri.points[i]);
					tri.normals[i] = uf::quaternion::rotate(body.transform->orientation, tri.normals[i]);
				}
			}
		}
		else {
		#if REORIENT_NORMALS_ON_FETCH
			auto triCenter = (tri.points[0] + tri.points[1] + tri.points[2]) / 3.0f;
			auto delta = body.transform->position - triCenter;
			for ( auto i = 0; i < 3; ++i ) {
				if ( uf::vector::dot(tri.normals[i], delta) < 0.0f ) tri.normals[i] = -tri.normals[i];
			}
		#endif
		}

		return tri;
	}
}

// 
namespace {
	bool meshAabb( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( MESH, AABB );

		const auto& mesh = a;
		const auto& aabb = b;

		const auto& meshData = *mesh.collider.u.mesh.mesh;
		const auto& bvh  = *mesh.collider.u.mesh.bvh;

		auto bounds = ::transformAabbToLocal( aabb.bounds, *mesh.transform );

		uf::stl::vector<int> candidates;
		::queryBVH(bvh, bounds, candidates);

		bool hit = false;
		for ( auto triID : candidates ) {
			auto tri = ::fetchTriangle( meshData, triID, aabb );
			auto closestTri = ::closestPointOnTriangle( ::aabbCenter( bounds ), tri );
			auto closestAabb = ::closestPointOnAABB( closestTri, bounds );

			if ( !uf::vector::isValid( closestTri ) ) {
				//UF_MSG_DEBUG("tri #{}={}, {}, {}", triID, uf::vector::toString( tri.points[0] ), uf::vector::toString( tri.points[1] ), uf::vector::toString( tri.points[2] ));
				//UF_MSG_DEBUG("closestTri={}, closestAabb={}", uf::vector::toString( closestTri ), uf::vector::toString( closestAabb ));
				continue;
			}
			
			auto delta = closestAabb - closestTri; // direction between points
			float dist2 = uf::vector::magnitude( delta );
			float tolerance = 1.0e-3; // std::max( 1.0e-3f, uf::vector::magnitude(aabb.velocity) * manifold.dt * 0.5f );
			if ( dist2 >= tolerance ) continue;
			float dist = std::sqrt( dist2 );

			auto contact = closestTri; // ( closestAabb + closestTri ) * 0.5f; // center of points
			auto normal = ::normalizeDelta( delta, dist, eps ); // uf::vector::normalize( ::interpolateWithBarycentric( tri.normals, ::computeBarycentric( contact, tri ) ));
			float penetration = tolerance - dist;

		#if REORIENT_NORMALS_ON_CONTACT
			if ( uf::vector::dot(normal, delta) < 0.0f ) normal = -normal;
		#endif

			contact = uf::transform::apply( *mesh.transform, contact );
			normal  = uf::quaternion::rotate( mesh.transform->orientation, normal );

			manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
			hit = true;
		}
		return hit;
	}
	bool meshSphere( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES(MESH, SPHERE);

		const auto& mesh = a;
		const auto& sphere = b;

		const auto& meshData = *mesh.collider.u.mesh.mesh;
		const auto& bvh  = *mesh.collider.u.mesh.bvh;

		auto bounds = ::transformAabbToLocal( ::computeAABB( sphere ), *mesh.transform );
		
		auto center = ::aabbCenter( bounds );
		float r = sphere.collider.u.sphere.radius;
		
		uf::stl::vector<int> candidates;
		::queryBVH(bvh, bounds, candidates);

		bool hit = false;
		for ( auto triID : candidates ) {
			auto tri = ::fetchTriangle( meshData, triID, sphere );
			auto closest = ::closestPointOnTriangle( ::aabbCenter(bounds), tri );

			if ( !uf::vector::isValid( closest ) ) {
				//UF_MSG_DEBUG("tri #{}={}, {}, {}", triID, uf::vector::toString( tri.points[0] ), uf::vector::toString( tri.points[1] ), uf::vector::toString( tri.points[2] ));
				//UF_MSG_DEBUG("closest={}", uf::vector::toString( closest ));
				continue;
			}

			auto delta = center - closest;
			float dist2 = uf::vector::magnitude(delta);
			if ( dist2 > r * r ) continue;
			float dist = std::sqrt( dist2 );

			auto contact = closest;
			auto normal = ::normalizeDelta( delta, dist, eps );
			float penetration = r - dist;

		#if REORIENT_NORMALS_ON_CONTACT
			if ( uf::vector::dot(normal, delta) < 0.0f ) normal = -normal;
		#endif

			contact = uf::transform::apply(*mesh.transform, contact);
			normal  = uf::quaternion::rotate(mesh.transform->orientation, normal);

			manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
			hit = true;
		}

		return hit;
	}
	// to-do
	bool meshPlane( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES(MESH, PLANE);
		return false;
	}
	bool meshCapsule( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES(MESH, CAPSULE);

		const auto& mesh = a;
		const auto& capsule = b;

		const auto& meshData = *mesh.collider.u.mesh.mesh;
		const auto& bvh  = *mesh.collider.u.mesh.bvh;

		// capsule line segment in world space
		auto [ p1, p2 ] = ::getCapsuleSegment(b);
		float r = b.collider.u.capsule.radius;
		
		auto bounds = ::transformAabbToLocal( ::computeSegmentAABB(p1, p2, r), *a.transform );

		uf::stl::vector<int> candidates;
		::queryBVH(bvh, bounds, candidates);

		// for some reason (the segment points), it's just easier to transform the triangle to world space, than compare in local then transform the contact to world
		bool hit = false;
		for ( auto triID : candidates ) {
			auto tri = ::fetchTriangle( meshData, triID, mesh );

			pod::Vector3f closestSeg, closestTri;
			float dist2 = ::segmentTriangleDistanceSq( p1, p2, tri, closestSeg, closestTri );

			if ( !uf::vector::isValid( closestTri ) ) {
				//UF_MSG_DEBUG("tri #{}={}, {}, {}", triID, uf::vector::toString( tri.points[0] ), uf::vector::toString( tri.points[1] ), uf::vector::toString( tri.points[2] ));
				//UF_MSG_DEBUG("closestTri={}", uf::vector::toString( closestTri ));
				continue;
			}

			if ( dist2 > r * r ) continue;
			float dist = std::sqrt( dist2 );
			auto delta = ( closestSeg - closestTri );

			auto contact = closestTri; // ( closestSeg + closestTri ) * 0.5f; // center of points
			auto normal = ::normalizeDelta( delta, dist, eps ); // uf::vector::normalize( ::interpolateWithBarycentric( tri.normals, ::computeBarycentric( contact, tri ) ));
			float penetration = r - dist;

		#if REORIENT_NORMALS_ON_CONTACT
			if ( uf::vector::dot(normal, delta) < 0.0f ) normal = -normal;
		#endif

			manifold.points.emplace_back(pod::Contact{ contact, normal, penetration });
			hit = true;
		}

		return hit;
	}
	// to-do
	bool meshMesh( const pod::RigidBody& a, const pod::RigidBody& b, pod::Manifold& manifold, float eps ) {
		ASSERT_COLLIDER_TYPES( MESH, MESH );

		auto& bodyA = a;
		auto& bvhA = *a.collider.u.mesh.bvh;
		auto& meshA = *a.collider.u.mesh.mesh;
		
		auto& bodyB = b;
		auto& meshB = *b.collider.u.mesh.mesh;
		auto& bvhB = *b.collider.u.mesh.bvh;

		pod::BVH::pair_t pairs;
		::traverseNodePair(bvhA, 0, bvhB, 0, pairs);

		for (auto [idA, idB] : pairs) {
			auto tA = ::fetchTriangle( meshA, idA, bodyA );
			auto tB = ::fetchTriangle( meshB, idB, bodyB );

			// narrowphase tri-tri overlap: SAT
			if ( ::triangleTriangleIntersect( tA, tB ) ) {
				pod::Vector3f contact = ( tA.points[0] + tB.points[0] )*0.5f;
				pod::Vector3f n = uf::vector::normalize( uf::vector::cross( tA.points[1] - tA.points[0], tA.points[2] - tA.points[0] ) );
				manifold.points.emplace_back(pod::Contact{ contact, n, 0.001f });
			}
		}
		return !manifold.points.empty();
	}
}