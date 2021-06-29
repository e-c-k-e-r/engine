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
#include <uf/ext/bullet/bullet.h>
#include <uf/utils/math/physics.h>
#include <uf/spec/controller/controller.h>

#include <sstream>
UF_BEHAVIOR_REGISTER_CPP(ext::PlayerBehavior)
#define this (&self)
void ext::PlayerBehavior::initialize( uf::Object& self ) {
	auto& transform = this->getComponent<pod::Transform<>>();
	auto& camera = this->getComponent<uf::Camera>();
	auto& cameraTransform = camera.getTransform();
	cameraTransform.reference = &transform;

	auto& collider = this->getComponent<pod::Bullet>();

	auto& metadata = this->getComponent<ext::PlayerBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();

#if 1
	{
		camera.setStereoscopic(true);
		
		pod::Transform<>& transform = camera.getTransform();
		
		transform.position = uf::vector::decode( metadataJson["camera"]["position"], transform.position );
		transform.scale = uf::vector::decode( metadataJson["camera"]["scale"], transform.scale );

		if ( metadataJson["camera"]["ortho"].as<bool>() ) {
			float l = metadataJson["camera"]["settings"]["left"].as<float>();
			float r = metadataJson["camera"]["settings"]["right"].as<float>();
			float b = metadataJson["camera"]["settings"]["bottom"].as<float>();
			float t = metadataJson["camera"]["settings"]["top"].as<float>();
			float n = metadataJson["camera"]["settings"]["near"].as<float>();
			float f = metadataJson["camera"]["settings"]["far"].as<float>();
			
			camera.setProjection( uf::matrix::orthographic( l, r, b, t, n, f ) );
		} else {
			float fov = metadataJson["camera"]["settings"]["fov"].as<float>(120) * (3.14159265358f / 180.0f);
			float znear = metadataJson["camera"]["settings"]["clip"]["near"].as<float>();
			float zfar = metadataJson["camera"]["settings"]["clip"]["far"].as<float>();
			
			pod::Vector2ui size = uf::vector::decode( metadataJson["camera"]["settings"]["size"], pod::Vector2ui{} );
			float raidou = (float) size.x / (float) size.y;

			if ( size.x == 0 || size.y == 0 )  {
				size = uf::vector::decode( ext::config["window"]["size"], pod::Vector2ui{} );
				raidou = (float) size.x / (float) size.y;
				this->addHook( "window:Resized", [&, fov, znear, zfar](ext::json::Value& json){
					pod::Vector2ui size = uf::vector::decode( json["window"]["size"], pod::Vector2ui{} );
					float raidou = (float) size.x / (float) size.y;
					camera.setProjection( uf::matrix::perspective( fov, raidou, znear, zfar ) );
				} );
			}
			camera.setProjection( uf::matrix::perspective( fov, raidou, znear, zfar ) );

		}

		camera.update(true);
	}
#else
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

		settings.mode = metadataJson["camera"]["ortho"].as<bool>() ? -1 : 1;

		settings.perspective.size = uf::vector::decode( metadataJson["camera"]["settings"]["size"], pod::Vector3f{} );
		// Update viewport
		if ( settings.perspective.size.x <= 0 || settings.perspective.size.y <= 0 )  {
			settings.perspective.size = uf::vector::decode( ext::config["window"]["size"], pod::Vector2ui{} );
			this->addHook( "window:Resized", [&](ext::json::Value& json){
				// Update persistent window sized (size stored to JSON file)
				pod::Vector2ui size = uf::vector::decode( json["window"]["size"], pod::Vector2ui{} ); {
				//	size.x = json["window"]["size"]["x"].as<size_t>();
				//	size.y = json["window"]["size"]["y"].as<size_t>();
				}
				/* Update camera's viewport */ {
					uf::Camera& camera = this->getComponent<uf::Camera>();
					camera.setSize({(pod::Math::num_t)size.x, (pod::Math::num_t)size.y});
				}
			} );
		}
		camera.setSize(settings.perspective.size);
		if ( settings.mode < 0 ) {
			settings.ortho.lr.x 	= metadataJson["camera"]["settings"]["left"].as<double>();
			settings.ortho.lr.y 	= metadataJson["camera"]["settings"]["right"].as<double>();
			settings.ortho.bt.x 	= metadataJson["camera"]["settings"]["bottom"].as<double>();
			settings.ortho.bt.y 	= metadataJson["camera"]["settings"]["top"].as<double>();
			settings.ortho.nf.x 	= metadataJson["camera"]["settings"]["near"].as<double>();
			settings.ortho.nf.y 	= metadataJson["camera"]["settings"]["far"].as<double>();

			camera.ortho( settings.ortho.lr, settings.ortho.bt, settings.ortho.nf );
		} else {
			settings.perspective.fov 		= metadataJson["camera"]["settings"]["fov"].as<double>();
			settings.perspective.bounds.x 	= metadataJson["camera"]["settings"]["clip"]["near"].as<double>();
			settings.perspective.bounds.y 	= metadataJson["camera"]["settings"]["clip"]["far"].as<double>();

			camera.setFov(settings.perspective.fov);
			camera.setBounds(settings.perspective.bounds);
		}
		camera.setStereoscopic(true);

		pod::Transform<>& transform = camera.getTransform();
		/* Transform initialization */ {
			transform.position.x = metadataJson["camera"]["position"][0].as<double>();
			transform.position.y = metadataJson["camera"]["position"][1].as<double>();
			transform.position.z = metadataJson["camera"]["position"][2].as<double>();

			transform.scale.x = metadataJson["camera"]["scale"][0].as<double>();
			transform.scale.y = metadataJson["camera"]["scale"][1].as<double>();
			transform.scale.z = metadataJson["camera"]["scale"][2].as<double>();
		}
		camera.update(true);
	}
