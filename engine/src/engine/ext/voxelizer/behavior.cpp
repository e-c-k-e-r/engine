#include <uf/config.h>
#if UF_USE_VULKAN

#include "behavior.h"

#include <uf/utils/renderer/renderer.h>

#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/ext/gltf/gltf.h>
#include <uf/engine/asset/asset.h>

#include <uf/ext/xatlas/xatlas.h>

#include "../light/behavior.h"
#include "../scene/behavior.h"
#include <uf/engine/ext.h>

#define ALIAS_OUTPUT_TO_RADIANCE 1
#define COMPUTE_MIPMAP_GENERATION 1

namespace {
	struct AtomicCounter {
		uint32_t counter;
	};
	struct PushConstants {
		uint32_t mips;
		uint32_t cascade;
		uint32_t numWorkGroups;
		uint32_t workGroupOffset;
	};
}

UF_BEHAVIOR_REGISTER_CPP(ext::VoxelizerSceneBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::VoxelizerSceneBehavior, ticks = true, renders = false, thread = "")
#define this (&self)
void ext::VoxelizerSceneBehavior::initialize( uf::Object& self ) {
	if ( this->getName() == "Main Menu" ) return; // do not setup
#if UF_USE_VULKAN
	auto& metadata = this->getComponent<ext::VoxelizerSceneBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();
	
	auto& scene = uf::scene::getCurrentScene();
	auto& sceneMetadataJson = scene.getComponent<uf::Serializer>();
	auto& sceneTextures = this->getComponent<pod::SceneTextures>();

	UF_BEHAVIOR_METADATA_BIND_SERIALIZER_HOOKS(metadata, metadataJson);

	auto mips = uf::vector::mips( metadata.voxelSize );
	for ( size_t i = 0; i < metadata.cascades; ++i ) {
		const bool HDR = false;
		auto& id = sceneTextures.voxels.id.emplace_back();
		id.sampler.descriptor.filter.min = uf::renderer::enums::Filter::NEAREST;
		id.sampler.descriptor.filter.mag = uf::renderer::enums::Filter::NEAREST;
		id.mips = 0;
		id.fromBuffers( NULL, 0, uf::renderer::enums::Format::R32_UINT, metadata.voxelSize.x, metadata.voxelSize.y, metadata.voxelSize.z, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );

		auto& normal = sceneTextures.voxels.normal.emplace_back();
		normal.mips = 0;
		normal.fromBuffers( NULL, 0, uf::renderer::enums::Format::R32_UINT, metadata.voxelSize.x, metadata.voxelSize.y, metadata.voxelSize.z, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );		

		auto& radiance = sceneTextures.voxels.radiance.emplace_back();
		radiance.mips = ALIAS_OUTPUT_TO_RADIANCE ? mips : 0;
		radiance.fromBuffers( NULL, 0, uf::renderer::enums::Format::R32_UINT, metadata.voxelSize.x, metadata.voxelSize.y, metadata.voxelSize.z, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL, ALIAS_OUTPUT_TO_RADIANCE ? VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT : VkImageCreateFlags{} );
		
		auto& output = sceneTextures.voxels.output.emplace_back();
		output.mips = mips;

	#if ALIAS_OUTPUT_TO_RADIANCE
		{
			output.aliasTexture( radiance );
			VkImageViewCreateInfo viewCreateInfo = {};
			viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewCreateInfo.image = radiance.image;
			viewCreateInfo.viewType = radiance.viewType;
			viewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
			viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
			viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewCreateInfo.subresourceRange.baseMipLevel = 0;
			viewCreateInfo.subresourceRange.levelCount = mips;
			viewCreateInfo.subresourceRange.baseArrayLayer = 0;
			viewCreateInfo.subresourceRange.layerCount = 1;

			VK_CHECK_RESULT(vkCreateImageView(uf::renderer::device.logicalDevice, &viewCreateInfo, nullptr, &output.view));
			VK_REGISTER_HANDLE( output.view );
			metadata.views.emplace_back( output.view );
		}
	#else
		output.fromBuffers( NULL, 0, uf::renderer::enums::Format::R8G8B8A8_UNORM, metadata.voxelSize.x, metadata.voxelSize.y, metadata.voxelSize.z, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );	
	#endif

	#if COMPUTE_MIPMAP_GENERATION
		for ( auto i = 1; i < mips; ++i ) {
			auto& mip = sceneTextures.voxels.outputMipmaps.emplace_back();
			mip.aliasTexture( output );
			VkImageViewCreateInfo viewCreateInfo = {};
			viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewCreateInfo.image = output.image;
			viewCreateInfo.viewType = output.viewType;
			viewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
			viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
			viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			viewCreateInfo.subresourceRange.baseMipLevel = i;
			viewCreateInfo.subresourceRange.levelCount = 1;
			viewCreateInfo.subresourceRange.baseArrayLayer = 0;
			viewCreateInfo.subresourceRange.layerCount = 1;

			VK_CHECK_RESULT(vkCreateImageView(uf::renderer::device.logicalDevice, &viewCreateInfo, nullptr, &mip.view));
			VK_REGISTER_HANDLE( mip.view );
			metadata.views.emplace_back( mip.view );
		}
	#endif
	}
	// initialize render mode
	{
		if ( metadata.fragmentSize.x == 0 ) metadata.fragmentSize.x = metadata.voxelSize.x;
		if ( metadata.fragmentSize.y == 0 ) metadata.fragmentSize.y = metadata.voxelSize.y;

		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		metadata.renderModeName = ::fmt::format("VXGI:{}", this->getUid());
		renderMode.metadata.name = metadata.renderModeName;
		if ( uf::renderer::settings::experimental::registerRenderMode ) uf::renderer::addRenderMode( &renderMode, metadata.renderModeName );

		auto& blitter = renderMode.blitter;
		renderMode.metadata.type = uf::renderer::settings::pipelines::names::vxgi;
		renderMode.metadata.pipeline = uf::renderer::settings::pipelines::names::vxgi;
		if ( uf::renderer::settings::pipelines::culling ) {
		//	renderMode.metadata.pipelines.emplace_back(uf::renderer::settings::pipelines::names::culling);
		}
		renderMode.metadata.pipelines.emplace_back(uf::renderer::settings::pipelines::names::vxgi);
		renderMode.metadata.samples = 1;
	//	renderMode.metadata.subpasses = metadata.cascades;
		renderMode.metadata.views = metadata.cascades;
		
		renderMode.width = metadata.fragmentSize.x;
		renderMode.height = metadata.fragmentSize.y;

		blitter.device = &uf::renderer::device;
		blitter.material.device = &uf::renderer::device;

		blitter.descriptor.renderMode = metadata.renderModeName;
		blitter.descriptor.subpass = -1;
		blitter.descriptor.bind.width = metadata.voxelSize.x;
		blitter.descriptor.bind.height = metadata.voxelSize.y;
		blitter.descriptor.bind.depth = metadata.voxelSize.z;
		blitter.descriptor.bind.point = VK_PIPELINE_BIND_POINT_COMPUTE;
		blitter.process = true;

		size_t maxLights = uf::config["engine"]["scenes"]["lights"]["max"].as<size_t>(512);
		size_t maxTextures2D = uf::config["engine"]["scenes"]["textures"]["max"]["2D"].as<size_t>(512);
		size_t maxTexturesCube = uf::config["engine"]["scenes"]["textures"]["max"]["cube"].as<size_t>(128);
		size_t maxTextures3D = uf::config["engine"]["scenes"]["textures"]["max"]["3D"].as<size_t>(1);
		size_t maxCascades = uf::config["engine"]["scenes"]["vxgi"]["cascades"].as<size_t>(16);
		size_t maxMips = uf::vector::mips( pod::Vector3ui{ 256, 256, 256 } ); // log2(256) = 9

		renderMode.metadata.json["shaders"] = true;
		{
			blitter.material.attachShader( uf::io::root+"/shaders/display/vxgi/comp.spv", uf::renderer::enums::Shader::COMPUTE, "" );
			auto& shader = blitter.material.getShader("compute", "");

			shader.setSpecializationConstants({
				{ "TEXTURES", maxTextures2D },
				{ "CUBEMAPS", maxTexturesCube },
				{ "CASCADES", maxCascades },
			});
			shader.setDescriptorCounts({
				{ "samplerTextures", maxTextures2D },
				{ "samplerCubemaps", maxTexturesCube },
				{ "voxelId", maxCascades },
				{ "voxelNormal", maxCascades },
				{ "voxelRadiance", maxCascades },
				{ "voxelOutput", maxCascades },
			});

			auto& scene = uf::scene::getCurrentScene();
			auto& sceneTextures = scene.getComponent<pod::SceneTextures>();
			
			shader.textures.clear();
			
			for ( auto& t : sceneTextures.voxels.id ) shader.textures.emplace_back().aliasTexture(t);
			for ( auto& t : sceneTextures.voxels.normal ) shader.textures.emplace_back().aliasTexture(t);
			for ( auto& t : sceneTextures.voxels.radiance ) shader.textures.emplace_back().aliasTexture(t);
			for ( auto& t : sceneTextures.voxels.output ) shader.textures.emplace_back().aliasTexture(t);
		}

	#if COMPUTE_MIPMAP_GENERATION
		{
			blitter.material.attachShader( uf::io::root+"/shaders/display/vxgi/mips.comp.spv", uf::renderer::enums::Shader::COMPUTE, "mipmap" );
			auto& shader = blitter.material.getShader("compute", "mipmap");

			shader.setSpecializationConstants({
				{ "TEXTURES", maxTextures2D },
				{ "CUBEMAPS", maxTexturesCube },
				{ "CASCADES", maxCascades },
				{ "MIPS", maxMips },
			});
			shader.setDescriptorCounts({
				{ "voxelRadiance", maxCascades },
				{ "voxelMips", maxCascades * (maxMips - 1) },
			});

			auto& scene = uf::scene::getCurrentScene();
			auto& sceneTextures = scene.getComponent<pod::SceneTextures>();
			
			shader.textures.clear();				
			for ( auto& t : sceneTextures.voxels.output ) shader.textures.emplace_back().aliasTexture(t);
			for ( auto& t : sceneTextures.voxels.outputMipmaps ) shader.textures.emplace_back().aliasTexture(t);

			metadata.atomicCounter.initialize( (const void*) nullptr, sizeof(::AtomicCounter) * 1, uf::renderer::enums::Buffer::STORAGE );
			shader.aliasBuffer("atomicCounter", metadata.atomicCounter);	
		}
	#endif

		renderMode.bindCallback( renderMode.CALLBACK_BEGIN, [&]( VkCommandBuffer commandBuffer, size_t _ ){
			// clear textures
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.baseArrayLayer = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 1;

			VkClearColorValue clearColor = { 0.0, 0.0, 0.0, 0.0 };
			for ( auto& t : sceneTextures.voxels.id ) vkCmdClearColorImage( commandBuffer, t.image, t.layout, &clearColor, 1, &subresourceRange );
			for ( auto& t : sceneTextures.voxels.normal ) vkCmdClearColorImage( commandBuffer, t.image, t.layout, &clearColor, 1, &subresourceRange );
			for ( auto& t : sceneTextures.voxels.radiance ) vkCmdClearColorImage( commandBuffer, t.image, t.layout, &clearColor, 1, &subresourceRange );
			for ( auto& t : sceneTextures.voxels.output ) vkCmdClearColorImage( commandBuffer, t.image, t.layout, &clearColor, 1, &subresourceRange );
		});

		// 
		renderMode.bindCallback( renderMode.CALLBACK_END, [&]( VkCommandBuffer commandBuffer, size_t _ ){
			// parse voxel lighting
			if ( blitter.initialized ) {
				auto descriptor = blitter.descriptor;
				//descriptor.pipeline = "lighting";
				blitter.record( commandBuffer, descriptor );
			}
			
			// generate mipmaps
		#if COMPUTE_MIPMAP_GENERATION
			if ( blitter.initialized ) {
				auto& shader = blitter.material.getShader("compute", "mipmap");
				auto mips = uf::vector::mips( pod::Vector3ui{ blitter.descriptor.bind.width, blitter.descriptor.bind.height, blitter.descriptor.bind.depth } );

				for ( auto cascade = 0; cascade < sceneTextures.voxels.output.size(); ++cascade ) {
					vkCmdFillBuffer(commandBuffer, metadata.atomicCounter.buffer, 0, 4, 0);
					VkMemoryBarrier counterBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
					counterBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
					counterBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
					vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &counterBarrier, 0, nullptr, 0, nullptr);


					auto& pushConstant = shader.pushConstants.front().get<::PushConstants>();
					pushConstant = {
						.mips = mips,
						.cascade = cascade,
						.numWorkGroups = 0,
						.workGroupOffset = 0,
					};
					auto descriptor = blitter.descriptor;
					descriptor.pipeline = "mipmap";

					blitter.record( commandBuffer, descriptor );
				}
			}
		#else
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.baseArrayLayer = 0;
			subresourceRange.layerCount = 1;
			for ( auto& t : sceneTextures.voxels.output ) {
				subresourceRange.levelCount = t.mips;
				t.setImageLayout( commandBuffer, t.image, t.layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange );
				t.generateMipmaps( commandBuffer, 0 );
				t.setImageLayout( commandBuffer, t.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, t.layout, subresourceRange );
			}
		#endif
		});
	}
