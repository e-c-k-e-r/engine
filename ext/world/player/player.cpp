#include "player.h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/window/window.h>
#include <uf/gl/camera/camera.h>
#include <uf/utils/audio/audio.h>

namespace {
	bool lockMouse = true;
	bool croutching = false;
}

void ext::Player::initialize() {
	ext::Craeture::initialize();

	this->addComponent<uf::Camera>(); {
		pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
		transform = uf::transform::initialize(transform);
		this->getComponent<uf::Camera>().getTransform().reference = this->getComponentPointer<pod::Transform<>>();
	}
	/* Load Config */ {
		struct {
			int mode = 0;
			struct {
				pod::Vector2 lr, bt, nf;
			} ortho;
			struct {
				pod::Math::num_t fov = 120;
				pod::Vector2 size = {640, 480};
				pod::Vector2 bounds = {0.5, 128.0};
			} perspective;
			pod::Vector3 offset = {0, 0, 0};
		} settings;
		uf::Serializer& metadata = this->getComponent<uf::Serializer>();

		uf::Camera& camera = this->getComponent<uf::Camera>();
		settings.mode = metadata["camera"]["ortho"].asBool() ? -1 : 1;
		settings.perspective.size.x = metadata["camera"]["settings"]["size"]["x"].asDouble();
		settings.perspective.size.y = metadata["camera"]["settings"]["size"]["y"].asDouble();
		camera.setSize(settings.perspective.size);
		if ( settings.mode < 0 ) {
			settings.ortho.lr.x 	= metadata["camera"]["settings"]["left"].asDouble();
			settings.ortho.lr.y 	= metadata["camera"]["settings"]["right"].asDouble();
			settings.ortho.bt.x 	= metadata["camera"]["settings"]["bottom"].asDouble();
			settings.ortho.bt.y 	= metadata["camera"]["settings"]["top"].asDouble();
			settings.ortho.nf.x 	= metadata["camera"]["settings"]["near"].asDouble();
			settings.ortho.nf.y 	= metadata["camera"]["settings"]["far"].asDouble();

			camera.ortho( settings.ortho.lr, settings.ortho.bt, settings.ortho.nf );
		} else {
			settings.perspective.fov 		= metadata["camera"]["settings"]["fov"].asDouble();
			settings.perspective.bounds.x 	= metadata["camera"]["settings"]["clip"]["near"].asDouble();
			settings.perspective.bounds.y 	= metadata["camera"]["settings"]["clip"]["far"].asDouble();

			camera.setFov(settings.perspective.fov);
			camera.setBounds(settings.perspective.bounds);
	
		}

		settings.offset.x = metadata["camera"]["offset"][0].asDouble();
		settings.offset.y = metadata["camera"]["offset"][1].asDouble();
		settings.offset.z = metadata["camera"]["offset"][2].asDouble();

		pod::Transform<>& transform = camera.getTransform();
		/* Transform initialization */ {
			transform.position.x = metadata["camera"]["position"][0].asDouble();
			transform.position.y = metadata["camera"]["position"][1].asDouble();
			transform.position.z = metadata["camera"]["position"][2].asDouble();
		}

		camera.setOffset(settings.offset);
		camera.update(true);

		// Update viewport
		if ( metadata["camera"]["settings"]["size"]["auto"].asBool() )  {
			uf::hooks.addHook( "window:Resized", [&](const std::string& event)->std::string{
				uf::Serializer json = event;

				// Update persistent window sized (size stored to JSON file)
				pod::Vector2ui size; {
					size.x = json["window"]["size"]["x"].asUInt64();
					size.y = json["window"]["size"]["y"].asUInt64();
				}
				/* Update camera's viewport */ {
					uf::Camera& camera = this->getComponent<uf::Camera>();
					camera.setSize({(pod::Math::num_t)size.x, (pod::Math::num_t)size.y});
				}

				return "true";
			} );
		}
	}

	// Rotate Camera
	uf::hooks.addHook( "window:Mouse.Moved", [&](const std::string& event)->std::string{
		static bool ignoreNext = false; if ( ignoreNext ) { ignoreNext = false; return "true"; }
		
		uf::Serializer json = event;
	
		pod::Vector2i delta = { json["mouse"]["delta"]["x"].asInt(), json["mouse"]["delta"]["y"].asInt() };
		pod::Vector2i size  = { json["mouse"]["size"]["x"].asInt(),  json["mouse"]["size"]["y"].asInt() };
		pod::Vector2 relta  = { (float) delta.x / size.x, (float) delta.y / size.y };
		if ( delta.x == 0 && delta.y == 0 ) return "true";
		if ( !::lockMouse ) return "true";

		bool updateCamera = false;
		uf::Camera& camera = this->getComponent<uf::Camera>();
		pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
		pod::Transform<>& cameraTransform = camera.getTransform();
		if ( delta.x != 0 ) uf::transform::rotate( transform, transform.up,                relta.x ), updateCamera = true;
		if ( delta.y != 0 ) {
			static float limit = 0; limit += -relta.y;
			if ( limit < 1 && limit > -1.2 ) uf::transform::rotate( cameraTransform, cameraTransform.right, relta.y ), updateCamera = true;
			else limit -= -relta.y;
		}
		if ( updateCamera ) {
			camera.updateView();
		}
		if ( ::lockMouse ) {
			uf::hooks.call("window:Mouse.Lock");
			ignoreNext = true;
		}
		return "true";
	});
	// Rotate model
	uf::hooks.addHook( "window:Mouse.Moved", [&](const std::string& event)->std::string{
		static bool ignoreNext = false; if ( ignoreNext ) { ignoreNext = false; return "true"; }
		
		uf::Serializer json = event;
	
		pod::Vector2i delta = { json["mouse"]["delta"]["x"].asInt(), json["mouse"]["delta"]["y"].asInt() };
		pod::Vector2i size  = { json["mouse"]["size"]["x"].asInt(),  json["mouse"]["size"]["y"].asInt() };
		pod::Vector2 relta  = { (float) delta.x / size.x, (float) delta.y / size.y };
		uf::Serializer& serializer = this->getComponent<uf::Serializer>();
		if ( !::lockMouse ) return "true";
		if ( !serializer["animation"]["status"]["turn"].asBool() ) {
			if ( delta.x < 0 ) {
				float leftLegDirection = serializer["animation"]["states"]["leftLeg"]["direction"].asFloat();
				float rightLegDirection = serializer["animation"]["states"]["rightLeg"]["direction"].asFloat();

				if ( rightLegDirection < 0 ) serializer["animation"]["states"]["rightLeg"]["direction"] = rightLegDirection * -1;
				if ( leftLegDirection > 0 ) serializer["animation"]["states"]["leftLeg"]["direction"] = leftLegDirection * -1;
			} else if ( delta.x > 0 ) {
				float leftLegDirection = serializer["animation"]["states"]["leftLeg"]["direction"].asFloat();
				float rightLegDirection = serializer["animation"]["states"]["rightLeg"]["direction"].asFloat();

				if ( rightLegDirection > 0 ) serializer["animation"]["states"]["rightLeg"]["direction"] = rightLegDirection * -1;
				if ( leftLegDirection < 0 ) serializer["animation"]["states"]["leftLeg"]["direction"] = leftLegDirection * -1;
			}
		}
		serializer["animation"]["status"]["turn"] = delta.x != 0;
		if ( delta.x == 0 && delta.y == 0 ) return "true";

		bool updateCamera = false;
		uf::Camera& camera = this->getComponent<uf::Camera>();
		pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
		pod::Transform<>& cameraTransform = camera.getTransform();
		if ( delta.y != 0 ) {
			static float limit = 0; limit += -relta.y;
			if ( limit < 1 && limit > -1 ) uf::transform::rotate( this->m_animation.transforms[serializer["animation"]["names"]["head"].asString()], {0, 0, 1}, -relta.y ), updateCamera = true;
			else limit -= -relta.y;
		}
		if ( ::lockMouse ) {
			uf::hooks.call("window:Mouse.Lock");
			ignoreNext = true;
		}
		return "true";
	});
}
void ext::Player::tick() {
	bool updateCamera = true;
	bool deltaCrouch = false;
	bool walking = false;
	struct {
		float move = uf::physics::time::delta * 4;
		float rotate = uf::physics::time::delta * 4;
	} speed;

	uf::Camera& camera = this->getComponent<uf::Camera>();
	pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
	uf::Serializer& serializer = this->getComponent<uf::Serializer>();

	if ( ::lockMouse ) {	
		if (uf::Window::isKeyPressed("W")) uf::transform::move( transform, transform.forward, speed.move ), updateCamera = walking = true;
		if (uf::Window::isKeyPressed("S")) uf::transform::move( transform, transform.forward, -speed.move ), updateCamera = walking = true;
		if (uf::Window::isKeyPressed("A")) uf::transform::move( transform, transform.right, -speed.move ), updateCamera = walking = true;
		if (uf::Window::isKeyPressed("D")) uf::transform::move( transform, transform.right, speed.move ), updateCamera = walking = true;
		
		if (uf::Window::isKeyPressed("Left")) uf::transform::rotate( transform, transform.up, -speed.rotate ), updateCamera = true;
		if (uf::Window::isKeyPressed("Right")) uf::transform::rotate( transform, transform.up, speed.rotate ), updateCamera = true;
		
		if (uf::Window::isKeyPressed("LControl")) {
			if ( !::croutching )  deltaCrouch = true;
			::croutching = true;
		} else {
			if ( ::croutching ) deltaCrouch = true;
			::croutching = false;
		}	
	}
	if ( deltaCrouch ) {
		float delta = 1.0f;
		if ( ::croutching ) camera.getTransform().position.y -= delta;
		else camera.getTransform().position.y += delta;
		updateCamera = true;
	}
	if ( updateCamera ) camera.updateView();
	if ( walking ) {
		serializer["animation"]["status"]["walk"] = true;
	} else {
		serializer["animation"]["status"]["walk"] = false;
		serializer["animation"]["status"]["rest"] = true;
	}

	if ( walking ) {
		uf::SoundEmitter& emitter = this->getComponent<uf::SoundEmitter>();
		int cycle = rand() % serializer["audio"]["footsteps"].size();
		std::string filename = serializer["audio"]["footsteps"][cycle].asString();
		uf::Audio& footstep = emitter.add(filename);

		bool playing = false;
		for ( uint i = 0; i < serializer["audio"]["footsteps"].size(); ++i ) {
			uf::Audio& audio = emitter.add(serializer["audio"]["footsteps"][i].asString());
			if ( audio.playing() ) playing = true;
		}
		if ( !playing ) {
			footstep.play();
			footstep.setPosition( transform.position );
		}
	}

	/* Lock Mouse */ {
		static uf::Timer<long long> timer(false);
		if ( !timer.running() ) timer.start();

		if ( uf::Window::isKeyPressed("Escape") && timer.elapsed().asDouble() >= 1 ) {
			::lockMouse = !::lockMouse;
			timer.reset();
		}
	}

	ext::Craeture::tick();
}
#include "../world.h"
void ext::Player::render() {
	ext::World& parent = this->getRootParent<ext::World>();
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	if ( &parent.getCamera() != this->getComponentPointer<uf::Camera>() || metadata["camera"]["ortho"].asBool() || metadata["model"]["render"].asBool() ) ext::Craeture::render();
}