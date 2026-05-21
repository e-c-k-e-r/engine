#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/integration.h>
#include <uf/utils/math/physics/solvers.h>

void impl::resolveManifold( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt ) {
	if ( uf::physics::settings.blockContactSolver ) {
		if ( impl::blockSolver( a, b, manifold, dt ) ) return;
	}
	for ( auto& contact : manifold.points ) impl::iterativeImpulseSolver( a, b, contact, dt );
}

void impl::solveContacts( uf::stl::vector<pod::Manifold>& manifolds, float dt ) {
	if ( uf::physics::settings.warmupSolver ) for ( auto& manifold : manifolds ) impl::warmupManifold( *manifold.a, *manifold.b, manifold, dt );
	for ( auto i = 0; i < uf::physics::settings.solverIterations; ++i ) for ( auto& manifold : manifolds ) impl::resolveManifold( *manifold.a, *manifold.b, manifold, dt );
}
void impl::solvePositions( uf::stl::vector<pod::Manifold>& manifolds, float dt, uint32_t iterations ) {
	if ( true || uf::physics::settings.baumgarteCorrectionPercent <= 0 ) return;
	for ( auto i = 0; i < iterations; ++i ) {
		float minSeparation = 0.0f;

		for ( auto& manifold : manifolds ) {
			auto& a = *manifold.a;
			auto& b = *manifold.b;
			auto tA = impl::getTransform( a );
			auto tB = impl::getTransform( b );

			if ( a.isStatic && b.isStatic ) continue;

			for ( auto& c : manifold.points ) {
				pod::Vector3f rA = uf::quaternion::rotate( tA.orientation, c.localA );
				pod::Vector3f rB = uf::quaternion::rotate( tB.orientation, c.localB );
				pod::Vector3f worldA = tA.position + rA;
				pod::Vector3f worldB = tB.position + rB;

				float penetration = -uf::vector::dot( worldB - worldA, c.normal );
				minSeparation = std::min( minSeparation, -penetration );

				float C = std::clamp( penetration - uf::physics::settings.baumgarteCorrectionSlop, 0.0f, uf::physics::settings.maxLinearCorrection );
				if ( C <= 0.0f ) continue;

				float invMassN = impl::computeEffectiveMass( a, b, rA, rB, c.normal );

				float lambda = (C / invMassN) * uf::physics::settings.baumgarteCorrectionPercent;
				pod::Vector3f P = c.normal * lambda;

				// apply impulses directly
				if ( !a.isStatic ) {
					pod::Vector3f translation = P * a.inverseMass;
					a.transform->position -= translation;
					tA.position -= translation;

					pod::Matrix3f invIa = impl::computeWorldInverseInertia(a);
					pod::Vector3f deltaAngleA = uf::matrix::multiply(invIa, uf::vector::cross(rA, -P));

					float angleA2 = uf::vector::magnitude( deltaAngleA );
					if ( angleA2 > EPS2 ) {
						float angleA = std::sqrt( angleA2);
						pod::Quaternion<> dq = uf::quaternion::axisAngle(deltaAngleA / angleA, angleA);
						uf::transform::rotate( *a.transform, dq );
						tA.orientation = uf::quaternion::multiply(dq, tA.orientation);
					}	
				}

				if ( !b.isStatic ) {
					pod::Vector3f translation = P * b.inverseMass;
					b.transform->position += translation;
					tB.position += translation;

					pod::Matrix3f invIb = impl::computeWorldInverseInertia(b);
					pod::Vector3f deltaAngleB = uf::matrix::multiply(invIb, uf::vector::cross(rB, P));

					float angleB2 = uf::vector::magnitude( deltaAngleB );
					if ( angleB2 > EPS2 ) {
						float angleB = std::sqrt( angleB2);
						pod::Quaternion<> dq = uf::quaternion::axisAngle(deltaAngleB / angleB, angleB);
						uf::transform::rotate( *b.transform, dq );
						tB.orientation = uf::quaternion::multiply(dq, tB.orientation);
					}
				}
			}
		}
	}
}