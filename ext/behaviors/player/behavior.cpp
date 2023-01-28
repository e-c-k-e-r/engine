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

#include "../scene/behavior.h"
#include "../../gui/manager/behavior.h"

UF_BEHAVIOR_REGISTER_CPP(ext::PlayerBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::PlayerBehavior, ticks = true, renders = false, multithread = true)
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

		auto cameraSettingsJson = metadataJson["camera"]["settings"];

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
			pod::Vector2ui size = uf::vector::decode( cameraSettingsJson["size"], pod::Vector2ui{ uf::renderer::settings::width, uf::renderer::settings::height } );
			float raidou = (float) size.x / (float) size.y;

			if ( size.x == 0 || size.y == 0 )  {
				size = uf::vector::decode( ext::config["window"]["size"], pod::Vector2ui{} );
				raidou = (float) size.x / (float) size.y;
			#if 0
				this->addHook( "window:Resized", [&, fov, range](pod::payloads::windowResized& payload){
					float width = uf::renderer::settings::
					float raidou = (float) payload.window.size.x / (float) payload.window.size.y;
					camera.setProjection( uf::matrix::perspective( fov, raidou, range.x, range.y ) );
				} );
			#endif
			}
			camera.setProjection( uf::matrix::perspective( fov, raidou, range.x, range.y ) );
		}
	//	camera.update(true);
	}
	
	// sloppy
	metadata.mouse.sensitivity = uf::vector::decode( ext::config["window"]["mouse"]["sensitivity"], metadata.mouse.sensitivity );
	metadata.mouse.smoothing = uf::vector::decode( ext::config["window"]["mouse"]["smoothing"], metadata.mouse.smoothing );

	this->addHook( "window:Mouse.CursorVisibility", [&](pod::payloads::windowMouseCursorVisibility& payload){
		metadata.system.control = !payload.mouse.visible;
	});
	this->addHook( "system:Control.%UID%", [&]( ext::json::Value& value ){
		metadata.system.control = value["control"].as<bool>(!metadata.system.control);
	});

	// Rotate Camera
#if !UF_INPUT_USE_ENUM_MOUSE
	this->addHook( "window:Mouse.Moved", [&](pod::payloads::windowMouseMoved& payload ){
		const pod::Vector2ui deadZone{0, 0};
		if ( (payload.mouse.delta.x == 0 && payload.mouse.delta.y == 0) || !metadata.system.control ) return;

		pod::Vector2f delta = {
			(float) metadata.mouse.sensitivity.x * (abs(payload.mouse.delta.x) < deadZone.x ? 0 : payload.mouse.delta.x) / payload.window.size.x,
			(float) metadata.mouse.sensitivity.y * (abs(payload.mouse.delta.y) < deadZone.y ? 0 : payload.mouse.delta.y) / payload.window.size.y
		};
		metadata.camera.queued += delta;
	});
#endif
	
#if UF_USE_DISCORD
	// Discord Integration
	this->addHook( "discord.Activity.Update.%UID%", [&](ext::json::Value& json){
		uf::stl::string leaderId = metadataJson[""]["party"][0].as<uf::stl::string>();
		uf::Serializer cardData = masterDataGet("Card", leaderId);
		uf::Serializer charaData = masterDataGet("Chara", cardData["character_id"].as<uf::stl::string>());
		uf::stl::string leader = charaData["name"].as<uf::stl::string>();

		ext::json::Value payload = json;
		payload["details"] = "Leader: " + leader;
		uf::hooks.call( "discord:Activity.Update", payload );
	});
	this->queueHook("discord.Activity.Update.%UID%", ext::json::null(), 1.0);
#endif

	UF_BEHAVIOR_METADATA_BIND_SERIALIZER_HOOKS(metadata, metadataJson);
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
		bool console = false;
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
			.console = uf::inputs::kbm::states::Tilde,
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
		//	if ( uf::inputs::controller::states::Y ) keys.vee = true;
			if ( uf::inputs::controller::states::Y ) keys.use = true;
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

		pod::Matrix4f previous;
	} stats;

	auto& metadata = this->getComponent<ext::PlayerBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();
