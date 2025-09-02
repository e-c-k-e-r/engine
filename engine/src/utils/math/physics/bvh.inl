// BVH
namespace {
	// collects a list of nodes that are overlapping with each other
	void traverseNodePair( const pod::BVH& bvh, int indexA, int indexB, pod::BVH::pair_t& pairs ) {
		const auto& nodeA = bvh.nodes[indexA];
		const auto& nodeB = bvh.nodes[indexB];

		if ( !::aabbOverlap( nodeA.bounds, nodeB.bounds ) ) return;

		if ( nodeA.count > 0 && nodeB.count > 0 ) {
			for ( auto i = 0; i < nodeA.count; ++i )
				for ( auto j = 0; j < nodeB.count; ++j ) {
					int indexA = bvh.indices[nodeA.start + i];
					int indexB = bvh.indices[nodeB.start + j];
					pairs.emplace_back(std::pair{indexA, indexB});
				}
			return;
		}

		if ( nodeA.count == 0 ) {
			::traverseNodePair( bvh, nodeA.left, indexB, pairs );
			::traverseNodePair( bvh, nodeA.right, indexB, pairs );
		} else if ( nodeB.count == 0 ) {
			::traverseNodePair( bvh, indexA, nodeB.left, pairs );
			::traverseNodePair( bvh, indexA, nodeB.right, pairs );
		}
	}
	// collects a list of nodes from each BVH that are overlapping with each other (for mesh v mesh)
	void traverseNodePair( const pod::BVH& bvhA, int indexA, const pod::BVH& bvhB, int indexB, pod::BVH::pair_t& pairs ) {
		const auto& nodeA = bvhA.nodes[indexA];
		const auto& nodeB = bvhB.nodes[indexB];

		if ( !::aabbOverlap( nodeA.bounds, nodeB.bounds ) ) return;

		if ( nodeA.count > 0 && nodeB.count > 0 ) {
			for ( auto i = 0; i < nodeA.count; i++ ) {
				for ( auto j = 0; j < nodeB.count; j++ ) {
					auto indexA = bvhA.indices[nodeA.start+i];
					auto indexB = bvhB.indices[nodeB.start+j];
					pairs.emplace_back(std::pair{indexA, indexB});
				}
			}
			return;
		}

		if ( nodeA.count == 0 ) {
			::traverseNodePair( bvhA, nodeA.left, bvhB , indexB, pairs );
			::traverseNodePair( bvhA, nodeA.right, bvhB , indexB, pairs );
		} else if ( nodeB.count == 0 ) {
			::traverseNodePair( bvhA, indexA, bvhB, nodeB.left, pairs );
			::traverseNodePair( bvhA, indexA, bvhB, nodeB.right, pairs );
		}
	}

	void traverseBVH( const pod::BVH& bvh, int nodeIdx, pod::BVH::pair_t& pairs ) {
		const auto& node = bvh.nodes[nodeIdx];

		if ( node.count > 0 ) {
			for ( auto i = 0; i < node.count; ++i ) {
				 for ( auto j = i + 1; j < node.count; ++j ) {
					int indexA = bvh.indices[node.start + i];
					int indexB = bvh.indices[node.start + j];

					if ( !::aabbOverlap( bvh.nodes[indexA].bounds, bvh.nodes[indexB].bounds ) ) {
						continue;
					}

					pairs.emplace_back(std::pair{indexA, indexB});
				 }
			}
			return;
		}

		// recurse children
		::traverseNodePair( bvh, node.left, node.right, pairs );
	}

	void queryOverlaps( const pod::BVH& bvh, pod::BVH::pair_t& outPairs ) {
		if ( bvh.nodes.empty() ) return;
		::traverseBVH( bvh, 0, outPairs );
	}	

	int buildBVHNode( pod::BVH& bvh, const uf::stl::vector<pod::AABB>& bounds, int start, int end, int capacity = 2 ) {
		pod::BVH::Node node{};
		node.left  = -1;
		node.right = -1;
		node.start = start;
		node.count = 0;
		node.bounds = bounds[bvh.indices[start]];

		// compute bounds of this node
		for ( auto i = start + 1; i < end; ++i) node.bounds = ::mergeAabb( node.bounds, bounds[bvh.indices[i]] );

		int count = end - start;
		if ( count <= capacity ) {
			// leaf
			node.start = start;
			node.count = count;
			int index = (int) bvh.nodes.size();
			bvh.nodes.emplace_back(node);
			return index;
		}

		// choose split axis by largest extent
		auto extent = node.bounds.max - node.bounds.min;
		int axis = (extent.x > extent.y && extent.x > extent.z) ? 0 : (extent.y > extent.z ? 1 : 2);

		// sort indices by centroid along axis
		std::sort( bvh.indices.begin() + start, bvh.indices.begin() + end, [&](size_t a, size_t b) {
			float ca = ::aabbCenter( bounds[a] )[axis];
			float cb = ::aabbCenter( bounds[b] )[axis];
			return ca < cb;
		});

		int mid = ( start + end ) / 2;
		int index = (int) bvh.nodes.size();
		bvh.nodes.emplace_back( node ); // insert now, gets filled later

		node.left = ::buildBVHNode( bvh, bounds, start, mid, capacity );
		node.right = ::buildBVHNode( bvh, bounds, mid, end, capacity );
		bvh.nodes[index] = node;
		return index;
	}

