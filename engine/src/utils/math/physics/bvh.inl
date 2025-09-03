// BVH
namespace {
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
		std::sort( bvh.indices.begin() + start, bvh.indices.begin() + end, [&](uint32_t a, uint32_t b) {
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

	int buildBVHNode_SAH( pod::BVH& bvh, const uf::stl::vector<pod::AABB>& bounds, int start, int end, int capacity = 4 ) {
		struct Bin {
			pod::AABB bounds;
			int count = 0;
		};

		pod::BVH::Node node{};
		node.left  = -1;
		node.right = -1;
		node.start = start;
		node.count = 0;
		node.bounds = bounds[bvh.indices[start]];

		for ( auto i = start + 1; i < end; ++i ) node.bounds = ::mergeAabb( node.bounds, bounds[bvh.indices[i]] );

		int count = end - start;
		if ( count <= capacity ) {
			node.count = count;
			int index = (int) bvh.nodes.size();
			bvh.nodes.emplace_back(node);
			return index;
		}

		constexpr int numBins = 16;
		static thread_local Bin bins[numBins];
		for ( auto i = 0; i < numBins; i++ ) bins[i] = {};

		auto extent = node.bounds.max - node.bounds.min;
		int bestAxis = -1, bestSplit = -1;
		float bestCost = std::numeric_limits<float>::infinity();

		for ( auto axis = 0; axis < 3; ++axis ) {
			if ( extent[axis] < EPS(1e-6f) ) continue;

			float minC = node.bounds.min[axis];
			float maxC = node.bounds.max[axis];
			float scale = (float) numBins / (maxC - minC);

			for ( auto i = start; i < end; ++i ) {
				int idx = bvh.indices[i];
				float c = ::aabbCenter( bounds[idx] )[axis];
				int binID = std::min(numBins - 1, (int)((c - minC) * scale));
				bins[binID].count++;
				bins[binID].bounds = ::mergeAabb( bins[binID].bounds, bounds[idx] );
			}

			pod::AABB leftBounds[numBins], rightBounds[numBins];
			int leftCount[numBins] = {}, rightCount[numBins] = {};

			pod::AABB acc;
			int cnt = 0;
			for ( auto i = 0; i < numBins; i++ ) {
				if ( bins[i].count > 0 ) acc = (cnt == 0) ? bins[i].bounds : ::mergeAabb( acc, bins[i].bounds );
				cnt += bins[i].count;
				leftBounds[i] = acc;
				leftCount[i] = cnt;
			}

			acc = {};
			cnt = 0;
			for ( auto i = numBins - 1; i >= 0; i-- ) {
				if ( bins[i].count > 0 ) acc = (cnt == 0) ? bins[i].bounds : ::mergeAabb( acc, bins[i].bounds );
				cnt += bins[i].count;
				rightBounds[i] = acc;
				rightCount[i] = cnt;
			}

			float parentArea = ::aabbSurfaceArea(node.bounds);
			for ( auto i = 0; i < numBins - 1; i++ ) {
				if ( leftCount[i] == 0 || rightCount[i + 1] == 0 ) continue;
				float cost = 1.0f + (
					( ::aabbSurfaceArea(leftBounds[i]) / parentArea ) * leftCount[i] +
					( ::aabbSurfaceArea(rightBounds[i + 1]) / parentArea ) * rightCount[i + 1]
				);
				if ( cost < bestCost ) {
					bestCost = cost;
					bestAxis = axis;
					bestSplit = i;
				}
			}
		}

		// fallback: no valid split → make leaf
		if ( bestAxis == -1 ) {
			node.count = count;
			int index = (int) bvh.nodes.size();
			bvh.nodes.emplace_back(node);
			return index;
		}

		float minC = node.bounds.min[bestAxis];
		float maxC = node.bounds.max[bestAxis];
		float scale = (float) numBins / (maxC - minC);

		auto midIt = std::partition( bvh.indices.begin() + start, bvh.indices.begin() + end, [&](int idx) {
			float c = ::aabbCenter( bounds[idx])[bestAxis ];
			int binID = std::min(numBins - 1, (int)((c - minC) * scale));
			return binID <= bestSplit;
		});

		int mid = (int) ( midIt - bvh.indices.begin() );

		// if partition failed (all left or all right), force leaf
		if ( mid == start || mid == end ) {
			node.count = count;
			int index = (int) bvh.nodes.size();
			bvh.nodes.emplace_back(node);
			return index;
		}

		int index = (int) bvh.nodes.size();
		bvh.nodes.emplace_back(node);

		node.left  = ::buildBVHNode_SAH( bvh, bounds, start, mid, capacity );
		node.right = ::buildBVHNode_SAH( bvh, bounds, mid, end, capacity );
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
		if ( ::useBvhSahBodies ) ::buildBVHNode_SAH( bvh, bounds, 0, bodies.size(), capacity );
		else ::buildBVHNode( bvh, bounds, 0, bodies.size(), capacity );
		// flatten if requested
		if ( ::flattenBvhBodies ) ::flattenBVH( bvh, 0 );

		// mark as clean
		bvh.dirty = false;
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

				if ( triID != bounds.size() ) UF_MSG_DEBUG("triID={}, bounds.size()={}", triID, bounds.size());

				bounds.emplace_back( aabb );
				bvh.indices.emplace_back( triID ); // triID => mesh.index.buffer[triID * 3];
			}
		}

		// recursively build BVH from indices
		if ( ::useBvhSahMeshes ) ::buildBVHNode_SAH( bvh, bounds, 0, triangles, capacity );
		else ::buildBVHNode( bvh, bounds, 0, triangles, capacity );
		// flatten if requested
		if ( ::flattenBvhMeshes ) ::flattenBVH( bvh, 0 );

		// mark as clean
		bvh.dirty = false;
	}
}

