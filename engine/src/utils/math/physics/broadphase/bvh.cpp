#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/broadphase/bvh.h>
#include <uf/utils/math/physics/narrowphase/ray.h>
#include <uf/utils/memory/reader.h>
#include <uf/utils/memory/writer.h>

pod::BVH::index_t impl::buildBVHNode( pod::BVH& bvh, const uf::stl::vector<pod::AABB>& bounds, pod::BVH::index_t start, pod::BVH::index_t end, pod::BVH::index_t capacity ) {
	pod::BVH::Node node{};
	node.left  = 0;
	node.right = 0;
	node.start = start;
	node.setCount(0);

	pod::AABB bound = bounds[bvh.indices[start]];
	for ( auto i = start + 1; i < end; ++i) bound = impl::mergeAabb( bound, bounds[bvh.indices[i]] );

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
		return impl::aabbCenter(bounds[a])[axis] < impl::aabbCenter(bounds[b])[axis];
	});

	pod::BVH::index_t index = (pod::BVH::index_t) bvh.nodes.size();
	bvh.nodes.emplace_back( node ); // insert now, gets filled later
	bvh.bounds.emplace_back( bound );

	node.left = impl::buildBVHNode( bvh, bounds, start, mid, capacity );
	node.right = impl::buildBVHNode( bvh, bounds, mid, end, capacity );
	bvh.nodes[index] = node;
	return index;
}

