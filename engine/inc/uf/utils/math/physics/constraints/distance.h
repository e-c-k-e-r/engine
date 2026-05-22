#pragma once

#include "../structs.h"

namespace impl {
	void solveDistanceConstraint( pod::Constraint& constraint, float dt );
}
/*
namespace uf {
	namespace physics {
		pod::Constraint& UF_API constrain( uf::Object&, uf::Object&, const pod::Vector3f&, const pod::Vector3f& );
		pod::Constraint& UF_API constrain( pod::World&, uf::Object&, uf::Object&, const pod::Vector3f&, const pod::Vector3f& );
		pod::Constraint& UF_API constrain( pod::PhysicsBody&, pod::PhysicsBody&, const pod::Vector3f&, const pod::Vector3f& );
		pod::Constraint& UF_API constrain( pod::World&, pod::PhysicsBody&, pod::PhysicsBody&, const pod::Vector3f&, const pod::Vector3f& );
	}
}
*/