namespace {
	pod::BVH::UpdatePolicy::Decision decideBVHUpdate( const pod::BVH& bvh, const uf::stl::vector<pod::PhysicsBody*>& bodies, const pod::BVH::UpdatePolicy& policy, int frameCounter ) {
		if ( bvh.indices.empty() || bvh.nodes.empty() || bvh.dirty ) return pod::BVH::UpdatePolicy::Decision::REBUILD;
		if ( bodies.empty() ) return pod::BVH::UpdatePolicy::Decision::NONE;

		int dirtyCount = 0;
		float oldRootArea = ::aabbSurfaceArea( bvh.nodes[0].bounds );

		// check each body
		for ( const auto* body : bodies ) {
			pod::AABB newBounds = ::computeAABB(*body);
			pod::AABB oldBounds = body->bounds;

			// compute displacement relative to size
			pod::Vector3f oldCenter = ( oldBounds.min + oldBounds.max ) * 0.5f;
			pod::Vector3f newCenter = ( newBounds.min + newBounds.max ) * 0.5f;
			float displacement = uf::vector::distance( newCenter, oldCenter );

			pod::Vector3f extent = oldBounds.max - oldBounds.min;
			float size = std::max({extent.x, extent.y, extent.z, 1e-6f});

			if ( displacement > policy.displacementThreshold * size ) ++dirtyCount;
		}

		float dirtyRatio = (float) dirtyCount / (float) bodies.size();

		// compute new root bounds
		pod::AABB newRoot = bodies[0]->bounds;
		for ( auto i = 1; i < bodies.size(); ++i ) {
			newRoot = ::mergeAabb(newRoot, bodies[i]->bounds);
		}

		float newRootArea = ::aabbSurfaceArea( newRoot );
		if ( dirtyRatio > policy.dirtyRatioThreshold || newRootArea > oldRootArea * policy.overlapThreshold || frameCounter % policy.maxFramesBeforeRebuild == 0 ) {
			return pod::BVH::UpdatePolicy::Decision::REBUILD;
		}
		if ( dirtyCount > 0 ) return pod::BVH::UpdatePolicy::Decision::REFIT;
		return pod::BVH::UpdatePolicy::Decision::NONE;
	}
}

namespace {
	void refitBVH( pod::BVH& bvh, const uf::stl::vector<pod::AABB>& bounds ) {
		if ( bvh.nodes.empty() ) return;

		// update leaf bounds
		#pragma omp parallel for
		for ( auto i = 0; i < bvh.nodes.size(); i++ ) {
			auto& node = bvh.nodes[i];
			if ( node.count > 0 ) {
				// leaf node: recompute bounds from bodies
				node.bounds = bounds[bvh.indices[node.start]];

				for ( int j = 1; j < node.count; j++ ) {
					node.bounds = ::mergeAabb(node.bounds, bounds[bvh.indices[node.start + j]] );
				}
			}
		}

		// update internal nodes bottom-up
		for ( int i = (int) bvh.nodes.size() - 1; i >= 0; i-- ) {
			auto& node = bvh.nodes[i];
			// internal node
			if ( node.count == 0 ) {
				node.bounds = ::mergeAabb(bvh.nodes[node.left].bounds, bvh.nodes[node.right].bounds);
			}
		}
	}
	