pod::BVH::index_t impl::buildBVHNode_SAH( pod::BVH& bvh, const uf::stl::vector<pod::AABB>& bounds, pod::BVH::index_t start, pod::BVH::index_t end, pod::BVH::index_t capacity ) {
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
	for ( auto i = start + 1; i < end; ++i) bound = impl::mergeAabb( bound, bounds[bvh.indices[i]] );

	pod::BVH::index_t count = end - start;
	if ( count <= capacity ) {
		node.setCount(count);
		pod::BVH::index_t index = (pod::BVH::index_t) bvh.nodes.size();
		bvh.nodes.emplace_back(node);
		bvh.bounds.emplace_back(bound);
		return index;
	}

	constexpr auto numBins = 16;
	static thread_local Bin bins[numBins]; // do I even need to make this a static buffer......

	auto extent = bound.max - bound.min;
	auto bestAxis = -1, bestSplit = -1;
	float bestCost = std::numeric_limits<float>::infinity();

	for ( auto axis = 0; axis < 3; ++axis ) {
		if ( extent[axis] < EPS ) continue;
		for ( auto i = 0; i < numBins; i++ ) {
			bins[i].count = 0;
			bins[i].bounds = {};
		}

		float minC = bound.min[axis];
		float maxC = bound.max[axis];
		float scale = (float) numBins / (maxC - minC);

		for ( auto i = start; i < end; ++i ) {
			pod::BVH::index_t idx = bvh.indices[i];
			float c = impl::aabbCenter( bounds[idx] )[axis];
			pod::BVH::index_t binID = std::min((pod::BVH::index_t)(numBins - 1), (pod::BVH::index_t)((c - minC) * scale));
			bins[binID].bounds = bins[binID].count == 0 ? bounds[idx] : impl::mergeAabb( bins[binID].bounds, bounds[idx] );
			++bins[binID].count;
		}

		pod::AABB leftBounds[numBins], rightBounds[numBins];
		pod::BVH::index_t leftCount[numBins] = {}, rightCount[numBins] = {};

		pod::AABB acc;
		pod::BVH::index_t cnt = 0;
		for ( auto i = 0; i < numBins; i++ ) {
			if ( bins[i].count > 0 ) acc = (cnt == 0) ? bins[i].bounds : impl::mergeAabb( acc, bins[i].bounds );
			cnt += bins[i].count;
			leftBounds[i] = acc;
			leftCount[i] = cnt;
		}


		acc = {};
		cnt = 0;
		for ( auto i = numBins - 1; i >= 0; i-- ) {
			if ( bins[i].count > 0 ) acc = (cnt == 0) ? bins[i].bounds : impl::mergeAabb( acc, bins[i].bounds );
			cnt += bins[i].count;
			rightBounds[i] = acc;
			rightCount[i] = cnt;
		}

		// precompute area
		float leftArea[numBins], rightArea[numBins];
		for ( auto i = 0; i < numBins; i++ ) leftArea[i] = impl::aabbSurfaceArea( leftBounds[i] );
		for ( auto i = 0; i < numBins; i++ ) rightArea[i] = impl::aabbSurfaceArea( rightBounds[i] );
		
		float parentArea = impl::aabbSurfaceArea(bound);

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
		float c = impl::aabbCenter( bounds[idx] )[bestAxis ];
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

	node.left  = impl::buildBVHNode_SAH( bvh, bounds, start, mid, capacity );
	node.right = impl::buildBVHNode_SAH( bvh, bounds, mid, end, capacity );
	bvh.nodes[index] = node;
	return index;
}

void impl::buildBroadphaseBVH( pod::BVH& bvh, const uf::stl::vector<pod::PhysicsBody*>& bodies, pod::BVH::index_t capacity, bool filters, bool filterType ) {
	if ( bodies.empty() ) return;

	bvh.indices.clear();
	bvh.nodes.clear();
	bvh.bounds.clear();
	bvh.indices.reserve(bodies.size());

	// stores bounds
	uf::stl::vector<pod::AABB> bounds(bodies.size(), { {FLT_MAX, FLT_MAX, FLT_MAX}, {-FLT_MAX, -FLT_MAX, -FLT_MAX} });

	// populate initial indices and bounds
	for ( auto i = 0; i < bodies.size(); ++i ) {
		if ( filters && ( bodies[i]->inverseMass == 0.0f ) != filterType ) continue;

		bounds[i] = bodies[i]->bounds;
		bvh.indices.emplace_back(i);
	}

	if ( bvh.indices.empty() ) return; // inserted nothing

	// recursively build BVH from indices
	if ( uf::physics::settings.useBvhSahBodies ) impl::buildBVHNode_SAH( bvh, bounds, 0, bvh.indices.size(), capacity );
	else impl::buildBVHNode( bvh, bounds, 0, bvh.indices.size(), capacity );
	// flatten if requested
	if ( uf::physics::settings.flattenBvhBodies ) impl::flattenBVH( bvh, 0 );

	// mark as clean
	bvh.dirty = false;
}

void impl::buildMeshBVH( pod::BVH& bvh, const uf::Mesh& mesh, pod::BVH::index_t capacity ) {
	uint32_t triangles = mesh.index.count / 3;

	bvh.indices.clear();
	bvh.nodes.clear();
	bvh.indices.reserve( triangles );

	// stores bounds
	uf::stl::vector<pod::AABB> bounds;
	bounds.reserve( triangles );

	const auto& views = mesh.buffer_views;
	UF_ASSERT( !views.empty() );


	// populate initial indices and bounds
	for ( auto& view : views ) {
		auto& indices   = view["index"];
		auto& positions = view["position"];

		auto tris = view.index.count / 3;
		
		for ( auto triIndexID = 0; triIndexID < tris; ++triIndexID ) {
			auto tri = uf::mesh::fetchTriangle( view, indices, positions, triIndexID );
			auto aabb = impl::computeTriangleAABB( tri );
			auto triID = triIndexID + (view.index.first / 3);

		//	if ( triID != bounds.size() ) UF_MSG_DEBUG("triID={}, bounds.size()={}", triID, bounds.size());

			bounds.emplace_back( aabb );
			bvh.indices.emplace_back( triID ); // triID => mesh.index.buffer[triID * 3];
		}
	}

	UF_ASSERT( !bounds.empty() );

	// recursively build BVH from indices
	if ( uf::physics::settings.useBvhSahMeshes ) impl::buildBVHNode_SAH( bvh, bounds, 0, bvh.indices.size(), capacity );
	else impl::buildBVHNode( bvh, bounds, 0, bvh.indices.size(), capacity );
	// flatten if requested
	if ( uf::physics::settings.flattenBvhMeshes ) impl::flattenBVH( bvh, 0 );

	// mark as clean
	bvh.dirty = false;
}

void impl::buildConvexHullBVH( pod::BVH& bvh, const uf::Mesh& mesh, pod::BVH::index_t capacity ) {
	const auto& views = mesh.buffer_views;
	UF_ASSERT( !views.empty() );

	uint32_t hullCount = views.size();

	bvh.indices.clear();
	bvh.nodes.clear();
	bvh.indices.reserve( hullCount );

	// stores bounds
	uf::stl::vector<pod::AABB> bounds;
	bounds.reserve( hullCount );

	// populate initial indices and bounds
	for ( size_t viewID = 0; viewID < hullCount; ++viewID ) {
		const auto& view = views[viewID];
		auto aabb = impl::computeConvexHullAABB( view );

		bounds.emplace_back( aabb );
		bvh.indices.emplace_back( viewID );
	}

	UF_ASSERT( !bounds.empty() );

	// recursively build BVH from indices
	if ( uf::physics::settings.useBvhSahMeshes ) impl::buildBVHNode_SAH( bvh, bounds, 0, bvh.indices.size(), capacity );
	else impl::buildBVHNode( bvh, bounds, 0, bvh.indices.size(), capacity );

	// flatten if requested
	if ( uf::physics::settings.flattenBvhMeshes ) impl::flattenBVH( bvh, 0 );

	// mark as clean
	bvh.dirty = false;
}

pod::BVH::UpdatePolicy::Decision impl::decideBVHUpdate( pod::BVH& bvh, uf::stl::vector<pod::PhysicsBody*>& bodies, const pod::BVH::UpdatePolicy& policy, size_t frameCounter ) {
	// BVH is not built
	if ( bvh.indices.empty() || bvh.nodes.empty() ) {
		return pod::BVH::UpdatePolicy::Decision::REBUILD;
	}
	if ( bodies.empty() ) return pod::BVH::UpdatePolicy::Decision::NONE;

	uint32_t dirtyCount = 0;
	float oldRootArea = impl::aabbSurfaceArea( bvh.bounds[0] );

	// update/check each body
	for ( auto i = 0; i < bvh.nodes.size(); ++i ) {
		auto& node = bvh.nodes[i];
		if ( /*node.count*/ node.getCount() == 0 ) continue;
		auto& body = *bodies[bvh.indices[node.start]];

		auto& oldBounds = bvh.bounds[i];
		auto& newBounds = body.bounds;

		// compute displacement relative to size
		pod::Vector3f oldCenter = ( oldBounds.min + oldBounds.max ) * 0.5f;
		pod::Vector3f newCenter = ( newBounds.min + newBounds.max ) * 0.5f;
		float displacement = uf::vector::distance( newCenter, oldCenter );

		pod::Vector3f extent = oldBounds.max - oldBounds.min;
		float size = std::max({extent.x, extent.y, extent.z, 1e-6f});

		if ( displacement > policy.displacementThreshold * size ) ++dirtyCount;
	}
/*
	for ( auto idx : bvh.indices ) {
		auto& body = *bodies[idx];

		// to-do: instead check against bounds in BVH
		pod::AABB oldBounds = body.bounds;
		body.bounds = impl::computeAABB( body );
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
		if ( node.getCount() == 0 ) continue;
		auto& bound = bvh.bounds[i];
		bound = bodies[bvh.indices[node.start]]->bounds;
		for ( auto i = 1; i < node.getCount(); ++i ) bound = impl::mergeAabb( bound, bodies[bvh.indices[node.start + i]]->bounds );
	}
*/

	float dirtyRatio = (float) dirtyCount / (float) bodies.size();

	// compute new root bounds
	pod::AABB newRoot = bodies[bvh.indices[0]]->bounds;
	for ( auto i = 1; i < bvh.indices.size(); ++i ) newRoot = impl::mergeAabb(newRoot, bodies[bvh.indices[i]]->bounds);

	float newRootArea = impl::aabbSurfaceArea( newRoot );
	// BVH is too out of date, rebuild it
	if ( bvh.dirty || dirtyRatio > policy.dirtyRatioThreshold || newRootArea > oldRootArea * policy.overlapThreshold || frameCounter % policy.maxFramesBeforeRebuild == 0 ) {
		return pod::BVH::UpdatePolicy::Decision::REBUILD;
	}
	// bodies moved, refit the BVH instead
	if ( dirtyCount > 0 ) return pod::BVH::UpdatePolicy::Decision::REFIT;
	return pod::BVH::UpdatePolicy::Decision::NONE;
}

void impl::refitBVH( pod::BVH& bvh, const uf::stl::vector<pod::AABB>& bounds ) {
	if ( bvh.nodes.empty() ) return;

	// update leaf bounds
	uf::stl::vector<pod::BVH::index_t> leaves;
	leaves.reserve(uf::physics::settings.reserveCount);
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
			bound = impl::mergeAabb(bound, bounds[bvh.indices[node.start + j]]);
	}

	// update internal nodes bottom-up
	for ( int64_t i = (int64_t) bvh.nodes.size() - 1; i >= 0; i-- ) {
		auto& node = bvh.nodes[i];
		auto& bound = bvh.bounds[i];
		// internal node
		if ( node.getCount() == 0 ) {
			bound = impl::mergeAabb(bvh.bounds[node.left], bvh.bounds[node.right]);
		}
	}

	if ( !bvh.flattened.empty() ) impl::flattenBVH( bvh, 0 );
}

