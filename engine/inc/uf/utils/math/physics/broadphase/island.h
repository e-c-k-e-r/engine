#pragma once

#include "../structs.h"

namespace impl {
	void buildIslands( const pod::BVH::pairs_t& pairs, const uf::stl::vector<pod::PhysicsBody*>& bodies, const uf::stl::vector<pod::Constraint*>& constraints, uf::stl::vector<pod::Island>& islands );
}