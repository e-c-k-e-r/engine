#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/integration.h>
#include <uf/utils/math/physics/solvers/block.h>

namespace impl {
	template<size_t N, typename T = float>
	void blockNxNSolver( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt ) {
		pod::Matrix<T,N> K = {};
		pod::Vector<T,N> rhs = {};
		pod::Vector<T,N> lambda = {};
		pod::Vector<T,N> residual = {};
		
		// precompute inverse masses
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

				// angular parts
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

		for ( auto i = 0; i < N; i++ ) {
			auto& contact = manifold.points[i];
			// full relative velocity, linear + angular
			pod::Vector3f vA = a.velocity + uf::vector::cross( a.angularVelocity, contact.point - pA );
			pod::Vector3f vB = b.velocity + uf::vector::cross( b.angularVelocity, contact.point - pB );
			float vRel = uf::vector::dot((vB - vA), contact.normal);

			// penetration bias with clamp
			float penetrationBias = std::max(contact.penetration - uf::physics::settings.baumgarteCorrectionSlop, 0.0f) * (uf::physics::settings.baumgarteCorrectionPercent / dt);
			penetrationBias = std::min(penetrationBias, 2.0f / dt); // clamp

			float maxPenetrationRecovery = 2.0f; // limit to 2 units per second
			if ( penetrationBias > maxPenetrationRecovery ) penetrationBias = maxPenetrationRecovery;

			rhs[i]	= -vRel + penetrationBias; // RHS is magnitude of correction needed
			lambda[i] = contact.accumulatedNormalImpulse;
		}

		residual = rhs - uf::matrix::multiply( K, lambda );
		pod::Matrix<T,N> Kinv = uf::matrix::inverse( K );
		pod::Vector<T,N> dLambda = uf::matrix::multiply( Kinv, residual );

		for ( auto i = 0; i < N; i++ ) {
			float newLambda = std::max(lambda[i] + dLambda[i], 0.0f);
			dLambda[i] = newLambda - lambda[i];
			lambda[i] = newLambda;
			manifold.points[i].accumulatedNormalImpulse = newLambda;
		}

		for ( auto i = 0; i < N; i++ ) {
			pod::Vector3f rA = manifold.points[i].point - pA;
			pod::Vector3f rB = manifold.points[i].point - pB;

			impl::applyImpulseTo( a, b, rA, rB, manifold.points[i].normal * dLambda[i] );
		}
	}
}

void impl::block2x2Solver( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt ) {
	return impl::blockNxNSolver<2>( a, b, manifold, dt );
}
void impl::block3x3Solver( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt ) {
	return impl::blockNxNSolver<3>( a, b, manifold, dt );
}
void impl::block4x4Solver( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt ) {
	return impl::blockNxNSolver<4>( a, b, manifold, dt );
}