// avoids creating a vector for bounds
void impl::refitBVH( pod::BVH& bvh, const uf::stl::vector<pod::PhysicsBody*>& bodies ) {
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
			bound = impl::mergeAabb( bound, bodies[bodyID]->bounds );
			node.setAsleep(node.isAsleep() && !bodies[bodyID]->activity.awake);
		}
	}

	// update internal nodes bottom-up
	for ( int64_t i = (int64_t) bvh.nodes.size() - 1; i >= 0; i-- ) {
		auto& node = bvh.nodes[i];
		if ( node.getCount() > 0 ) continue;
		// internal node
		bvh.bounds[i] = impl::mergeAabb( bvh.bounds[node.left], bvh.bounds[node.right] );
		node.setAsleep( bvh.nodes[node.left].isAsleep() && bvh.nodes[node.right].isAsleep());
	}

	if ( !bvh.flattened.empty() ) impl::flattenBVH( bvh, 0 );
}

void impl::refitBVH( pod::BVH& bvh, const uf::Mesh& mesh ) {
	uint32_t triangles = mesh.index.count / 3;

	uf::stl::vector<pod::AABB> bounds;
	bounds.reserve( triangles );

	const auto& views = mesh.buffer_views;
	UF_ASSERT( !views.empty() );

	// populate initial indices and bounds
	for ( auto& view : views ) {
		auto& indices   = view["index"];
		auto& positions = view["position"];

		auto tris = view.index.count / 3;
		for ( auto triIndexID = 0; triIndexID < tris; ++triIndexID ) {
			auto tri = uf::mesh::fetchTriangle( view, indices, positions, triIndexID );
			auto aabb = impl::computeTriangleAABB( tri );
			bounds.emplace_back(aabb);
		}
	}
	
	// recursively build BVH from indices
	impl::refitBVH( bvh, bounds );
}

