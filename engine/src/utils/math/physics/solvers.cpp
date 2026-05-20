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
// unused
void impl::solvePositions( uf::stl::vector<pod::Manifold>& manifolds, float dt, uint32_t iterations ) {
	for ( auto i = 0; i < iterations; ++i ) {
		for ( auto& m : manifolds ) {
			pod::Contact s = {};
			float weight = 0;
			for ( auto& c : m.points ) {
				float w = std::max( c.penetration, 0.0f );
				s.normal += c.normal * w;
				s.penetration = std::max(s.penetration, c.penetration);
				weight += w;
			}
			s.normal = weight > 0.0f ? uf::vector::normalize(s.normal) : pod::Vector3f{0,1,0};
			
			impl::positionCorrection( *m.a, *m.b, s );
		}
	}
}