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
#include <uf/ext/ext.h>


UF_BEHAVIOR_REGISTER_CPP(ext::VoxelizerSceneBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::VoxelizerSceneBehavior, ticks = true, renders = false, multithread = false)
#define this (&self)
void ext::VoxelizerSceneBehavior::initialize( uf::Object& self ) {
#if UF_USE_VULKAN
	auto& metadata = this->getComponent<ext::VoxelizerSceneBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();
	
	auto& scene = uf::scene::getCurrentScene();
	auto& sceneMetadataJson = scene.getComponent<uf::Serializer>();
	auto& sceneTextures = this->getComponent<pod::SceneTextures>();

	UF_BEHAVIOR_METADATA_BIND_SERIALIZER_HOOKS(metadata, metadataJson);

	// initialize voxel map
#if 0
	{
		const uint32_t DEFAULT_VOXEL_SIZE = ext::config["engine"]["scenes"]["vxgi"]["size"].as<uint32_t>(256);
		const float DEFAULT_VOXELIZE_LIMITER = ext::config["engine"]["scenes"]["vxgi"]["limiter"].as<float>(0);
		const uint32_t DEFAULT_DISPATCH_SIZE = ext::config["engine"]["scenes"]["vxgi"]["dispatch"].as<uint32_t>(8);
		const uint32_t DEFAULT_CASCADES = ext::config["engine"]["scenes"]["vxgi"]["cascades"].as<uint32_t>(8);
		const float DEFAULT_CASCADE_POWER = ext::config["engine"]["scenes"]["vxgi"]["cascadePower"].as<float>(1.5);
		const float DEFAULT_GRANULARITY = ext::config["engine"]["scenes"]["vxgi"]["granularity"].as<float>(2.0);
		const float DEFAULT_SHADOWS = ext::config["engine"]["scenes"]["vxgi"]["shadows"].as<size_t>(8);
		const float DEFAULT_PIXEL_SCALE = ext::config["engine"]["scenes"]["vxgi"]["voxelizeScale"].as<float>(1);
		const float DEFAULT_OCCLUSION_FALLOFF = ext::config["engine"]["scenes"]["vxgi"]["occlusionFalloff"].as<float>(128.0f);

		if ( metadata.voxelSize.x == 0 ) metadata.voxelSize.x = DEFAULT_VOXEL_SIZE;
		if ( metadata.voxelSize.y == 0 ) metadata.voxelSize.y = DEFAULT_VOXEL_SIZE;
		if ( metadata.voxelSize.z == 0 ) metadata.voxelSize.z = DEFAULT_VOXEL_SIZE;
		
		if ( metadata.limiter.frequency == 0 ) metadata.limiter.frequency = DEFAULT_VOXELIZE_LIMITER;

		if ( metadata.dispatchSize.x == 0 ) metadata.dispatchSize.x = DEFAULT_DISPATCH_SIZE;
		if ( metadata.dispatchSize.y == 0 ) metadata.dispatchSize.y = DEFAULT_DISPATCH_SIZE;
		if ( metadata.dispatchSize.z == 0 ) metadata.dispatchSize.z = DEFAULT_DISPATCH_SIZE;

		if ( metadata.cascades == 0 ) metadata.cascades = DEFAULT_CASCADES;
		if ( metadata.cascadePower == 0 ) metadata.cascadePower = DEFAULT_CASCADE_POWER;
		if ( metadata.granularity == 0 ) metadata.granularity = DEFAULT_GRANULARITY;
		if ( metadata.voxelizeScale == 0 ) metadata.voxelizeScale = DEFAULT_PIXEL_SCALE;
		if ( metadata.occlusionFalloff == 0 ) metadata.occlusionFalloff = DEFAULT_OCCLUSION_FALLOFF;
		if ( metadata.shadows == 0 ) metadata.shadows = DEFAULT_SHADOWS;

		metadata.extents.min = uf::vector::decode( ext::config["engine"]["scenes"]["vxgi"]["extents"]["min"], pod::Vector3f{-32, -32, -32} );
		metadata.extents.max = uf::vector::decode( ext::config["engine"]["scenes"]["vxgi"]["extents"]["max"], pod::Vector3f{ 32,  32,  32} );

	//	uf::stl::vector<uint8_t> empty(metadata.voxelSize.x * metadata.voxelSize.y * metadata.voxelSize.z * sizeof(uint8_t) * 4);	
	}
#endif

	for ( size_t i = 0; i < metadata.cascades; ++i ) {
		const bool HDR = false;
		auto& id = sceneTextures.voxels.id.emplace_back();
	//	auto& uv = sceneTextures.voxels.uv.emplace_back();
		auto& normal = sceneTextures.voxels.normal.emplace_back();
		auto& radiance = sceneTextures.voxels.radiance.emplace_back();
	//	auto& depth = sceneTextures.voxels.depth.emplace_back();

		id.sampler.descriptor.filter.min = uf::renderer::enums::Filter::NEAREST;
		id.sampler.descriptor.filter.mag = uf::renderer::enums::Filter::NEAREST;

		auto HDR_FORMAT = uf::renderer::enums::Format::R32G32B32A32_SFLOAT;
		auto SDR_FORMAT = uf::renderer::enums::Format::R16G16B16A16_SFLOAT; // uf::renderer::enums::Format::R8G8B8A8_UNORM

		id.fromBuffers( NULL, 0, uf::renderer::enums::Format::R16G16_UINT, metadata.voxelSize.x, metadata.voxelSize.y, metadata.voxelSize.z, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );
	//	uv.fromBuffers( NULL, 0, uf::renderer::enums::Format::R16G16_SFLOAT, metadata.voxelSize.x, metadata.voxelSize.y, metadata.voxelSize.z, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );
		normal.fromBuffers( NULL, 0, uf::renderer::enums::Format::R16G16_SFLOAT, metadata.voxelSize.x, metadata.voxelSize.y, metadata.voxelSize.z, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );
		radiance.fromBuffers( NULL, 0, uf::renderer::settings::pipelines::hdr ? HDR_FORMAT : SDR_FORMAT, metadata.voxelSize.x, metadata.voxelSize.y, metadata.voxelSize.z, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );
	//	depth.fromBuffers( (void*) empty.data(), empty.size(), uf::renderer::enums::Format::R16_SFLOAT, metadata.voxelSize.x, metadata.voxelSize.y, metadata.voxelSize.z, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );
	}
	// initialize render mode
	{
		if ( metadata.fragmentSize.x == 0 ) metadata.fragmentSize.x = metadata.voxelSize.x;
		if ( metadata.fragmentSize.y == 0 ) metadata.fragmentSize.y = metadata.voxelSize.y;

		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		metadata.renderModeName = "VXGI:" + std::to_string((int) this->getUid());
		uf::renderer::addRenderMode( &renderMode, metadata.renderModeName );

		renderMode.metadata.type = uf::renderer::settings::pipelines::names::vxgi;
		renderMode.metadata.pipeline = uf::renderer::settings::pipelines::names::vxgi;
		if ( uf::renderer::settings::pipelines::culling ) {
			renderMode.metadata.pipelines.emplace_back(uf::renderer::settings::pipelines::names::culling);
		}
		renderMode.metadata.pipelines.emplace_back(uf::renderer::settings::pipelines::names::vxgi);
		renderMode.metadata.samples = 1;
		renderMode.metadata.subpasses = metadata.cascades;

		renderMode.blitter.device = &ext::vulkan::device;
		renderMode.width = metadata.fragmentSize.x;
		renderMode.height = metadata.fragmentSize.y;

	//	renderMode.metadata.limiter.frequency = metadata.limiter.frequency;

		uf::stl::string computeShaderFilename = "/shaders/display/vxgi/comp.spv";
		if ( renderMode.metadata.samples > 1 ) {
			computeShaderFilename = uf::string::replace( computeShaderFilename, "comp", "msaa.comp" );
		}
	//	if ( uf::renderer::settings::invariant::deferredSampling ) {
	//		computeShaderFilename = uf::string::replace( computeShaderFilename, "comp", "deferredSampling.comp" );
	//	}
		renderMode.metadata.json["shaders"]["compute"] = computeShaderFilename;
		renderMode.blitter.descriptor.renderMode = metadata.renderModeName;
		renderMode.blitter.descriptor.subpass = -1;
		renderMode.blitter.descriptor.inputs.dispatch = {
			(metadata.voxelSize.x / metadata.dispatchSize.x),
			(metadata.voxelSize.y / metadata.dispatchSize.y),
			(metadata.voxelSize.z / metadata.dispatchSize.z),
		};
		renderMode.blitter.process = true;

		size_t maxTextures2D = ext::config["engine"]["scenes"]["textures"]["max"]["2D"].as<size_t>(512);
		size_t maxTexturesCube = ext::config["engine"]["scenes"]["textures"]["max"]["cube"].as<size_t>(128);
		size_t maxTextures3D = ext::config["engine"]["scenes"]["textures"]["max"]["3D"].as<size_t>(1);

		for ( size_t i = 0; i < maxTextures2D; ++i ) renderMode.blitter.material.textures.emplace_back().aliasTexture(uf::renderer::Texture2D::empty);
		for ( size_t i = 0; i < maxTexturesCube; ++i ) renderMode.blitter.material.textures.emplace_back().aliasTexture(uf::renderer::TextureCube::empty);
		for ( size_t i = 0; i < maxTextures3D; ++i ) renderMode.blitter.material.textures.emplace_back().aliasTexture(uf::renderer::Texture3D::empty);
		
		for ( auto& t : sceneTextures.voxels.id ) renderMode.blitter.material.textures.emplace_back().aliasTexture(t);
		for ( auto& t : sceneTextures.voxels.normal ) renderMode.blitter.material.textures.emplace_back().aliasTexture(t);
		for ( auto& t : sceneTextures.voxels.uv ) renderMode.blitter.material.textures.emplace_back().aliasTexture(t);
		for ( auto& t : sceneTextures.voxels.radiance ) renderMode.blitter.material.textures.emplace_back().aliasTexture(t);
		for ( auto& t : sceneTextures.voxels.depth ) renderMode.blitter.material.textures.emplace_back().aliasTexture(t);

		renderMode.bindCallback( renderMode.CALLBACK_BEGIN, [&]( VkCommandBuffer commandBuffer, size_t _ ){
			// clear textures
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.baseArrayLayer = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 1;

			VkClearColorValue clearColor = { 0.0, 0.0, 0.0, 0.0 };
			for ( auto& t : sceneTextures.voxels.id ) vkCmdClearColorImage( commandBuffer, t.image, t.imageLayout, &clearColor, 1, &subresourceRange );
			for ( auto& t : sceneTextures.voxels.normal ) vkCmdClearColorImage( commandBuffer, t.image, t.imageLayout, &clearColor, 1, &subresourceRange );
			for ( auto& t : sceneTextures.voxels.uv ) vkCmdClearColorImage( commandBuffer, t.image, t.imageLayout, &clearColor, 1, &subresourceRange );
			for ( auto& t : sceneTextures.voxels.radiance ) vkCmdClearColorImage( commandBuffer, t.image, t.imageLayout, &clearColor, 1, &subresourceRange );
			for ( auto& t : sceneTextures.voxels.depth ) vkCmdClearColorImage( commandBuffer, t.image, t.imageLayout, &clearColor, 1, &subresourceRange );
		});
		renderMode.bindCallback( renderMode.CALLBACK_END, [&]( VkCommandBuffer commandBuffer, size_t _ ){
			// parse voxel lighting
			if ( renderMode.blitter.initialized ) {
				auto& pipeline = renderMode.blitter.getPipeline();
				pipeline.record(renderMode.blitter, commandBuffer);
			}
			
			// generate mipmaps
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.baseArrayLayer = 0;
			subresourceRange.layerCount = 1;
			for ( auto& t : sceneTextures.voxels.radiance ) {
				subresourceRange.levelCount = t.mips;
				t.setImageLayout(
					commandBuffer,
					t.image,
					t.imageLayout,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					subresourceRange
				);
				t.generateMipmaps( commandBuffer, 0 );
				t.setImageLayout(
					commandBuffer,
					t.image,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					t.imageLayout,
					subresourceRange
				);
			}
		});
	#if 0
		renderMode.bindCallback( renderMode.EXECUTE_BEGIN, [&]( VkCommandBuffer _, size_t __ ) {
			auto& controller = scene.getController();
			auto controllerTransform = uf::transform::flatten( controller.getComponent<uf::Camera>().getTransform() );
			pod::Vector3f controllerPosition = controllerTransform.position - metadata.extents.min;
			controllerPosition.x = floor(controllerPosition.x);
			controllerPosition.y = floor(controllerPosition.y);
			controllerPosition.z = floor(controllerPosition.z);
			controllerPosition += metadata.extents.min;
			controllerPosition.x = floor(controllerPosition.x);
			controllerPosition.y = floor(controllerPosition.y);
			controllerPosition.z = -floor(controllerPosition.z);

			pod::Vector3f min = metadata.extents.min + controllerPosition;
			pod::Vector3f max = metadata.extents.max + controllerPosition;

			metadata.extents.matrix = uf::matrix::orthographic( min.x, max.x, min.y, max.y, min.z, max.z );

			auto& graph = scene.getGraph();
			for ( auto entity : graph ) {
				if ( !entity->hasComponent<uf::Graphic>() ) continue;
				auto& graphic = entity->getComponent<uf::Graphic>();
				if ( graphic.material.hasShader("geometry", "vxgi") ) {
					auto& shader = graphic.material.getShader("geometry", "vxgi");
					struct UniformDescriptor {
						/*alignas(16)*/ pod::Matrix4f matrix;
						/*alignas(4)*/ float cascadePower;
						/*alignas(4)*/ float padding1;
						/*alignas(4)*/ float padding2;
						/*alignas(4)*/ float padding3;
					};
				#if UF_UNIFORMS_REUSE
					auto& uniform = shader.getUniform("UBO");
					auto& uniforms = uniform.get<UniformDescriptor>();
					
					uniforms = UniformDescriptor{
						.matrix = metadata.extents.matrix,
						.cascadePower = metadata.cascadePower,
					};
					shader.updateUniform( "UBO", uniform );
				#else
					UniformDescriptor uniforms = {
						.matrix = metadata.extents.matrix,
						.cascadePower = metadata.cascadePower,
					};
					shader.updateBuffer( uniforms, shader.getUniformBuffer("UBO") );
				#endif
				}
			}
		} );
	#endif
	#if 0
		auto& deferredRenderMode = uf::renderer::getRenderMode("", true);
		deferredRenderMode.bindCallback( renderMode.CALLBACK_BEGIN, [&]( VkCommandBuffer commandBuffer, size_t _ ){
			VkImageMemoryBarrier imageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.graphics; //VK_QUEUE_FAMILY_IGNORED
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.graphics; //VK_QUEUE_FAMILY_IGNORED
			imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
			imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
			imageMemoryBarrier.subresourceRange.layerCount = 1;
			imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

			for ( auto& t : sceneTextures.voxels.radiance ) {
				VkPipelineStageFlags srcStageMask, dstStageMask;
				imageMemoryBarrier.image = t.image;
				imageMemoryBarrier.oldLayout = t.imageLayout;
				imageMemoryBarrier.newLayout = t.imageLayout;
				imageMemoryBarrier.subresourceRange.levelCount = t.mips;
			
			
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
				srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			
			
			/*
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			*/
				
				vkCmdPipelineBarrier( commandBuffer,
					srcStageMask, dstStageMask,
					VK_FLAGS_NONE,
					0, NULL,
					0, NULL,
					1, &imageMemoryBarrier
				);
			}
		});
		deferredRenderMode.bindCallback( renderMode.CALLBACK_END, [&]( VkCommandBuffer commandBuffer, size_t _ ){
			VkImageMemoryBarrier imageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.graphics; //VK_QUEUE_FAMILY_IGNORED
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.graphics; //VK_QUEUE_FAMILY_IGNORED
			imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
			imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
			imageMemoryBarrier.subresourceRange.layerCount = 1;
			imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

			for ( auto& t : sceneTextures.voxels.radiance ) {
				VkPipelineStageFlags srcStageMask, dstStageMask;
				imageMemoryBarrier.image = t.image;
				imageMemoryBarrier.oldLayout = t.imageLayout;
				imageMemoryBarrier.newLayout = t.imageLayout;
				imageMemoryBarrier.subresourceRange.levelCount = t.mips;
			
			/*
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
				srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			*/
			
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				
				vkCmdPipelineBarrier( commandBuffer,
					srcStageMask, dstStageMask,
					VK_FLAGS_NONE,
					0, NULL,
					0, NULL,
					1, &imageMemoryBarrier
				);
			}
		});
	#endif
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
			auto controllerTransform = uf::transform::flatten( controller.getComponent<uf::Camera>().getTransform() );
			pod::Vector3f controllerPosition = controllerTransform.position - metadata.extents.min;
			controllerPosition.x = floor(controllerPosition.x);
			controllerPosition.y = floor(controllerPosition.y);
			controllerPosition.z = floor(controllerPosition.z);
			controllerPosition += metadata.extents.min;
			controllerPosition.x = floor(controllerPosition.x);
			controllerPosition.y = floor(controllerPosition.y);
			controllerPosition.z = -floor(controllerPosition.z);

			pod::Vector3f min = metadata.extents.min + controllerPosition;
			pod::Vector3f max = metadata.extents.max + controllerPosition;

			metadata.extents.matrix = uf::matrix::orthographic( min.x, max.x, min.y, max.y, min.z, max.z );

			auto& graph = scene.getGraph();
			for ( auto entity : graph ) {
				if ( !entity->hasComponent<uf::Graphic>() ) continue;
				auto& graphic = entity->getComponent<uf::Graphic>();
				if ( graphic.material.hasShader("geometry", uf::renderer::settings::pipelines::names::vxgi) ) {
					auto& shader = graphic.material.getShader("geometry", uf::renderer::settings::pipelines::names::vxgi);
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


				#if UF_UNIFORMS_REUSE
					auto& uniform = shader.getUniform("UBO");
					auto& uniforms = uniform.get<UniformDescriptor>();
					
					uniforms = UniformDescriptor{
						.matrix = metadata.extents.matrix,
						.cascadePower = metadata.cascadePower,
						.granularity = metadata.granularity,
						.voxelizeScale = 1.0f / (metadata.voxelizeScale * std::max<uint32_t>( metadata.voxelSize.x, std::max<uint32_t>(metadata.voxelSize.y, metadata.voxelSize.z))),
						.occlusionFalloff = metadata.occlusionFalloff,
						
						.traceStartOffsetFactor = metadata.traceStartOffsetFactor,
						.shadows = metadata.shadows,
					};
					shader.updateUniform( "UBO", uniform );
				#else
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
				#endif
				}
			}
		}
	#endif
	}
	ext::ExtSceneBehavior::bindBuffers( scene, metadata.renderModeName, "compute", "" );

	auto& deferredRenderMode = uf::renderer::getRenderMode("", true);
	auto& deferredBlitter = *deferredRenderMode.getBlitter();
	if ( deferredBlitter.material.hasShader("compute", "deferred-compute") ) {
		ext::ExtSceneBehavior::bindBuffers( scene, "", "compute", "deferred-compute" );
	} else {
		ext::ExtSceneBehavior::bindBuffers( scene, "", "fragment", "deferred" );
	}
#endif
}
void ext::VoxelizerSceneBehavior::render( uf::Object& self ){}
void ext::VoxelizerSceneBehavior::destroy( uf::Object& self ){
#if UF_USE_VULKAN
	if ( this->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		uf::renderer::removeRenderMode( &renderMode, false );
		this->deleteComponent<uf::renderer::RenderTargetRenderMode>();
	}
#endif
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

	serializer["vxgi"]["extents"]["min"] = uf::vector::encode(/*this->*/extents.min);
	serializer["vxgi"]["extents"]["max"] = uf::vector::encode(/*this->*/extents.max);
}
void ext::VoxelizerSceneBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ) {
	// merge vxgi settings with global settings
	{
		const auto& globalSettings = ext::config["engine"]["scenes"]["vxgi"];
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

	/*this->*/extents.min = uf::vector::decode( serializer["vxgi"]["extents"]["min"], /*this->*/extents.min );
	/*this->*/extents.max = uf::vector::decode( serializer["vxgi"]["extents"]["max"], /*this->*/extents.max );
}
#undef this
#endif