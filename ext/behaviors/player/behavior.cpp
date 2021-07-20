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
UF_BEHAVIOR_TRAITS_CPP(ext::PlayerBehavior, ticks = true, renders = false, multithread = false)
#define this (&self)
void ext::PlayerBehavior::initialize( uf::Object& self ) {
	auto& transform = this->getComponent<pod::Transform<>>();

	auto& collider = this->getComponent<pod::Bullet>();

	auto& metadata = this->getComponent<ext::PlayerBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();
	
	auto& camera = this->getComponent<uf::Camera>();
	auto& cameraTransform = camera.getTransform();
	{
		camera.setStereoscopic(true);
				
		cameraTransform.position = uf::vector::decode( metadataJson["camera"]["position"], cameraTransform.position );
		cameraTransform.scale = uf::vector::decode( metadataJson["camera"]["scale"], cameraTransform.scale );
		cameraTransform.reference = &transform;

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
			pod::Vector2f range = uf::vector::decode( metadataJson["camera"]["settings"]["clip"], pod::Vector2f{ 0.1, 64.0f } );
			pod::Vector2ui size = uf::vector::decode( metadataJson["camera"]["settings"]["size"], pod::Vector2ui{} );
			float raidou = (float) size.x / (float) size.y;

			if ( size.x == 0 || size.y == 0 )  {
				size = uf::vector::decode( ext::config["window"]["size"], pod::Vector2ui{} );
				raidou = (float) size.x / (float) size.y;
				this->addHook( "window:Resized", [&, fov, range](ext::json::Value& json){
					pod::Vector2ui size = uf::vector::decode( json["window"]["size"], pod::Vector2ui{} );
					float raidou = (float) size.x / (float) size.y;
					camera.setProjection( uf::matrix::perspective( fov, raidou, range.x, range.y ) );
				} );
			}
			camera.setProjection( uf::matrix::perspective( fov, raidou, range.x, range.y ) );
		}
		camera.update(true);
	}
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

	this->addHook( "object:UpdateMetadata.%UID%", [&](ext::json::Value& json){	
		metadata.deserialize(self, metadataJson);
	});
	metadata.deserialize(self, metadataJson);
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
	metadata.deserialize(self, metadataJson);
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
		uf::Serializer payload;
		uf::hooks.call("menu:Pause", payload);
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
	metadata.serialize(self, metadataJson);
#endif
}

void ext::PlayerBehavior::render( uf::Object& self ){}
void ext::PlayerBehavior::destroy( uf::Object& self ){}
void ext::PlayerBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ){
	auto& serializerSystem = serializer["system"];
	auto& serializerSystemPhysics = serializerSystem["physics"];
	auto& serializerAudioFootstep = serializer["audio"]["footstep"];
	auto& serializerCamera = serializer["camera"];
	auto& serializerCameraLimit = serializerCamera["limit"];
	
	serializerSystem["menu"] = /*this->*/system.menu;
	serializerSystem["control"] = /*this->*/system.control;
	serializerSystem["crouching"] = /*this->*/system.crouching;
	serializerSystem["noclipped"] = /*this->*/system.noclipped;
	serializerSystemPhysics["impulse"] = /*this->*/system.physics.impulse;
	serializerSystemPhysics["rotate"] = /*this->*/system.physics.rotate;
	serializerSystemPhysics["move"] = /*this->*/system.physics.move;
	serializerSystemPhysics["run"] = /*this->*/system.physics.run;
	serializerSystemPhysics["walk"] = /*this->*/system.physics.walk;
	serializerSystemPhysics["collision"] = /*this->*/system.physics.collision;
	serializerSystemPhysics["jump"] = uf::vector::encode(/*this->*/system.physics.jump);
	serializerSystemPhysics["crouch"] = /*this->*/system.physics.crouch;
	serializerAudioFootstep["list"] = /*this->*/audio.footstep.list;
	serializerAudioFootstep["volume"] = /*this->*/audio.footstep.volume;
	serializerCamera["invert"] = uf::vector::encode(/*this->*/camera.invert);
	serializerCameraLimit["current"] = uf::vector::encode(/*this->*/camera.limit.current);
	serializerCameraLimit["minima"] = uf::vector::encode(/*this->*/camera.limit.min);
	serializerCameraLimit["maxima"] = uf::vector::encode(/*this->*/camera.limit.max);
}
void ext::PlayerBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ){
	auto& serializerSystem = serializer["system"];
	auto& serializerAudioFootstep = serializer["audio"]["footstep"];
	auto& serializerSystemPhysics = serializerSystem["physics"];
	auto& serializerCamera = serializer["camera"];
	auto& serializerCameraLimit = serializerCamera["limit"];

	/*this->*/system.menu = serializerSystem["menu"].as<uf::stl::string>();
	/*this->*/system.control = serializerSystem["control"].as<bool>();
	/*this->*/system.crouching = serializerSystem["crouching"].as<bool>();
	/*this->*/system.noclipped = serializerSystem["noclipped"].as<bool>();
	/*this->*/system.physics.impulse = serializerSystemPhysics["impulse"].as<bool>();
	/*this->*/system.physics.rotate = serializerSystemPhysics["rotate"].as<float>();
	/*this->*/system.physics.move = serializerSystemPhysics["move"].as<float>();
	/*this->*/system.physics.run = serializerSystemPhysics["run"].as<float>();
	/*this->*/system.physics.walk = serializerSystemPhysics["walk"].as<float>();
	/*this->*/system.physics.collision = serializerSystemPhysics["collision"].as<bool>();
	/*this->*/system.physics.jump = uf::vector::decode(serializerSystemPhysics["jump"], pod::Vector3f{});
	/*this->*/system.physics.crouch = serializerSystemPhysics["crouch"].as<float>();
	ext::json::forEach( serializerAudioFootstep["list"], [&]( const ext::json::Value& value ){
		/*this->*/audio.footstep.list.emplace_back(value);
	});
	/*this->*/audio.footstep.volume = serializerAudioFootstep["volume"].as<float>();

	/*this->*/camera.invert = uf::vector::decode( serializerCamera["invert"], pod::Vector3t<bool>{false, false, false} );
	/*this->*/camera.limit.current = uf::vector::decode( serializerCameraLimit["current"], pod::Vector3f{NAN, NAN, NAN} );
	/*this->*/camera.limit.min = uf::vector::decode( serializerCameraLimit["minima"], pod::Vector3f{NAN, NAN, NAN} );
	/*this->*/camera.limit.max = uf::vector::decode( serializerCameraLimit["maxima"], pod::Vector3f{NAN, NAN, NAN} );

//	for ( uint_fast8_t i = 0; i < 3; ++i ) this->camera.limit.current[i] = serializerCameraLimit["current"][i].as<float>(NAN);
//	for ( uint_fast8_t i = 0; i < 3; ++i ) this->camera.limit.min[i] = serializerCameraLimit["minima"][i].as<float>(NAN);
//	for ( uint_fast8_t i = 0; i < 3; ++i ) this->camera.limit.max[i] = serializerCameraLimit["maxima"][i].as<float>(NAN);
}
#undef this