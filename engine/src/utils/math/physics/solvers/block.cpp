#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/integration.h>
#include <uf/utils/math/physics/solvers/block.h>

namespace impl {
	template<size_t N, typename T = float>
	bool blockNxNSolver( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt ) {
		pod::Matrix<T,N> K = {};
		
		float invMassA = ( a.isStatic ? 0.0f : a.inverseMass );
		float invMassB = ( b.isStatic ? 0.0f : b.inverseMass );
		
		pod::Matrix3f invIa = computeWorldInverseInertia( a );
		pod::Matrix3f invIb = computeWorldInverseInertia( b );

		auto pA = impl::getPosition( a, true );
		auto pB = impl::getPosition( b, true );

		for ( auto i = 0; i < N; i++ ) {
			pod::Vector3f rA_i = manifold.points[i].point - pA;
			pod::Vector3f rB_i = manifold.points[i].point - pB;
			pod::Vector3f n_i = manifold.points[i].normal;
			
			for ( auto j = 0; j < N; j++ ) {
				pod::Vector3f rA_j = manifold.points[j].point - pA;
				pod::Vector3f rB_j = manifold.points[j].point - pB;
				pod::Vector3f n_j = manifold.points[j].normal;

				float termLinear = (invMassA + invMassB) * uf::vector::dot(n_i, n_j);

				pod::Vector3f raXnj = uf::vector::cross(rA_j, n_j);
				pod::Vector3f rbXnj = uf::vector::cross(rB_j, n_j);

				pod::Vector3f Ia_raXnj = uf::matrix::multiply( invIa, raXnj );
				pod::Vector3f Ib_rbXnj = uf::matrix::multiply( invIb, rbXnj );

				pod::Vector3f crossA = uf::vector::cross(Ia_raXnj, rA_i);
				pod::Vector3f crossB = uf::vector::cross(Ib_rbXnj, rB_i);

				float termAngular = uf::vector::dot(n_i, crossA + crossB);

				K(i,j) = termLinear + termAngular;
			}

			K(i,i) += 1e-3f;
		}

		pod::Vector<T,N> rhsVel = {};
		pod::Vector<T,N> K_lambdaVel = {};

		pod::Vector<T,N> rhsPos = {};
		pod::Vector<T,N> K_lambdaPos = {};

		for ( auto i = 0; i < N; i++ ) {
			auto& contact = manifold.points[i];

			pod::Vector3f vA = a.velocity + uf::vector::cross( a.angularVelocity, contact.point - pA );
			pod::Vector3f vB = b.velocity + uf::vector::cross( b.angularVelocity, contact.point - pB );
			float vRel = uf::vector::dot((vB - vA), contact.normal);

			float e = std::min(a.material.restitution, b.material.restitution);
			float restitutionBias = (vRel < -1.0f) ? -e * vRel : 0.0f;

			rhsVel[i] = -vRel + restitutionBias;

			pod::Vector3f pseudoVa = a.pseudoVelocity + uf::vector::cross( a.pseudoAngularVelocity, contact.point - pA );
			pod::Vector3f pseudoVb = b.pseudoVelocity + uf::vector::cross( b.pseudoAngularVelocity, contact.point - pB );
			float pseudoVRel = uf::vector::dot((pseudoVb - pseudoVa), contact.normal);

			float penetrationBias = std::max(contact.penetration - uf::physics::settings.baumgarteCorrectionSlop, 0.0f) * (uf::physics::settings.baumgarteCorrectionPercent / dt);

			rhsPos[i] = -pseudoVRel + penetrationBias;

			K_lambdaVel[i] = contact.accumulatedNormalImpulse;
			K_lambdaPos[i] = contact.accumulatedPseudoImpulse;
		}

		pod::Matrix<T,N> Kinv = uf::matrix::inverse( K );

		pod::Vector<T,N> residualVel = rhsVel - uf::matrix::multiply( K, K_lambdaVel );
		pod::Vector<T,N> dLambdaVel = uf::matrix::multiply( Kinv, residualVel );

		pod::Vector<T,N> residualPos = rhsPos - uf::matrix::multiply( K, K_lambdaPos );
		pod::Vector<T,N> dLambdaPos = uf::matrix::multiply( Kinv, residualPos );

		// check if contacts are all valid
		int invalidContactIndex = -1;
		for ( auto i = 0; i < N; i++ ) {
			if ( K_lambdaVel[i] + dLambdaVel[i] < 0.0f || K_lambdaPos[i] + dLambdaPos[i] < 0.0f ) {
				invalidContactIndex = i;
				break;
			}
		}
		// invalid contact found
		if ( invalidContactIndex != -1 ) {
			bool success = false;
			// reduce the manifold
			if ( N > 1 ) {
				pod::Manifold reducedManifold = manifold;
				reducedManifold.points.erase( reducedManifold.points.begin() + invalidContactIndex );
				manifold.points[invalidContactIndex].accumulatedNormalImpulse = 0.0f;
				manifold.points[invalidContactIndex].accumulatedPseudoImpulse = 0.0f;
				manifold.points[invalidContactIndex].accumulatedTangentImpulse = 0.0f;

				// re-solve
				success = impl::blockSolver( a, b, reducedManifold, dt );
				// copy back to original manifold
				for ( size_t i = 0, r = 0; i < N; i++ ) {
					if ( i != invalidContactIndex ) {
						manifold.points[i] = reducedManifold.points[r++];
					}
				}
			}
			return success;
		}

		for ( auto i = 0; i < N; i++ ) {
			auto& contact = manifold.points[i];
			pod::Vector3f rA = manifold.points[i].point - pA;
			pod::Vector3f rB = manifold.points[i].point - pB;

			// real impulse
			{
				float newLambdaVel = contact.accumulatedNormalImpulse + dLambdaVel[i];
				dLambdaVel[i] = newLambdaVel - contact.accumulatedNormalImpulse;
				contact.accumulatedNormalImpulse = newLambdaVel;
				impl::applyImpulseTo( a, b, rA, rB, manifold.points[i].normal * dLambdaVel[i] );
			}
			// pseudo impulse
			{
				float newLambdaPos = contact.accumulatedPseudoImpulse + dLambdaPos[i];
				dLambdaPos[i] = newLambdaPos - contact.accumulatedPseudoImpulse;
				contact.accumulatedPseudoImpulse = newLambdaPos;
				impl::applyPseudoImpulseTo( a, b, rA, rB, manifold.points[i].normal * dLambdaPos[i] );
			}
			// friction
			{
				pod::Vector3f vA = a.velocity + uf::vector::cross( a.angularVelocity, rA );
				pod::Vector3f vB = b.velocity + uf::vector::cross( b.angularVelocity, rB );
				pod::Vector3f rv = vB - vA;

				pod::Vector3f tangent = rv - contact.normal * uf::vector::dot(rv, contact.normal);
				float tangentMag2 = uf::vector::magnitude(tangent);

				if ( tangentMag2 > EPS2 ) {
					tangent /= std::sqrt( tangentMag2 );

					float invMassT = impl::computeEffectiveMass(a, b, rA, rB, tangent);
					float vt = uf::vector::dot(rv, tangent);
					float jt = -vt / invMassT;

					float mu = std::sqrt(a.material.dynamicFriction * b.material.dynamicFriction);
					float maxFriction = mu * contact.accumulatedNormalImpulse;

					float jtOld = contact.accumulatedTangentImpulse;
					float jtNew = std::clamp(jtOld + jt, -maxFriction, maxFriction);
					contact.accumulatedTangentImpulse = jtNew;

					impl::applyImpulseTo(a, b, rA, rB, tangent * (jtNew - jtOld));
				}
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