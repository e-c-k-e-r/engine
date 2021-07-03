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

#include <uf/utils/memory/unordered_map.h>
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
UF_BEHAVIOR_TRAITS_CPP(ext::GuiManagerBehavior, ticks = true, renders = false, multithread = false)
UF_BEHAVIOR_REGISTER_AS_OBJECT(ext::GuiManagerBehavior, ext::GuiManager)
#define this (&self)
void ext::GuiManagerBehavior::initialize( uf::Object& self ) {
	// add gui render mode
	if ( !uf::renderer::hasRenderMode( "Gui", true ) ) {
		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		uf::stl::string name = "Gui";
		uf::renderer::addRenderMode( &renderMode, name );
		renderMode.blitter.descriptor.subpass = 1;
	}

	auto& scene = uf::scene::getCurrentScene();	
	auto& controller = scene.getController();
	auto& metadata = controller.getComponent<ext::GuiManagerBehavior::Metadata>();
	auto& metadataJson = controller.getComponent<uf::Serializer>();

	this->addHook( "window:Resized", [&](ext::json::Value& json){
		pod::Vector2ui size = uf::vector::decode( json["window"]["size"], pod::Vector2ui{} );
		ext::gui::size.current = size;
	//	ext::gui::size.reference = size;
	} );
	this->addHook( "window:Mouse.Moved", [&](ext::json::Value& json){
		bool clicked = false;
		bool down = json["mouse"]["state"].as<uf::stl::string>() == "Down";
		pod::Vector2ui position = uf::vector::decode( json["mouse"]["position"], pod::Vector2ui{} );

		pod::Vector2f click; {
			click.x = (float) position.x / (float) ext::gui::size.current.x;
			click.y = (float) position.y / (float) ext::gui::size.current.y;
			click.x = (click.x * 2.0f) - 1.0f;
			click.y = (click.y * 2.0f) - 1.0f;
			float x = click.x;
			float y = click.y;
		}
		if ( metadata.overlay.cursor.type == "mouse" ) {
			metadata.overlay.cursor.position = click;
		}
	});

	metadata.serialize = [&](){
		metadataJson["overlay"]["enabled"] = metadata.overlay.enabled;
		metadataJson["overlay"]["floating"] = metadata.overlay.floating;
		metadataJson["overlay"]["transform"] = uf::transform::encode( metadata.overlay.transform );

		metadataJson["overlay"]["cursor"]["type"] = metadata.overlay.cursor.type;
		metadataJson["overlay"]["cursor"]["enabled"] = metadata.overlay.cursor.enabled;
		metadataJson["overlay"]["cursor"]["position"] = uf::vector::encode( metadata.overlay.cursor.position );
	 	metadataJson["overlay"]["cursor"]["radius"] = metadata.overlay.cursor.radius;
	 	metadataJson["overlay"]["cursor"]["color"] = uf::vector::encode( metadata.overlay.cursor.color );
	};
	metadata.deserialize = [&](){
		metadata.overlay.enabled = metadataJson["overlay"]["enabled"].as( metadata.overlay.enabled );
		metadata.overlay.floating = metadataJson["overlay"]["floating"].as( metadata.overlay.floating );
		metadata.overlay.transform = uf::transform::decode( metadataJson["overlay"]["transform"], metadata.overlay.transform );

		metadata.overlay.cursor.type = metadataJson["overlay"]["cursor"]["type"].as( metadata.overlay.cursor.type );
		metadata.overlay.cursor.enabled = metadataJson["overlay"]["cursor"]["enabled"].as( metadata.overlay.cursor.enabled );
		metadata.overlay.cursor.position = uf::vector::decode( metadataJson["overlay"]["cursor"]["position"], metadata.overlay.cursor.position );
	 	metadata.overlay.cursor.radius = metadataJson["overlay"]["cursor"]["radius"].as( metadata.overlay.cursor.radius );
	 	metadata.overlay.cursor.color = uf::vector::decode( metadataJson["overlay"]["cursor"]["color"], metadata.overlay.cursor.color );
	};
	this->addHook( "object:UpdateMetadata.%UID%", metadata.deserialize);
	metadata.deserialize();
}
void ext::GuiManagerBehavior::tick( uf::Object& self ) {
#if UF_USE_VULKAN
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
	
	auto& scene = uf::scene::getCurrentScene();
	auto& controller = scene.getController();
	auto& camera = controller.getComponent<uf::Camera>();
	auto& metadata = this->getComponent<ext::GuiManagerBehavior::Metadata>();
	auto& metadataJson = controller.getComponent<uf::Serializer>();
#if UF_ENTITY_METADATA_USE_JSON
	metadata.deserialize();
#else
	if ( metadata.overlay.cursor.type == "" && metadata.deserialize ) metadata.deserialize();
#endif
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
/*
	auto& uniform = shader.getUniform("UBO");
	auto& uniforms = uniform.get<UniformDescriptor>();
*/
	UniformDescriptor uniforms;
	for ( size_t i = 0; i < 2; ++i ) {
	#if UF_USE_OPENVR
		if ( ext::openvr::enabled && metadata.overlay.enabled ) {
			if ( metadata.overlay.floating ) {
				pod::Matrix4f model = uf::transform::model( metadata.overlay.transform );
				uniforms.matrices.models[i] = camera.getProjection(i) * ext::openvr::hmdEyePositionMatrix( i == 0 ? vr::Eye_Left : vr::Eye_Right ) * model;
			} else {
				auto translation = uf::matrix::translate( uf::matrix::identity(), camera.getTransform().position + controller.getComponent<pod::Transform<>>().position );
				auto rotation = uf::quaternion::matrix( controller.getComponent<pod::Transform<>>().orientation );

				pod::Matrix4f model = translation * rotation * uf::transform::model( metadata.overlay.transform );
				uniforms.matrices.models[i] = camera.getProjection(i) * camera.getView(i) * model;
			}
		} else {
	#else
		{
	#endif
			pod::Matrix4t<> model = uf::matrix::translate( uf::matrix::identity(), { 0, 0, 1 } );
			uniforms.matrices.models[i] = model;
		}

		if ( metadata.overlay.cursor.enabled ) {
			uniforms.cursor.position = metadata.overlay.cursor.position * 0.5f + 0.5f;
			uniforms.cursor.radius = uf::matrix::multiply<float>( uf::matrix::inverse( uf::matrix::scale( uf::matrix::identity() , metadata.overlay.transform.scale) ), pod::Vector3f{ metadata.overlay.cursor.radius, metadata.overlay.cursor.radius, 0 } );
		}
		uniforms.cursor.color = metadata.overlay.cursor.color;
	}

//	shader.updateUniform( "UBO", uniform );
//	blitter.updateBuffer( (const void*) &uniforms, sizeof(uniforms), shader.getUniformBuffer("UBO") );
	blitter.updateBuffer( uniforms, shader.getUniformBuffer("UBO") );
#endif
}
void ext::GuiManagerBehavior::render( uf::Object& self ){}
void ext::GuiManagerBehavior::destroy( uf::Object& self ){
	if ( this->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		uf::renderer::removeRenderMode( &renderMode, false );
		this->deleteComponent<uf::renderer::RenderTargetRenderMode>();
	}
}
#undef this