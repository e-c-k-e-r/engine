#include "behavior.h"

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

#include <uf/utils/renderer/renderer.h>

UF_BEHAVIOR_REGISTER_CPP(ext::PortalsBehavior)
UF_BEHAVIOR_REGISTER_AS_OBJECT(ext::PortalsBehavior, Portals)
#define this (&self)
void ext::PortalsBehavior::initialize( uf::Object& self ) {
	auto& metadata = this->getComponent<uf::Serializer>();

	uf::Object& red = *(new uf::Object); this->addChild(red); uf::instantiator::bind( "Portal", red );
	uf::Object& blue = *(new uf::Object); this->addChild(blue); uf::instantiator::bind( "Portal", blue );

	metadata["portals"][0]["name"] = "Portal";
	metadata["portals"][1]["name"] = "Portal";

	red.load( (uf::Serializer) metadata["portals"][0] );
	blue.load( (uf::Serializer) metadata["portals"][1] );

	red.initialize();
	blue.initialize();

	red.getComponent<uf::Serializer>()["target"] = blue.getUid();
	blue.getComponent<uf::Serializer>()["target"] = red.getUid();
}
void ext::PortalsBehavior::tick( uf::Object& self ) {
}
void ext::PortalsBehavior::render( uf::Object& self ){}
void ext::PortalsBehavior::destroy( uf::Object& self ){}
#undef this

UF_BEHAVIOR_REGISTER_CPP(ext::PortalBehavior)
UF_BEHAVIOR_REGISTER_AS_OBJECT(ext::PortalBehavior, Portal)
#define this (&self)
void ext::PortalBehavior::initialize( uf::Object& self ) {
	auto& metadata = this->getComponent<uf::Serializer>();
	auto& transform = this->getComponent<pod::Transform<>>();
	auto& camera = this->getComponent<uf::Camera>();
	{
		auto& scene = uf::scene::getCurrentScene();
		auto& controller = scene.getController();
		// copies camera settings
		camera = controller.getComponent<uf::Camera>();
	}
	{
		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		std::string name = "RT:" + std::to_string((int) this->getUid());
		uf::renderer::addRenderMode( &renderMode, name );
		if ( ext::openvr::enabled ) {
			ext::openvr::initialize();
		
			uint32_t width, height;
			ext::openvr::recommendedResolution( width, height );

			renderMode.width = width;
			renderMode.height = height;
		}
	}
}
void ext::PortalBehavior::tick( uf::Object& self ) {
	auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
	renderMode.setTarget("");
	
	auto& scene = uf::scene::getCurrentScene();
	auto& controller = scene.getController();
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
		auto& otherSideTransform = this->getParent().findByUid( metadata["target"].as<size_t>() )->getComponent<pod::Transform<>>();

		for ( std::size_t i = 0; i < 2; ++i ) {
			pod::Matrix4f controllerCameraToWorldMatrix = uf::matrix::inverse( conCamera.getView(i) );
			pod::Matrix4f worldToPortalMatrix = uf::matrix::inverse( uf::transform::model( prtTransform ) );
			pod::Matrix4f otherSideToWorldMatrix = uf::transform::model( otherSideTransform );
			camera.setView( uf::matrix::inverse(otherSideToWorldMatrix * worldToPortalMatrix * controllerCameraToWorldMatrix), i );
		}
	}
}
void ext::PortalBehavior::render( uf::Object& self ){
	{
		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		auto& blitter = renderMode.blitter;

		auto& transform = this->getComponent<pod::Transform<>>();
		auto& scene = uf::scene::getCurrentScene();
		auto& controller = scene.getController();
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
void ext::PortalBehavior::destroy( uf::Object& self ){
	auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
	uf::renderer::removeRenderMode( &renderMode, false );
	this->deleteComponent<uf::renderer::RenderTargetRenderMode>();
}
#undef this