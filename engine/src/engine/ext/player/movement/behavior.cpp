#include "behavior.h"
#include "../input/behavior.h"
#include "../camera/behavior.h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/window/window.h>
#include <uf/utils/window/payloads.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/audio/audio.h>
#include <uf/ext/openvr/openvr.h>
#include <uf/engine/graph/graph.h>
#include <uf/utils/math/physics.h>
#include <uf/spec/controller/controller.h>
#include <uf/utils/io/inputs.h>

#define ONE_OVER_SIXTY 0.016666f

UF_BEHAVIOR_REGISTER_CPP(ext::PlayerMovementBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::PlayerMovementBehavior, ticks = true, renders = false, thread = uf::thread::asyncThreadName)
#define this (&self)

void ext::PlayerMovementBehavior::initialize(uf::Object& self) {
	auto& metadata = this->getComponent<ext::PlayerMovementBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();
	UF_BEHAVIOR_METADATA_BIND_SERIALIZER_HOOKS(metadata, metadataJson);
}

void ext::PlayerMovementBehavior::tick(uf::Object& self) {
	if ( !this->hasComponent<ext::PlayerInputBehavior::Metadata>() ) return;

	auto& input = this->getComponent<ext::PlayerInputBehavior::Metadata>();
	auto& metadata = this->getComponent<ext::PlayerMovementBehavior::Metadata>();
	auto& transform = this->getComponent<pod::Transform<>>();
	auto& physicsBody = this->getComponent<pod::PhysicsBody>();

	auto& camera = this->getComponent<uf::Camera>();
	auto& cameraTransform = camera.getTransform();
	
	auto cameraAxes = uf::transform::axes( cameraTransform );
	auto axes = uf::transform::axes( transform );

	/*if ( metadata.camera.fixed ) {
		axes = uf::transform::axes( cameraTransform );
		axes.forward.y = 0;
		axes.forward = uf::vector::normalize( axes.forward );
	}
	else*/ if ( metadata.noclipped || physicsBody.gravity == pod::Vector3f{0,0,0} ){
		axes.forward.y += cameraAxes.forward.y;
		axes.forward = uf::vector::normalize( axes.forward );
	}

	bool wasFloored = metadata.floored;
	metadata.deltaCrouch = false;
	metadata.floored = metadata.noclipped;
	if ( !metadata.floored ) {
		if ( physicsBody.activity.grounded ) {
			metadata.floored = true;
		}
	}
	if ( physicsBody.gravity == pod::Vector3f{0,0,0} ) metadata.noclipped = true;

	{
		TIMER(0.25, input.noclipToggle) {
			bool state = !metadata.noclipped;
			metadata.noclipped = state;
			if (!state) {
				uf::physics::setGravity(physicsBody);
				uf::physics::setColliderCategory(physicsBody, "PLAYER");
				uf::physics::setColliderMask(physicsBody, "PLAYER");
			} else {
				uf::physics::setGravity(physicsBody, pod::Vector3f{0,0,0});
				uf::physics::setColliderCategory(physicsBody, "NONE");
				uf::physics::setColliderMask(physicsBody, "NONE");
			}
			UF_MSG_DEBUG("{}abled noclip: {}", (state ? "En" : "Dis"), uf::vector::toString(transform.position));
		}
	}

	float currentSpeed = metadata.settings.move;
	if ( input.run ) currentSpeed = metadata.settings.run;
	else if ( input.walk ) currentSpeed = metadata.settings.walk;

	currentSpeed *= input.magnitude;

	float currentFriction = metadata.settings.friction;
	if ( metadata.noclipped ) currentSpeed *= 1.5f;
	if ( !metadata.floored || metadata.noclipped ) currentFriction = 1.0f;
	if ( metadata.noclipped ) physicsBody.velocity = {};

	pod::Vector3f target = (axes.forward * input.movement.y) + (axes.right * input.movement.x);
	if (uf::vector::norm(target) > 0) {
		target = uf::vector::normalize(target);
	}

	physicsBody.velocity *= { currentFriction, 1.0f, currentFriction };

	metadata.walking = (input.movement.x != 0 || input.movement.y != 0);
	metadata.running = input.run;

	if ( metadata.walking && !metadata.noclipped && physicsBody.object ) {
		float stepHeight = 0.65f;
		float lookAhead = 0.15f;

		float radius = 0.5f;
		float cylHalfHeight = 1.0f;
		if ( physicsBody.collider.type == pod::ShapeType::CAPSULE ) {
			radius = physicsBody.collider.capsule.radius;
			cylHalfHeight = uf::vector::norm(physicsBody.collider.capsule.up);
		}

		pod::Vector3f centerPos = transform.position + physicsBody.offsetPosition;
		float feetY = centerPos.y - (cylHalfHeight + radius);

		pod::Vector3f checkDir = target;
		checkDir.y = 0;
		if ( uf::vector::norm(checkDir) > 0.0f ) {
			checkDir = uf::vector::normalize(checkDir);
		}

		bool steppedThisFrame = false;

		if ( uf::vector::norm(checkDir) > 0.0f ) {
			pod::Ray highFwdRay;
			highFwdRay.origin = centerPos;
			highFwdRay.origin.y = feetY + stepHeight + 0.1f;
			highFwdRay.direction = checkDir;

			auto highFwdHit = uf::physics::rayCast( highFwdRay, physicsBody, radius + lookAhead + 0.1f );

			if ( highFwdHit.contact.penetration > (radius + lookAhead) ) {
				pod::Ray downRay;
				downRay.origin = centerPos + (checkDir * (radius + lookAhead));
				downRay.origin.y = feetY + stepHeight + 0.1f;
				downRay.direction = -axes.up;

				auto downHit = uf::physics::rayCast( downRay, physicsBody, stepHeight * 1.5f );

				if ( downHit.contact.penetration <= stepHeight * 1.5f ) {
					float floorDot = uf::vector::dot(downHit.contact.normal, axes.up);
					float stairYDifference = downHit.contact.point.y - feetY;

					if ( floorDot > uf::physics::settings.groundedThreshold ) {
						if ( stairYDifference > 0.05f && stairYDifference <= stepHeight ) {
							transform.position.y += stairYDifference;

							if ( this->hasComponent<ext::PlayerCameraBehavior::Metadata>() ) {
								this->getComponent<ext::PlayerCameraBehavior::Metadata>().stairOffset -= stairYDifference;
							}

							if ( physicsBody.velocity.y < 0.0f ) physicsBody.velocity.y = 0.0f;
							metadata.floored = true;
							physicsBody.activity.grounded = true;
							steppedThisFrame = true;
						}
					}
				}
			}
		}

		// commenting this out makes the camera smooth
		bool allowedToSnap = wasFloored || (physicsBody.velocity.y > -3.0f && physicsBody.velocity.y < 0.0f);

		if ( !steppedThisFrame && allowedToSnap && !physicsBody.activity.grounded && physicsBody.velocity.y <= 0.1f ) {
			pod::Vector3f rayOffsets[3] = {
				pod::Vector3f{0, 0, 0},
				checkDir * (radius * 0.4f),
				checkDir * -(radius * 0.4f)
			};

			bool snappedThisFrame = false;

			for ( int i = 0; i < 3; ++i ) {
				if ( snappedThisFrame ) break;

				pod::Ray stickRay;
				stickRay.origin = centerPos + rayOffsets[i];
				stickRay.direction = -axes.up;

				float castDist = (cylHalfHeight + radius) + stepHeight + 0.1f;
				auto stickHit = uf::physics::rayCast( stickRay, physicsBody, castDist );

				if ( stickHit.contact.penetration > 0.0f && stickHit.contact.penetration <= castDist ) {
					float floorDot = uf::vector::dot(stickHit.contact.normal, axes.up);
					float floorY = stickHit.contact.point.y;
					float dropDist = feetY - floorY;

					if ( floorDot > uf::physics::settings.groundedThreshold ) {

						if ( dropDist > 0.01f && dropDist <= stepHeight ) {
							bool shouldSnapDown = true;

							if ( uf::vector::norm(checkDir) > 0.0f ) {
								pod::Ray fwdDownRay;
								fwdDownRay.origin = centerPos + (checkDir * (radius + 0.1f));
								fwdDownRay.origin.y = feetY + stepHeight + 0.1f;
								fwdDownRay.direction = -axes.up;

								float fwdCastDist = stepHeight * 1.5f;
								auto fwdHit = uf::physics::rayCast(fwdDownRay, physicsBody, fwdCastDist);

								if ( fwdHit.contact.penetration > 0.0f && fwdHit.contact.penetration <= fwdCastDist ) {
									float fwdDropDist = feetY - fwdHit.contact.point.y;

									if ( fwdDropDist >= -0.25f && fwdDropDist <= 0.05f ) {
										shouldSnapDown = false;
									}
								}
							}

							if ( shouldSnapDown ) {
								float hSpeed = uf::vector::norm(pod::Vector3f{physicsBody.velocity.x, 0.0f, physicsBody.velocity.z});
								if ( hSpeed < 1.0f ) hSpeed = currentSpeed;
								physicsBody.velocity.y = -hSpeed * 1.5f;

								if ( uf::vector::norm(target) > 0.0f ) {
									physicsBody.velocity.x = target.x * currentSpeed;
									physicsBody.velocity.z = target.z * currentSpeed;
								}

								metadata.floored = true;
								physicsBody.activity.grounded = true;
								snappedThisFrame = true;
							}
						}
					}
				}
			}
		}
	}

	if ( metadata.walking ) {
		float factor = metadata.floored ? 1.0f : metadata.settings.air;
		if ( metadata.noclipped ) {
			physicsBody.velocity += target * currentSpeed * 50.0f * ONE_OVER_SIXTY;
		} else {
			physicsBody.velocity += target * std::clamp(
				currentSpeed * factor - uf::vector::dot(physicsBody.velocity, target),
				0.0f,
				currentSpeed * 10.0f * ONE_OVER_SIXTY
			);
		}

		auto dot = uf::vector::dot( axes.forward, target );
		if ( !metadata.settings.strafe && dot < 1.0f ) {
			auto axis = axes.up;
			float angle = uf::vector::signedAngle( axes.forward, target, axis ) * ONE_OVER_SIXTY /*uf::physics::time::delta*/ * 4;

			if ( physicsBody.object ) uf::physics::applyRotation( physicsBody, axis, angle ); else
			uf::transform::rotate( transform, axis, angle );
		}
	}

	{
		TIMER( 0.0625, metadata.floored && input.jump && !metadata.noclipped ) {
			physicsBody.velocity += axes.up * metadata.settings.jump;
		}
	}
	if ( metadata.floored && input.jump && metadata.noclipped ) {
		transform.position += axes.up * metadata.settings.jump * uf::physics::time::delta * 4.0f;
	}

	if ( input.crouch ) {
		if ( metadata.noclipped ) transform.position -= axes.up * metadata.settings.jump * uf::physics::time::delta * 4.0f;
		else {
			if ( !metadata.crouching ) metadata.deltaCrouch = true;
			metadata.crouching = true;
		}
	} else {
		if ( metadata.crouching ) metadata.deltaCrouch = true;
		metadata.crouching = false;
	}

	if ( metadata.deltaCrouch && !metadata.noclipped && physicsBody.object ) {
		float halfCrouch = metadata.settings.crouch * 0.5f;
		if ( physicsBody.collider.type == pod::ShapeType::CAPSULE ) {
			if ( metadata.crouching ) {
				physicsBody.collider.capsule.up.y -= halfCrouch;
				physicsBody.offsetPosition.y += halfCrouch;

				if ( metadata.floored ) {
					transform.position.y -= metadata.settings.crouch;
				}
			} else {
				physicsBody.collider.capsule.up.y += halfCrouch;
				physicsBody.offsetPosition.y -= halfCrouch;
			}
		}
	}

	if ( physicsBody.object ) uf::physics::setVelocity(physicsBody, physicsBody.velocity);
	else transform.position += physicsBody.velocity * ONE_OVER_SIXTY;
}

