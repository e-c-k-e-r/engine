#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/narrowphase.h>
#include <uf/utils/math/physics/constraints.h>
#include <uf/utils/math/physics/draw.h>
#include <uf/engine/scene/scene.h>
#include <uf/engine/graph/graph.h>


void impl::drawManifold( const pod::Manifold& manifold ) {
	if ( !uf::physics::settings.debugDraw.contacts ) return;
	for ( auto& contact : manifold.points ) {
		auto& start = contact.point;
		auto end = contact.point + (contact.normal * MIN(contact.penetration, 0.1f) * 2);

		uf::debug::addLine( start, end, pod::Vector4f{ 1, 0, 0, 1 } );
	}
}
void impl::drawBody( const pod::PhysicsBody& body ) {
	if ( !(body.collider.category & uf::physics::settings.debugDraw.mask) ) return;
	// draw wireframe
	switch ( body.collider.type ) {
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
	// draw name
	auto& scene = uf::scene::getCurrentScene();
	auto& controller = scene.getController();
	auto& camera = scene.getCamera( controller );
	auto& bounds = body.bounds;
	
	auto transform = impl::getTransform( body );
	auto cameraTransform = uf::transform::flatten( camera.getTransform() );
	auto cameraAxes = uf::transform::axes( cameraTransform );
	auto& projection = camera.getProjection();
	auto fov = std::atan(1.0f / fabs(projection(1,1)));	
	auto angleThreshold = std::cos(fov * 1.5f);
	auto viewThresholdSq = std::pow(10, 2);

	cameraTransform.position.y -= 1;

	// continuously pick the closest point on the AABB
	auto position = impl::closestPointOnAABB( cameraTransform.position, bounds );
	auto dir = position - cameraTransform.position;
	auto magSq = uf::vector::magnitude( dir );
	if ( magSq > EPS2 ) dir /= std::sqrt( magSq );
	auto dot = uf::vector::dot( cameraAxes.forward, dir );
	position -= cameraAxes.forward * 0.1f;

	if ( magSq < viewThresholdSq && dot > angleThreshold ) {
		STATIC_THREAD_LOCAL(uf::stl::vector<uf::stl::string>, strings);
		strings.emplace_back( ::fmt::format("{}\n", uf::string::toString( *body.object ) ) );
		strings.emplace_back( ::fmt::format("Mass: {:.3f} kg\n", body.inverseMass == 0.0f ? 0.0f : 1.0f / body.inverseMass) );
		strings.emplace_back( ::fmt::format(
			"Position: {} | Pitch: {:.1f} | Yaw: {:.1f} | Roll: {:.1f}\n",
			uf::vector::toString( transform.position, "{}" ),
			(RAD_2_DEG) * uf::quaternion::pitch( transform.orientation ),
			(RAD_2_DEG) * uf::quaternion::yaw( transform.orientation ),
			(RAD_2_DEG) * uf::quaternion::roll( transform.orientation )
		) );
		if ( body.velocity != pod::Vector3f{} && body.angularVelocity != pod::Vector3f{} ) {
			strings.emplace_back( ::fmt::format( "Velocity: {} | Angular Velocity: {}\n", uf::vector::toString( body.velocity, "{}" ), uf::vector::toString( body.angularVelocity, "{}" ) ) ); 
		}
		strings.emplace_back( ::fmt::format("Awake: {} | Timer: {:.3f} | Grounded: {}\n", body.activity.awake, body.activity.sleepTimer, body.activity.grounded) );
		strings.emplace_back( ::fmt::format("Category: {:#X} | Mask: {:#X}", body.collider.category, body.collider.mask) );
		uf::debug::drawText( uf::string::join(strings, ""), position );
	}
}
void impl::drawConstraint( const pod::Constraint& constraint ) {
	if ( !uf::physics::settings.debugDraw.constraints ) return;
	if ( !(constraint.a->collider.category & uf::physics::settings.debugDraw.mask) ) return;
	if ( !(constraint.b->collider.category & uf::physics::settings.debugDraw.mask) ) return;

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