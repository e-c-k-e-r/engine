#include "behavior.h"
#include "../gui.h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/window/window.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/math/physics.h>

#include <uf/utils/text/glyph.h>
#include <uf/engine/asset/asset.h>
#include <uf/engine/scene/scene.h>

#include <uf/utils/memory/unordered_map.h>

#include <uf/utils/renderer/renderer.h>
#include <uf/ext/openvr/openvr.h>

#include <uf/utils/http/http.h>
#include <uf/utils/audio/audio.h>

#include <uf/utils/window/payloads.h>

#include <sys/stat.h>
#include <fstream>
#include <regex>
#include <locale>
#include <codecvt>

ext::gui::Size ext::gui::size = {
	.current = {
		0,
		0,
	},
#if UF_ENV_DREAMCAST
	.reference = { 640, 480 },
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
		renderMode.blitter.descriptor.subpass = 0;
		renderMode.metadata.type = "single";
		uf::renderer::addRenderMode( &renderMode, name );
	}

	auto& scene = uf::scene::getCurrentScene();	
	auto& controller = scene.getController();
	auto& metadata = controller.getComponent<ext::GuiManagerBehavior::Metadata>();
	auto& metadataJson = controller.getComponent<uf::Serializer>();

	ext::gui::size.current = uf::vector::decode( ext::config["window"]["size"], pod::Vector2f{} );
	this->addHook( "window:Resized", [&](pod::payloads::windowResized& payload){
		ext::gui::size.current = payload.window.size;
		ext::gui::size.reference = payload.window.size;
	} );
	this->addHook( "window:Mouse.Moved", [&](pod::payloads::windowMouseMoved& payload){
		bool clicked = false;

		pod::Vector2f click;
		click.x = (float) payload.mouse.position.x / (float) ext::gui::size.current.x;
		click.y = (float) payload.mouse.position.y / (float) ext::gui::size.current.y;
		click.x = (click.x * 2.0f) - 1.0f;
		click.y = (click.y * 2.0f) - 1.0f;
		float x = click.x;
		float y = click.y;
		if ( metadata.overlay.cursor.type == "mouse" ) {
			metadata.overlay.cursor.position = click;
		}
	});

	UF_BEHAVIOR_METADATA_BIND_SERIALIZER_HOOKS(metadata, metadataJson);
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

	return;
	
	auto& scene = uf::scene::getCurrentScene();
	auto& controller = scene.getController();
	auto& camera = controller.getComponent<uf::Camera>();
	auto& metadata = this->getComponent<ext::GuiManagerBehavior::Metadata>();
	auto& metadataJson = controller.getComponent<uf::Serializer>();
#if UF_ENTITY_METADATA_USE_JSON
	metadata.deserialize();
#else
//	if ( metadata.overlay.cursor.type == "" ) metadata.deserialize( self, metadataJson );
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
#if UF_UNIFORMS_REUSE
	auto& uniform = shader.getUniform("UBO");
	auto& uniforms = uniform.get<UniformDescriptor>();
#else
	UniformDescriptor uniforms;
#endif
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
#if UF_UNIFORMS_REUSE
	shader.updateUniform( "UBO", uniform );
#else
	shader.updateBuffer( (const void*) &uniforms, sizeof(uniforms), shader.getUniformBuffer("UBO") );
#endif
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
void ext::GuiManagerBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ){
	serializer["overlay"]["enabled"] = /*this->*/overlay.enabled;
	serializer["overlay"]["floating"] = /*this->*/overlay.floating;
	serializer["overlay"]["transform"] = uf::transform::encode( /*this->*/overlay.transform );

	serializer["overlay"]["cursor"]["type"] = /*this->*/overlay.cursor.type;
	serializer["overlay"]["cursor"]["enabled"] = /*this->*/overlay.cursor.enabled;
	serializer["overlay"]["cursor"]["position"] = uf::vector::encode( /*this->*/overlay.cursor.position );
 	serializer["overlay"]["cursor"]["radius"] = /*this->*/overlay.cursor.radius;
 	serializer["overlay"]["cursor"]["color"] = uf::vector::encode( /*this->*/overlay.cursor.color );
}
void ext::GuiManagerBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ){
	/*this->*/overlay.enabled = serializer["overlay"]["enabled"].as( /*this->*/overlay.enabled );
	/*this->*/overlay.floating = serializer["overlay"]["floating"].as( /*this->*/overlay.floating );
	/*this->*/overlay.transform = uf::transform::decode( serializer["overlay"]["transform"], /*this->*/overlay.transform );

	/*this->*/overlay.cursor.type = serializer["overlay"]["cursor"]["type"].as( /*this->*/overlay.cursor.type );
	/*this->*/overlay.cursor.enabled = serializer["overlay"]["cursor"]["enabled"].as( /*this->*/overlay.cursor.enabled );
	/*this->*/overlay.cursor.position = uf::vector::decode( serializer["overlay"]["cursor"]["position"], /*this->*/overlay.cursor.position );
 	/*this->*/overlay.cursor.radius = serializer["overlay"]["cursor"]["radius"].as( /*this->*/overlay.cursor.radius );
 	/*this->*/overlay.cursor.color = uf::vector::decode( serializer["overlay"]["cursor"]["color"], /*this->*/overlay.cursor.color );
}
#undef this