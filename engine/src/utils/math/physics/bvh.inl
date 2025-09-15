namespace {
	int32_t flattenBVH( pod::BVH& bvh, int32_t nodeID );

	void queryFlatBVH( const pod::BVH&, const pod::AABB& bounds, uf::stl::vector<int32_t>& out );
	void queryFlatBVH( const pod::BVH&, const pod::Ray& ray, uf::stl::vector<int32_t>& out, float maxDist = FLT_MAX );
	
	void queryFlatOverlaps( const pod::BVH& bvh, pod::BVH::pairs_t& outPairs );
	void queryFlatOverlaps( const pod::BVH& bvhA, const pod::BVH& bvhB, pod::BVH::pairs_t& outPairs );
}

// BVH
namespace {
	int32_t buildBVHNode( pod::BVH& bvh, const uf::stl::vector<pod::AABB>& bounds, int32_t start, int32_t end, int32_t capacity = 2 ) {
		pod::BVH::Node node{};
		node.left  = -1;
		node.right = -1;
		node.start = start;
		node.count = 0;
		node.bounds = bounds[bvh.indices[start]];

		// compute bounds of this node
		for ( auto i = start + 1; i < end; ++i) node.bounds = ::mergeAabb( node.bounds, bounds[bvh.indices[i]] );

		int32_t count = end - start;
		if ( count <= capacity ) {
			// leaf
			node.start = start;
			node.count = count;
			int32_t index = (int32_t) bvh.nodes.size();
			bvh.nodes.emplace_back(node);
			return index;
		}

		// choose split axis by largest extent
		auto extent = node.bounds.max - node.bounds.min;
		auto axis = (extent.x > extent.y && extent.x > extent.z) ? 0 : (extent.y > extent.z ? 1 : 2);

		// sort indices by centroid along axis
		std::sort( bvh.indices.begin() + start, bvh.indices.begin() + end, [&](uint32_t a, uint32_t b) {
			float ca = ::aabbCenter( bounds[a] )[axis];
			float cb = ::aabbCenter( bounds[b] )[axis];
			return ca < cb;
		});

		int32_t mid = ( start + end ) / 2;
		int32_t index = (int32_t) bvh.nodes.size();
		bvh.nodes.emplace_back( node ); // insert now, gets filled later

		node.left = ::buildBVHNode( bvh, bounds, start, mid, capacity );
		node.right = ::buildBVHNode( bvh, bounds, mid, end, capacity );
		bvh.nodes[index] = node;
		return index;
	}

	int32_t buildBVHNode_SAH( pod::BVH& bvh, const uf::stl::vector<pod::AABB>& bounds, int32_t start, int32_t end, int32_t capacity = 4 ) {
		struct Bin {
			pod::AABB bounds;
			int32_t count = 0;
		};

		pod::BVH::Node node{};
		node.left  = -1;
		node.right = -1;
		node.start = start;
		node.count = 0;
		node.bounds = bounds[bvh.indices[start]];

		for ( auto i = start + 1; i < end; ++i ) node.bounds = ::mergeAabb( node.bounds, bounds[bvh.indices[i]] );

		int32_t count = end - start;
		if ( count <= capacity ) {
			node.count = count;
			int32_t index = (int32_t) bvh.nodes.size();
			bvh.nodes.emplace_back(node);
			return index;
		}

		constexpr auto numBins = 16;
		static thread_local Bin bins[numBins];
		for ( auto i = 0; i < numBins; i++ ) bins[i] = {};

		auto extent = node.bounds.max - node.bounds.min;
		auto bestAxis = -1, bestSplit = -1;
		float bestCost = std::numeric_limits<float>::infinity();

		for ( auto axis = 0; axis < 3; ++axis ) {
			if ( extent[axis] < EPS(1e-6f) ) continue;

			float minC = node.bounds.min[axis];
			float maxC = node.bounds.max[axis];
			float scale = (float) numBins / (maxC - minC);

			for ( auto i = start; i < end; ++i ) {
				int32_t idx = bvh.indices[i];
				float c = ::aabbCenter( bounds[idx] )[axis];
				int32_t binID = std::min(numBins - 1, (int32_t)((c - minC) * scale));
				bins[binID].count++;
				bins[binID].bounds = ::mergeAabb( bins[binID].bounds, bounds[idx] );
			}

			pod::AABB leftBounds[numBins], rightBounds[numBins];
			int32_t leftCount[numBins] = {}, rightCount[numBins] = {};

			pod::AABB acc;
			int32_t cnt = 0;
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
			int32_t index = (int32_t) bvh.nodes.size();
			bvh.nodes.emplace_back(node);
			return index;
		}

		float minC = node.bounds.min[bestAxis];
		float maxC = node.bounds.max[bestAxis];
		float scale = (float) numBins / (maxC - minC);

		auto midIt = std::partition( bvh.indices.begin() + start, bvh.indices.begin() + end, [&](int32_t idx) {
			float c = ::aabbCenter( bounds[idx])[bestAxis ];
			int32_t binID = std::min(numBins - 1, (int32_t)((c - minC) * scale));
			return binID <= bestSplit;
		});

		int32_t mid = (int32_t) ( midIt - bvh.indices.begin() );

		// if partition failed (all left or all right), force leaf
		if ( mid == start || mid == end ) {
			node.count = count;
			int32_t index = (int32_t) bvh.nodes.size();
			bvh.nodes.emplace_back(node);
			return index;
		}

		int32_t index = (int32_t) bvh.nodes.size();
		bvh.nodes.emplace_back(node);

		node.left  = ::buildBVHNode_SAH( bvh, bounds, start, mid, capacity );
		node.right = ::buildBVHNode_SAH( bvh, bounds, mid, end, capacity );
		bvh.nodes[index] = node;
		return index;
	}

