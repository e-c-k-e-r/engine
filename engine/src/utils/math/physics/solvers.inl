#include "./solvers/iterativeImpulse.inl"
#include "./solvers/block.inl"
#include "./solvers/psg.inl"

namespace {
	void resolveManifold( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt ) {
		if ( uf::physics::impl::settings.blockContactSolver ) {
			if ( manifold.points.size() == 2 ) return ::block2x2Solver( a, b, manifold, dt );
			if ( manifold.points.size() == 3 ) return ::block3x3Solver( a, b, manifold, dt );
			if ( manifold.points.size() == 4 ) return ::block4x4Solver( a, b, manifold, dt );
		}
		if ( uf::physics::impl::settings.psgContactSolver )  return ::blockPGSSolver( a, b, manifold, dt );
		for ( auto& contact : manifold.points ) ::iterativeImpulseSolver( a, b, contact, dt );
	}

	void solveContacts( uf::stl::vector<pod::Manifold>& manifolds, float dt ) {
		if ( uf::physics::impl::settings.warmupSolver ) for ( auto& manifold : manifolds ) ::warmupManifold( *manifold.a, *manifold.b, manifold, dt );
		for ( auto i = 0; i < uf::physics::impl::settings.solverIterations; ++i ) for ( auto& manifold : manifolds ) ::resolveManifold( *manifold.a, *manifold.b, manifold, dt );
	}

	void solvePositions( uf::stl::vector<pod::Manifold>& manifolds, float dt, uint32_t iterations = 2 ) {
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
				
				::positionCorrection( *m.a, *m.b, s );
			}
		}
	}
}