pod::BVH::index_t impl::flattenBVH( pod::BVH& bvh, pod::BVH::index_t nodeID ) {
	if ( nodeID == 0 ) {
		bvh.flattened.clear();
		bvh.flatBounds.clear();
		bvh.primitiveToNode.clear();

		bvh.flattened.reserve(bvh.nodes.size());
		bvh.flatBounds.reserve(bvh.bounds.size());
		bvh.primitiveToNode.resize(bvh.indices.size());
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
		flat.skipIndex = flatID + 1;
		bvh.flattened[flatID] = flat;

		for ( uint32_t i = 0; i < node.getCount(); ++i ) {
			bvh.primitiveToNode[bvh.indices[node.start + i]] = flatID;
		}
		return flatID + 1;
	}
	// internal
	else {
		flat.start = 0;
		flat.setCount(0);

		pod::BVH::index_t leftID  = impl::flattenBVH( bvh, node.left );
		pod::BVH::index_t rightID = impl::flattenBVH( bvh, node.right );

		flat.skipIndex = rightID; // skip entire subtree
		bvh.flattened[flatID] = flat;
		return rightID;
	}
}

// collects a list of nodes that are overlapping with each other
void impl::traverseNodePair(const pod::BVH& bvh, pod::BVH::index_t nodeAID, pod::BVH::index_t nodeBID, pod::BVH::pairs_t& pairs) {
	const auto& nodeA = bvh.nodes[nodeAID];
	const auto& nodeB = bvh.nodes[nodeBID];

	if ( nodeA.isAsleep() && nodeB.isAsleep() ) return;
	if ( !impl::aabbOverlap( bvh.bounds[nodeAID], bvh.bounds[nodeBID] ) ) return;

	if ( nodeA.getCount() > 0 && nodeB.getCount() > 0 ) {
		for ( auto i = 0; i < nodeA.getCount(); ++i ) {
			for ( auto j = 0; j < nodeB.getCount(); ++j ) {
				pod::BVH::index_t bodyA = bvh.indices[nodeA.start + i];
				pod::BVH::index_t bodyB = bvh.indices[nodeB.start + j];
				if ( bodyA == bodyB ) continue;
				if ( bodyA > bodyB ) std::swap( bodyA, bodyB );

				pairs.emplace_back(bodyA, bodyB);
			}
		}
		return;
	}

	if ( nodeA.getCount() == 0 ) {
		impl::traverseNodePair( bvh, nodeA.left, nodeBID, pairs );
		impl::traverseNodePair( bvh, nodeA.right, nodeBID, pairs );
	} else if ( nodeB.getCount() == 0 ) {
		impl::traverseNodePair( bvh, nodeAID, nodeB.left, pairs );
		impl::traverseNodePair( bvh, nodeAID, nodeB.right, pairs );
	}
}
// collects a list of nodes from each BVH that are overlapping with each other (for mesh v mesh)
void impl::traverseNodePair( const pod::BVH& bvhA, pod::BVH::index_t nodeAID, const pod::BVH& bvhB, pod::BVH::index_t nodeBID, pod::BVH::pairs_t& pairs ) {
	const auto& nodeA = bvhA.nodes[nodeAID];
	const auto& nodeB = bvhB.nodes[nodeBID];

	if ( nodeA.isAsleep() && nodeB.isAsleep() ) return;
	if ( !impl::aabbOverlap( bvhA.bounds[nodeAID], bvhB.bounds[nodeBID] ) ) return;

	if ( nodeA.getCount() > 0 && nodeB.getCount() > 0 ) {
		for ( auto i = 0; i < nodeA.getCount(); ++i ) {
			for ( auto j = 0; j < nodeB.getCount(); ++j ) {
				pod::BVH::index_t bodyA = bvhA.indices[nodeA.start + i];
				pod::BVH::index_t bodyB = bvhB.indices[nodeB.start + j];
				if ( bodyA == bodyB ) continue;
				//if ( bodyA > bodyB ) std::swap( bodyA, bodyB );

				pairs.emplace_back(bodyA, bodyB);
			}
		}
		return;
	}

	if ( nodeA.getCount() == 0 && nodeB.getCount() == 0 ) {
		if ( impl::aabbSurfaceArea(bvhA.bounds[nodeAID]) > impl::aabbSurfaceArea(bvhB.bounds[nodeBID]) ) {
			impl::traverseNodePair( bvhA, nodeA.left, bvhB, nodeBID, pairs );
			impl::traverseNodePair( bvhA, nodeA.right, bvhB, nodeBID, pairs );
		} else {
			impl::traverseNodePair( bvhA, nodeAID, bvhB, nodeB.left, pairs );
			impl::traverseNodePair( bvhA, nodeAID, bvhB, nodeB.right, pairs );
		}
	}
	else if ( nodeA.getCount() == 0 ) {
		impl::traverseNodePair( bvhA, nodeA.left, bvhB, nodeBID, pairs );
		impl::traverseNodePair( bvhA, nodeA.right, bvhB, nodeBID, pairs );
	}
	else if ( nodeB.getCount() == 0 ) {
		impl::traverseNodePair( bvhA, nodeAID, bvhB, nodeB.left, pairs );
		impl::traverseNodePair( bvhA, nodeAID, bvhB, nodeB.right, pairs );
	}
}