	void buildBroadphaseBVH( pod::BVH& bvh, const uf::stl::vector<pod::PhysicsBody*>& bodies, int32_t capacity = 2, bool filters = false, bool filterType = false ) {
		if ( bodies.empty() ) return;

		bvh.indices.clear();
		bvh.nodes.clear();
		bvh.indices.reserve(bodies.size());

		// stores bounds
		uf::stl::vector<pod::AABB> bounds(bodies.size(), { {FLT_MAX, FLT_MAX, FLT_MAX}, {-FLT_MAX, -FLT_MAX, -FLT_MAX} });

		// populate initial indices and bounds
		for ( auto i = 0; i < bodies.size(); ++i ) {
			if ( filters && bodies[i]->isStatic != filterType ) continue;

			bounds[i] = bodies[i]->bounds;
			bvh.indices.emplace_back(i);
		}

		if ( bvh.indices.empty() ) return; // inserted nothing

		// recursively build BVH from indices
		if ( ::useBvhSahBodies ) ::buildBVHNode_SAH( bvh, bounds, 0, bvh.indices.size(), capacity );
		else ::buildBVHNode( bvh, bounds, 0, bvh.indices.size(), capacity );
		// flatten if requested
		if ( ::flattenBvhBodies ) ::flattenBVH( bvh, 0 );

		// mark as clean
		bvh.dirty = false;
	}

	void buildMeshBVH( pod::BVH& bvh, const uf::Mesh& mesh, int32_t capacity = 4 ) {
		uint32_t triangles = mesh.index.count / 3;

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
				auto tri = ::fetchTriangle( view, indices, positions, triIndexID );
				auto aabb = ::computeTriangleAABB( tri );
				auto triID = triIndexID + (view.index.first / 3);

				if ( triID != bounds.size() ) UF_MSG_DEBUG("triID={}, bounds.size()={}", triID, bounds.size());

				bounds.emplace_back( aabb );
				bvh.indices.emplace_back( triID ); // triID => mesh.index.buffer[triID * 3];
			}
		}

		// recursively build BVH from indices
		if ( ::useBvhSahMeshes ) ::buildBVHNode_SAH( bvh, bounds, 0, bvh.indices.size(), capacity );
		else ::buildBVHNode( bvh, bounds, 0, bvh.indices.size(), capacity );
		// flatten if requested
		if ( ::flattenBvhMeshes ) ::flattenBVH( bvh, 0 );

		// mark as clean
		bvh.dirty = false;
	}
}