	// avoids creating a vector for bounds
	void refitBVH( pod::BVH& bvh, const uf::stl::vector<pod::PhysicsBody*>& bodies ) {
		if ( bvh.nodes.empty() ) return;

		// update leaf bounds
		#pragma omp parallel for
		for ( auto i = 0; i < bvh.nodes.size(); i++ ) {
			auto& node = bvh.nodes[i];
			if ( node.count > 0 ) {
				// leaf node: recompute bounds from bodies
				auto nodeID = bvh.indices[node.start];

				node.bounds = ::computeAABB( *bodies[nodeID] );
				node.asleep = !bodies[nodeID]->activity.awake;

				for ( int j = 1; j < node.count; j++ ) {
					auto bodyID = bvh.indices[node.start + j];
					node.bounds = ::mergeAabb(node.bounds, ::computeAABB( *bodies[bodyID] ) );
					node.asleep = node.asleep && !bodies[bodyID]->activity.awake;
				}
			}
		}

		// update internal nodes bottom-up
		for ( int i = (int) bvh.nodes.size() - 1; i >= 0; i-- ) {
			auto& node = bvh.nodes[i];
			// internal node
			if ( node.count == 0 ) {
				const auto& leftNode = bvh.nodes[node.left];
				const auto& rightNode = bvh.nodes[node.right];
				node.bounds = ::mergeAabb(leftNode.bounds, rightNode.bounds);
				node.asleep = leftNode.asleep && rightNode.asleep;
			}
		}
	}

	void refitBVH( pod::BVH& bvh, const uf::Mesh& mesh ) {
		int triangles = mesh.index.count / 3;

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
				bounds.emplace_back(aabb);
			}
		}
		
		// recursively build BVH from indices
		::refitBVH( bvh, bounds );
	}
}

namespace {
	int flattenBVH( pod::BVH& bvh, int nodeID ) {
		if ( nodeID == 0 ) bvh.flattened.reserve(bvh.nodes.size());

		const auto& node = bvh.nodes[nodeID];

		int flatID = (int) bvh.flattened.size();
		bvh.flattened.emplace_back(); // placeholder

		pod::BVH::FlatNode flat{};
		flat.bounds = node.bounds;
		flat.start = -1;
		flat.count = -1;
		flat.skipIndex = -1;
		flat.asleep = node.asleep;

		// leaf
		if ( node.count > 0 ) {
			flat.start = node.start;
			flat.count = node.count;
			flat.skipIndex = flatID + 1; // next node after this leaf
			bvh.flattened[flatID] = flat;
			return flatID + 1;
		}
		// internal
		else {
			flat.start = -1;
			flat.count = 0;

			int leftID  = ::flattenBVH( bvh, node.left );
			int rightID = ::flattenBVH( bvh, node.right );

			flat.skipIndex = rightID; // skip entire subtree
			bvh.flattened[flatID] = flat;
			return rightID;
		}
	}
}

namespace {
	// collects a list of nodes that are overlapping with each other
	void traverseNodePair(const pod::BVH& bvh, int nodeAID, int nodeBID, pod::BVH::pairs_t& pairs) {
		const auto& nodeA = bvh.nodes[nodeAID];
		const auto& nodeB = bvh.nodes[nodeBID];

		if ( nodeA.asleep || nodeB.asleep || !::aabbOverlap( nodeA.bounds, nodeB.bounds ) ) return;

		if ( nodeA.count > 0 && nodeB.count > 0 ) {
			for ( auto i = 0; i < nodeA.count; ++i ) {
				for ( auto j = 0; j < nodeB.count; ++j ) {
					int bodyA = bvh.indices[nodeA.start + i];
					int bodyB = bvh.indices[nodeB.start + j];
					if ( bodyA == bodyB ) continue;
					if ( bodyA > bodyB ) std::swap( bodyA, bodyB );

					pairs.emplace_back(bodyA, bodyB);
				}
			}
			return;
		}

		if ( nodeA.count == 0 ) {
			::traverseNodePair( bvh, nodeA.left, nodeBID, pairs );
			::traverseNodePair( bvh, nodeA.right, nodeBID, pairs );
		}
		if ( nodeB.count == 0 ) {
			::traverseNodePair( bvh, nodeAID, nodeB.left, pairs );
			::traverseNodePair( bvh, nodeAID, nodeB.right, pairs );
		}
	}
	// collects a list of nodes from each BVH that are overlapping with each other (for mesh v mesh)
	void traverseNodePair( const pod::BVH& bvhA, int nodeAID, const pod::BVH& bvhB, int nodeBID, pod::BVH::pairs_t& pairs ) {
		const auto& nodeA = bvhA.nodes[nodeAID];
		const auto& nodeB = bvhB.nodes[nodeBID];

		if ( nodeA.asleep || nodeB.asleep || !::aabbOverlap( nodeA.bounds, nodeB.bounds ) ) return;

		if ( nodeA.count > 0 && nodeB.count > 0 ) {
			for ( auto i = 0; i < nodeA.count; ++i ) {
				for ( auto j = 0; j < nodeB.count; ++j ) {
					int bodyA = bvhA.indices[nodeA.start + i];
					int bodyB = bvhB.indices[nodeB.start + j];

					pairs.emplace_back(bodyA, bodyB);
				}
			}
			return;
		}

		if ( nodeA.count == 0 ) {
			::traverseNodePair( bvhA, nodeA.left, bvhB, nodeBID, pairs );
			::traverseNodePair( bvhA, nodeA.right, bvhB, nodeBID, pairs );
		}
		if ( nodeB.count == 0 ) {
			::traverseNodePair( bvhA, nodeAID, bvhB, nodeB.left, pairs );
			::traverseNodePair( bvhA, nodeAID, bvhB, nodeB.right, pairs );
		}
	}

