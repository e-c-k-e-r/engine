#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/integration.h>
#include <uf/utils/math/physics/solvers/pgs.h>

// Projected Gauss-Seidel solver
void impl::blockPGSSolver( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt ) {
	const uint32_t count = std::min( (uint32_t) manifold.points.size(), (uint32_t) 4 );
	for ( auto i = 0; i < count; i++ ) {
		auto& c = manifold.points[i];

		pod::Vector3f rA = c.point - impl::getPosition( a, true );
		pod::Vector3f rB = c.point - impl::getPosition( b, true );

		// normal impulse
		pod::Vector3f dv = ( b.velocity + uf::vector::cross( b.angularVelocity, rB ) ) -
						   ( a.velocity + uf::vector::cross( a.angularVelocity, rA ) );
		float vn = uf::vector::dot( dv, c.normal );

		float e = std::min( a.material.restitution, b.material.restitution );
		float restitutionBias = (vn < -1.0f) ? -e * vn : 0.0f;
		float effectiveMassN = impl::computeEffectiveMass( a, b, rA, rB, c.normal );

		float lambdaN = (-vn + restitutionBias) / effectiveMassN;
		float oldImpulseN = c.accumulatedNormalImpulse;
		c.accumulatedNormalImpulse = std::max( oldImpulseN + lambdaN, 0.0f );

		impl::applyImpulseTo( a, b, rA, rB, c.normal * (c.accumulatedNormalImpulse - oldImpulseN) );

		// tangent impulse
		dv = ( b.velocity + uf::vector::cross( b.angularVelocity, rB ) ) -
			 ( a.velocity + uf::vector::cross( a.angularVelocity, rA ) );

		pod::Vector3f tangent = dv - c.normal * uf::vector::dot(dv, c.normal);
		float tMag2 = uf::vector::magnitude(tangent);
		if ( tMag2 > EPS2 ) {
			tangent /= std::sqrt(tMag2);
			c.tangent = tangent;
		} else if ( uf::vector::magnitude(c.tangent) < EPS ) {
			c.tangent = impl::computeTangent( c.normal );
		}

		float vt = uf::vector::dot( dv, c.tangent );
		float effectiveMassT = impl::computeEffectiveMass( a, b, rA, rB, c.tangent );
		float lambdaT = -vt / effectiveMassT;

		float mu = std::sqrt(a.material.dynamicFriction * b.material.dynamicFriction);
		float maxFriction = mu * c.accumulatedNormalImpulse;

		float oldImpulseT = c.accumulatedTangentImpulse;
		c.accumulatedTangentImpulse = std::clamp( oldImpulseT + lambdaT, -maxFriction, maxFriction );

		impl::applyImpulseTo( a, b, rA, rB, c.tangent * (c.accumulatedTangentImpulse - oldImpulseT) );

		// pseudo impulse
		pod::Vector3f pseudoDv = ( b.pseudoVelocity + uf::vector::cross( b.pseudoAngularVelocity, rB ) ) -
								 ( a.pseudoVelocity + uf::vector::cross( a.pseudoAngularVelocity, rA ) );
		float pseudoVn = uf::vector::dot( pseudoDv, c.normal );

		float penetrationBias = std::max( c.penetration - uf::physics::settings.baumgarteCorrectionSlop, 0.0f ) *
								(uf::physics::settings.baumgarteCorrectionPercent / dt);
		penetrationBias = std::min( penetrationBias, 2.0f / dt );

		float lambdaP = (-pseudoVn + penetrationBias) / effectiveMassN;
		float oldImpulseP = c.accumulatedPseudoImpulse;
		c.accumulatedPseudoImpulse = std::max( oldImpulseP + lambdaP, 0.0f );

		impl::applyPseudoImpulseTo( a, b, rA, rB, c.normal * (c.accumulatedPseudoImpulse - oldImpulseP) );
	}
}