#include <uf/engine/object/behaviors/render.h>

#include <uf/engine/object/object.h>
#include <uf/engine/asset/asset.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/time/time.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/ext/gltf/gltf.h>

#define UF_UNIFORMS_UPDATE_WITH_JSON 0

UF_BEHAVIOR_REGISTER_CPP(uf::RenderBehavior)
UF_BEHAVIOR_TRAITS_CPP(uf::RenderBehavior, ticks = true, renders = true, multithread = false)
#define this (&self)
void uf::RenderBehavior::initialize( uf::Object& self ) {}
void uf::RenderBehavior::tick( uf::Object& self ) {
	if ( !this->hasComponent<uf::Graphic>() ) return;
	const auto& metadata = this->getComponent<uf::Serializer>();
	const auto& scene = uf::scene::getCurrentScene();
	const auto& graphic = this->getComponent<uf::Graphic>();
	const auto& transform = this->getComponent<pod::Transform<>>();
	const auto& controller = scene.getController();
	const auto& camera = controller.getComponent<uf::Camera>();
	
	if ( !graphic.initialized ) return;
#if UF_USE_OPENGL
/*
	auto uniformBuffer = graphic.getUniform();
	pod::Uniform& uniform = *((pod::Uniform*) uf::renderer::device.getBuffer(uniformBuffer.buffer));
	uniform.projection = camera.getProjection();
	uniform.modelView = camera.getView(); // * uf::transform::model( transform );
	graphic.updateUniform( &uniform, sizeof(uniform) );
*/
#elif UF_USE_VULKAN
	if ( graphic.material.hasShader("vertex") ) {
		auto& shader = graphic.material.getShader("vertex");
		auto& uniform = shader.getUniform("UBO");

	#if UF_UNIFORMS_UPDATE_WITH_JSON
	//	auto uniforms = shader.getUniformJson("UBO");
		ext::json::Value uniforms;
		uniforms["model"] = uf::matrix::encode( uf::transform::model( transform ) );
	//	uniforms["colors"] = metadata["color"];
		shader.updateUniform("UBO", uniforms );
	#else
		struct UniformDescriptor {
			/*alignas(16)*/ pod::Matrix4f model;
		};
	#if UF_UNIFORMS_REUSE
		auto& uniforms = uniform.get<UniformDescriptor>();
		uniforms = {
			.model = uf::transform::model( transform );
	//		.color = uf::vector::decode( metadata["color"], pod::Vector4f{ 1, 1, 1, 1 } ),
		};
		shader.updateUniform( "UBO", uniform );
	#else
		UniformDescriptor uniforms = {
			.model = uf::transform::model( transform ),
	//		.color = uf::vector::decode( metadata["color"], pod::Vector4f{ 1, 1, 1, 1 } ),
		};
		graphic.updateBuffer( uniforms, shader.getUniformBuffer("UBO") );
	#endif
	#endif
	}
#endif
}
void uf::RenderBehavior::render( uf::Object& self ) {
	if ( !this->hasComponent<uf::Graphic>() ) return;
	const auto& metadata = this->getComponent<uf::Serializer>();
	const auto& scene = uf::scene::getCurrentScene();
	const auto& graphic = this->getComponent<uf::Graphic>();
	const auto& transform = this->getComponent<pod::Transform<>>();
	const auto& controller = scene.getController();
	const auto& camera = controller.getComponent<uf::Camera>();
	
	if ( !graphic.initialized ) return;
#if UF_USE_OPENGL
#if UF_UNIFORMS_REUSE
	auto uniforms = graphic.getUniform();
	pod::Uniform& uniform = *((pod::Uniform*) uf::renderer::device.getBuffer(uniforms.buffer));
	uniform = {
		.modelView = camera.getView() * uf::transform::model( transform ),
		.projection = camera.getProjection(),
	};
	graphic.updateUniform( &uniform, sizeof(uniform) );
#else
	pod::Uniform uniform = {
		.modelView = camera.getView() * uf::transform::model( transform ),
		.projection = camera.getProjection(),
	};
	graphic.updateUniform( &uniform, sizeof(uniform) );
#endif
#elif UF_USE_VULKAN
	if ( graphic.material.hasShader("vertex") ) {
		auto& shader = graphic.material.getShader("vertex");
		auto& uniform = shader.getUniform("Camera");

	#if UF_UNIFORMS_UPDATE_WITH_JSON
	//	auto uniforms = shader.getUniformJson("Camera");
		ext::json::Value uniforms;
		ext::json::reserve( uniforms["viewport"], uf::camera::maxViews );
		for ( uint_fast8_t i = 0; i < uf::camera::maxViews; ++i ) {
			uniforms["viewport"][i]["view"] = uf::matrix::encode( camera.getView( i ) );
			uniforms["viewport"][i]["projection"] = uf::matrix::encode( camera.getProjection( i ) );
		}
		shader.updateUniform("Camera", uniforms);
	#else
	#if UF_UNIFORMS_REUSE
		auto& uniforms = uniform.get<pod::Camera::Viewports>();
		uniforms = camera.data().viewport;
		shader.updateUniform("Camera", uniform);
	#else
		pod::Camera::Viewports uniforms = camera.data().viewport;
		graphic.updateBuffer( uniforms, shader.getUniformBuffer("Camera") );
	#endif
	#endif
	}

#endif
}
void uf::RenderBehavior::destroy( uf::Object& self ) {}
void uf::RenderBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ) {}
void uf::RenderBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ) {}
#undef this