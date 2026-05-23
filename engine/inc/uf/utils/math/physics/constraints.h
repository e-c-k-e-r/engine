#pragma once

#include "structs.h"

#include "constraints/contact.h"
#include "constraints/ballSocket.h"
#include "constraints/hinge.h"
#include "constraints/coneTwist.h"
#include "constraints/slider.h"
#include "constraints/distance.h"
#include "constraints/weld.h"
#include "constraints/spring.h"
#include "constraints/motor.h"

namespace impl {
	void solveConstraint( pod::Constraint& constraint, float dt );
	void solveConstraints( uf::stl::vector<pod::Constraint*>& constraint, float dt );
}
	
namespace uf {
	namespace physics {
		pod::Constraint& UF_API constrain( pod::PhysicsBody&, pod::PhysicsBody& );		
		void UF_API unconstrain( pod::PhysicsBody& );

		void UF_API setConstraintLimits( pod::Constraint& constraint, float lower, float upper );
	}
}