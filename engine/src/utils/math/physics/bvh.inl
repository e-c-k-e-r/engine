namespace {
	pod::BVH::index_t flattenBVH( pod::BVH& bvh, pod::BVH::index_t nodeID );

	void queryFlatBVH( const pod::BVH&, const pod::AABB& bounds, uf::stl::vector<pod::BVH::index_t>& out );
	void queryFlatBVH( const pod::BVH&, const pod::Ray& ray, uf::stl::vector<pod::BVH::index_t>& out, float maxDist = FLT_MAX );
	
	void queryFlatOverlaps( const pod::BVH& bvh, pod::BVH::pairs_t& outPairs );
	void queryFlatOverlaps( const pod::BVH& bvhA, const pod::BVH& bvhB, pod::BVH::pairs_t& outPairs );
}

// BVH
namespace {
	pod::BVH::index_t buildBVHNode( pod::BVH& bvh, const uf::stl::vector<pod::AABB>& bounds, pod::BVH::index_t start, pod::BVH::index_t end, pod::BVH::index_t capacity = 2 ) {
		pod::BVH::Node node{};
		node.left  = 0;
		node.right = 0;
		node.start = start;
		node.setCount(0);

		pod::AABB bound = bounds[bvh.indices[start]];
		for ( auto i = start + 1; i < end; ++i) bound = ::mergeAabb( bound, bounds[bvh.indices[i]] );

		pod::BVH::index_t count = end - start;
		if ( count <= capacity ) {
			// leaf
			node.start = start;
			node.setCount(count);
			pod::BVH::index_t index = (pod::BVH::index_t) bvh.nodes.size();
			bvh.nodes.emplace_back(node);
			bvh.bounds.emplace_back(bound);
			return index;
		}

		// choose split axis by largest extent
		auto extent = bound.max - bound.min;
		auto axis = (extent.x > extent.y && extent.x > extent.z) ? 0 : (extent.y > extent.z ? 1 : 2);

		// sort indices by centroid along axis
		auto mid = ( start + end ) / 2;
		std::nth_element(bvh.indices.begin() + start, bvh.indices.begin() + mid, bvh.indices.begin() + end, [&](uint32_t a, uint32_t b) {
			return ::aabbCenter(bounds[a])[axis] < ::aabbCenter(bounds[b])[axis];
		});

		pod::BVH::index_t index = (pod::BVH::index_t) bvh.nodes.size();
		bvh.nodes.emplace_back( node ); // insert now, gets filled later
		bvh.bounds.emplace_back( bound );

		node.left = ::buildBVHNode( bvh, bounds, start, mid, capacity );
		node.right = ::buildBVHNode( bvh, bounds, mid, end, capacity );
		bvh.nodes[index] = node;
		return index;
	}

