#pragma once

#include "structs.h"

namespace impl {
	/*FORCE_INLINE*/ pod::SolverBodyContext solverBodyContext( const pod::PhysicsBody& body );

	float computeEffectiveMass( const pod::SolverBodyContext& a, const pod::SolverBodyContext& b, const pod::JacobianRow& row );
	float computeEffectiveMass( pod::PhysicsBody& a, pod::PhysicsBody& b, const pod::Vector3f& rA, const pod::Vector3f& rB, const pod::Vector3f& n );
	float computeMassMatrixLine( const pod::SolverBodyContext& a, const pod::SolverBodyContext& b, const pod::JacobianRow& rowI, const pod::JacobianRow& rowJ );
	float computeAngularMassMatrixLine( const pod::SolverBodyContext& a, const pod::SolverBodyContext& b, const pod::Vector3f& n_i, const pod::Vector3f& n_j );

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