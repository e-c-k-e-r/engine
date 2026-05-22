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

	auto rA = uf::quaternion::rotate( tA.orientation, joint.localAnchorA );
	auto rB = uf::quaternion::rotate( tB.orientation, joint.localAnchorB );

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