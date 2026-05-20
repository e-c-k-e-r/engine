#pragma once

#include "impl.h"

namespace impl {
	float computeEffectiveMass( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Vector3f& rA, const pod::Vector3f& rB, const pod::Vector3f& n );
	void applyImpulseTo( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Vector3f& rA, const pod::Vector3f& rB, const pod::Vector3f& impulse );
	void applyPseudoImpulseTo( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Vector3f& rA, const pod::Vector3f& rB, const pod::Vector3f& impulse );
	void applyRollingResistance( pod::PhysicsBody& body, float dt );
	void bindManifold( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt = 0 );
	bool generateContactsGjk( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt );
	bool generateContacts( pod::PhysicsBody& a, pod::PhysicsBody& b, pod::Manifold& manifold, float dt );
	void computeLocalContacts( pod::Manifold& manifold );
	bool similarContact( const pod::Contact& a, const pod::Contact& b, float distSqThreshold = 1.0e-2f, float normThreshold = 0.9f );
	void reduceContacts( pod::Manifold& manifold );
	void mergeContacts( pod::Manifold& manifold );
	void retrieveContacts( pod::Manifold& current, const pod::Manifold& previous, float distanceThreshold = 0.1f, float separationThreshold = 0.1f, float decay = 0.85f );
	void prepareManifoldCache( uf::stl::unordered_map<size_t, pod::Manifold>& cache, const uf::stl::vector<pod::Island>& islands, const uf::stl::vector<pod::PhysicsBody*>& bodies );
	void updateManifoldCache( const uf::stl::vector<pod::Manifold>& manifolds, uf::stl::unordered_map<size_t, pod::Manifold>& cache );
	void pruneManifoldCache( uf::stl::unordered_map<size_t, pod::Manifold>& cache );
	void warmupContacts( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Contact& c, float dt );
	void warmupManifold( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Manifold& manifold, float dt );
	void snapVelocity( pod::PhysicsBody& body, float dt, float threshold = 0.01f );
	void positionCorrection( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Contact& contact );
	void integrate( pod::PhysicsBody& body, float dt );
}