#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/integration.h>
#include <uf/utils/math/physics/narrowphase.h>

float impl::computeEffectiveMass( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Vector3f& rA, const pod::Vector3f& rB, const pod::Vector3f& n ) {
	float inverseMass = 0.0f;
	if ( !a.isStatic ) inverseMass += a.inverseMass;
	if ( !b.isStatic ) inverseMass += b.inverseMass;

	float angularTermA = 0.0f;
	float angularTermB = 0.0f;

	if ( !a.isStatic ) {
		auto invIa = impl::computeWorldInverseInertia(a);
		auto crossA = uf::vector::cross(rA, n);
		auto I_crossA = uf::matrix::multiply(invIa, crossA);
		angularTermA = uf::vector::dot(uf::vector::cross(I_crossA, rA), n);
	}
	if ( !b.isStatic ) {
		auto invIb = impl::computeWorldInverseInertia(b);
		auto crossB = uf::vector::cross(rB, n);
		auto I_crossB = uf::matrix::multiply(invIb, crossB);
		angularTermB = uf::vector::dot(uf::vector::cross(I_crossB, rB), n);
	}

	float result = inverseMass + angularTermA + angularTermB;
	if (result < EPS) result = 1.0f; // prevent divide by zero
	return result;
}

void impl::applyImpulseTo( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Vector3f& rA, const pod::Vector3f& rB, const pod::Vector3f& impulse ) {
	if ( !a.isStatic ) {
		a.velocity -= impulse * a.inverseMass;
		//a.angularVelocity -= (uf::vector::cross(rA, impulse)) * a.inverseInertiaTensor;
		pod::Matrix3f invIa = impl::computeWorldInverseInertia( a );
		a.angularVelocity -= uf::matrix::multiply( invIa, uf::vector::cross(rA, impulse) );
	}
	if ( !b.isStatic ) {
		b.velocity += impulse * b.inverseMass;
		//b.angularVelocity += (uf::vector::cross(rB, impulse)) * b.inverseInertiaTensor;
		pod::Matrix3f invIb = impl::computeWorldInverseInertia( b );
		b.angularVelocity += uf::matrix::multiply( invIb, uf::vector::cross(rB, impulse) );
	}
}

void impl::applyRollingResistance( pod::PhysicsBody& body, float dt ) {
	if ( body.isStatic ) return;

	float rollingFriction = 0.02f; // to-do: derive from material
	float angularSpeed2 = uf::vector::magnitude( body.angularVelocity );
	if ( angularSpeed2 < EPS2 ) return;

	//body.angularVelocity += body.angularVelocity * body.mass * -rollingFriction * dt;
	body.angularVelocity *= std::max(0.0f, 1.0f - rollingFriction * dt);
}

void impl::bindManifold( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt ) {
	manifold.a = &a;
	manifold.b = &b;
	manifold.dt = dt;
	manifold.points.clear();
}

bool impl::generateContactsGjk( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt ) {
	impl::bindManifold( a, b, manifold, dt );

	pod::Simplex simplex;

	if ( !impl::gjk(a,b,simplex) ) return false;
	auto result = impl::epa( a, b, simplex );
	if ( !impl::generateClippingManifold( a, b, result, manifold ) ) return false;

	return true;
}

