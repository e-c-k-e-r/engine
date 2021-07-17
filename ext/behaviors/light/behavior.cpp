#include "behavior.h"

#include <uf/utils/renderer/renderer.h>

#include <uf/utils/math/matrix.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/camera/camera.h>

namespace {
	struct {
		size_t current = 0;
		uf::stl::vector<uf::Object*> lights;
	} roundRobin;
	const pod::Quaternion<> rotations[6] = {
		uf::quaternion::axisAngle( { 0, 1, 0 }, 1 * 1.57079633f ), // right
		uf::quaternion::axisAngle( { 0, 1, 0 }, 3 * 1.57079633f ), // left
		uf::quaternion::axisAngle( { 1, 0, 0 }, 3 * 1.57079633f ), // down
		uf::quaternion::axisAngle( { 1, 0, 0 }, 1 * 1.57079633f ), // up
		uf::quaternion::axisAngle( { 0, 1, 0 }, 0 * 1.57079633f ), // front
		uf::quaternion::axisAngle( { 0, 1, 0 }, 2 * 1.57079633f ), // back
	};
}

UF_BEHAVIOR_REGISTER_CPP(ext::LightBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::LightBehavior, ticks = true, renders = false, multithread = false)
#define this (&self)
void ext::LightBehavior::initialize( uf::Object& self ) {
	auto& metadata = this->getComponent<ext::LightBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();
	auto& transform = this->getComponent<pod::Transform<>>();
	auto& camera = this->getComponent<uf::Camera>();
	auto& scene = uf::scene::getCurrentScene();
	auto& controller = scene.getController();
	auto& sceneMetadata = scene.getComponent<uf::Serializer>();

	if ( !sceneMetadata["system"]["lights"]["round robin hook bound"].as<bool>() ) {
		sceneMetadata["system"]["lights"]["round robin hook bound"] = true;
		scene.addHook("game:Frame.Start", [&]( const ext::json::Value& payload ){
			if ( ++::roundRobin.current >= ::roundRobin.lights.size() ) ::roundRobin.current = 0;
		});
	}
/*
	if ( !metadataJson["light"]["bias"]["shader"].is<float>() ) metadataJson["light"]["bias"]["shader"] = 0.000000005f;
*/
	if ( !metadataJson["light"]["bias"]["constant"].is<float>() ) metadataJson["light"]["bias"]["constant"] = 0.00005f;
	if ( !metadataJson["light"]["type"].is<uf::stl::string>() ) metadataJson["light"]["type"] = "point";
	if ( !ext::json::isArray( metadataJson["light"]["color"] ) ) {
		metadataJson["light"]["color"][0] = 1; //metadataJson["light"]["color"]["random"].as<bool>() ? (rand() % 100) / 100.0 : 1;
		metadataJson["light"]["color"][1] = 1; //metadataJson["light"]["color"]["random"].as<bool>() ? (rand() % 100) / 100.0 : 1;
		metadataJson["light"]["color"][2] = 1; //metadataJson["light"]["color"]["random"].as<bool>() ? (rand() % 100) / 100.0 : 1;
	}
#if UF_USE_OPENGL
	metadataJson["light"]["shadows"] = false;
#endif
	if ( !sceneMetadata["system"]["config"]["engine"]["scenes"]["shadows"]["enabled"].as<bool>(true) )
		metadataJson["light"]["shadows"] = false;
	if ( metadataJson["light"]["shadows"].as<bool>() ) {
		auto& cameraTransform = camera.getTransform();
		cameraTransform.reference = &transform;
		camera.setStereoscopic(false);
		
		::roundRobin.lights.emplace_back(this);

		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		renderMode.metadata.type = "depth";
		renderMode.metadata.pipeline = "depth";
		if ( uf::renderer::settings::experimental::culling ) {
			renderMode.metadata.pipelines.emplace_back("culling");
		}
		renderMode.metadata.json["descriptor"]["depth bias"] = metadataJson["light"]["bias"];
		renderMode.metadata.json["descriptor"]["renderMode"] = metadataJson["renderMode"];

		if ( metadataJson["light"]["type"].as<uf::stl::string>() == "point" ) {
			metadataJson["light"]["fov"] = 90.0f;
			renderMode.metadata.subpasses = 6;
		}

		float fov = metadataJson["light"]["fov"].as<float>(90) * (3.14159265358f / 180.0f);
		pod::Vector2f radius = uf::vector::decode(  metadataJson["light"]["radius"], pod::Vector2f{0.001, 32} );
		pod::Vector2ui size{};
		if ( ext::json::isArray( metadataJson["light"]["resolution"] ) ) {
			size = uf::vector::decode( metadataJson["light"]["resolution"], pod::Vector2ui{ uf::renderer::settings::width, uf::renderer::settings::height } );
		} else if ( metadataJson["light"]["resolution"].is<float>() ) {
			size_t r = metadataJson["light"]["resolution"].as<size_t>();
			size.x = r;
			size.y = r;
		} else {
			size = pod::Vector2ui{ uf::renderer::settings::width, uf::renderer::settings::height };
		}
		if ( radius.y < radius.x ) radius.y = 256;
		camera.setProjection( uf::matrix::perspective( fov, (float) size.x / (float) size.y, radius.x, radius.y ) );
		camera.update(true);
		
		uf::stl::string name = "RT:" + std::to_string((int) this->getUid());
		uf::renderer::addRenderMode( &renderMode, name );
		renderMode.blitter.process = false;
		renderMode.width = size.x;
		renderMode.height = size.y;
	}

	this->addHook( "object:UpdateMetadata.%UID%", [&](){
		metadata.deserialize(self, metadataJson);
	});
	metadata.deserialize(self, metadataJson);
}
void ext::LightBehavior::tick( uf::Object& self ) {
#if !UF_ENV_DREAMCAST
	auto& metadata = this->getComponent<ext::LightBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();
	if ( this->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		renderMode.setTarget("");
	}
#if UF_ENTITY_METADATA_USE_JSON
	metadata.deserialize(self, metadataJson);
#endif
#if 0
	// light fade action
	if ( ext::json::isObject( metadataJson["light"]["fade"] ) ) {
		if ( ext::json::isNull( metadataJson["light"]["backup"]["power"] ) ) {
			metadataJson["light"]["backup"]["power"] = metadataJson["light"]["power"];
		}
		if ( ext::json::isNull( metadataJson["light"]["backup"]["color"] ) ) {
			metadataJson["light"]["backup"]["color"] = metadataJson["light"]["color"];
		}
		// fade towards
		int direction = metadataJson["light"]["fade"]["increment"].as<bool>() ? 1 : -1;
		metadataJson["light"]["fade"]["timer"] = metadataJson["light"]["fade"]["timer"].as<float>() + metadataJson["light"]["fade"]["rate"].as<float>() * uf::physics::time::delta * direction;

		// 0 .. delta .. 1 .. (1 + timeout * 0.5)
		if ( direction == 1 && metadataJson["light"]["fade"]["timer"].as<float>() >= 0.5f * metadataJson["light"]["fade"]["timeout"].as<float>() + 1.0f ) {
			metadataJson["light"]["fade"]["increment"] = false;
		} else if ( direction == -1 && metadataJson["light"]["fade"]["timer"].as<float>() <= -0.5f * metadataJson["light"]["fade"]["timeout"].as<float>() ) {
			metadataJson["light"]["fade"]["increment"] = true;
		}
		{
			float delta = metadataJson["light"]["fade"]["timer"].as<float>();
			delta = std::clamp( delta, 0.f, 1.f );
			if ( metadataJson["light"]["fade"]["power"].is<double>() ) {
				metadataJson["light"]["power"] = uf::math::lerp( metadataJson["light"]["backup"]["power"].as<float>(), metadataJson["light"]["fade"]["power"].as<float>(), delta );
			}
			if ( ext::json::isArray( metadataJson["light"]["fade"]["color"] ) ) {
				pod::Vector3f fadeColor; {
					fadeColor.x = metadataJson["light"]["fade"]["color"][0].as<float>();
					fadeColor.y = metadataJson["light"]["fade"]["color"][1].as<float>();
					fadeColor.z = metadataJson["light"]["fade"]["color"][2].as<float>();
				}
				pod::Vector3f origColor; {
					origColor.x = metadataJson["light"]["backup"]["color"][0].as<float>();
					origColor.y = metadataJson["light"]["backup"]["color"][1].as<float>();
					origColor.z = metadataJson["light"]["backup"]["color"][2].as<float>();
				}
				pod::Vector3f color = uf::vector::lerp( origColor, fadeColor, delta );

				metadataJson["light"]["color"][0] = color[0];
				metadataJson["light"]["color"][1] = color[1];
				metadataJson["light"]["color"][2] = color[2];
			}
		}
	}
	// light flicker action
	if ( ext::json::isObject( metadataJson["light"]["flicker"] ) ) {
		float r = (rand() % 100) / 100.0;
		float rate = metadataJson["light"]["flicker"]["rate"].as<float>();
		if ( ext::json::isNull( metadataJson["light"]["backup"]["power"] ) ) {
			metadataJson["light"]["backup"]["power"] = metadataJson["light"]["power"];
		}
		metadataJson["light"]["flicker"]["timer"] = metadataJson["light"]["flicker"]["timer"].as<float>() + uf::physics::time::delta;
		if ( metadataJson["light"]["flicker"]["timer"].as<float>() >= metadataJson["light"]["flicker"]["timeout"].as<float>() ) {
			metadataJson["light"]["flicker"]["timer"] = 0;
			metadataJson["light"]["power"] = (r > rate) ? metadataJson["light"]["flicker"]["power"].as<float>() : metadataJson["light"]["backup"]["power"].as<float>();
		}
	}
#endif
	// limit updating our shadow map
	if ( this->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
		
		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		// enable renderer every X seconds
		if ( metadata.renderer.limiter > 0 ) {
			if ( metadata.renderer.timer > metadata.renderer.limiter ) {
				metadata.renderer.timer = 0;
				renderMode.execute = true;
			} else {
				metadata.renderer.timer = metadata.renderer.timer + uf::physics::time::delta;
				renderMode.execute = false;
			}
		} else {
			// round robin, enable if it's the light's current turn
			if ( metadata.renderer.mode == "round robin" ) {
				if ( ::roundRobin.current < ::roundRobin.lights.size() )
					renderMode.execute = ::roundRobin.lights[::roundRobin.current] == this;
			// render only if the light is used
			} else if ( metadata.renderer.mode == "occlusion" ) {
				renderMode.execute = metadata.renderer.rendered;
			// light baking, but sadly re-bakes every time the command buffer is recorded
			} else if ( metadata.renderer.mode == "once" ) {
				renderMode.execute = !metadata.renderer.rendered;
				metadata.renderer.rendered = true;
			} else if ( metadata.renderer.mode == "in-range" ) {
				metadata.renderer.rendered = false;
			}
		}
		// skip if we're handling the camera view matrix position ourselves
		if ( renderMode.execute && !metadata.renderer.external ) {
			auto& camera = this->getComponent<uf::Camera>();
			// omni light
			if ( metadata.shadows && std::abs(metadata.type) == 1 ) {
				auto transform = camera.getTransform();
				for ( size_t i = 0; i < 6; ++i ) {
					transform.orientation = ::rotations[i];
					auto model = uf::transform::model( transform, false );
					camera.setView( uf::matrix::inverse( model ), i );
				}
			} else {
				camera.update(true);
			}
		}
	}
#if UF_ENTITY_METADATA_USE_JSON
	metadata.serialize(self, metadataJson);
#endif
#endif
}
void ext::LightBehavior::render( uf::Object& self ){}
void ext::LightBehavior::destroy( uf::Object& self ){
	if ( this->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		uf::renderer::removeRenderMode( &renderMode, false );
		this->deleteComponent<uf::renderer::RenderTargetRenderMode>();
	}

	::roundRobin.lights.erase(std::remove(::roundRobin.lights.begin(), ::roundRobin.lights.end(), this), ::roundRobin.lights.end());
	::roundRobin.current = 0;
}
void ext::LightBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ){
	serializer["light"]["color"] = uf::vector::encode( /*this->*/color );
	serializer["light"]["power"] = /*this->*/power;
	serializer["light"]["bias"]["shader"] = /*this->*/bias;
	serializer["light"]["shadows"] = /*this->*/shadows;
	serializer["system"]["renderer"]["mode"] = /*this->*/renderer.mode;
	serializer["light"]["external update"] = /*this->*/renderer.external;
	serializer["system"]["renderer"]["timer"] = /*this->*/renderer.limiter;
	switch ( /*this->*/type ) {
		case 0: serializer["light"]["type"] = "point"; break;
		case 1: serializer["light"]["type"] = "spot"; break;
		case 2: serializer["light"]["type"] = "spot"; break;
	}
}
void ext::LightBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ){	
	/*this->*/color = uf::vector::decode( serializer["light"]["color"], pod::Vector3f{1,1,1} );
	/*this->*/power = serializer["light"]["power"].as<float>();
	/*this->*/bias = serializer["light"]["bias"]["shader"].as<float>();
	/*this->*/shadows = serializer["light"]["shadows"].as<bool>();
	/*this->*/renderer.mode = serializer["system"]["renderer"]["mode"].as<uf::stl::string>();
	/*this->*/renderer.rendered = false;
	/*this->*/renderer.external = serializer["light"]["external update"].as<bool>();
	/*this->*/renderer.limiter = serializer["system"]["renderer"]["timer"].as<float>();
	if ( serializer["light"]["type"].is<size_t>() ) {
		/*this->*/type = serializer["light"]["type"].as<size_t>();
	} else if ( serializer["light"]["type"].is<uf::stl::string>() ) {
		uf::stl::string lightType = serializer["light"]["type"].as<uf::stl::string>();
		if ( lightType == "point" ) /*this->*/type = 1;
		else if ( lightType == "spot" ) /*this->*/type = 2;
	}
	if ( serializer["light"]["dynamic"].as<bool>() ) /*this->*/type = -/*this->*/type;
}
#undef this