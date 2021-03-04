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
#include <uf/spec/controller/controller.h>

#include <sstream>
#if 0
namespace {
	uf::Serializer masterDataGet( const std::string& table, const std::string& key ) {
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& mastertable = scene.getComponent<uf::Serializer>();
		return mastertable["system"]["mastertable"][table][key];
	}
}
#endif
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
				#if UF_USE_BULLET
					ext::bullet::applyRotation( collider, transform.up, relta.x );
				#endif
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
#if 0
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
#endif
#if 0
#if UF_USE_DISCORD
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
#endif
#endif
#if !UF_ENTITY_METADATA_USE_JSON
	{
		auto& metadataBehavior = this->getComponent<ext::PlayerBehavior::Metadata>();
		auto& metadataSystem = metadata["system"];
		metadataBehavior.system.menu = metadataSystem["menu"].as<std::string>();
		metadataBehavior.system.control = metadataSystem["control"].as<bool>();
		metadataBehavior.system.crouching = metadataSystem["crouching"].as<bool>();
		metadataBehavior.system.noclipped = metadataSystem["noclipped"].as<bool>();
		auto& metadataSystemPhysics = metadataSystem["physics"];
		metadataBehavior.system.physics.impulse = metadataSystemPhysics["impulse"].as<bool>();
		metadataBehavior.system.physics.rotate = metadataSystemPhysics["rotate"].as<float>();
		metadataBehavior.system.physics.move = metadataSystemPhysics["move"].as<float>();
		metadataBehavior.system.physics.run = metadataSystemPhysics["run"].as<float>();
		metadataBehavior.system.physics.walk = metadataSystemPhysics["walk"].as<float>();
		metadataBehavior.system.physics.collision = metadataSystemPhysics["collision"].as<bool>();
		metadataBehavior.system.physics.jump = uf::vector::decode(metadataSystemPhysics["jump"], pod::Vector3f{});
		metadataBehavior.system.physics.crouch = metadataSystemPhysics["crouch"].as<float>();
		auto& metadataAudioFootstep = metadata["audio"]["footstep"];
		ext::json::forEach( metadataAudioFootstep["list"], [&]( const ext::json::Value& value ){
			metadataBehavior.audio.footstep.list.emplace_back(value);
		});
		metadataBehavior.audio.footstep.volume = metadataAudioFootstep["volume"].as<float>();
	}
