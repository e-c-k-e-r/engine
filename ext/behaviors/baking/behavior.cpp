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

#include "../light/behavior.h"
#include "../scene/behavior.h"

#define UF_BAKER_SAVE_MULTITHREAD 1

UF_BEHAVIOR_REGISTER_CPP(ext::BakingBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::BakingBehavior, ticks = true, renders = false, multithread = false)
#define this (&self)
void ext::BakingBehavior::initialize( uf::Object& self ) {
#if UF_USE_VULKAN
	auto& metadataJson = this->getComponent<uf::Serializer>();
	auto& metadata = this->getComponent<ext::BakingBehavior::Metadata>();
	auto& scene = uf::scene::getCurrentScene();
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

	this->addHook( "entity:PostInitialization.%UID%", [&]( ext::json::Value& ){
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
		uf::renderer::addRenderMode( &renderMode, metadata.renderModeName );

		renderMode.execute = false;
		renderMode.metadata.type = "single";
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
		for ( auto& key : uf::graph::storage.texture2Ds.keys ) textures2D.emplace_back().aliasTexture( uf::graph::storage.texture2Ds.map[key] );
		// bind shadow maps
		for ( auto& texture : uf::graph::storage.shadow2Ds ) textures2D.emplace_back().aliasTexture(texture);
		for ( auto& texture : uf::graph::storage.shadowCubes ) texturesCube.emplace_back().aliasTexture(texture);

		metadata.buffers.baked.fromBuffers( NULL, 0, uf::renderer::enums::Format::R8G8B8A8_UNORM, metadata.size.x, metadata.size.y, metadata.max.layers, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );

		scene.process([&]( uf::Entity* entity ) {
			if ( !entity->hasComponent<uf::Graphic>() ) return;
			auto& graphic = entity->getComponent<uf::Graphic>();
			auto& shader = graphic.material.getShader("fragment", "baking");

			for ( auto& t : textures2D ) shader.textures.emplace_back().aliasTexture( t );
			for ( auto& t : texturesCube ) shader.textures.emplace_back().aliasTexture( t );

			shader.textures.emplace_back().aliasTexture( metadata.buffers.baked );
		});
		UF_MSG_DEBUG("Finished initialiation.");
	});

	this->queueHook( "entity:PostInitialization.%UID%", ext::json::null(), 1 );
#endif
}
void ext::BakingBehavior::tick( uf::Object& self ) {
#if 1
#if UF_USE_VULKAN
	if ( !this->hasComponent<uf::renderer::RenderTargetRenderMode>() ) return;
	auto& metadata = this->getComponent<ext::BakingBehavior::Metadata>();
	auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
	if ( renderMode.executed && !metadata.initialized.renderMode ) goto PREPARE;
	else if ( renderMode.executed && !metadata.initialized.map ) {
		TIMER(1.0, (metadata.trigger.mode == "rendered" || (metadata.trigger.mode == "key" && uf::Window::isKeyPressed(metadata.trigger.value))) && ) {
			goto SAVE;
		}
	}
	return;
PREPARE: {
	UF_MSG_DEBUG("Preparing graphics to bake...");

	metadata.initialized.renderMode = true;
	renderMode.execute = true;
	renderMode.setTarget("");
	uf::renderer::states::rebuild = true;

	UF_MSG_DEBUG("Graphic configured, ready to bake " << metadata.max.layers << " lightmaps");
	return;
}
SAVE: {
#if 1
	renderMode.execute = false;
	UF_MSG_DEBUG("Baking...");

	pod::Thread::container_t jobs;

	for ( size_t i = 0; i < metadata.max.layers; ++i ) {
		jobs.emplace_back([&, i]{
		//	auto image = renderMode.screenshot(0, i);
			auto image = metadata.buffers.baked.screenshot(i);
			uf::stl::string filename = uf::string::replace( metadata.output, "%i", std::to_string(i) );
			bool status = image.save(filename);
			UF_MSG_DEBUG("Writing to " << filename << ": " << status);
		});
	}
#if UF_BAKER_SAVE_MULTITHREAD
	if ( !jobs.empty() ) uf::thread::batchWorkers_Async( jobs );
#else
	for ( auto& job : jobs ) job();
#endif
	UF_MSG_DEBUG("Baked.");
	metadata.initialized.map = true;

	auto& scene = uf::scene::getCurrentScene();
	auto& sceneMetadata = scene.getComponent<ext::ExtSceneBehavior::Metadata>();

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
#endif
	return;
}
#endif
#endif
}
void ext::BakingBehavior::render( uf::Object& self ){}
void ext::BakingBehavior::destroy( uf::Object& self ){
	if ( this->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		uf::renderer::removeRenderMode( &renderMode, false );
	//	this->deleteComponent<uf::renderer::RenderTargetRenderMode>();
	}
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