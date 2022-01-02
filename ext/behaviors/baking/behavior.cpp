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
		metadata.output = this->grabURI( metadataJson["baking"]["output"].as<uf::stl::string>(), metadataJson["baking"]["root"].as<uf::stl::string>() );
		metadata.renderModeName = "B:" + std::to_string((int) this->getUid());

		metadata.trigger.mode = metadataJson["baking"]["trigger"]["mode"].as<uf::stl::string>();
		metadata.trigger.value = metadataJson["baking"]["trigger"]["value"].as<uf::stl::string>();

		if ( metadataJson["baking"]["resolution"].is<size_t>() )
			metadata.size = { metadataJson["baking"]["resolution"].as<size_t>(), metadataJson["baking"]["resolution"].as<size_t>() };

		metadata.max.shadows = metadataJson["baking"]["shadows"].as<size_t>(metadata.max.shadows);

		metadata.cull = metadataJson["baking"]["cull"].as<bool>();

		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		uf::renderer::addRenderMode( &renderMode, metadata.renderModeName );

		renderMode.execute = false;
		renderMode.metadata.type = "single";
		renderMode.metadata.pipeline = "baking";
		renderMode.metadata.samples = 1;
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

		scene.process([&]( uf::Entity* entity ) {
			if ( !entity->hasComponent<uf::Graphic>() ) return;
			auto& graphic = entity->getComponent<uf::Graphic>();
			auto& shader = graphic.material.getShader("fragment", "baking");

			for ( auto& t : textures2D ) shader.textures.emplace_back().aliasTexture( t );
			for ( auto& t : texturesCube ) shader.textures.emplace_back().aliasTexture( t );
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

	UF_MSG_DEBUG("Graphic configured, ready to bake");
	return;
}
SAVE: {
#if 1
	renderMode.execute = false;
	UF_MSG_DEBUG("Baking...");
	auto image = renderMode.screenshot();
	uf::stl::string filename = metadata.output;
	bool status = image.save(filename);
	UF_MSG_DEBUG("Writing to " << filename << ": " << status);
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