	void buildBroadphaseBVH( pod::BVH& bvh, const uf::stl::vector<pod::PhysicsBody*>& bodies, int capacity = 2 ) {
		bvh.indices.clear();
		bvh.nodes.clear();
		bvh.indices.reserve(bodies.size());

		// stores bounds
		uf::stl::vector<pod::AABB> bounds;
		bounds.reserve(bodies.size());

		// populate initial indices and bounds
		for ( auto i = 0; i < bodies.size(); ++i ) {
			bounds.emplace_back(::computeAABB( *bodies[i] ));
			bvh.indices.emplace_back(i); // i => bodies[i]
		}

		// recursively build BVH from indices
		::buildBVHNode( bvh, bounds, 0, bodies.size(), capacity );
	}

	void buildMeshBVH( pod::BVH& bvh, const uf::Mesh& mesh, int capacity = 4 ) {
		int triangles = mesh.index.count / 3;

		bvh.indices.clear();
		bvh.nodes.clear();
		bvh.indices.reserve( triangles );

		// stores bounds
		uf::stl::vector<pod::AABB> bounds;
		bounds.reserve( triangles );

		auto views = mesh.makeViews({"position"});
		UF_ASSERT( !views.empty() );

		// populate initial indices and bounds
		for ( auto& view : views ) {
			auto& indices   = view["index"];
			auto& positions = view["position"];

			auto tris = view.index.count / 3;
			for ( auto triIndexID = 0; triIndexID < tris; ++triIndexID ) {
				auto aabb = ::computeTriangleAABB( positions.data(view.vertex.first), positions.stride(), indices.data(view.index.first), mesh.index.size, triIndexID );
				auto triID = triIndexID + (view.index.first / 3);

				bounds.emplace_back(aabb);
				bvh.indices.emplace_back(triID); // triID => mesh.index.buffer[triID * 3];
			}
		}
		UF_MSG_DEBUG("Built BVH with {} triangles from mesh with {} triangles ({} draw commands)", bounds.size(), triangles, views.size());
		
		// recursively build BVH from indices
		::buildBVHNode( bvh, bounds, 0, triangles, capacity );
	}
}

namespace {
	// query a BVH with an AABB via a stack
	void queryBVH( const pod::BVH& bvh, const pod::AABB& bounds, uf::stl::vector<int>& indices ) {
		if ( bvh.nodes.empty() ) return;

		uf::stl::stack<int> stack;
		stack.push(0);

		while ( !stack.empty() ) {
			int idx = stack.top(); stack.pop();
			auto& node = bvh.nodes[idx];
			if ( !::aabbOverlap( bounds, node.bounds ) ) continue;

			if ( node.count > 0 ) {
				for ( auto i = 0; i < node.count; ++i) indices.emplace_back(bvh.indices[node.start + i]);
			} else {
				stack.push(node.left);
				stack.push(node.right);
			}
		}
	}
	void queryBVH( const pod::BVH& bvh, const pod::PhysicsBody& body, uf::stl::vector<int>& indices ) {
		return ::queryBVH( bvh, body.bounds, indices );
	}
	
	// query a BVH with an AABB via recursion
	void queryBVH( const pod::BVH& bvh, const pod::AABB& bounds, uf::stl::vector<int>& indices, int nodeID ) {
		const auto& node = bvh.nodes[nodeID];
		if ( !::aabbOverlap( node.bounds, bounds ) ) return;

		if ( node.count > 0 ) {
			for ( auto i = 0; i < node.count; ++i ) indices.emplace_back(bvh.indices[node.start + i]);
			return;
		}

		// recurse
		::queryBVH( bvh, bounds, indices, node.left );
		::queryBVH( bvh, bounds, indices, node.right );
	}

	// query a BVH with a ray via a stack
	void queryBVH( const pod::BVH& bvh, const pod::Ray& ray, uf::stl::vector<int>& indices, float maxDist ) {
		if ( bvh.nodes.empty() ) return;

		uf::stl::stack<int> stack;
		stack.push(0);

		while ( !stack.empty() ) {
			int idx = stack.top(); stack.pop();
			const auto& node = bvh.nodes[idx];

			float tMin, tMax;
			if ( !::rayAabbIntersect( ray, node.bounds, tMin, tMax ) ) continue;
			if ( tMin > maxDist ) continue;

			if ( node.count > 0 ) {
				for ( auto i = 0; i < node.count; ++i) indices.emplace_back(bvh.indices[node.start + i]);
			} else {
				stack.push(node.left);
				stack.push(node.right);
			}
		}
	}
	// query a BVH with a ray via recursion
	void queryBVH( const pod::BVH& bvh, const pod::Ray& ray, uf::stl::vector<int>& indices, int nodeID, float maxDist ) {
		const auto& node = bvh.nodes[nodeID];
		float tMin, tMax;
		if ( !::rayAabbIntersect( ray, node.bounds, tMin, tMax ) ) return;
		if ( tMin > maxDist ) return;

		if ( node.count > 0 ) {
			for ( auto i = 0; i < node.count; ++i ) indices.emplace_back(bvh.indices[node.start + i]);
			return;
		}

		// recurse
		::queryBVH( bvh, ray, indices, node.left, maxDist );
		::queryBVH( bvh, ray, indices, node.right, maxDist );
	}
}