#endif
}
void ext::VoxelizerSceneBehavior::tick( uf::Object& self ) {
#if UF_USE_VULKAN
	if ( !this->hasComponent<uf::renderer::RenderTargetRenderMode>() ) return;

	auto& metadata = this->getComponent<ext::VoxelizerSceneBehavior::Metadata>();
	auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
	
	auto& scene = uf::scene::getCurrentScene();
	auto& sceneTextures = scene.getComponent<pod::SceneTextures>();
	renderMode.setTarget("");

	if ( renderMode.executed ) {
		if ( !metadata.initialized ) metadata.initialized = true;

		if ( metadata.limiter.frequency > 0 ) {
			if ( metadata.limiter.timer > metadata.limiter.frequency ) {
				metadata.limiter.timer = 0;
				renderMode.metadata.limiter.execute = true;
			} else {
				metadata.limiter.timer = metadata.limiter.timer + uf::physics::time::delta;
				renderMode.metadata.limiter.execute = false;
			}
		}
	
	#if 1
	//	bool should = false;
	//	if ( renderMode.metadata.limiter.frequency <= 0 && renderMode.metadata.limiter.timer <= 0 ) should = true;
	//	else if ( renderMode.metadata.limiter.timer + renderMode.metadata.limiter.frequency >= renderMode.metadata.limiter.frequency ) should = true;

	//	if ( renderMode.execute ) {
		if ( renderMode.metadata.limiter.execute ) {
	//	if ( should ) {
			auto& controller = scene.getController();
			auto& camera = scene.getCamera( controller );
			auto controllerTransform = uf::transform::flatten( camera.getTransform() );
			
			float voxelWorldSizeX = (metadata.extents.max.x - metadata.extents.min.x) / (float)(metadata.voxelSize.x);
			float voxelWorldSizeY = (metadata.extents.max.y - metadata.extents.min.y) / (float)(metadata.voxelSize.y);
			float voxelWorldSizeZ = (metadata.extents.max.z - metadata.extents.min.z) / (float)(metadata.voxelSize.z);

			pod::Vector3f controllerPosition = controllerTransform.position - metadata.extents.min;

			controllerPosition.x = std::floor(controllerPosition.x / voxelWorldSizeX) * voxelWorldSizeX;
			controllerPosition.y = std::floor(controllerPosition.y / voxelWorldSizeY) * voxelWorldSizeY;
			controllerPosition.z = std::floor(controllerPosition.z / voxelWorldSizeZ) * voxelWorldSizeZ;

			controllerPosition += metadata.extents.min;

			controllerPosition.x = std::floor(controllerPosition.x / voxelWorldSizeX) * voxelWorldSizeX;
			controllerPosition.y = std::floor(controllerPosition.y / voxelWorldSizeY) * voxelWorldSizeY;
			controllerPosition.z = -std::floor(controllerPosition.z / voxelWorldSizeZ) * voxelWorldSizeZ;

			pod::Vector3f min = metadata.extents.min + controllerPosition;
			pod::Vector3f max = metadata.extents.max + controllerPosition;

			metadata.extents.matrix = uf::matrix::orthographic( min.x, max.x, min.y, max.y, min.z, max.z );

			auto/*&*/ graph = scene.getGraph();
			for ( auto entity : graph ) {
				if ( !entity->hasComponent<uf::Graphic>() ) continue;
				auto& blitter = entity->getComponent<uf::Graphic>();
				if ( blitter.material.hasShader("geometry", uf::renderer::settings::pipelines::names::vxgi) ) {
					auto& shader = blitter.material.getShader("geometry", uf::renderer::settings::pipelines::names::vxgi);
					struct UniformDescriptor {
						/*alignas(16)*/ pod::Matrix4f matrix;
						/*alignas(4)*/ float cascadePower;
						/*alignas(4)*/ float granularity;
						/*alignas(4)*/ float voxelizeScale;
						/*alignas(4)*/ float occlusionFalloff;
						
						/*alignas(4)*/ float traceStartOffsetFactor;
						/*alignas(4)*/ uint32_t shadows;
						/*alignas(4)*/ uint32_t padding2;
						/*alignas(4)*/ uint32_t padding3;
					};

					UniformDescriptor uniforms = {
						.matrix = metadata.extents.matrix,
						.cascadePower = metadata.cascadePower,
						.granularity = metadata.granularity,
						.voxelizeScale = 1.0f / (metadata.voxelizeScale * std::max<uint32_t>( metadata.voxelSize.x, std::max<uint32_t>(metadata.voxelSize.y, metadata.voxelSize.z))),
						.occlusionFalloff = metadata.occlusionFalloff,
						
						.traceStartOffsetFactor = metadata.traceStartOffsetFactor,
						.shadows = metadata.shadows,
					};
					shader.updateBuffer( (const void*) &uniforms, sizeof(uniforms), shader.getUniformBuffer("UBO") );
				}
			}
		}
	#endif
	}

	ext::ExtSceneBehavior::bindBuffers( scene, metadata.renderModeName, "compute", "" );

	auto& deferredRenderMode = uf::renderer::getRenderMode("", true);
	auto& deferredBlitter = deferredRenderMode.getBlitter();
	if ( deferredBlitter.material.hasShader("compute", "deferred") ) {
		ext::ExtSceneBehavior::bindBuffers( scene, "", "compute", "deferred" );
	} else {
		ext::ExtSceneBehavior::bindBuffers( scene, "", "fragment", "deferred" );
	}
#endif
}
void ext::VoxelizerSceneBehavior::render( uf::Object& self ){}
void ext::VoxelizerSceneBehavior::destroy( uf::Object& self ){
	auto& metadata = this->getComponent<ext::VoxelizerSceneBehavior::Metadata>();
	metadata.atomicCounter.destroy(false);
	for ( auto& view : metadata.views ) {
		ext::vulkan::mutex.lock();
		auto& texture = uf::renderer::device.transient.textures.emplace_back();
		ext::vulkan::mutex.unlock();

		texture.device = &uf::renderer::device;
		texture.view = view;
	}
}
void ext::VoxelizerSceneBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ) {
	serializer["vxgi"]["size"] = /*this->*/voxelSize.x;
	serializer["vxgi"]["limiter"] = /*this->*/limiter.frequency;
	serializer["vxgi"]["dispatch"] = /*this->*/dispatchSize.x;

	serializer["vxgi"]["cascades"] = /*this->*/cascades;
	serializer["vxgi"]["cascadePower"] = /*this->*/cascadePower;
	serializer["vxgi"]["granularity"] = /*this->*/granularity;
	serializer["vxgi"]["voxelizeScale"] = /*this->*/voxelizeScale;
	serializer["vxgi"]["occlusionFalloff"] = /*this->*/occlusionFalloff;
	serializer["vxgi"]["traceStartOffsetFactor"] = /*this->*/traceStartOffsetFactor;
	serializer["vxgi"]["shadows"] = /*this->*/shadows;
	serializer["vxgi"]["filtering"] = /*this->*/filtering;

	serializer["vxgi"]["extents"]["min"] = uf::vector::encode(/*this->*/extents.min);
	serializer["vxgi"]["extents"]["max"] = uf::vector::encode(/*this->*/extents.max);
}
void ext::VoxelizerSceneBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ) {
	// merge vxgi settings with global settings
	{
		const auto& globalSettings = uf::config["engine"]["scenes"]["vxgi"];
		ext::json::forEach( globalSettings, [&]( const uf::stl::string& key, const ext::json::Value& value ){
			if ( !ext::json::isNull( serializer["vxgi"][key] ) ) return;
			serializer["vxgi"][key] = value;
		} );
	}

	/*this->*/voxelSize.x = serializer["vxgi"]["size"].as(/*this->*/voxelSize.x);
	/*this->*/voxelSize.y = serializer["vxgi"]["size"].as(/*this->*/voxelSize.y);
	/*this->*/voxelSize.z = serializer["vxgi"]["size"].as(/*this->*/voxelSize.z);
	
	/*this->*/limiter.frequency = serializer["vxgi"]["limiter"].as(/*this->*/limiter.frequency);

	/*this->*/dispatchSize.x = serializer["vxgi"]["dispatch"].as(/*this->*/dispatchSize.x);
	/*this->*/dispatchSize.y = serializer["vxgi"]["dispatch"].as(/*this->*/dispatchSize.x);
	/*this->*/dispatchSize.z = serializer["vxgi"]["dispatch"].as(/*this->*/dispatchSize.x);

	/*this->*/cascades = serializer["vxgi"]["cascades"].as(/*this->*/cascades);
	/*this->*/cascadePower = serializer["vxgi"]["cascadePower"].as(/*this->*/cascadePower);
	/*this->*/granularity = serializer["vxgi"]["granularity"].as(/*this->*/granularity);
	/*this->*/voxelizeScale = serializer["vxgi"]["voxelizeScale"].as(/*this->*/voxelizeScale);
	/*this->*/occlusionFalloff = serializer["vxgi"]["occlusionFalloff"].as(/*this->*/occlusionFalloff);
	/*this->*/traceStartOffsetFactor = serializer["vxgi"]["traceStartOffsetFactor"].as(/*this->*/traceStartOffsetFactor);
	/*this->*/shadows = serializer["vxgi"]["shadows"].as(/*this->*/shadows);
	/*this->*/filtering = serializer["vxgi"]["filtering"].as(/*this->*/filtering);

	/*this->*/extents.min = uf::vector::decode( serializer["vxgi"]["extents"]["min"], /*this->*/extents.min );
	/*this->*/extents.max = uf::vector::decode( serializer["vxgi"]["extents"]["max"], /*this->*/extents.max );
}
#undef this
#endif