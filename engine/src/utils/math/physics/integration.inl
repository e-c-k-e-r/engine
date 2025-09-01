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
		//	if ( uf::vector::magnitude( impulse ) > 1.0e-4 ) UF_MSG_DEBUG("aV delta={}", uf::vector::toString(impulse * a.inverseMass));
		}
		if ( !b.isStatic ) {
			b.velocity += impulse * b.inverseMass;
			b.angularVelocity += (uf::vector::cross(rB, impulse)) * b.inverseInertiaTensor;
		//	if ( uf::vector::magnitude( impulse ) > 1.0e-4 ) UF_MSG_DEBUG("bV delta={}", uf::vector::toString(impulse * b.inverseMass));
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

	bool similarContact( const pod::Contact& a, const pod::Contact& b, float distSq = 1.0e-2f, float norm = 0.9f ) {
		return uf::vector::distanceSquared(a.point, b.point) < distSq && uf::vector::dot(a.normal, b.normal) > norm;
	}

	void reduceContacts( pod::Manifold& manifold ) {
		if ( manifold.points.size() <= 4 ) return;

		uf::stl::vector<pod::Contact> result;
		for ( auto& c : manifold.points ) {
			// prune invalid contacts
			if ( !uf::vector::isValid( c.point ) ) continue;

			bool merged = false;
			for ( auto& r : result ) {
				if ( !::similarContact( c, r ) ) continue;
				// merge, pick deeper penetration
				if ( c.penetration > r.penetration ) r = c;
				merged = true;
				break;
			}
			if ( !merged ) result.emplace_back(c);
		}
		
		// UF_MSG_DEBUG("Reduced {} => {} contacts", manifold.points.size(), result.size());

		// keep only deepest + farthest up to 4
		std::sort(result.begin(), result.end(), [](auto& a, auto& b){ return a.penetration > b.penetration; });
		if ( result.size() > 4 ) result.resize(4);

		manifold.points = result;
	}

	void mergeContacts( pod::Manifold& manifold ) {
		uf::stl::vector<pod::Contact> result;
		
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
		
	//	UF_MSG_DEBUG("Merged {} => {} contacts", manifold.points.size(), result.size());

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

	void warmupContacts( pod::RigidBody& a, pod::RigidBody& b, const pod::Contact& c, float dt ) {
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
	void warmupManifold( pod::RigidBody& a, pod::RigidBody& b, const pod::Manifold& manifold, float dt ) {
		for ( auto& contact : manifold.points ) {
			::warmupContacts( a, b, contact, dt );
		}
	}

	// baumgarte position correction
	void positionCorrection( pod::RigidBody& a, pod::RigidBody& b, const pod::Contact& contact ) {
		if ( ::baumgarteCorrectionPercent <= 0 ) return;

		float correctionMagnitude = std::max(contact.penetration - ::baumgarteCorrectionSlop, 0.0f) / (a.inverseMass + b.inverseMass) * ::baumgarteCorrectionPercent;
		pod::Vector3f correction = contact.normal * correctionMagnitude;

		if ( !a.isStatic ) a.transform/*.reference*/->position -= correction * a.inverseMass;
		if ( !b.isStatic ) b.transform/*.reference*/->position += correction * b.inverseMass;
	}

	void integrate( pod::RigidBody& body, float dt ) {
		// only integrate dynamic bodies
		if ( body.isStatic || body.mass == 0 ) return;

		auto& world = *body.world;

		// linear integration
		pod::Vector3f acceleration = body.forceAccumulator * body.inverseMass;
		acceleration += world.gravity; // apply gravity
		body.velocity += acceleration * dt;
		body.transform/*.reference*/->position += body.velocity * dt;

		// angular integration
		body.angularVelocity += body.torqueAccumulator * body.inverseInertiaTensor * dt;

		// update orientation
		if ( uf::vector::magnitude( body.angularVelocity ) > EPS(1.0e-8f) ) {
			pod::Quaternion<> dq = uf::quaternion::axisAngle(uf::vector::normalize(body.angularVelocity), uf::vector::magnitude(body.angularVelocity)*dt);
			uf::transform::rotate( *body.transform/*.reference*/, dq );
		}

		// update AABB
		body.bounds = ::computeAABB( body );

		// reset accumulators
		body.forceAccumulator = {};
		body.torqueAccumulator = {};


		// apply rolling resistance
		::applyRollingResistance(body, dt);
	}
}