#endif
	metadataJson["system"]["control"] = true;
	this->addHook( "window:Mouse.CursorVisibility", [&](ext::json::Value& json){
		metadata.system.control = !json["state"].as<bool>();
	});

	// Rotate Camera
	this->addHook( "window:Mouse.Moved", [&](pod::payloads::windowMouseMoved& payload ){
		float sensitivity = 2;
		pod::Vector2 relta = { (float) sensitivity * payload.mouse.delta.x / payload.window.size.x, (float) sensitivity * payload.mouse.delta.y / payload.window.size.y };
		if ( (payload.mouse.delta.x == 0 && payload.mouse.delta.y == 0) || !metadata.system.control ) return;

		bool updateCamera = false;
		if ( payload.mouse.delta.x != 0 ) {
			if ( metadata.camera.invert.x ) relta.x *= -1;
			metadata.camera.limit.current.x += relta.x;
			if ( metadata.camera.limit.current.x != metadata.camera.limit.current.x || ( metadata.camera.limit.current.x < metadata.camera.limit.max.x && metadata.camera.limit.current.x > metadata.camera.limit.min.x ) ) {
				if ( collider.body && !collider.shared ) {
				#if UF_USE_BULLET
					ext::bullet::applyRotation( collider, transform.up, relta.x );
				#endif
				} else {
					uf::transform::rotate( transform, transform.up, relta.x ), updateCamera = true;
				}
			} else metadata.camera.limit.current.x -= relta.x;
		}
		if ( payload.mouse.delta.y != 0 ) {
			if ( metadata.camera.invert.y ) relta.y *= -1;
			metadata.camera.limit.current.y += relta.y;
			if ( metadata.camera.limit.current.y != metadata.camera.limit.current.y || ( metadata.camera.limit.current.y < metadata.camera.limit.max.y && metadata.camera.limit.current.y > metadata.camera.limit.min.y ) ) {
			//	if ( collider.body && !collider.shared ) {
			//		ext::bullet::applyRotation( collider, cameraTransform.right, relta.y );
			//	} else {
					uf::transform::rotate( cameraTransform, cameraTransform.right, relta.y );
			//	}
				updateCamera = true;
			} else metadata.camera.limit.current.y -= relta.y;
		}
		if ( updateCamera ) camera.update(true);
	});
