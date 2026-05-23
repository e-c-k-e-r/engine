#include <uf/utils/math/physics/impl.h>
#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/integration.h>
#include <uf/utils/math/physics/solvers/iterativeImpulse.h>

void impl::iterativeImpulseSolver( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Contact& contact, float dt ) {
	auto rA = contact.point - impl::getPosition( a, true );
	auto rB = contact.point - impl::getPosition( b, true );

	float invMassN = impl::computeEffectiveMass(a, b, rA, rB, contact.normal);

	// normal impulse
	{
		auto vA = a.velocity + uf::vector::cross(a.angularVelocity, rA);
		auto vB = b.velocity + uf::vector::cross(b.angularVelocity, rB);
		auto rv = vB - vA;

		auto gA = uf::physics::getGravity( a );
		auto gB = uf::physics::getGravity( b );
		float vSlop = std::sqrt( std::max( uf::vector::magnitude( gA ), uf::vector::magnitude( gB ) ) ) * dt;

		float restitutionBias = 0.0f;
		float e = std::min(a.material.restitution, b.material.restitution);
		float velAlongNormal = uf::vector::dot(rv, contact.normal);
		if ( velAlongNormal < -vSlop ) restitutionBias = -e * velAlongNormal;
		float targetVelocity = restitutionBias;

		float jN = (targetVelocity - velAlongNormal) / invMassN;
		impl::applyImpulseTo(a, b, rA, rB, contact.normal, jN, contact.accumulatedNormalImpulse, 0 );
	}
	// pseudo impulse
	if ( !uf::physics::settings.ngsPositionSolver ) {
		float penetrationBias = std::max(contact.penetration - uf::physics::settings.baumgarteCorrectionSlop, 0.0f) * (uf::physics::settings.baumgarteCorrectionPercent / dt);
		penetrationBias = std::min(penetrationBias, uf::physics::settings.maxLinearCorrection / dt);

		auto pseudoVa = a.pseudoVelocity + uf::vector::cross(a.pseudoAngularVelocity, rA);
		auto pseudoVb = b.pseudoVelocity + uf::vector::cross(b.pseudoAngularVelocity, rB);
		float pseudoVelAlongNormal = uf::vector::dot(pseudoVb - pseudoVa, contact.normal);

		float jP = (penetrationBias - pseudoVelAlongNormal) / invMassN;
		impl::applyPseudoImpulseTo(a, b, rA, rB, contact.normal, jP, contact.accumulatedPseudoImpulse, 0 );
	}
	// tangent friction
	{
		auto vA = a.velocity + uf::vector::cross(a.angularVelocity, rA);
		auto vB = b.velocity + uf::vector::cross(b.angularVelocity, rB);
		auto rv = vB - vA;
		auto tangent = rv - contact.normal * uf::vector::dot(rv, contact.normal);
		float tMag2 = uf::vector::magnitude(tangent);
		if ( tMag2 > EPS2 ) {
			contact.tangent = tangent / std::sqrt( tMag2 );
		} else if ( uf::vector::magnitude(contact.tangent) < EPS2 ) {
			contact.tangent = impl::computeTangent( contact.normal );
		}

		float invMassT = impl::computeEffectiveMass(a, b, rA, rB, contact.tangent);
		float vt = uf::vector::dot(rv, contact.tangent);
		float jT = -vt / invMassT;

		float mu_s = std::sqrt(a.material.staticFriction * b.material.staticFriction);
		float mu_d = std::sqrt(a.material.dynamicFriction * b.material.dynamicFriction);

		float normalForce = contact.accumulatedNormalImpulse;
		if ( std::fabs(jT) > normalForce * mu_s ) {
			jT = -normalForce * mu_d;
		}
		float maxFriction = mu_s * normalForce;
		impl::applyImpulseTo( a, b, rA, rB, contact.tangent, jT, contact.accumulatedTangentImpulse, -maxFriction, maxFriction );
	}
}