	void traverseBVH( const pod::BVH& bvh, int nodeID, pod::BVH::pairs_t& pairs ) {
		const auto& node = bvh.nodes[nodeID];

		if ( node.count > 0 ) {
			for ( auto i = 0; i < node.count; ++i ) {
				for ( auto j = i + 1; j < node.count; ++j ) {
					int bodyA = bvh.indices[node.start + i];
					int bodyB = bvh.indices[node.start + j];

					if ( bodyA == bodyB ) continue;
					if ( bodyA > bodyB ) std::swap( bodyA, bodyB );

					pairs.emplace_back(bodyA, bodyB);
				}
			}
			return;
		}

		::traverseNodePair( bvh, node.left, node.right, pairs );
	}

	void queryOverlaps( const pod::BVH& bvh, pod::BVH::pairs_t& outPairs ) {
		if ( !bvh.flattened.empty() ) return ::queryFlatOverlaps( bvh, outPairs );

		if ( bvh.nodes.empty() ) return;
		outPairs.reserve(::reserveCount);
		::traverseBVH( bvh, 0, outPairs );
	}

	void queryOverlaps( const pod::BVH& bvhA, const pod::BVH& bvhB, pod::BVH::pairs_t& outPairs ) {
		if ( !bvhA.flattened.empty() && !bvhB.flattened.empty() ) return ::queryFlatOverlaps( bvhA, bvhB, outPairs );

		if ( bvhA.nodes.empty() || bvhB.nodes.empty() ) return;
		outPairs.reserve(::reserveCount);
		::traverseNodePair(bvhA, 0, bvhB, 0, outPairs);
	}
}

namespace {
	// query a BVH with an AABB via a stack
	void queryBVH( const pod::BVH& bvh, const pod::AABB& bounds, uf::stl::vector<int32_t>& outIndices ) {
		if ( !bvh.flattened.empty() ) return ::queryFlatBVH( bvh, bounds, outIndices );

		outIndices.reserve(::reserveCount);

		if ( bvh.nodes.empty() ) return;

		uf::stl::stack<int32_t> stack;
		stack.push(0);

		while ( !stack.empty() ) {
			int idx = stack.top(); stack.pop();
			auto& node = bvh.nodes[idx];
			if ( node.asleep || !::aabbOverlap( bounds, node.bounds ) ) continue;

			if ( node.count > 0 ) {
				for ( auto i = 0; i < node.count; ++i) outIndices.emplace_back(bvh.indices[node.start + i]);
			} else {
				stack.push(node.left);
				stack.push(node.right);
			}
		}
	}
	void queryBVH( const pod::BVH& bvh, const pod::PhysicsBody& body, uf::stl::vector<int32_t>& outIndices ) {
		return ::queryBVH( bvh, body.bounds, outIndices );
	}
	
