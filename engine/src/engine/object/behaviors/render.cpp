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

UF_BEHAVIOR_REGISTER_CPP(RenderBehavior)
#define this (&self)
void uf::RenderBehavior::initialize( uf::Object& self ) {	

}
void uf::RenderBehavior::destroy( uf::Object& self ) {

}
void uf::RenderBehavior::tick( uf::Object& self ) {

}
void uf::RenderBehavior::render( uf::Object& self ) {
	if ( !this->hasComponent<uf::Graphic>() ) return;
	auto& metadata = this->getComponent<uf::Serializer>();
	auto& scene = uf::scene::getCurrentScene();
	auto& graphic = this->getComponent<uf::Graphic>();
	auto& transform = this->getComponent<pod::Transform<>>();
	auto& controller = scene.getController();
	auto& camera = controller.getComponent<uf::Camera>();		
	
	if ( !graphic.initialized ) return;

	auto& uniforms = graphic.material.shaders.front().uniforms.front().get<uf::StereoMeshDescriptor>();
	uniforms.matrices.model = uf::transform::model( transform );
	for ( std::size_t i = 0; i < 2; ++i ) {
		uniforms.matrices.view[i] = camera.getView( i );
		uniforms.matrices.projection[i] = camera.getProjection( i );
	}

	uniforms.color[0] = 1;
	uniforms.color[1] = 1;
	uniforms.color[2] = 1;
	uniforms.color[3] = 1;

	graphic.material.shaders.front().updateBuffer( uniforms, 0, false );
}
#undef this