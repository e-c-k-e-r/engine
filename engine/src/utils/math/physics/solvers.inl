namespace {
	void iterativeImpulseSolver( pod::RigidBody& a, pod::RigidBody& b, pod::Contact& contact, float dt ) {
		// relative positions from centers to contact point
		pod::Vector3f rA = contact.point - ::getPosition( a, true );
		pod::Vector3f rB = contact.point - ::getPosition( b, true );

		// relative velocity at contact
		pod::Vector3f vA = a.velocity + uf::vector::cross(a.angularVelocity, rA);
		pod::Vector3f vB = b.velocity + uf::vector::cross(b.angularVelocity, rB);
		pod::Vector3f rv = vB - vA;

		// normal contact velocity
		float velAlongNormal = uf::vector::dot(rv, contact.normal);
		float velTolerance = 0; // -1.0e3f;
		if ( velAlongNormal > velTolerance ) return; // if separating, no impulse

		// compute restitution (bounce)
		float e = std::min(a.material.restitution, b.material.restitution);

		// nullify restitution if velocity is small enough
		if ( fabs(velAlongNormal) < 1.0f) e = 0.0f;

		// effective inverse mass along normal
		float invMassN = ::computeEffectiveMass(a, b, rA, rB, contact.normal);

		// normal impulse scalar
		float jn = -(1.0f + e) * velAlongNormal;
		jn /= invMassN;
		if ( ::warmupSolver ) {
			float jnOld = contact.accumulatedNormalImpulse;
			float jnNew = std::max(0.0f, jnOld + jn);
			float jnDelta = jnNew - jnOld;
			contact.accumulatedNormalImpulse = jnNew;
			jn = jnDelta;
		}

		::applyImpulseTo(a, b, rA, rB, contact.normal * jn);

		// tangent direction
		pod::Vector3f tangent = rv - contact.normal * uf::vector::dot(rv, contact.normal);
		float tangentMag = uf::vector::magnitude(tangent);
		if (tangentMag > EPS(1e-6f)) {
			tangent /= tangentMag;

			// effective mass along tangent
			float invMassT = ::computeEffectiveMass(a, b, rA, rB, tangent);

			// tangential relative velocity
			float vt = uf::vector::dot(rv, tangent);

			// required tangential impulse to cancel tangent velocity
			float jt = -vt / invMassT;

			// friction coefficients
			float mu_s = std::sqrt(a.material.staticFriction * b.material.staticFriction);
			float mu_d = std::sqrt(a.material.dynamicFriction * b.material.dynamicFriction);
			
			if ( std::fabs(jt) > jn * mu_s) jt = -jn * mu_d; // dynamic friction: resist sliding proportionally

			if ( ::warmupSolver ) {
				float maxFriction = mu_s * contact.accumulatedNormalImpulse;
				float jtOld = contact.accumulatedTangentImpulse;
				float jtNew = std::max(-maxFriction, std::min(jtOld + jt, maxFriction));
				float jtDelta = jtNew - jtOld;
				contact.accumulatedTangentImpulse = jtNew;
				contact.tangent = tangent;
				jt = jtDelta;
			}

			::applyImpulseTo(a, b, rA, rB, tangent * jt);
		}

		::positionCorrection(a, b, contact);
	}

	template<size_t N, typename T = float>
	void blockNxNSolver( pod::RigidBody& a, pod::RigidBody& b, pod::Manifold& manifold, float dt ) {
		pod::Matrix<T,N> K = {};
		pod::Vector<T,N> rhs = {};
		pod::Vector<T,N> lambda = {};
		pod::Vector<T,N> residual = {};
		
		// precompute inverse masses
		float invMassA = ( a.isStatic ? 0.0f : a.inverseMass );
		float invMassB = ( b.isStatic ? 0.0f : b.inverseMass );

		auto pA = ::getPosition( a, true );
		auto pB = ::getPosition( b, true );

		for (int i = 0; i < N; i++) {
			pod::Vector3f rA_i = manifold.points[i].point - pA;
			pod::Vector3f rB_i = manifold.points[i].point - pB;

			for (int j = 0; j < N; j++) {
				pod::Vector3f rA_j = manifold.points[j].point - pA;
				pod::Vector3f rB_j = manifold.points[j].point - pB;

				pod::Vector3f n_i = manifold.points[i].normal;
				pod::Vector3f n_j = manifold.points[j].normal;

				float termLinear = (invMassA + invMassB) * uf::vector::dot(n_i, n_j);

				// angular parts
				pod::Vector3f raXnj = uf::vector::cross(rA_j, n_j);
				pod::Vector3f rbXnj = uf::vector::cross(rB_j, n_j);

				pod::Vector3f Ia_raXnj = a.inverseInertiaTensor * raXnj;
				pod::Vector3f Ib_rbXnj = b.inverseInertiaTensor * rbXnj;

				pod::Vector3f crossA = uf::vector::cross(Ia_raXnj, rA_i);
				pod::Vector3f crossB = uf::vector::cross(Ib_rbXnj, rB_i);

				float termAngular = uf::vector::dot(n_i, crossA + crossB);

				K(i,j) = termLinear + termAngular;
			}

			K(i,i) += 1e-3f;
		}

		#if 0
		pod::Vector3f relVelLinear = b.velocity - a.velocity;
		for ( auto i = 0; i < N; i++ ) {
			float vRel = uf::vector::dot( relVelLinear, manifold.points[i].normal );

			float penetrationBias = std::max( manifold.points[i].penetration - ::baumgarteCorrectionSlop, 0.0f ) * ( ::baumgarteCorrectionPercent / dt );
			float cDot = vRel + penetrationBias;

			rhs[i] = (cDot < 0.0f) ? -cDot : 0.0f;
			lambda[i] = manifold.points[i].accumulatedNormalImpulse; // warmup
		}
		#endif
		for ( auto i = 0; i < N; i++ ) {
			auto& contact = manifold.points[i];
			// full relative velocity, linear + angular
			pod::Vector3f vA = a.velocity + uf::vector::cross( a.angularVelocity, contact.point - pA );
			pod::Vector3f vB = b.velocity + uf::vector::cross( b.angularVelocity, contact.point - pB );
			float vRel = uf::vector::dot((vB - vA), contact.normal);

			// penetration bias with clamp
			float penetrationBias = std::max(contact.penetration - ::baumgarteCorrectionSlop, 0.0f) * (::baumgarteCorrectionPercent / dt);
			penetrationBias = std::min(penetrationBias, 2.0f / dt); // clamp

			float cDot = vRel + penetrationBias;

			rhs[i]	= (cDot < 0.0f) ? -cDot : 0.0f; // RHS is magnitude of correction needed
			lambda[i] = contact.accumulatedNormalImpulse;
		}

		residual = rhs - uf::matrix::multiply( K, lambda );
		pod::Matrix<T,N> Kinv = uf::matrix::invert( K );
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

			::applyImpulseTo( a, b, rA, rB, manifold.points[i].normal * dLambda[i] );
		//	::positionCorrection( a, b, manifold.points[i] );
		}
	}

	void block2x2Solver( pod::RigidBody& a, pod::RigidBody& b, pod::Manifold& manifold, float dt ) {
		return ::blockNxNSolver<2>( a, b, manifold, dt );
	}
	void block3x3Solver( pod::RigidBody& a, pod::RigidBody& b, pod::Manifold& manifold, float dt ) {
		return ::blockNxNSolver<3>( a, b, manifold, dt );
	}
	void block4x4Solver( pod::RigidBody& a, pod::RigidBody& b, pod::Manifold& manifold, float dt ) {
		return ::blockNxNSolver<4>( a, b, manifold, dt );
	}

	void blockPGSSolver( pod::RigidBody& a, pod::RigidBody& b, pod::Manifold& manifold, float dt ) {
		const int count = std::min( (int) manifold.points.size(), 4 );
		// precompute inv mass
		float invMassA = ( a.isStatic ? 0.0f : a.inverseMass );
		float invMassB = ( b.isStatic ? 0.0f : b.inverseMass );

		// precompute Jacobians for each contact
		struct ContactCache {
			pod::Vector3f normal;
			pod::Vector3f rA, rB;
			float bias;
			float effectiveMass;
		} cache[4];

		for ( auto i = 0; i < count; i++ ) {
			auto& c = manifold.points[i];
			cache[i].normal = c.normal;
			cache[i].rA = c.point - ::getPosition( a, true );
			cache[i].rB = c.point - ::getPosition( b, true );

			// bias = restitution + Baumgarte correction
			float vRel = uf::vector::dot(
				(b.velocity + uf::vector::cross(b.angularVelocity, cache[i].rB)) -
				(a.velocity + uf::vector::cross(a.angularVelocity, cache[i].rA)),
				cache[i].normal
			);

			float e = std::min( a.material.restitution, b.material.restitution );
			float penetrationBias = std::max(c.penetration - ::baumgarteCorrectionSlop, 0.0f) * (::baumgarteCorrectionPercent / dt);

			cache[i].bias = (vRel < -1.0f ? -e * vRel : 0.0f) + penetrationBias;

			// effective mass = 1 / (J M^-1 J^T)
			pod::Vector3f rnA = uf::vector::cross( cache[i].rA, cache[i].normal );
			pod::Vector3f rnB = uf::vector::cross( cache[i].rB, cache[i].normal );

			pod::Vector3f Ia_rnA = rnA * a.inverseInertiaTensor; // diag mult
			pod::Vector3f Ib_rnB = rnB * b.inverseInertiaTensor;

			float Knormal = invMassA + invMassB + uf::vector::dot(uf::vector::cross(Ia_rnA, cache[i].rA) + uf::vector::cross(Ib_rnB, cache[i].rB), cache[i].normal);

			cache[i].effectiveMass = (Knormal > 0.0f) ? 1.0f / Knormal : 0.0f;
		}

		// initialize lambdas (warm start)
		pod::Vector4f lambda = {};
		for ( auto i = 0; i < count; i++ ) {
			lambda[i] = manifold.points[i].accumulatedNormalImpulse;
		}

		// iterative PGS loop
		for ( auto iter = 0; iter < ::solverIterations; iter++ ) {
			for (int i = 0; i < count; i++) {
				auto& c = manifold.points[i];
				auto& cc = cache[i];

				// relative velocity along normal
				pod::Vector3f dv = (b.velocity + uf::vector::cross(b.angularVelocity, cc.rB)) - (a.velocity + uf::vector::cross(a.angularVelocity, cc.rA));
				float vn = uf::vector::dot(dv, cc.normal);

				// compute delta impulse
				float delta = cc.effectiveMass * (-(vn + cc.bias));

				// accumulate and clamp
				float newLambda = std::max( lambda[i] + delta, 0.0f );
				delta = newLambda - lambda[i];
				lambda[i] = newLambda;

				// apply impulse
				::applyImpulseTo( a, b, cc.rA, cc.rB, cc.normal * delta );

				// position correction
				::positionCorrection( a, b, c );
			}
		}

		// store lambdas back
		for ( auto i = 0; i < count; i++ ) {
			manifold.points[i].accumulatedNormalImpulse = lambda[i];
			// ::positionCorrection( a, b, manifold.points[i] );
		}
	}

	void resolveManifold( pod::RigidBody& a, pod::RigidBody& b, pod::Manifold& manifold, float dt ) {
		if ( ::blockContactSolver ) {
			if ( manifold.points.size() == 2 ) return ::block2x2Solver( a, b, manifold, dt );
			if ( manifold.points.size() == 3 ) return ::block3x3Solver( a, b, manifold, dt );
			if ( manifold.points.size() == 4 ) return ::block4x4Solver( a, b, manifold, dt );
		}
		if ( ::psgContactSolver )  return ::blockPGSSolver( a, b, manifold, dt );
		for ( auto& contact : manifold.points ) ::iterativeImpulseSolver( a, b, contact, dt );
	}

	void solveContacts( uf::stl::vector<pod::Manifold>& manifolds, float dt ) {
		if ( ::warmupSolver ) for ( auto& manifold : manifolds ) ::warmupManifold( *manifold.a, *manifold.b, manifold, dt );
		for ( auto i = 0; i < ::solverIterations; ++i ) for ( auto& manifold : manifolds ) ::resolveManifold( *manifold.a, *manifold.b, manifold, dt );
	}
}