#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/integration.h>
#include <uf/utils/math/physics/solvers/iterativeImpulse.h>

void impl::iterativeImpulseSolver( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Contact& contact, float dt ) {
	pod::Vector3f rA = contact.point - impl::getPosition( a, true );
	pod::Vector3f rB = contact.point - impl::getPosition( b, true );

	pod::Vector3f vA = a.velocity + uf::vector::cross(a.angularVelocity, rA);
	pod::Vector3f vB = b.velocity + uf::vector::cross(b.angularVelocity, rB);
	pod::Vector3f rv = vB - vA;

	float velAlongNormal = uf::vector::dot(rv, contact.normal);

	float e = std::min(a.material.restitution, b.material.restitution);
	float restitutionBias = 0.0f;
	if ( velAlongNormal < -1.0f ) restitutionBias = -e * velAlongNormal;

	float targetVelocity = restitutionBias;
	float invMassN = impl::computeEffectiveMass(a, b, rA, rB, contact.normal);

	{
		float jn = (targetVelocity - velAlongNormal) / invMassN;

		float jnOld = contact.accumulatedNormalImpulse;
		float jnNew = std::max(0.0f, jnOld + jn);
		float jnDelta = jnNew - jnOld;
		contact.accumulatedNormalImpulse = jnNew;
		jn = jnDelta;

		impl::applyImpulseTo(a, b, rA, rB, contact.normal * jn);
	}
	{
		float penetrationBias = std::max(contact.penetration - uf::physics::settings.baumgarteCorrectionSlop, 0.0f) * (uf::physics::settings.baumgarteCorrectionPercent / dt);
		penetrationBias = std::min(penetrationBias, 2.0f / dt);

		pod::Vector3f pseudoVa = a.pseudoVelocity + uf::vector::cross(a.pseudoAngularVelocity, rA);
		pod::Vector3f pseudoVb = b.pseudoVelocity + uf::vector::cross(b.pseudoAngularVelocity, rB);
		float pseudoVelAlongNormal = uf::vector::dot(pseudoVb - pseudoVa, contact.normal);

		float jPseudo = (penetrationBias - pseudoVelAlongNormal) / invMassN;

		float jPseudoOld = contact.accumulatedPseudoImpulse;
		float jPseudoNew = std::max(0.0f, jPseudoOld + jPseudo);
		contact.accumulatedPseudoImpulse = jPseudoNew;
		jPseudo = jPseudoNew - jPseudoOld;

		impl::applyPseudoImpulseTo(a, b, rA, rB, contact.normal * jPseudo);
	}

	// tangent direction
	pod::Vector3f tangent = rv - contact.normal * uf::vector::dot(rv, contact.normal);
	float tangentMag2 = uf::vector::magnitude(tangent);
	if ( tangentMag2 > EPS2 ) {
		tangent /= std::sqrt( tangentMag2 );
		float invMassT = impl::computeEffectiveMass(a, b, rA, rB, tangent);
		float vt = uf::vector::dot(rv, tangent);
		float jt = -vt / invMassT;

		float mu_s = std::sqrt(a.material.staticFriction * b.material.staticFriction);
		float mu_d = std::sqrt(a.material.dynamicFriction * b.material.dynamicFriction);

		float normalForce = contact.accumulatedNormalImpulse;
		if ( std::fabs(jt) > normalForce * mu_s ) {
			jt = -normalForce * mu_d;
		}

		float maxFriction = mu_s * normalForce;
		float jtOld = contact.accumulatedTangentImpulse;
		float jtNew = std::clamp(jtOld + jt, -maxFriction, maxFriction);
		float jtDelta = jtNew - jtOld;
		contact.accumulatedTangentImpulse = jtNew;
		contact.tangent = tangent;
		jt = jtDelta;

		impl::applyImpulseTo(a, b, rA, rB, tangent * jt);
	}
}