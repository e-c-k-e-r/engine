#include "behavior.h"

#include <uf/utils/renderer/renderer.h>

#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/camera/camera.h>

namespace {
	struct {
		size_t current = 0;
		std::vector<uf::Object*> lights;
	} roundRobin;
}

UF_BEHAVIOR_REGISTER_CPP(ext::LightBehavior)
#define this (&self)
void ext::LightBehavior::initialize( uf::Object& self ) {
	auto& metadata = this->getComponent<uf::Serializer>();
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

	if ( !metadata["light"]["bias"]["constant"].is<float>() ) {
		metadata["light"]["bias"]["constant"] = 0.00005f;
	}
	if ( !ext::json::isArray( metadata["light"]["color"] ) ) {
		metadata["light"]["color"][0] = 1; //metadata["light"]["color"]["random"].as<bool>() ? (rand() % 100) / 100.0 : 1;
		metadata["light"]["color"][1] = 1; //metadata["light"]["color"]["random"].as<bool>() ? (rand() % 100) / 100.0 : 1;
		metadata["light"]["color"][2] = 1; //metadata["light"]["color"]["random"].as<bool>() ? (rand() % 100) / 100.0 : 1;
	}
#if UF_USE_OPENGL
	metadata["light"]["shadows"] = false;
#endif
	if ( metadata["light"]["shadows"].as<bool>() ) {
		auto& cameraTransform = camera.getTransform();
		cameraTransform.reference = &transform;
		camera.setStereoscopic(false);
		if ( ext::json::isArray( metadata["light"]["radius"] ) ) {
			auto bounds = camera.getBounds();
			bounds.x = metadata["light"]["radius"][0].as<float>();
			bounds.y = metadata["light"]["radius"][1].as<float>();
			camera.setBounds(bounds);
		}
		if ( ext::json::isArray( metadata["light"]["resolution"] ) ) {
			camera.setSize( uf::vector::decode( metadata["light"]["resolution"], pod::Vector2ui{ uf::renderer::settings::width, uf::renderer::settings::height } ) );
		} else if ( metadata["light"]["resolution"].is<float>() ) {
			size_t size = metadata["light"]["resolution"].as<size_t>();
			camera.setSize( {size, size} );
		} else {
			camera.setSize( pod::Vector2ui{ uf::renderer::settings::width, uf::renderer::settings::height } );
		}
		
		::roundRobin.lights.emplace_back(this);

		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		renderMode.metadata["type"] = "depth";
		renderMode.metadata["depth bias"] = metadata["light"]["bias"];
		renderMode.metadata["renderMode"] = metadata["renderMode"];
		// std::cout << this->getName() << ": " << renderMode.metadata["renderMode"] << std::endl;

		if ( metadata["light"]["type"].as<std::string>() == "point" ) {
			metadata["light"]["fov"] = 90.0f;
			renderMode.metadata["subpasses"] = 6;
		}
		if ( metadata["light"]["fov"].is<float>() ) {
			camera.setFov( metadata["light"]["fov"].as<float>() );
		}
		camera.updateView();
		camera.updateProjection();
		
		std::string name = "RT:" + std::to_string((int) this->getUid());
		uf::renderer::addRenderMode( &renderMode, name );
		renderMode.blitter.process = false;
		auto& cameraSize = camera.getSize();
		renderMode.width = cameraSize.x;
		renderMode.height = cameraSize.y;

	}

}
void ext::LightBehavior::tick( uf::Object& self ) {
	auto& metadata = this->getComponent<uf::Serializer>();
	if ( this->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		renderMode.setTarget("");
	}
	
	auto& transform = this->getComponent<pod::Transform<>>();
	auto& parent = this->getParent();
	auto& parentMetadata = parent.getComponent<uf::Serializer>();
	auto& parentTransform = parent.getComponent<pod::Transform<>>();
	// copy from our parent
//	if ( parentMetadata["system"]["name"] == "Light" ) {
	if ( metadata["light"]["bound"].as<bool>() ) {
		metadata["light"] = parentMetadata["light"];
		metadata["light"]["bound"] = true;
		if ( metadata["light"]["static"].as<bool>() ) {
			auto flatten = uf::transform::flatten( parentTransform );
			transform.position = flatten.position;
		}
	} else {
		// light fade action
		if ( ext::json::isObject( metadata["light"]["fade"] ) ) {
			if ( ext::json::isNull( metadata["light"]["backup"]["power"] ) ) {
				metadata["light"]["backup"]["power"] = metadata["light"]["power"];
			}
			if ( ext::json::isNull( metadata["light"]["backup"]["color"] ) ) {
				metadata["light"]["backup"]["color"] = metadata["light"]["color"];
			}
			// fade towards
			int direction = metadata["light"]["fade"]["increment"].as<bool>() ? 1 : -1;
			metadata["light"]["fade"]["timer"] = metadata["light"]["fade"]["timer"].as<float>() + metadata["light"]["fade"]["rate"].as<float>() * uf::physics::time::delta * direction;

			// 0 .. delta .. 1 .. (1 + timeout * 0.5)
			if ( direction == 1 && metadata["light"]["fade"]["timer"].as<float>() >= 0.5f * metadata["light"]["fade"]["timeout"].as<float>() + 1.0f ) {
				metadata["light"]["fade"]["increment"] = false;
			} else if ( direction == -1 && metadata["light"]["fade"]["timer"].as<float>() <= -0.5f * metadata["light"]["fade"]["timeout"].as<float>() ) {
				metadata["light"]["fade"]["increment"] = true;
			}
			{
				float delta = metadata["light"]["fade"]["timer"].as<float>();
				delta = std::clamp( delta, 0.f, 1.f );
				if ( metadata["light"]["fade"]["power"].is<double>() ) {
					metadata["light"]["power"] = uf::math::lerp( metadata["light"]["backup"]["power"].as<float>(), metadata["light"]["fade"]["power"].as<float>(), delta );
				}
				if ( ext::json::isArray( metadata["light"]["fade"]["color"] ) ) {
					pod::Vector3f fadeColor; {
						fadeColor.x = metadata["light"]["fade"]["color"][0].as<float>();
						fadeColor.y = metadata["light"]["fade"]["color"][1].as<float>();
						fadeColor.z = metadata["light"]["fade"]["color"][2].as<float>();
					}
					pod::Vector3f origColor; {
						origColor.x = metadata["light"]["backup"]["color"][0].as<float>();
						origColor.y = metadata["light"]["backup"]["color"][1].as<float>();
						origColor.z = metadata["light"]["backup"]["color"][2].as<float>();
					}
					pod::Vector3f color = uf::vector::lerp( origColor, fadeColor, delta );

					metadata["light"]["color"][0] = color[0];
					metadata["light"]["color"][1] = color[1];
					metadata["light"]["color"][2] = color[2];
				}
			}
		}
		// light flicker action
		if ( ext::json::isObject( metadata["light"]["flicker"] ) ) {
			float r = (rand() % 100) / 100.0;
			float rate = metadata["light"]["flicker"]["rate"].as<float>();
			if ( ext::json::isNull( metadata["light"]["backup"]["power"] ) ) {
				metadata["light"]["backup"]["power"] = metadata["light"]["power"];
			}
			metadata["light"]["flicker"]["timer"] = metadata["light"]["flicker"]["timer"].as<float>() + uf::physics::time::delta;
			if ( metadata["light"]["flicker"]["timer"].as<float>() >= metadata["light"]["flicker"]["timeout"].as<float>() ) {
				metadata["light"]["flicker"]["timer"] = 0;
				metadata["light"]["power"] = (r > rate) ? metadata["light"]["flicker"]["power"].as<float>() : metadata["light"]["backup"]["power"].as<float>();
			}
		}
	}

	// skip if we're handling the camera view matrix position ourselves
	if ( !metadata["light"]["external update"].as<bool>() ) {
		auto& camera = this->getComponent<uf::Camera>();
		// omni light
		if ( metadata["light"]["shadows"].as<bool>() && metadata["light"]["type"].as<std::string>() == "point" ) {
			auto transform = camera.getTransform();
			std::vector<pod::Quaternion<>> rotations = {
			/*
				{0, 0, 0, 1},
				{0, 0.707107, 0, -0.707107},
				{0, 1, 0, 0},
				{0, 0.707107, 0, 0.707107},
				{-0.707107, 0, 0, -0.707107},
				{0.707107,  0, 0, -0.707107},
			*/
				uf::quaternion::axisAngle( { 0, 1, 0 }, 0 * 1.57079633 ),
				uf::quaternion::axisAngle( { 0, 1, 0 }, 1 * 1.57079633 ),
				uf::quaternion::axisAngle( { 0, 1, 0 }, 2 * 1.57079633 ),
				uf::quaternion::axisAngle( { 0, 1, 0 }, 3 * 1.57079633 ),
				
				uf::quaternion::axisAngle( { 1, 0, 0 }, 1 * 1.57079633 ),
				uf::quaternion::axisAngle( { 1, 0, 0 }, 3 * 1.57079633 ),
			};
			for ( size_t i = 0; i < rotations.size(); ++i ) {
				auto transform = camera.getTransform();
				transform.orientation = rotations[i];
				camera.setView( uf::matrix::inverse( uf::transform::model( transform, false ) ), i );
			}
		} else {
			camera.updateView();
		}
	//	for ( std::size_t i = 0; i < 2; ++i ) camera.setView( uf::matrix::inverse( uf::transform::model( transform ) ), i );
	}

	// limit updating our shadow map
	if ( this->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
		bool execute = true;
		// enable renderer every X seconds
		if ( metadata["system"]["renderer"]["limiter"].is<float>() ) {
			if ( metadata["system"]["renderer"]["timer"].as<float>() > metadata["system"]["renderer"]["limiter"].as<float>() ) {
				metadata["system"]["renderer"]["timer"] = 0;
				execute = true;
			} else {
				metadata["system"]["renderer"]["timer"] = metadata["system"]["renderer"]["timer"].as<float>() + uf::physics::time::delta;
				execute = false;
			}
		} else if ( metadata["system"]["renderer"]["mode"].is<std::string>() ) {
			std::string mode = metadata["system"]["renderer"]["mode"].as<std::string>();
			// round robin, enable if it's the light's current turn
			if ( mode == "round robin" ) {
				if ( ::roundRobin.current < ::roundRobin.lights.size() ) execute = ::roundRobin.lights[::roundRobin.current] == this;
			// render only if the light is used
			} else if ( mode == "occlusion" ) {
				execute = metadata["system"]["renderer"]["rendered"].as<bool>();
			// light baking, but sadly re-bakes every time the command buffer is recorded
			} else if ( mode == "once" ) {
				execute = !metadata["system"]["renderer"]["rendered"].as<bool>();
				metadata["system"]["renderer"]["rendered"] = true;
			}
		}
		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		renderMode.execute = execute;
	}
}
void ext::LightBehavior::render( uf::Object& self ){

}
void ext::LightBehavior::destroy( uf::Object& self ){
	if ( this->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		uf::renderer::removeRenderMode( &renderMode, false );
		this->deleteComponent<uf::renderer::RenderTargetRenderMode>();
	}

	::roundRobin.lights.erase(std::remove(::roundRobin.lights.begin(), ::roundRobin.lights.end(), this), ::roundRobin.lights.end());
	::roundRobin.current = 0;
}
#undef this