#include "behavior.h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/image/atlas.h>
#include <uf/utils/mesh/mesh.h>

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
#include <uf/utils/text/graphic.h>
#include <uf/engine/asset/asset.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/memory/unordered_map.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/ext/openvr/openvr.h>
#include <uf/utils/http/http.h>
#include <uf/utils/audio/audio.h>
#include <uf/utils/window/payloads.h>
#include <uf/utils/string/hash.h>

#include "../payload.h"
#include "../behavior.h"
#include "../manager/behavior.h"

UF_BEHAVIOR_REGISTER_CPP(ext::GuiGlyphBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::GuiGlyphBehavior, ticks = true, renders = false, thread = "")
#define this (&self)

void ext::GuiGlyphBehavior::initialize( uf::Object& self ) {
	auto& metadata = this->getComponent<ext::GuiGlyphBehavior::Metadata>();
	auto& metadataGui = this->getComponent<ext::GuiBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();

	this->addHook( "gui:UpdateText.%UID%", [&](ext::json::Value& payload){
		auto string = payload["string"].as(metadata.string);
		auto font = payload["font"].as(metadata.font);
		bool forced = payload["force"].as(false);

		// override
	#if UF_USE_OPENGL
		metadata.spread = 0;
	#endif
		metadataGui.scaling = "none";

		auto& scene = uf::scene::getCurrentScene();
		auto& mesh = this->getComponent<uf::Mesh>();
		auto& atlas = this->getComponent<uf::Atlas>();
		auto& images = atlas.getImages();

		uf::stl::unordered_map<size_t, uf::stl::string> glyph_atlas_map;

		auto settings = pod::GlyphSettings{
			.alignment = metadataGui.alignment,
			.font = font,
			.size = metadata.size,
			.spread = metadata.spread,
			.padding = metadata.padding,
		};
		auto tokens = uf::glyph::parseTextTokens( string, metadataGui.color );
		auto layout = uf::glyph::calculateLayout( tokens, settings );
		uf::glyph::generateAtlas( layout, settings, atlas );
		uf::glyph::generateMesh( layout, settings, atlas, mesh );

		// set proper shaders
		if ( metadata.spread > 0 ) {
			metadataJson["shaders"]["vertex"] = uf::io::root+"/shaders/gui/text/vert.spv";
			metadataJson["shaders"]["fragment"] = uf::io::root+"/shaders/gui/text/frag.spv";
		}

		// fire image update
		{
			ext::payloads::GuiInitializationPayload payload;
			payload.image = (uf::Image*) &atlas.getAtlas();
			payload.mesh = &mesh;
			payload.free = false;
			this->callHook( "gui:Update.%UID%", payload );
		}

	});

	UF_BEHAVIOR_METADATA_BIND_SERIALIZER_HOOKS(metadata, metadataJson);
}
void ext::GuiGlyphBehavior::tick( uf::Object& self ) {
	if ( !this->hasComponent<uf::Graphic>() ) return;

#if !UF_USE_OPENGL

	auto& transform = this->getComponent<pod::Transform<>>();
	auto& metadata = this->getComponent<ext::GuiGlyphBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();
	auto& mesh = this->getComponent<uf::Mesh>();
	auto& graphic = this->getComponent<uf::Graphic>();
	auto model = uf::matrix::identity();
	
	auto& scene = uf::scene::getCurrentScene();
	auto& controller = scene.getController();
	auto& camera = controller.getComponent<uf::Camera>();

	// bind UBO
	if ( graphic.material.hasShader("vertex") ) {
		auto& shader = graphic.material.getShader("vertex");
		if ( shader.hasUniform("UBO_Glyph") ) {
			auto& uniformBuffer = shader.getUniformBuffer("UBO_Glyph");
			struct Glyph {
				/*alignas(16)*/ pod::Vector4f stroke;

				/*alignas(8)*/ pod::Vector2i range;
				/*alignas(4)*/ int32_t spread;
				/*alignas(4)*/ float weight;

				/*alignas(4)*/ float fillWeight;
				/*alignas(4)*/ float scale;
				/*alignas(4)*/ float padding1;
				/*alignas(4)*/ float padding2;
			} ubo = {
				.stroke = metadata.shader.stroke,
				.range = metadata.shader.range,
				
				.spread = metadata.spread,
				.weight = metadata.shader.weight,
				.fillWeight = metadata.shader.fillWeight,
				.scale = metadata.shader.scale,
			};

			shader.updateBuffer( (const void*) &ubo, sizeof(ubo), uniformBuffer );
		}
	}
#endif
}
void ext::GuiGlyphBehavior::render( uf::Object& self ){}
void ext::GuiGlyphBehavior::destroy( uf::Object& self ){}

#undef this

void ext::GuiGlyphBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ){
	serializer["string"] = /*this->*/string;
	serializer["font"] = /*this->*/font;
	serializer["spread"] = /*this->*/spread;
	serializer["padding"] = uf::vector::encode( /*this->*/padding);

	serializer["scale"] = /*this->*/shader.scale;
	serializer["weight"] = /*this->*/shader.weight;
	serializer["fillWeight"] = /*this->*/shader.fillWeight;
	serializer["stroke"] = uf::vector::encode( /*this->*/shader.stroke);
	serializer["range"] = uf::vector::encode( /*this->*/shader.range);
}
void ext::GuiGlyphBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ){
	size_t oldHash = uf::glyph::hashSettings( string, pod::GlyphSettings{
		.alignment = "",
		.font = /*this->*/font,
		.size = /*this->*/size,
		.spread = /*this->*/spread,
		.padding = /*this->*/padding,
	});

	/*this->*/string = serializer["string"].as(/*this->*/string);
	/*this->*/font = serializer["font"].as(/*this->*/font);
	/*this->*/spread = serializer["spread"].as(/*this->*/spread);
	/*this->*/padding = uf::vector::decode(serializer["padding"], /*this->*/padding);
	
	/*this->*/shader.scale = serializer["scale"].as(/*this->*/shader.scale);
	/*this->*/shader.weight = serializer["weight"].as(/*this->*/shader.weight);
	/*this->*/shader.fillWeight = serializer["fillWeight"].as(/*this->*/shader.fillWeight);
	/*this->*/shader.stroke = uf::vector::decode(serializer["stroke"], /*this->*/shader.stroke);
	/*this->*/shader.range = uf::vector::decode(serializer["range"], /*this->*/shader.range);

	size_t newHash = uf::glyph::hashSettings( string, pod::GlyphSettings{
		.alignment = "",
		.font = /*this->*/font,
		.size = /*this->*/size,
		.spread = /*this->*/spread,
		.padding = /*this->*/padding,
	});
	
	// fire text update
	if ( oldHash != newHash ) {
		ext::json::Value payload;
		payload["string"] = /*this->*/string;
		self.callHook("gui:UpdateText.%UID%", payload);
	}
}