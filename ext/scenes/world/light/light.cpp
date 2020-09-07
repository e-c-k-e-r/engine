#include "light.h"

#include <uf/ext/vulkan/rendertarget.h>
#include <uf/ext/vulkan/rendermodes/rendertarget.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/camera/camera.h>

EXT_OBJECT_REGISTER_CPP(Light)
void ext::Light::initialize() {
	uf::Object::initialize();
	auto& metadata = this->getComponent<uf::Serializer>();
	auto& transform = this->getComponent<pod::Transform<>>();
	auto& camera = this->getComponent<uf::Camera>();
	if ( metadata["light"]["shadows"]["enabled"].asBool() ) {
		auto& renderMode = this->getComponent<ext::vulkan::RenderTargetRenderMode>();
		std::string name = "Render Target: " + std::to_string((int) this->getUid());
		ext::vulkan::addRenderMode( &renderMode, name );
		if ( metadata["light"]["shadows"]["resolution"].isArray() ) {
			renderMode.width = metadata["light"]["shadows"]["resolution"][0].asUInt64();
			renderMode.height = metadata["light"]["shadows"]["resolution"][1].asUInt64();
		} else {
			renderMode.width = metadata["light"]["shadows"]["resolution"].asUInt64();
			renderMode.height = metadata["light"]["shadows"]["resolution"].asUInt64();
		}
		{
			auto& scene = uf::scene::getCurrentScene();
			auto& controller = *scene.getController();

			camera = controller.getComponent<uf::Camera>();
			camera.getTransform() = {};
			camera.setStereoscopic(false);
			if ( metadata["light"]["shadows"]["fov"].isNumeric() ) {
				camera.setFov( metadata["light"]["shadows"]["fov"].asFloat() );
				camera.updateProjection();
			}
		}
	}
	if ( !metadata["light"].isArray() ) {
		metadata["light"]["color"][0] = 1; //metadata["light"]["color"]["random"].asBool() ? (rand() % 100) / 100.0 : 1;
		metadata["light"]["color"][1] = 1; //metadata["light"]["color"]["random"].asBool() ? (rand() % 100) / 100.0 : 1;
		metadata["light"]["color"][2] = 1; //metadata["light"]["color"]["random"].asBool() ? (rand() % 100) / 100.0 : 1;
	}
}
void ext::Light::tick() {
	uf::Object::tick();

	if ( this->hasComponent<ext::vulkan::RenderTargetRenderMode>() ) {
		auto& renderMode = this->getComponent<ext::vulkan::RenderTargetRenderMode>();
		renderMode.target = "";
	}
	
	auto& transform = this->getComponent<pod::Transform<>>();
	if (  this->hasComponent<pod::Physics>() ) {
		pod::Physics& physics = this->getComponent<pod::Physics>();
		transform = uf::physics::update( transform, physics );
	}
	auto& camera = this->getComponent<uf::Camera>();
	auto& metadata = this->getComponent<uf::Serializer>();
	if ( metadata["light"]["external update"].isNull() || (!metadata["light"]["external update"].isNull() && !metadata["light"]["external update"].asBool()) ) {
		for ( std::size_t i = 0; i < 2; ++i ) {
			camera.setView( uf::matrix::inverse( uf::transform::model( transform ) ), i );
		}
	}
}
void ext::Light::render() {
	uf::Object::render();
}
void ext::Light::destroy() {
	if ( this->hasComponent<ext::vulkan::RenderTargetRenderMode>() ) {
		auto& renderMode = this->getComponent<ext::vulkan::RenderTargetRenderMode>();
		ext::vulkan::removeRenderMode( &renderMode, false );
	}
	uf::Object::destroy();
}