void impl::traverseNodePair( const pod::BVH& bvhA, pod::BVH::index_t nodeAID, const pod::BVH& bvhB, pod::BVH::index_t nodeBID, const pod::Transform<>& relTransform, pod::BVH::pairs_t& pairs ) {
	const auto& nodeA = bvhA.nodes[nodeAID];
	const auto& nodeB = bvhB.nodes[nodeBID];

	if ( nodeA.isAsleep() && nodeB.isAsleep() ) return;

	pod::AABB boundsB_in_A = impl::transformAabbToWorld(bvhB.bounds[nodeBID], relTransform);
	if ( !impl::aabbOverlap( bvhA.bounds[nodeAID], boundsB_in_A ) ) return;

	if ( nodeA.getCount() > 0 && nodeB.getCount() > 0 ) {
		for ( auto i = 0; i < nodeA.getCount(); ++i ) {
			for ( auto j = 0; j < nodeB.getCount(); ++j ) {
				pairs.emplace_back(bvhA.indices[nodeA.start + i], bvhB.indices[nodeB.start + j]);
			}
		}
		return;
	}

	if ( nodeA.getCount() == 0 && nodeB.getCount() == 0 ) {
		if ( impl::aabbSurfaceArea(bvhA.bounds[nodeAID]) > impl::aabbSurfaceArea(boundsB_in_A) ) {
			impl::traverseNodePair( bvhA, nodeA.left, bvhB, nodeBID, relTransform, pairs );
			impl::traverseNodePair( bvhA, nodeA.right, bvhB, nodeBID, relTransform, pairs );
		} else {
			impl::traverseNodePair( bvhA, nodeAID, bvhB, nodeB.left, relTransform, pairs );
			impl::traverseNodePair( bvhA, nodeAID, bvhB, nodeB.right, relTransform, pairs );
		}
	}
	else if ( nodeA.getCount() == 0 ) {
		impl::traverseNodePair( bvhA, nodeA.left, bvhB, nodeBID, relTransform, pairs );
		impl::traverseNodePair( bvhA, nodeA.right, bvhB, nodeBID, relTransform, pairs );
	}
	else if ( nodeB.getCount() == 0 ) {
		impl::traverseNodePair( bvhA, nodeAID, bvhB, nodeB.left, relTransform, pairs );
		impl::traverseNodePair( bvhA, nodeAID, bvhB, nodeB.right, relTransform, pairs );
	}
}

void impl::traverseBVH( const pod::BVH& bvh, pod::BVH::index_t nodeID, pod::BVH::pairs_t& pairs ) {
	const auto& node = bvh.nodes[nodeID];

	if ( node.getCount() > 0 ) {
		for ( auto i = 0; i < node.getCount(); ++i ) {
			for ( auto j = i + 1; j < node.getCount(); ++j ) {
				pod::BVH::index_t bodyA = bvh.indices[node.start + i];
				pod::BVH::index_t bodyB = bvh.indices[node.start + j];

				if ( bodyA == bodyB ) continue;
				if ( bodyA > bodyB ) std::swap( bodyA, bodyB );

				pairs.emplace_back(bodyA, bodyB);
			}
		}
		return;
	}

	impl::traverseNodePair( bvh, node.left, node.right, pairs );

	impl::traverseBVH( bvh, node.left, pairs );
	impl::traverseBVH( bvh, node.right, pairs );
}

void impl::queryOverlaps( const pod::BVH& bvh, pod::BVH::pairs_t& outPairs ) {
	if ( !bvh.flattened.empty() ) return impl::queryFlatOverlaps( bvh, outPairs );

	if ( bvh.nodes.empty() ) return;
	outPairs.reserve(uf::physics::settings.reserveCount);
	impl::traverseBVH( bvh, 0, outPairs );

	impl::postprocessPairs( outPairs );
}

void impl::queryOverlaps( const pod::BVH& bvhA, const pod::BVH& bvhB, pod::BVH::pairs_t& outPairs ) {
	if ( !bvhA.flattened.empty() && !bvhB.flattened.empty() ) return impl::queryFlatOverlaps( bvhA, bvhB, outPairs );

	if ( bvhA.nodes.empty() || bvhB.nodes.empty() ) return;
	outPairs.reserve(uf::physics::settings.reserveCount);
	impl::traverseNodePair(bvhA, 0, bvhB, 0, outPairs);

	impl::postprocessPairs( outPairs );
}

void impl::queryOverlaps( const pod::BVH& bvhA, const pod::BVH& bvhB, const pod::Transform<>& relTransform, pod::BVH::pairs_t& outPairs ) {
	if ( !bvhA.flattened.empty() && !bvhB.flattened.empty() ) return impl::queryFlatOverlaps( bvhA, bvhB, relTransform, outPairs );

	if ( bvhA.nodes.empty() || bvhB.nodes.empty() ) return;
	outPairs.reserve(uf::physics::settings.reserveCount);
	impl::traverseNodePair(bvhA, 0, bvhB, 0, relTransform, outPairs);

	impl::postprocessPairs( outPairs );
}

// query a BVH with an AABB via a stack
void impl::queryBVH( const pod::BVH& bvh, const pod::AABB& bounds, uf::stl::vector<pod::BVH::index_t>& outIndices ) {
	if ( bvh.nodes.empty() ) return;
	
	if ( !bvh.flattened.empty() ) return impl::queryFlatBVH( bvh, bounds, outIndices );

	outIndices.reserve(uf::physics::settings.reserveCount);

	static thread_local uf::stl::stack<pod::BVH::index_t> stack;
	//stack.clear(); // there is no stack.clear(), and the stack should already be cleared by the end of this function
	stack.push(0);

	while ( !stack.empty() ) {
		pod::BVH::index_t idx = stack.top(); stack.pop();
		auto& node = bvh.nodes[idx];
		if ( node.isAsleep() ) continue;
		if ( !impl::aabbOverlap( bounds, bvh.bounds[idx] ) ) continue;

		if ( node.getCount() > 0 ) {
			for ( auto i = 0; i < node.getCount(); ++i) outIndices.emplace_back(bvh.indices[node.start + i]);
		} else {
			stack.push(node.left);
			stack.push(node.right);
		}
	}
}
void impl::queryBVH( const pod::BVH& bvh, const pod::PhysicsBody& body, uf::stl::vector<pod::BVH::index_t>& outIndices ) {
	return impl::queryBVH( bvh, body.bounds, outIndices );
}

