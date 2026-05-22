#pragma once

#include "impl.h"

namespace impl {
	float computeEffectiveMass( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Vector3f& rA, const pod::Vector3f& rB, const pod::Vector3f& n );
	void applyImpulseTo( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Vector3f& rA, const pod::Vector3f& rB, const pod::Vector3f& impulse );
	void applyImpulseTo(
		pod::PhysicsBody& a, pod::PhysicsBody& b,
		const pod::Vector3f& rA, const pod::Vector3f& rB,
		const pod::Vector3f& direction, float magnitude,
		float& accumulatedImpulse,
		float minLimit = -std::numeric_limits<float>::max(), float maxLimit = std::numeric_limits<float>::max()
	);
	void applyImpulseTo(
		pod::PhysicsBody& a, pod::PhysicsBody& b,
		const pod::Vector3f& rA, const pod::Vector3f& rB,
		const pod::Vector3f& impulse,
		pod::Vector3f& accumulatedImpulse,
		float minLimit = -std::numeric_limits<float>::max(), float maxLimit = std::numeric_limits<float>::max()
	);
	
	void applyPseudoImpulseTo( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Vector3f& rA, const pod::Vector3f& rB, const pod::Vector3f& impulse );
	void applyPseudoImpulseTo(
		pod::PhysicsBody& a, pod::PhysicsBody& b,
		const pod::Vector3f& rA, const pod::Vector3f& rB,
		const pod::Vector3f& direction, float magnitude,
		float& accumulatedImpulse,
		float minLimit = -std::numeric_limits<float>::max(), float maxLimit = std::numeric_limits<float>::max()
	);
	void applyPseudoImpulseTo(
		pod::PhysicsBody& a, pod::PhysicsBody& b,
		const pod::Vector3f& rA, const pod::Vector3f& rB,
		const pod::Vector3f& impulse,
		pod::Vector3f& accumulatedImpulse,
		float minLimit = -std::numeric_limits<float>::max(), float maxLimit = std::numeric_limits<float>::max()
	);

	float accumulateImpulseTo(
		float magnitude, float& accumulatedImpulse,
		float minLimit = -std::numeric_limits<float>::max(), float maxLimit = std::numeric_limits<float>::max()
	);
	pod::Vector3f accumulateImpulseTo(
		const pod::Vector3f& impulse, pod::Vector3f& accumulatedImpulse,
		float minLimit = -std::numeric_limits<float>::max(), float maxLimit = std::numeric_limits<float>::max()
	);

	void applyRollingResistance( pod::PhysicsBody& body, float dt );
	void snapVelocity( pod::PhysicsBody& body, float dt, float threshold = 0.01f );
	void integrate( pod::PhysicsBody& body, float dt );
}