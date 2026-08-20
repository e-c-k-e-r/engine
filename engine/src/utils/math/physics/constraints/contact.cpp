#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/narrowphase.h>
#include <uf/utils/math/physics/integration.h>
#include <uf/utils/math/physics/solvers.h>
#include <uf/utils/math/physics/constraints/contact.h>

void impl::bindManifold( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt ) {
	manifold.a = &a;
	manifold.b = &b;
	manifold.points.clear();
	manifold.points.reserve(4);
}

bool impl::generateManifoldGjk( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt ) {
	impl::bindManifold( a, b, manifold, dt );

	pod::Simplex simplex;

	if ( !impl::gjk(a,b,simplex) ) return false;
	auto result = impl::epa( a, b, simplex );
	if ( !impl::generateClippingManifold( a, b, result, manifold ) ) return false;

	return true;
}

bool impl::generateManifold( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt ) {
	bool useGjk = uf::physics::settings.useGjk;
	if ( a.collider.type == pod::ShapeType::MESH || b.collider.type == pod::ShapeType::MESH ) useGjk = false;
	//if ( a.collider.type == pod::ShapeType::PLANE || b.collider.type == pod::ShapeType::PLANE ) useGjk = false;
	if ( useGjk ) return generateManifoldGjk( a, b, manifold, dt );
	impl::bindManifold( a, b, manifold, dt );

#define CHECK_CONTACT( A, B, fun )\
	if ( a.collider.type == pod::ShapeType::A && b.collider.type == pod::ShapeType::B ) return fun( a, b, manifold );

	CHECK_CONTACT( AABB, AABB, impl::aabbAabb );
	CHECK_CONTACT( AABB, OBB, impl::aabbObb );
	CHECK_CONTACT( AABB, SPHERE, impl::aabbSphere );
	CHECK_CONTACT( AABB, PLANE, impl::aabbPlane );
	CHECK_CONTACT( AABB, CAPSULE, impl::aabbCapsule );
	CHECK_CONTACT( AABB, MESH, impl::aabbMesh );
	CHECK_CONTACT( AABB, CONVEX_HULL, impl::aabbHull );

	CHECK_CONTACT( OBB, AABB, impl::obbAabb );
	CHECK_CONTACT( OBB, OBB, impl::obbObb );
	CHECK_CONTACT( OBB, SPHERE, impl::obbSphere );
	CHECK_CONTACT( OBB, PLANE, impl::obbPlane );
	CHECK_CONTACT( OBB, CAPSULE, impl::obbCapsule );
	CHECK_CONTACT( OBB, MESH, impl::obbMesh );
	CHECK_CONTACT( OBB, CONVEX_HULL, impl::obbHull );

	CHECK_CONTACT( SPHERE, AABB, impl::sphereAabb );
	CHECK_CONTACT( SPHERE, OBB, impl::sphereObb );
	CHECK_CONTACT( SPHERE, SPHERE, impl::sphereSphere );
	CHECK_CONTACT( SPHERE, PLANE, impl::spherePlane );
	CHECK_CONTACT( SPHERE, CAPSULE, impl::sphereCapsule );
	CHECK_CONTACT( SPHERE, MESH, impl::sphereMesh );
	CHECK_CONTACT( SPHERE, CONVEX_HULL, impl::sphereHull );

	CHECK_CONTACT( PLANE, AABB, impl::planeAabb );
	CHECK_CONTACT( PLANE, OBB, impl::planeObb );
	CHECK_CONTACT( PLANE, SPHERE, impl::planeSphere );
	CHECK_CONTACT( PLANE, PLANE, impl::planePlane );
	CHECK_CONTACT( PLANE, CAPSULE, impl::planeCapsule );
	CHECK_CONTACT( PLANE, MESH, impl::planeMesh );
	CHECK_CONTACT( PLANE, CONVEX_HULL, impl::planeHull );

	CHECK_CONTACT( CAPSULE, AABB, impl::capsuleAabb );
	CHECK_CONTACT( CAPSULE, OBB, impl::capsuleObb );
	CHECK_CONTACT( CAPSULE, SPHERE, impl::capsuleSphere );
	CHECK_CONTACT( CAPSULE, PLANE, impl::capsulePlane );
	CHECK_CONTACT( CAPSULE, CAPSULE, impl::capsuleCapsule );
	CHECK_CONTACT( CAPSULE, MESH, impl::capsuleMesh );
	CHECK_CONTACT( CAPSULE, CONVEX_HULL, impl::capsuleHull );

	CHECK_CONTACT( MESH, AABB, impl::meshAabb );
	CHECK_CONTACT( MESH, OBB, impl::meshObb );
	CHECK_CONTACT( MESH, SPHERE, impl::meshSphere );
	CHECK_CONTACT( MESH, PLANE, impl::meshPlane );
	CHECK_CONTACT( MESH, CAPSULE, impl::meshCapsule );
	CHECK_CONTACT( MESH, MESH, impl::meshMesh );
	CHECK_CONTACT( MESH, CONVEX_HULL, impl::meshHull );

	CHECK_CONTACT( CONVEX_HULL, AABB, impl::hullAabb );
	CHECK_CONTACT( CONVEX_HULL, OBB, impl::hullObb );
	CHECK_CONTACT( CONVEX_HULL, SPHERE, impl::hullSphere );
	CHECK_CONTACT( CONVEX_HULL, PLANE, impl::hullPlane );
	CHECK_CONTACT( CONVEX_HULL, CAPSULE, impl::hullCapsule );
	CHECK_CONTACT( CONVEX_HULL, MESH, impl::hullMesh );
	CHECK_CONTACT( CONVEX_HULL, CONVEX_HULL, impl::hullHull );

	UF_EXCEPTION("unregistered contact: {} vs {}", (int) a.collider.type, (int) b.collider.type );
	
	return false;
}

