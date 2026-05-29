#include <uf/utils/math/physics/impl.h>
#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/integration.h>
#include <uf/utils/math/physics/constraints.h>

void impl::solveSpringConstraint( pod::Constraint& constraint, float dt ) {
	auto& a = *constraint.a;
	auto& b = *constraint.b;
	auto& joint = constraint.spring;

	auto tA = impl::getTransform( a );
	auto tB = impl::getTransform( b );

	auto rA = uf::quaternion::rotate( tA.orientation, joint.localAnchorA * tA.scale );
	auto rB = uf::quaternion::rotate( tB.orientation, joint.localAnchorB * tB.scale );

	auto worldAnchorA = tA.position + rA;
	auto worldAnchorB = tB.position + rB;
	auto delta = worldAnchorB - worldAnchorA;

	float currentDistance2 = uf::vector::magnitude( delta );
	if ( currentDistance2 < EPS ) return;
	float currentDistance = std::sqrt( currentDistance2 );

	pod::Vector3f normal = delta / currentDistance;
	float distanceError = currentDistance - joint.restLength;

	auto vA = a.velocity + uf::vector::cross(a.angularVelocity, rA);
	auto vB = b.velocity + uf::vector::cross(b.angularVelocity, rB);
	float relVelAlongNormal = uf::vector::dot( vB - vA, normal );

	float invMassN = impl::computeEffectiveMass( a, b, rA, rB, normal );
	if ( invMassN < EPS ) return;

	float gamma = joint.damping + (dt * joint.stiffness);
	gamma = (gamma > EPS) ? (1.0f / (dt * gamma)) : 0.0f;

	float beta = dt * joint.stiffness * gamma;

	float bias = distanceError * (beta / dt);
	float j = -(relVelAlongNormal + bias + (gamma * joint.accumulatedImpulse)) / (invMassN + gamma);

	impl::applyImpulseTo( a, b, rA, rB, normal, j, joint.accumulatedImpulse );
}

pod::Constraint& uf::physics::constrainSpring( pod::Constraint& constraint, const pod::Vector3f& pA, const pod::Vector3f& pB, float stiffness, float damping ) {
	auto tA = impl::getTransform( *constraint.a );
	auto tB = impl::getTransform( *constraint.b );

	auto& joint = constraint.spring;
	constraint.type = pod::ConstraintType::SPRING;
	joint.localAnchorA = uf::transform::applyInverse( tA, pA );
	joint.localAnchorB = uf::transform::applyInverse( tB, pB );

	joint.restLength = uf::vector::distance( pB, pA );
	joint.stiffness = stiffness;
	joint.damping = damping;
	joint.accumulatedImpulse = 0.0f;

	return constraint;
}

void impl::drawSpring( const pod::Constraint& constraint ) {
	if ( !constraint.a || !constraint.b ) return;

}