#if UF_ENTITY_METADATA_USE_JSON
	metadata.deserialize(self, metadataJson);
#endif

#if 1
	if ( uf::renderer::states::resized && uf::renderer::settings::width > 0 && uf::renderer::settings::height > 0 ) {
		auto cameraSettingsJson = metadataJson["camera"]["settings"];
		
		float fov = cameraSettingsJson["fov"].as<float>(120) * (3.14159265358f / 180.0f);
		float raidou = (float) uf::renderer::settings::width / (float) uf::renderer::settings::height;
		pod::Vector2f range = uf::vector::decode( cameraSettingsJson["clip"], pod::Vector2f{ 0.1, 64.0f } );

		camera.setProjection( uf::matrix::perspective( fov, raidou, range.x, range.y ) );
	}
#endif

	stats.menu = metadata.system.menu;
	stats.noclipped = metadata.system.noclipped;
	stats.floored = stats.noclipped;
	auto& collider = this->getComponent<pod::PhysicsState>();

	{
		float t = -1;
		uf::Object* hit = NULL;
		pod::Vector3f center = transform.position + metadata.movement.floored.feet;
		pod::Vector3f direction = metadata.movement.floored.floor;
		if ( !stats.floored && collider.body && (hit = uf::physics::impl::rayCast( collider, center, direction, t )) ) {
			if ( metadata.movement.floored.print ) UF_MSG_DEBUG("Floored: {} | {}", hit->getName(), t);
			stats.floored = true;
		}
	}
#if 0
	TIMER(0.25, keys.use ) {
		size_t uid = 0;
		float depth = -1;
		uf::Object* pointer = NULL;
		float length = metadata.use.length;
	//	pod::Vector3f center = transform.position + cameraTransform.position;
	//	pod::Vector3f direction = uf::vector::normalize( transform.forward + pod::Vector3f{ 0, cameraTransform.forward.y, 0 } ) * length;
		auto flattened = uf::transform::flatten( cameraTransform );
		pod::Vector3f center = flattened.position;
		pod::Vector3f direction = flattened.forward * length;

		pointer = uf::physics::impl::rayCast( center, direction, this, depth );
		ext::json::Value payload;
		if ( pointer ) { 
			payload["user"] = this->getUid();
			payload["uid"] = pointer->getUid();
			payload["depth"] = uf::vector::norm(direction * depth);
			pointer->lazyCallHook( "entity:Use.%UID%", payload );
		} else {
			payload["user"] = this->getUid();
			payload["uid"] = 0;
			payload["depth"] = -1;
		}
		this->lazyCallHook( "entity:Use.%UID%", payload );
	/*
		auto& emitter = this->getComponent<uf::MappedSoundEmitter>();
		uf::stl::string filename = pointer ? "./ui/select.ogg" : "./ui/deny.ogg";
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
	*/
	}
