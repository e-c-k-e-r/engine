#include "behavior.h"
#include "../input/behavior.h"
#include "../movement/behavior.h"

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
#include <uf/ext/openvr/openvr.h>

#include "../../scene/behavior.h"

#define ONE_OVER_SIXTY 0.016666f

UF_BEHAVIOR_REGISTER_CPP(ext::PlayerCameraBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::PlayerCameraBehavior, ticks = true, renders = false, thread = uf::thread::asyncThreadName)
#define this (&self)

void ext::PlayerCameraBehavior::initialize( uf::Object& self ) {
	auto& transform = this->getComponent<pod::Transform<>>();
	auto& metadata = this->getComponent<ext::PlayerCameraBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();

	auto& camera = this->getComponent<uf::Camera>();
	auto& cameraTransform = camera.getTransform();

	camera.setStereoscopic(true);

	cameraTransform.position = uf::vector::decode(metadataJson["camera"]["position"], cameraTransform.position);
	cameraTransform.scale = uf::vector::decode(metadataJson["camera"]["scale"], cameraTransform.scale);
	cameraTransform.orientation = uf::vector::decode(metadataJson["camera"]["orientation"], cameraTransform.orientation);

	cameraTransform.reference = metadata.fixed ? NULL : &transform;

	auto cameraSettingsJson = metadataJson["camera"]["settings"];
	if ( metadataJson["camera"]["ortho"].as<bool>() ) {
		float l = cameraSettingsJson["left"].as<float>();
		float r = cameraSettingsJson["right"].as<float>();
		float b = cameraSettingsJson["bottom"].as<float>();
		float t = cameraSettingsJson["top"].as<float>();
		float n = cameraSettingsJson["near"].as<float>();
		float f = cameraSettingsJson["far"].as<float>();

		camera.setProjection( uf::matrix::orthographic( l, r, b, t, n, f ) );
	} else {
		float fov = cameraSettingsJson["fov"].as<float>(120) * (3.14159265358f / 180.0f);
		pod::Vector2f range = uf::vector::decode(cameraSettingsJson["clip"], pod::Vector2f{0.1, 64.0f});
		pod::Vector2ui size = uf::vector::decode(cameraSettingsJson["size"], pod::Vector2ui{uf::renderer::settings::width, uf::renderer::settings::height});
		float raidou = (float) size.x / (float) size.y;

		if ( size.x == 0 || size.y == 0 ) {
			size = uf::vector::decode(uf::config["window"]["size"], pod::Vector2ui{});
			raidou = (float) size.x / (float) size.y;
		}

		if ( ext::openvr::enabled ) {
			camera.setProjection( ext::openvr::hmdProjectionMatrix(0, range.x, range.y), 0 );
			camera.setProjection( ext::openvr::hmdProjectionMatrix(1, range.x, range.y), 1 );
		} else {
			camera.setProjection( uf::matrix::perspective( fov, raidou, range.x, range.y ) );
		}
	}
	camera.update();

	metadata.mouse.sensitivity = uf::vector::decode(uf::config["window"]["mouse"]["sensitivity"], metadata.mouse.sensitivity);
	metadata.mouse.smoothing = uf::vector::decode(uf::config["window"]["mouse"]["smoothing"], metadata.mouse.smoothing);

	UF_BEHAVIOR_METADATA_BIND_SERIALIZER_HOOKS(metadata, metadataJson);
}