#endif
}
void ext::PlayerBehavior::tick( uf::Object& self ) {
	auto& camera = this->getComponent<uf::Camera>();
	auto& transform = this->getComponent<pod::Transform<>>();
	auto& physics = this->getComponent<pod::Physics>();
	auto& scene = uf::scene::getCurrentScene();

	struct {
		bool forward;
		bool backwards;
		bool left;
		bool right;
	
		bool lookLeft;
		bool lookRight;
		bool running;
		bool walk;
		bool jump;
		bool crouch;
		bool paused;
		bool vee;
	} keys = {
		.forward = uf::Window::isKeyPressed("W"),
		.backwards = uf::Window::isKeyPressed("S"),
		.left = uf::Window::isKeyPressed("A"),
		.right = uf::Window::isKeyPressed("D"),
		.lookLeft = uf::Window::isKeyPressed("Left"),
		.lookRight = uf::Window::isKeyPressed("Right"),
		.running = uf::Window::isKeyPressed("LShift"),
		.walk = uf::Window::isKeyPressed("LAlt"),
		.jump = uf::Window::isKeyPressed(" "),
		.crouch = uf::Window::isKeyPressed("LControl"),
		.paused = uf::Window::isKeyPressed("Escape"),
		.vee = uf::Window::isKeyPressed("V"),
	};

	if ( spec::controller::connected() ) {
	#if UF_USE_OPENVR
		if ( spec::controller::pressed( "R_DPAD_UP" ) ) keys.forward = true;
		if ( spec::controller::pressed( "R_DPAD_DOWN" ) ) keys.backwards = true;
		if ( spec::controller::pressed( "R_DPAD_LEFT" ) ) keys.lookLeft = true; // keys.left = true;
		if ( spec::controller::pressed( "R_DPAD_RIGHT" ) ) keys.lookRight = true; // keys.right = true;
		if ( spec::controller::pressed( "R_JOYSTICK" ) ) keys.running = true;
		if ( spec::controller::pressed( "R_A" ) ) keys.jump = true;

		if ( spec::controller::pressed( "L_DPAD_UP" ) ) keys.forward = true;
		if ( spec::controller::pressed( "L_DPAD_DOWN" ) ) keys.backwards = true;
		if ( spec::controller::pressed( "L_DPAD_LEFT" ) ) keys.lookLeft = true;
		if ( spec::controller::pressed( "L_DPAD_RIGHT" ) ) keys.lookRight = true;
		if ( spec::controller::pressed( "L_JOYSTICK" ) ) keys.crouch = true, keys.walk = true;
		if ( spec::controller::pressed( "L_A" ) ) keys.paused = true;
	#else
		if ( spec::controller::pressed( "DPAD_UP" ) ) keys.forward = true;
		if ( spec::controller::pressed( "DPAD_DOWN" ) ) keys.backwards = true;
		if ( spec::controller::pressed( "DPAD_LEFT" ) ) keys.lookLeft = true;
		if ( spec::controller::pressed( "DPAD_RIGHT" ) ) keys.lookRight = true;
		if ( spec::controller::pressed( "A" ) ) keys.jump = true;
		if ( spec::controller::pressed( "B" ) ) keys.running = true;
		if ( spec::controller::pressed( "X" ) ) keys.crouch = true, keys.walk = true;
		if ( spec::controller::pressed( "Y" ) ) keys.vee = true;
		if ( spec::controller::pressed( "L_TRIGGER" ) ) keys.left = true;
		if ( spec::controller::pressed( "R_TRIGGER" ) ) keys.right = true;
		if ( spec::controller::pressed( "START" ) ) keys.paused = true;
	#endif
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

	auto& metadata = this->getComponent<ext::PlayerBehavior::Metadata>();
#if UF_ENTITY_METADATA_USE_JSON
	auto& metadataJson = this->getComponent<uf::Serializer>();
	auto& metadataSystem = metadataJson["system"];
	metadata.system.menu = metadataSystem["menu"].as<std::string>();
	metadata.system.control = metadataSystem["control"].as<bool>();
	metadata.system.crouching = metadataSystem["crouching"].as<bool>();
	metadata.system.noclipped = metadataSystem["noclipped"].as<bool>();
	auto& metadataSystemPhysics = metadataSystem["physics"];
	metadata.system.physics.impulse = metadataSystemPhysics["impulse"].as<bool>();
	metadata.system.physics.rotate = metadataSystemPhysics["rotate"].as<float>();
	metadata.system.physics.move = metadataSystemPhysics["move"].as<float>();
	metadata.system.physics.run = metadataSystemPhysics["run"].as<float>();
	metadata.system.physics.walk = metadataSystemPhysics["walk"].as<float>();
	metadata.system.physics.collision = metadataSystemPhysics["collision"].as<bool>();
	metadata.system.physics.jump = uf::vector::decode(metadataSystemPhysics["jump"], pod::Vector3f{});
	metadata.system.physics.crouch = metadataSystemPhysics["crouch"].as<float>();
	auto& metadataAudioFootstep = metadataJson["audio"]["footstep"];
	ext::json::forEach( metadataAudioFootstep["list"], [&]( const ext::json::Value& value ){
		metadata.audio.footstep.list.emplace_back(value);
	});
	metadata.audio.footstep.volume = metadataAudioFootstep["volume"].as<float>();
#endif
	stats.floored = fabs(physics.linear.velocity.y) < 0.01f;
	stats.menu = metadata.system.menu;
	stats.impulse = metadata.system.physics.impulse;
	stats.noclipped = metadata.system.noclipped;
	struct {
		float move = 4;
		float walk = 1;
		float run = 8;
		float rotate = uf::physics::time::delta;
		float limitSquared = 4*4;
	} speed; {
		speed.rotate *= metadata.system.physics.rotate;
		speed.move = metadata.system.physics.move;
		speed.run = metadata.system.physics.run / speed.move;
		speed.walk = metadata.system.physics.walk / speed.move;
	}
	if ( !metadata.system.physics.collision ) {
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
		metadata.system.control = false;
		uf::hooks.call("menu:Pause");
	}
	else if ( !metadata.system.control ) {
		stats.menu = "menu";
	} else if ( stats.menu == "" ) {
		metadata.system.control = true;
	} else {
		metadata.system.control = false;
	}
	metadata.system.menu = stats.menu;
	auto& collider = this->getComponent<pod::Bullet>();
	if ( metadata.system.control ) {	
		{
			TIMER(0.25, keys.vee && ) {
				bool state = !stats.noclipped;
				metadata.system.noclipped = state;
				
				UF_DEBUG_MSG( (state ? "En" : "Dis") << "abled noclip: " << uf::vector::toString(transform.position));
				if ( state ) {
				#if UF_USE_BULLET
					if ( collider.body ) {
						collider.body->setGravity(btVector3(0,0.0,0));
						collider.body->setCollisionFlags(collider.body->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
						collider.body->setActivationState(DISABLE_SIMULATION);
					}
				#endif
				} else {
				#if UF_USE_BULLET
					if ( collider.body ) {
						collider.body->setGravity(btVector3(0,-9.81,0));
						collider.body->setCollisionFlags(collider.body->getCollisionFlags() & ~btCollisionObject::CF_NO_CONTACT_RESPONSE);
						collider.body->setActivationState(DISABLE_DEACTIVATION);
					}
				#endif
				}
				stats.noclipped = state;
			}
		}

		if ( stats.floored ) {
			pod::Transform<> translator = transform;
		#if UF_USE_OPENVR
			if ( ext::openvr::context ) {
				bool useController = true;
				translator.orientation = uf::quaternion::multiply( transform.orientation * pod::Vector4f{1,1,1,1}, useController ? (ext::openvr::controllerQuaternion( vr::Controller_Hand::Hand_Right ) * pod::Vector4f{1,1,1,1}) : ext::openvr::hmdQuaternion() );
				translator = uf::transform::reorient( translator );
				
				translator.forward *= { 1, 0, 1 };
				translator.right *= { 1, 0, 1 };
				
				translator.forward = uf::vector::normalize( translator.forward );
				translator.right = uf::vector::normalize( translator.right );
			}
		#endif
			pod::Vector3f queued = {};
			if ( keys.forward || keys.backwards ) {
				int polarity = keys.forward ? 1 : -1;
				float mag = uf::vector::magnitude(physics.linear.velocity * pod::Vector3{1, 0, 1});
				if ( mag < speed.limitSquared ) {
					mag = uf::vector::magnitude(physics.linear.velocity + translator.forward * speed.move * polarity);
				} else mag = speed.limitSquared;
				pod::Vector3 correction = translator.forward * ::sqrt(mag) * polarity;
				
				if ( collider.body && !collider.shared ) {
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
					mag = uf::vector::magnitude(physics.linear.velocity + translator.right * speed.move * polarity);
				} else mag = speed.limitSquared;
				pod::Vector3 correction = translator.right * ::sqrt(mag) * polarity;
				
				if ( collider.body && !collider.shared ) {
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
				#if UF_USE_BULLET
					ext::bullet::move( collider, physics.linear.velocity );
				#endif
				}
			}
			if ( !keys.forward && !keys.backwards && !keys.left && !keys.right ) {
				if ( collider.body && !collider.shared ) {
					physics.linear.velocity.x = 0;
					physics.linear.velocity.z = 0;
				#if UF_USE_BULLET
					ext::bullet::move( collider, physics.linear.velocity );
				#endif
				}
			}
			if ( keys.jump ) {
				pod::Vector3f yump = metadata.system.physics.jump;
				if ( collider.body && !collider.shared ) {
					if ( fabs(yump.x) > 0.001f ) physics.linear.velocity.x = yump.x;
					if ( fabs(yump.y) > 0.001f ) physics.linear.velocity.y = yump.y;
					if ( fabs(yump.z) > 0.001f ) physics.linear.velocity.z = yump.z;
				#if UF_USE_BULLET
					ext::bullet::move( collider, physics.linear.velocity );
				#endif
				} else {
					transform.position += yump * uf::physics::time::delta;
				}
			}
		}

		if ( keys.lookLeft ) {
			if ( collider.body && !collider.shared ) {
			#if UF_USE_BULLET
				ext::bullet::applyRotation( collider, transform.up, -speed.rotate );
			#endif
			} else {
				uf::transform::rotate( transform, transform.up, -speed.rotate );
			}
			stats.updateCamera = true;
		}
		if ( keys.lookRight ) {
			if ( collider.body && !collider.shared ) {
			#if UF_USE_BULLET
				ext::bullet::applyRotation( collider, transform.up, speed.rotate );
			#endif
			} else {
				uf::transform::rotate( transform, transform.up, speed.rotate );
			}
			stats.updateCamera = true;
		}
		if ( keys.crouch ) {
			pod::Vector3f yump = metadata.system.physics.jump;
			if ( stats.noclipped ) {
				if ( collider.body && !collider.shared ) {
					if ( fabs(yump.x) > 0.001f ) physics.linear.velocity.x = -yump.x;
					if ( fabs(yump.y) > 0.001f ) physics.linear.velocity.y = -yump.y;
					if ( fabs(yump.z) > 0.001f ) physics.linear.velocity.z = -yump.z;
				#if UF_USE_BULLET
					ext::bullet::move( collider, physics.linear.velocity );
				#endif
				}
			} else {
				if ( !metadata.system.physics.collision ) {
					transform.position -= yump * uf::physics::time::delta;
				} else {
					if ( !metadata.system.crouching )  stats.deltaCrouch = true;
					metadata.system.crouching = true;
				}
			}
		} else {
			if ( metadata.system.crouching ) stats.deltaCrouch = true;
			metadata.system.crouching = false;
		}
	}
	if ( stats.noclipped && !keys.forward && !keys.backwards && !keys.left && !keys.right && !keys.jump && !keys.crouch ) {
		if ( collider.body && !collider.shared ) {
			physics.linear.velocity = {};
		#if UF_USE_BULLET
			ext::bullet::move( collider, physics.linear.velocity );
		#endif
		}
	}
	if ( stats.deltaCrouch ) {
		float delta = metadata.system.physics.crouch;
		if ( metadata.system.crouching ) camera.getTransform().position.y -= delta;
		else camera.getTransform().position.y += delta;
		stats.updateCamera = true;
	}
#if UF_USE_OPENAL
	if ( stats.floored ) {
		if ( stats.walking ) {
			auto& emitter = this->getComponent<uf::MappedSoundEmitter>();
			int cycle = rand() % metadata.audio.footstep.list.size();
			std::string filename = metadata.audio.footstep.list[cycle];
			uf::Audio& footstep = emitter.add(filename);

			bool playing = false;
			for ( uint i = 0; i < metadata.audio.footstep.list.size(); ++i ) {
				uf::Audio& audio = emitter.add(metadata.audio.footstep.list[i]);
				if ( audio.playing() ) playing = true;
			}
			if ( !playing ) {
				footstep.play();
				footstep.setVolume(metadata.audio.footstep.volume);
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
#endif
#if !UF_ENV_DREAMCAST
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
#endif
#if UF_USE_LUA
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
#endif
	if ( stats.updateCamera ) camera.updateView();

#if UF_ENTITY_METADATA_USE_JSON
	auto& metadataJson = this->getComponent<uf::Serializer>();
	auto& metadataSystem = metadataJson["system"]
	metadataSystem["menu"] = metadata.system.menu;
	metadataSystem["control"] = metadata.system.control;
	metadataSystem["crouching"] = metadata.system.crouching;
	metadataSystem["noclipped"] = metadata.system.noclipped;
	auto& metadataSystemPhysics = metadataSystem["physics"]
	metadataSystemPhysics["impulse"] = metadata.system.physics.impulse;
	metadataSystemPhysics["rotate"] = metadata.system.physics.rotate;
	metadataSystemPhysics["move"] = metadata.system.physics.move;
	metadataSystemPhysics["run"] = metadata.system.physics.run;
	metadataSystemPhysics["walk"] = metadata.system.physics.walk;
	metadataSystemPhysics["collision"] = metadata.system.physics.collision;
	metadataSystemPhysics["jump"] = uf::vector::encode(metadata.system.physics.jump);
	metadataSystemPhysics["crouch"] = metadata.system.physics.crouch;
	auto& metadataAudioFootstep = metadata["audio"]["footstep"];
	metadataAudioFootstep["list"] = metadata.audio.footstep.list;
	metadataAudioFootstep["volume"] = metadata.audio.footstep.volume
#endif
}

void ext::PlayerBehavior::render( uf::Object& self ){}
void ext::PlayerBehavior::destroy( uf::Object& self ){}
#undef this