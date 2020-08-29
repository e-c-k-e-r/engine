#include "light.h"

#include <uf/ext/vulkan/rendertarget.h>
#include <uf/ext/vulkan/rendermodes/rendertarget.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/camera/camera.h>

EXT_OBJECT_REGISTER_CPP(Light)
void ext::Light::initialize() {
	uf::Object::initialize();
	auto& metadata = this->getComponent<uf::Serializer>();
	auto& transform = this->getComponent<pod::Transform<>>();
	auto& camera = this->getComponent<uf::Camera>();
	{
		auto& scene = uf::scene::getCurrentScene();
		auto& controller = *scene.getController();
		camera = controller.getComponent<uf::Camera>();

		camera.setFov( metadata["light"]["fov"].asFloat() );
	}
	{
		auto& renderMode = this->getComponent<ext::vulkan::RenderTargetRenderMode>();
		renderMode.width = 512;
		renderMode.height = 512;
		std::string name = "Render Target: " + std::to_string((int) this->getUid());
		ext::vulkan::addRenderMode( &renderMode, name );
	}
}
void ext::Light::tick() {
	uf::Object::tick();

	auto& renderMode = this->getComponent<ext::vulkan::RenderTargetRenderMode>();
	renderMode.target = "";
	
	auto& camera = this->getComponent<uf::Camera>();
	auto& transform = this->getComponent<pod::Transform<>>();
	for ( std::size_t i = 0; i < 2; ++i ) {
		camera.setView( uf::matrix::inverse( uf::transform::model( transform ) ), i );
	}
}
void ext::Light::render() {
	uf::Object::render();
/*
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
*/
}
void ext::Light::destroy() {
	auto& renderMode = this->getComponent<ext::vulkan::RenderTargetRenderMode>();
	ext::vulkan::removeRenderMode( &renderMode, false );
	uf::Object::destroy();
}