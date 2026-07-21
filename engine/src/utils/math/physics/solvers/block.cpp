#include <uf/utils/math/physics/impl.h>
#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/integration.h>
#include <uf/utils/math/physics/solvers/block.h>

namespace impl {
	template<size_t N, typename T = float>
	bool blockNxNSolver( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt ) {
		auto ctxA = impl::solverBodyContext( a );
		auto ctxB = impl::solverBodyContext( b );

		auto pA = impl::getPosition( a, true );
		auto pB = impl::getPosition( b, true );

		auto gA = uf::physics::getGravity( a );
		auto gB = uf::physics::getGravity( b );
		float vSlop = std::sqrt( std::max( uf::vector::magnitude( gA ), uf::vector::magnitude( gB ) ) ) * dt;

		pod::Matrix<T,N> K = {};
		for ( auto i = 0; i < N; i++ ) {
			auto& cI = manifold.points[i];
			auto rowI = pod::JacobianRow{ cI.point - pA, cI.point - pB, cI.normal };

			for ( auto j = 0; j < N; j++ ) {
				auto& cJ = manifold.points[j];
				auto rowJ = pod::JacobianRow{ cJ.point - pA, cJ.point - pB, cJ.normal };
				K(i,j) = impl::computeMassMatrixLine( ctxA, ctxB, rowI, rowJ );
			}

			K(i,i) += uf::physics::settings.contactCFM * ( 1.0f + ctxA.invM + ctxB.invM );
		}

		pod::Vector<T,N> rhs = {};
		pod::Vector<T,N> K_lambda = {};

		for ( auto i = 0; i < N; i++ ) {
			auto& contact = manifold.points[i];

			pod::Vector3f vA = a.velocity + uf::vector::cross( a.angularVelocity, contact.point - pA );
			pod::Vector3f vB = b.velocity + uf::vector::cross( b.angularVelocity, contact.point - pB );
			float vRel = uf::vector::dot((vB - vA), contact.normal);

			float e = std::min(a.material.restitution, b.material.restitution);
			if ( a.inverseMass == 0.0f || b.inverseMass == 0.0f) e = 0.0f;
			float restitutionBias = (vRel < -vSlop) ? -e * vRel : 0.0f;

			rhs[i] = -vRel + restitutionBias;
			K_lambda[i] = contact.accumulatedNormalImpulse;
		}

		pod::Matrix<T,N> Kinv = uf::matrix::inverse( K );

		pod::Vector<T,N> residual = rhs - uf::matrix::multiply( K, K_lambda );
		pod::Vector<T,N> dLambda = uf::matrix::multiply( Kinv, residual );

		// check if contacts are all valid
		int invalidContactIndex = -1;
		for ( auto i = 0; i < N; i++ ) {
			if ( K_lambda[i] + dLambda[i] < 0.0f ) {
				invalidContactIndex = i;
				break;
			}
		}
		// invalid contact found
		if ( invalidContactIndex != -1 ) {
			bool success = false;
			// reduce the manifold
			if ( uf::physics::settings.resolveBlockContact && N > 1 ) {
			#if 1
				pod::Manifold reducedManifold = manifold;
				reducedManifold.points.erase( reducedManifold.points.begin() + invalidContactIndex );
				// re-solve
				success = impl::blockSolver( a, b, reducedManifold, dt );
				// copy back to original manifold
				if ( success ) manifold = reducedManifold;
			#else
				pod::Manifold reducedManifold = manifold;
				reducedManifold.points.erase( reducedManifold.points.begin() + invalidContactIndex );
				manifold.points[invalidContactIndex].accumulatedNormalImpulse = 0.0f;
				manifold.points[invalidContactIndex].accumulatedTangentImpulse = 0.0f;

				// re-solve
				success = impl::blockSolver( a, b, reducedManifold, dt );
				// copy back to original manifold
				for ( size_t i = 0, r = 0; i < N; i++ ) {
					if ( i != invalidContactIndex ) {
						manifold.points[i] = reducedManifold.points[r++];
					}
				}
			#endif
			}
			return success;
		}

		for ( auto i = 0; i < N; i++ ) {
			auto& contact = manifold.points[i];
			pod::Vector3f rA = contact.point - pA;
			pod::Vector3f rB = contact.point - pB;

			// normal impulse
			{
				float jN = dLambda[i];
				impl::applyImpulseTo( a, b, rA, rB, contact.normal, jN, contact.accumulatedNormalImpulse );
			}
			// pseudo impulse
			if ( !uf::physics::settings.ngsPositionSolver ) {
				float penetrationBias = std::max(contact.penetration - uf::physics::settings.baumgarteCorrectionSlop, 0.0f) * (uf::physics::settings.baumgarteCorrectionPercent / dt);
				penetrationBias = std::min(penetrationBias, uf::physics::settings.maxLinearCorrection / dt);

				pod::Vector3f pseudoVa = a.pseudoVelocity + uf::vector::cross(a.pseudoAngularVelocity, rA);
				pod::Vector3f pseudoVb = b.pseudoVelocity + uf::vector::cross(b.pseudoAngularVelocity, rB);
				float pseudoVelAlongNormal = uf::vector::dot(pseudoVb - pseudoVa, contact.normal);

				float invMassN = impl::computeEffectiveMass(a, b, rA, rB, contact.normal);
				float jP = (penetrationBias - pseudoVelAlongNormal) / invMassN;
				impl::applyPseudoImpulseTo(a, b, rA, rB, contact.normal, jP, contact.accumulatedPseudoImpulse, 0 );
			}
			// tangent friction
			{
				pod::Vector3f vA = a.velocity + uf::vector::cross( a.angularVelocity, rA );
				pod::Vector3f vB = b.velocity + uf::vector::cross( b.angularVelocity, rB );
				pod::Vector3f rv = vB - vA;

				pod::Vector3f tangent = rv - contact.normal * uf::vector::dot(rv, contact.normal);
				float tMag2 = uf::vector::magnitude(tangent);
				if ( tMag2 > EPS2 ) {
					contact.tangent = tangent / std::sqrt( tMag2 );
				} else if ( uf::vector::magnitude(contact.tangent) < EPS2 ) {
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
			/*
				float mu = std::sqrt(a.material.dynamicFriction * b.material.dynamicFriction);
				float maxFriction = mu * contact.accumulatedNormalImpulse;
			*/
				impl::applyImpulseTo( a, b, rA, rB, contact.tangent, jt, contact.accumulatedTangentImpulse, -maxFriction, maxFriction );
			}
		}

		return true;
	}
}

bool impl::block2x2Solver( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt ) {
	return impl::blockNxNSolver<2>( a, b, manifold, dt );
}
bool impl::block3x3Solver( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt ) {
	return impl::blockNxNSolver<3>( a, b, manifold, dt );
}
bool impl::block4x4Solver( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt ) {
	return impl::blockNxNSolver<4>( a, b, manifold, dt );
}
bool impl::blockSolver( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt ) {
	if ( manifold.points.size() == 2 ) return impl::block2x2Solver( a, b, manifold, dt );
	if ( manifold.points.size() == 3 ) return impl::block3x3Solver( a, b, manifold, dt );
	if ( manifold.points.size() == 4 ) return impl::block4x4Solver( a, b, manifold, dt );
	return false;
}