bool impl::generateContacts( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt ) {
	bool useGjk = uf::physics::settings.useGjk;
	if ( a.collider.type == pod::ShapeType::MESH || b.collider.type == pod::ShapeType::MESH ) useGjk = false;
	//if ( a.collider.type == pod::ShapeType::PLANE || b.collider.type == pod::ShapeType::PLANE ) useGjk = false;
	if ( useGjk ) return generateContactsGjk( a, b, manifold, dt );
	impl::bindManifold( a, b, manifold, dt );

#define CHECK_CONTACT( A, B, fun )\
	if ( a.collider.type == pod::ShapeType::A && b.collider.type == pod::ShapeType::B ) return fun( a, b, manifold );

	CHECK_CONTACT( AABB, AABB, impl::aabbAabb );
	CHECK_CONTACT( AABB, SPHERE, impl::aabbSphere );
	CHECK_CONTACT( AABB, PLANE, impl::aabbPlane );
	CHECK_CONTACT( AABB, CAPSULE, impl::aabbCapsule );
	CHECK_CONTACT( AABB, MESH, impl::aabbMesh );
	CHECK_CONTACT( AABB, CONVEX_HULL, impl::aabbHull );

	CHECK_CONTACT( SPHERE, AABB, impl::sphereAabb );
	CHECK_CONTACT( SPHERE, SPHERE, impl::sphereSphere );
	CHECK_CONTACT( SPHERE, PLANE, impl::spherePlane );
	CHECK_CONTACT( SPHERE, CAPSULE, impl::sphereCapsule );
	CHECK_CONTACT( SPHERE, MESH, impl::sphereMesh );
	CHECK_CONTACT( SPHERE, CONVEX_HULL, impl::sphereHull );

	CHECK_CONTACT( PLANE, AABB, impl::planeAabb );
	CHECK_CONTACT( PLANE, SPHERE, impl::planeSphere );
	CHECK_CONTACT( PLANE, PLANE, impl::planePlane );
	CHECK_CONTACT( PLANE, CAPSULE, impl::planeCapsule );
	CHECK_CONTACT( PLANE, MESH, impl::planeMesh );
	CHECK_CONTACT( PLANE, CONVEX_HULL, impl::planeHull );

	CHECK_CONTACT( CAPSULE, AABB, impl::capsuleAabb );
	CHECK_CONTACT( CAPSULE, SPHERE, impl::capsuleSphere );
	CHECK_CONTACT( CAPSULE, PLANE, impl::capsulePlane );
	CHECK_CONTACT( CAPSULE, CAPSULE, impl::capsuleCapsule );
	CHECK_CONTACT( CAPSULE, MESH, impl::capsuleMesh );
	CHECK_CONTACT( CAPSULE, CONVEX_HULL, impl::capsuleHull );

	CHECK_CONTACT( MESH, AABB, impl::meshAabb );
	CHECK_CONTACT( MESH, SPHERE, impl::meshSphere );
	CHECK_CONTACT( MESH, PLANE, impl::meshPlane );
	CHECK_CONTACT( MESH, CAPSULE, impl::meshCapsule );
	CHECK_CONTACT( MESH, MESH, impl::meshMesh );
	CHECK_CONTACT( MESH, CONVEX_HULL, impl::meshHull );

	CHECK_CONTACT( CONVEX_HULL, AABB, impl::hullAabb );
	CHECK_CONTACT( CONVEX_HULL, SPHERE, impl::hullSphere );
	CHECK_CONTACT( CONVEX_HULL, PLANE, impl::hullPlane );
	CHECK_CONTACT( CONVEX_HULL, CAPSULE, impl::hullCapsule );
	CHECK_CONTACT( CONVEX_HULL, MESH, impl::hullMesh );
	CHECK_CONTACT( CONVEX_HULL, CONVEX_HULL, impl::hullHull );

	UF_EXCEPTION("unregistered contact: {} vs {}", (int) a.collider.type, (int) b.collider.type );
	
	return false;
}

void impl::computeLocalContacts( pod::Manifold& manifold ) {
	if ( manifold.points.empty() ) return;

	auto& a = *manifold.a;
	auto& b = *manifold.b;

	auto tA = impl::getTransform( a );
	auto tB = impl::getTransform( b );

	for ( auto& c : manifold.points ) {
		c.localA = uf::transform::applyInverse( tA, c.point - c.normal * (c.penetration * 0.5f) );
		c.localB = uf::transform::applyInverse( tB, c.point + c.normal * (c.penetration * 0.5f) );
	}
}

bool impl::similarContact( const pod::Contact& a, const pod::Contact& b, float distSqThreshold, float normThreshold ) {
	return uf::vector::distanceSquared(a.point, b.point) < distSqThreshold && uf::vector::dot(a.normal, b.normal) > normThreshold;
}

