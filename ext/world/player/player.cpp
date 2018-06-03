#include "player.h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/window/window.h>
#include <uf/gl/camera/camera.h>
#include <uf/utils/audio/audio.h>

#include "../gui/gui.h"
#include "../light/light.h"
#include "../world.h"


namespace {
	bool lockMouse = true;
	bool croutching = false;

	std::function<uf::Entity*(uf::Entity*, const std::string&)> findByName = [&]( uf::Entity* parent, const std::string& name ) {
		for ( uf::Entity* entity : parent->getChildren() ) {
			if ( entity->getName() == name ) return entity;
			uf::Entity* p = findByName(entity, name);
			if ( p ) return p;
		}
		return (uf::Entity*) NULL;
	};
	std::function<uf::Entity*(uf::Entity*, uint)> findByUid = [&]( uf::Entity* parent, uint id ) {
		for ( uf::Entity* entity : parent->getChildren() ) {
			if ( entity->getUid() == id ) return entity;
			uf::Entity* p = findByUid(entity, id);
			if ( p ) return p;
		}
		return (uf::Entity*) NULL;
	};
}

void ext::Player::initialize() {
	ext::Craeture::initialize();

	this->addComponent<uf::Camera>(); {
		pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
		transform = uf::transform::initialize(transform);
		this->getComponent<uf::Camera>().getTransform().reference = this->getComponentPointer<pod::Transform<>>();
	}
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
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
			this->getComponent<uf::Hooks>().addHook( "window:Resized", [&](const std::string& event)->std::string{
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
	this->getComponent<uf::Hooks>().addHook( "window:Mouse.Moved", [&](const std::string& event)->std::string{
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
		uf::Serializer& serializer = this->getComponent<uf::Serializer>();
		if ( delta.x != 0 ) {
			double current, minima, maxima; {
				current = serializer["camera"]["limit"]["current"][0] != Json::nullValue ? serializer["camera"]["limit"]["current"][0].asDouble() : NAN;
				minima = serializer["camera"]["limit"]["minima"][0] != Json::nullValue ? serializer["camera"]["limit"]["minima"][0].asDouble() : NAN;
				maxima = serializer["camera"]["limit"]["maxima"][0] != Json::nullValue ? serializer["camera"]["limit"]["maxima"][0].asDouble() : NAN;
			}
			if ( serializer["camera"]["invert"][0].asBool() ) relta.x *= -1;
			current += relta.x;
			if ( current != current || ( current < maxima && current > minima ) ) uf::transform::rotate( transform, transform.up, relta.x ), updateCamera = true; else current -= relta.x;
			if ( serializer["camera"]["limit"]["current"][0] != Json::nullValue ) serializer["camera"]["limit"]["current"][0] = current;
		}
		if ( delta.y != 0 ) {
			double current, minima, maxima; {
				current = serializer["camera"]["limit"]["current"][1] != Json::nullValue ? serializer["camera"]["limit"]["current"][1].asDouble() : NAN;
				minima = serializer["camera"]["limit"]["minima"][1] != Json::nullValue ? serializer["camera"]["limit"]["minima"][1].asDouble() : NAN;
				maxima = serializer["camera"]["limit"]["maxima"][1] != Json::nullValue ? serializer["camera"]["limit"]["maxima"][1].asDouble() : NAN;
			}
			if ( serializer["camera"]["invert"][1].asBool() ) relta.y *= -1;
			current += relta.y;
			if ( current != current || ( current < maxima && current > minima ) ) {
				uf::transform::rotate( cameraTransform, cameraTransform.right, relta.y );
				uf::transform::rotate( this->m_animation.transforms[serializer["animation"]["names"]["head"].asString()], {0, 0, 1}, -relta.y );
				updateCamera = true;
			} else current -= relta.y;
			if ( serializer["camera"]["limit"]["current"][1] != Json::nullValue ) serializer["camera"]["limit"]["current"][1] = current;
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
	this->getComponent<uf::Hooks>().addHook( "window:Mouse.Moved", [&](const std::string& event)->std::string{
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
	/*
		if ( delta.y != 0 ) {
			double current, minima, maxima; {
				current = serializer["camera"]["limit"]["current"][1] != Json::nullValue ? serializer["camera"]["limit"]["current"][1].asDouble() : NAN;
				minima = serializer["camera"]["limit"]["minima"][1] != Json::nullValue ? serializer["camera"]["limit"]["minima"][1].asDouble() : NAN;
				maxima = serializer["camera"]["limit"]["maxima"][1] != Json::nullValue ? serializer["camera"]["limit"]["maxima"][1].asDouble() : NAN;
			}
			if ( serializer["camera"]["invert"][1].asBool() ) relta.y *= -1;
			current += relta.y;
			std::cout << "Head: " << current << ", " << minima << ", " << maxima << std::endl;
			if ( current != current || ( current < maxima && current > minima ) ) uf::transform::rotate( this->m_animation.transforms[serializer["animation"]["names"]["head"].asString()], {0, 0, 1}, relta.y ), updateCamera = true;
		}
		if ( false && delta.y != 0 ) {
			static float limit = 0; limit += -relta.y;
			if ( limit < 1 && limit > -1 ) uf::transform::rotate( this->m_animation.transforms[serializer["animation"]["names"]["head"].asString()], {0, 0, 1}, -relta.y ), updateCamera = true;
			else limit -= -relta.y;
		}
	*/
		if ( ::lockMouse ) {
			uf::hooks.call("window:Mouse.Lock");
			ignoreNext = true;
		}
		return "true";
	});

	// Player Light
	if ( metadata["light"]["should"].asBool() ) {
		pod::Vector3 color = { 1, 1, 1 };
		int radius = 3;
		for ( int i = 0; i < radius; ++i ) {
			uf::Entity* entity = new ext::Light;
			if ( !((ext::Object*) entity)->load("./light/config.json") ) { uf::iostream << "Error loading `" << "./light/config.json" << "!" << "\n"; delete entity; return; }
			this->addChild(*entity);

			uf::Serializer& lMetadata = entity->getComponent<uf::Serializer>();
			lMetadata["light"] = metadata["light"];

			entity->initialize();

			pod::Transform<>& parent = this->getComponent<pod::Transform<>>();
			pod::Transform<>& transform = entity->getComponent<pod::Transform<>>();//entity->getComponent<uf::Camera>().getTransform();
			transform = uf::transform::initialize( transform );
			uf::transform::rotate( transform, transform.up, (360.0 / radius) * (3.1415926/180.0) * i );
			entity->getComponent<uf::Camera>().update(true);
		}
		/* down */ {
			uf::Entity* entity = new ext::Light;
			if ( !((ext::Object*) entity)->load("./light/config.json") ) { uf::iostream << "Error loading `" << "./light/config.json" << "!" << "\n"; delete entity; return; }
			this->addChild(*entity);

			uf::Serializer& lMetadata = entity->getComponent<uf::Serializer>();
			lMetadata["light"] = metadata["light"];

			entity->initialize();

			pod::Transform<>& parent = this->getComponent<pod::Transform<>>();
			pod::Transform<>& transform = entity->getComponent<pod::Transform<>>();//entity->getComponent<uf::Camera>().getTransform();
			transform = uf::transform::initialize( transform );
			uf::transform::rotate( transform, transform.right, 1.5708 * 1 );
			entity->getComponent<uf::Camera>().setFov(120);
			entity->getComponent<uf::Camera>().update(true);
		}
	}
}
#include <sstream>
void ext::Player::tick() {
	bool updateCamera = true;
	bool deltaCrouch = false;
	bool walking = false;
	struct {
		float move = 4; //uf::physics::time::delta * 4;
		float rotate = uf::physics::time::delta * 0.75;
		float limitSquared = 4*4;
	} speed;

	struct {
		bool running;
		bool light;
	} keys = {
		.running = uf::Window::isKeyPressed("LShift"),
		.light = uf::Window::isKeyPressed("F"),
	};

	uf::Camera& camera = this->getComponent<uf::Camera>();
	pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
	pod::Physics& physics = this->getComponent<pod::Physics>();
	uf::Serializer& serializer = this->getComponent<uf::Serializer>();

	if (keys.running) speed.move = 8;

	speed.limitSquared = speed.move * speed.move;

	/* Update light positions */ if ( this->getComponent<uf::Serializer>()["light"]["should"].asBool() ) {
		pod::Transform<>& parent = this->getComponent<pod::Transform<>>();
		pod::Transform<>& camera = this->getComponent<uf::Camera>().getTransform();

		uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	//	if ( metadata["light"]["battery"] == Json::nullValue ) {
			if ( metadata["light"]["battery"]["level"] == Json::nullValue ) metadata["light"]["battery"]["level"] = 1.0;
			if ( metadata["light"]["battery"]["rate"] == Json::nullValue ) metadata["light"]["battery"]["rate"] = 0.1;
			if ( metadata["light"]["battery"]["depleted"] == Json::nullValue ) metadata["light"]["battery"]["depleted"] = false;
			if ( metadata["light"]["battery"]["attenuation"]["from"] == Json::nullValue ) metadata["light"]["battery"]["attenuation"]["from"] = 0.05;
			if ( metadata["light"]["battery"]["attenuation"]["to"] == Json::nullValue ) metadata["light"]["battery"]["attenuation"]["to"] = 0.0005;
	//	}
		struct {
			float level;
			float rate;
			bool depleted;
			float from;
			float to;
		} battery = {
			.level = metadata["light"]["battery"]["level"].asFloat(),
			.rate = metadata["light"]["battery"]["rate"].asFloat(),
			.depleted = metadata["light"]["battery"]["depleted"].asBool(),
		};

		/* Calculate ""battery"" */ {
			if ( battery.level < 0 ) {
				battery.depleted = true;
			}
			if ( keys.light && !battery.depleted ) {
				battery.level -= battery.rate * uf::physics::time::delta;
			} else if ( battery.level < 1 ) {
				battery.level += battery.rate * uf::physics::time::delta;
				if ( battery.level >= 1 ) {
					battery.level = 1;
					battery.depleted = false;
				}
			}
			metadata["light"]["battery"]["level"] = battery.level;
			metadata["light"]["battery"]["rate"] = battery.rate;
			metadata["light"]["battery"]["depleted"] = battery.depleted;
		}
		/* Light Element */ {
			uf::Serializer& metadata = this->getComponent<uf::Serializer>();
			uint uid = metadata["gui"]["Battery Element"].asUInt();
			uf::Entity& parent = this->getRootParent<ext::World>();		
			ext::Gui* gui = (ext::Gui*) ::findByName(&parent, "Gui Manager");
			uf::Entity* element = ::findByUid(gui, uid);
			if ( keys.light ) {
				if ( !element ) {
					std::string config = "./gui/text/config.json";
					element = new ext::Gui;
					ext::Gui& guiElement = *((ext::Gui*) element);
					if ( !guiElement.load(config) ) { uf::iostream << "Error loading `" << config << "!" << "\n"; delete element; }
					else {
						gui->addChild(*element);
						uf::Serializer& gMetadata = element->getComponent<uf::Serializer>();
						gMetadata["type"] = "Battery Element";
						gMetadata["gui"]["text"] = "Battery: ?%";
						gMetadata["gui"]["position"][0] = 0.02;
						gMetadata["gui"]["position"][1] = 0.8;
						gMetadata["gui"]["origin"] = "bottom";
						gMetadata["gui"]["direction"] = "up";
						element->initialize();
						metadata["gui"]["Battery Element"] = element->getUid();
					}
				} else {
					std::string string = "?"; {
						std::stringstream sstream; sstream.precision(0);
						sstream << std::fixed << battery.level * 100;
						string = sstream.str();
					}
					uf::Serializer& gMetadata = element->getComponent<uf::Serializer>();
					gMetadata["gui"]["render"] = true;
					gMetadata["gui"]["text"] = "Battery: " + string + "%";
				}
			} else {
				if ( element ) {
					uf::Serializer& gMetadata = element->getComponent<uf::Serializer>();
					gMetadata["gui"]["render"] = false;
				}
			}
		}
		for ( uf::Entity* entity : this->getChildren() ) { if ( entity->getName() != "Light" ) continue;
			pod::Transform<>& transform = entity->getComponent<pod::Transform<>>();
			transform.position = parent.position + camera.position + pod::Vector3{0, 1, 0};
			
			uf::Serializer& lMetadata = entity->getComponent<uf::Serializer>();
			lMetadata["light"]["render"] = battery.depleted ? false : keys.light;
		/*
			lMetadata["light"]["color"][0] = battery;
			lMetadata["light"]["color"][1] = battery;
			lMetadata["light"]["color"][2] = battery;
		*/
			float atten = 0.05; {
				atten = uf::vector::lerp( pod::Vector1{ battery.from }, pod::Vector1{ battery.to }, battery.level ).x;
			}
			lMetadata["light"]["attenuation"] = atten;
		}
	}

	/* Running Element */ {
		uf::Serializer& metadata = this->getComponent<uf::Serializer>();
		uint uid = metadata["gui"]["Sprinting Element"].asUInt();
		uf::Entity& parent = this->getRootParent<ext::World>();		
		ext::Gui* gui = (ext::Gui*) ::findByName(&parent, "Gui Manager");
		uf::Entity* element = ::findByUid(gui, uid);

		if ( keys.running ) {
			if ( !element ) {
				std::string config = "./gui/text/config.json";
				element = new ext::Gui;
				ext::Gui& guiElement = *((ext::Gui*) element);
				if ( !guiElement.load(config) ) { uf::iostream << "Error loading `" << config << "!" << "\n"; delete element; }
				else {
					gui->addChild(*element);

					uf::Serializer& gMetadata = element->getComponent<uf::Serializer>();
					
					gMetadata["type"] = "Sprinting Element";
					gMetadata["gui"]["text"] = "Sprint";
					gMetadata["gui"]["position"][0] = 0.02;
					gMetadata["gui"]["position"][1] = 0.02;
					gMetadata["gui"]["origin"] = "bottom";
					gMetadata["gui"]["direction"] = "up";
				/*
					gMetadata["gui"]["size"] = 32;
					gMetadata["gui"]["scale"] = 1.5;
					gMetadata["gui"]["shadowbox"]["color"][0] = 0.05;
					gMetadata["gui"]["shadowbox"]["color"][1] = 0.05;
					gMetadata["gui"]["shadowbox"]["color"][2] = 0.05;
					gMetadata["gui"]["shadowbox"]["padding"]["left"] = 5;
					gMetadata["gui"]["shadowbox"]["padding"]["right"] = 5;
					gMetadata["gui"]["shadowbox"]["padding"]["bottom"] = 5;
					gMetadata["gui"]["shadowbox"]["padding"]["top"] = 5;
					gMetadata["gui"]["shadowbox"]["ignore"] = true;
				*/
					element->initialize();
					
					metadata["gui"]["Sprinting Element"] = element->getUid();
				}
			} else {
				uf::Serializer& gMetadata = element->getComponent<uf::Serializer>();
				gMetadata["gui"]["render"] = true;
			}
		} else {
			if ( element ) {
				uf::Serializer& gMetadata = element->getComponent<uf::Serializer>();
				gMetadata["gui"]["render"] = false;
			}
		}
	}

	bool floored = physics.linear.velocity.y == 0;
	if ( ::lockMouse ) {	
	/*
		if (uf::Window::isKeyPressed("W")) uf::transform::move( transform, transform.forward, speed.move ), updateCamera = walking = true;
		if (uf::Window::isKeyPressed("S")) uf::transform::move( transform, transform.forward, -speed.move ), updateCamera = walking = true;
		if (uf::Window::isKeyPressed("A")) uf::transform::move( transform, transform.right, -speed.move ), updateCamera = walking = true;
		if (uf::Window::isKeyPressed("D")) uf::transform::move( transform, transform.right, speed.move ), updateCamera = walking = true;
	*/
		if ( true || floored ) {
			if (uf::Window::isKeyPressed("W")) {
				float mag = uf::vector::magnitude(physics.linear.velocity * pod::Vector3{1, 0, 1});
				if ( mag < speed.limitSquared ) {
					physics.linear.velocity += transform.forward * speed.move;
					mag = uf::vector::magnitude(physics.linear.velocity);
				} else mag = speed.limitSquared;
				pod::Vector3 correction = transform.forward * sqrt(mag);
				physics.linear.velocity.x = correction.x;
				physics.linear.velocity.z = correction.z;
				updateCamera = walking = true;
			}
			if (uf::Window::isKeyPressed("S")) {
				float mag = uf::vector::magnitude(physics.linear.velocity * pod::Vector3{1, 0, 1});
				if ( mag < speed.limitSquared ) {
					physics.linear.velocity += transform.forward * -speed.move;
					mag = uf::vector::magnitude(physics.linear.velocity);
				} else mag = speed.limitSquared;
				pod::Vector3 correction = transform.forward * -sqrt(mag);
				physics.linear.velocity.x = correction.x;
				physics.linear.velocity.z = correction.z;
				updateCamera = walking = true;
			}
			if (uf::Window::isKeyPressed("A")) {
				float mag = uf::vector::magnitude(physics.linear.velocity * pod::Vector3{1, 0, 1});
				if ( mag < speed.limitSquared ) {
					physics.linear.velocity += transform.right * -speed.move;
					mag = uf::vector::magnitude(physics.linear.velocity);
				} else mag = speed.limitSquared;
				pod::Vector3 correction = transform.right * -sqrt(mag);
				physics.linear.velocity.x = correction.x;
				physics.linear.velocity.z = correction.z;
				updateCamera = walking = true;
			}
			if (uf::Window::isKeyPressed("D")) {
				float mag = uf::vector::magnitude(physics.linear.velocity * pod::Vector3{1, 0, 1});
				if ( mag < speed.limitSquared ) {
					physics.linear.velocity += transform.right * speed.move;
					mag = uf::vector::magnitude(physics.linear.velocity);
				} else mag = speed.limitSquared;
				pod::Vector3 correction = transform.right * sqrt(mag);
				physics.linear.velocity.x = correction.x;
				physics.linear.velocity.z = correction.z;
				updateCamera = walking = true;
			}
			if ( floored && uf::Window::isKeyPressed(" ") ) {
				if ( serializer["collision"]["jump"] != Json::nullValue ) {
					if ( serializer["collision"]["jump"][0].asFloat() != 0 ) physics.linear.velocity.x = serializer["collision"]["jump"][0].asFloat();
					if ( serializer["collision"]["jump"][1].asFloat() != 0 ) physics.linear.velocity.y = serializer["collision"]["jump"][1].asFloat();
					if ( serializer["collision"]["jump"][2].asFloat() != 0 ) physics.linear.velocity.z = serializer["collision"]["jump"][2].asFloat();
				}
			}
		}

		if (uf::Window::isKeyPressed("Left")) {
			uf::transform::rotate( transform, transform.up, -speed.rotate ), updateCamera = true;
		}
		if (uf::Window::isKeyPressed("Right")) {
			uf::transform::rotate( transform, transform.up, speed.rotate ), updateCamera = true;
		}
		
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

	if ( floored ) {
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
		} else {
			physics.linear.velocity.x = 0;
			physics.linear.velocity.y = 0;
			physics.linear.velocity.z = 0;
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

	camera.update(true);

	ext::Craeture::tick();
}
void ext::Player::render() {
	uf::Entity::render();

	ext::World& parent = this->getRootParent<ext::World>();
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	if ( &parent.getCamera() != this->getComponentPointer<uf::Camera>() || metadata["camera"]["ortho"].asBool() || metadata["model"]["render"].asBool() ) {
		ext::Craeture::render();
	}
}