#include "behavior.h"

#include <uf/utils/renderer/renderer.h>

#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/camera/camera.h>
#include <uf/ext/gltf/gltf.h>
#include <uf/engine/asset/asset.h>

#include <uf/ext/xatlas/xatlas.h>

#include "../light/behavior.h"
#include "../scene/behavior.h"

#define COMP_SHADER_USED 1

UF_BEHAVIOR_REGISTER_CPP(ext::VoxelizerBehavior)
#define this (&self)
void ext::VoxelizerBehavior::initialize( uf::Object& self ) {
#if UF_USE_VULKAN
	auto& metadata = this->getComponent<ext::VoxelizerBehavior::Metadata>();
	auto& sceneTextures = this->getComponent<pod::SceneTextures>();
	// initialize voxel map
	{
		if ( metadata.voxelSize.x == 0 ) metadata.voxelSize.x = 256;
		if ( metadata.voxelSize.y == 0 ) metadata.voxelSize.y = 256;
		if ( metadata.voxelSize.z == 0 ) metadata.voxelSize.z = 256;

		std::vector<uint8_t> empty(metadata.voxelSize.x * metadata.voxelSize.y * metadata.voxelSize.z * sizeof(uint8_t) * 4);
		
		sceneTextures.voxels.id.sampler.descriptor.filter.min = VK_FILTER_NEAREST;
		sceneTextures.voxels.id.sampler.descriptor.filter.mag = VK_FILTER_NEAREST;

		sceneTextures.voxels.id.fromBuffers( (void*) empty.data(), empty.size(), uf::renderer::enums::Format::R16G16_UINT, metadata.voxelSize.x, metadata.voxelSize.y, metadata.voxelSize.z, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );
		sceneTextures.voxels.normal.fromBuffers( (void*) empty.data(), empty.size(), uf::renderer::enums::Format::R16G16_SFLOAT, metadata.voxelSize.x, metadata.voxelSize.y, metadata.voxelSize.z, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );
		sceneTextures.voxels.uv.fromBuffers( (void*) empty.data(), empty.size(), uf::renderer::enums::Format::R16G16_SFLOAT, metadata.voxelSize.x, metadata.voxelSize.y, metadata.voxelSize.z, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );
		sceneTextures.voxels.albedo.fromBuffers( (void*) empty.data(), empty.size(), uf::renderer::enums::Format::R8G8B8A8_UNORM, metadata.voxelSize.x, metadata.voxelSize.y, metadata.voxelSize.z, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );
	}
	// initialize render mode
	{
	//	if ( metadata.fragmentSize.x == 0 ) metadata.fragmentSize.x = metadata.voxelSize.x * 2;
	//	if ( metadata.fragmentSize.y == 0 ) metadata.fragmentSize.y = metadata.voxelSize.y * 2;

		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		metadata.renderModeName = "SVOGI:" + std::to_string((int) this->getUid());
		uf::renderer::addRenderMode( &renderMode, metadata.renderModeName );
		renderMode.metadata["type"] = "svogi";
		renderMode.metadata["samples"] = 1;

		renderMode.blitter.device = &ext::vulkan::device;
		renderMode.width = metadata.fragmentSize.x;
		renderMode.height = metadata.fragmentSize.y;
	#if COMP_SHADER_USED
		renderMode.metadata["shaders"]["compute"] = "/shaders/display/svogi.comp.spv";
		renderMode.blitter.descriptor.renderMode = metadata.renderModeName;
		renderMode.blitter.descriptor.subpass = -1;
		renderMode.blitter.process = true;
	#else
		renderMode.metadata["shaders"] = false;
		renderMode.blitter.process = false;
	#endif

		renderMode.blitter.material.textures.emplace_back().aliasTexture(sceneTextures.voxels.id); //this->getComponent<uf::renderer::Texture3D>());
		renderMode.blitter.material.textures.emplace_back().aliasTexture(sceneTextures.voxels.normal); //this->getComponent<uf::renderer::Texture3D>());
		renderMode.blitter.material.textures.emplace_back().aliasTexture(sceneTextures.voxels.uv); //this->getComponent<uf::renderer::Texture3D>());
		renderMode.blitter.material.textures.emplace_back().aliasTexture(sceneTextures.voxels.albedo); //this->getComponent<uf::renderer::Texture3D>());
		renderMode.blitter.material.textures.emplace_back().aliasTexture(sceneTextures.noise); //this->getComponent<uf::renderer::Texture3D>());
		renderMode.blitter.material.textures.emplace_back().aliasTexture(sceneTextures.skybox); //this->getComponent<uf::renderer::TextureCube>());

		renderMode.bindCallback( renderMode.CALLBACK_BEGIN, [&]( VkCommandBuffer commandBuffer ){
			// clear textures
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.baseArrayLayer = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 1;

			VkClearColorValue clearColor = { 0.0, 0.0, 0.0, 0.0 };
			vkCmdClearColorImage( commandBuffer, sceneTextures.voxels.id.image, sceneTextures.voxels.id.imageLayout, &clearColor, 1, &subresourceRange );
			vkCmdClearColorImage( commandBuffer, sceneTextures.voxels.normal.image, sceneTextures.voxels.normal.imageLayout, &clearColor, 1, &subresourceRange );
			vkCmdClearColorImage( commandBuffer, sceneTextures.voxels.uv.image, sceneTextures.voxels.uv.imageLayout, &clearColor, 1, &subresourceRange );
			vkCmdClearColorImage( commandBuffer, sceneTextures.voxels.albedo.image, sceneTextures.voxels.albedo.imageLayout, &clearColor, 1, &subresourceRange );
		});
		renderMode.bindCallback( renderMode.CALLBACK_END, [&]( VkCommandBuffer commandBuffer ){
			// parse voxel lighting
		#if COMP_SHADER_USED
			if ( renderMode.blitter.initialized ) {
				auto& pipeline = renderMode.blitter.getPipeline();
				pipeline.record(renderMode.blitter, commandBuffer);
				vkCmdDispatch(commandBuffer, metadata.voxelSize.x / metadata.dispatchSize.x, metadata.voxelSize.y / metadata.dispatchSize.y, metadata.voxelSize.z / metadata.dispatchSize.z);
			}
		#endif
			// generate mipmaps
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.baseArrayLayer = 0;
			subresourceRange.levelCount = sceneTextures.voxels.albedo.mips;
			subresourceRange.layerCount = 1;

			sceneTextures.voxels.albedo.setImageLayout(
				commandBuffer,
				sceneTextures.voxels.albedo.image,
				sceneTextures.voxels.albedo.imageLayout,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				subresourceRange
			);
			sceneTextures.voxels.albedo.generateMipmaps( commandBuffer, 0 );
			sceneTextures.voxels.albedo.setImageLayout(
				commandBuffer,
				sceneTextures.voxels.albedo.image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				sceneTextures.voxels.albedo.imageLayout,
				subresourceRange
			);
		});
	}
#endif
}
void ext::VoxelizerBehavior::tick( uf::Object& self ) {
#if UF_USE_VULKAN
	if ( !this->hasComponent<uf::renderer::RenderTargetRenderMode>() ) return;

	auto& metadata = this->getComponent<ext::VoxelizerBehavior::Metadata>();
	auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
	renderMode.setTarget("");
	if ( renderMode.executed && !metadata.initialized ) {
	//	renderMode.execute = false;
		metadata.initialized = true;
	}
#if COMP_SHADER_USED
	auto& scene = uf::scene::getCurrentScene();
	ext::ExtSceneBehavior::bindBuffers( scene, metadata.renderModeName, true );
#endif
#endif
}
void ext::VoxelizerBehavior::render( uf::Object& self ){}
void ext::VoxelizerBehavior::destroy( uf::Object& self ){
#if UF_USE_VULKAN
	if ( this->hasComponent<uf::renderer::RenderTargetRenderMode>() ) {
		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		uf::renderer::removeRenderMode( &renderMode, false );
		this->deleteComponent<uf::renderer::RenderTargetRenderMode>();
	}
#endif
}
#undef this