#endif

	if ( collider.stats.gravity == pod::Vector3f{0,0,0} ) stats.noclipped = true;

	struct {
		float move = 4;
		float walk = 1;
		float run = 8;
		float rotate = 1;
		float friction = 0.8f;
		float air = 1.0f;
	} speed; {
		speed.rotate = metadata.movement.rotate * uf::physics::time::delta;
		speed.move = metadata.movement.move;
		speed.run = metadata.movement.run;
		speed.walk = metadata.movement.walk;
		speed.friction = metadata.movement.friction;
		speed.air = metadata.movement.air;
		
		if ( stats.noclipped ) {
			speed.move *= 1.5;
			speed.run *= 1.5;
		}
		if ( !stats.floored || stats.noclipped ) speed.friction = 1;
		if ( stats.noclipped ) physics.linear.velocity = {};
	}
	if ( keys.running ) speed.move = speed.run;
	else if ( keys.walk ) speed.move = speed.walk;

	{
		uf::Object* menu = (uf::Object*) scene.globalFindByName("Gui: Menu");
		if ( !menu ) stats.menu = "";
		// make assumptions
		if ( stats.menu == "" && keys.paused ) {
			stats.menu = "paused";
			metadata.system.control = false;
			pod::payloads::menuOpen payload;
			payload.name = "pause";
			uf::hooks.call("menu:Open", payload);
		} else if ( !metadata.system.control ) {
			stats.menu = "menu";
		} else  {
			metadata.system.control = stats.menu == "";
		}
		metadata.system.menu = stats.menu;
	}

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

	if ( stats.noclipped || collider.stats.gravity == pod::Vector3f{0,0,0} ){
		translator.forward.y += cameraTransform.forward.y;
		translator.forward = uf::vector::normalize( translator.forward );
	}

	if ( metadata.system.control ) {
		// noclip handler
		TIMER(0.25, keys.vee ) {
			bool state = !stats.noclipped;
			metadata.system.noclipped = state;
			if ( collider.body ) {
				collider.body->enableGravity(!state);
				uf::physics::impl::activateCollision(collider, !state);
			}
			
			stats.noclipped = state;
			UF_MSG_DEBUG( "{}abled noclip: {}", (state ? "En" : "Dis"), uf::vector::toString(transform.position));
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
			//	float delta = collider.body ? uf::physics::impl::timescale : uf::physics::time::delta;
				float delta = uf::physics::time::delta;

				physics.linear.velocity += target * std::clamp( speed.move * factor - uf::vector::dot( physics.linear.velocity, target ), 0.0f, speed.move * 10 * delta );
			}
		}
		if ( !stats.floored ) stats.walking = false;		
	}
	TIMER(0.0625, stats.floored && keys.jump && !stats.noclipped ) {
		physics.linear.velocity += translator.up * metadata.movement.jump;
	}
	if ( stats.floored && keys.jump && stats.noclipped ) transform.position += translator.up * metadata.movement.jump * uf::physics::time::delta * 4.0f;
	if ( keys.crouch ) {
		if ( stats.noclipped ) transform.position -= translator.up * metadata.movement.jump * uf::physics::time::delta * 4.0f;
		else {
			if ( !metadata.system.crouching )  stats.deltaCrouch = true;
			metadata.system.crouching = true;
		}
	} else {
		if ( metadata.system.crouching ) stats.deltaCrouch = true;
		metadata.system.crouching = false;
	}

	// 
	#if UF_INPUT_USE_ENUM_MOUSE
	{
		const pod::Vector2ui deadZone{0, 0};
		const auto& mouseDelta = uf::inputs::kbm::states::Mouse;
		bool shouldnt = (mouseDelta.x == 0 && mouseDelta.y == 0) || !metadata.system.control;
		if ( !shouldnt ) {
			pod::Vector2f delta = {
				(float) metadata.mouse.sensitivity.x * (abs(mouseDelta.x) < deadZone.x ? 0 : mouseDelta.x),
				(float) metadata.mouse.sensitivity.y * (abs(mouseDelta.y) < deadZone.y ? 0 : mouseDelta.y)
			};
			metadata.camera.queued += delta;
		}
	}
	#endif

	if ( metadata.camera.queued.x != 0 || metadata.camera.queued.y != 0 ) {
		auto lookDelta = metadata.camera.queued;
		if ( abs(lookDelta.x) > uf::physics::time::delta / metadata.mouse.smoothing.x ) lookDelta.x *= uf::physics::time::delta * metadata.mouse.smoothing.x;
		if ( abs(lookDelta.y) > uf::physics::time::delta / metadata.mouse.smoothing.y ) lookDelta.y *= uf::physics::time::delta * metadata.mouse.smoothing.y;
		metadata.camera.queued -= lookDelta;
		if ( lookDelta.x != 0 ) {
			if ( metadata.camera.invert.x ) lookDelta.x *= -1;
			metadata.camera.limit.current.x += lookDelta.x;
			if ( metadata.camera.limit.current.x != metadata.camera.limit.current.x || ( metadata.camera.limit.current.x < metadata.camera.limit.max.x && metadata.camera.limit.current.x > metadata.camera.limit.min.x ) ) {
				if ( collider.body ) uf::physics::impl::applyRotation( collider, transform.up, lookDelta.x ); else
					uf::transform::rotate( transform, transform.up, lookDelta.x );
			} else metadata.camera.limit.current.x -= lookDelta.x;
		}
		if ( lookDelta.y != 0 ) {
			if ( metadata.camera.invert.y ) lookDelta.y *= -1;
			metadata.camera.limit.current.y += lookDelta.y;
				if ( metadata.camera.limit.current.y != metadata.camera.limit.current.y || ( metadata.camera.limit.current.y < metadata.camera.limit.max.y && metadata.camera.limit.current.y > metadata.camera.limit.min.y ) ) {
				//	if ( collider.body && !collider.shared ) uf::physics::impl::applyRotation( collider, cameraTransform.right, lookDelta.y ); else
					uf::transform::rotate( cameraTransform, cameraTransform.right, lookDelta.y );
			} else metadata.camera.limit.current.y -= lookDelta.y;
		}
	} else if ( keys.lookRight ^ keys.lookLeft ) {
		if ( collider.body ) uf::physics::impl::applyRotation( collider, transform.up, speed.rotate * (keys.lookRight ? 1 : -1) ); else
		uf::transform::rotate( transform, transform.up, speed.rotate * (keys.lookRight ? 1 : -1) );
	}
	{
		if ( collider.body ) uf::physics::impl::setVelocity( collider, physics.linear.velocity ); else 
		transform.position += physics.linear.velocity * uf::physics::time::delta;
	}


	if ( stats.deltaCrouch ) {
		float delta = metadata.movement.crouch;
		if ( metadata.system.crouching ) cameraTransform.position.y -= delta;
		else cameraTransform.position.y += delta;
	}