#if 0
	this->addHook( ":Update.%UID%", [&](ext::json::Value& json){
	//	for ( auto& member : json[""]["transients"] ) {
		ext::json::forEach(metadataJson[""]["transients"], [&](ext::json::Value& member){
			if ( member["type"] != "player" ) return;
			uf::stl::string id = member["id"].as<uf::stl::string>();
			metadataJson[""]["transients"][id]["hp"] = member["hp"];
			metadataJson[""]["transients"][id]["mp"] = member["mp"];
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
		
		metadataJson["system"].erase("battle");
		metadataJson["system"]["cooldown"] = uf::physics::time::current + 5;
	});
	
	// detect collision against transients, engage in battle
	this->addHook( "world:Collision.%UID%", [&](ext::json::Value& json){
		if ( metadataJson["system"]["cooldown"].as<float>() > uf::physics::time::current ) return;
		if ( !metadataJson["system"]["control"].as<bool>() ) return;
		
		uf::stl::string state = metadataJson["system"]["state"].as<uf::stl::string>();
		if ( state != "" && state != "null" ) return;

		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Entity* entity = scene.findByUid(json["entity"].as<size_t>());

		if ( !entity ) return;

		uf::Serializer& pMetadata = entity->getComponent<uf::Serializer>();

		uf::stl::string onCollision = pMetadata["system"]["onCollision"].as<uf::stl::string>();

		if ( onCollision == "battle" ) {
			uf::Serializer payload;
			payload["battle"]["enemy"] = pMetadata[""];
			payload["battle"]["enemy"]["uid"] = json["entity"].as<size_t>();
			payload["battle"]["player"] = metadataJson[""];
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
	//	metadataJson["system"]["cooldown"] = uf::physics::time::current + 5;
		metadataJson["system"]["control"] = false;
		metadataJson["system"]["menu"] = onCollision;
	});
	this->addHook( "world:Battle.End", [&](ext::json::Value& json){
		metadataJson["system"]["menu"] = "";
		metadataJson["system"]["cooldown"] = uf::physics::time::current + 5;

		// update
		{
			uf::Serializer payload;
			payload[""]["transients"] = json["battle"]["transients"];
			this->callHook( ":Update.%UID%", payload );
		}
	});
	this->addHook( "menu:Dialogue.End", [&](ext::json::Value& json){
		metadataJson["system"]["menu"] = "";
		metadataJson["system"]["cooldown"] = uf::physics::time::current + 1;
	});
#endif
#if 0
#if UF_USE_DISCORD
	// Discord Integration
	this->addHook( "discord.Activity.Update.%UID%", [&](ext::json::Value& json){
		uf::stl::string leaderId = metadataJson[""]["party"][0].as<uf::stl::string>();
		uf::Serializer cardData = masterDataGet("Card", leaderId);
		uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].as<uf::stl::string>());
		uf::stl::string leader = charaData["name"].as<uf::stl::string>();

		uf::Serializer payload = json;
		payload["details"] = "Leader: " + leader;
		uf::hooks.call( "discord:Activity.Update", payload );
	});
	this->queueHook("discord.Activity.Update.%UID%", ext::json::null(), 1.0);
#endif
#endif
	metadata.serialize = [&](){
		auto& metadataSystem = metadataJson["system"];
		auto& metadataSystemPhysics = metadataSystem["physics"];
		auto& metadataAudioFootstep = metadataJson["audio"]["footstep"];
		auto& metadataCamera = metadataJson["camera"];
		auto& metadataCameraLimit = metadataCamera["limit"];
		
		metadataSystem["menu"] = metadata.system.menu;
		metadataSystem["control"] = metadata.system.control;
		metadataSystem["crouching"] = metadata.system.crouching;
		metadataSystem["noclipped"] = metadata.system.noclipped;
		metadataSystemPhysics["impulse"] = metadata.system.physics.impulse;
		metadataSystemPhysics["rotate"] = metadata.system.physics.rotate;
		metadataSystemPhysics["move"] = metadata.system.physics.move;
		metadataSystemPhysics["run"] = metadata.system.physics.run;
		metadataSystemPhysics["walk"] = metadata.system.physics.walk;
		metadataSystemPhysics["collision"] = metadata.system.physics.collision;
		metadataSystemPhysics["jump"] = uf::vector::encode(metadata.system.physics.jump);
		metadataSystemPhysics["crouch"] = metadata.system.physics.crouch;
		metadataAudioFootstep["list"] = metadata.audio.footstep.list;
		metadataAudioFootstep["volume"] = metadata.audio.footstep.volume;
		metadataCamera["invert"] = uf::vector::encode(metadata.camera.invert);
		metadataCameraLimit["current"] = uf::vector::encode(metadata.camera.limit.current);
		metadataCameraLimit["minima"] = uf::vector::encode(metadata.camera.limit.min);
		metadataCameraLimit["maxima"] = uf::vector::encode(metadata.camera.limit.max);
	};
	metadata.deserialize = [&](){
		auto& metadataSystem = metadataJson["system"];
		auto& metadataAudioFootstep = metadataJson["audio"]["footstep"];
		auto& metadataSystemPhysics = metadataSystem["physics"];
		auto& metadataCamera = metadataJson["camera"];
		auto& metadataCameraLimit = metadataCamera["limit"];

		metadata.system.menu = metadataSystem["menu"].as<uf::stl::string>();
		metadata.system.control = metadataSystem["control"].as<bool>();
		metadata.system.crouching = metadataSystem["crouching"].as<bool>();
		metadata.system.noclipped = metadataSystem["noclipped"].as<bool>();
		metadata.system.physics.impulse = metadataSystemPhysics["impulse"].as<bool>();
		metadata.system.physics.rotate = metadataSystemPhysics["rotate"].as<float>();
		metadata.system.physics.move = metadataSystemPhysics["move"].as<float>();
		metadata.system.physics.run = metadataSystemPhysics["run"].as<float>();
		metadata.system.physics.walk = metadataSystemPhysics["walk"].as<float>();
		metadata.system.physics.collision = metadataSystemPhysics["collision"].as<bool>();
		metadata.system.physics.jump = uf::vector::decode(metadataSystemPhysics["jump"], pod::Vector3f{});
		metadata.system.physics.crouch = metadataSystemPhysics["crouch"].as<float>();
		ext::json::forEach( metadataAudioFootstep["list"], [&]( const ext::json::Value& value ){
			metadata.audio.footstep.list.emplace_back(value);
		});
		metadata.audio.footstep.volume = metadataAudioFootstep["volume"].as<float>();

		metadata.camera.invert = uf::vector::decode( metadataCamera["invert"], pod::Vector3t<bool>{false, false, false} );
		metadata.camera.limit.current = uf::vector::decode( metadataCameraLimit["current"], pod::Vector3f{NAN, NAN, NAN} );
		metadata.camera.limit.min = uf::vector::decode( metadataCameraLimit["minima"], pod::Vector3f{NAN, NAN, NAN} );
		metadata.camera.limit.max = uf::vector::decode( metadataCameraLimit["maxima"], pod::Vector3f{NAN, NAN, NAN} );

	//	for ( uint_fast8_t i = 0; i < 3; ++i ) metadata.camera.limit.current[i] = metadataCameraLimit["current"][i].as<float>(NAN);
	//	for ( uint_fast8_t i = 0; i < 3; ++i ) metadata.camera.limit.min[i] = metadataCameraLimit["minima"][i].as<float>(NAN);
	//	for ( uint_fast8_t i = 0; i < 3; ++i ) metadata.camera.limit.max[i] = metadataCameraLimit["maxima"][i].as<float>(NAN);
	};
	this->addHook( "object:UpdateMetadata.%UID%", metadata.deserialize);
	metadata.deserialize();
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
		uf::stl::string menu = "";
		uf::stl::string targetAnimation = "";
	} stats;

	auto& metadata = this->getComponent<ext::PlayerBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();
