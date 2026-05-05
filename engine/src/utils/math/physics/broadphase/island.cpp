#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/broadphase/island.h>

namespace impl {
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
}

// to-do: rewrite this, I'm pretty sure it's faulty
void impl::buildIslands( const pod::BVH::pairs_t& pairs, const uf::stl::vector<pod::PhysicsBody*>& bodies, uf::stl::vector<pod::Island>& islands ) {
	UnionFind unionizer(bodies.size());

	// union all pairs
	for ( auto& [a, b] : pairs ) {
		if ( !bodies[a]->isStatic && !bodies[b]->isStatic ) {
			unionizer.unite(a, b);
		}
	}

	// map root to island index
	typedef uf::stl::unordered_map<pod::BVH::index_t, pod::BVH::index_t> map_t;
	STATIC_THREAD_LOCAL(map_t, rootToIsland);

	islands.clear();
	islands.reserve(bodies.size());

	for ( auto i = 0; i < bodies.size(); i++ ) {
		if ( bodies[i]->isStatic ) continue;

		pod::BVH::index_t root = unionizer.find(i);

		auto [ it, inserted ] = rootToIsland.try_emplace( root, (pod::BVH::index_t) islands.size());
		if ( inserted ) islands.emplace_back();

		pod::BVH::index_t islandID = rootToIsland[root];
		islands[islandID].indices.emplace_back( i );
	}

	// collect pairs per island
	for ( auto& [a, b] : pairs ) {
		// do not insert these pairs if they're non-colliding
		if ( !impl::shouldCollide( *bodies[a], *bodies[b] ) ) continue;

		// just in case
		pod::BVH::index_t dynamicIndex = bodies[a]->isStatic ? b : a;
		if ( bodies[a]->isStatic && bodies[b]->isStatic ) continue;

		pod::BVH::index_t root = unionizer.find(a);
		if ( rootToIsland.find(root) != rootToIsland.end() ) {
			pod::BVH::index_t islandID = rootToIsland[root];

			islands[islandID].pairs.emplace_back(a, b);

			if ( bodies[a]->activity.awake || bodies[b]->activity.awake ) {
				impl::wakeBody( *bodies[dynamicIndex] );
			}
		}
	}

	// update islands
	for ( auto it = islands.begin(); it != islands.end(); ) {
		auto& island = *it;
		island.awake = false;

		// wake island if something is awake in it
		for ( auto idx : island.indices ) {
			auto& body = *bodies[idx];			
			if ( !body.activity.awake ) continue;
			island.awake = true;
		}
		
		// update bodies within island
		for ( auto idx : island.indices )
			(island.awake ? impl::wakeBody : impl::sleepBody)( *bodies[idx] );

		// erase sleeping island
		if ( !island.awake ) {
			it = islands.erase(it);
		} else {
			++it;
		}
	}
}