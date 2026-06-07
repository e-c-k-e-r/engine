#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/integration.h>
#include <uf/utils/math/physics/solvers.h>

void impl::solvePositions( uf::stl::vector<pod::Manifold>& manifolds, float dt, uint32_t iterations ) {
	if ( !uf::physics::settings.ngsPositionSolver ) return;
	if ( uf::physics::settings.baumgarteCorrectionPercent <= 0 ) return;
	for ( auto i = 0; i < iterations; ++i ) {
		for ( auto& manifold : manifolds ) {
			auto& a = *manifold.a;
			auto& b = *manifold.b;

			if ( (a.collider.category & pod::Collider::CATEGORY_TRIGGER) || (b.collider.category & pod::Collider::CATEGORY_TRIGGER) ) return;
			
			auto tA = impl::getTransform( a );
			auto tB = impl::getTransform( b );

			auto& aT = *a.transform;
			auto& bT = *b.transform;

			if ( a.inverseMass == 0.0f && b.inverseMass == 0.0f ) continue;

			for ( auto& c : manifold.points ) {
				auto ctxA = impl::solverBodyContext( a );
				auto ctxB = impl::solverBodyContext( b );

				auto rA = uf::quaternion::rotate( tA.orientation, c.localA * tA.scale );
				auto rB = uf::quaternion::rotate( tB.orientation, c.localB * tB.scale );

				auto pA = tA.position + rA;
				auto pB = tB.position + rB;
				
				auto row = pod::JacobianRow{ rA, rB, c.normal };

				float penetration = uf::vector::dot( pB - pA, c.normal );

				float C = std::clamp( penetration - uf::physics::settings.baumgarteCorrectionSlop, 0.0f, uf::physics::settings.maxLinearCorrection );
				if ( C <= 0.0f ) continue;

				float invMassN = impl::computeEffectiveMass( ctxA, ctxB, row );
				float lambda = (C / invMassN) * uf::physics::settings.baumgarteCorrectionPercent;
				pod::Vector3f P = c.normal * lambda;

				// apply impulses directly
				if ( ctxA.invM > 0.0f ) {
					pod::Vector3f translation = P * ctxA.invM;
					aT.position -= translation;
					tA.position -= translation;

					pod::Vector3f deltaAngleA = uf::matrix::multiply(ctxA.invI, uf::vector::cross(rA, -P));
					float angleA2 = uf::vector::magnitude( deltaAngleA );
					if ( angleA2 > EPS2 ) {
						float angleA = std::sqrt( angleA2 );
						pod::Quaternion<> dq = uf::quaternion::axisAngle(deltaAngleA / angleA, angleA);
						uf::transform::rotate( *a.transform, dq );
						tA.orientation = uf::quaternion::multiply(dq, tA.orientation);
					}	
				}

				if ( ctxB.invM > 0.0f ) {
					pod::Vector3f translation = P * ctxB.invM;
					bT.position += translation;
					tB.position += translation;

					pod::Vector3f deltaAngleB = uf::matrix::multiply(ctxB.invI, uf::vector::cross(rB, P));
					float angleB2 = uf::vector::magnitude( deltaAngleB );
					if ( angleB2 > EPS2 ) {
						float angleB = std::sqrt( angleB2 );
						pod::Quaternion<> dq = uf::quaternion::axisAngle(deltaAngleB / angleB, angleB);
						uf::transform::rotate( *b.transform, dq );
						tB.orientation = uf::quaternion::multiply(dq, tB.orientation);
					}
				}
			}
		}
	}
}