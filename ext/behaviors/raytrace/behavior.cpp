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

UF_BEHAVIOR_REGISTER_CPP(ext::RayTraceSceneBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::RayTraceSceneBehavior, ticks = true, renders = false, multithread = false)
#define this (&self)
void ext::RayTraceSceneBehavior::initialize( uf::Object& self ) {
	auto& metadata = this->getComponent<ext::RayTraceSceneBehavior::Metadata>();

	if ( !uf::renderer::hasRenderMode("Compute", true) ) {
		auto* renderMode = new uf::renderer::RenderTargetRenderMode;
		renderMode->setTarget("Compute");
		renderMode->metadata.json["shaders"]["vertex"] = "/shaders/display/renderTargetSimple.vert.spv";
		renderMode->metadata.json["shaders"]["fragment"] = "/shaders/display/renderTargetSimple.frag.spv";
		renderMode->blitter.descriptor.subpass = 1;
		renderMode->metadata.type = uf::renderer::settings::pipelines::names::rt;
		renderMode->metadata.pipelines.emplace_back(uf::renderer::settings::pipelines::names::rt);
		renderMode->execute = false;
		uf::renderer::addRenderMode( renderMode, "Compute" );
	}

	if ( ext::config["engine"]["ext"]["vulkan"]["framebuffer"]["size"].is<float>() ) {
		metadata.renderer.scale = ext::config["engine"]["ext"]["vulkan"]["framebuffer"]["size"].as(metadata.renderer.scale);
	}
}
void ext::RayTraceSceneBehavior::tick( uf::Object& self ) {
	auto& metadata = this->getComponent<ext::RayTraceSceneBehavior::Metadata>();

	static uf::stl::vector<pod::Instance> previousInstances;
	uf::stl::vector<pod::Instance> instances = uf::graph::storage.instances.flatten();
	if ( instances.empty() ) return;

	static uf::stl::vector<uf::Graphic*> previousGraphics;
	uf::stl::vector<uf::Graphic*> graphics;
	this->process([&]( uf::Entity* entity ) {
		if ( !entity->hasComponent<uf::Graphic>() ) return;
		auto& graphic = entity->getComponent<uf::Graphic>();
		if ( graphic.accelerationStructures.bottoms.empty() ) return;
		graphics.emplace_back(&graphic);
	});
	if ( graphics.empty() ) return;
	
	bool update = false;
	auto& graphic = this->getComponent<uf::renderer::Graphic>();
	if ( !metadata.renderer.bound ) {
		graphic.initialize("Compute");
		update = true;
	} else {
		if ( previousInstances.size() != instances.size() ) update = true;
		else if ( previousGraphics.size() != graphics.size() ) update = true;
	//	else if ( memcmp( previousInstances.data(), instances.data(), instances.size() * sizeof(decltype(instances)::value_type) ) != 0 ) update = true;
		else if ( memcmp( previousGraphics.data(), graphics.data(), graphics.size() * sizeof(decltype(graphics)::value_type) ) != 0 ) update = true;
		else for ( size_t i = 0; i < instances.size() && !update; ++i ) {
			if ( !uf::matrix::equals( instances[i].model, previousInstances[i].model, 0.0001f ) )
				update = true;
		}
	}
	if ( update ) {
		graphic.generateTopAccelerationStructure( graphics, instances );

		auto& sceneMetadata = this->getComponent<ext::ExtSceneBehavior::Metadata>();
		sceneMetadata.shader.frameAccumulateReset = true;

		previousInstances = instances;
		previousGraphics = graphics;
	}

	if ( !metadata.renderer.bound ) {
		if ( graphic.material.hasShader("ray:gen", uf::renderer::settings::pipelines::names::rt) ) {
			auto& shader = graphic.material.getShader("ray:gen", uf::renderer::settings::pipelines::names::rt);
			//
			auto& image = shader.textures.emplace_back();
			image.fromBuffers(
				NULL, 0,
				uf::renderer::enums::Format::R8G8B8A8_UNORM,
				uf::renderer::settings::width * metadata.renderer.scale, uf::renderer::settings::height * metadata.renderer.scale, 1, 1,
				VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL
			);

			graphic.descriptor.pipeline = uf::renderer::settings::pipelines::names::rt;
			graphic.descriptor.inputs.width = image.width;
			graphic.descriptor.inputs.height = image.height;

			//
			auto& scene = uf::scene::getCurrentScene();
			auto& sceneMetadataJson = scene.getComponent<uf::Serializer>();
			size_t maxLights = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["lights"]["max"].as<size_t>(512);
			size_t maxTextures2D = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["textures"]["max"]["2D"].as<size_t>(512);
			size_t maxTexturesCube = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["textures"]["max"]["cube"].as<size_t>(128);
			size_t maxTextures3D = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["textures"]["max"]["3D"].as<size_t>(1);
			size_t maxCascades = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["vxgi"]["cascades"].as<size_t>(16);

			shader.buffers.emplace_back( uf::graph::storage.buffers.instance.alias() );
			shader.buffers.emplace_back( uf::graph::storage.buffers.material.alias() );
			shader.buffers.emplace_back( uf::graph::storage.buffers.texture.alias() );
			shader.buffers.emplace_back( uf::graph::storage.buffers.light.alias() );

			uint32_t* specializationConstants = (uint32_t*) (void*) shader.specializationConstants;
			for ( auto pair : shader.metadata.definitions.specializationConstants ) {
				auto& sc = pair.second;
				if ( sc.name == "TEXTURES" ) sc.value.ui = (specializationConstants[sc.index] = maxTextures2D);
				else if ( sc.name == "CUBEMAPS" ) sc.value.ui = (specializationConstants[sc.index] = maxTexturesCube);
				else if ( sc.name == "CASCADES" ) sc.value.ui = (specializationConstants[sc.index] = maxCascades);
			}
			for ( auto pair : shader.metadata.definitions.textures ) {
				auto& tx = pair.second;
				for ( auto& layout : shader.descriptorSetLayoutBindings ) {
					if ( layout.binding != tx.binding ) continue;
					if ( tx.name == "samplerTextures" ) layout.descriptorCount = maxTextures2D;
					else if ( tx.name == "samplerCubemaps" ) layout.descriptorCount = maxTexturesCube;
					else if ( tx.name == "voxelId" ) layout.descriptorCount = maxCascades;
					else if ( tx.name == "voxelUv" ) layout.descriptorCount = maxCascades;
					else if ( tx.name == "voxelNormal" ) layout.descriptorCount = maxCascades;
					else if ( tx.name == "voxelRadiance" ) layout.descriptorCount = maxCascades;
				}
			}
			
			metadata.renderer.bound = true;
		}
		if ( graphic.material.hasShader("ray:hit:closest", uf::renderer::settings::pipelines::names::rt) ) {
			auto& shader = graphic.material.getShader("ray:hit:closest", uf::renderer::settings::pipelines::names::rt);
			shader.buffers.emplace_back( uf::graph::storage.buffers.instanceAddresses.alias() );
		}
		if ( graphic.material.hasShader("ray:hit:any", uf::renderer::settings::pipelines::names::rt) ) {
			auto& shader = graphic.material.getShader("ray:hit:any", uf::renderer::settings::pipelines::names::rt);
			shader.buffers.emplace_back( uf::graph::storage.buffers.instanceAddresses.alias() );
		}

		graphic.process = false;
	}
	/* Update lights */ {
		ext::ExtSceneBehavior::bindBuffers( *this, graphic, "ray:gen", uf::renderer::settings::pipelines::names::rt );
	}
	if ( !metadata.renderer.bound ) return;

	auto& shader = graphic.material.getShader("ray:gen", uf::renderer::settings::pipelines::names::rt);
	auto& image = shader.textures.front();
	
	auto& renderMode = uf::renderer::getRenderMode("Compute", true);
	auto& blitter = *renderMode.getBlitter();
	if ( blitter.material.hasShader("fragment") ) {
		auto& shader = blitter.material.getShader("fragment");
		if ( shader.textures.empty() ) {
			auto& tex = shader.textures.emplace_back();
			tex.aliasTexture( image );
			tex.sampler.descriptor.filter.min = VK_FILTER_NEAREST;
			tex.sampler.descriptor.filter.mag = VK_FILTER_NEAREST;

			renderMode.execute = true;
			graphic.process = true;
		}
	}
	
	if ( uf::renderer::states::resized ) {
		image.destroy();
		image.fromBuffers(
			NULL, 0,
			uf::renderer::enums::Format::R8G8B8A8_UNORM,
			uf::renderer::settings::width * metadata.renderer.scale, uf::renderer::settings::height * metadata.renderer.scale, 1, 1,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL
		);

		graphic.descriptor.inputs.width = image.width;
		graphic.descriptor.inputs.height = image.height;
		
		graphic.getPipeline().update( graphic );

		if ( blitter.material.hasShader("fragment") ) {
			auto& shader = blitter.material.getShader("fragment");
			shader.textures.front().aliasTexture( image );
		}
	}

	TIMER(1.0, uf::Window::isKeyPressed("R") && ) {
		UF_MSG_DEBUG("Screenshotting RT scene...");
		image.screenshot().save("./data/rt.png");
	}
}
void ext::RayTraceSceneBehavior::render( uf::Object& self ){}
void ext::RayTraceSceneBehavior::destroy( uf::Object& self ){
	auto& metadata = this->getComponent<ext::RayTraceSceneBehavior::Metadata>();
	auto& graphic = this->getComponent<uf::renderer::Graphic>();

	graphic.destroy();
}
void ext::RayTraceSceneBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ) {}
void ext::RayTraceSceneBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ) {}
#undef this
#endif