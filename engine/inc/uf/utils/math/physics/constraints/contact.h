#pragma once

#include "../structs.h"

namespace impl {
	void bindManifold( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt = 0 );
	bool generateManifoldGjk( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt );
	bool generateManifold( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt );
	void computeLocalManifold( pod::Manifold& manifold );
	bool similarContact( const pod::Contact& a, const pod::Contact& b, float distSqThreshold = 1.0e-2f, float normThreshold = 0.9f );
	void reduceManifold( pod::Manifold& manifold );
	void mergeManifold( pod::Manifold& manifold );
	void retrieveManifold( pod::Manifold& current, const pod::Manifold& previous, float distanceThreshold = 0.1f, float separationThreshold = 0.1f, float decay = 0.85f );
	void prepareManifoldCache( uf::stl::unordered_map<size_t, pod::Manifold>& cache, const uf::stl::vector<pod::Island>& islands, const uf::stl::vector<pod::PhysicsBody*>& bodies );
	void updateManifoldCache( pod::Island& island, const pod::CollisionEvent::array_t& previous, uf::stl::unordered_map<size_t, pod::Manifold>& cache );
	void pruneManifoldCache( uf::stl::unordered_map<size_t, pod::Manifold>& cache );
	void warmupContact( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Contact& c, float dt );
	void warmupManifold( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Manifold& manifold, float dt );
	
	void resolveManifold( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt );
	void solveManifold( uf::stl::vector<pod::Manifold>& manifolds, float dt );

	void dispatchManifold( pod::Manifold& manifold, pod::CollisionEvent::events_t& events, pod::CollisionEvent::array_t& active, const pod::CollisionEvent::array_t& previous );
}