namespace {
	pod::BVH::UpdatePolicy::Decision decideBVHUpdate( const pod::BVH& bvh, uf::stl::vector<pod::PhysicsBody*>& bodies, const pod::BVH::UpdatePolicy& policy, size_t frameCounter ) {
		// BVH is not built
		if ( bvh.indices.empty() || bvh.nodes.empty() ) {
			return pod::BVH::UpdatePolicy::Decision::REBUILD;
		}
		if ( bodies.empty() ) return pod::BVH::UpdatePolicy::Decision::NONE;

		uint32_t dirtyCount = 0;
		float oldRootArea = ::aabbSurfaceArea( bvh.nodes[0].bounds );

		// update/check each body
		for ( auto idx : bvh.indices ) {
			auto& body = *bodies[idx];

			pod::AABB oldBounds = body.bounds;
			body.bounds = ::computeAABB( body );
			pod::AABB newBounds = body.bounds;

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
		pod::AABB newRoot = bodies[bvh.indices[0]]->bounds;
		for ( auto i = 1; i < bvh.indices.size(); ++i ) {
			newRoot = ::mergeAabb(newRoot, bodies[bvh.indices[i]]->bounds);
		}

		float newRootArea = ::aabbSurfaceArea( newRoot );
		// BVH is too out of date, rebuild it
		if ( bvh.dirty || dirtyRatio > policy.dirtyRatioThreshold || newRootArea > oldRootArea * policy.overlapThreshold || frameCounter % policy.maxFramesBeforeRebuild == 0 ) {
			return pod::BVH::UpdatePolicy::Decision::REBUILD;
		}
		// bodies moved, refit the BVH instead
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

				for ( auto j = 1; j < node.count; j++ ) {
					node.bounds = ::mergeAabb(node.bounds, bounds[bvh.indices[node.start + j]] );
				}
			}
		}

		// update internal nodes bottom-up
		for ( int32_t i = (int32_t) bvh.nodes.size() - 1; i >= 0; i-- ) {
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

				node.bounds = bodies[nodeID]->bounds;
				node.asleep = !bodies[nodeID]->activity.awake;

				for ( auto j = 1; j < node.count; j++ ) {
					auto bodyID = bvh.indices[node.start + j];
					node.bounds = ::mergeAabb(node.bounds, bodies[bodyID]->bounds );
					node.asleep = node.asleep && !bodies[bodyID]->activity.awake;
				}
			}
		}

		// update internal nodes bottom-up
		for ( int32_t i = (int32_t) bvh.nodes.size() - 1; i >= 0; i-- ) {
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
		uint32_t triangles = mesh.index.count / 3;

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
				auto tri = ::fetchTriangle( view, indices, positions, triIndexID );
				auto aabb = ::computeTriangleAABB( tri );
				bounds.emplace_back(aabb);
			}
		}
		
		// recursively build BVH from indices
		::refitBVH( bvh, bounds );
	}
}

namespace {
	int32_t flattenBVH( pod::BVH& bvh, int32_t nodeID ) {
		if ( nodeID == 0 ) bvh.flattened.reserve(bvh.nodes.size());

		const auto& node = bvh.nodes[nodeID];

		int32_t flatID = (int32_t) bvh.flattened.size();
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

			int32_t leftID  = ::flattenBVH( bvh, node.left );
			int32_t rightID = ::flattenBVH( bvh, node.right );

			flat.skipIndex = rightID; // skip entire subtree
			bvh.flattened[flatID] = flat;
			return rightID;
		}
	}
}