	// query a BVH with an AABB via recursion
	void queryBVH( const pod::BVH& bvh, const pod::AABB& bounds, uf::stl::vector<int32_t>& outIndices, int nodeID ) {
		if ( !bvh.flattened.empty() ) return ::queryFlatBVH( bvh, bounds, outIndices );

		if ( nodeID == 0 ) outIndices.reserve(::reserveCount);

		const auto& node = bvh.nodes[nodeID];
		if ( node.asleep || !::aabbOverlap( node.bounds, bounds ) ) return;

		if ( node.count > 0 ) {
			for ( auto i = 0; i < node.count; ++i ) outIndices.emplace_back(bvh.indices[node.start + i]);
			return;
		}

		// recurse
		::queryBVH( bvh, bounds, outIndices, node.left );
		::queryBVH( bvh, bounds, outIndices, node.right );
	}

	// query a BVH with a ray via a stack
	void queryBVH( const pod::BVH& bvh, const pod::Ray& ray, uf::stl::vector<int32_t>& outIndices, float maxDist ) {
		if ( !bvh.flattened.empty() ) return ::queryFlatBVH( bvh, ray, outIndices, maxDist );

		if ( bvh.nodes.empty() ) return;
		outIndices.reserve(::reserveCount);

		uf::stl::stack<int32_t> stack;
		stack.push(0);

		while ( !stack.empty() ) {
			int idx = stack.top(); stack.pop();
			const auto& node = bvh.nodes[idx];

			float tMin, tMax;
			if ( node.asleep || !::rayAabbIntersect( ray, node.bounds, tMin, tMax ) ) continue;
			if ( tMin > maxDist ) continue;

			if ( node.count > 0 ) {
				for ( auto i = 0; i < node.count; ++i) outIndices.emplace_back(bvh.indices[node.start + i]);
			} else {
				stack.push(node.left);
				stack.push(node.right);
			}
		}
	}
	// query a BVH with a ray via recursion
	void queryBVH( const pod::BVH& bvh, const pod::Ray& ray, uf::stl::vector<int32_t>& outIndices, int nodeID, float maxDist ) {
		if ( !bvh.flattened.empty() ) return ::queryFlatBVH( bvh, ray, outIndices, maxDist );

		if ( nodeID == 0 ) outIndices.reserve(::reserveCount);

		const auto& node = bvh.nodes[nodeID];
		float tMin, tMax;
		if ( node.asleep || !::rayAabbIntersect( ray, node.bounds, tMin, tMax ) ) return;
		if ( tMin > maxDist ) return;

		if ( node.count > 0 ) {
			for ( auto i = 0; i < node.count; ++i ) outIndices.emplace_back(bvh.indices[node.start + i]);
			return;
		}

		// recurse
		::queryBVH( bvh, ray, outIndices, node.left, maxDist );
		::queryBVH( bvh, ray, outIndices, node.right, maxDist );
	}
}


namespace {
	void queryFlatOverlaps( const pod::BVH& bvh, pod::BVH::pairs_t& outPairs ) {
		auto& nodes = bvh.flattened;
		auto& indices = bvh.indices;

		outPairs.clear();
		outPairs.reserve(::reserveCount);

		for ( auto i = 0; i < (int) nodes.size(); ++i ) {
			const auto& nodeA = nodes[i];
			if ( nodeA.count <= 0 || nodeA.asleep ) continue;

			for ( auto j = i + 1; j < (int) nodes.size(); ++j ) {
				const auto& nodeB = nodes[j];
				if ( nodeB.count <= 0 || nodeB.asleep ) continue;

				if ( !::aabbOverlap( nodeA.bounds, nodeB.bounds ) ) continue;

				for ( auto ia = 0; ia < nodeA.count; ++ia ) {
					for ( auto ib = 0; ib < nodeB.count; ++ib ) {
						auto indexA = indices[nodeA.start + ia];
						auto indexB = indices[nodeB.start + ib];

						if ( indexA == indexB ) continue;
						if ( indexA > indexB ) std::swap( indexA, indexB );

						outPairs.emplace_back( indexA, indexB );
					}
				}
			}
		}
	}
	void queryFlatOverlaps( const pod::BVH& bvhA, const pod::BVH& bvhB, pod::BVH::pairs_t& outPairs ) {
		auto& nodesA = bvhA.flattened;
		auto& indicesA = bvhA.indices;
		
		auto& nodesB = bvhB.flattened;
		auto& indicesB = bvhB.indices;

		if ( nodesA.empty() || nodesB.empty() ) return;
		
		outPairs.clear();
		outPairs.reserve(::reserveCount);

		for ( auto i = 0; i < (int) nodesA.size(); ++i ) {
			const auto& nodeA = nodesA[i];
			if ( nodeA.count <= 0 || nodeA.asleep ) continue;

			for ( auto j = 0; j < (int) nodesB.size(); ++j ) {
				const auto& nodeB = nodesB[j];
				if ( nodeB.count <= 0 || nodeB.asleep ) continue;

				if ( !::aabbOverlap( nodeA.bounds, nodeB.bounds ) ) continue;

				for ( auto ia = 0; ia < nodeA.count; ++ia ) {
					for (auto ib = 0; ib < nodeB.count; ++ib ) {
						auto indexA = indicesA[nodeA.start + ia];
						auto indexB = indicesB[nodeB.start + ib];

						outPairs.emplace_back( indexA, indexB );
					}
				}
			}
		}
	}