#if UF_ENTITY_METADATA_USE_JSON
	metadata.deserialize();
#endif
	stats.menu = metadata.system.menu;
	stats.impulse = metadata.system.physics.impulse;
	stats.noclipped = metadata.system.noclipped;
	stats.floored = fabs(physics.linear.velocity.y) < 0.01f || stats.noclipped;
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
		
		if ( stats.noclipped ) {
			speed.move *= 4.0;
			speed.run *= 2.0;
		}
	}
	if ( !metadata.system.physics.collision ) {
		stats.impulse = true;
	}
	if ( keys.running ) speed.move *= speed.run;
	else if ( keys.walk ) speed.move *= speed.walk;
	speed.limitSquared = speed.move * speed.move;

	uf::Object* menu = (uf::Object*) scene.globalFindByName("Gui: Menu");
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
				
				UF_MSG_DEBUG( (state ? "En" : "Dis") << "abled noclip: " << uf::vector::toString(transform.position));
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
				
				if ( !stats.noclipped ) {
					translator.forward *= { 1, 0, 1 };
					translator.right *= { 1, 0, 1 };
				}

				translator.forward = uf::vector::normalize( translator.forward );
				translator.right = uf::vector::normalize( translator.right );
			} else
		#endif
			if ( stats.noclipped ){
				auto& cameraTransform = camera.getTransform();
			//	translator = uf::transform::flatten( cameraTransform );
				translator.forward.y += cameraTransform.forward.y;
			}
			pod::Vector3f queued = {};
			if ( keys.forward || keys.backwards ) {
				float polarity = keys.forward ? 1 : -1;
				float mag = uf::vector::magnitude(physics.linear.velocity); // * pod::Vector3{1, 0, 1});
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
						
						if ( stats.noclipped ) physics.linear.velocity.y = correction.y;
					} else {
						correction *= uf::physics::time::delta;
						transform.position.x += correction.x;
						transform.position.z += correction.z;
						
						if ( stats.noclipped ) transform.position.y += correction.y;
					}
				}
				stats.updateCamera = (stats.walking = true);
			}
			if ( keys.left || keys.right ) {
				float polarity = keys.right ? 1 : -1;
				float mag = uf::vector::magnitude(physics.linear.velocity); // * pod::Vector3{1, 0, 1});
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
					
					if ( stats.noclipped ) physics.linear.velocity.y = queued.y;
				#if UF_USE_BULLET
					ext::bullet::move( collider, physics.linear.velocity );
				#endif
				}
			}
			if ( !keys.forward && !keys.backwards && !keys.left && !keys.right ) {
				if ( collider.body && !collider.shared ) {
					physics.linear.velocity.x = 0;
					physics.linear.velocity.z = 0;
					
					if ( stats.noclipped ) physics.linear.velocity.y = 0;
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
					transform.position += uf::vector::multiply(yump, uf::physics::time::delta);
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
					transform.position -= uf::vector::multiply(yump, uf::physics::time::delta);
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
			uf::stl::string filename = metadata.audio.footstep.list[cycle];
			uf::Audio& footstep = emitter.has(filename) ? emitter.get(filename) : emitter.load(filename);

			bool playing = false;
			for ( uint i = 0; i < metadata.audio.footstep.list.size(); ++i ) {
				if ( !emitter.has(metadata.audio.footstep.list[i]) ) continue;
				uf::Audio& audio = emitter.get( metadata.audio.footstep.list[i] );
				if ( audio.playing() ) playing = true;
			}
			if ( !playing ) {
				// [0, 1]
				float modulation = (rand() % 100) / 100.0;
				// [0, 0.1]
				modulation *= 0.1f;
				// [-0.05, 0.05]
				modulation -= 0.05f;
				if ( keys.running ) modulation += 0.5f;
				footstep.setPitch( 1 + modulation );
				footstep.setVolume( metadata.audio.footstep.volume );
				footstep.setPosition( transform.position );

				footstep.setTime( 0 );
				footstep.play();
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
#if UF_USE_LUA && !UF_ENV_DREAMCAST
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
	if ( stats.updateCamera ) camera.update(true);

#if UF_ENTITY_METADATA_USE_JSON
	metadata.serialize();
#endif
}

void ext::PlayerBehavior::render( uf::Object& self ){}
void ext::PlayerBehavior::destroy( uf::Object& self ){}
#undef this