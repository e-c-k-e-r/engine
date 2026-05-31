#include <uf/utils/math/physics/impl.h>
#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/integration.h>
#include <uf/utils/math/physics/constraints.h>

void impl::solveDistanceConstraint( pod::Constraint& constraint, float dt ) {
	auto& a = *constraint.a;
	auto& b = *constraint.b;
	auto& joint = constraint.distance;

	auto tA = impl::getTransform( a );
	auto tB = impl::getTransform( b );

	auto rA = uf::quaternion::rotate( tA.orientation, joint.localAnchorA * tA.scale );
	auto rB = uf::quaternion::rotate( tB.orientation, joint.localAnchorB * tB.scale );

	auto pA = tA.position + rA;
	auto pB = tB.position + rB;

	auto delta = pB - pA;
	float currentDistance2 = uf::vector::magnitude( delta );
	if ( currentDistance2 < EPS2 ) return;
	float currentDistance = std::sqrt( currentDistance2 );

	pod::Vector3f normal = delta / currentDistance;
	float distanceError = currentDistance - joint.targetDistance;

	auto vA = a.velocity + uf::vector::cross(a.angularVelocity, rA);
	auto vB = b.velocity + uf::vector::cross(b.angularVelocity, rB);
	float relVelAlongNormal = uf::vector::dot( vB - vA, normal );

	float invMassN = impl::computeEffectiveMass( a, b, rA, rB, normal );
	if ( invMassN < EPS ) return;

	float bias = (uf::physics::settings.baumgarteCorrectionPercent / dt) * distanceError;
	if ( joint.isRope && currentDistance <= joint.targetDistance ) bias = 0;
	float j = -(relVelAlongNormal + bias) / invMassN;

	impl::applyImpulseTo( a, b, rA, rB, normal, j, joint.accumulatedImpulse, -FLT_MAX, joint.isRope ? 0 : FLT_MAX );
}

pod::Constraint& uf::physics::constrainDistance( pod::Constraint& constraint, const pod::Vector3f& pA, const pod::Vector3f& pB, bool isRope ) {
	auto tA = impl::getTransform( *constraint.a );
	auto tB = impl::getTransform( *constraint.b );

	auto& joint = constraint.distance;
	constraint.type = pod::ConstraintType::DISTANCE;
	joint.localAnchorA = impl::applyInverse( tA, pA );
	joint.localAnchorB = impl::applyInverse( tB, pB );

	joint.targetDistance = uf::vector::distance( pB, pA );
	joint.accumulatedImpulse = 0.0f;
	joint.isRope = isRope;

	return constraint;
}

void impl::drawDistance( const pod::Constraint& constraint ) {
	if ( !constraint.a || !constraint.b ) return;

	const auto& joint = constraint.distance;
	auto tA = impl::getTransform( *constraint.a );
	auto tB = impl::getTransform( *constraint.b );

	auto pA = impl::apply( tA, joint.localAnchorA );
	auto pB = impl::apply( tB, joint.localAnchorB );

	uf::debug::drawLine( tA.position, pA );
	uf::debug::drawLine( tB.position, pB );

	uf::debug::drawLine( pA, pB );

	// crosshair
	float size = 0.1f;
	uf::debug::drawLine( pA - pod::Vector3f{size, 0.0f, 0.0f}, pA + pod::Vector3f{size, 0.0f, 0.0f} );
	uf::debug::drawLine( pA - pod::Vector3f{0.0f, size, 0.0f}, pA + pod::Vector3f{0.0f, size, 0.0f} );
	uf::debug::drawLine( pA - pod::Vector3f{0.0f, 0.0f, size}, pA + pod::Vector3f{0.0f, 0.0f, size} );

}