void ext::PlayerMovementBehavior::render(uf::Object& self) {}
void ext::PlayerMovementBehavior::destroy(uf::Object& self) {}
void ext::PlayerMovementBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ){
	serializer["system"]["crouching"] = crouching;
	serializer["system"]["noclipped"] = noclipped;

	serializer["physics"]["friction"] = settings.friction;

	serializer["movement"]["rotate"] = settings.rotate;
	serializer["movement"]["move"] = settings.move;
	serializer["movement"]["run"] = settings.run;
	serializer["movement"]["walk"] = settings.walk;
	serializer["movement"]["air"] = settings.air;
	serializer["movement"]["strafe"] = settings.strafe;
	serializer["movement"]["crouch"] = settings.crouch;
	serializer["movement"]["jump"] = uf::vector::encode(settings.jump);
}
void ext::PlayerMovementBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ){
	crouching = serializer["system"]["crouching"].as(crouching);
	noclipped = serializer["system"]["noclipped"].as(noclipped);

	settings.friction = serializer["physics"]["friction"].as(settings.friction);

	settings.rotate = serializer["movement"]["rotate"].as(settings.rotate);
	settings.move = serializer["movement"]["move"].as(settings.move);
	settings.run = serializer["movement"]["run"].as(settings.run);
	settings.walk = serializer["movement"]["walk"].as(settings.walk);
	settings.air = serializer["movement"]["air"].as(settings.air);
	settings.strafe = serializer["movement"]["strafe"].as(settings.strafe);
	settings.crouch = serializer["movement"]["crouch"].as(settings.crouch);
	settings.jump = uf::vector::decode(serializer["movement"]["jump"], settings.jump);
}
#undef this

#include <uf/ext/lua/component.h>
UF_LUA_REGISTER_USERTYPE_AND_COMPONENT(ext::PlayerMovementBehavior::Metadata,
	UF_LUA_REGISTER_USERTYPE_MEMBER(ext::PlayerMovementBehavior::Metadata::walking),
	UF_LUA_REGISTER_USERTYPE_MEMBER(ext::PlayerMovementBehavior::Metadata::running),
	UF_LUA_REGISTER_USERTYPE_MEMBER(ext::PlayerMovementBehavior::Metadata::crouching),
	UF_LUA_REGISTER_USERTYPE_MEMBER(ext::PlayerMovementBehavior::Metadata::floored),
	UF_LUA_REGISTER_USERTYPE_MEMBER(ext::PlayerMovementBehavior::Metadata::noclipped),
	UF_LUA_REGISTER_USERTYPE_MEMBER(ext::PlayerMovementBehavior::Metadata::deltaCrouch)
)