void impl::reduceContacts( pod::Manifold& manifold ) {
	if ( manifold.points.size() <= 4 ) return;

	STATIC_THREAD_LOCAL(uf::stl::vector<pod::Contact>, result);
	result.reserve(4);

	for ( auto& c : manifold.points ) {
		if ( !uf::vector::isValid(c.point) ) continue;

		bool merged = false;
		for ( auto& r : result ) {
			if ( !impl::similarContact(c, r) ) continue;
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

void impl::mergeContacts( pod::Manifold& manifold ) {
	STATIC_THREAD_LOCAL(uf::stl::vector<pod::Contact>, result);
	result.reserve(4);

	for ( auto& c : manifold.points ) {
		bool merged = false;
		for ( auto& r : result ) {
			if ( !impl::similarContact( c, r ) ) continue;
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

void impl::retrieveContacts( pod::Manifold& current, const pod::Manifold& previous, float distanceThreshold, float separationThreshold, float decay ) {
	auto& a = *current.a;
	auto& b = *current.b;

	auto tA = impl::getTransform( a );
	auto tB = impl::getTransform( b );

	uf::stl::vector<pod::Contact> merged = current.points;

	float distSqThresh = distanceThreshold * distanceThreshold;
	for ( const auto& oldContact : previous.points ) {
		// reproject point according to current transform
		auto worldA = uf::transform::apply( tA, oldContact.localA );
		auto worldB = uf::transform::apply( tB, oldContact.localB );

		auto delta = worldB - worldA;
		auto normal = current.points.empty() ? oldContact.normal : current.points[0].normal;
		float penetration = -uf::vector::dot( delta, normal );
		if ( penetration < -separationThreshold ) continue;

		pod::Vector3f projectedDelta = delta + normal * penetration;
		float tangentialDriftSq = uf::vector::dot( projectedDelta, projectedDelta );
		if ( tangentialDriftSq > distSqThresh ) continue;

		pod::Contact validContact = oldContact;
		validContact.point = (worldA + worldB) * 0.5f;
		validContact.normal = normal;
		validContact.penetration = penetration;
		++validContact.lifetime;

		validContact.accumulatedNormalImpulse *= decay;
		validContact.accumulatedTangentImpulse *= decay;

		bool isDuplicate = false;
		for ( auto& c : current.points ) {
			if ( impl::similarContact( validContact, c ) ) {
				c.accumulatedNormalImpulse = validContact.accumulatedNormalImpulse;
				c.accumulatedTangentImpulse = validContact.accumulatedTangentImpulse;
				c.lifetime = validContact.lifetime;
				isDuplicate = true;
				break;
			}
		}

		if ( !isDuplicate ) merged.emplace_back( validContact );
	}

	current.points = merged;
}

void impl::prepareManifoldCache( uf::stl::unordered_map<size_t, pod::Manifold>& cache, const uf::stl::vector<pod::Island>& islands, const uf::stl::vector<pod::PhysicsBody*>& bodies ) {
	for ( const auto& island : islands ) {
		for ( const auto& pair : island.pairs ) {
			auto& a = *bodies[pair.first];
			auto& b = *bodies[pair.second];

			cache[ impl::makePairKey( a, b ) ];
		}
	}
}

void impl::updateManifoldCache( const uf::stl::vector<pod::Manifold>& manifolds, uf::stl::unordered_map<size_t, pod::Manifold>& cache ) {
	for ( const auto& m : manifolds ) {
		auto it = cache.find( impl::makePairKey( *m.a, *m.b ) );
		if ( it == cache.end() ) continue; // assert
		it->second = m;
	}
}

void impl::pruneManifoldCache( uf::stl::unordered_map<size_t, pod::Manifold>& cache ) {
	for ( auto itCache = cache.begin(); itCache != cache.end(); ) {
		auto& manifold = itCache->second;

		// prune points that are too old
		for ( auto it = manifold.points.begin(); it != manifold.points.end(); ) {
			if ( it->lifetime > uf::physics::settings.manifoldCacheLifetime ) it = manifold.points.erase(it);
			else ++it;
		}

		// empty manifold, kill it
		if ( manifold.points.empty() ) itCache = cache.erase(itCache);
		else ++itCache;
	}
}

void impl::warmupContacts( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Contact& c, float dt ) {
	if ( !c.lifetime ) return; // too new

	// build relative offsets
	pod::Vector3f rA = c.point - impl::getPosition( a );
	pod::Vector3f rB = c.point - impl::getPosition( b );

	// normal impulse
	pod::Vector3f Pn = c.normal * c.accumulatedNormalImpulse;
	impl::applyImpulseTo( a, b, rA, rB, Pn );

	// tangent basis
	pod::Vector3f Pt = c.tangent * c.accumulatedTangentImpulse;
	impl::applyImpulseTo( a, b, rA, rB, Pt );

//	UF_MSG_DEBUG("Warming, Pn={}, Pt={}, lifetime={}", uf::vector::toString(Pn), uf::vector::toString(Pt), c.lifetime );
}
void impl::warmupManifold( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Manifold& manifold, float dt ) {
	for ( auto& contact : manifold.points ) {
		impl::warmupContacts( a, b, contact, dt );
	}
}

// snap velocity for grounded bodies
void impl::snapVelocity( pod::PhysicsBody& body, float dt, float threshold ) {
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
void impl::positionCorrection( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Contact& contact ) {
	if ( uf::physics::settings.baumgarteCorrectionPercent <= 0 ) return;
	if ( a.isStatic && b.isStatic ) return;

	// penetration depth beyond slop
	float penetration = std::max( contact.penetration - uf::physics::settings.baumgarteCorrectionSlop, 0.0f );
	if ( penetration <= 0.0f ) return;

	// compute correction magnitude
	float invMassA = ( a.isStatic ? 0.0f : a.inverseMass );
	float invMassB = ( b.isStatic ? 0.0f : b.inverseMass );
	float totalInvMass = invMassA + invMassB;
	if ( totalInvMass <= EPS ) return;

	// apply correction vector
	pod::Vector3f correction = contact.normal * (penetration / totalInvMass) * uf::physics::settings.baumgarteCorrectionPercent;
	
	if ( !a.isStatic ) a.transform->position -= correction * invMassA;
	if ( !b.isStatic ) b.transform->position += correction * invMassB;
}


void impl::integrate( pod::PhysicsBody& body, float dt ) {
	// only integrate awake and dynamic bodies
	if ( !body.activity.awake || body.isStatic || body.mass == 0 ) return;

	auto& world = *body.world;

	// linear integration
	pod::Vector3f acceleration = body.forceAccumulator * body.inverseMass;
	acceleration += uf::physics::getGravity( body ); // apply gravity
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
	impl::applyRollingResistance( body, dt );

	// update activity state
	impl::updateActivity( body, dt );
}