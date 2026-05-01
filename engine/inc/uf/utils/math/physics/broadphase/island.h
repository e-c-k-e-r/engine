#pragma once

#include "../impl.h"

namespace impl {
	void buildIslands( const pod::BVH::pairs_t& pairs, const uf::stl::vector<pod::PhysicsBody*>& bodies, uf::stl::vector<pod::Island>& islands );
}