void ext::PlayerCameraBehavior::tick( uf::Object& self ) {
	auto& metadata = this->getComponent<ext::PlayerCameraBehavior::Metadata>();
	auto& transform = this->getComponent<pod::Transform<>>();
	auto& camera = this->getComponent<uf::Camera>();
	auto& cameraTransform = camera.getTransform();

	auto axes = uf::transform::axes(transform);
	auto cameraAxes = uf::transform::axes(cameraTransform);

	if ( uf::renderer::states::resized && uf::renderer::settings::width > 0 && uf::renderer::settings::height > 0 ) {
		auto& metadataJson = this->getComponent<uf::Serializer>();
		auto cameraSettingsJson = metadataJson["camera"]["settings"];

		float fov = cameraSettingsJson["fov"].as<float>(120) * (3.14159265358f / 180.0f);
		float raidou = (float) uf::renderer::settings::width / (float) uf::renderer::settings::height;
		pod::Vector2f range = uf::vector::decode(cameraSettingsJson["clip"], pod::Vector2f{0.1, 64.0f});

		camera.setProjection(uf::matrix::perspective(fov, raidou, range.x, range.y));
	}

	if ( this->hasComponent<ext::PlayerInputBehavior::Metadata>() ) {
		auto& input = this->getComponent<ext::PlayerInputBehavior::Metadata>();

		if ( input.look.x != 0 && input.look.y != 0 ) {
			metadata.queued.x += input.look.x * metadata.mouse.sensitivity.x;
			metadata.queued.y += input.look.y * metadata.mouse.sensitivity.y;

			// Note: ensure you reset input.look = {0,0} at the start of PlayerInputBehavior::tick!
			input.look = {0, 0};
		}

		if ( metadata.queued.x != 0 || metadata.queued.y != 0 ) {
			auto lookDelta = metadata.queued;
			metadata.queued -= lookDelta * metadata.mouse.smoothing;

			if ( lookDelta.x != 0 ) {
				if ( metadata.invert.x ) lookDelta.x *= -1;
				metadata.limit.current.x += lookDelta.x;

				if ( metadata.limit.current.x != metadata.limit.current.x || (metadata.limit.current.x < metadata.limit.max.x && metadata.limit.current.x > metadata.limit.min.x) ) {
					if ( this->hasComponent<pod::PhysicsBody>() ) {
						auto& physicsBody = this->getComponent<pod::PhysicsBody>();
						if ( physicsBody.object ) uf::physics::applyRotation( physicsBody, axes.up, lookDelta.x );
						else uf::transform::rotate( transform, axes.up, lookDelta.x );
					} else {
						uf::transform::rotate(transform, axes.up, lookDelta.x);
					}
				} else metadata.limit.current.x -= lookDelta.x;
			}

			if ( lookDelta.y != 0 ) {
				if ( metadata.invert.y ) lookDelta.y *= -1;
				metadata.limit.current.y += lookDelta.y;

				if ( metadata.limit.current.y != metadata.limit.current.y || (metadata.limit.current.y < metadata.limit.max.y && metadata.limit.current.y > metadata.limit.min.y) ) {
					uf::transform::rotate( cameraTransform, cameraAxes.right, lookDelta.y );
				} else metadata.limit.current.y -= lookDelta.y;
			}
		}
	}

	if ( metadata.fixed ) {
		cameraTransform.reference = NULL;
		cameraTransform.position = transform.position + metadata.offset;
	} else {
		if ( metadata.offset != pod::Vector3f{0,0,0} ) {
			metadata.intermediary.position = uf::quaternion::rotate( transform.orientation, metadata.offset );
			metadata.intermediary.reference = &transform;
			cameraTransform.reference = &metadata.intermediary;
		}
		if ( this->hasComponent<ext::PlayerInputBehavior::Metadata>() && this->hasComponent<ext::PlayerMovementBehavior::Metadata>() ) {
			auto& input = this->getComponent<ext::PlayerInputBehavior::Metadata>();
			auto& movement = this->getComponent<ext::PlayerMovementBehavior::Metadata>();

			float targetRoll = -input.movement.x * 0.02f;
			metadata.viewRoll = std::lerp(metadata.viewRoll, targetRoll, 8.0f * ONE_OVER_SIXTY);

			float rollDelta = metadata.viewRoll - metadata.previousRoll;
			metadata.previousRoll = metadata.viewRoll;

			uf::transform::rotate(cameraTransform, cameraAxes.forward, rollDelta);

			if ( this->hasComponent<pod::PhysicsBody>() ) {
				auto& physicsBody = this->getComponent<pod::PhysicsBody>();
				float currentYVel = physicsBody.velocity.y;

				if ( movement.floored && !metadata.wasFloored ) {
					if ( metadata.lastYVelocity < -4.0f ) {
						metadata.punchVelocity += metadata.lastYVelocity * 0.025f;
					}
				}

				metadata.punchVelocity += (0.0f - metadata.viewPunch) * 0.2f;
				metadata.punchVelocity *= 0.7f;
				metadata.viewPunch += metadata.punchVelocity;

				metadata.viewPunch = std::clamp(metadata.viewPunch, -0.5f, 0.0f);

				float punchDelta = metadata.viewPunch - metadata.previousPunch;
				metadata.previousPunch = metadata.viewPunch;

				float lerpSpeed = (metadata.stairOffset > 0.0f) ? 5.0f : 15.0f;
				metadata.stairOffset = std::lerp(metadata.stairOffset, 0.0f, lerpSpeed * ONE_OVER_SIXTY);

				if ( std::abs(metadata.stairOffset) < 0.0001f ) metadata.stairOffset = 0.0f;

				float stairDelta = metadata.stairOffset - metadata.previousStairOffset;
				metadata.previousStairOffset = metadata.stairOffset;

				cameraTransform.position.y += punchDelta + stairDelta;

				metadata.wasFloored = movement.floored;
				metadata.lastYVelocity = currentYVel;
			}
		}
	}


	camera.update();
}