#if UF_USE_OPENAL
	if ( stats.floored && !stats.noclipped ) {
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
				footstep.setVolume( metadata.audio.footstep.volume * (metadata.system.crouching ? 0.5 : 1) );
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
#if 0 && UF_USE_LUA && !UF_ENV_DREAMCAST
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
	// this causes bigly memory leaks
	{
		pod::Quaternion<> combinedDeltaOrientation = {0,0,0,1};
		pod::Vector3f combinedDeltaAngles = {};
		TRACK_ORIENTATION(transform.orientation);
		TRACK_ORIENTATION(camera.getTransform().orientation);
		float magnitude = uf::quaternion::magnitude( combinedDeltaOrientation );
		if ( magnitude > 0.0001f ) {
			ext::json::Value payload;
			payload["delta"] = uf::vector::encode( combinedDeltaOrientation );
			payload["angle"]["pitch"] = combinedDeltaAngles.x;
			payload["angle"]["yaw"] = combinedDeltaAngles.y;
			payload["angle"]["roll"] = combinedDeltaAngles.z;
			payload["magnitude"] = magnitude;
			this->callHook("controller:Camera.Rotated", payload);
		}
	}
#endif
//	camera.update(true);

#if UF_ENTITY_METADATA_USE_JSON
	metadata.serialize(self, metadataJson);
#endif
}

