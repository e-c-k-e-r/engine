#include "behavior.h"

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

UF_BEHAVIOR_REGISTER_CPP(ext::PlayerInputBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::PlayerInputBehavior, ticks = true, renders = false, thread = uf::thread::asyncThreadName)
#define this (&self)
void ext::PlayerInputBehavior::initialize(uf::Object& self) {
	auto& transform = this->getComponent<pod::Transform<>>();

	auto& state = this->getComponent<ext::PlayerInputBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();
	
	auto& scene = uf::scene::getCurrentScene();

	this->addHook( "window:Mouse.CursorVisibility", [&](pod::payloads::windowMouseCursorVisibility& payload){
		state.control = !payload.mouse.visible;
	});
	this->addHook( "system:Control.%UID%", [&]( ext::json::Value& value ){
		state.control = value["control"].as<bool>(!state.control);
	});

	ext::json::Value payload;
	payload["uid"] = this->getUid();
	this->queueHook("controller:Ready", payload, 0.0f );

	UF_BEHAVIOR_METADATA_BIND_SERIALIZER_HOOKS(state, metadataJson);
}

void ext::PlayerInputBehavior::tick(uf::Object& self) {
	auto& state = this->getComponent<ext::PlayerInputBehavior::Metadata>();

	// reset state
	state.movement = {};
	state.look = {};
	state.magnitude = 1.0f;
	state.jump = false;
	state.crouch = false;
	state.run = false;
	state.walk = false;
	state.use = false;
	state.noclipToggle = false;
	state.menuToggle = false;

	// not in control
	if ( !state.control ) return;

	if ( uf::Window::focused) {
		if ( uf::inputs::kbm::states::W ) state.movement.y += 1.0f;
		if ( uf::inputs::kbm::states::S ) state.movement.y -= 1.0f;
		if ( uf::inputs::kbm::states::D ) state.movement.x += 1.0f;
		if ( uf::inputs::kbm::states::A ) state.movement.x -= 1.0f;

		state.run = uf::inputs::kbm::states::LShift;
		state.walk = uf::inputs::kbm::states::LAlt;
		state.jump = uf::inputs::kbm::states::Space;
		state.crouch = uf::inputs::kbm::states::LControl;
		state.menuToggle = uf::inputs::kbm::states::Escape;
		state.noclipToggle = uf::inputs::kbm::states::V;
		state.use = uf::inputs::kbm::states::E;
	}

	if ( spec::controller::connected() ) {
		float deadzone = 0.01f;
		auto stick = uf::inputs::controller::states::L_JOYSTICK;

		if ( abs(stick.x) > deadzone || abs(stick.y) > deadzone ) {
			state.movement.x = stick.x;
			state.movement.y = stick.y;
			state.magnitude = uf::vector::norm( stick );
		}

		if ( uf::inputs::controller::states::A ) state.jump = true;
		if ( uf::inputs::controller::states::B ) state.noclipToggle = true;
		if ( uf::inputs::controller::states::X ) { state.crouch = true; state.walk = true; }
		if ( uf::inputs::controller::states::Y ) state.use = true;
		if ( uf::inputs::controller::states::START ) state.menuToggle = true;
	}

	if ( state.movement.x != 0 && state.movement.y != 0 && state.magnitude == 1.0f ) {
		state.movement = uf::vector::normalize( state.movement );
	}

	{
		auto& scene = uf::scene::getCurrentScene();
		auto* menu = scene.globalFindByName("Gui: Menu");
		if ( !menu ) state.menu = "";
		if ( state.menu == "" && state.menuToggle ) {
			state.menu = "paused";
			state.control = false;

			pod::payloads::menuOpen payload;
			payload.name = "pause";
			uf::hooks.call("menu:Open", payload);
		} else {
			state.control = state.menu == "";
		}
	}

#if UF_INPUT_USE_ENUM_MOUSE && !UF_ENV_DREAMCAST
	const auto& mouseDelta = uf::inputs::kbm::states::Mouse;
	state.look.x += mouseDelta.x * ONE_OVER_SIXTY;
	state.look.y += mouseDelta.y * ONE_OVER_SIXTY;
#endif
}

void ext::PlayerInputBehavior::render(uf::Object& self) {}
void ext::PlayerInputBehavior::destroy(uf::Object& self) {}
void ext::PlayerInputBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ){
	serializer["system"]["control"] = control;
	serializer["system"]["menu"] = menu;
}
void ext::PlayerInputBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ){
	control = serializer["system"]["control"].as(control);
	menu = serializer["system"]["menu"].as(menu);
}
#undef this

#include <uf/ext/lua/component.h>
UF_LUA_REGISTER_USERTYPE_AND_COMPONENT(ext::PlayerInputBehavior::Metadata,
	UF_LUA_REGISTER_USERTYPE_MEMBER(ext::PlayerInputBehavior::Metadata::control),
	UF_LUA_REGISTER_USERTYPE_MEMBER(ext::PlayerInputBehavior::Metadata::menu),
	UF_LUA_REGISTER_USERTYPE_MEMBER(ext::PlayerInputBehavior::Metadata::movement),
	UF_LUA_REGISTER_USERTYPE_MEMBER(ext::PlayerInputBehavior::Metadata::look),
	UF_LUA_REGISTER_USERTYPE_MEMBER(ext::PlayerInputBehavior::Metadata::jump),
	UF_LUA_REGISTER_USERTYPE_MEMBER(ext::PlayerInputBehavior::Metadata::crouch),
	UF_LUA_REGISTER_USERTYPE_MEMBER(ext::PlayerInputBehavior::Metadata::run),
	UF_LUA_REGISTER_USERTYPE_MEMBER(ext::PlayerInputBehavior::Metadata::walk),
	UF_LUA_REGISTER_USERTYPE_MEMBER(ext::PlayerInputBehavior::Metadata::use),
	UF_LUA_REGISTER_USERTYPE_MEMBER(ext::PlayerInputBehavior::Metadata::magnitude)
)