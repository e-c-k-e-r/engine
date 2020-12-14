#include "behavior.h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/window/window.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/audio/audio.h>
#include <uf/ext/openvr/openvr.h>
#include <uf/ext/gltf/graph.h>
#include <uf/ext/bullet/bullet.h>
#include <uf/utils/math/physics.h>

#include <sstream>

namespace {
	uf::Serializer masterTableGet( const std::string& table ) {
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& mastertable = scene.getComponent<uf::Serializer>();
		return mastertable["system"]["mastertable"][table];
	}
	uf::Serializer masterDataGet( const std::string& table, const std::string& key ) {
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& mastertable = scene.getComponent<uf::Serializer>();
		return mastertable["system"]["mastertable"][table][key];
	}
	inline int64_t parseInt( const std::string& str ) {
		return atoi(str.c_str());
	}

	inline std::string colorString( const std::string& hex ) {
		return "%#" + hex + "%";
	}
}

UF_BEHAVIOR_REGISTER_CPP(ext::PlayerBehavior)
#define this (&self)
void ext::PlayerBehavior::initialize( uf::Object& self ) {
	auto& transform = this->getComponent<pod::Transform<>>();
	auto& camera = this->getComponent<uf::Camera>();
	camera.getTransform().reference = &transform;

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
			bool stereoscopic = true;
		} settings;

		uf::Camera& camera = this->getComponent<uf::Camera>();
		settings.mode = metadata["camera"]["ortho"].as<bool>() ? -1 : 1;
		settings.perspective.size.x = metadata["camera"]["settings"]["size"]["x"].as<double>();
		settings.perspective.size.y = metadata["camera"]["settings"]["size"]["y"].as<double>();
		camera.setSize(settings.perspective.size);
		if ( settings.mode < 0 ) {
			settings.ortho.lr.x 	= metadata["camera"]["settings"]["left"].as<double>();
			settings.ortho.lr.y 	= metadata["camera"]["settings"]["right"].as<double>();
			settings.ortho.bt.x 	= metadata["camera"]["settings"]["bottom"].as<double>();
			settings.ortho.bt.y 	= metadata["camera"]["settings"]["top"].as<double>();
			settings.ortho.nf.x 	= metadata["camera"]["settings"]["near"].as<double>();
			settings.ortho.nf.y 	= metadata["camera"]["settings"]["far"].as<double>();

			camera.ortho( settings.ortho.lr, settings.ortho.bt, settings.ortho.nf );
		} else {
			settings.perspective.fov 		= metadata["camera"]["settings"]["fov"].as<double>();
			settings.perspective.bounds.x 	= metadata["camera"]["settings"]["clip"]["near"].as<double>();
			settings.perspective.bounds.y 	= metadata["camera"]["settings"]["clip"]["far"].as<double>();

			camera.setFov(settings.perspective.fov);
			camera.setBounds(settings.perspective.bounds);
		}
		camera.setStereoscopic(true);

		settings.offset.x = metadata["camera"]["offset"][0].as<double>();
		settings.offset.y = metadata["camera"]["offset"][1].as<double>();
		settings.offset.z = metadata["camera"]["offset"][2].as<double>();

		pod::Transform<>& transform = camera.getTransform();
		/* Transform initialization */ {
			transform.position.x = metadata["camera"]["position"][0].as<double>();
			transform.position.y = metadata["camera"]["position"][1].as<double>();
			transform.position.z = metadata["camera"]["position"][2].as<double>();

			transform.scale.x = metadata["camera"]["scale"][0].as<double>();
			transform.scale.y = metadata["camera"]["scale"][1].as<double>();
			transform.scale.z = metadata["camera"]["scale"][2].as<double>();
		}

		camera.setOffset(settings.offset);
		camera.update(true);

		// Update viewport
		if ( metadata["camera"]["settings"]["size"]["auto"].as<bool>() )  {
			this->addHook( "window:Resized", [&](ext::json::Value& json){
				// Update persistent window sized (size stored to JSON file)
				pod::Vector2ui size; {
					size.x = json["window"]["size"]["x"].as<size_t>();
					size.y = json["window"]["size"]["y"].as<size_t>();
				}
				/* Update camera's viewport */ {
					uf::Camera& camera = this->getComponent<uf::Camera>();
					camera.setSize({(pod::Math::num_t)size.x, (pod::Math::num_t)size.y});
				}
			} );
		}
	}

	metadata["system"]["control"] = true;
	
	this->addHook( "window:Mouse.CursorVisibility", [&](ext::json::Value& json){
		metadata["system"]["control"] = !json["state"].as<bool>();	
	});

	// Rotate Camera
	this->addHook( "window:Mouse.Moved", [&](ext::json::Value& json){
		// discard events sent by os, only trust client now
		if ( !ext::json::isObject(json) ) return;
		if ( json["invoker"] != "client" ) return;

		pod::Vector2i delta = { json["mouse"]["delta"]["x"].as<int>(), json["mouse"]["delta"]["y"].as<int>() };
		pod::Vector2i size  = { json["mouse"]["size"]["x"].as<int>(),  json["mouse"]["size"]["y"].as<int>() };
		pod::Vector2 relta  = { (float) delta.x / size.x, (float) delta.y / size.y };
		relta *= 2;
		if ( delta.x == 0 && delta.y == 0 ) return;
		if ( !metadata["system"]["control"].as<bool>() ) return;

		bool updateCamera = false;
		uf::Camera& camera = this->getComponent<uf::Camera>();
		pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
		pod::Transform<>& cameraTransform = camera.getTransform();
		auto& collider = this->getComponent<pod::Bullet>();
		if ( delta.x != 0 ) {
			double current, minima, maxima; {
				current = !ext::json::isNull( metadata["camera"]["limit"]["current"][0] ) ? metadata["camera"]["limit"]["current"][0].as<double>() : NAN;
				minima = !ext::json::isNull( metadata["camera"]["limit"]["minima"][0] ) ? metadata["camera"]["limit"]["minima"][0].as<double>() : NAN;
				maxima = !ext::json::isNull( metadata["camera"]["limit"]["maxima"][0] ) ? metadata["camera"]["limit"]["maxima"][0].as<double>() : NAN;
			}
			if ( metadata["camera"]["invert"][0].as<bool>() ) relta.x *= -1;
			current += relta.x;
			if ( current != current || ( current < maxima && current > minima ) ) {
				if ( collider.body && !collider.shared ) {
					ext::bullet::applyRotation( collider, transform.up, relta.x );
				} else {
					uf::transform::rotate( transform, transform.up, relta.x ), updateCamera = true;
				}
			} else current -= relta.x;

			if ( !ext::json::isNull( metadata["camera"]["limit"]["current"][0] ) ) metadata["camera"]["limit"]["current"][0] = current;
		}
		if ( delta.y != 0 ) {
			double current, minima, maxima; {
				current = !ext::json::isNull( metadata["camera"]["limit"]["current"][1] ) ? metadata["camera"]["limit"]["current"][1].as<double>() : NAN;
				minima = !ext::json::isNull( metadata["camera"]["limit"]["minima"][1] ) ? metadata["camera"]["limit"]["minima"][1].as<double>() : NAN;
				maxima = !ext::json::isNull( metadata["camera"]["limit"]["maxima"][1] ) ? metadata["camera"]["limit"]["maxima"][1].as<double>() : NAN;
			}
			if ( metadata["camera"]["invert"][1].as<bool>() ) relta.y *= -1;
			current += relta.y;
			if ( current != current || ( current < maxima && current > minima ) ) {
			//	if ( collider.body && !collider.shared ) {
			//		ext::bullet::applyRotation( collider, cameraTransform.right, relta.y );
			//	} else {
					uf::transform::rotate( cameraTransform, cameraTransform.right, relta.y );
			//	}
				updateCamera = true;
			} else current -= relta.y;
			if ( !ext::json::isNull( metadata["camera"]["limit"]["current"][1] ) ) metadata["camera"]["limit"]["current"][1] = current;
		}
		if ( updateCamera ) {
			camera.updateView();
		}
	});

	this->addHook( ":Update.%UID%", [&](ext::json::Value& json){
	//	for ( auto& member : json[""]["transients"] ) {
		ext::json::forEach(metadata[""]["transients"], [&](ext::json::Value& member){
			if ( member["type"] != "player" ) return;
			std::string id = member["id"].as<std::string>();
			metadata[""]["transients"][id]["hp"] = member["hp"];
			metadata[""]["transients"][id]["mp"] = member["mp"];
		});
	});

	// handle after battles and establishes cooldowns
	this->addHook( "world:Battle.End.%UID%", [&](ext::json::Value& json){
		// update
		{
			uf::Serializer payload;
			payload[""]["transients"] = json["battle"]["transients"];
			this->callHook( ":Update.%UID%", payload );
		}
		
		metadata["system"].erase("battle");
		metadata["system"]["cooldown"] = uf::physics::time::current + 5;
	});
	
	// detect collision against transients, engage in battle
	this->addHook( "world:Collision.%UID%", [&](ext::json::Value& json){
		if ( metadata["system"]["cooldown"].as<float>() > uf::physics::time::current ) return;
		if ( !metadata["system"]["control"].as<bool>() ) return;
		
		std::string state = metadata["system"]["state"].as<std::string>();
		if ( state != "" && state != "null" ) return;

		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Entity* entity = scene.findByUid(json["entity"].as<size_t>());

		if ( !entity ) return;

		uf::Serializer& pMetadata = entity->getComponent<uf::Serializer>();

		std::string onCollision = pMetadata["system"]["onCollision"].as<std::string>();

		if ( onCollision == "battle" ) {
			uf::Serializer payload;
			payload["battle"]["enemy"] = pMetadata[""];
			payload["battle"]["enemy"]["uid"] = json["entity"].as<size_t>();
			payload["battle"]["player"] = metadata[""];
			payload["battle"]["player"]["uid"] = this->getUid();
			payload["battle"]["music"] = pMetadata["music"];
			
			this->callHook("world:Battle.Start", payload);
		} else if ( onCollision == "npc" ) {
			if ( !uf::Window::isKeyPressed("E") ) return;
			uf::Serializer payload;
			payload["dialogue"] = pMetadata["dialogue"];
			payload["uid"] = json["entity"].as<size_t>();
			this->callHook("menu:Dialogue.Start", payload);
		}
	//	metadata["system"]["cooldown"] = uf::physics::time::current + 5;
		metadata["system"]["control"] = false;
		metadata["system"]["menu"] = onCollision;
	});
	this->addHook( "world:Battle.End", [&](ext::json::Value& json){
		metadata["system"]["menu"] = "";
		metadata["system"]["cooldown"] = uf::physics::time::current + 5;

		// update
		{
			uf::Serializer payload;
			payload[""]["transients"] = json["battle"]["transients"];
			this->callHook( ":Update.%UID%", payload );
		}
	});
	this->addHook( "menu:Dialogue.End", [&](ext::json::Value& json){
		metadata["system"]["menu"] = "";
		metadata["system"]["cooldown"] = uf::physics::time::current + 1;
	});

	// Discord Integration
	this->addHook( "discord.Activity.Update.%UID%", [&](ext::json::Value& json){
		std::string leaderId = metadata[""]["party"][0].as<std::string>();
		uf::Serializer cardData = masterDataGet("Card", leaderId);
		uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].as<std::string>());
		std::string leader = charaData["name"].as<std::string>();

		uf::Serializer payload = json;
		payload["details"] = "Leader: " + leader;
		uf::hooks.call( "discord:Activity.Update", payload );
	});
	this->queueHook("discord.Activity.Update.%UID%", ext::json::null(), 1.0);
}
void ext::PlayerBehavior::tick( uf::Object& self ) {
	uf::Camera& camera = this->getComponent<uf::Camera>();
	pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
	pod::Physics& physics = this->getComponent<pod::Physics>();
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Scene& scene = uf::scene::getCurrentScene();

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
		if ( ext::openvr::controllerState( vr::Controller_Hand::Hand_Right, "dpadUp" )["state"].as<bool>() ) keys.forward = true;
		if ( ext::openvr::controllerState( vr::Controller_Hand::Hand_Right, "dpadDown" )["state"].as<bool>() ) keys.backwards = true;
		if ( ext::openvr::controllerState( vr::Controller_Hand::Hand_Right, "dpadLeft" )["state"].as<bool>() ) keys.lookLeft = true; // keys.left = true;
		if ( ext::openvr::controllerState( vr::Controller_Hand::Hand_Right, "dpadRight" )["state"].as<bool>() ) keys.lookRight = true; // keys.right = true;
		if ( ext::openvr::controllerState( vr::Controller_Hand::Hand_Right, "thumbclick" )["state"].as<bool>() ) keys.running = true;
		if ( ext::openvr::controllerState( vr::Controller_Hand::Hand_Right, "a" )["state"].as<bool>() ) keys.jump = true;

		if ( ext::openvr::controllerState( vr::Controller_Hand::Hand_Left, "dpadUp" )["state"].as<bool>() ) keys.forward = true;
		if ( ext::openvr::controllerState( vr::Controller_Hand::Hand_Left, "dpadDown" )["state"].as<bool>() ) keys.backwards = true;
		if ( ext::openvr::controllerState( vr::Controller_Hand::Hand_Left, "dpadLeft" )["state"].as<bool>() ) keys.lookLeft = true;
		if ( ext::openvr::controllerState( vr::Controller_Hand::Hand_Left, "dpadRight" )["state"].as<bool>() ) keys.lookRight = true;
		if ( ext::openvr::controllerState( vr::Controller_Hand::Hand_Left, "thumbclick" )["state"].as<bool>() ) keys.crouch = true, keys.walk = true;
		if ( ext::openvr::controllerState( vr::Controller_Hand::Hand_Left, "a" )["state"].as<bool>() ) keys.paused = true;
	}
	
	struct {
		bool updateCamera = true;
		bool deltaCrouch = false;
		bool walking = false;
		bool floored = true;
		bool impulse = true;
		bool noclipped = false;
		std::string menu = "";
		std::string targetAnimation = "";
	} stats;

	stats.floored = fabs(physics.linear.velocity.y) < 0.01f;
	stats.menu = metadata["system"]["menu"].as<std::string>();
	stats.impulse = metadata["system"]["physics"]["impulse"].as<bool>();
	stats.noclipped = metadata["system"]["noclipped"].as<bool>();
	struct {
		float move = 4;
		float walk = 1;
		float run = 8;
		float rotate = uf::physics::time::delta;
		float limitSquared = 4*4;
	} speed; {
		speed.rotate *= metadata["system"]["physics"]["rotate"].as<float>();
		speed.move = metadata["system"]["physics"]["move"].as<float>();
		speed.run = metadata["system"]["physics"]["run"].as<float>() / metadata["system"]["physics"]["move"].as<float>();
		speed.walk = metadata["system"]["physics"]["walk"].as<float>() / metadata["system"]["physics"]["move"].as<float>();
	}
	if ( !metadata["system"]["physics"]["collision"].as<bool>() ) {
		stats.impulse = true;
	}
	if ( keys.running ) speed.move *= speed.run;
	else if ( keys.walk ) speed.move *= speed.walk;
	speed.limitSquared = speed.move * speed.move;

	uf::Object* menu = (uf::Object*) this->getRootParent().findByName("Gui: Menu");
	if ( !menu ) stats.menu = "";
	// make assumptions
	if ( stats.menu == "" && keys.paused ) {
		stats.menu = "paused";
		metadata["system"]["control"] = false;
		uf::hooks.call("menu:Pause");
	}
	else if ( !metadata["system"]["control"].as<bool>() ) {
		stats.menu = "menu";
	} else if ( stats.menu == "" ) {
		metadata["system"]["control"] = true;
	} else {
		metadata["system"]["control"] = false;
	}
	metadata["system"]["menu"] = stats.menu;

	auto& collider = this->getComponent<pod::Bullet>();

	if ( metadata["system"]["control"].as<bool>() ) {	
		{
			TIMER(0.25, keys.vee && ) {
				bool state = !metadata["system"]["noclipped"].as<bool>();
				metadata["system"]["noclipped"] = state;
				
				std::cout << "Toggling noclip: " << transform.position.x << ", " << transform.position.y << ", " << transform.position.z << std::endl;
				if ( state ) {
					std::cout << "Enabled noclip" << std::endl;
					if ( collider.body ) {
						collider.body->setGravity(btVector3(0,0.0,0));
						collider.body->setCollisionFlags(collider.body->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
						collider.body->setActivationState(DISABLE_SIMULATION);
					}
				} else {
					std::cout << "Disabled noclip" << std::endl;
					if ( collider.body ) {
						collider.body->setGravity(btVector3(0,-9.81,0));
						collider.body->setCollisionFlags(collider.body->getCollisionFlags() & ~btCollisionObject::CF_NO_CONTACT_RESPONSE);
						collider.body->setActivationState(DISABLE_DEACTIVATION);
					}
				}
				stats.noclipped = state;
			}
		}

		if ( stats.floored ) {
			pod::Transform<> translator = transform;
			if ( ext::openvr::context ) {
				bool useController = true;
				translator.orientation = uf::quaternion::multiply( transform.orientation * pod::Vector4f{1,1,1,1}, useController ? (ext::openvr::controllerQuaternion( vr::Controller_Hand::Hand_Right ) * pod::Vector4f{1,1,1,1}) : ext::openvr::hmdQuaternion() );
				translator = uf::transform::reorient( translator );
				
				translator.forward *= { 1, 0, 1 };
				translator.right *= { 1, 0, 1 };
				
				translator.forward = uf::vector::normalize( translator.forward );
				translator.right = uf::vector::normalize( translator.right );
			}
			pod::Vector3f queued = {};
			if ( keys.forward || keys.backwards ) {
				int polarity = keys.forward ? 1 : -1;
				float mag = uf::vector::magnitude(physics.linear.velocity * pod::Vector3{1, 0, 1});
				if ( mag < speed.limitSquared ) {
				//	physics.linear.velocity += translator.forward * speed.move * polarity;
				//	mag = uf::vector::magnitude(physics.linear.velocity);
					mag = uf::vector::magnitude(physics.linear.velocity + translator.forward * speed.move * polarity);
				} else mag = speed.limitSquared;
				pod::Vector3 correction = translator.forward * sqrt(mag) * polarity;
				
				if ( collider.body && !collider.shared ) {
				//	physics.linear.velocity.x += correction.x;
				//	physics.linear.velocity.z += correction.z;
				//	ext::bullet::move( collider, physics.linear.velocity );
					queued += correction;
				} else {
					if ( stats.impulse && stats.noclipped ) {
						physics.linear.velocity.x = correction.x;
						physics.linear.velocity.z = correction.z;
					} else {
						correction *= uf::physics::time::delta;
						transform.position.x += correction.x;
						transform.position.z += correction.z;
					}
				}
				stats.updateCamera = (stats.walking = true);
			}
			if ( keys.left || keys.right ) {
				int polarity = keys.right ? 1 : -1;
				float mag = uf::vector::magnitude(physics.linear.velocity * pod::Vector3{1, 0, 1});
				if ( mag < speed.limitSquared ) {
				//	physics.linear.velocity += translator.right * speed.move * polarity;
				//	mag = uf::vector::magnitude(physics.linear.velocity);
					mag = uf::vector::magnitude(physics.linear.velocity + translator.right * speed.move * polarity);
				} else mag = speed.limitSquared;
				pod::Vector3 correction = translator.right * sqrt(mag) * polarity;
				
				if ( collider.body && !collider.shared ) {
				//	physics.linear.velocity.x += correction.x;
				//	physics.linear.velocity.z += correction.z;
				//	ext::bullet::move( collider, physics.linear.velocity );
					queued += correction;
				} else {
					if ( stats.impulse && stats.noclipped ) {
						physics.linear.velocity.x = correction.x;
						physics.linear.velocity.z = correction.z;
					} else {
						correction *= uf::physics::time::delta;
						transform.position.x += correction.x;
						transform.position.z += correction.z;
					}
				}
				stats.updateCamera = (stats.walking = true);
			}
			if ( keys.left || keys.right || keys.forward || keys.backwards ) {
				if ( collider.body && !collider.shared ) {
					physics.linear.velocity.x = queued.x;
					physics.linear.velocity.z = queued.z;
					ext::bullet::move( collider, physics.linear.velocity );
				}
			}
			if ( !keys.forward && !keys.backwards && !keys.left && !keys.right ) {
				if ( collider.body && !collider.shared ) {
					physics.linear.velocity.x = 0;
					physics.linear.velocity.z = 0;
					ext::bullet::move( collider, physics.linear.velocity );
				}
			}
			if ( keys.jump ) {
				if ( collider.body && !collider.shared ) {
					pod::Vector3f yump = uf::vector::decode(metadata["system"]["physics"]["jump"], pod::Vector3f{});
					if ( fabs(yump.x) > 0.001f ) physics.linear.velocity.x = yump.x;
					if ( fabs(yump.y) > 0.001f ) physics.linear.velocity.y = yump.y;
					if ( fabs(yump.z) > 0.001f ) physics.linear.velocity.z = yump.z;
					ext::bullet::move( collider, physics.linear.velocity );
				} else {
					if ( metadata["system"]["physics"]["jump"][0].as<float>() != 0 ) transform.position.x += metadata["system"]["physics"]["jump"][0].as<float>() * uf::physics::time::delta;
					if ( metadata["system"]["physics"]["jump"][1].as<float>() != 0 ) transform.position.y += metadata["system"]["physics"]["jump"][1].as<float>() * uf::physics::time::delta;
					if ( metadata["system"]["physics"]["jump"][2].as<float>() != 0 ) transform.position.z += metadata["system"]["physics"]["jump"][2].as<float>() * uf::physics::time::delta;
				}
			}
		}

		if ( keys.lookLeft ) {
			if ( collider.body && !collider.shared ) {
				ext::bullet::applyRotation( collider, transform.up, -speed.rotate );
			} else {
				uf::transform::rotate( transform, transform.up, -speed.rotate ), stats.updateCamera = true;
			}
			stats.updateCamera = true;
		}
		if ( keys.lookRight ) {
			if ( collider.body && !collider.shared ) {
				ext::bullet::applyRotation( collider, transform.up, speed.rotate );
			} else {
				uf::transform::rotate( transform, transform.up, speed.rotate ), stats.updateCamera = true;
			}
			stats.updateCamera = true;
		}
		if ( keys.crouch ) {
			if ( stats.noclipped ) {
				if ( collider.body && !collider.shared ) {
					pod::Vector3f yump = uf::vector::decode(metadata["system"]["physics"]["jump"], pod::Vector3f{});
					if ( fabs(yump.x) > 0.001f ) physics.linear.velocity.x = -yump.x;
					if ( fabs(yump.y) > 0.001f ) physics.linear.velocity.y = -yump.y;
					if ( fabs(yump.z) > 0.001f ) physics.linear.velocity.z = -yump.z;
					ext::bullet::move( collider, physics.linear.velocity );
				}
			} else {
				if ( !metadata["system"]["physics"]["collision"].as<bool>() ) {
					if ( metadata["system"]["physics"]["jump"][0].as<float>() != 0 ) transform.position.x -= metadata["system"]["physics"]["jump"][0].as<float>() * uf::physics::time::delta;
					if ( metadata["system"]["physics"]["jump"][1].as<float>() != 0 ) transform.position.y -= metadata["system"]["physics"]["jump"][1].as<float>() * uf::physics::time::delta;
					if ( metadata["system"]["physics"]["jump"][2].as<float>() != 0 ) transform.position.z -= metadata["system"]["physics"]["jump"][2].as<float>() * uf::physics::time::delta;
				} else {
					if ( !metadata["system"]["crouching"].as<bool>() )  stats.deltaCrouch = true;
					metadata["system"]["crouching"] = true;
				}
			}
		} else {
			if ( metadata["system"]["crouching"].as<bool>() ) stats.deltaCrouch = true;
			metadata["system"]["crouching"] = false;
		}
	}
	if ( stats.noclipped && !keys.forward && !keys.backwards && !keys.left && !keys.right && !keys.jump && !keys.crouch ) {
		if ( collider.body && !collider.shared ) {
			physics.linear.velocity = {};
			ext::bullet::move( collider, physics.linear.velocity );
		}
	}
	if ( stats.deltaCrouch ) {
		float delta = metadata["system"]["physics"]["crouch"].as<float>();
		if ( metadata["system"]["crouching"].as<bool>() ) camera.getTransform().position.y -= delta;
		else camera.getTransform().position.y += delta;
		stats.updateCamera = true;
	}

	if ( stats.floored ) {
		if ( stats.walking ) {
			auto& emitter = this->getComponent<uf::MappedSoundEmitter>();
			int cycle = rand() % metadata["audio"]["footstep"]["list"].size();
			std::string filename = metadata["audio"]["footstep"]["list"][cycle].as<std::string>();
			uf::Audio& footstep = emitter.add(filename);

			bool playing = false;
			for ( uint i = 0; i < metadata["audio"]["footstep"]["list"].size(); ++i ) {
				uf::Audio& audio = emitter.add(metadata["audio"]["footstep"]["list"][i].as<std::string>());
				if ( audio.playing() ) playing = true;
			}
			if ( !playing ) {
				footstep.play();
				footstep.setVolume(metadata["audio"]["footstep"]["volume"].as<float>());
				footstep.setPosition( transform.position );

				// [0, 1]
				float modulation = (rand() % 100) / 100.0;
				// [0, 0.1]
				modulation *= 0.1f;
				// [-0.05, 0.05]
				modulation -= 0.05f;
				if ( keys.running ) modulation += 0.5f;
				footstep.setPitch(1 + modulation);
			}
			// set animation to walk
			stats.targetAnimation = "walk";
		} else if ( !keys.jump ) {
			stats.targetAnimation = "idle_wank";
		}
	}
#if 0
	{
		TIMER(0.25, keys.vee && ) {
			if ( ext::json::isNull( metadata["system"]["physics"]["backup"]["collision"] ) )
				metadata["system"]["physics"]["backup"]["collision"] = metadata["system"]["physics"]["collision"];
			if ( !metadata["system"]["physics"]["collision"].as<bool>() ) {
				metadata["system"]["physics"]["collision"] = metadata["system"]["physics"]["backup"]["collision"];
			} else {
				metadata["system"]["physics"]["collision"] = 0;
			}
			
			std::cout << "Toggling noclip: " << transform.position.x << ", " << transform.position.y << ", " << transform.position.z << std::endl;
			std::cout << metadata.dump(1, '\t') << std::endl;

		//	metadata["system"]["physics"]["collision"] = !metadata["system"]["physics"]["collision"].as<bool>();
			physics.linear.velocity = {0,0,0};
		}
	}
	

	if ( ext::json::isObject( metadata["system"]["physics"]["clamp"] ) ) {
		if ( ext::json::isArray( metadata["system"]["physics"]["clamp"]["x"] ) ) {
			transform.position.x = std::clamp( transform.position.x, metadata["system"]["physics"]["clamp"]["x"][0].as<float>(), metadata["system"]["physics"]["clamp"]["x"][1].as<float>() );
		}
		if ( ext::json::isArray( metadata["system"]["physics"]["clamp"]["y"] ) ) {
			auto previous = transform.position.y;
			transform.position.y = std::clamp( transform.position.y, metadata["system"]["physics"]["clamp"]["y"][0].as<float>(), metadata["system"]["physics"]["clamp"]["y"][1].as<float>() );
			if ( transform.position.y > previous ) {
				physics.linear.velocity.y = 0;
				stats.floored = true;
			}
		}
		if ( ext::json::isArray( metadata["system"]["physics"]["clamp"]["z"] ) ) {
			transform.position.z = std::clamp( transform.position.z, metadata["system"]["physics"]["clamp"]["z"][0].as<float>(), metadata["system"]["physics"]["clamp"]["z"][1].as<float>() );
		}
	}
	
	if ( metadata["system"]["control"].as<bool>() ) {	
		if ( stats.floored ) {
			pod::Transform<> translator = transform;
			if ( ext::openvr::context ) {
				bool useController = true;
				translator.orientation = uf::quaternion::multiply( transform.orientation * pod::Vector4f{1,1,1,1}, useController ? (ext::openvr::controllerQuaternion( vr::Controller_Hand::Hand_Right ) * pod::Vector4f{1,1,1,1}) : ext::openvr::hmdQuaternion() );
				translator = uf::transform::reorient( translator );
				
				translator.forward *= { 1, 0, 1 };
				translator.right *= { 1, 0, 1 };
				
				translator.forward = uf::vector::normalize( translator.forward );
				translator.right = uf::vector::normalize( translator.right );
			}
			if ( keys.forward || keys.backwards ) {
				int polarity = keys.forward ? 1 : -1;
				float mag = uf::vector::magnitude(physics.linear.velocity * pod::Vector3{1, 0, 1});
				if ( mag < speed.limitSquared ) {
					physics.linear.velocity += translator.forward * speed.move * polarity;
					mag = uf::vector::magnitude(physics.linear.velocity);
				} else mag = speed.limitSquared;
				pod::Vector3 correction = translator.forward * sqrt(mag) * polarity;
				if ( stats.impulse ) {
					physics.linear.velocity.x = correction.x;
					physics.linear.velocity.z = correction.z;
				} else {
					correction *= uf::physics::time::delta;
					transform.position.x += correction.x;
					transform.position.z += correction.z;
				}
				stats.updateCamera = (stats.walking = true);
			}
			if ( keys.left || keys.right ) {
				int polarity = keys.right ? 1 : -1;
				float mag = uf::vector::magnitude(physics.linear.velocity * pod::Vector3{1, 0, 1});
				if ( mag < speed.limitSquared ) {
					physics.linear.velocity += translator.right * speed.move * polarity;
					mag = uf::vector::magnitude(physics.linear.velocity);
				} else mag = speed.limitSquared;
				pod::Vector3 correction = translator.right * sqrt(mag) * polarity;
				if ( stats.impulse ) {
					physics.linear.velocity.x = correction.x;
					physics.linear.velocity.z = correction.z;
				} else {
					correction *= uf::physics::time::delta;
					transform.position.x += correction.x;
					transform.position.z += correction.z;
				}
				stats.updateCamera = (stats.walking = true);
			}
			if ( keys.jump ) {
				if ( !metadata["system"]["physics"]["collision"].as<bool>() ) {
					if ( metadata["system"]["physics"]["jump"][0].as<float>() != 0 ) transform.position.x += metadata["system"]["physics"]["jump"][0].as<float>() * uf::physics::time::delta;
					if ( metadata["system"]["physics"]["jump"][1].as<float>() != 0 ) transform.position.y += metadata["system"]["physics"]["jump"][1].as<float>() * uf::physics::time::delta;
					if ( metadata["system"]["physics"]["jump"][2].as<float>() != 0 ) transform.position.z += metadata["system"]["physics"]["jump"][2].as<float>() * uf::physics::time::delta;
				} else {
					if ( metadata["system"]["physics"]["jump"][0].as<float>() != 0 ) physics.linear.velocity.x = metadata["system"]["physics"]["jump"][0].as<float>();
					if ( metadata["system"]["physics"]["jump"][1].as<float>() != 0 ) physics.linear.velocity.y = metadata["system"]["physics"]["jump"][1].as<float>();
					if ( metadata["system"]["physics"]["jump"][2].as<float>() != 0 ) physics.linear.velocity.z = metadata["system"]["physics"]["jump"][2].as<float>();
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
			if ( !metadata["system"]["physics"]["collision"].as<bool>() ) {
				if ( metadata["system"]["physics"]["jump"][0].as<float>() != 0 ) transform.position.x -= metadata["system"]["physics"]["jump"][0].as<float>() * uf::physics::time::delta;
				if ( metadata["system"]["physics"]["jump"][1].as<float>() != 0 ) transform.position.y -= metadata["system"]["physics"]["jump"][1].as<float>() * uf::physics::time::delta;
				if ( metadata["system"]["physics"]["jump"][2].as<float>() != 0 ) transform.position.z -= metadata["system"]["physics"]["jump"][2].as<float>() * uf::physics::time::delta;
			} else {
				if ( !metadata["system"]["crouching"].as<bool>() )  stats.deltaCrouch = true;
				metadata["system"]["crouching"] = true;
			}
		} else {
			if ( metadata["system"]["crouching"].as<bool>() ) stats.deltaCrouch = true;
			metadata["system"]["crouching"] = false;
		}	
	}
	if ( stats.deltaCrouch ) {
		float delta = metadata["system"]["physics"]["crouch"].as<float>();
		if ( metadata["system"]["crouching"].as<bool>() ) camera.getTransform().position.y -= delta;
		else camera.getTransform().position.y += delta;
		stats.updateCamera = true;
	}

	if ( stats.floored ) {
		if ( stats.walking ) {
			uf::SoundEmitter& emitter = this->getComponent<uf::SoundEmitter>();
			int cycle = rand() % metadata["audio"]["footstep"]["list"].size();
			std::string filename = metadata["audio"]["footstep"]["list"][cycle].as<std::string>();
			uf::Audio& footstep = emitter.add(filename);

			bool playing = false;
			for ( uint i = 0; i < metadata["audio"]["footstep"]["list"].size(); ++i ) {
				uf::Audio& audio = emitter.add(metadata["audio"]["footstep"]["list"][i].as<std::string>());
				if ( audio.playing() ) playing = true;
			}
			if ( !playing ) {
				footstep.play();
				footstep.setVolume(metadata["audio"]["footstep"]["volume"].as<float>());
				footstep.setPosition( transform.position );

				// [0, 1]
				float modulation = (rand() % 100) / 100.0;
				// [0, 0.1]
				modulation *= 0.1f;
				// [-0.05, 0.05]
				modulation -= 0.05f;
				if ( keys.running ) modulation += 0.5f;
				footstep.setPitch(1 + modulation);
			}
			// set animation to walk
			stats.targetAnimation = "walk";
		} else if ( !keys.jump ) {
			physics.linear.velocity.x = 0;
			physics.linear.velocity.y = 0;
			physics.linear.velocity.z = 0;
			stats.targetAnimation = "idle_wank";
		}
	}
#endif
	
	// set animation to idle
	if ( stats.targetAnimation != "" ) {
		auto* playerModel = this->findByName("Player: Model");
		if ( playerModel && playerModel->hasComponent<pod::Graph>() ) {
			auto& graph = playerModel->getComponent<pod::Graph>();
			bool should = true;
			if ( graph.sequence.empty() && graph.sequence.front() == stats.targetAnimation ) should = false;
			if ( should ) {
				graph.settings.animations.loop = true;
				uf::graph::animate( graph, stats.targetAnimation );
			}
		}
	}
/*
	{
		auto flatten = uf::transform::flatten( transform );
		static pod::Quaternion<> storedCameraOrientation = transform.orientation;

		const pod::Quaternion<> prevCameraOrientation = storedCameraOrientation;
		const pod::Quaternion<> curCameraOrientation = transform.orientation;
		const pod::Quaternion<> deltaOrientation = uf::quaternion::multiply( curCameraOrientation, uf::quaternion::inverse( prevCameraOrientation ) ) ;
		const pod::Vector3f deltaAngles = uf::quaternion::eulerAngles( deltaOrientation );
		float magnitude = uf::vector::magnitude( deltaOrientation );
		if ( magnitude > 0.0001f ) {
			uf::Serializer payload;
			payload["previous"] = uf::vector::encode( prevCameraOrientation );
			payload["current"] = uf::vector::encode( curCameraOrientation );
			payload["delta"] = uf::vector::encode( deltaOrientation );
			payload["euler"] = uf::vector::encode( deltaAngles );
			payload["angle"]["pitch"] = deltaAngles.x;
			payload["angle"]["yaw"] = deltaAngles.y;
			payload["angle"]["roll"] = deltaAngles.z;
			payload["magnitude"] = magnitude;
			this->callHook("controller:Camera.Rotated", payload);
		}
		storedCameraOrientation = transform.orientation;
	}
*/
/*
	// translate movement in HMD to give HUD feedback
	if ( ext::openvr::context ) {
		static pod::Quaternion<> prevCameraOrientation = ext::openvr::hmdQuaternion();
		const pod::Quaternion<> curCameraOrientation = ext::openvr::hmdQuaternion();
		const pod::Vector3f prevEulerAngle = uf::quaternion::eulerAngles( prevCameraOrientation );
		const pod::Vector3f curEulerAngle = uf::quaternion::eulerAngles( curCameraOrientation );
		const pod::Vector3f deltaAngles = uf::vector::subtract( curEulerAngle, prevEulerAngle );
		float magnitude = uf::vector::magnitude( deltaAngles );
		if ( magnitude > 0.0001f ) {
			uf::Serializer payload;
			payload["previous"] = uf::vector::encode( prevCameraOrientation );
			payload["current"] = uf::vector::encode( curCameraOrientation );
			payload["euler"] = uf::vector::encode( deltaAngles );
			payload["angle"]["pitch"] = deltaAngles.x;
			payload["angle"]["yaw"] = deltaAngles.y;
			payload["angle"]["roll"] = deltaAngles.z;
			payload["magnitude"] = magnitude;
			this->callHook("controller:Camera.Rotated", payload);
		}
		prevCameraOrientation = curCameraOrientation;
	}
*/
	#define TRACK_ORIENTATION(ORIENTATION) {\
		static pod::Quaternion<> storedCameraOrientation = ORIENTATION;\
		const pod::Quaternion<> prevCameraOrientation = storedCameraOrientation;\
		const pod::Quaternion<> curCameraOrientation = ORIENTATION;\
		const pod::Quaternion<> deltaOrientation = uf::quaternion::multiply( curCameraOrientation, uf::quaternion::inverse( prevCameraOrientation ) ) ;\
		const pod::Vector3f deltaAngles = uf::quaternion::eulerAngles( deltaOrientation );\
		combinedDeltaAngles = uf::vector::add( combinedDeltaAngles, deltaAngles );\
		combinedDeltaOrientation = uf::quaternion::multiply( deltaOrientation, combinedDeltaOrientation );\
		storedCameraOrientation = ORIENTATION;\
	}

	{
		pod::Quaternion<> combinedDeltaOrientation = {0,0,0,1};
		pod::Vector3f combinedDeltaAngles = {};
		TRACK_ORIENTATION(transform.orientation);
		TRACK_ORIENTATION(camera.getTransform().orientation);
		float magnitude = uf::quaternion::magnitude( combinedDeltaOrientation );
		if ( magnitude > 0.0001f ) {
			uf::Serializer payload;
			payload["delta"] = uf::vector::encode( combinedDeltaOrientation );
			payload["angle"]["pitch"] = combinedDeltaAngles.x;
			payload["angle"]["yaw"] = combinedDeltaAngles.y;
			payload["angle"]["roll"] = combinedDeltaAngles.z;
			payload["magnitude"] = magnitude;
			this->callHook("controller:Camera.Rotated", payload);
		}
	}


	if ( stats.updateCamera )
		camera.updateView();
}

void ext::PlayerBehavior::render( uf::Object& self ){}
void ext::PlayerBehavior::destroy( uf::Object& self ){}
#undef this