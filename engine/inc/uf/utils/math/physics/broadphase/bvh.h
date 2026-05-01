#pragma once

#include "../impl.h"

namespace impl {	
	pod::BVH::index_t buildBVHNode( pod::BVH& bvh, const uf::stl::vector<pod::AABB>& bounds, pod::BVH::index_t start, pod::BVH::index_t end, pod::BVH::index_t capacity = 2 );
	pod::BVH::index_t buildBVHNode_SAH( pod::BVH& bvh, const uf::stl::vector<pod::AABB>& bounds, pod::BVH::index_t start, pod::BVH::index_t end, pod::BVH::index_t capacity = 4 );

	void buildBroadphaseBVH( pod::BVH& bvh, const uf::stl::vector<pod::PhysicsBody*>& bodies, pod::BVH::index_t capacity = 2, bool filters = false, bool filterType = false );
	void buildMeshBVH( pod::BVH& bvh, const uf::Mesh& mesh, pod::BVH::index_t capacity = 4 );
	void buildConvexHullBVH( pod::BVH& bvh, const uf::Mesh& mesh, pod::BVH::index_t capacity = 1 );
	
	pod::BVH::UpdatePolicy::Decision decideBVHUpdate( pod::BVH& bvh, uf::stl::vector<pod::PhysicsBody*>& bodies, const pod::BVH::UpdatePolicy& policy, size_t frameCounter );
	void refitBVH( pod::BVH& bvh, const uf::stl::vector<pod::AABB>& bounds );
	void refitBVH( pod::BVH& bvh, const uf::stl::vector<pod::PhysicsBody*>& bodies );
	void refitBVH( pod::BVH& bvh, const uf::Mesh& mesh );
	
	pod::BVH::index_t flattenBVH( pod::BVH& bvh, pod::BVH::index_t nodeID );

	void traverseNodePair(const pod::BVH& bvh, pod::BVH::index_t nodeAID, pod::BVH::index_t nodeBID, pod::BVH::pairs_t& pairs);
	void traverseNodePair( const pod::BVH& bvhA, pod::BVH::index_t nodeAID, const pod::BVH& bvhB, pod::BVH::index_t nodeBID, pod::BVH::pairs_t& pairs );
	void traverseNodePair( const pod::BVH& bvhA, pod::BVH::index_t nodeAID, const pod::BVH& bvhB, pod::BVH::index_t nodeBID, const pod::Transform<>& relTransform, pod::BVH::pairs_t& pairs );
	void traverseBVH( const pod::BVH& bvh, pod::BVH::index_t nodeID, pod::BVH::pairs_t& pairs );
	
	void queryOverlaps( const pod::BVH& bvh, pod::BVH::pairs_t& outPairs );
	void queryOverlaps( const pod::BVH& bvhA, const pod::BVH& bvhB, pod::BVH::pairs_t& outPairs );
	void queryOverlaps( const pod::BVH& bvhA, const pod::BVH& bvhB, const pod::Transform<>& relTransform, pod::BVH::pairs_t& outPairs );
	
	void queryBVH( const pod::BVH& bvh, const pod::AABB& bounds, uf::stl::vector<pod::BVH::index_t>& outIndices );
	void queryBVH( const pod::BVH& bvh, const pod::PhysicsBody& body, uf::stl::vector<pod::BVH::index_t>& outIndices );
	void queryBVH( const pod::BVH& bvh, const pod::AABB& bounds, uf::stl::vector<pod::BVH::index_t>& outIndices, pod::BVH::index_t nodeID );
	void queryBVH( const pod::BVH& bvh, const pod::Ray& ray, uf::stl::vector<pod::BVH::index_t>& outIndices, float maxDist = FLT_MAX );
	void queryBVH( const pod::BVH& bvh, const pod::Ray& ray, uf::stl::vector<pod::BVH::index_t>& outIndices, pod::BVH::index_t nodeID, float maxDist = FLT_MAX );
	
	void queryFlatOverlaps( const pod::BVH& bvh, pod::BVH::pairs_t& outPairs );
	void queryFlatOverlaps( const pod::BVH& bvhA, const pod::BVH& bvhB, pod::BVH::pairs_t& outPairs );
	void queryFlatOverlaps( const pod::BVH& bvhA, const pod::BVH& bvhB, const pod::Transform<>& relTransform, pod::BVH::pairs_t& outPairs );
	void queryFlatBVH( const pod::BVH& bvh, const pod::AABB& bounds, uf::stl::vector<pod::BVH::index_t>& outIndices );
	void queryFlatBVH( const pod::BVH& bvh, const pod::Ray& ray, uf::stl::vector<pod::BVH::index_t>& outIndices, float maxDist = FLT_MAX );
	
	void postprocessPairs( pod::BVH::pairs_t& pairs );
}