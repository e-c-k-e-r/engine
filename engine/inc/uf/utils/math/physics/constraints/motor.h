#pragma once

#include "../structs.h"

namespace impl {
    void solve1DLinearMotor(
	   pod::PhysicsBody& a, pod::PhysicsBody& b,
        const pod::Vector3f& rA, const pod::Vector3f& rB, const pod::Vector3f& axis,
        float targetVelocity, float maxForce, float& accumulatedImpulse, float dt
    );
    void solve1DAngularMotor(
        pod::PhysicsBody& a, pod::PhysicsBody& b,
        const pod::Vector3f& axis,
        float targetVelocity, float maxTorque,
        float& accumulatedImpulse, float dt
    );
}