void ext::PlayerCameraBehavior::render( uf::Object& self ) {}
void ext::PlayerCameraBehavior::destroy( uf::Object& self ) {}
void ext::PlayerCameraBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ){
	serializer["camera"]["invert"] = uf::vector::encode(invert);
	serializer["camera"]["limit"]["current"] = uf::vector::encode(limit.current);
	serializer["camera"]["limit"]["minima"] = uf::vector::encode(limit.min);
	serializer["camera"]["limit"]["maxima"] = uf::vector::encode(limit.max);
	serializer["camera"]["settings"]["fixed"] = fixed;
	serializer["camera"]["offset"] = uf::vector::encode(offset);
}
void ext::PlayerCameraBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ){
	invert = uf::vector::decode(serializer["camera"]["invert"], invert);
	limit.current = uf::vector::decode(serializer["camera"]["limit"]["current"], limit.current);
	limit.min = uf::vector::decode(serializer["camera"]["limit"]["minima"], limit.min);
	limit.max = uf::vector::decode(serializer["camera"]["limit"]["maxima"], limit.max);
	fixed = serializer["camera"]["settings"]["fixed"].as(fixed);
	offset = uf::vector::decode(serializer["camera"]["offset"], offset);
}
#undef this

#include <uf/ext/lua/component.h>
UF_LUA_REGISTER_USERTYPE_AND_COMPONENT(ext::PlayerCameraBehavior::Metadata,
	UF_LUA_REGISTER_USERTYPE_MEMBER(ext::PlayerCameraBehavior::Metadata::fixed),
	UF_LUA_REGISTER_USERTYPE_MEMBER(ext::PlayerCameraBehavior::Metadata::offset),
	UF_LUA_REGISTER_USERTYPE_MEMBER(ext::PlayerCameraBehavior::Metadata::viewRoll),
	UF_LUA_REGISTER_USERTYPE_MEMBER(ext::PlayerCameraBehavior::Metadata::viewPunch),
	UF_LUA_REGISTER_USERTYPE_MEMBER(ext::PlayerCameraBehavior::Metadata::punchVelocity),
	UF_LUA_REGISTER_USERTYPE_MEMBER(ext::PlayerCameraBehavior::Metadata::lastYVelocity),
	UF_LUA_REGISTER_USERTYPE_MEMBER(ext::PlayerCameraBehavior::Metadata::wasFloored),
	UF_LUA_REGISTER_USERTYPE_MEMBER(ext::PlayerCameraBehavior::Metadata::stairOffset)
)