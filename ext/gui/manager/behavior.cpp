#include "behavior.h"
#include "../gui.h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/window/window.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/math/physics.h>

#include <uf/utils/text/glyph.h>
#include <uf/engine/asset/asset.h>
#include <uf/engine/scene/scene.h>

#include <unordered_map>
#include <locale>
#include <codecvt>

#include <uf/utils/renderer/renderer.h>
#include <uf/ext/openvr/openvr.h>

#include <uf/utils/http/http.h>
#include <uf/utils/audio/audio.h>
#include <sys/stat.h>
#include <fstream>

#include <regex>

ext::gui::Size ext::gui::size = {
	.current = {
		0,
		0,
	},
#if UF_ENV_DREAMCAST
	.reference = { 960, 720 },
#else
	.reference = { 1920, 1080 },
#endif
};


UF_BEHAVIOR_REGISTER_CPP(ext::GuiManagerBehavior)
UF_BEHAVIOR_REGISTER_AS_OBJECT(ext::GuiManagerBehavior, ext::GuiManager)
#define this (&self)
void ext::GuiManagerBehavior::initialize( uf::Object& self ) {
	// add gui render mode
	if ( !uf::renderer::hasRenderMode( "Gui", true ) ) {
		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		std::string name = "Gui";
		uf::renderer::addRenderMode( &renderMode, name );
		renderMode.blitter.descriptor.subpass = 1;
	}

	auto& metadataJson = this->getComponent<uf::Serializer>();
	auto& assetLoader = this->getComponent<uf::Asset>();

	this->addHook( "window:Resized", [&](ext::json::Value& json){

		pod::Vector2ui size = uf::vector::decode( json["window"]["size"], pod::Vector2ui{} ); {
		//	size.x = json["window"]["size"]["x"].as<size_t>();
		//	size.y = json["window"]["size"]["y"].as<size_t>();
		}
		ext::gui::size.current = size;
	//	ext::gui::size.reference = size;
	} );
	this->addHook( "window:Mouse.Moved", [&](ext::json::Value& json){

		bool down = json["mouse"]["state"].as<std::string>() == "Down";
		bool clicked = false;
		pod::Vector2ui position = uf::vector::decode( json["mouse"]["position"], pod::Vector2ui{} ); {
		//	position.x = json["mouse"]["position"]["x"].as<int>() > 0 ? json["mouse"]["position"]["x"].as<size_t>() : 0;
		//	position.y = json["mouse"]["position"]["y"].as<int>() > 0 ? json["mouse"]["position"]["y"].as<size_t>() : 0;
		}
		pod::Vector2f click; {
			click.x = (float) position.x / (float) ext::gui::size.current.x;
			click.y = (float) position.y / (float) ext::gui::size.current.y;
			click.x = (click.x * 2.0f) - 1.0f;
			click.y = (click.y * 2.0f) - 1.0f;
			float x = click.x;
			float y = click.y;
		}

		{
			auto& scene = uf::scene::getCurrentScene();
			auto& controller = scene.getController();
			auto& metadata = controller.getComponent<ext::GuiManagerBehavior::Metadata>();
			auto& metadataJson = controller.getComponent<uf::Serializer>();
		//	if ( metadata.cursor.type == "mouse" ) metadata.cursor.position = click;		
			if ( metadataJson["overlay"]["cursor"]["type"].as<std::string>() == "mouse" ) {
				metadataJson["overlay"]["cursor"]["position"] = uf::vector::encode( click );
			}
		}
	});
}
void ext::GuiManagerBehavior::tick( uf::Object& self ) {
}
void ext::GuiManagerBehavior::render( uf::Object& self ){
#if UF_USE_VULKAN
	auto& scene = uf::scene::getCurrentScene();
	auto& controller = scene.getController();
	auto& camera = controller.getComponent<uf::Camera>();
	auto& metadataJson = controller.getComponent<uf::Serializer>();

	uf::renderer::RenderTargetRenderMode* renderModePointer = NULL;
	if ( this->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
		renderModePointer = this->getComponentPointer<uf::renderer::RenderTargetRenderMode>();
	} else {
		renderModePointer = (uf::renderer::RenderTargetRenderMode*) &uf::renderer::getRenderMode( "Gui", true );
	}
	auto& renderMode = *renderModePointer;
	auto& blitter = renderMode.blitter;

	if ( !blitter.initialized ) return;
	if ( !blitter.material.hasShader("fragment") ) return;
	
	struct UniformDescriptor {
		struct {
			/*alignas(16)*/ pod::Matrix4f models[2];
		} matrices;
		struct {
			/*alignas(8)*/ pod::Vector2f position = { 0.5f, 0.5f };
			/*alignas(8)*/ pod::Vector2f radius = { 0.1f, 0.1f };
			/*alignas(16)*/ pod::Vector4f color = { 1, 1, 1, 1 };
		} cursor;
	//	/*alignas(8)*/ pod::Vector2f alpha;
	};

	auto& shader = blitter.material.getShader("vertex");
	auto& uniform = shader.getUniform("UBO");
	auto& uniforms = uniform.get<UniformDescriptor>();

	for ( size_t i = 0; i < 2; ++i ) {
		pod::Transform<> transform;
		transform.position = uf::vector::decode( metadataJson["overlay"]["position"], transform.position );
		transform.scale = uf::vector::decode( metadataJson["overlay"]["scale"], transform.scale );
		transform.orientation = uf::vector::decode( metadataJson["overlay"]["orientation"], transform.orientation );
	#if UF_USE_OPENVR
		if ( ext::openvr::enabled && metadataJson["overlay"]["enabled"].as<bool>() ) {
			if ( metadataJson["overlay"]["floating"].as<bool>() ) {
				pod::Matrix4f model = uf::transform::model( transform );
				uniforms.matrices.models[i] = camera.getProjection(i) * ext::openvr::hmdEyePositionMatrix( i == 0 ? vr::Eye_Left : vr::Eye_Right ) * model;
			} else {
				auto translation = uf::matrix::translate( uf::matrix::identity(), camera.getTransform().position + controller.getComponent<pod::Transform<>>().position );
				auto rotation = uf::quaternion::matrix( controller.getComponent<pod::Transform<>>().orientation );

				pod::Matrix4f model = translation * rotation * uf::transform::model( transform );
				uniforms.matrices.models[i] = camera.getProjection(i) * camera.getView(i) * model;
			}
		} else {
	#else
		{
	#endif
			pod::Matrix4t<> model = uf::matrix::translate( uf::matrix::identity(), { 0, 0, 1 } );
			uniforms.matrices.models[i] = model;
		}


		pod::Vector3f cursorSize = { 0, 0 };
		if ( metadataJson["overlay"]["cursor"]["enabled"].as<bool>() ) {
			uniforms.cursor.position.x = (metadataJson["overlay"]["cursor"]["position"][0].as<float>() + 1.0f) * 0.5f; //(::mouse.position.x + 1.0f) * 0.5f;
			uniforms.cursor.position.y = (metadataJson["overlay"]["cursor"]["position"][1].as<float>() + 1.0f) * 0.5f; //(::mouse.position.y + 1.0f) * 0.5f;

			cursorSize.x = metadataJson["overlay"]["cursor"]["radius"].as<float>();
			cursorSize.y = metadataJson["overlay"]["cursor"]["radius"].as<float>();
			
			cursorSize = uf::matrix::multiply<float>( uf::matrix::inverse( uf::matrix::scale( uf::matrix::identity() , transform.scale) ), cursorSize );
		
			uniforms.cursor.radius.x = cursorSize.x;
			uniforms.cursor.radius.y = cursorSize.y;
		}
		
		uniforms.cursor.color = uf::vector::decode( metadataJson["overlay"]["cursor"]["color"], pod::Vector4f{0,0,0,0} );
	}

	shader.updateUniform( "UBO", uniform );
#endif
}
void ext::GuiManagerBehavior::destroy( uf::Object& self ){
	if ( this->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		uf::renderer::removeRenderMode( &renderMode, false );
		this->deleteComponent<uf::renderer::RenderTargetRenderMode>();
	}
}
#undef this