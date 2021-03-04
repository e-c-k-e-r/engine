#if 0
#include "behavior.h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/window/window.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/audio/audio.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/math/collision.h>

#include "../../scenes/worldscape//battle.h"

#include "../../scenes/worldscape/terrain/generator.h"

#include <uf/engine/asset/asset.h>

UF_BEHAVIOR_REGISTER_CPP(ext::CraetureBehavior)
#define this (&self)
void ext::CraetureBehavior::initialize( uf::Object& self ) {
	pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
	auto& camera = this->getComponent<uf::Camera>();
	camera.getTransform().reference = &transform;

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
#if 0
	pod::Physics& physics = this->getComponent<pod::Physics>();
	physics.linear.velocity = {0,0,0};
	physics.linear.acceleration = {0,-9.81,0};
	physics.rotational.velocity = uf::quaternion::axisAngle( {0,1,0}, (pod::Math::num_t) 0 );
	physics.rotational.acceleration = {0,0,0,0};

	/* Gravity */ {
		if ( !ext::json::isNull( metadata["system"]["physics"]["gravity"] ) ) {
			physics.linear.acceleration.x = metadata["system"]["physics"]["gravity"][0].as<float>();
			physics.linear.acceleration.y = metadata["system"]["physics"]["gravity"][1].as<float>();
			physics.linear.acceleration.z = metadata["system"]["physics"]["gravity"][2].as<float>();
		}
		if ( !metadata["system"]["physics"]["collision"].as<bool>() )  {
			physics.linear.acceleration.x = 0;
			physics.linear.acceleration.y = 0;
			physics.linear.acceleration.z = 0;
		}
	}
	/* Collider */ {
		uf::Collider& collider = this->getComponent<uf::Collider>();

		collider.clear();
		auto* box = new uf::BoundingBox( {0, 1.5, 0}, {0.7, 1.6, 0.7} );
		box->getTransform().reference = &transform;
		collider.add(box);
	}
	/* RPG */ {
		ext::HousamoBattle& battle = this->getComponent<ext::HousamoBattle>();
	}
#endif
	// Hooks
#if 0
	static uf::Timer<long long> timer(true);
	this->addHook( "world:Collision.%UID%", [&](ext::json::Value& json){
		size_t uid = json["uid"].as<size_t>();
		// do not collide with children
		// if ( this->findByUid(uid) ) return;

		pod::Vector3 normal;
		float scale = metadata["system"]["physics"]["collision"].as<float>();
		float depth = json["depth"].as<float>() * scale;
	//	if ( fabs(depth) < 0.005 ) return; //std::cout << "Collision depth: " << depth << std::endl;

		normal.x = json["normal"][0].as<float>();
		normal.y = json["normal"][1].as<float>();
		normal.z = json["normal"][2].as<float>();
		pod::Vector3 correction = normal * depth;

	#if 0
		transform.position -= correction;

		if ( normal.x == 1 || normal.x == -1 ) physics.linear.velocity.x = 0;
		if ( normal.y == 1 || normal.y == -1 ) physics.linear.velocity.y = 0;
		if ( normal.z == 1 || normal.z == -1 ) physics.linear.velocity.z = 0;
	#endif
	});
#endif
	this->addHook( "asset:Cache.Sound.%UID%", [&](ext::json::Value& json){
		uf::Scene& world = uf::scene::getCurrentScene();
		uf::Serializer& masterdata = world.getComponent<uf::Serializer>();
		
		std::string filename = json["filename"].as<std::string>();

		if ( uf::io::extension(filename) != "ogg" ) return;

		if ( filename == "" ) return;
		uf::Audio& sfx = this->getComponent<uf::SoundEmitter>().add(filename);
		sfx.setVolume(masterdata["volumes"]["sfx"].as<float>());
		auto& pTransform = world.getController().getComponent<pod::Transform<>>();
		sfx.setPosition( transform.position );
		sfx.play();
	});
#if 0
	this->addHook( "world:Craeture.OnHit.%UID%", [&](ext::json::Value& json){
		uint64_t phase = json["phase"].as<size_t>();
		// start color
		pod::Vector4f color = { 1, 1, 1, 0 };
		if ( phase == 0 ) {
			color = pod::Vector4f{
				json["color"][0].as<float>(),
				json["color"][1].as<float>(),
				json["color"][2].as<float>(),
				json["color"][3].as<float>(),
			};
		}
		metadata["color"][0] = color[0];
		metadata["color"][1] = color[1];
		metadata["color"][2] = color[2];
		metadata["color"][3] = color[3];

		if ( metadata["timers"]["hurt"].as<float>() < timer.elapsed().asDouble() ) {

			metadata["timers"]["hurt"] = timer.elapsed().asDouble() + 1.0f;
			uf::Scene& scene = uf::scene::getCurrentScene();
			uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
			assetLoader.cache(uf::io::root+"/audio/battle/hurt.ogg", "asset:Cache.Sound." + std::to_string(this->getUid()));
		}
	});
	this->addHook( "world:Craeture.Hurt.%UID%", [&](ext::json::Value& json){
		if ( metadata["timers"]["flash"].as<float>() < timer.elapsed().asDouble() ) {
			metadata["timers"]["flash"] = timer.elapsed().asDouble() + 0.4f;
			for ( int i = 0; i < 16; ++i ) {
				uf::Serializer payload;
				payload["phase"] = i % 2 == 0 ? 0 : 1;
				payload["color"][0] = 1.0f;
				payload["color"][1] = 0.0f;
				payload["color"][2] = 0.0f;
				payload["color"][3] = 0.6f;
				this->queueHook("world:Craeture.OnHit.%UID%", payload, 0.05f * i);
			}
		}
	});
#endif
}
void ext::CraetureBehavior::tick( uf::Object& self ) {
#if 0
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Scene& scene = uf::scene::getCurrentScene();
	uf::Serializer& sMetadata = scene.getComponent<uf::Serializer>();
	uf::Serializer& pMetadata = scene.getController().getComponent<uf::Serializer>();

	if ( !pMetadata["system"]["control"].as<bool>() ) return;
	if ( !sMetadata["system"]["physics"]["optimizations"]["entity-local update"].as<bool>() ) return;

	pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
	pod::Physics& physics = this->getComponent<pod::Physics>();
	/* Gravity */ {
		if ( !ext::json::isNull( metadata["system"]["physics"]["gravity"] ) ) {
			physics.linear.acceleration.x = metadata["system"]["physics"]["gravity"][0].as<float>();
			physics.linear.acceleration.y = metadata["system"]["physics"]["gravity"][1].as<float>();
			physics.linear.acceleration.z = metadata["system"]["physics"]["gravity"][2].as<float>();
		}
		if ( !metadata["system"]["physics"]["collision"].as<bool>() )  {
			physics.linear.acceleration.x = 0;
			physics.linear.acceleration.y = 0;
			physics.linear.acceleration.z = 0;
		}
	}
	transform = uf::physics::update( transform, physics );
#endif
}

void ext::CraetureBehavior::render( uf::Object& self ){}
void ext::CraetureBehavior::destroy( uf::Object& self ){}
#undef this
#endif