void impl::computeLocalManifold( pod::Manifold& manifold ) {
	if ( manifold.points.empty() ) return;

	auto& a = *manifold.a;
	auto& b = *manifold.b;

	auto tA = impl::getTransform( a );
	auto tB = impl::getTransform( b );

	for ( auto& c : manifold.points ) {
		c.localA = impl::applyInverse( tA, c.point + c.normal * (c.penetration * 0.5f) );
		c.localB = impl::applyInverse( tB, c.point - c.normal * (c.penetration * 0.5f) );
	}
}

bool impl::similarContact( const pod::Contact& a, const pod::Contact& b, float distSqThreshold, float normThreshold ) {
	return uf::vector::distanceSquared(a.point, b.point) < distSqThreshold && uf::vector::dot(a.normal, b.normal) > normThreshold;
}

void impl::reduceManifold( pod::Manifold& manifold ) {
	if ( manifold.points.size() <= 4 ) return;

	int idx0 = 0, idx1 = 0, idx2 = 0, idx3 = 0;

	// deepest
	float maxPenetration = -FLT_MAX;
	for ( int i = 0; i < manifold.points.size(); ++i ) {
		if ( manifold.points[i].penetration > maxPenetration ) {
			maxPenetration = manifold.points[i].penetration;
			idx0 = i;
		}
	}

	// furthest
	float maxDistSq = -1.0f;
	auto p0 = manifold.points[idx0].point;
	for ( int i = 0; i < manifold.points.size(); ++i ) {
		if ( i == idx0 ) continue;
		float distSq = uf::vector::distanceSquared( p0, manifold.points[i].point );
		if ( distSq > maxDistSq ) {
			maxDistSq = distSq;
			idx1 = i;
		}
	}

	// max area
	float maxAreaSq = -1.0f;
	auto p1 = manifold.points[idx1].point;
	auto edge0 = p1 - p0;
	for ( int i = 0; i < manifold.points.size(); ++i ) {
		if ( i == idx0 || i == idx1 ) continue;
		auto edge1 = manifold.points[i].point - p0;
		auto crossVec = uf::vector::cross( edge0, edge1 );
		float areaSq = uf::vector::dot( crossVec, crossVec );
		if ( areaSq > maxAreaSq ) {
			maxAreaSq = areaSq;
			idx2 = i;
		}
	}

	// largest convex quad
	float maxDistToCenterSq = -1.0f;
	auto p2 = manifold.points[idx2].point;
	auto center = (p0 + p1 + p2) / 3.0f;
	for ( int i = 0; i < manifold.points.size(); ++i ) {
		if ( i == idx0 || i == idx1 || i == idx2 ) continue;
		float distSq = uf::vector::distanceSquared( center, manifold.points[i].point );
		if ( distSq > maxDistToCenterSq ) {
			maxDistToCenterSq = distSq;
			idx3 = i;
		}
	}

	// rebuild
	pod::Manifold reducedManifold = manifold;
	reducedManifold.points.clear();
	reducedManifold.points.reserve( 4 );

	reducedManifold.points.emplace_back( manifold.points[idx0] );
	reducedManifold.points.emplace_back( manifold.points[idx1] );
	reducedManifold.points.emplace_back( manifold.points[idx2] );
	reducedManifold.points.emplace_back( manifold.points[idx3] );

	manifold.points = std::move( reducedManifold.points );
}