namespace {
	// collects a list of nodes that are overlapping with each other
	void traverseNodePair(const pod::BVH& bvh, int32_t nodeAID, int32_t nodeBID, pod::BVH::pairs_t& pairs) {
		const auto& nodeA = bvh.nodes[nodeAID];
		const auto& nodeB = bvh.nodes[nodeBID];

		if ( nodeA.asleep || nodeB.asleep || !::aabbOverlap( nodeA.bounds, nodeB.bounds ) ) return;

		if ( nodeA.count > 0 && nodeB.count > 0 ) {
			for ( auto i = 0; i < nodeA.count; ++i ) {
				for ( auto j = 0; j < nodeB.count; ++j ) {
					int32_t bodyA = bvh.indices[nodeA.start + i];
					int32_t bodyB = bvh.indices[nodeB.start + j];
					if ( bodyA == bodyB ) continue;
					if ( bodyA > bodyB ) std::swap( bodyA, bodyB );

					pairs.emplace(bodyA, bodyB);
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
	void traverseNodePair( const pod::BVH& bvhA, int32_t nodeAID, const pod::BVH& bvhB, int32_t nodeBID, pod::BVH::pairs_t& pairs ) {
		const auto& nodeA = bvhA.nodes[nodeAID];
		const auto& nodeB = bvhB.nodes[nodeBID];

		if ( nodeA.asleep || nodeB.asleep || !::aabbOverlap( nodeA.bounds, nodeB.bounds ) ) return;

		if ( nodeA.count > 0 && nodeB.count > 0 ) {
			for ( auto i = 0; i < nodeA.count; ++i ) {
				for ( auto j = 0; j < nodeB.count; ++j ) {
					int32_t bodyA = bvhA.indices[nodeA.start + i];
					int32_t bodyB = bvhB.indices[nodeB.start + j];
					if ( bodyA == bodyB ) continue;
					if ( bodyA > bodyB ) std::swap( bodyA, bodyB );

					pairs.emplace(bodyA, bodyB);
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

	void traverseBVH( const pod::BVH& bvh, int32_t nodeID, pod::BVH::pairs_t& pairs ) {
		const auto& node = bvh.nodes[nodeID];

		if ( node.count > 0 ) {
			for ( auto i = 0; i < node.count; ++i ) {
				for ( auto j = i + 1; j < node.count; ++j ) {
					int32_t bodyA = bvh.indices[node.start + i];
					int32_t bodyB = bvh.indices[node.start + j];

					if ( bodyA == bodyB ) continue;
					if ( bodyA > bodyB ) std::swap( bodyA, bodyB );

					pairs.emplace(bodyA, bodyB);
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
		if ( bvh.nodes.empty() ) return;
		
		if ( !bvh.flattened.empty() ) return ::queryFlatBVH( bvh, bounds, outIndices );

		outIndices.reserve(::reserveCount);

		uf::stl::stack<int32_t> stack;
		stack.push(0);

		while ( !stack.empty() ) {
			int32_t idx = stack.top(); stack.pop();
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
	void queryBVH( const pod::BVH& bvh, const pod::AABB& bounds, uf::stl::vector<int32_t>& outIndices, int32_t nodeID ) {
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
			int32_t idx = stack.top(); stack.pop();
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
	void queryBVH( const pod::BVH& bvh, const pod::Ray& ray, uf::stl::vector<int32_t>& outIndices, int32_t nodeID, float maxDist ) {
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

		outPairs.reserve(::reserveCount);

		for ( auto i = 0; i < nodes.size(); ++i ) {
			const auto& nodeA = nodes[i];
			if ( nodeA.count <= 0 || nodeA.asleep ) continue;

			for ( auto j = i + 1; j < nodes.size(); ++j ) {
				const auto& nodeB = nodes[j];
				if ( nodeB.count <= 0 || nodeB.asleep ) continue;

				if ( !::aabbOverlap( nodeA.bounds, nodeB.bounds ) ) continue;

				for ( auto ia = 0; ia < nodeA.count; ++ia ) {
					for ( auto ib = 0; ib < nodeB.count; ++ib ) {
						auto indexA = indices[nodeA.start + ia];
						auto indexB = indices[nodeB.start + ib];

						if ( indexA == indexB ) continue;
						if ( indexA > indexB ) std::swap( indexA, indexB );

						outPairs.emplace( indexA, indexB );
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
		
		outPairs.reserve(::reserveCount);

		for ( auto i = 0; i < nodesA.size(); ++i ) {
			const auto& nodeA = nodesA[i];
			if ( nodeA.count <= 0 || nodeA.asleep ) continue;

			for ( auto j = 0; j < nodesB.size(); ++j ) {
				const auto& nodeB = nodesB[j];
				if ( nodeB.count <= 0 || nodeB.asleep ) continue;

				if ( !::aabbOverlap( nodeA.bounds, nodeB.bounds ) ) continue;

				for ( auto ia = 0; ia < nodeA.count; ++ia ) {
					for (auto ib = 0; ib < nodeB.count; ++ib ) {
						auto indexA = indicesA[nodeA.start + ia];
						auto indexB = indicesB[nodeB.start + ib];

						outPairs.emplace( indexA, indexB );
					}
				}
			}
		}
	}

	void queryFlatBVH( const pod::BVH& bvh, const pod::AABB& bounds, uf::stl::vector<int32_t>& outIndices ) {
		auto& nodes = bvh.flattened;
		auto& indices = bvh.indices;

		outIndices.reserve(::reserveCount);

		int32_t idx = 0;
		while ( idx < nodes.size() ) {
			const auto& node = nodes[idx];

			if ( !node.asleep && ::aabbOverlap( bounds, node.bounds ) ) {
				// leaf
				if ( node.count > 0 ) {
					for ( auto i = 0; i < node.count; ++i ) {
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

		int32_t idx = 0;
		while ( idx < nodes.size() ) {
			const auto& node = nodes[idx];
			float tMin, tMax;
			if ( !node.asleep && ::rayAabbIntersect( ray, node.bounds, tMin, tMax ) && tMin <= maxDist ) {
				// leaf
				if ( node.count > 0 ) {
					for ( auto i = 0; i < node.count; ++i ) {
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
	struct UnionFind {
		uf::stl::vector<int32_t> parent;
		uf::stl::vector<int32_t> rank;

		UnionFind( int32_t n ) {
			parent.resize(n);
			rank.resize(n, 0);
			
			for ( auto i = 0; i < n; i++ )
				parent[i] = i;
		}

		int32_t find( int32_t x ) {
			if ( parent[x] != x ) parent[x] = find(parent[x]);
			return parent[x];
		}

		void unite( int32_t a, int32_t b ) {
			int32_t rootA = find(a);
			int32_t rootB = find(b);

			if ( rootA == rootB ) return;

			// union by rank
			if ( rank[rootA] < rank[rootB] ) parent[rootA] = rootB;
			else if ( rank[rootA] > rank[rootB] ) parent[rootB] = rootA;
			else {
				parent[rootB] = rootA;
				rank[rootA]++;
			}
		}
	};
	void buildIslands( const pod::BVH::pairs_t& pairs, const uf::stl::vector<pod::PhysicsBody*>& bodies, uf::stl::vector<pod::Island>& islands ) {
		UnionFind unionizer(bodies.size());

		// union all pairs
		for ( auto& [a, b] : pairs ) {
			unionizer.unite(a, b);
		}

		// map root to island index
		uf::stl::unordered_map<int32_t, int32_t> rootToIsland;

		islands.clear();
		islands.reserve(bodies.size());

		for ( auto i = 0; i < bodies.size(); i++ ) {
			int32_t root = unionizer.find(i);

			if (rootToIsland.find(root) == rootToIsland.end()) {
				rootToIsland[root] = (int32_t) islands.size();
				islands.emplace_back();
			}

			int32_t islandID = rootToIsland[root];
			islands[islandID].indices.emplace_back( i );
		}

		// collect pairs per island
		for ( auto& [a, b] : pairs ) {
			// do not insert these pairs if they're non-colliding
			if ( !::shouldCollide( *bodies[a], *bodies[b] ) ) continue;

			int32_t root = unionizer.find(a);
			int32_t islandID = rootToIsland[root];
			islands[islandID].pairs.emplace(a, b);
		}
	}

	bool updateIsland( pod::Island& island, uf::stl::vector<pod::PhysicsBody*>& bodies, float dt ) {
		island.awake = false;

		for ( auto idx : island.indices ) {
			auto& body = *bodies[idx];
			if ( !body.activity.awake ) continue;

			float linSpeed = uf::vector::norm( body.velocity );
			float angSpeed = uf::vector::norm( body.angularVelocity );

			if ( linSpeed < pod::Activity::linearSleepEpsilon && angSpeed < pod::Activity::angularSleepEpsilon) {
				body.activity.sleepTimer += dt;
			} else {
				body.activity.sleepTimer = 0.0f;
				island.awake = true;
			}

			if ( body.activity.sleepTimer < pod::Activity::sleepThreshold ) {
				island.awake = true;
			}
		}

		// update bodies within island
		for ( auto idx : island.indices )
			(island.awake ? ::wakeBody : ::sleepBody)( *bodies[idx] );

		return island.awake;
	}
}