void ext::PlayerBehavior::render( uf::Object& self ){}
void ext::PlayerBehavior::destroy( uf::Object& self ){}
void ext::PlayerBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ){
	auto& serializerSystem = serializer["system"];
	auto& serializerPhysics = serializer["physics"];
	auto& serializerMovement = serializer["movement"];
	auto& serializerAudioFootstep = serializer["audio"]["footstep"];
	auto& serializerCamera = serializer["camera"];
	auto& serializerCameraLimit = serializerCamera["limit"];
	
	serializerSystem["menu"] = /*this->*/system.menu;
	serializerSystem["control"] = /*this->*/system.control;
	serializerSystem["crouching"] = /*this->*/system.crouching;
	serializerSystem["noclipped"] = /*this->*/system.noclipped;
	serializerPhysics["friction"] = /*this->*/movement.friction;
	serializerMovement["rotate"] = /*this->*/movement.rotate;
	serializerMovement["move"] = /*this->*/movement.move;
	serializerMovement["run"] = /*this->*/movement.run;
	serializerMovement["walk"] = /*this->*/movement.walk;
	serializerMovement["air"] = /*this->*/movement.air;
	serializerMovement["jump"] = uf::vector::encode(/*this->*/movement.jump);
	serializerMovement["floored"]["feet"] = uf::vector::encode(/*this->*/movement.floored.feet);
	serializerMovement["floored"]["floor"] = uf::vector::encode(/*this->*/movement.floored.floor);
	serializerMovement["floored"]["print"] = /*this->*/movement.floored.print;
	serializerMovement["crouch"] = /*this->*/movement.crouch;
//	serializerMovement["look"] = /*this->*/movement.look;
	serializerAudioFootstep["list"] = /*this->*/audio.footstep.list;
	serializerAudioFootstep["volume"] = /*this->*/audio.footstep.volume;
	serializerCamera["invert"] = uf::vector::encode(/*this->*/camera.invert);
	serializerCameraLimit["current"] = uf::vector::encode(/*this->*/camera.limit.current);
	serializerCameraLimit["minima"] = uf::vector::encode(/*this->*/camera.limit.min);
	serializerCameraLimit["maxima"] = uf::vector::encode(/*this->*/camera.limit.max);

	serializer["use"]["length"] = /*this->*/use.length;
}
void ext::PlayerBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ){
	auto& serializerSystem = serializer["system"];
	auto& serializerAudioFootstep = serializer["audio"]["footstep"];
	auto& serializerPhysics = serializer["physics"];
	auto& serializerMovement = serializer["movement"];
	auto& serializerCamera = serializer["camera"];
	auto& serializerCameraLimit = serializerCamera["limit"];

	/*this->*/system.menu = serializerSystem["menu"].as(/*this->*/system.menu);
	/*this->*/system.control = serializerSystem["control"].as(/*this->*/system.control);
	/*this->*/system.crouching = serializerSystem["crouching"].as(/*this->*/system.crouching);
	/*this->*/system.noclipped = serializerSystem["noclipped"].as(/*this->*/system.noclipped);
	/*this->*/movement.friction = serializerPhysics["friction"].as(/*this->*/movement.friction);
	/*this->*/movement.rotate = serializerMovement["rotate"].as(/*this->*/movement.rotate);
	/*this->*/movement.move = serializerMovement["move"].as(/*this->*/movement.move);
	/*this->*/movement.run = serializerMovement["run"].as(/*this->*/movement.run);
	/*this->*/movement.walk = serializerMovement["walk"].as(/*this->*/movement.walk);
	/*this->*/movement.air = serializerMovement["air"].as(/*this->*/movement.air);
	/*this->*/movement.jump = uf::vector::decode(serializerMovement["jump"], /*this->*/movement.jump);
	/*this->*/movement.floored.feet = uf::vector::decode(serializerMovement["floored"]["feet"], /*this->*/movement.floored.feet);
	/*this->*/movement.floored.floor = uf::vector::decode(serializerMovement["floored"]["floor"], /*this->*/movement.floored.floor);
	/*this->*/movement.floored.print = serializerMovement["floored"]["print"].as(/*this->*/movement.floored.print);
	/*this->*/movement.crouch = serializerMovement["crouch"].as(/*this->*/movement.crouch);
//	/*this->*/movement.look = serializerMovement["look"].as<float>(1.0f);
	ext::json::forEach( serializerAudioFootstep["list"], [&]( const ext::json::Value& value ){
		/*this->*/audio.footstep.list.emplace_back(value);
	});
	/*this->*/audio.footstep.volume = serializerAudioFootstep["volume"].as<float>();

	/*this->*/camera.invert = uf::vector::decode( serializerCamera["invert"], /*this->*/camera.invert );
	/*this->*/camera.limit.current = uf::vector::decode( serializerCameraLimit["current"], /*this->*/camera.limit.current );
	/*this->*/camera.limit.min = uf::vector::decode( serializerCameraLimit["minima"], /*this->*/camera.limit.min );
	/*this->*/camera.limit.max = uf::vector::decode( serializerCameraLimit["maxima"], /*this->*/camera.limit.max );

	/*this->*/use.length = serializer["use"]["length"].as(/*this->*/use.length);
}
#undef this