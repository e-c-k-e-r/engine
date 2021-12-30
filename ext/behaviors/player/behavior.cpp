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

#include <sstream>

UF_BEHAVIOR_REGISTER_CPP(ext::PlayerBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::PlayerBehavior, ticks = true, renders = false, multithread = false)
#define this (&self)
void ext::PlayerBehavior::initialize( uf::Object& self ) {
	auto& transform = this->getComponent<pod::Transform<>>();

	auto& collider = this->getComponent<pod::PhysicsState>();

	auto& metadata = this->getComponent<ext::PlayerBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();
	
	auto& camera = this->getComponent<uf::Camera>();
	auto& cameraTransform = camera.getTransform();
	
	auto& scene = uf::scene::getCurrentScene();
	{
		camera.setStereoscopic(true);
				
		cameraTransform.position = uf::vector::decode( metadataJson["camera"]["position"], cameraTransform.position );
		cameraTransform.scale = uf::vector::decode( metadataJson["camera"]["scale"], cameraTransform.scale );
		cameraTransform.reference = &transform;

		auto& cameraSettingsJson = metadataJson["camera"]["settings"];

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
			pod::Vector2f range = uf::vector::decode( cameraSettingsJson["clip"], pod::Vector2f{ 0.1, 64.0f } );
			pod::Vector2ui size = uf::vector::decode( cameraSettingsJson["size"], pod::Vector2ui{} );
			float raidou = (float) size.x / (float) size.y;

			if ( size.x == 0 || size.y == 0 )  {
				size = uf::vector::decode( ext::config["window"]["size"], pod::Vector2ui{} );
				raidou = (float) size.x / (float) size.y;

				this->addHook( "window:Resized", [&, fov, range](pod::payloads::windowResized& payload){
					float raidou = (float) payload.window.size.x / (float) payload.window.size.y;
					camera.setProjection( uf::matrix::perspective( fov, raidou, range.x, range.y ) );
				} );
			}
			camera.setProjection( uf::matrix::perspective( fov, raidou, range.x, range.y ) );
		}
		camera.update(true);
	}
	
	// sloppy
	metadata.mouse.sensitivity = uf::vector::decode( ext::config["window"]["cursor"]["sensitivity"], metadata.mouse.sensitivity );

	this->addHook( "window:Mouse.CursorVisibility", [&](pod::payloads::windowMouseCursorVisibility& payload){
		metadata.system.control = !payload.mouse.visible;
	});

	// Rotate Camera
	this->addHook( "window:Mouse.Moved", [&](pod::payloads::windowMouseMoved& payload ){
		pod::Vector2 relta = { (float) metadata.mouse.sensitivity.x * payload.mouse.delta.x / payload.window.size.x, (float) metadata.mouse.sensitivity.y * payload.mouse.delta.y / payload.window.size.y };
		if ( (payload.mouse.delta.x == 0 && payload.mouse.delta.y == 0) || !metadata.system.control ) return;

		if ( payload.mouse.delta.x != 0 ) {
			if ( metadata.camera.invert.x ) relta.x *= -1;
			metadata.camera.limit.current.x += relta.x;
			if ( metadata.camera.limit.current.x != metadata.camera.limit.current.x || ( metadata.camera.limit.current.x < metadata.camera.limit.max.x && metadata.camera.limit.current.x > metadata.camera.limit.min.x ) ) {
			if ( collider.body ) uf::physics::impl::applyRotation( collider, transform.up, relta.x ); else
				uf::transform::rotate( transform, transform.up, relta.x );
			} else metadata.camera.limit.current.x -= relta.x;
		}
		if ( payload.mouse.delta.y != 0 ) {
			if ( metadata.camera.invert.y ) relta.y *= -1;
			metadata.camera.limit.current.y += relta.y;
			if ( metadata.camera.limit.current.y != metadata.camera.limit.current.y || ( metadata.camera.limit.current.y < metadata.camera.limit.max.y && metadata.camera.limit.current.y > metadata.camera.limit.min.y ) ) {
			//	if ( collider.body && !collider.shared ) {
			//		uf::physics::impl::applyRotation( collider, cameraTransform.right, relta.y );
			//	} else {
					uf::transform::rotate( cameraTransform, cameraTransform.right, relta.y );
			//	}
			} else metadata.camera.limit.current.y -= relta.y;
		}
		camera.update(true);
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

	this->addHook( "object:Serialize.%UID%", [&](ext::json::Value& json){ metadata.serialize(self, metadataJson); });
	this->addHook( "object:Deserialize.%UID%", [&](ext::json::Value& json){	 metadata.deserialize(self, metadataJson); });
	metadata.deserialize(self, metadataJson);
}
void ext::PlayerBehavior::tick( uf::Object& self ) {
	auto& camera = this->getComponent<uf::Camera>();
	auto& cameraTransform = camera.getTransform();

	auto& transform = this->getComponent<pod::Transform<>>();
	auto& physics = this->getComponent<pod::Physics>();
	auto& scene = uf::scene::getCurrentScene();

	struct {
		bool forward = false;
		bool backwards = false;
		bool left = false;
		bool right = false;
	
		bool lookLeft = false;
		bool lookRight = false;
		bool running = false;
		bool walk = false;
		bool jump = false;
		bool crouch = false;
		bool paused = false;
		bool vee = false;
		bool use = false;
	} keys;

	if ( uf::Window::focused ) {
		keys = {
			.forward = uf::inputs::kbm::states::W,
			.backwards = uf::inputs::kbm::states::S,
			.left = uf::inputs::kbm::states::A,
			.right = uf::inputs::kbm::states::D,
			.lookLeft = uf::inputs::kbm::states::Left,
			.lookRight = uf::inputs::kbm::states::Right,
			.running = uf::inputs::kbm::states::LShift,
			.walk = uf::inputs::kbm::states::LAlt,
			.jump = uf::inputs::kbm::states::Space,
			.crouch = uf::inputs::kbm::states::LControl,
			.paused = uf::inputs::kbm::states::Escape,
			.vee = uf::inputs::kbm::states::V,
			.use = uf::inputs::kbm::states::E,
		};
		if ( spec::controller::connected() ) {
		#if UF_USE_OPENVR
			if ( uf::inputs::controller::states::R_DPAD_UP ) keys.forward = true;
			if ( uf::inputs::controller::states::R_DPAD_DOWN ) keys.backwards = true;
			if ( uf::inputs::controller::states::R_DPAD_LEFT ) keys.lookLeft = true; // keys.left = true;
			if ( uf::inputs::controller::states::R_DPAD_RIGHT ) keys.lookRight = true; // keys.right = true;
			if ( uf::inputs::controller::states::R_JOYSTICK ) keys.running = true;
			if ( uf::inputs::controller::states::R_A ) keys.jump = true;

			if ( uf::inputs::controller::states::L_DPAD_UP ) keys.forward = true;
			if ( uf::inputs::controller::states::L_DPAD_DOWN ) keys.backwards = true;
			if ( uf::inputs::controller::states::L_DPAD_LEFT ) keys.lookLeft = true;
			if ( uf::inputs::controller::states::L_DPAD_RIGHT ) keys.lookRight = true;
			if ( uf::inputs::controller::states::L_JOYSTICK ) keys.crouch = true, keys.walk = true;
			if ( uf::inputs::controller::states::L_A ) keys.paused = true;
		#else
			if ( uf::inputs::controller::states::DPAD_UP ) keys.forward = true;
			if ( uf::inputs::controller::states::DPAD_DOWN ) keys.backwards = true;
			if ( uf::inputs::controller::states::DPAD_LEFT ) keys.lookLeft = true;
			if ( uf::inputs::controller::states::DPAD_RIGHT ) keys.lookRight = true;
			if ( uf::inputs::controller::states::A ) keys.jump = true;
			if ( uf::inputs::controller::states::B ) keys.running = true;
			if ( uf::inputs::controller::states::X ) keys.crouch = true, keys.walk = true;
			if ( uf::inputs::controller::states::Y ) keys.vee = true;
			if ( uf::inputs::controller::states::L_TRIGGER ) keys.left = true;
			if ( uf::inputs::controller::states::R_TRIGGER ) keys.right = true;
			if ( uf::inputs::controller::states::START ) keys.paused = true;
		#endif
		}
	}

	struct {
		bool deltaCrouch = false;
		bool walking = false;
		bool floored = true;
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
	stats.noclipped = metadata.system.noclipped;
	stats.floored = stats.noclipped;
	auto& collider = this->getComponent<pod::PhysicsState>();
	if ( !stats.floored && collider.body && uf::physics::impl::rayCast( transform.position, transform.position - pod::Vector3f{0,1,0} ) >= 0.0f ) stats.floored = true; else
	stats.floored |= fabs(physics.linear.velocity.y) < 0.01f;

	TIMER(0.125, keys.use && ) {
		size_t uid = 0;
		uf::Object* pointer = NULL;
		float length = 4.0f;
		pod::Vector3f pos = transform.position + cameraTransform.position;
		pod::Vector3f dir = uf::vector::normalize( transform.forward + pod::Vector3f{ 0, cameraTransform.forward.y, 0 } ) * length;

		float depth = uf::physics::impl::rayCast( pos, pos + dir, pointer );
		if ( pointer ) { 
			uf::Serializer payload;
			payload["uid"] = this->getUid();
			payload["depth"] = depth;
			pointer->callHook( "entity:Use.%UID%", payload );
		} else {
			auto& emitter = this->getComponent<uf::MappedSoundEmitter>();
			uf::stl::string filename = "./ui/deny.ogg";
			uf::Audio& sfx = emitter.has(filename) ? emitter.get(filename) : emitter.load(filename);

			bool playing = false;
			if ( !sfx.playing() )  {
			#if UF_AUDIO_MAPPED_VOLUMES
				sfx.setVolume(uf::audio::volumes.count("sfx") > 0 ? uf::audio::volumes.at("sfx") : 1.0);
			#else
				sfx.setVolume(uf::audio::volumes::sfx);
			#endif
				sfx.setPosition( transform.position );
				sfx.setTime( 0 );
				sfx.play();
			}
		}
	}

	struct {
		float move = 4;
		float walk = 1;
		float run = 8;
		float rotate = 1;
		float friction = 0.8f;
		float air = 1.0f;
	} speed; {
		float scale = 1;
	#if UF_USE_OPENGL
		scale = 10;
	#endif

		speed.rotate = metadata.movement.rotate * uf::physics::time::delta;
		speed.move = metadata.movement.move * scale;
		speed.run = metadata.movement.run * scale;
		speed.walk = metadata.movement.walk * scale;
		speed.friction = metadata.movement.friction;
		speed.air = metadata.movement.air;
		
		if ( stats.noclipped ) {
			speed.move *= 4.0;
			speed.run *= 2.0;
		}
		if ( !stats.floored || stats.noclipped ) speed.friction = 1;
		if ( stats.noclipped ) physics.linear.velocity = {};
	}
	if ( keys.running ) speed.move = speed.run;
	else if ( keys.walk ) speed.move = speed.walk;

	uf::Object* menu = (uf::Object*) scene.globalFindByName("Gui: Menu");
	if ( !menu ) stats.menu = "";
	// make assumptions
	if ( stats.menu == "" && keys.paused ) {
		stats.menu = "paused";
		metadata.system.control = false;
		pod::payloads::menuOpen payload;
		payload.name = "pause";
		uf::hooks.call("menu:Open", payload);
	}
	else if ( !metadata.system.control ) {
		stats.menu = "menu";
	} else if ( stats.menu == "" ) {
		metadata.system.control = true;
	} else {
		metadata.system.control = false;
	}
	metadata.system.menu = stats.menu;

	pod::Transform<> translator = transform;
#if UF_USE_OPENVR
	// use the orientation of our controller to determine our target
	if ( ext::openvr::context ) {
		bool useController = true;
		translator.orientation = uf::quaternion::multiply( transform.orientation, useController ? ext::openvr::controllerQuaternion( vr::Controller_Hand::Hand_Right ) : ext::openvr::hmdQuaternion() );
		translator = uf::transform::reorient( translator );
		
		// flatten if not noclipped
		if ( !stats.noclipped ) {
			translator.forward *= { 1, 0, 1 };
			translator.right *= { 1, 0, 1 };
		}

		translator.forward = uf::vector::normalize( translator.forward );
		translator.right = uf::vector::normalize( translator.right );
	} else
#endif
	// un-flatted if noclipped
	if ( stats.noclipped ){
		translator.forward.y += cameraTransform.forward.y;
		translator.forward = uf::vector::normalize( translator.forward );
	}

	if ( metadata.system.control ) {
		// noclip handler
		TIMER(0.25, keys.vee && ) {
			bool state = !stats.noclipped;
			metadata.system.noclipped = state;
			
			UF_MSG_DEBUG( (state ? "En" : "Dis") << "abled noclip: " << uf::vector::toString(transform.position));
		#if 0
			if ( state ) {
				if ( collider.body ) {
					collider.body->setGravity(btVector3(0,0.0,0));
					collider.body->setCollisionFlags(collider.body->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
				}
			} else {
				if ( collider.body ) {
					collider.body->setGravity(btVector3(0,-9.81,0));
					collider.body->setCollisionFlags(collider.body->getCollisionFlags() & ~btCollisionObject::CF_NO_CONTACT_RESPONSE);
				}
			}
		#endif
			stats.noclipped = state;
		}
		// movement handler
		// setup desired direction
		pod::Vector3f target = {};
		if ( keys.forward ^ keys.backwards ) target += translator.forward * (keys.forward ? 1 : -1);
		if ( keys.left ^ keys.right ) target += translator.right * (keys.right ? 1 : -1);
		target = uf::vector::normalize( target );

		physics.linear.velocity *= { speed.friction, 1, speed.friction };

		stats.walking = (keys.forward ^ keys.backwards) || (keys.left ^ keys.right);
		if ( stats.walking ) {
			float factor = stats.floored ? 1.0f : speed.air;
			if ( stats.noclipped ) {
				physics.linear.velocity += target * speed.move;
			} else {
				physics.linear.velocity += target * std::clamp( speed.move * factor - uf::vector::dot( physics.linear.velocity, target ), 0.0f, speed.move * 10 * uf::physics::time::delta );
			}
		}
		if ( !stats.floored ) stats.walking = false;		
	}
	if ( stats.floored && keys.jump ) {
		physics.linear.velocity += translator.up * metadata.movement.jump;
	}
	if ( keys.crouch ) {
		if ( stats.noclipped ) physics.linear.velocity -= translator.up * metadata.movement.jump;
		else {
			if ( !metadata.system.crouching )  stats.deltaCrouch = true;
			metadata.system.crouching = true;
		}
	} else {
		if ( metadata.system.crouching ) stats.deltaCrouch = true;
		metadata.system.crouching = false;
	}

	if ( keys.lookRight ^ keys.lookLeft ) {
		if ( collider.body ) uf::physics::impl::applyRotation( collider, transform.up, speed.rotate * (keys.lookRight ? 1 : -1) ); else
		uf::transform::rotate( transform, transform.up, speed.rotate * (keys.lookRight ? 1 : -1) );
	}
	{
		if ( collider.body ) uf::physics::impl::setVelocity( collider, physics.linear.velocity ); else 
		transform.position += physics.linear.velocity * uf::physics::time::delta;
	}


	if ( stats.deltaCrouch ) {
		float delta = metadata.movement.crouch;
		if ( metadata.system.crouching ) camera.getTransform().position.y -= delta;
		else camera.getTransform().position.y += delta;
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
	camera.update(true);

#if UF_ENTITY_METADATA_USE_JSON
	metadata.serialize(self, metadataJson);
#endif
}

void ext::PlayerBehavior::render( uf::Object& self ){}
void ext::PlayerBehavior::destroy( uf::Object& self ){}
void ext::PlayerBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ){
	auto& serializerSystem = serializer["system"];
	auto& serializerSystemPhysics = serializerSystem["physics"];
	auto& serializerSystemPhysicsMovement = serializerSystemPhysics["movement"];
	auto& serializerAudioFootstep = serializer["audio"]["footstep"];
	auto& serializerCamera = serializer["camera"];
	auto& serializerCameraLimit = serializerCamera["limit"];
	
	serializerSystem["menu"] = /*this->*/system.menu;
	serializerSystem["control"] = /*this->*/system.control;
	serializerSystem["crouching"] = /*this->*/system.crouching;
	serializerSystem["noclipped"] = /*this->*/system.noclipped;
	serializerSystemPhysics["friction"] = /*this->*/movement.friction;
	serializerSystemPhysicsMovement["rotate"] = /*this->*/movement.rotate;
	serializerSystemPhysicsMovement["move"] = /*this->*/movement.move;
	serializerSystemPhysicsMovement["run"] = /*this->*/movement.run;
	serializerSystemPhysicsMovement["walk"] = /*this->*/movement.walk;
	serializerSystemPhysicsMovement["air"] = /*this->*/movement.air;
	serializerSystemPhysicsMovement["jump"] = uf::vector::encode(/*this->*/movement.jump);
	serializerSystemPhysicsMovement["crouch"] = /*this->*/movement.crouch;
//	serializerSystemPhysicsMovement["look"] = /*this->*/movement.look;
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
	auto& serializerSystemPhysicsMovement = serializerSystemPhysics["movement"];
	auto& serializerCamera = serializer["camera"];
	auto& serializerCameraLimit = serializerCamera["limit"];

	/*this->*/system.menu = serializerSystem["menu"].as<uf::stl::string>();
	/*this->*/system.control = serializerSystem["control"].as<bool>();
	/*this->*/system.crouching = serializerSystem["crouching"].as<bool>();
	/*this->*/system.noclipped = serializerSystem["noclipped"].as<bool>();
	/*this->*/movement.friction = serializerSystemPhysics["friction"].as<float>();
	/*this->*/movement.rotate = serializerSystemPhysicsMovement["rotate"].as<float>();
	/*this->*/movement.move = serializerSystemPhysicsMovement["move"].as<float>();
	/*this->*/movement.run = serializerSystemPhysicsMovement["run"].as<float>();
	/*this->*/movement.walk = serializerSystemPhysicsMovement["walk"].as<float>();
	/*this->*/movement.air = serializerSystemPhysicsMovement["air"].as<float>();
	/*this->*/movement.jump = uf::vector::decode(serializerSystemPhysicsMovement["jump"], pod::Vector3f{});
	/*this->*/movement.crouch = serializerSystemPhysicsMovement["crouch"].as<float>();
//	/*this->*/movement.look = serializerSystemPhysicsMovement["look"].as<float>(1.0f);
	ext::json::forEach( serializerAudioFootstep["list"], [&]( const ext::json::Value& value ){
		/*this->*/audio.footstep.list.emplace_back(value);
	});
	/*this->*/audio.footstep.volume = serializerAudioFootstep["volume"].as<float>();

	/*this->*/camera.invert = uf::vector::decode( serializerCamera["invert"], pod::Vector3t<bool>{false, false, false} );
	/*this->*/camera.limit.current = uf::vector::decode( serializerCameraLimit["current"], pod::Vector3f{NAN, NAN, NAN} );
	/*this->*/camera.limit.min = uf::vector::decode( serializerCameraLimit["minima"], pod::Vector3f{NAN, NAN, NAN} );
	/*this->*/camera.limit.max = uf::vector::decode( serializerCameraLimit["maxima"], pod::Vector3f{NAN, NAN, NAN} );
}
#undef this