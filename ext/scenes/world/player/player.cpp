#include "player.h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/window/window.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/audio/audio.h>
#include <uf/ext/openvr/openvr.h>

#include <sstream>

#include "../gui/pause.h"
#include "../gui/battle.h"
#include "..//.h"
#include "../world.h"

namespace {
	ext::World* world;

	uf::Serializer masterTableGet( const std::string& table ) {
		if ( !world ) world = (ext::World*) uf::Entity::globalFindByName("World");
		if ( !world ) return std::string();
		uf::Serializer& mastertable = world->getComponent<uf::Serializer>();
		return mastertable["system"]["mastertable"][table];
	}
	uf::Serializer masterDataGet( const std::string& table, const std::string& key ) {
		if ( !world ) world = (ext::World*) uf::Entity::globalFindByName("World");
		if ( !world ) return std::string();
		uf::Serializer& mastertable = world->getComponent<uf::Serializer>();
		return mastertable["system"]["mastertable"][table][key];
	}
	inline int64_t parseInt( const std::string& str ) {
		return atoi(str.c_str());
	}

	inline std::string colorString( const std::string& hex ) {
		return "%#" + hex + "%";
	}
}

EXT_OBJECT_REGISTER_CPP(Player)

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
			this->addHook( "window:Resized", [&](const std::string& event)->std::string{
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

	metadata["system"]["control"] = true;
	
	this->addHook( "window:Mouse.CursorVisibility", [&](const std::string& event)->std::string{
		uf::Serializer json = event;
		metadata["system"]["control"] = !json["state"].asBool();	
		return "true";
	});

	// Rotate Camera
	this->addHook( "window:Mouse.Moved", [&](const std::string& event)->std::string{		
		uf::Serializer json = event;

		// discard events sent by os, only trust client now
		if ( json["invoker"] != "client" ) return "true";

		pod::Vector2i delta = { json["mouse"]["delta"]["x"].asInt(), json["mouse"]["delta"]["y"].asInt() };
		pod::Vector2i size  = { json["mouse"]["size"]["x"].asInt(),  json["mouse"]["size"]["y"].asInt() };
		pod::Vector2 relta  = { (float) delta.x / size.x, (float) delta.y / size.y };
		relta *= 2;
		if ( delta.x == 0 && delta.y == 0 ) return "true";
		if ( !metadata["system"]["control"].asBool() ) return "true";

		bool updateCamera = false;
		uf::Camera& camera = this->getComponent<uf::Camera>();
		pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
		pod::Transform<>& cameraTransform = camera.getTransform();
		if ( delta.x != 0 ) {
			double current, minima, maxima; {
				current = metadata["camera"]["limit"]["current"][0] != Json::nullValue ? metadata["camera"]["limit"]["current"][0].asDouble() : NAN;
				minima = metadata["camera"]["limit"]["minima"][0] != Json::nullValue ? metadata["camera"]["limit"]["minima"][0].asDouble() : NAN;
				maxima = metadata["camera"]["limit"]["maxima"][0] != Json::nullValue ? metadata["camera"]["limit"]["maxima"][0].asDouble() : NAN;
			}
			if ( metadata["camera"]["invert"][0].asBool() ) relta.x *= -1;
			current += relta.x;
			if ( current != current || ( current < maxima && current > minima ) ) uf::transform::rotate( transform, transform.up, relta.x ), updateCamera = true; else current -= relta.x;
			if ( metadata["camera"]["limit"]["current"][0] != Json::nullValue ) metadata["camera"]["limit"]["current"][0] = current;
		}
		if ( delta.y != 0 ) {
			double current, minima, maxima; {
				current = metadata["camera"]["limit"]["current"][1] != Json::nullValue ? metadata["camera"]["limit"]["current"][1].asDouble() : NAN;
				minima = metadata["camera"]["limit"]["minima"][1] != Json::nullValue ? metadata["camera"]["limit"]["minima"][1].asDouble() : NAN;
				maxima = metadata["camera"]["limit"]["maxima"][1] != Json::nullValue ? metadata["camera"]["limit"]["maxima"][1].asDouble() : NAN;
			}
			if ( metadata["camera"]["invert"][1].asBool() ) relta.y *= -1;
			current += relta.y;
			if ( current != current || ( current < maxima && current > minima ) ) {
				uf::transform::rotate( cameraTransform, cameraTransform.right, relta.y );
			//	uf::transform::rotate( this->m_animation.transforms[metadata["animation"]["names"]["head"].asString()], {0, 0, 0}, -relta.y );
				updateCamera = true;
			} else current -= relta.y;
			if ( metadata["camera"]["limit"]["current"][1] != Json::nullValue ) metadata["camera"]["limit"]["current"][1] = current;
		}
		if ( updateCamera ) {
			camera.updateView();
		}
		return "true";
	});

	this->addHook( ":Update.%UID%", [&](const std::string& event)->std::string{
		uf::Serializer json = event;
		
		for ( auto& member : json[""]["transients"] ) {
			if ( member["type"] != "player" ) continue;
			std::string id = member["id"].asString();
			metadata[""]["transients"][id]["hp"] = member["hp"];
			metadata[""]["transients"][id]["mp"] = member["mp"];
		}
		
		return "true";
	});

	// handle after battles and establishes cooldowns
	this->addHook( "world:Battle.End.%UID%", [&](const std::string& event)->std::string{		
		uf::Serializer json = event;

		// update
		{
			uf::Serializer payload;
			payload[""]["transients"] = json["battle"]["transients"];
			this->callHook( ":Update.%UID%", payload );
		}
		
		metadata["system"].removeMember("battle");
		metadata["system"]["cooldown"] = uf::physics::time::current + 5;

		return "true";
	});
	
	// detect collision against transients, engage in battle
	this->addHook( "world:Collision.%UID%", [&](const std::string& event)->std::string{	
		if ( metadata["system"]["cooldown"].asFloat() > uf::physics::time::current ) return "false";
		if ( !metadata["system"]["control"].asBool() ) return "false";

		uf::Serializer json = event;
		
		std::string state = metadata["system"]["state"].asString();
		if ( state != "" && state != "null" ) return "false";

		ext::World& world = this->getRootParent<ext::World>();
		uf::Entity* entity = world.findByUid(json["entity"].asUInt64());

		if ( !entity ) return "false";

		uf::Serializer& pMetadata = entity->getComponent<uf::Serializer>();

		std::string onCollision = pMetadata["system"]["onCollision"].asString();

		if ( onCollision == "battle" ) {
			uf::Serializer payload;
			payload["battle"]["enemy"] = pMetadata[""];
			payload["battle"]["enemy"]["uid"] = json["entity"].asUInt64();
			payload["battle"]["player"] = metadata[""];
			payload["battle"]["player"]["uid"] = this->getUid();
			payload["battle"]["music"] = pMetadata["music"];
			
			this->callHook("world:Battle.Start", payload);
		} else if ( onCollision == "npc" ) {
			if ( !uf::Window::isKeyPressed("E") ) return "false";
			uf::Serializer payload;
			payload["dialogue"] = pMetadata["dialogue"];
			payload["uid"] = json["entity"].asUInt64();
			this->callHook("menu:Dialogue.Start", payload);
		}
	//	metadata["system"]["cooldown"] = uf::physics::time::current + 5;
		metadata["system"]["control"] = false;
		metadata["system"]["menu"] = onCollision;
		return "true";
	});
	this->addHook( "world:Battle.End", [&](const std::string& event)->std::string{
		uf::Serializer json = event;
	
		metadata["system"]["menu"] = "";
		metadata["system"]["cooldown"] = uf::physics::time::current + 5;

		// update
		{
			uf::Serializer payload;
			payload[""]["transients"] = json["battle"]["transients"];
			this->callHook( ":Update.%UID%", payload );
		}

		return "true";
	});
	this->addHook( "menu:Dialogue.End", [&](const std::string& event)->std::string{
		uf::Serializer json = event;
	
		metadata["system"]["menu"] = "";
		metadata["system"]["cooldown"] = uf::physics::time::current + 1;

		return "true";
	});

	// Discord Integration
	this->addHook( "discord.Activity.Update.%UID%", [&](const std::string& event)->std::string{
		uf::Serializer payload;

		std::string leaderId = metadata[""]["party"][0].asString();
		uf::Serializer cardData = masterDataGet("Card", leaderId);
		uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].asString());
		std::string leader = charaData["name"].asString();

		payload["details"] = "Leader: " + leader;
		uf::hooks.call( "discord:Activity.Update", payload );

		return "true";
	});
	this->queueHook("discord.Activity.Update.%UID%", "", 1.0);
}
void ext::Player::tick() {
	uf::Camera& camera = this->getComponent<uf::Camera>();
	pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
	pod::Physics& physics = this->getComponent<pod::Physics>();
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	ext::World& world = this->getRootParent<ext::World>();

	struct {
		bool running = uf::Window::isKeyPressed("LShift");
		bool light = uf::Window::isKeyPressed("F");
		bool jump = uf::Window::isKeyPressed(" ");
		bool forward = uf::Window::isKeyPressed("W");
		bool backwards = uf::Window::isKeyPressed("S");
		bool left = uf::Window::isKeyPressed("A");
		bool right = uf::Window::isKeyPressed("D");
		bool lookLeft = uf::Window::isKeyPressed("Left");
		bool lookRight = uf::Window::isKeyPressed("Right");
		bool crouch = uf::Window::isKeyPressed("LControl");
		bool paused = uf::Window::isKeyPressed("Escape");
		bool aux = uf::Window::isKeyPressed("F");
		bool vee = uf::Window::isKeyPressed("V");
		bool walk = uf::Window::isKeyPressed("LAlt");
	} keys;

	if ( ext::openvr::context ) {
		if ( ext::openvr::controllerState( vr::Controller_Hand::Hand_Right, "dpadUp" )["state"].asBool() ) keys.forward = true;
		if ( ext::openvr::controllerState( vr::Controller_Hand::Hand_Right, "dpadDown" )["state"].asBool() ) keys.backwards = true;
		if ( ext::openvr::controllerState( vr::Controller_Hand::Hand_Right, "dpadLeft" )["state"].asBool() ) keys.lookLeft = true; //keys.left = true;
		if ( ext::openvr::controllerState( vr::Controller_Hand::Hand_Right, "dpadRight" )["state"].asBool() ) keys.lookRight = true; //keys.right = true;

	//	if ( ext::openvr::controllerState( vr::Controller_Hand::Hand_Left, "dpadUp" )["state"].asBool() ) keys.forward = true;
	//	if ( ext::openvr::controllerState( vr::Controller_Hand::Hand_Left, "dpadDown" )["state"].asBool() ) keys.backwards = true;
		if ( ext::openvr::controllerState( vr::Controller_Hand::Hand_Left, "dpadLeft" )["state"].asBool() ) keys.lookLeft = true;
		if ( ext::openvr::controllerState( vr::Controller_Hand::Hand_Left, "dpadRight" )["state"].asBool() ) keys.lookRight = true;
	//	std::cout << ext::openvr::controllerState( vr::Controller_Hand::Hand_Right ) << std::endl;
	}
	
	struct {
		bool updateCamera = true;
		bool deltaCrouch = false;
		bool walking = false;
		bool floored = true;
		std::string menu = "";
	} stats;
	stats.floored = physics.linear.velocity.y == 0;
	stats.menu = metadata["system"]["menu"].asString();

	struct {
		float move = 4; //uf::physics::time::delta * 4;
		float rotate = uf::physics::time::delta * 1.25f;
		float limitSquared = 4*4;
	} speed;
	static uf::Timer<long long> timer(false);
	if ( !timer.running() ) timer.start();
	if ( keys.vee ) {
		if ( timer.elapsed().asDouble() >= 0.25 ) {
			timer.reset();
			metadata["collision"]["should"] = !metadata["collision"]["should"].asBool();

			physics.linear.velocity = {0,0,0};
		}
	}
	if ( keys.running ) speed.move *= 2;
	else if ( keys.walk ) speed.move /= 4;
	speed.limitSquared = speed.move * speed.move;

	uf::Object* menu = (uf::Object*) this->getRootParent().findByName("Gui: Menu");
	if ( !menu ) stats.menu = "";
	
	// make assumptions
	if ( stats.menu == "" && keys.paused ) {
		stats.menu = "paused";
		metadata["system"]["control"] = false;
		uf::hooks.call("menu:Pause");
	}
	else if ( !metadata["system"]["control"].asBool() ) {
		stats.menu = "menu";
	} else if ( stats.menu == "" ) {
		metadata["system"]["control"] = true;
	} else {
		metadata["system"]["control"] = false;
	}
	metadata["system"]["menu"] = stats.menu;
	
	if ( metadata["system"]["control"].asBool() ) {	
		if ( stats.floored ) {
			pod::Transform<> translator = transform;
			if ( ext::openvr::context ) {
			//	translator.orientation = uf::quaternion::multiply( transform.orientation * pod::Vector4f{1,1,1,-1}, ext::openvr::hmdQuaternion() * pod::Vector4f{1,1,1,-1} );
			//	translator.orientation = uf::quaternion::multiply( ext::openvr::hmdQuaternion(), transform.orientation );
				//translator.orientation = ext::openvr::hmdQuaternion();
				bool useController = true;
				translator.orientation = uf::quaternion::multiply( transform.orientation * pod::Vector4f{1,1,1,1}, useController ? (ext::openvr::controllerQuaternion( vr::Controller_Hand::Hand_Right ) * pod::Vector4f{1,1,1,-1}) : ext::openvr::hmdQuaternion() );
				translator = uf::transform::reorient( translator );
				{
					translator.forward *= { 1, 0, 1 };
					translator.right *= { 1, 0, 1 };
					
					translator.forward = uf::vector::normalize( translator.forward );
					translator.right = uf::vector::normalize( translator.right );
				}
			}
			if ( keys.forward ) {
				float mag = uf::vector::magnitude(physics.linear.velocity * pod::Vector3{1, 0, 1});
				if ( mag < speed.limitSquared ) {
					physics.linear.velocity += translator.forward * speed.move;
					mag = uf::vector::magnitude(physics.linear.velocity);
				} else mag = speed.limitSquared;
				pod::Vector3 correction = translator.forward * sqrt(mag);
				physics.linear.velocity.x = correction.x;
				physics.linear.velocity.z = correction.z;
				stats.updateCamera = (stats.walking = true);
			}
			if ( keys.backwards ) {
				float mag = uf::vector::magnitude(physics.linear.velocity * pod::Vector3{1, 0, 1});
				if ( mag < speed.limitSquared ) {
					physics.linear.velocity += translator.forward * -speed.move;
					mag = uf::vector::magnitude(physics.linear.velocity);
				} else mag = speed.limitSquared;
				pod::Vector3 correction = translator.forward * -sqrt(mag);
				physics.linear.velocity.x = correction.x;
				physics.linear.velocity.z = correction.z;
				stats.updateCamera = (stats.walking = true);
			}
			if ( keys.left ) {
				float mag = uf::vector::magnitude(physics.linear.velocity * pod::Vector3{1, 0, 1});
				if ( mag < speed.limitSquared ) {
					physics.linear.velocity += translator.right * -speed.move;
					mag = uf::vector::magnitude(physics.linear.velocity);
				} else mag = speed.limitSquared;
				pod::Vector3 correction = translator.right * -sqrt(mag);
				physics.linear.velocity.x = correction.x;
				physics.linear.velocity.z = correction.z;
				stats.updateCamera = (stats.walking = true);
			}
			if ( keys.right ) {
				float mag = uf::vector::magnitude(physics.linear.velocity * pod::Vector3{1, 0, 1});
				if ( mag < speed.limitSquared ) {
					physics.linear.velocity += translator.right * speed.move;
					mag = uf::vector::magnitude(physics.linear.velocity);
				} else mag = speed.limitSquared;
				pod::Vector3 correction = translator.right * sqrt(mag);
				physics.linear.velocity.x = correction.x;
				physics.linear.velocity.z = correction.z;
				stats.updateCamera = (stats.walking = true);
			}
			if ( keys.jump && metadata["collision"]["jump"] != Json::nullValue ) {
				if ( !metadata["collision"]["should"].asBool() ) {
					if ( metadata["collision"]["jump"][0].asFloat() != 0 ) transform.position.x += metadata["collision"]["jump"][0].asFloat() * uf::physics::time::delta;
					if ( metadata["collision"]["jump"][1].asFloat() != 0 ) transform.position.y += metadata["collision"]["jump"][1].asFloat() * uf::physics::time::delta;
					if ( metadata["collision"]["jump"][2].asFloat() != 0 ) transform.position.z += metadata["collision"]["jump"][2].asFloat() * uf::physics::time::delta;
				} else {
					if ( metadata["collision"]["jump"][0].asFloat() != 0 ) physics.linear.velocity.x = metadata["collision"]["jump"][0].asFloat();
					if ( metadata["collision"]["jump"][1].asFloat() != 0 ) physics.linear.velocity.y = metadata["collision"]["jump"][1].asFloat();
					if ( metadata["collision"]["jump"][2].asFloat() != 0 ) physics.linear.velocity.z = metadata["collision"]["jump"][2].asFloat();
				}
			}
		}

		if ( keys.lookLeft ) {
			uf::transform::rotate( transform, transform.up, -speed.rotate ), stats.updateCamera = true;
		}
		if ( keys.lookRight ) {
			uf::transform::rotate( transform, transform.up, speed.rotate ), stats.updateCamera = true;
		}
		
		if ( keys.crouch ) {
			if ( !metadata["collision"]["should"].asBool() ) {
				if ( metadata["collision"]["jump"][0].asFloat() != 0 ) transform.position.x -= metadata["collision"]["jump"][0].asFloat() * uf::physics::time::delta;
				if ( metadata["collision"]["jump"][1].asFloat() != 0 ) transform.position.y -= metadata["collision"]["jump"][1].asFloat() * uf::physics::time::delta;
				if ( metadata["collision"]["jump"][2].asFloat() != 0 ) transform.position.z -= metadata["collision"]["jump"][2].asFloat() * uf::physics::time::delta;
			} else {
				if ( !metadata["system"]["crouching"].asBool() )  stats.deltaCrouch = true;
				metadata["system"]["crouching"] = true;
			}
		} else {
			if ( metadata["system"]["crouching"].asBool() ) stats.deltaCrouch = true;
			metadata["system"]["crouching"] = false;
		}	
	}
	if ( stats.deltaCrouch ) {
		float delta = 1.0f;
		if ( metadata["system"]["crouching"].asBool() ) camera.getTransform().position.y -= delta;
		else camera.getTransform().position.y += delta;
		stats.updateCamera = true;
	}

	if ( stats.floored ) {
		if ( stats.walking ) {
			uf::SoundEmitter& emitter = this->getComponent<uf::SoundEmitter>();
			int cycle = rand() % metadata["audio"]["footsteps"].size();
			std::string filename = metadata["audio"]["footsteps"][cycle].asString();
			uf::Audio& footstep = emitter.add(filename);

			bool playing = false;
			for ( uint i = 0; i < metadata["audio"]["footsteps"].size(); ++i ) {
				uf::Audio& audio = emitter.add(metadata["audio"]["footsteps"][i].asString());
				if ( audio.playing() ) playing = true;
			}
			if ( !playing ) {
				footstep.play();
				footstep.setVolume(0.1f);
				footstep.setPosition( transform.position );
			}
		} else {
			physics.linear.velocity.x = 0;
			physics.linear.velocity.y = 0;
			physics.linear.velocity.z = 0;
		}
	}

	if ( stats.updateCamera ) camera.updateView();

	ext::Craeture::tick();
}
void ext::Player::render() {
	ext::Craeture::render();
}