	pod::BVH::index_t buildBVHNode_SAH( pod::BVH& bvh, const uf::stl::vector<pod::AABB>& bounds, pod::BVH::index_t start, pod::BVH::index_t end, pod::BVH::index_t capacity = 4 ) {
		struct Bin {
			pod::AABB bounds;
			pod::BVH::index_t count = 0;
		};

		pod::BVH::Node node{};
		node.left  = 0;
		node.right = 0;
		node.start = start;
		node.setCount(0);

		pod::AABB bound = bounds[bvh.indices[start]];
		for ( auto i = start + 1; i < end; ++i) bound = ::mergeAabb( bound, bounds[bvh.indices[i]] );

		pod::BVH::index_t count = end - start;
		if ( count <= capacity ) {
			node.setCount(count);
			pod::BVH::index_t index = (pod::BVH::index_t) bvh.nodes.size();
			bvh.nodes.emplace_back(node);
			bvh.bounds.emplace_back(bound);
			return index;
		}

		constexpr auto numBins = 16;
		static thread_local Bin bins[numBins];
		for ( auto i = 0; i < numBins; i++ ) bins[i].count = 0;

		auto extent = bound.max - bound.min;
		auto bestAxis = -1, bestSplit = -1;
		float bestCost = std::numeric_limits<float>::infinity();

		for ( auto axis = 0; axis < 3; ++axis ) {
			if ( extent[axis] < EPS(1e-6f) ) continue;

			float minC = bound.min[axis];
			float maxC = bound.max[axis];
			float scale = (float) numBins / (maxC - minC);

			for ( auto i = start; i < end; ++i ) {
				pod::BVH::index_t idx = bvh.indices[i];
				float c = ::aabbCenter( bounds[idx] )[axis];
				pod::BVH::index_t binID = std::min((pod::BVH::index_t)(numBins - 1), (pod::BVH::index_t)((c - minC) * scale));
				bins[binID].bounds = bins[binID].count == 0 ? bounds[idx] : ::mergeAabb( bins[binID].bounds, bounds[idx] );
				++bins[binID].count;
			}

			pod::AABB leftBounds[numBins], rightBounds[numBins];
			pod::BVH::index_t leftCount[numBins] = {}, rightCount[numBins] = {};

			pod::AABB acc;
			pod::BVH::index_t cnt = 0;
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

			// precompute area
			float leftArea[numBins], rightArea[numBins];
			for ( auto i = 0; i < numBins; i++ ) leftArea[i] = ::aabbSurfaceArea( leftBounds[i] );
			for ( auto i = 0; i < numBins; i++ ) rightArea[i] = ::aabbSurfaceArea( rightBounds[i] );
			
			float parentArea = ::aabbSurfaceArea(bound);

			for ( auto i = 0; i < numBins - 1; i++ ) {
				if ( leftCount[i] == 0 || rightCount[i + 1] == 0 ) continue;
				float cost = 1.0f + (
					( leftArea[i] / parentArea ) * leftCount[i] +
					( rightArea[i + 1] / parentArea ) * rightCount[i + 1]
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
			node.setCount(count); // node.count = count;
			pod::BVH::index_t index = (pod::BVH::index_t) bvh.nodes.size();
			bvh.nodes.emplace_back(node);
			bvh.bounds.emplace_back(bound);
			return index;
		}

		float minC = bound.min[bestAxis];
		float maxC = bound.max[bestAxis];
		float scale = (float) numBins / (maxC - minC);

		auto midIt = std::partition( bvh.indices.begin() + start, bvh.indices.begin() + end, [&](pod::BVH::index_t idx) {
			float c = ::aabbCenter( bounds[idx] )[bestAxis ];
			pod::BVH::index_t binID = std::min((pod::BVH::index_t)(numBins - 1), (pod::BVH::index_t)((c - minC) * scale));
			return binID <= bestSplit;
		});

		pod::BVH::index_t mid = (pod::BVH::index_t) ( midIt - bvh.indices.begin() );

		// if partition failed (all left or all right), force leaf
		if ( mid == start || mid == end ) {
			node.setCount(count); // node.count = count;
			pod::BVH::index_t index = (pod::BVH::index_t) bvh.nodes.size();
			bvh.nodes.emplace_back(node);
			bvh.bounds.emplace_back(bound);
			return index;
		}

		pod::BVH::index_t index = (pod::BVH::index_t) bvh.nodes.size();
		bvh.nodes.emplace_back(node);
		bvh.bounds.emplace_back(bound);

		node.left  = ::buildBVHNode_SAH( bvh, bounds, start, mid, capacity );
		node.right = ::buildBVHNode_SAH( bvh, bounds, mid, end, capacity );
		bvh.nodes[index] = node;
		return index;
	}

	void buildBroadphaseBVH( pod::BVH& bvh, const uf::stl::vector<pod::PhysicsBody*>& bodies, pod::BVH::index_t capacity = 2, bool filters = false, bool filterType = false ) {
		if ( bodies.empty() ) return;

		bvh.indices.clear();
		bvh.nodes.clear();
		bvh.bounds.clear();
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

	void buildMeshBVH( pod::BVH& bvh, const uf::Mesh& mesh, pod::BVH::index_t capacity = 4 ) {
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
	pod::BVH::UpdatePolicy::Decision decideBVHUpdate( pod::BVH& bvh, uf::stl::vector<pod::PhysicsBody*>& bodies, const pod::BVH::UpdatePolicy& policy, size_t frameCounter ) {
		// BVH is not built
		if ( bvh.indices.empty() || bvh.nodes.empty() ) {
			return pod::BVH::UpdatePolicy::Decision::REBUILD;
		}
		if ( bodies.empty() ) return pod::BVH::UpdatePolicy::Decision::NONE;

		uint32_t dirtyCount = 0;
		float oldRootArea = ::aabbSurfaceArea( bvh.bounds[0] );

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
		// update nodes
		for ( auto i = 0; i < bvh.nodes.size(); ++i ) {
			auto& node = bvh.nodes[i];
			if ( /*node.count*/ node.getCount() == 0 ) continue;
			auto& bound = bvh.bounds[i];
			bound = bodies[bvh.indices[node.start]]->bounds;
			for ( auto i = 1; i < node.getCount() /*node.count*/; ++i ) bound = ::mergeAabb( bound, bodies[bvh.indices[node.start + i]]->bounds );
		}

		float dirtyRatio = (float) dirtyCount / (float) bodies.size();

		// compute new root bounds
		pod::AABB newRoot = bodies[bvh.indices[0]]->bounds;
		for ( auto i = 1; i < bvh.indices.size(); ++i ) newRoot = ::mergeAabb(newRoot, bodies[bvh.indices[i]]->bounds);

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
		uf::stl::vector<pod::BVH::index_t> leaves;
		leaves.reserve(::reserveCount);
		for ( auto i = 0; i < bvh.nodes.size(); i++ ) {
			if ( bvh.nodes[i].getCount() == 0 ) continue;
			leaves.emplace_back(i);
		}

		// recompute bounds from bodies
		for ( auto i = 0; i < leaves.size(); i++ ) {
			auto nodeID = leaves[i];
			auto& node = bvh.nodes[nodeID];
			auto& bound = bvh.bounds[nodeID];
			bound = bounds[bvh.indices[node.start]];
			for ( auto j = 1; j < node.getCount(); j++ )
				bound = ::mergeAabb(bound, bounds[bvh.indices[node.start + j]]);
		}

		// update internal nodes bottom-up
		for ( pod::BVH::index_t i = (pod::BVH::index_t) bvh.nodes.size() - 1; i >= 0; i-- ) {
			auto& node = bvh.nodes[i];
			auto& bound = bvh.bounds[i];
			// internal node
			if ( node.getCount() == 0 ) {
				bound = ::mergeAabb(bvh.bounds[node.left], bvh.bounds[node.right]);
			}
		}
	}
	
	// avoids creating a vector for bounds
	void refitBVH( pod::BVH& bvh, const uf::stl::vector<pod::PhysicsBody*>& bodies ) {
		if ( bvh.nodes.empty() ) return;

		// update leaf bounds
		//#pragma omp parallel for
		for ( auto i = 0; i < bvh.nodes.size(); i++ ) {
			auto& node = bvh.nodes[i];
			if ( node.getCount() == 0 ) continue;
			auto& bound = bvh.bounds[i];
			// leaf node: recompute bounds from bodies
			auto nodeID = bvh.indices[node.start];

			bound = bodies[nodeID]->bounds;
			node.setAsleep(!bodies[nodeID]->activity.awake);

			for ( auto j = 1; j < node.getCount(); j++ ) {
				auto bodyID = bvh.indices[node.start + j];
				bound = ::mergeAabb( bound, bodies[bodyID]->bounds );
				node.setAsleep(node.isAsleep() && !bodies[bodyID]->activity.awake);
			}
		}

		// update internal nodes bottom-up
		for ( int64_t i = (int64_t) bvh.nodes.size() - 1; i >= 0; i-- ) {
			auto& node = bvh.nodes[i];
			if ( node.getCount() > 0 ) continue;
			// internal node
			bvh.bounds[i] = ::mergeAabb( bvh.bounds[node.left], bvh.bounds[node.right] );
			node.setAsleep( bvh.nodes[node.left].isAsleep() && bvh.nodes[node.right].isAsleep());
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
	pod::BVH::index_t flattenBVH( pod::BVH& bvh, pod::BVH::index_t nodeID ) {
		if ( nodeID == 0 ) {
			bvh.flattened.clear();
			bvh.flatBounds.clear();
			bvh.flattened.reserve(bvh.nodes.size());
			bvh.flatBounds.reserve(bvh.bounds.size());
		}

		const auto& node = bvh.nodes[nodeID];

		pod::BVH::index_t flatID = (pod::BVH::index_t) bvh.flattened.size();
		bvh.flattened.emplace_back(); // placeholder
		bvh.flatBounds.emplace_back( bvh.bounds[nodeID] );

		pod::BVH::FlatNode flat{};
		flat.start = 0;
		flat.setCount(0);
		flat.skipIndex = 0;
		flat.setAsleep(node.isAsleep());

		// leaf
		if ( node.getCount() > 0 ) {
			flat.start = node.start;
			flat.setCount(node.getCount());
			flat.skipIndex = flatID + 1; // next node after this leaf
			bvh.flattened[flatID] = flat;
			return flatID + 1;
		}
		// internal
		else {
			flat.start = 0;
			flat.setCount(0);

			pod::BVH::index_t leftID  = ::flattenBVH( bvh, node.left );
			pod::BVH::index_t rightID = ::flattenBVH( bvh, node.right );

			flat.skipIndex = rightID; // skip entire subtree
			bvh.flattened[flatID] = flat;
			return rightID;
		}
	}
}

namespace {
	// collects a list of nodes that are overlapping with each other
	void traverseNodePair(const pod::BVH& bvh, pod::BVH::index_t nodeAID, pod::BVH::index_t nodeBID, pod::BVH::pairs_t& pairs) {
		const auto& nodeA = bvh.nodes[nodeAID];
		const auto& nodeB = bvh.nodes[nodeBID];

		if ( nodeA.isAsleep() || nodeB.isAsleep() || !::aabbOverlap( bvh.bounds[nodeAID], bvh.bounds[nodeBID] ) ) return;

		if ( nodeA.getCount() > 0 && nodeB.getCount() > 0 ) {
			for ( auto i = 0; i < nodeA.getCount(); ++i ) {
				for ( auto j = 0; j < nodeB.getCount(); ++j ) {
					pod::BVH::index_t bodyA = bvh.indices[nodeA.start + i];
					pod::BVH::index_t bodyB = bvh.indices[nodeB.start + j];
					if ( bodyA == bodyB ) continue;
					if ( bodyA > bodyB ) std::swap( bodyA, bodyB );

					pairs.emplace(bodyA, bodyB);
				}
			}
			return;
		}

		if ( nodeA.getCount() == 0 ) {
			::traverseNodePair( bvh, nodeA.left, nodeBID, pairs );
			::traverseNodePair( bvh, nodeA.right, nodeBID, pairs );
		}
		if ( nodeB.getCount() == 0 ) {
			::traverseNodePair( bvh, nodeAID, nodeB.left, pairs );
			::traverseNodePair( bvh, nodeAID, nodeB.right, pairs );
		}
	}
	// collects a list of nodes from each BVH that are overlapping with each other (for mesh v mesh)
	void traverseNodePair( const pod::BVH& bvhA, pod::BVH::index_t nodeAID, const pod::BVH& bvhB, pod::BVH::index_t nodeBID, pod::BVH::pairs_t& pairs ) {
		const auto& nodeA = bvhA.nodes[nodeAID];
		const auto& nodeB = bvhB.nodes[nodeBID];

		if ( nodeA.isAsleep() || nodeB.isAsleep() || !::aabbOverlap( bvhA.bounds[nodeAID], bvhB.bounds[nodeBID] ) ) return;

		if ( nodeA.getCount() > 0 && nodeB.getCount() > 0 ) {
			for ( auto i = 0; i < nodeA.getCount(); ++i ) {
				for ( auto j = 0; j < nodeB.getCount(); ++j ) {
					pod::BVH::index_t bodyA = bvhA.indices[nodeA.start + i];
					pod::BVH::index_t bodyB = bvhB.indices[nodeB.start + j];
					if ( bodyA == bodyB ) continue;
					if ( bodyA > bodyB ) std::swap( bodyA, bodyB );

					pairs.emplace(bodyA, bodyB);
				}
			}
			return;
		}

		if ( nodeA.getCount() == 0 ) {
			::traverseNodePair( bvhA, nodeA.left, bvhB, nodeBID, pairs );
			::traverseNodePair( bvhA, nodeA.right, bvhB, nodeBID, pairs );
		}
		if ( nodeB.getCount() == 0 ) {
			::traverseNodePair( bvhA, nodeAID, bvhB, nodeB.left, pairs );
			::traverseNodePair( bvhA, nodeAID, bvhB, nodeB.right, pairs );
		}
	}

	void traverseBVH( const pod::BVH& bvh, pod::BVH::index_t nodeID, pod::BVH::pairs_t& pairs ) {
		const auto& node = bvh.nodes[nodeID];

		if ( node.getCount() > 0 ) {
			for ( auto i = 0; i < node.getCount(); ++i ) {
				for ( auto j = i + 1; j < node.getCount(); ++j ) {
					pod::BVH::index_t bodyA = bvh.indices[node.start + i];
					pod::BVH::index_t bodyB = bvh.indices[node.start + j];

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
	void queryBVH( const pod::BVH& bvh, const pod::AABB& bounds, uf::stl::vector<pod::BVH::index_t>& outIndices ) {
		if ( bvh.nodes.empty() ) return;
		
		if ( !bvh.flattened.empty() ) return ::queryFlatBVH( bvh, bounds, outIndices );

		outIndices.reserve(::reserveCount);

		thread_local uf::stl::stack<pod::BVH::index_t> stack;
		//stack.clear(); // there is no stack.clear(), and the stack should already be cleared by the end of this function
		stack.push(0);

		while ( !stack.empty() ) {
			pod::BVH::index_t idx = stack.top(); stack.pop();
			auto& node = bvh.nodes[idx];
			if ( node.isAsleep() || !::aabbOverlap( bounds, bvh.bounds[idx] ) ) continue;

			if ( node.getCount() > 0 ) {
				for ( auto i = 0; i < node.getCount(); ++i) outIndices.emplace_back(bvh.indices[node.start + i]);
			} else {
				stack.push(node.left);
				stack.push(node.right);
			}
		}
	}
	void queryBVH( const pod::BVH& bvh, const pod::PhysicsBody& body, uf::stl::vector<pod::BVH::index_t>& outIndices ) {
		return ::queryBVH( bvh, body.bounds, outIndices );
	}
	
	// query a BVH with an AABB via recursion
	void queryBVH( const pod::BVH& bvh, const pod::AABB& bounds, uf::stl::vector<pod::BVH::index_t>& outIndices, pod::BVH::index_t nodeID ) {
		if ( !bvh.flattened.empty() ) return ::queryFlatBVH( bvh, bounds, outIndices );

		if ( nodeID == 0 ) outIndices.reserve(::reserveCount);

		const auto& node = bvh.nodes[nodeID];
		if ( node.isAsleep() || !::aabbOverlap( bounds, bvh.bounds[nodeID] ) ) return;

		if ( node.getCount() > 0 ) {
			for ( auto i = 0; i < node.getCount(); ++i ) outIndices.emplace_back(bvh.indices[node.start + i]);
			return;
		}

		// recurse
		::queryBVH( bvh, bounds, outIndices, node.left );
		::queryBVH( bvh, bounds, outIndices, node.right );
	}

	// query a BVH with a ray via a stack
	void queryBVH( const pod::BVH& bvh, const pod::Ray& ray, uf::stl::vector<pod::BVH::index_t>& outIndices, float maxDist ) {
		if ( !bvh.flattened.empty() ) return ::queryFlatBVH( bvh, ray, outIndices, maxDist );

		if ( bvh.nodes.empty() ) return;
		outIndices.reserve(::reserveCount);

		thread_local uf::stl::stack<pod::BVH::index_t> stack;
		//stack.clear(); // there is no stack.clear(), and the stack should already be cleared by the end of this function
		stack.push(0);

		while ( !stack.empty() ) {
			pod::BVH::index_t idx = stack.top(); stack.pop();
			const auto& node = bvh.nodes[idx];

			float tMin, tMax;
			if ( node.isAsleep() || !::rayAabbIntersect( ray, bvh.bounds[idx], tMin, tMax ) ) continue;
			if ( tMin > maxDist ) continue;

			if ( node.getCount() > 0 ) {
				for ( auto i = 0; i < node.getCount(); ++i) outIndices.emplace_back(bvh.indices[node.start + i]);
			} else {
				stack.push(node.left);
				stack.push(node.right);
			}
		}
	}
	// query a BVH with a ray via recursion
	void queryBVH( const pod::BVH& bvh, const pod::Ray& ray, uf::stl::vector<pod::BVH::index_t>& outIndices, pod::BVH::index_t nodeID, float maxDist ) {
		if ( !bvh.flattened.empty() ) return ::queryFlatBVH( bvh, ray, outIndices, maxDist );

		if ( nodeID == 0 ) outIndices.reserve(::reserveCount);

		const auto& node = bvh.nodes[nodeID];
		float tMin, tMax;
		if ( node.isAsleep() || !::rayAabbIntersect( ray, bvh.bounds[nodeID], tMin, tMax ) ) return;
		if ( tMin > maxDist ) return;

		if ( node.getCount() > 0 ) {
			for ( auto i = 0; i < node.getCount(); ++i ) outIndices.emplace_back(bvh.indices[node.start + i]);
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
			if ( nodeA.getCount() <= 0 || nodeA.isAsleep() ) continue;

			for ( auto j = i + 1; j < nodes.size(); ++j ) {
				const auto& nodeB = nodes[j];
				if ( nodeB.getCount() <= 0 || nodeB.isAsleep() ) continue;

				if ( !::aabbOverlap( bvh.flatBounds[i], bvh.flatBounds[j] ) ) continue;

				for ( auto ia = 0; ia < nodeA.getCount(); ++ia ) {
					for ( auto ib = 0; ib < nodeB.getCount(); ++ib ) {
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
			if ( nodeA.getCount() <= 0 || nodeA.isAsleep() ) continue;

			for ( auto j = 0; j < nodesB.size(); ++j ) {
				const auto& nodeB = nodesB[j];
				if ( nodeB.getCount() <= 0 || nodeB.isAsleep() ) continue;

				if ( !::aabbOverlap( bvhA.flatBounds[i], bvhB.flatBounds[j] ) ) continue;

				for ( auto ia = 0; ia < nodeA.getCount(); ++ia ) {
					for (auto ib = 0; ib < nodeB.getCount(); ++ib ) {
						auto indexA = indicesA[nodeA.start + ia];
						auto indexB = indicesB[nodeB.start + ib];

						outPairs.emplace( indexA, indexB );
					}
				}
			}
		}
	}

	void queryFlatBVH( const pod::BVH& bvh, const pod::AABB& bounds, uf::stl::vector<pod::BVH::index_t>& outIndices ) {
		auto& nodes = bvh.flattened;
		auto& indices = bvh.indices;

		outIndices.reserve(::reserveCount);

		pod::BVH::index_t idx = 0;
		while ( idx < nodes.size() ) {
			const auto& node = nodes[idx];

			if ( !node.isAsleep() && ::aabbOverlap( bounds, bvh.flatBounds[idx] ) ) {
				// leaf
				if ( node.getCount() > 0 ) {
					for ( auto i = 0; i < node.getCount(); ++i ) {
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
	void queryFlatBVH( const pod::BVH& bvh, const pod::Ray& ray, uf::stl::vector<pod::BVH::index_t>& outIndices, float maxDist ) {
		auto& nodes = bvh.flattened;
		auto& indices = bvh.indices;

		outIndices.reserve(::reserveCount);

		pod::BVH::index_t idx = 0;
		while ( idx < nodes.size() ) {
			const auto& node = nodes[idx];
			float tMin, tMax;
			if ( !node.isAsleep() && ::rayAabbIntersect( ray, bvh.flatBounds[idx], tMin, tMax ) && tMin <= maxDist ) {
				// leaf
				if ( node.getCount() > 0 ) {
					for ( auto i = 0; i < node.getCount(); ++i ) {
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
		uf::stl::vector<pod::BVH::index_t> parent;
		uf::stl::vector<pod::BVH::index_t> rank;

		UnionFind( pod::BVH::index_t n ) {
			parent.resize(n);
			rank.resize(n, 0);
			
			for ( auto i = 0; i < n; i++ )
				parent[i] = i;
		}

		pod::BVH::index_t find( pod::BVH::index_t x ) {
			if ( parent[x] != x ) parent[x] = find(parent[x]);
			return parent[x];
		}

		void unite( pod::BVH::index_t a, pod::BVH::index_t b ) {
			pod::BVH::index_t rootA = find(a);
			pod::BVH::index_t rootB = find(b);

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
		uf::stl::unordered_map<pod::BVH::index_t, pod::BVH::index_t> rootToIsland;

		islands.clear();
		islands.reserve(bodies.size());

		for ( auto i = 0; i < bodies.size(); i++ ) {
			pod::BVH::index_t root = unionizer.find(i);

			if (rootToIsland.find(root) == rootToIsland.end()) {
				rootToIsland[root] = (pod::BVH::index_t) islands.size();
				islands.emplace_back();
			}

			pod::BVH::index_t islandID = rootToIsland[root];
			islands[islandID].indices.emplace_back( i );
		}

		// collect pairs per island
		for ( auto& [a, b] : pairs ) {
			// do not insert these pairs if they're non-colliding
			if ( !::shouldCollide( *bodies[a], *bodies[b] ) ) continue;

			pod::BVH::index_t root = unionizer.find(a);
			pod::BVH::index_t islandID = rootToIsland[root];
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