	void queryFlatBVH( const pod::BVH& bvh, const pod::AABB& bounds, uf::stl::vector<int32_t>& outIndices ) {
		auto& nodes = bvh.flattened;
		auto& indices = bvh.indices;

		outIndices.reserve(::reserveCount);

		int idx = 0;
		while ( idx < (int) nodes.size() ) {
			const auto& node = nodes[idx];

			if ( !node.asleep && ::aabbOverlap( bounds, node.bounds ) ) {
				// leaf
				if ( node.count > 0 ) {
					for ( int i = 0; i < node.count; ++i ) {
						outIndices.emplace_back( indices[node.start + i] );
					}
				}
				++idx;
			} else {
				// skip this subtree
				idx = node.skipIndex;
			}
		}
	}
	void queryFlatBVH( const pod::BVH& bvh, const pod::Ray& ray, uf::stl::vector<int32_t>& outIndices, float maxDist ) {
		auto& nodes = bvh.flattened;
		auto& indices = bvh.indices;

		outIndices.reserve(::reserveCount);

		int idx = 0;
		while ( idx < (int) nodes.size() ) {
			const auto& node = nodes[idx];
			float tMin, tMax;
			if ( !node.asleep && ::rayAabbIntersect( ray, node.bounds, tMin, tMax ) && tMin <= maxDist ) {
				// leaf
				if ( node.count > 0 ) {
					for ( int i = 0; i < node.count; ++i ) {
						outIndices.emplace_back( indices[node.start + i] );
					}
				}
				++idx;
			} else {
				// skip this subtree
				idx = node.skipIndex;
			}
		}
	}
}

namespace {
	void buildIslands( const pod::BVH::pairs_t& pairs, const uf::stl::vector<pod::PhysicsBody*>& bodies, uf::stl::vector<pod::Island>& islands ) {
		islands.reserve(::reserveCount);

		int n = (int) bodies.size();
		uf::stl::vector<int32_t> visited(n, -1);

		for ( auto i = 0; i < n; i++ ) {
			if ( visited[i] != -1 ) continue;

			// new island
			pod::Island island = {};
			uf::stl::stack<int32_t> stack;
			stack.push(i);

			while ( !stack.empty() ) {
				int idx = stack.top(); stack.pop();
				if ( visited[idx] != -1 ) continue;
				visited[idx] = (int) islands.size();

				island.bodies.emplace_back( bodies[idx] );

				// traverse neighbors
				for ( auto& [a, b] : pairs ) {
					int neighbor = -1;
					if ( a == idx ) neighbor = b;
					else if ( b == idx ) neighbor = a;
					if ( neighbor != -1 && visited[neighbor] == -1 ) {
						stack.push(neighbor);
					}
				}
			}

			islands.emplace_back( std::move( island ) );
		}
	}

	void updateIsland( pod::Island& island, float dt ) {
		bool allStill = true;

		for ( auto* b : island.bodies ) {
			auto& body = *b;
			if ( !body.activity.awake ) continue;

			float linSpeed = uf::vector::norm( body.velocity );
			float angSpeed = uf::vector::norm( body.angularVelocity );

			if ( linSpeed < pod::Activity::linearSleepEpsilon && angSpeed < pod::Activity::angularSleepEpsilon) {
				body.activity.sleepTimer += dt;
			} else {
				body.activity.sleepTimer = 0.0f;
				allStill = false;
			}

			if ( body.activity.sleepTimer < pod::Activity::sleepThreshold ) {
				allStill = false;
			}
		}

		// put entire island to sleep
		if ( allStill ) {
			island.awake = false;
			for ( auto* b : island.bodies ) ::sleepBody( *b );
		}
		// at least one body is awake
		else {
			for ( auto* b : island.bodies ) ::wakeBody( *b );
			island.awake = true;
		}
	}
}