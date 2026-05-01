namespace {
	void iterativeImpulseSolver( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Contact& contact, float dt ) {
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
		if ( uf::physics::impl::settings.warmupSolver ) {
			float jnOld = contact.accumulatedNormalImpulse;
			float jnNew = std::max(0.0f, jnOld + jn);
			float jnDelta = jnNew - jnOld;
			contact.accumulatedNormalImpulse = jnNew;
			jn = jnDelta;
		}

		::applyImpulseTo(a, b, rA, rB, contact.normal * jn);

		// tangent direction
		pod::Vector3f tangent = rv - contact.normal * uf::vector::dot(rv, contact.normal);
		float tangentMag2 = uf::vector::magnitude(tangent);
		if ( tangentMag2 > EPS2 ) {
			tangent /= std::sqrt( tangentMag2 );

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

			if ( uf::physics::impl::settings.warmupSolver ) {
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
	}
}