void impl::mergeManifold( pod::Manifold& manifold ) {
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

void impl::retrieveManifold( pod::Manifold& current, const pod::Manifold& previous, float distanceThreshold, float separationThreshold, float decay ) {
	auto& a = *current.a;
	auto& b = *current.b;

	auto tA = impl::getTransform( a );
	auto tB = impl::getTransform( b );

	uf::stl::vector<pod::Contact> merged = current.points;

	float distSqThresh = distanceThreshold * distanceThreshold;
	for ( const auto& oldContact : previous.points ) {
		// reproject point according to current transform
		auto worldA = impl::apply( tA, oldContact.localA );
		auto worldB = impl::apply( tB, oldContact.localB );

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
		for ( auto& c : merged ) {
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

void impl::updateManifoldCache( pod::Island& island, const pod::CollisionEvent::array_t& previous, uf::stl::unordered_map<size_t, pod::Manifold>& cache ) {
	auto& manifolds = island.manifolds;
	auto& active = island.active;
	auto& events = island.events;

	auto cacheLifetime = uf::physics::settings.manifoldCacheLifetime ? uf::physics::settings.manifoldCacheLifetime : MAX(1, uf::physics::settings.substeps) * 2;

	for ( auto& m : manifolds ) {
		if ( m.points.empty() ) continue;
		m.primaryContactID = 0;

		if ( m.points.size() > 1 ) {
			float maxImpulse = -1.0f;
			float maxPen = -FLT_MAX;
			for ( size_t i = 0; i < m.points.size(); ++i ) {
				if ( m.points[i].accumulatedNormalImpulse > maxImpulse ) {
					m.primaryContactID = i;
					maxImpulse = m.points[i].accumulatedNormalImpulse;
				}
				if ( m.points[i].penetration > maxPen ) {
					maxPen = m.points[i].penetration;
					m.primaryContactID = i;
				}
			}
		}

		impl::dispatchManifold( m, events, active, previous );

		if ( uf::physics::settings.warmupSolver ) {
			auto it = uf::physics::settings.manifoldsCache.find( impl::makePairKey( *m.a, *m.b ) );
			if ( it != uf::physics::settings.manifoldsCache.end() ) {
				it->second = m;

				auto& cachedPoints = it->second.points;
				auto newEnd = std::remove_if(cachedPoints.begin(), cachedPoints.end(),
					[cacheLifetime](pod::Contact& c) {
						c.lifetime++;
						return c.lifetime > cacheLifetime;
					});
				cachedPoints.erase(newEnd, cachedPoints.end());
			}
		}
	}
}

void impl::pruneManifoldCache( uf::stl::unordered_map<size_t, pod::Manifold>& cache ) {
	auto cacheLifetime = uf::physics::settings.manifoldCacheLifetime;
	if ( !cacheLifetime ) {
		cacheLifetime = MAX(1, uf::physics::settings.substeps) * 2;
	}

	for ( auto itCache = cache.begin(); itCache != cache.end(); ) {
		auto& manifold = itCache->second;

		if ( manifold.a && manifold.b && !manifold.a->activity.awake && !manifold.b->activity.awake ) {
			++itCache;
			continue;
		}

		// prune points that are too old
		auto newEnd = std::remove_if(manifold.points.begin(), manifold.points.end(),
			[cacheLifetime](const pod::Contact& c) { return c.lifetime > cacheLifetime; });
		manifold.points.erase(newEnd, manifold.points.end());

		// empty manifold, kill it
		if ( manifold.points.empty() ) {
			itCache = cache.erase(itCache);
		} else {
			++itCache;
		}
	}
}

void impl::warmupContact( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Contact& c, float dt ) {
	if ( !c.lifetime ) return; // too new

	// build relative offsets (use transform origin, consistent with the solver)
	pod::Vector3f rA = c.point - impl::getPosition( a, true );
	pod::Vector3f rB = c.point - impl::getPosition( b, true );

	// normal impulse
	pod::Vector3f Pn = c.normal * c.accumulatedNormalImpulse;
	impl::applyImpulseTo( a, b, rA, rB, Pn );

	// tangent basis
	pod::Vector3f Pt = c.tangent * c.accumulatedTangentImpulse;
	impl::applyImpulseTo( a, b, rA, rB, Pt );
}
void impl::warmupManifold( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Manifold& manifold, float dt ) {
	for ( auto& contact : manifold.points ) {
		impl::warmupContact( a, b, contact, dt );
	}
}

void impl::resolveManifold( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt ) {
	// ignore if a body is a trigger
	if ( (a.collider.category & pod::Collider::CATEGORY_TRIGGER) || (b.collider.category & pod::Collider::CATEGORY_TRIGGER) ) return;

	if ( uf::physics::settings.blockContactSolver ) {
		if ( impl::blockSolver( a, b, manifold, dt ) ) return;
	}
	for ( auto& contact : manifold.points ) impl::iterativeImpulseSolver( a, b, contact, dt );
}

void impl::solveManifold( uf::stl::vector<pod::Manifold>& manifolds, float dt ) {
	if ( uf::physics::settings.warmupSolver ) for ( auto& manifold : manifolds ) impl::warmupManifold( *manifold.a, *manifold.b, manifold, dt );
	for ( auto i = 0; i < uf::physics::settings.solverIterations; ++i ) for ( auto& manifold : manifolds ) impl::resolveManifold( *manifold.a, *manifold.b, manifold, dt );
}

void impl::dispatchManifold( pod::Manifold& manifold, pod::CollisionEvent::events_t& events, pod::CollisionEvent::array_t& active, const pod::CollisionEvent::array_t& previous ) {
	auto pairKey = impl::makePairKey( *manifold.a, *manifold.b );

	active.emplace_back(pod::PairState{ pairKey, manifold.a, manifold.b });

	auto it = std::lower_bound(previous.begin(), previous.end(), pod::PairState{pairKey, nullptr, nullptr});
	bool isNew = (it == previous.end() || it->pairKey != pairKey);

	auto& contact = manifold.points[manifold.primaryContactID];

	events.emplace_back(pod::CollisionEvent{
		.state = isNew ? pod::CollisionState::ENTER : pod::CollisionState::SUSTAIN,
		.a = manifold.a,
		.b = manifold.b,
		.point = contact.point,
		.normal = contact.normal,
		.impulse = contact.accumulatedNormalImpulse,
		.featureA = contact.featureA,
		.featureB = contact.featureB,
	});
}