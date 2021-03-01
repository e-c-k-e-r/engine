#include <uf/engine/object/behaviors/render.h>

#include <uf/engine/object/object.h>
#include <uf/engine/asset/asset.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/time/time.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/ext/gltf/gltf.h>

#define UF_UNIFORMS_UPDATE_WITH_JSON 0

UF_BEHAVIOR_REGISTER_CPP(uf::RenderBehavior)
#define this (&self)
void uf::RenderBehavior::initialize( uf::Object& self ) {}
void uf::RenderBehavior::destroy( uf::Object& self ) {}
void uf::RenderBehavior::tick( uf::Object& self ) {}
void uf::RenderBehavior::render( uf::Object& self ) {
	if ( !this->hasComponent<uf::Graphic>() ) return;
	auto& metadata = this->getComponent<uf::Serializer>();
	auto& scene = uf::scene::getCurrentScene();
	auto& graphic = this->getComponent<uf::Graphic>();
	auto& transform = this->getComponent<pod::Transform<>>();
	auto& controller = scene.getController();
	auto& camera = controller.getComponent<uf::Camera>();
	
	if ( !graphic.initialized ) return;
#if UF_USE_OPENGL
	auto uniformBuffer = graphic.getUniform();
	pod::Uniform& uniform = *((pod::Uniform*) uf::renderer::device.getBuffer(uniformBuffer.buffer));
	uniform.projection = camera.getProjection();
	uniform.modelView = camera.getView(); // * uf::transform::model( transform );
	graphic.updateUniform( &uniform, sizeof(uniform) );
#elif UF_USE_VULKAN
	if ( !graphic.material.hasShader("vertex") ) return;

	auto& shader = graphic.material.getShader("vertex");
	auto& uniform = shader.getUniform("UBO");

	if ( !ext::json::isNull(metadata["system"]["loaded"]) && !metadata["system"]["loaded"].as<bool>() ) return;
#if UF_UNIFORMS_UPDATE_WITH_JSON
//	auto uniforms = shader.getUniformJson("UBO");
	ext::json::Value uniforms;
	uniforms["matrices"]["model"] = uf::matrix::encode( uf::transform::model( transform ) );
	for ( std::size_t i = 0; i < uf::renderer::settings::maxViews; ++i ) {
		uniforms["matrices"]["view"][i] = uf::matrix::encode( camera.getView( i ) );
		uniforms["matrices"]["projection"][i] = uf::matrix::encode( camera.getProjection( i ) );
	}
	if ( ext::json::isArray(metadata["color"]) ) {
		uniforms["color"][0] = metadata["color"][0];
		uniforms["color"][1] = metadata["color"][1];
		uniforms["color"][2] = metadata["color"][2];
		uniforms["color"][3] = metadata["color"][3];
	} else {
		uniforms["color"][0] = 1.0f;
		uniforms["color"][1] = 1.0f;
		uniforms["color"][2] = 1.0f;
		uniforms["color"][3] = 1.0f;
	}
	shader.updateUniform("UBO", uniforms );
#else
	auto& uniforms = uniform.get<uf::MeshDescriptor<>>();
	uniforms.matrices.model = uf::transform::model( transform );
	for ( std::size_t i = 0; i < uf::renderer::settings::maxViews; ++i ) {
		uniforms.matrices.view[i] = camera.getView( i );
		uniforms.matrices.projection[i] = camera.getProjection( i );
	}
	if ( ext::json::isArray(metadata["color"]) ) {
		uniforms.color[0] = metadata["color"][0].as<float>();
		uniforms.color[1] = metadata["color"][1].as<float>();
		uniforms.color[2] = metadata["color"][2].as<float>();
		uniforms.color[3] = metadata["color"][3].as<float>();
	} else {
		uniforms.color = { 1, 1, 1, 1 };
	}
	shader.updateUniform( "UBO", uniform );
#endif
#endif
}
#undef this