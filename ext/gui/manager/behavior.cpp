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
	.reference = {
		1920,
		1080
	},
};


UF_BEHAVIOR_REGISTER_CPP(ext::GuiManagerBehavior)
UF_BEHAVIOR_REGISTER_AS_OBJECT(ext::GuiManagerBehavior, ext::GuiManager)
#define this (&self)
void ext::GuiManagerBehavior::initialize( uf::Object& self ) {
	{
		// ext::gui::size.current.x = uf::renderer::settings::width;
		// ext::gui::size.current.y = uf::renderer::settings::height;
	}
	// add gui render mode
	if ( !uf::renderer::hasRenderMode( "Gui", true ) ) {
		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		std::string name = "Gui";
		uf::renderer::addRenderMode( &renderMode, name );
		renderMode.blitter.descriptor.subpass = 1;
	}

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Asset& assetLoader = this->getComponent<uf::Asset>();

	this->addHook( "window:Resized", [&](ext::json::Value& json){

		pod::Vector2ui size; {
			size.x = json["window"]["size"]["x"].as<size_t>();
			size.y = json["window"]["size"]["y"].as<size_t>();
		}
		ext::gui::size.current = size;
	//	ext::gui::size.reference = size;
	} );
	this->addHook( "window:Mouse.Moved", [&](ext::json::Value& json){

		bool down = json["mouse"]["state"].as<std::string>() == "Down";
		bool clicked = false;
		pod::Vector2ui position; {
			position.x = json["mouse"]["position"]["x"].as<int>() > 0 ? json["mouse"]["position"]["x"].as<size_t>() : 0;
			position.y = json["mouse"]["position"]["y"].as<int>() > 0 ? json["mouse"]["position"]["y"].as<size_t>() : 0;
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
			auto& metadata = controller.getComponent<uf::Serializer>();

			if ( metadata["overlay"]["cursor"]["type"].as<std::string>() == "mouse" ) {
				metadata["overlay"]["cursor"]["position"][0] = click.x;
				metadata["overlay"]["cursor"]["position"][1] = click.y;
			}
		}
	});
}
void ext::GuiManagerBehavior::tick( uf::Object& self ) {
	
}
void ext::GuiManagerBehavior::render( uf::Object& self ){
	auto& scene = uf::scene::getCurrentScene();
	auto& controller = scene.getController();
	auto& camera = controller.getComponent<uf::Camera>();
	auto& metadata = controller.getComponent<uf::Serializer>();


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
			alignas(16) pod::Matrix4f models[2];
		} matrices;
		struct {
			alignas(8) pod::Vector2f position = { 0.5f, 0.5f };
			alignas(8) pod::Vector2f radius = { 0.1f, 0.1f };
			alignas(16) pod::Vector4f color = { 1, 1, 1, 1 };
		} cursor;
	//	alignas(8) pod::Vector2f alpha;
	};
	auto& shader = blitter.material.shaders.front();
	auto& uniforms = shader.uniforms.front().get<UniformDescriptor>();

	for ( size_t i = 0; i < 2; ++i ) {
		pod::Transform<> transform;
		if ( ext::json::isArray( metadata["overlay"]["position"] ) ) 
			transform.position = {
				metadata["overlay"]["position"][0].as<float>(),
				metadata["overlay"]["position"][1].as<float>(),
				metadata["overlay"]["position"][2].as<float>(),
			};
		if ( ext::json::isArray( metadata["overlay"]["scale"] ) ) 
			transform.scale = {
				metadata["overlay"]["scale"][0].as<float>(),
				metadata["overlay"]["scale"][1].as<float>(),
				metadata["overlay"]["scale"][2].as<float>(),
			};
		if ( ext::json::isArray( metadata["overlay"]["orientation"] ) ) 
			transform.orientation = {
				metadata["overlay"]["orientation"][0].as<float>(),
				metadata["overlay"]["orientation"][1].as<float>(),
				metadata["overlay"]["orientation"][2].as<float>(),
				metadata["overlay"]["orientation"][3].as<float>(),
			};
		if ( ext::openvr::enabled && (metadata["overlay"]["enabled"].as<bool>() || uf::renderer::getRenderMode("Stereoscopic Deferred", true).getType() == "Stereoscopic Deferred" )) {
			if ( metadata["overlay"]["floating"].as<bool>() ) {
				pod::Matrix4f model = uf::transform::model( transform );
				uniforms.matrices.models[i] = camera.getProjection(i) * ext::openvr::hmdEyePositionMatrix( i == 0 ? vr::Eye_Left : vr::Eye_Right ) * model;
			} else {
				auto translation = uf::matrix::translate( uf::matrix::identity(), camera.getTransform().position + controller.getComponent<pod::Transform<>>().position );
				auto rotation = uf::quaternion::matrix( controller.getComponent<pod::Transform<>>().orientation );

				pod::Matrix4f model = translation * rotation * uf::transform::model( transform );
				uniforms.matrices.models[i] = camera.getProjection(i) * camera.getView(i) * model;
			}
		} else {
			pod::Matrix4t<> model = uf::matrix::translate( uf::matrix::identity(), { 0, 0, 1 } );
			uniforms.matrices.models[i] = model;
		}


		pod::Vector3f cursorSize = { 0, 0 };
		if ( metadata["overlay"]["cursor"]["enabled"].as<bool>() ) {
			uniforms.cursor.position.x = (metadata["overlay"]["cursor"]["position"][0].as<float>() + 1.0f) * 0.5f; //(::mouse.position.x + 1.0f) * 0.5f;
			uniforms.cursor.position.y = (metadata["overlay"]["cursor"]["position"][1].as<float>() + 1.0f) * 0.5f; //(::mouse.position.y + 1.0f) * 0.5f;

			cursorSize.x = metadata["overlay"]["cursor"]["radius"].as<float>();
			cursorSize.y = metadata["overlay"]["cursor"]["radius"].as<float>();
			
			cursorSize = uf::matrix::multiply<float>( uf::matrix::inverse( uf::matrix::scale( uf::matrix::identity() , transform.scale) ), cursorSize );
		
			uniforms.cursor.radius.x = cursorSize.x;
			uniforms.cursor.radius.y = cursorSize.y;
		}
		
		uniforms.cursor.color.x = metadata["overlay"]["cursor"]["color"][0].as<float>();
		uniforms.cursor.color.y = metadata["overlay"]["cursor"]["color"][1].as<float>();
		uniforms.cursor.color.z = metadata["overlay"]["cursor"]["color"][2].as<float>();
		uniforms.cursor.color.w = metadata["overlay"]["cursor"]["color"][3].as<float>();
	}
	shader.updateBuffer( (void*) &uniforms, sizeof(uniforms), 0 );
}
void ext::GuiManagerBehavior::destroy( uf::Object& self ){
	if ( this->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		uf::renderer::removeRenderMode( &renderMode, false );
		this->deleteComponent<uf::renderer::RenderTargetRenderMode>();
	}
}
#undef this