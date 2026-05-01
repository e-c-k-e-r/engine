#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/integration.h>
#include <uf/utils/math/physics/solvers/psg.h>

void impl::blockPGSSolver( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt ) {
	const uint32_t count = std::min( (uint32_t) manifold.points.size(), (uint32_t) 4 );
	struct ContactCache {
		pod::Vector3f normal;
		pod::Vector3f tangent;
		pod::Vector3f rA, rB;
		float bias;
		float effectiveMassN;
		float effectiveMassT;
		float accumulatedNormalImpulse;
		float accumulatedTangentImpulse;
	};

	// precompute contact caches
	ContactCache cache[4];
	for ( auto i = 0; i < count; i++ ) {
		auto& c = manifold.points[i];
		auto& cc = cache[i];

		cc.normal = c.normal;
		cc.tangent = impl::computeTangent( c.normal );
		cc.rA = c.point - impl::getPosition( a, true );
		cc.rB = c.point - impl::getPosition( b, true );

		// relative velocity along normal
		pod::Vector3f dv = ( b.velocity + uf::vector::cross( b.angularVelocity, cc.rB ) ) - ( a.velocity + uf::vector::cross( a.angularVelocity, cc.rA ) );
		float vn = uf::vector::dot( dv, cc.normal );

		// restitution bias + baumgarte
		float e = std::min( a.material.restitution, b.material.restitution );
		float penetrationBias = std::max( c.penetration - uf::physics::settings.baumgarteCorrectionSlop, 0.0f ) * (uf::physics::settings.baumgarteCorrectionPercent / dt);
		float restitutionBias = (vn < -1.0f) ? -e * vn : 0.0f;
		cc.bias = restitutionBias + penetrationBias;

		// effective mass (normal)
		pod::Vector3f rnA = uf::vector::cross( cc.rA, cc.normal );
		pod::Vector3f rnB = uf::vector::cross( cc.rB, cc.normal );
		float Kn = (a.isStatic ? 0.0f : a.inverseMass) + (b.isStatic ? 0.0f : b.inverseMass) +
				   uf::vector::dot( uf::vector::cross( rnA * a.inverseInertiaTensor, cc.rA ) + uf::vector::cross( rnB * b.inverseInertiaTensor, cc.rB ), cc.normal );
		cc.effectiveMassN = (Kn > 0.0f) ? 1.0f / Kn : 0.0f;

		// effective mass (tangent)
		pod::Vector3f rtA = uf::vector::cross( cc.rA, cc.tangent );
		pod::Vector3f rtB = uf::vector::cross( cc.rB, cc.tangent );
		float Kt = (a.isStatic ? 0.0f : a.inverseMass) + (b.isStatic ? 0.0f : b.inverseMass) +
				   uf::vector::dot( uf::vector::cross( rtA * a.inverseInertiaTensor, cc.rA ) + uf::vector::cross( rtB * b.inverseInertiaTensor, cc.rB ), cc.tangent );
		cc.effectiveMassT = ( Kt > 0.0f ) ? ( 1.0f / Kt ) : 0.0f;

		// warm start
	#if 1
		cc.accumulatedNormalImpulse = c.accumulatedNormalImpulse;
		cc.accumulatedTangentImpulse = c.accumulatedTangentImpulse;

		// apply warm-start impulses
		pod::Vector3f P = cc.normal * cc.accumulatedNormalImpulse + cc.tangent * cc.accumulatedTangentImpulse;
		
		impl::applyImpulseTo(a, b, cc.rA, cc.rB, P);
	#endif
	}

	// iterative PGS
	for ( auto iter = 0; iter < uf::physics::settings.solverIterations; iter++ ) {
		for ( auto i = 0; i < count; i++ ) {
			auto& cc = cache[i];

			// relative velocity
			pod::Vector3f dv = ( b.velocity + uf::vector::cross( b.angularVelocity, cc.rB ) ) - ( a.velocity + uf::vector::cross( a.angularVelocity, cc.rA ) );

			// normal constraint
			float vn = uf::vector::dot( dv, cc.normal );
			float lambdaN = cc.effectiveMassN * (-vn + cc.bias);
			float oldImpulseN = cc.accumulatedNormalImpulse;
			cc.accumulatedNormalImpulse = std::max( oldImpulseN + lambdaN, 0.0f );
			float dPn = cc.accumulatedNormalImpulse - oldImpulseN;

			impl::applyImpulseTo( a, b, cc.rA, cc.rB, cc.normal * dPn );

			// friction constraint
			dv = ( b.velocity + uf::vector::cross( b.angularVelocity, cc.rB ) ) - ( a.velocity + uf::vector::cross( a.angularVelocity, cc.rA ) );
			float vt = uf::vector::dot( dv, cc.tangent );
			float lambdaT = cc.effectiveMassT * (-vt);
			float maxFriction = ( a.material.dynamicFriction + b.material.dynamicFriction ) * 0.5f * cc.accumulatedNormalImpulse;

			float oldImpulseT = cc.accumulatedTangentImpulse;
			cc.accumulatedTangentImpulse = std::clamp( oldImpulseT + lambdaT, -maxFriction, maxFriction );
			float dPt = cc.accumulatedTangentImpulse - oldImpulseT;

			impl::applyImpulseTo( a, b, cc.rA, cc.rB, cc.tangent * dPt );
		}
	}

	// store impulses back into manifold
	for ( auto i = 0; i < count; i++ ) {
		manifold.points[i].accumulatedNormalImpulse = cache[i].accumulatedNormalImpulse;
		manifold.points[i].accumulatedTangentImpulse = cache[i].accumulatedTangentImpulse;
	}
}