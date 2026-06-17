#include "behavior.h"
#include "../input/behavior.h"

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

UF_BEHAVIOR_REGISTER_CPP(ext::PlayerInteractionBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::PlayerInteractionBehavior, ticks = true, renders = false, thread = uf::thread::asyncThreadName)
#define this (&self)

void ext::PlayerInteractionBehavior::initialize(uf::Object& self) {
	auto& metadata = this->getComponent<ext::PlayerInteractionBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();
	
	UF_BEHAVIOR_METADATA_BIND_SERIALIZER_HOOKS( metadata, metadataJson );
}

void ext::PlayerInteractionBehavior::tick(uf::Object& self) {
	if (!this->hasComponent<ext::PlayerInputBehavior::Metadata>()) return;
	if (!this->hasComponent<pod::PhysicsBody>()) return; // We need a physics body for the raycast!

	auto& input = this->getComponent<ext::PlayerInputBehavior::Metadata>();
	auto& metadata = this->getComponent<ext::PlayerInteractionBehavior::Metadata>();
	auto& physicsBody = this->getComponent<pod::PhysicsBody>();

	TIMER(0.25, input.use) {
		auto& camera = this->getComponent<uf::Camera>();
		auto cameraTransform = camera.getTransform();
		auto flattened = uf::transform::flatten(cameraTransform);
		auto axes = uf::transform::axes( flattened );

		pod::Vector3f center = flattened.position;
		pod::Vector3f direction = axes.forward;

		pod::RayQuery query = uf::physics::rayCast( pod::Ray{center, direction}, physicsBody, metadata.length );

		uf::Object* pointer = query.hit ? query.body->object : NULL;
		float depth = query.hit ? query.contact.penetration : -1;

		ext::json::Value payload;
		payload["user"] = this->getUid();
		payload["uid"] = pointer ? pointer->getUid() : 0;
		payload["depth"] = depth;

		if ( pointer ) {
			pointer->lazyCallHook("entity:Use.%UID%", payload);
		}
		this->lazyCallHook("entity:Use.%UID%", payload);
	}
}

void ext::PlayerInteractionBehavior::render(uf::Object& self) {}
void ext::PlayerInteractionBehavior::destroy(uf::Object& self) {}

void ext::PlayerInteractionBehavior::Metadata::serialize(uf::Object& self, uf::Serializer& serializer) {
	serializer["use"]["length"] = length;
}
void ext::PlayerInteractionBehavior::Metadata::deserialize(uf::Object& self, uf::Serializer& serializer) {
	length = serializer["use"]["length"].as(length);
}
#undef this