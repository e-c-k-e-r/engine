#include <uf/config.h>
#if UF_USE_VULKAN
#include "behavior.h"

#include <uf/utils/renderer/renderer.h>

#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/camera/camera.h>
#include <uf/ext/gltf/gltf.h>
#include <uf/engine/asset/asset.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/ext/xatlas/xatlas.h>
#include <uf/ext/texconv/texconv.h>

#include "../light/behavior.h"
#include "../scene/behavior.h"

#define UF_BAKER_SAVE_MULTITHREAD 0 // really slow

namespace {
	bool accumulated = false;
	uint8_t frames = 5;
}

UF_BEHAVIOR_REGISTER_CPP(ext::BakingBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::BakingBehavior, ticks = true, renders = false, thread = "")
#define this (&self)
void ext::BakingBehavior::initialize( uf::Object& self ) {
#if UF_USE_VULKAN
	auto& metadataJson = this->getComponent<uf::Serializer>();
	auto& metadata = this->getComponent<ext::BakingBehavior::Metadata>();
	auto& scene = uf::scene::getCurrentScene();
	auto& storage = uf::graph::getStorage( scene );
	auto& sceneMetadata = scene.getComponent<ext::ExtSceneBehavior::Metadata>();
	auto& controller = scene.getController();
	auto& controllerTransform = controller.getComponent<pod::Transform<>>();

	metadata.previous.lights = sceneMetadata.light.max;
	metadata.previous.shadows = sceneMetadata.shadow.max;
	metadata.previous.update = sceneMetadata.shadow.update;

	sceneMetadata.light.max = metadata.max.shadows;
	sceneMetadata.shadow.max = metadata.max.shadows;
	sceneMetadata.shadow.update = metadata.max.shadows;
	UF_MSG_DEBUG("Temporarily altering shadow limits...");

	{
		metadata.uniforms.lights = MIN(sceneMetadata.light.max, metadata.max.shadows);
		metadata.uniforms.gamma = sceneMetadata.light.gamma;
		metadata.uniforms.exposure = sceneMetadata.light.exposure;
		metadata.buffers.uniforms.initialize( (const void*) &metadata.uniforms, sizeof(metadata.uniforms), uf::renderer::enums::Buffer::UNIFORM );
	}

	this->addHook( "entity:PostInitialization.%UID%", [&](){
		metadata.output = this->resolveURI( metadataJson["baking"]["output"].as<uf::stl::string>(), metadataJson["baking"]["root"].as<uf::stl::string>() );
		metadata.renderModeName = "B:" + std::to_string((int) this->getUid());

		metadata.trigger.mode = metadataJson["baking"]["trigger"]["mode"].as( metadata.trigger.mode );
		metadata.trigger.value = metadataJson["baking"]["trigger"]["value"].as( metadata.trigger.value );
		metadata.trigger.quits = metadataJson["baking"]["trigger"]["quits"].as( metadata.trigger.quits );

		if ( metadataJson["baking"]["resolution"].is<size_t>() )
			metadata.size = { metadataJson["baking"]["resolution"].as<size_t>(), metadataJson["baking"]["resolution"].as<size_t>() };

		metadata.max.shadows = metadataJson["baking"]["shadows"].as<size_t>(metadata.max.shadows);
		metadata.max.layers = std::max( metadataJson["baking"]["layers"].as<size_t>(metadata.max.layers), (size_t) 1 );

		metadata.cull = metadataJson["baking"]["cull"].as<bool>();

		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();

		renderMode.execute = false;
		renderMode.metadata.type = "depth"; // "single";
		renderMode.metadata.pipeline = "baking";
		renderMode.metadata.samples = 1;
	//	renderMode.metadata.views = metadata.max.layers; // gl_Layer doesn't work
		renderMode.metadata.json["descriptor"]["cull mode"] = "none";

		renderMode.width = metadata.size.x;
		renderMode.height = metadata.size.y;
		renderMode.blitter.process = false;
		
		UF_MSG_DEBUG("Binding...");

		uf::stl::vector<uf::renderer::Texture2D> textures2D;
		uf::stl::vector<uf::renderer::TextureCube> texturesCube;
		// bind scene textures
		for ( auto& key : storage.images.keys ) textures2D.emplace_back().aliasTexture( storage.images.map[key].handle );
		// bind shadow maps
		for ( auto& texture : storage.shadow2Ds ) textures2D.emplace_back().aliasTexture(texture);
		for ( auto& texture : storage.shadowCubes ) texturesCube.emplace_back().aliasTexture(texture);

	//	::totalIDs = storage.primitives.keys.size();

		metadata.buffers.baked.mips = 0;
		metadata.buffers.baked.fromBuffers( NULL, 0, uf::renderer::enums::Format::R8G8B8A8_UNORM, metadata.size.x, metadata.size.y, metadata.max.layers, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_IMAGE_LAYOUT_GENERAL );

		scene.process([&]( uf::Entity* entity ) {
			if ( !entity->hasComponent<uf::Graphic>() ) return;
			auto& graphic = entity->getComponent<uf::Graphic>();
			if ( !graphic.material.hasShader("fragment", "baking") ) return;

			auto& shader = graphic.material.getShader("fragment", "baking");

			for ( auto& t : textures2D ) shader.textures.emplace_back().aliasTexture( t );
			for ( auto& t : texturesCube ) shader.textures.emplace_back().aliasTexture( t );

			shader.textures.emplace_back().aliasTexture( metadata.buffers.baked );

			shader.buffers.insert( shader.buffers.begin(), metadata.buffers.uniforms.alias() );
		});
		renderMode.metadata.name = metadata.renderModeName;
		if ( uf::renderer::settings::experimental::registerRenderMode ) uf::renderer::addRenderMode( &renderMode, metadata.renderModeName );
		uf::renderer::states::rebuild = true;
		UF_MSG_DEBUG("Finished initialiation.");
	});

	this->queueHook( "entity:PostInitialization.%UID%", 2.0f );
#endif
}
void ext::BakingBehavior::tick( uf::Object& self ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& sceneMetadata = scene.getComponent<ext::ExtSceneBehavior::Metadata>();
	
#if UF_USE_VULKAN
	if ( !this->hasComponent<uf::renderer::RenderTargetRenderMode>() ) return;
	auto& metadata = this->getComponent<ext::BakingBehavior::Metadata>();
	auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
	
	vkDeviceWaitIdle( ext::vulkan::device );
	if ( renderMode.executed && !metadata.initialized.renderMode ) goto PREPARE;
	else if ( renderMode.executed && !metadata.initialized.map ) {
		sceneMetadata.shadow.update = metadata.previous.update;
		if ( --::frames == 0 && !accumulated ) {
			accumulated = true;
			renderMode.execute = false;
			UF_MSG_DEBUG("Finished accumulating lightmaps");
		}
		TIMER(1.0, accumulated && (metadata.trigger.mode == "rendered" || (metadata.trigger.mode == "key" && uf::Window::isKeyPressed(metadata.trigger.value))) ) {
			goto SAVE;
		}
	}
	return;
PREPARE: {
	UF_MSG_DEBUG("Preparing graphics to bake...");

	uf::renderer::settings::defaultCommandBufferImmediate = true;
	metadata.initialized.renderMode = true;
	renderMode.execute = true;
	renderMode.setTarget("");
	uf::renderer::states::rebuild = true;

	UF_MSG_DEBUG("Graphic configured, ready to bake {} lightmaps", metadata.max.layers);
	return;
}
SAVE: {
	renderMode.execute = false;
	UF_MSG_DEBUG("Baking...");


#if UF_BAKER_SAVE_MULTITHREAD
	auto tasks = uf::thread::schedule(true);
#else
	auto tasks = uf::thread::schedule(false);
#endif
	// nothing should render to 0
	for ( size_t i = 0; i < metadata.max.layers; ++i ) {
		tasks.queue([&, i]{
			auto image = metadata.buffers.baked.screenshot(i);
			uf::stl::string filename = uf::string::replace( metadata.output, "%i", std::to_string(i) );
			bool status = image.save(filename);
			UF_MSG_DEBUG("Writing to {}: {}", filename, status);

		// export DC's .dtex
		#if UF_USE_DC_TEXCONV
			// convert RGBE to RGBA
			auto* pixels = (pod::Vector4ub*) image.getPixels().data();
			for ( auto p = 0; p < metadata.size.x * metadata.size.y; ++p ) {
				auto& pixel = pixels[p];
				if ( pixel.w == 0 ) {
					pixel = {0,0,0,255};
					continue;
				}

				// decode
				float exp = (float) pixel.w - 128.0f;
				float mult = std::exp2(exp);

				const float gamma = 1.0f / 2.2f;
				auto linear = pod::Vector3f{ pixel.x, pixel.y, pixel.z } * mult / 255.0f;
				// tone-map
				FOR_EACH( 3, {
					linear[i] = linear[i] / ( 1 + linear[i] );
				});
				// gamma correct
				linear = uf::vector::pow( uf::vector::clamp( linear, 0.0f, 1.0f ), gamma );
				// 0-1 => 0-255
				linear *= 255.0f;
				pixel = { (uint8_t)(linear.x), (uint8_t)(linear.y), (uint8_t)(linear.z), 255 };
			}
			// downscale with bilinear interpolation
			auto converted = image.scale( {128, 128} );
			auto dtex = ext::texconv::convert( converted, "RGB565" );
			ext::texconv::save( dtex, uf::string::replace( filename, ".png", "" ), false );
		#endif
		});
	}
	uf::thread::execute( tasks );
	
	UF_MSG_DEBUG("Baked.");
//	ext::vulkan::states::frameSkip = false;
	metadata.initialized.map = true;

	sceneMetadata.light.max = metadata.previous.lights;
	sceneMetadata.shadow.max = metadata.previous.shadows;
	sceneMetadata.shadow.update = metadata.previous.update;

	UF_MSG_DEBUG("Reverted shadow limits");

	ext::json::Value payload;
	payload["uid"] = this->getUid();
	uf::scene::getCurrentScene().queueHook("system:Destroy", payload);

	if ( metadata.trigger.quits ) {
		payload["message"] = "Termination after lightmap baking requested.";
		uf::scene::getCurrentScene().queueHook("system:Quit", payload);
	}
	return;
}
#endif
}
void ext::BakingBehavior::render( uf::Object& self ){}
void ext::BakingBehavior::destroy( uf::Object& self ){
/*
	if ( this->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		uf::renderer::removeRenderMode( &renderMode, false );
	//	this->deleteComponent<uf::renderer::RenderTargetRenderMode>();
	}
*/
#if 0
	if ( this->hasComponent<pod::Graph>() ) {
		auto& graph = this->getComponent<pod::Graph>();
		uf::graph::destroy( graph );
	}
#endif
}
void ext::BakingBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ) {}
void ext::BakingBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ) {}
#undef this
#endif