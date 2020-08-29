#include "portal.h"

#include <uf/utils/graphic/mesh.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/math/collision.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/math/physics.h>

#include <uf/utils/window/window.h>

#include <uf/ext/openvr/openvr.h>

#include <uf/ext/vulkan/rendermodes/rendertarget.h>

#include "../terrain/generator.h"
#include "../world.h"

namespace {

}

EXT_OBJECT_REGISTER_CPP(Portals)
void ext::Portals::initialize() {
	uf::Object::initialize();

	auto& metadata = this->getComponent<uf::Serializer>();

	ext::Portal& red = *(new ext::Portal); this->addChild(red);
	ext::Portal& blue = *(new ext::Portal); this->addChild(blue);

	metadata["portals"][0]["name"] = "Portal";
	metadata["portals"][1]["name"] = "Portal";

	red.load( metadata["portals"][0] );
	blue.load( metadata["portals"][1] );

	red.initialize();
	blue.initialize();

	red.getComponent<uf::Serializer>()["target"] = blue.getUid();
	blue.getComponent<uf::Serializer>()["target"] = red.getUid();
}
void ext::Portals::tick() {
	uf::Object::tick();
}
void ext::Portals::render() {
	uf::Object::render();
}
void ext::Portals::destroy() {
	uf::Object::destroy();
}

EXT_OBJECT_REGISTER_CPP(Portal)
void ext::Portal::initialize() {
	uf::Object::initialize();

	auto& metadata = this->getComponent<uf::Serializer>();
	auto& transform = this->getComponent<pod::Transform<>>();
	auto& camera = this->getComponent<uf::Camera>();
	{
		auto& scene = uf::scene::getCurrentScene();
		auto& controller = *scene.getController();
		// copies camera settings
		camera = controller.getComponent<uf::Camera>();
	}
	{
		auto& renderMode = this->getComponent<ext::vulkan::RenderTargetRenderMode>();
		std::string name = "Render Target: " + std::to_string((int) this->getUid());
		ext::vulkan::addRenderMode( &renderMode, name );
		if ( ext::openvr::enabled ) {
			ext::openvr::initialize();
		
			uint32_t width, height;
			ext::openvr::recommendedResolution( width, height );

			renderMode.width = width;
			renderMode.height = height;
		}
	}
}
void ext::Portal::tick() {
	uf::Object::tick();
	
	auto& renderMode = this->getComponent<ext::vulkan::RenderTargetRenderMode>();
	renderMode.target = "";
	
	auto& scene = uf::scene::getCurrentScene();
	auto& controller = *scene.getController();
	auto& camera = this->getComponent<uf::Camera>();
	auto& metadata = this->getComponent<uf::Serializer>();
/*
	static pod::Transform<> otherSideTransform = this->getComponent<pod::Transform<>>();
	{
		float step = 4.0f;
		if ( uf::Window::isKeyPressed("B") ) otherSideTransform.position.x -= step * uf::physics::time::delta;
		if ( uf::Window::isKeyPressed("M") ) otherSideTransform.position.x += step * uf::physics::time::delta;
		if ( uf::Window::isKeyPressed("G") ) otherSideTransform.position.y += step * uf::physics::time::delta;
		if ( uf::Window::isKeyPressed("J") ) otherSideTransform.position.y -= step * uf::physics::time::delta;
		if ( uf::Window::isKeyPressed("H") ) otherSideTransform.position.z += step * uf::physics::time::delta;
		if ( uf::Window::isKeyPressed("N") ) otherSideTransform.position.z -= step * uf::physics::time::delta;
		if ( uf::Window::isKeyPressed("L") ) {
			std::cout << otherSideTransform.position.x << ", " << otherSideTransform.position.y << ", " << otherSideTransform.position.z << "\t" << step * uf::physics::time::delta << std::endl;
		}
	}
*/
	{
		auto& conCamera = controller.getComponent<uf::Camera>();
		auto& prtTransform = this->getComponent<pod::Transform<>>();
		auto& otherSideTransform = this->getParent().findByUid( metadata["target"].asUInt() )->getComponent<pod::Transform<>>();

		for ( std::size_t i = 0; i < 2; ++i ) {
			pod::Matrix4f controllerCameraToWorldMatrix = uf::matrix::inverse( conCamera.getView(i) );
			pod::Matrix4f worldToPortalMatrix = uf::matrix::inverse( uf::transform::model( prtTransform ) );
			pod::Matrix4f otherSideToWorldMatrix = uf::transform::model( otherSideTransform );
			camera.setView( uf::matrix::inverse(otherSideToWorldMatrix * worldToPortalMatrix * controllerCameraToWorldMatrix), i );
		}
	}

}
void ext::Portal::render() {
	uf::Object::render();
	{
		auto& renderMode = this->getComponent<ext::vulkan::RenderTargetRenderMode>();
		auto& blitter = renderMode.blitter;

		auto& transform = this->getComponent<pod::Transform<>>();
		auto& scene = uf::scene::getCurrentScene();
		auto& controller = *scene.getController();
		auto& camera = this->getComponent<uf::Camera>();
		auto& controllerCamera = controller.getComponent<uf::Camera>();
		if ( !blitter.initialized ) return;

		struct UniformDescriptor {
			struct {
				alignas(16) pod::Matrix4f models[2];
			} matrices;
			struct {
				alignas(8) pod::Vector2f position = { 0.5f, 0.5f };
				alignas(8) pod::Vector2f radius = { 0.1f, 0.1f };
				alignas(16) pod::Vector4f color = { 1, 1, 1, 1 };
			} cursor;
			alignas(4) float alpha;
		};

		auto& shader = blitter.material.shaders.front();
		auto& uniforms = shader.uniforms.front().get<UniformDescriptor>();

		for ( std::size_t i = 0; i < 2; ++i ) {
			pod::Matrix4f model = uf::transform::model( transform );

			uniforms.matrices.models[i] = controllerCamera.getProjection(i) * controllerCamera.getView(i) * model;

			uniforms.alpha = 1.0f;

			uniforms.cursor.position.x = -1.0f;
			uniforms.cursor.position.y = -1.0f;

			uniforms.cursor.radius.x = 0.0f;
			uniforms.cursor.radius.y = 0.0f;
			
			uniforms.cursor.color.x = 0.0f;
			uniforms.cursor.color.y = 0.0f;
			uniforms.cursor.color.z = 0.0f;
			uniforms.cursor.color.w = 0.0f;
		}
		shader.updateBuffer( (void*) &uniforms, sizeof(uniforms), 0 );
	}
}
void ext::Portal::destroy() {
	auto& renderMode = this->getComponent<ext::vulkan::RenderTargetRenderMode>();
	ext::vulkan::removeRenderMode( &renderMode, false );
	uf::Object::destroy();
}