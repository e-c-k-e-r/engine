#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/narrowphase.h>
#include <uf/utils/math/physics/constraints.h>
#include <uf/utils/math/physics/draw.h>
#include <uf/engine/scene/scene.h>
#include <uf/engine/graph/graph.h>


void impl::drawManifold( const pod::Manifold& manifold ) {
	for ( auto& contact : manifold.points ) {
		auto& start = contact.point;
		auto end = contact.point + (contact.normal * MIN(contact.penetration, 0.1f) * 2);

		uf::debug::addLine( start, end, pod::Vector4f{ 1, 0, 0, 1 } );
	}
}
void impl::drawBody( const pod::PhysicsBody& body ) {
	if ( !(body.collider.category & uf::physics::settings.debugDraw) ) return;
	switch( body.collider.type ) {
		case pod::ShapeType::AABB:
			impl::drawAabb( body );
		break;
		case pod::ShapeType::OBB:
			impl::drawObb( body );
		break;
		case pod::ShapeType::SPHERE:
			impl::drawSphere( body );
		break;
		case pod::ShapeType::CAPSULE:
			impl::drawCapsule( body );
		break;
		case pod::ShapeType::PLANE:
			impl::drawPlane( body );
		break;
		case pod::ShapeType::TRIANGLE:
			impl::drawTriangle( body );
		break;
		case pod::ShapeType::MESH:
			impl::drawMesh( body );
		break;
		case pod::ShapeType::CONVEX_HULL:
			impl::drawHull( body );
		break;
	}
}
void impl::drawConstraint( const pod::Constraint& constraint ) {
	if ( !(constraint.a->collider.category & uf::physics::settings.debugDraw) ) return;
	if ( !(constraint.b->collider.category & uf::physics::settings.debugDraw) ) return;

	switch( constraint.type ) {
		case pod::ConstraintType::BALL_AND_SOCKET:
			impl::drawBallSocket( constraint );
		break;
		case pod::ConstraintType::HINGE:
			impl::drawHinge( constraint );
		break;
		case pod::ConstraintType::CONE_TWIST:
			impl::drawConeTwist( constraint );
		break;
		case pod::ConstraintType::SLIDER:
			impl::drawSlider( constraint );
		break;
		case pod::ConstraintType::DISTANCE:
			impl::drawDistance( constraint );
		break;
		case pod::ConstraintType::WELD:
			impl::drawWeld( constraint );
		break;
		case pod::ConstraintType::SPRING:
			impl::drawSpring( constraint );
		break;
	}
}

void impl::draw( const pod::World& world, float dt ) {
	for ( auto* body : world.bodies ) impl::drawBody( *body );
	for ( auto* constraint : world.constraints ) impl::drawConstraint( *constraint );
}