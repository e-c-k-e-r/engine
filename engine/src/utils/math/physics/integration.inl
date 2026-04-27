namespace {
	float computeEffectiveMass( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Vector3f& rA, const pod::Vector3f& rB, const pod::Vector3f& n ) {
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
		if (result < EPS) result = 1.0f; // prevent divide by zero
		return result;
	}

	void applyImpulseTo( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Vector3f& rA, const pod::Vector3f& rB, const pod::Vector3f& impulse ) {
		if ( !a.isStatic ) {
			a.velocity -= impulse * a.inverseMass;
			//a.angularVelocity -= (uf::vector::cross(rA, impulse)) * a.inverseInertiaTensor;
			pod::Matrix3f invIa = computeWorldInverseInertia( a );
			a.angularVelocity -= uf::matrix::multiply( invIa, uf::vector::cross(rA, impulse) );
		}
		if ( !b.isStatic ) {
			b.velocity += impulse * b.inverseMass;
			//b.angularVelocity += (uf::vector::cross(rB, impulse)) * b.inverseInertiaTensor;
			pod::Matrix3f invIb = computeWorldInverseInertia( b );
			a.angularVelocity += uf::matrix::multiply( invIb, uf::vector::cross(rB, impulse) );
		}
	}

	void applyRollingResistance( pod::PhysicsBody& body, float dt ) {
		if ( body.isStatic ) return;

		float rollingFriction = 0.02f; // to-do: derive from material
		float angularSpeed2 = uf::vector::magnitude( body.angularVelocity );
		if ( angularSpeed2 < EPS2 ) return;

		body.angularVelocity += body.angularVelocity * body.mass * -rollingFriction * dt;
		// body.angularVelocity *= -rollingFriction * dt;
	}

	void bindManifold( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt = 0 ) {
		manifold.a = &a;
		manifold.b = &b;
		manifold.dt = dt;
		manifold.points.clear();
	}

	bool generateContactsGjk( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt ) {
		::bindManifold( a, b, manifold, dt );

		pod::Simplex simplex;

		if ( !::gjk(a,b,simplex) ) return false;

		auto result = ::epa( a, b, simplex );
		
		manifold.points.clear();
		manifold.points.emplace_back(result);
		return true;
	}

	bool generateContacts( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt ) {
		if ( uf::physics::impl::settings.useGjk ) return generateContactsGjk( a, b, manifold, dt );
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

	bool similarContact( const pod::Contact& a, const pod::Contact& b, float distSq = 1.0e-2f, float norm = 0.9f ) {
		return uf::vector::distanceSquared(a.point, b.point) < distSq && uf::vector::dot(a.normal, b.normal) > norm;
	}

	void reduceContacts( pod::Manifold& manifold ) {
		if ( manifold.points.size() <= 4 ) return;

		static thread_local uf::stl::vector<pod::Contact> result;
		result.clear();
		result.reserve(4);

		for ( auto& c : manifold.points ) {
			if ( !uf::vector::isValid(c.point) ) continue;

			bool merged = false;
			for ( auto& r : result ) {
				if ( !::similarContact(c, r) ) continue;
				if ( c.penetration > r.penetration ) r = c;
				merged = true;
				break;
			}
			if ( !merged ) {
				if ( result.size() < 4 ) {
					result.emplace_back(c);
				} else {
					// Replace weakest if this one is stronger
					auto weakest = 0;
					for ( auto i = 1; i < 4; i++ ) {
						if ( result[i].penetration < result[weakest].penetration ) weakest = i;
					}
					if ( c.penetration > result[weakest].penetration ) result[weakest] = c;
				}
			}
		}

		manifold.points = result;
	}

	void mergeContacts( pod::Manifold& manifold ) {
		static thread_local uf::stl::vector<pod::Contact> result;
		result.clear();
		result.reserve(4);

		for ( auto& c : manifold.points ) {
			bool merged = false;
			for ( auto& r : result ) {
				if ( !::similarContact( c, r ) ) continue;
				// merge: average position + normal, keep max penetration
				r.point  = ( r.point + c.point ) * 0.5f;
				r.normal = uf::vector::normalize( r.normal + c.normal );
				r.penetration = std::max( r.penetration, c.penetration );
				merged = true;
				break;
			}
			if ( !merged ) result.emplace_back( c );
		}
		
		manifold.points = result;
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

	void prepareManifoldCache( uf::stl::unordered_map<size_t, pod::Manifold>& cache, const uf::stl::vector<pod::Island>& islands, const uf::stl::vector<pod::PhysicsBody*>& bodies ) {
		for ( const auto& island : islands ) {
			for ( const auto& pair : island.pairs ) {
				auto& a = *bodies[pair.first];
				auto& b = *bodies[pair.second];

				cache[ ::makePairKey( a, b ) ];
			}
		}
	}

	void updateManifoldCache( const uf::stl::vector<pod::Manifold>& manifolds, uf::stl::unordered_map<size_t, pod::Manifold>& cache ) {
		for ( const auto& m : manifolds ) {
			auto it = cache.find( ::makePairKey( *m.a, *m.b ) );
			if ( it == cache.end() ) continue; // assert
			it->second = m;
		}
	}

	void pruneManifoldCache( uf::stl::unordered_map<size_t, pod::Manifold>& cache ) {
		for ( auto itCache = cache.begin(); itCache != cache.end(); ) {
			auto& manifold = itCache->second;

			// prune points that are too old
			for ( auto it = manifold.points.begin(); it != manifold.points.end(); ) {
				if ( it->lifetime > uf::physics::impl::settings.manifoldCacheLifetime ) it = manifold.points.erase(it);
				else ++it;
			}

			// empty manifold, kill it
			if ( manifold.points.empty() ) itCache = cache.erase(itCache);
			else ++itCache;
		}
	}

	void warmupContacts( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Contact& c, float dt ) {
		if ( !c.lifetime ) return; // too new

		// build relative offsets
		pod::Vector3f rA = c.point - ::getPosition( a );
		pod::Vector3f rB = c.point - ::getPosition( b );

		// normal impulse
		pod::Vector3f Pn = c.normal * c.accumulatedNormalImpulse;
		::applyImpulseTo( a, b, rA, rB, Pn );

		// tangent basis
		pod::Vector3f Pt = c.tangent * c.accumulatedTangentImpulse;
		::applyImpulseTo( a, b, rA, rB, Pt );

	//	UF_MSG_DEBUG("Warming, Pn={}, Pt={}, lifetime={}", uf::vector::toString(Pn), uf::vector::toString(Pt), c.lifetime );
	}
	void warmupManifold( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Manifold& manifold, float dt ) {
		for ( auto& contact : manifold.points ) {
			::warmupContacts( a, b, contact, dt );
		}
	}
	
	// snap velocity for grounded bodies
	void snapVelocity( pod::PhysicsBody& body, float dt, float threshold = 0.01f ) {
		if ( !body.activity.grounded || !body.activity.awake ) return;

		float thresholdSq = threshold * threshold;
		// snap velocity if body is grounded and nearly still
		float linSpeedSq = uf::vector::magnitude( body.velocity );
		float angSpeedSq = uf::vector::magnitude( body.angularVelocity );

		// cancel out vertical component
		if ( fabs(body.velocity.y) < thresholdSq ) body.velocity.y = 0.0f;
		// cancel out velocity entirely
		if ( linSpeedSq < thresholdSq ) body.velocity = {};
		// cancel out rotational velocity entirely
		if ( angSpeedSq < thresholdSq ) body.angularVelocity = {};
	}

	// baumgarte position correction
	void positionCorrection( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Contact& contact ) {
		if ( uf::physics::impl::settings.baumgarteCorrectionPercent <= 0 ) return;
		if ( a.isStatic && b.isStatic ) return;

		// penetration depth beyond slop
		float penetration = std::max( contact.penetration - uf::physics::impl::settings.baumgarteCorrectionSlop, 0.0f );
		if ( penetration <= 0.0f ) return;

		// compute correction magnitude
		float invMassA = ( a.isStatic ? 0.0f : a.inverseMass );
		float invMassB = ( b.isStatic ? 0.0f : b.inverseMass );
		float totalInvMass = invMassA + invMassB;
		if ( totalInvMass <= EPS ) return;

		// apply correction vector
		pod::Vector3f correction = contact.normal * (penetration / totalInvMass) * uf::physics::impl::settings.baumgarteCorrectionPercent;
		
		if ( !a.isStatic ) a.transform->position -= correction * invMassA;
		if ( !b.isStatic ) b.transform->position += correction * invMassB;
	}


	void integrate( pod::PhysicsBody& body, float dt ) {
		// only integrate awake and dynamic bodies
		if ( !body.activity.awake || body.isStatic || body.mass == 0 ) return;

		auto& world = *body.world;

		// linear integration
		pod::Vector3f acceleration = body.forceAccumulator * body.inverseMass;
		acceleration += uf::physics::impl::getGravity( body ); // apply gravity
		body.velocity += acceleration * dt;

		// angular integration
		//body.angularVelocity += body.torqueAccumulator * body.inverseInertiaTensor * dt;
		{
			pod::Matrix3f R = uf::quaternion::matrix3(body.transform->orientation);
			pod::Vector3f localTorque = uf::matrix::multiply( uf::matrix::transpose(R), body.torqueAccumulator );
			pod::Vector3f localAngAccel = localTorque * body.inverseInertiaTensor; // element-wise
			body.angularVelocity += uf::matrix::multiply( R, localAngAccel ) * dt;
		}

		// update position
		body.transform/*.reference*/->position += body.velocity * dt;

		// update orientation
		float angularSpeed2 = uf::vector::magnitude( body.angularVelocity );
		if ( angularSpeed2 > EPS2 ) {
			float angularSpeed = std::sqrt( angularSpeed2 );
			pod::Quaternion<> dq = uf::quaternion::axisAngle( body.angularVelocity / angularSpeed, angularSpeed * dt);
			uf::transform::rotate( *body.transform/*.reference*/, dq );
		}

		// reset accumulators
		body.forceAccumulator = {};
		body.torqueAccumulator = {};

		// apply rolling resistance
		::applyRollingResistance( body, dt );

		// update activity state
		::updateActivity( body, dt );
	}
}