#include "behavior.h"

#include <uf/utils/renderer/renderer.h>

#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/camera/camera.h>

EXT_BEHAVIOR_REGISTER_CPP(LightBehavior)
#define this (&self)
void ext::LightBehavior::initialize( uf::Object& self ) {
	auto& metadata = this->getComponent<uf::Serializer>();
	auto& transform = this->getComponent<pod::Transform<>>();
	auto& camera = this->getComponent<uf::Camera>();
	auto& scene = uf::scene::getCurrentScene();
	auto& controller = scene.getController();

	if ( metadata["light"]["shadows"]["enabled"].asBool() ) {
		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		std::string name = "RT:" + std::to_string((int) this->getUid());
		uf::renderer::addRenderMode( &renderMode, name );
		renderMode.blitter.process = false;
		camera = controller.getComponent<uf::Camera>();
		camera.getTransform() = {};
		camera.setStereoscopic(false);
		if ( metadata["light"]["shadows"]["fov"].isNumeric() ) {
			camera.setFov( metadata["light"]["shadows"]["fov"].asFloat() );
			camera.updateProjection();
		}
		if ( metadata["light"]["radius"].isArray() ) {
			auto bounds = camera.getBounds();
			bounds.x = metadata["light"]["radius"][0].asFloat();
			bounds.y = metadata["light"]["radius"][1].asFloat();
			camera.setBounds(bounds);
		}
		if ( metadata["light"]["shadows"]["resolution"].isArray() ) {
			renderMode.width = metadata["light"]["shadows"]["resolution"][0].asUInt64();
			renderMode.height = metadata["light"]["shadows"]["resolution"][1].asUInt64();
			auto size = camera.getSize();
			size.x = renderMode.width;
			size.y = renderMode.height;
			camera.setSize(size);
		} else {
			renderMode.width = metadata["light"]["shadows"]["resolution"].asUInt64();
			renderMode.height = metadata["light"]["shadows"]["resolution"].asUInt64();
			auto size = camera.getSize();
			size.x = renderMode.width;
			size.y = renderMode.height;
			camera.setSize(size);
		}
	}
	if ( !metadata["light"]["shadows"]["bias"].isNumeric() ) {
		metadata["light"]["shadows"]["bias"] = 0.00005f;
	}
	if ( !metadata["light"]["color"].isArray() ) {
		metadata["light"]["color"][0] = 1; //metadata["light"]["color"]["random"].asBool() ? (rand() % 100) / 100.0 : 1;
		metadata["light"]["color"][1] = 1; //metadata["light"]["color"]["random"].asBool() ? (rand() % 100) / 100.0 : 1;
		metadata["light"]["color"][2] = 1; //metadata["light"]["color"]["random"].asBool() ? (rand() % 100) / 100.0 : 1;
	}
}
void ext::LightBehavior::tick( uf::Object& self ) {
	auto& metadata = this->getComponent<uf::Serializer>();
	if ( this->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		renderMode.target = "";
	}
	
	auto& transform = this->getComponent<pod::Transform<>>();
	if (  this->hasComponent<pod::Physics>() ) {
		pod::Physics& physics = this->getComponent<pod::Physics>();
		transform = uf::physics::update( transform, physics );
	}

	auto& parent = this->getParent();
	auto& parentMetadata = parent.getComponent<uf::Serializer>();
	if ( parentMetadata["system"]["name"] == "Light" ) {
		metadata["light"] = parentMetadata["light"];
	} else {
		if ( metadata["light"]["fade"].isObject() ) {
			if ( metadata["light"]["backup"]["power"].isNull() ) {
				metadata["light"]["backup"]["power"] = metadata["light"]["power"];
			}
			if ( metadata["light"]["backup"]["color"].isNull() ) {
				metadata["light"]["backup"]["color"] = metadata["light"]["color"];
			}
			// fade towards
			int direction = metadata["light"]["fade"]["increment"].asBool() ? 1 : -1;
			metadata["light"]["fade"]["timer"] = metadata["light"]["fade"]["timer"].asFloat() + metadata["light"]["fade"]["rate"].asFloat() * uf::physics::time::delta * direction;

			// 0 .. delta .. 1 .. (1 + timeout * 0.5)

			if ( direction == 1 && metadata["light"]["fade"]["timer"].asFloat() >= 0.5f * metadata["light"]["fade"]["timeout"].asFloat() + 1.0f ) {
				metadata["light"]["fade"]["increment"] = false;
			} else if ( direction == -1 && metadata["light"]["fade"]["timer"].asFloat() <= -0.5f * metadata["light"]["fade"]["timeout"].asFloat() ) {
				metadata["light"]["fade"]["increment"] = true;
			}
			{
				float delta = metadata["light"]["fade"]["timer"].asFloat();
				delta = std::clamp( delta, 0.f, 1.f );
				if ( metadata["light"]["fade"]["power"].isNumeric() ) {
					metadata["light"]["power"] = uf::math::lerp( metadata["light"]["backup"]["power"].asFloat(), metadata["light"]["fade"]["power"].asFloat(), delta );
				}
				if ( metadata["light"]["fade"]["color"].isArray() ) {
					pod::Vector3f fadeColor; {
						fadeColor.x = metadata["light"]["fade"]["color"][0].asFloat();
						fadeColor.y = metadata["light"]["fade"]["color"][1].asFloat();
						fadeColor.z = metadata["light"]["fade"]["color"][2].asFloat();
					}
					pod::Vector3f origColor; {
						origColor.x = metadata["light"]["backup"]["color"][0].asFloat();
						origColor.y = metadata["light"]["backup"]["color"][1].asFloat();
						origColor.z = metadata["light"]["backup"]["color"][2].asFloat();
					}
					pod::Vector3f color = uf::vector::lerp( origColor, fadeColor, delta );

					metadata["light"]["color"][0] = color[0];
					metadata["light"]["color"][1] = color[1];
					metadata["light"]["color"][2] = color[2];
				}
			}
		}
		if ( metadata["light"]["flicker"].isObject() ) {
			float r = (rand() % 100) / 100.0;
			float rate = metadata["light"]["flicker"]["rate"].asFloat();
			if ( metadata["light"]["backup"]["power"].isNull() ) {
				metadata["light"]["backup"]["power"] = metadata["light"]["power"];
			}
			metadata["light"]["flicker"]["timer"] = metadata["light"]["flicker"]["timer"].asFloat() + uf::physics::time::delta;
			if ( metadata["light"]["flicker"]["timer"].asFloat() >= metadata["light"]["flicker"]["timeout"].asFloat() ) {
				metadata["light"]["flicker"]["timer"] = 0;
				metadata["light"]["power"] = (r > rate) ? metadata["light"]["flicker"]["power"].asFloat() : metadata["light"]["backup"]["power"];
			}
		}
	}

	if ( parentMetadata["system"]["type"] == "Light" ) {
		auto& parentTransform = parent.getComponent<pod::Transform<>>();
		transform.position = parentTransform.position;
	} else if ( metadata["system"]["track"] == "player" ) {
		auto& scene = uf::scene::getCurrentScene();
		auto& controllerCamera = scene.getController().getComponent<uf::Camera>().getTransform();
		auto& controllerTransform = scene.getController().getComponent<pod::Transform<>>();
		transform.position = controllerTransform.position + controllerCamera.position;
	}
	if ( metadata["light"]["external update"].isNull() || (!metadata["light"]["external update"].isNull() && !metadata["light"]["external update"].asBool()) ) {
		auto& camera = this->getComponent<uf::Camera>();
		for ( std::size_t i = 0; i < 2; ++i ) {
			camera.setView( uf::matrix::inverse( uf::transform::model( transform ) ), i );
		}
	}

	// render every other frame, maybe
	if ( false ) {
		if ( metadata["system"]["renderer"]["limit rendering"]["limiter"].isNumeric() && this->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
			auto& limiterMetadata = metadata["system"]["renderer"]["limit rendering"];
			auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
			if ( limiterMetadata["timer"].asFloat() > 1.0f / limiterMetadata["limiter"].asFloat() ) {
				limiterMetadata["timer"] = 0;
				renderMode.execute = true;
			} else {
				limiterMetadata["timer"] = limiterMetadata["timer"].asFloat() + uf::physics::time::delta;
				renderMode.execute = false;
			}
		}
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
}
#undef this