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

namespace {
	uf::renderer::Texture emptyTexture;
}

#define UF_USE_EXTERNAL_IMAGE 1

UF_BEHAVIOR_REGISTER_CPP(ext::RayTraceSceneBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::RayTraceSceneBehavior, ticks = true, renders = false, multithread = true)
#define this (&self)
void ext::RayTraceSceneBehavior::initialize( uf::Object& self ) {
	auto& metadata = this->getComponent<ext::RayTraceSceneBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();
	UF_BEHAVIOR_METADATA_BIND_SERIALIZER_HOOKS(metadata, metadataJson);

	if ( metadata.renderer.full ) {
		if ( !uf::renderer::hasRenderMode("Compute:RT", true) ) {
			auto* renderMode = new uf::renderer::RenderTargetRenderMode;
			renderMode->setTarget("Compute:RT");

			{
				uf::stl::string vertexShaderFilename = "/shaders/display/renderTarget/vert.spv";
				uf::stl::string fragmentShaderFilename = "/shaders/display/renderTarget/frag.spv";
				
				std::pair<bool, uf::stl::string> settings[] = {
					{ uf::renderer::settings::pipelines::postProcess, "postProcess.frag" },
				//	{ msaa > 1, "msaa.frag" },
				};
				FOR_ARRAY( settings ) if ( settings[i].first ) fragmentShaderFilename = uf::string::replace( fragmentShaderFilename, "frag", settings[i].second );

				renderMode->metadata.json["shaders"]["vertex"] = vertexShaderFilename;
				renderMode->metadata.json["shaders"]["fragment"] = fragmentShaderFilename;
			}
			
			renderMode->blitter.descriptor.renderMode = "Swapchain";
			renderMode->blitter.descriptor.subpass = 0;

			renderMode->metadata.type = uf::renderer::settings::pipelines::names::rt;
			renderMode->metadata.pipelines.emplace_back(uf::renderer::settings::pipelines::names::rt);
			renderMode->execute = false;
			renderMode->metadata.limiter.execute = false;
		//	renderMode->blitter.process = false;
			uf::renderer::addRenderMode( renderMode, "Compute:RT" );
		}
	#if UF_USE_EXTERNAL_IMAGE
		{
			uf::stl::vector<uint8_t> pixels = { 
				0, 0, 0, 0, 0, 0, 0, 0,
				0, 0, 0, 0, 0, 0, 0, 0,
			};
			::emptyTexture.sampler.descriptor.filter.min = VK_FILTER_NEAREST;
			::emptyTexture.sampler.descriptor.filter.mag = VK_FILTER_NEAREST;
			::emptyTexture.fromBuffers(
				(void*) &pixels[0],
				pixels.size(),
				uf::renderer::enums::Format::R8G8B8A8_UNORM,
				2, 2,
				uf::renderer::device,
				VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL
			);

			auto& renderMode = uf::renderer::getRenderMode("Compute:RT", true);
			auto& blitter = *renderMode.getBlitter();
			blitter.material.textures.emplace_back().aliasTexture( ::emptyTexture );
		}
	#endif
	}
}
void ext::RayTraceSceneBehavior::tick( uf::Object& self ) {
	auto& metadata = this->getComponent<ext::RayTraceSceneBehavior::Metadata>();

	static uf::stl::vector<pod::Instance> previousInstances;
	uf::stl::vector<pod::Instance> instances = uf::graph::storage.instances.flatten();
	if ( instances.empty() ) return;

	static uf::stl::vector<uf::Graphic*> previousGraphics;
	uf::stl::vector<uf::Graphic*> graphics;
	auto& scene = uf::scene::getCurrentScene();
	auto& graph = scene.getGraph();
	for ( auto entity : graph ) {
		if ( !entity->hasComponent<uf::Graphic>() ) continue;
		auto& graphic = entity->getComponent<uf::Graphic>();
		if ( graphic.accelerationStructures.bottoms.empty() ) continue;
		graphics.emplace_back(&graphic);
	}
	if ( graphics.empty() ) return;
	
	bool update = false;
	auto& renderMode = uf::renderer::getRenderMode("", true);
	auto& graphic = metadata.renderer.full ? this->getComponent<uf::renderer::Graphic>() : *renderMode.getBlitter();
	if ( !metadata.renderer.bound ) {
		if ( metadata.renderer.full ) {
			graphic.initialize("Compute:RT");
		}
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
		uf::renderer::states::frameAccumulateReset = true;

		previousInstances = instances;
		previousGraphics = graphics;
	}

	if ( !metadata.renderer.bound ) {
		if ( graphic.material.hasShader("ray:gen", uf::renderer::settings::pipelines::names::rt) ) {
			metadata.renderer.bound = true;
			if ( metadata.renderer.full ) {
				graphic.process = false;
				auto& shader = graphic.material.getShader("ray:gen", uf::renderer::settings::pipelines::names::rt);
			#if !UF_USE_EXTERNAL_IMAGE
				auto& renderMode = uf::renderer::getRenderMode("Compute:RT", true);
				shader.aliasAttachment("color", &renderMode, VK_IMAGE_LAYOUT_GENERAL);
				shader.aliasAttachment("bright", &renderMode, VK_IMAGE_LAYOUT_GENERAL);
				shader.aliasAttachment("motion", &renderMode, VK_IMAGE_LAYOUT_GENERAL);

				graphic.descriptor.bind.width = renderMode.width > 0 ? renderMode.width : uf::renderer::settings::width;
				graphic.descriptor.bind.height = renderMode.height > 0 ? renderMode.height : uf::renderer::settings::height;
				graphic.descriptor.bind.point = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
				graphic.descriptor.pipeline = uf::renderer::settings::pipelines::names::rt;
			#else
				//
				pod::Vector2ui size = metadata.renderer.size;
				if ( size.x == 0 ) size.x = uf::renderer::settings::width;
				if ( size.y == 0 ) size.y = uf::renderer::settings::height;

				auto& image = shader.textures.emplace_back();
				image.fromBuffers(
					NULL, 0,
					uf::renderer::settings::pipelines::hdr ? uf::renderer::enums::Format::HDR : uf::renderer::enums::Format::SDR,
					size.x, size.y, 1, 1,
					VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL
				);

				graphic.descriptor.pipeline = uf::renderer::settings::pipelines::names::rt;
				graphic.descriptor.bind.width = image.width;
				graphic.descriptor.bind.height = image.height;
				graphic.descriptor.bind.point = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
			#endif

				//
				auto& scene = uf::scene::getCurrentScene();
				auto& sceneMetadataJson = scene.getComponent<uf::Serializer>();
				size_t maxLights = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["lights"]["max"].as<size_t>(512);
				size_t maxTextures2D = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["textures"]["max"]["2D"].as<size_t>(512);
				size_t maxTexturesCube = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["textures"]["max"]["cube"].as<size_t>(128);
				size_t maxTextures3D = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["textures"]["max"]["3D"].as<size_t>(1);
				size_t maxCascades = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["vxgi"]["cascades"].as<size_t>(16);

				shader.buffers.emplace_back( uf::graph::storage.buffers.instance.alias() );
				shader.buffers.emplace_back( uf::graph::storage.buffers.instanceAddresses.alias() );
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

			}
		}
	}
	if ( metadata.renderer.full ) {
		/* Update lights */ {
			ext::ExtSceneBehavior::bindBuffers( *this, graphic, "ray:gen", uf::renderer::settings::pipelines::names::rt );
		}
		if ( !metadata.renderer.bound ) return;
	#if !UF_USE_EXTERNAL_IMAGE
		if ( uf::renderer::states::resized ) {
			auto& renderMode = uf::renderer::getRenderMode("Compute:RT", true);
			graphic.descriptor.bind.width = renderMode.width > 0 ? renderMode.width : uf::renderer::settings::width;
			graphic.descriptor.bind.height = renderMode.height > 0 ? renderMode.height : uf::renderer::settings::height;
			graphic.descriptor.bind.point = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
			graphic.descriptor.pipeline = uf::renderer::settings::pipelines::names::rt;
			graphic.getPipeline().update( graphic );
		}
	#else
		{
			auto& shader = graphic.material.getShader("ray:gen", uf::renderer::settings::pipelines::names::rt);
			auto& image = shader.textures.front();
			
			auto& renderMode = uf::renderer::getRenderMode("Compute:RT", true);
			auto& blitter = *renderMode.getBlitter();

			if ( blitter.material.hasShader("fragment") ) {
				auto& shader = blitter.material.getShader("fragment");
				if ( shader.textures.empty() || ( !shader.textures.empty() && shader.textures.front().image == ::emptyTexture.image ) ) {
					shader.textures.clear();
					auto& tex = shader.textures.emplace_back();
					tex.aliasTexture( image );

					tex.sampler.descriptor.filter.min = metadata.renderer.filter;
					tex.sampler.descriptor.filter.mag = metadata.renderer.filter;

					renderMode.execute = true;
					renderMode.metadata.limiter.execute = true;
					blitter.process = true;
					graphic.process = true;
				}
			}
			
			if ( uf::renderer::states::resized ) {
				pod::Vector2ui size = metadata.renderer.size;
				if ( size.x == 0 ) size.x = uf::renderer::settings::width;
				if ( size.y == 0 ) size.y = uf::renderer::settings::height;
				image.destroy();
				image.fromBuffers(
					NULL, 0,
					uf::renderer::settings::pipelines::hdr ? uf::renderer::enums::Format::HDR : uf::renderer::enums::Format::SDR,
					size.x, size.y, 1, 1,
					VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL
				);

				graphic.descriptor.bind.width = image.width;
				graphic.descriptor.bind.height = image.height;
				graphic.descriptor.bind.point = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
				
				graphic.getPipeline().update( graphic );

				if ( blitter.material.hasShader("fragment") ) {
					auto& shader = blitter.material.getShader("fragment");
					shader.textures.front().aliasTexture( image );
				}
			}
		}
	#endif
	#if 0
		TIMER(1.0, uf::Window::isKeyPressed("R") ) {
			UF_MSG_DEBUG("Screenshotting RT scene...");
			image.screenshot().save("./data/rt.png");
		}
	#endif
	}
}
void ext::RayTraceSceneBehavior::render( uf::Object& self ){}
void ext::RayTraceSceneBehavior::destroy( uf::Object& self ){
	auto& metadata = this->getComponent<ext::RayTraceSceneBehavior::Metadata>();

	if ( !metadata.renderer.full ) return;
	auto& graphic = this->getComponent<uf::renderer::Graphic>();
	graphic.destroy();
}
void ext::RayTraceSceneBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ) {

}
void ext::RayTraceSceneBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ) {
	// merge vxgi settings with global settings
	{
		const auto& globalSettings = ext::config["engine"]["scenes"]["rt"];
		ext::json::forEach( globalSettings, [&]( const uf::stl::string& key, const ext::json::Value& value ){
			if ( !ext::json::isNull( serializer["rt"][key] ) ) return;
			serializer["rt"][key] = value;
		} );
	}

	if ( serializer["rt"]["filter"].is<uf::stl::string>() ) {
		const auto mode = uf::string::lowercase( serializer["rt"]["filter"].as<uf::stl::string>() );
		if ( mode == "nearest" ) /*this->*/renderer.filter = uf::renderer::enums::Filter::NEAREST;
		else if ( mode == "linear" ) /*this->*/renderer.filter = uf::renderer::enums::Filter::LINEAR;
		else UF_MSG_WARNING("Invalid Filter enum string specified: {}", mode);
	}

	if ( serializer["rt"]["size"].is<float>() ) {
		/*this->*/renderer.scale = serializer["rt"]["size"].as(/*this->*/renderer.scale);
	} else if ( ext::json::isArray( serializer["rt"]["size"] ) ) {
		/*this->*/renderer.size = uf::vector::decode( serializer["rt"]["size"], /*this->*/renderer.size );
	} else if ( ext::config["engine"]["ext"]["vulkan"]["framebuffer"]["size"].is<float>() ) {
		/*this->*/renderer.scale = ext::config["engine"]["ext"]["vulkan"]["framebuffer"]["size"].as(/*this->*/renderer.scale);
	}
	
	/*this->*/renderer.full =  serializer["rt"]["full"].as(/*this->*/renderer.full);

	/*this->*/settings.defaultRayBounds = uf::vector::decode(serializer["rt"]["defaultRayBounds"], settings.defaultRayBounds); // serializer["rt"]["defaultRayBounds"].as(/*this->*/settings.defaultRayBounds);
	/*this->*/settings.alphaTestOffset = serializer["rt"]["alphaTestOffset"].as(/*this->*/settings.alphaTestOffset);
	/*this->*/settings.samples = serializer["rt"]["samples"].as(/*this->*/settings.samples);
	/*this->*/settings.paths = serializer["rt"]["paths"].as(/*this->*/settings.paths);
	/*this->*/settings.frameAccumulationMinimum = serializer["rt"]["frameAccumulationMinimum"].as(/*this->*/settings.frameAccumulationMinimum);
}
#undef this
#endif