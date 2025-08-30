namespace {
	float computeEffectiveMass( pod::RigidBody& a, pod::RigidBody& b, const pod::Vector3f& rA, const pod::Vector3f& rB, const pod::Vector3f& n ) {
		float inverseMass = 0.0f;
		if ( !a.isStatic ) inverseMass += a.inverseMass;
		if ( !b.isStatic ) inverseMass += b.inverseMass;

		float angularTermA = 0.0f;
		float angularTermB = 0.0f;

		if ( !a.isStatic ) {
			pod::Vector3f crossA = uf::vector::cross(rA, n);
			angularTermA = uf::vector::dot(uf::vector::cross(crossA * a.inverseInertiaTensor, rA), n);
		}
		if ( !b.isStatic ) {
			pod::Vector3f crossB = uf::vector::cross(rB, n);
			angularTermB = uf::vector::dot(uf::vector::cross(crossB * b.inverseInertiaTensor, rB), n);
		}

		float result = inverseMass + angularTermA + angularTermB;
		if (result < EPS(1e-8f)) result = 1.0f; // prevent divide by zero
		return result;
	}

	void applyImpulseTo( pod::RigidBody& a, pod::RigidBody& b, const pod::Vector3f& rA, const pod::Vector3f& rB, const pod::Vector3f& impulse ) {
		if ( !a.isStatic ) {
			a.velocity -= impulse * a.inverseMass;
			a.angularVelocity -= (uf::vector::cross(rA, impulse)) * a.inverseInertiaTensor;
		}
		if ( !b.isStatic ) {
			b.velocity += impulse * b.inverseMass;
			b.angularVelocity += (uf::vector::cross(rB, impulse)) * b.inverseInertiaTensor;
		}
	}

	void applyRollingResistance( pod::RigidBody& body, float dt ) {
		if ( body.isStatic ) return;

		float rollingFriction = 0.02f; // to-do: derive from material
		float angularSpeed = uf::vector::magnitude( body.angularVelocity );
		if ( angularSpeed < EPS(1.0e-6f) ) return;

		body.angularVelocity += body.angularVelocity * body.mass * -rollingFriction * dt;
		// body.angularVelocity *= -rollingFriction * dt;
	}

	void bindManifold( pod::RigidBody& a, pod::RigidBody& b, pod::Manifold& manifold, float dt = 0 ) {
		manifold.a = &a;
		manifold.b = &b;
		manifold.dt = dt;
		manifold.points.clear();
	}

	bool generateContactsGjk( pod::RigidBody& a, pod::RigidBody& b, pod::Manifold& manifold, float dt ) {
		::bindManifold( a, b, manifold, dt );

		pod::Simplex simplex;

		if ( !::gjk(a,b,simplex) ) return false;

		auto result = ::epa( a, b, simplex );
		
		manifold.points.clear();
		manifold.points.emplace_back(result);
		return true;
	}

	bool generateContacts( pod::RigidBody& a, pod::RigidBody& b, pod::Manifold& manifold, float dt ) {
		if ( ::useGjk ) return generateContactsGjk( a, b, manifold, dt );
		::bindManifold( a, b, manifold, dt );

	#define CHECK_CONTACT( A, B, fun )\
		if ( a.collider.type == pod::ShapeType::A && b.collider.type == pod::ShapeType::B ) return fun( a, b, manifold );

		CHECK_CONTACT( AABB, AABB, aabbAabb );
		CHECK_CONTACT( AABB, SPHERE, aabbSphere );
		CHECK_CONTACT( AABB, PLANE, aabbPlane );
		CHECK_CONTACT( AABB, CAPSULE, aabbCapsule );
		CHECK_CONTACT( AABB, MESH, aabbMesh );

		CHECK_CONTACT( SPHERE, AABB, sphereAabb );
		CHECK_CONTACT( SPHERE, SPHERE, sphereSphere );
		CHECK_CONTACT( SPHERE, PLANE, spherePlane );
		CHECK_CONTACT( SPHERE, CAPSULE, sphereCapsule );
		CHECK_CONTACT( SPHERE, MESH, sphereMesh );

		CHECK_CONTACT( PLANE, AABB, planeAabb );
		CHECK_CONTACT( PLANE, SPHERE, planeSphere );
		CHECK_CONTACT( PLANE, PLANE, planePlane );
		CHECK_CONTACT( PLANE, CAPSULE, planeCapsule );
		CHECK_CONTACT( PLANE, MESH, planeMesh );

		CHECK_CONTACT( CAPSULE, AABB, capsuleAabb );
		CHECK_CONTACT( CAPSULE, SPHERE, capsuleSphere );
		CHECK_CONTACT( CAPSULE, PLANE, capsulePlane );
		CHECK_CONTACT( CAPSULE, CAPSULE, capsuleCapsule );
		CHECK_CONTACT( CAPSULE, MESH, capsuleMesh );

		CHECK_CONTACT( MESH, AABB, meshAabb );
		CHECK_CONTACT( MESH, SPHERE, meshSphere );
		CHECK_CONTACT( MESH, PLANE, meshPlane );
		CHECK_CONTACT( MESH, CAPSULE, meshCapsule );
		CHECK_CONTACT( MESH, MESH, meshMesh );
		
		return false;
	}

	bool similarContact( const pod::Contact& a, const pod::Contact& b, float eps = 1.0e-3f, float distSq = 0.95f ) {
		return uf::vector::distanceSquared(a.point, b.point) < eps && uf::vector::dot(a.normal, b.normal) > distSq;
	}

	void reduceContacts( pod::Manifold& manifold ) {
		if ( manifold.points.size() <= 4 ) return;

		uf::stl::vector<pod::Contact> reduced;
		for ( auto& c : manifold.points ) {
			bool merged = false;
			for ( auto& r : reduced ) {
				if ( !::similarContact( c, r ) ) continue;
				// merge, pick deeper penetration
				if (c.penetration > r.penetration) r = c;
				merged = true;
				break;
			}
			if ( !merged ) reduced.emplace_back(c);
		}

		// keep only deepest + farthest up to 4
		std::sort(reduced.begin(), reduced.end(), [](auto& a, auto& b){ return a.penetration > b.penetration; });
		if (reduced.size() > 4) reduced.resize(4);

		manifold.points = reduced;
	}

	void retrieveContacts( pod::Manifold& current, const pod::Manifold& previous, float decay = 0.35f ) {
		for ( auto& c : current.points ) {
			for ( auto& p : previous.points ) {
				if ( !::similarContact( c, p ) ) continue;
				c.accumulatedNormalImpulse = p.accumulatedNormalImpulse * decay;
				c.accumulatedTangentImpulse = p.accumulatedTangentImpulse * decay;
				c.lifetime = p.lifetime + 1;
				break;
			}
		}
	}

	void warmupContacts( pod::RigidBody& a, pod::RigidBody& b, const pod::Contact& c, float dt ) {
		if ( !c.lifetime ) return; // too new

		// build relative offsets
		pod::Vector3f rA = c.point - a.transform->position;
		pod::Vector3f rB = c.point - b.transform->position;

		// normal impulse
		pod::Vector3f Pn = c.normal * c.accumulatedNormalImpulse;
		::applyImpulseTo( a, b, rA, rB, Pn );

		// tangent basis
		pod::Vector3f Pt = c.tangent * c.accumulatedTangentImpulse;
		::applyImpulseTo( a, b, rA, rB, Pt );

		UF_MSG_DEBUG("Warming, Pn={}, Pt={}, lifetime={}", uf::vector::toString(Pn), uf::vector::toString(Pt), c.lifetime );
	}

	// baumgarte position correction
	void positionCorrection( pod::RigidBody& a, pod::RigidBody& b, const pod::Contact& contact ) {
		float correctionMagnitude = std::max(contact.penetration - ::baumgarteCorrectionSlop, 0.0f) / (a.inverseMass + b.inverseMass) * ::baumgarteCorrectionPercent;
		pod::Vector3f correction = contact.normal * correctionMagnitude;

		if ( !a.isStatic ) a.transform->position -= correction * a.inverseMass;
		if ( !b.isStatic ) b.transform->position += correction * b.inverseMass;
	}

	void resolveContact( pod::RigidBody& a, pod::RigidBody& b, pod::Contact& contact, float dt ) {
		// relative positions from centers to contact point
		pod::Vector3f rA = contact.point - a.transform->position;
		pod::Vector3f rB = contact.point - b.transform->position;

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

		pod::Vector3f normalImpulse = contact.normal * jn;
		::applyImpulseTo(a, b, rA, rB, normalImpulse);

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

			pod::Vector3f frictionImpulse = tangent * jt;
			::applyImpulseTo(a, b, rA, rB, frictionImpulse);
		}

		::positionCorrection(a, b, contact);
	}

	void solveContacts( uf::stl::vector<pod::Manifold>& manifolds, float dt ) {
		if ( ::warmupSolver ) {
			for ( auto& m : manifolds ) {
				for ( auto& c : m.points ) {
					::warmupContacts(*m.a, *m.b, c, dt);
				}
			}
		}

		for ( auto i = 0; i < ::solverIterations; ++i ) {
			for ( auto& m : manifolds ) {
			#if 0
				if ( ::blockContactSolver ) {
					//::solveManifoldBlockN( m, dt );
					::solveManifoldBlock2x2( m, dt );
				} else {
					for ( auto& c : m.points ) {
						::resolveContact(*m.a, *m.b, c, dt);
					}
				}
			#endif
				for ( auto& c : m.points ) ::resolveContact(*m.a, *m.b, c, dt);
			}
		}
	}

	void integrate( pod::RigidBody& body, float dt ) {
		// only integrate dynamic bodies
		if ( body.isStatic || body.mass == 0 ) return;

		auto& world = *body.world;

		// linear integration
		pod::Vector3f acceleration = body.forceAccumulator * body.inverseMass;
		acceleration += world.gravity; // apply gravity
		body.velocity += acceleration * dt;

		auto previous = body.transform->position;
		body.transform->position += body.velocity * dt;

		// angular integration
		body.angularVelocity += body.torqueAccumulator * body.inverseInertiaTensor * dt;

		// update orientation
		if ( uf::vector::magnitude( body.angularVelocity ) > EPS(1.0e-8f) ) {
			pod::Quaternion<> dq = uf::quaternion::axisAngle(uf::vector::normalize(body.angularVelocity), uf::vector::magnitude(body.angularVelocity)*dt);
			uf::transform::rotate( *body.transform, dq );
		}

		// reset accumulators
		body.forceAccumulator = {};
		body.torqueAccumulator = {};

		// update AABB
		body.bounds = ::computeAABB( body );

		// apply rolling resistance
		::applyRollingResistance(body, dt);
	}
}