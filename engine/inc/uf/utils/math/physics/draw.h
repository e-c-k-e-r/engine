#pragma once

#include "structs.h"
#include <uf/utils/debug/draw.h>

namespace impl {
	void drawManifold( const pod::Manifold& manifold );
	void drawBody( const pod::PhysicsBody& body );
	void drawConstraint( const pod::Constraint& constraint );
	void draw( const pod::World& world, float dt = 0 );
}