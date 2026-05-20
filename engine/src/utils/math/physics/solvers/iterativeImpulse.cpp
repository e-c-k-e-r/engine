#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/integration.h>
#include <uf/utils/math/physics/solvers/iterativeImpulse.h>

void impl::iterativeImpulseSolver( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Contact& contact, float dt ) {
	pod::Vector3f rA = contact.point - impl::getPosition( a, true );
	pod::Vector3f rB = contact.point - impl::getPosition( b, true );

	float invMassN = impl::computeEffectiveMass(a, b, rA, rB, contact.normal);

	// real impulse
	{
		pod::Vector3f vA = a.velocity + uf::vector::cross(a.angularVelocity, rA);
		pod::Vector3f vB = b.velocity + uf::vector::cross(b.angularVelocity, rB);
		pod::Vector3f rv = vB - vA;

		float restitutionBias = 0.0f;
		float e = std::min(a.material.restitution, b.material.restitution);
		float velAlongNormal = uf::vector::dot(rv, contact.normal);
		if ( velAlongNormal < -1.0f ) restitutionBias = -e * velAlongNormal;
		float targetVelocity = restitutionBias;

		float jn = (targetVelocity - velAlongNormal) / invMassN;

		float jnOld = contact.accumulatedNormalImpulse;
		float jnNew = std::max(0.0f, jnOld + jn);
		float jnDelta = jnNew - jnOld;
		contact.accumulatedNormalImpulse = jnNew;
		jn = jnDelta;

		impl::applyImpulseTo(a, b, rA, rB, contact.normal * jn);
	}
	// pseudo impulse
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

	// tangent friction
	{
		pod::Vector3f vA = a.velocity + uf::vector::cross(a.angularVelocity, rA);
		pod::Vector3f vB = b.velocity + uf::vector::cross(b.angularVelocity, rB);
		pod::Vector3f rv = vB - vA;
		pod::Vector3f tangent = rv - contact.normal * uf::vector::dot(rv, contact.normal);
		float tMag2 = uf::vector::magnitude(tangent);
		if ( tMag2 > EPS2 ) {
			contact.tangent = tangent / std::sqrt(tMag2);
		} else if ( uf::vector::magnitude(contact.tangent) < EPS ) {
			contact.tangent = impl::computeTangent( contact.normal );
		}

		float invMassT = impl::computeEffectiveMass(a, b, rA, rB, contact.tangent);
		float vt = uf::vector::dot(rv, contact.tangent);
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
		jt = jtDelta;

		impl::applyImpulseTo(a, b, rA, rB, contact.tangent * jt);
	}
}