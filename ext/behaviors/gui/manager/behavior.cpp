#include "behavior.h"

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

UF_BEHAVIOR_REGISTER_CPP(ext::GuiManagerBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::GuiManagerBehavior, ticks = true, renders = true, multithread = false)
#define this (&self)
void ext::GuiManagerBehavior::initialize( uf::Object& self ) {
	auto& metadata = this->getComponent<ext::GuiManagerBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();

	metadata.size = uf::vector::decode( ext::config["window"]["size"], pod::Vector2f{} );
	this->addHook( "window:Resized", [&](pod::payloads::windowResized& payload){
		metadata.size = payload.window.size;
	} );

	UF_BEHAVIOR_METADATA_BIND_SERIALIZER_HOOKS(metadata, metadataJson);
}
void ext::GuiManagerBehavior::tick( uf::Object& self ) {
	// it would be sugoi to have this handled on init
	// but sometimes a scene that owns the Gui rendermode is deferred, and therefore the new scene doesn't create its Gui rendermode
	// this is the mess it is now because the program /COULD/ instead just have master rendermodes instead of per-scene rendermodes
	// and this oversight seems to only happen when registerRenderMode = false
	auto& metadata = this->getComponent<ext::GuiManagerBehavior::Metadata>();
	if ( !metadata.boundGui && !uf::renderer::hasRenderMode( "Gui", true ) ) {
		UF_MSG_DEBUG("ADDING RENDER MODE");
		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		uf::stl::string name = "Gui";
		renderMode.blitter.descriptor.renderMode = "Swapchain";
		renderMode.blitter.descriptor.subpass = 0;
		renderMode.metadata.type = "single";
		renderMode.metadata.name = name;
		if ( uf::renderer::settings::experimental::registerRenderMode ) uf::renderer::addRenderMode( &renderMode, name );

		metadata.boundGui = true;
	}
}
void ext::GuiManagerBehavior::render( uf::Object& self ){
	auto& renderMode = this->hasComponent<uf::renderer::RenderTargetRenderMode>() ? this->getComponent<uf::renderer::RenderTargetRenderMode>() : uf::renderer::getRenderMode( "Gui", true );

	if ( &renderMode != uf::renderer::getCurrentRenderMode() ) return;

	auto& blitter = renderMode.blitter;

	if ( blitter.material.hasShader("fragment") ) {
		auto& shader = blitter.material.getShader("fragment");

		struct {
			float curTime = 0;
			float gamma = 1.0;
			float exposure = 1.0;
			uint32_t padding = 0;
		} uniforms = {
			.curTime = uf::time::current,
			.gamma = 1,
			.exposure = 1
		};

		shader.updateBuffer( (const void*) &uniforms, sizeof(uniforms), shader.getUniformBuffer("UBO") );
	}
}
void ext::GuiManagerBehavior::destroy( uf::Object& self ){}
void ext::GuiManagerBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ){
	serializer["size"] = uf::vector::encode( /*this->*/size );
}
void ext::GuiManagerBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ){
	/*this->*/size = uf::vector::decode( serializer["size"], /*this->*/size );
}
#undef this