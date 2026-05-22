#pragma once

#include "impl.h"

#include "constraints/contact.h"
#include "constraints/ballSocket.h"
#include "constraints/hinge.h"
#include "constraints/coneTwist.h"

namespace impl {
	void solveConstraint( pod::Constraint& constraint, float dt );
}