// query a BVH with an AABB via recursion
void impl::queryBVH( const pod::BVH& bvh, const pod::AABB& bounds, uf::stl::vector<pod::BVH::index_t>& outIndices, pod::BVH::index_t nodeID ) {
	if ( !bvh.flattened.empty() ) return impl::queryFlatBVH( bvh, bounds, outIndices );

	if ( nodeID == 0 ) outIndices.reserve(uf::physics::settings.reserveCount);

	const auto& node = bvh.nodes[nodeID];
	if ( node.isAsleep() ) return;
	if ( !impl::aabbOverlap( bounds, bvh.bounds[nodeID] ) ) return;

	if ( node.getCount() > 0 ) {
		for ( auto i = 0; i < node.getCount(); ++i ) outIndices.emplace_back(bvh.indices[node.start + i]);
		return;
	}

	// recurse
	impl::queryBVH( bvh, bounds, outIndices, node.left );
	impl::queryBVH( bvh, bounds, outIndices, node.right );
}

// query a BVH with a ray via a stack
void impl::queryBVH( const pod::BVH& bvh, const pod::Ray& ray, uf::stl::vector<pod::BVH::index_t>& outIndices, float maxDist ) {
	if ( !bvh.flattened.empty() ) return impl::queryFlatBVH( bvh, ray, outIndices, maxDist );

	if ( bvh.nodes.empty() ) return;
	outIndices.reserve(uf::physics::settings.reserveCount);

	static thread_local uf::stl::stack<pod::BVH::index_t> stack;
	//stack.clear(); // there is no stack.clear(), and the stack should already be cleared by the end of this function
	stack.push(0);

	while ( !stack.empty() ) {
		pod::BVH::index_t idx = stack.top(); stack.pop();
		const auto& node = bvh.nodes[idx];

		float tMin, tMax;
		//if ( node.isAsleep() ) continue;
		if ( !impl::rayAabbIntersect( ray, bvh.bounds[idx], tMin, tMax ) ) continue;
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
void impl::queryBVH( const pod::BVH& bvh, const pod::Ray& ray, uf::stl::vector<pod::BVH::index_t>& outIndices, pod::BVH::index_t nodeID, float maxDist ) {
	if ( !bvh.flattened.empty() ) return impl::queryFlatBVH( bvh, ray, outIndices, maxDist );

	if ( nodeID == 0 ) outIndices.reserve(uf::physics::settings.reserveCount);

	const auto& node = bvh.nodes[nodeID];
	float tMin, tMax;
	//if ( node.isAsleep() ) return;
	if ( !impl::rayAabbIntersect( ray, bvh.bounds[nodeID], tMin, tMax ) ) return;
	if ( tMin > maxDist ) return;

	if ( node.getCount() > 0 ) {
		for ( auto i = 0; i < node.getCount(); ++i ) outIndices.emplace_back(bvh.indices[node.start + i]);
		return;
	}

	// recurse
	impl::queryBVH( bvh, ray, outIndices, node.left, maxDist );
	impl::queryBVH( bvh, ray, outIndices, node.right, maxDist );
}

void impl::queryFlatOverlaps( const pod::BVH& bvh, pod::BVH::pairs_t& outPairs ) {
	auto& nodes = bvh.flattened;
	auto& bounds = bvh.flatBounds;
	auto& indices = bvh.indices;

	if ( nodes.empty() ) return;
	outPairs.reserve( uf::physics::settings.reserveCount );

	for ( pod::BVH::index_t a = 0; a < nodes.size(); ++a ) {
		const auto& nodeA = nodes[a];
		if ( nodeA.getCount() <= 0 || nodeA.isUnloaded() ) continue;

		const auto& boundsA = bounds[a];
		pod::BVH::index_t b = a + 1;
		while ( b < nodes.size() ) {
			const auto& nodeB = nodes[b];

			if ( ( nodeA.isAsleep() && nodeB.isAsleep() ) ||  nodeB.isUnloaded() || !impl::aabbOverlap( boundsA, bounds[b] ) ) {
				b = nodeB.skipIndex;
				continue;
			}

			if ( nodeB.getCount() > 0 ) {
				for ( pod::BVH::index_t ia = 0; ia < nodeA.getCount(); ++ia ) {
					for ( pod::BVH::index_t ib = 0; ib < nodeB.getCount(); ++ib ) {
						auto indexA = indices[nodeA.start + ia];
						auto indexB = indices[nodeB.start + ib];

						if ( indexA == indexB ) continue;
						if ( indexA > indexB ) std::swap(indexA, indexB);

						outPairs.emplace_back( indexA, indexB );
					}
				}
			}
			++b;
		}
	}

	impl::postprocessPairs( outPairs );
}
void impl::queryFlatOverlaps( const pod::BVH& bvhA, const pod::BVH& bvhB, pod::BVH::pairs_t& outPairs ) {
	auto& nodesA = bvhA.flattened;
	auto& boundsA = bvhA.flatBounds;
	auto& indicesA = bvhA.indices;

	auto& nodesB = bvhB.flattened;
	auto& boundsB = bvhB.flatBounds;
	auto& indicesB = bvhB.indices;

	if ( nodesA.empty() || nodesB.empty() ) return;
	outPairs.reserve(uf::physics::settings.reserveCount);

	STATIC_THREAD_LOCAL(pod::BVH::pairs_t, stack);
	stack.emplace_back(0, 0);

	while ( !stack.empty() ) {
		auto [a, b] = stack.back();
		stack.pop_back();

		const auto& nodeA = nodesA[a];
		const auto& nodeB = nodesB[b];

		if ( nodeA.isAsleep() && nodeB.isAsleep() ) continue;
		if ( nodeA.isUnloaded() || nodeB.isUnloaded() ) continue;

		if ( !impl::aabbOverlap( boundsA[a], boundsB[b] ) ) {
			continue;
		}

		bool isLeafA = (nodeA.getCount() > 0);
		bool isLeafB = (nodeB.getCount() > 0);

		if ( isLeafA && isLeafB ) {
			for ( pod::BVH::index_t ia = 0; ia < nodeA.getCount(); ++ia ) {
				for ( pod::BVH::index_t ib = 0; ib < nodeB.getCount(); ++ib ) {
					auto indexA = indicesA[nodeA.start + ia];
					auto indexB = indicesB[nodeB.start + ib];
					outPairs.emplace_back(indexA, indexB);
				}
			}
		}
		else if ( isLeafA ) {
			pod::BVH::index_t rightB = nodesB[b + 1].skipIndex;
			stack.emplace_back(a, b + 1);
			stack.emplace_back(a, rightB);
		}
		else if ( isLeafB ) {
			pod::BVH::index_t rightA = nodesA[a + 1].skipIndex;
			stack.emplace_back(a + 1,  b);
			stack.emplace_back(rightA, b);
		}
		else {
			pod::BVH::index_t rightA = nodesA[a + 1].skipIndex;
			pod::BVH::index_t rightB = nodesB[b + 1].skipIndex;

			stack.emplace_back(a + 1, b + 1);
			stack.emplace_back(a + 1, rightB);
			stack.emplace_back(rightA, b + 1);
			stack.emplace_back(rightA, rightB);
		}
	}

	impl::postprocessPairs( outPairs );
}

void impl::queryFlatOverlaps( const pod::BVH& bvhA, const pod::BVH& bvhB, const pod::Transform<>& relTransform, pod::BVH::pairs_t& outPairs ) {
	auto& nodesA = bvhA.flattened;
	auto& boundsA = bvhA.flatBounds;
	auto& indicesA = bvhA.indices;

	auto& nodesB = bvhB.flattened;
	auto& boundsB = bvhB.flatBounds;
	auto& indicesB = bvhB.indices;

	if ( nodesA.empty() || nodesB.empty() ) return;
	outPairs.reserve(uf::physics::settings.reserveCount);

	STATIC_THREAD_LOCAL(pod::BVH::pairs_t, stack);
	stack.emplace_back(0, 0);

	while ( !stack.empty() ) {
		auto [a, b] = stack.back();
		stack.pop_back();

		const auto& nodeA = bvhA.flattened[a];
		const auto& nodeB = bvhB.flattened[b];

		if ( nodeA.isAsleep() && nodeB.isAsleep() ) continue;
		if ( nodeA.isUnloaded() || nodeB.isUnloaded() ) continue;

		pod::AABB boundsB_in_A = impl::transformAabbToWorld(boundsB[b], relTransform);
		if ( !impl::aabbOverlap( boundsA[a], boundsB_in_A ) ) continue;

		bool isLeafA = (nodeA.getCount() > 0);
		bool isLeafB = (nodeB.getCount() > 0);

		if ( isLeafA && isLeafB ) {
			for ( pod::BVH::index_t ia = 0; ia < nodeA.getCount(); ++ia ) {
				for ( pod::BVH::index_t ib = 0; ib < nodeB.getCount(); ++ib ) {
					auto indexA = indicesA[nodeA.start + ia];
					auto indexB = indicesB[nodeB.start + ib];
					outPairs.emplace_back(indexA, indexB);
				}
			}
		}
		else if ( isLeafA ) {
			pod::BVH::index_t rightB = nodesB[b + 1].skipIndex;
			stack.emplace_back(a, b + 1);
			stack.emplace_back(a, rightB);
		}
		else if ( isLeafB ) {
			pod::BVH::index_t rightA = nodesA[a + 1].skipIndex;
			stack.emplace_back(a + 1,  b);
			stack.emplace_back(rightA, b);
		}
		else {
			pod::BVH::index_t rightA = nodesA[a + 1].skipIndex;
			pod::BVH::index_t rightB = nodesB[b + 1].skipIndex;

			stack.emplace_back(a + 1, b + 1);
			stack.emplace_back(a + 1, rightB);
			stack.emplace_back(rightA, b + 1);
			stack.emplace_back(rightA, rightB);
		}
	}

	impl::postprocessPairs( outPairs );
}

void impl::queryFlatBVH( const pod::BVH& bvh, const pod::AABB& bounds, uf::stl::vector<pod::BVH::index_t>& outIndices ) {
	auto& nodes = bvh.flattened;
	auto& indices = bvh.indices;

	outIndices.reserve(uf::physics::settings.reserveCount);

	pod::BVH::index_t idx = 0;
	while ( idx < nodes.size() ) {
		const auto& node = nodes[idx];

		if ( !node.isAsleep() && !node.isUnloaded() && impl::aabbOverlap( bounds, bvh.flatBounds[idx] ) ) {
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

void impl::queryFlatBVH( const pod::BVH& bvh, const pod::Ray& ray, uf::stl::vector<pod::BVH::index_t>& outIndices, float maxDist ) {
	auto& nodes = bvh.flattened;
	auto& indices = bvh.indices;

	outIndices.reserve(uf::physics::settings.reserveCount);

	pod::BVH::index_t idx = 0;
	while ( idx < nodes.size() ) {
		const auto& node = nodes[idx];
		float tMin, tMax;
		if ( !node.isAsleep() && !node.isUnloaded() && impl::rayAabbIntersect( ray, bvh.flatBounds[idx], tMin, tMax ) && tMin <= maxDist ) {
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

void impl::postprocessPairs( pod::BVH::pairs_t& pairs ) {
	std::sort(pairs.begin(), pairs.end());
	pairs.erase(std::unique(pairs.begin(), pairs.end()), pairs.end());
}

void uf::bvh::flagAsActive( pod::BVH& bvh, uint32_t index, bool active ) {
	if ( index < bvh.primitiveToNode.size() ) {
		uint32_t flatNodeID = bvh.primitiveToNode[index];
		if ( flatNodeID < bvh.flattened.size() ) {
			bvh.flattened[flatNodeID].setUnloaded(!active);
		}
		return;
	}

	if ( !bvh.nodes.empty() ) {
		for ( auto& node : bvh.nodes ) {
			if ( node.getCount() > 0 ) {
				for ( uint32_t i = 0; i < node.getCount(); ++i ) {
					if ( bvh.indices[node.start + i] == index ) {
						node.setUnloaded(!active);
						return;
					}
				}
			}
		}
	}
}

size_t uf::bvh::serialize( const pod::BVH& bvh, uf::stl::vector<uint8_t>& outBuffer, uint32_t offset ) {
	uf::stl::writer writer( outBuffer, offset, true );

	writer.write( (uint32_t)( bvh.indices.size() ) );
	writer.write( (uint32_t)( bvh.nodes.size() ) );
	writer.write( (uint32_t)( bvh.flattened.size() ) );

	if ( !bvh.indices.empty() ) writer.write( bvh.indices );
	if ( !bvh.nodes.empty() ) { writer.write( bvh.nodes ); writer.write( bvh.bounds); }
	if ( !bvh.flattened.empty() ) { writer.write( bvh.flattened ); writer.write( bvh.flatBounds ); writer.write( bvh.primitiveToNode ); }

	return writer.offset() - offset;
}

bool uf::bvh::deserialize( pod::BVH& bvh, const uf::stl::vector<uint8_t>& buffer, uint32_t offset, uint32_t length ) {
	uf::stl::reader reader( buffer, offset, length > 0 ? length : buffer.size(), true, true );

	const uint32_t* pNumIndices = reader.read<uint32_t>();
	const uint32_t* pNumNodes   = reader.read<uint32_t>();
	const uint32_t* pNumFlat	= reader.read<uint32_t>();

	if ( !pNumIndices || !pNumNodes || !pNumFlat ) return false;

	uint32_t numIndices = *pNumIndices;
	uint32_t numNodes   = *pNumNodes;
	uint32_t numFlat	= *pNumFlat;

	if ( numIndices > 0 ) {
		if ( !reader.read( numIndices, bvh.indices ) ) return false;
	} else {
		bvh.indices.clear();
	}

	if ( numNodes > 0 ) {
		if ( numFlat > 0 ) {
			reader.skip( numNodes * sizeof(pod::BVH::Node) );
			reader.skip( numNodes * sizeof(pod::AABB) );
			bvh.nodes.clear();
			bvh.nodes.shrink_to_fit();
			bvh.bounds.clear();
			bvh.bounds.shrink_to_fit();
		} else {
			if ( !reader.read( numNodes, bvh.nodes ) ) return false;
			if ( !reader.read( numNodes, bvh.bounds ) ) return false;
		}
	} else {
		bvh.nodes.clear();
		bvh.bounds.clear();
	}

	if ( numFlat > 0 ) {
		if ( !reader.read( numFlat, bvh.flattened ) ) return false;
		if ( !reader.read( numFlat, bvh.flatBounds ) ) return false;
		if ( !reader.read( numIndices, bvh.primitiveToNode ) ) return false;
	} else {
		bvh.flattened.clear();
		bvh.flatBounds.clear();
		bvh.primitiveToNode.clear();
	}

	bvh.dirty = false;
	return true;
}

void uf::bvh::build( pod::BVH& bvh, const uf::Mesh& mesh ) {
	return impl::buildMeshBVH( bvh, mesh, uf::physics::settings.meshBvhCapacity );
}