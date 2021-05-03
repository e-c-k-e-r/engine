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
#include <uf/ext/ext.h>

#define COMP_SHADER_USED 1

UF_BEHAVIOR_REGISTER_CPP(ext::VoxelizerBehavior)
#define this (&self)
void ext::VoxelizerBehavior::initialize( uf::Object& self ) {
#if UF_USE_VULKAN
	auto& metadata = this->getComponent<ext::VoxelizerBehavior::Metadata>();
	auto& scene = uf::scene::getCurrentScene();
	auto& sceneMetadataJson = scene.getComponent<uf::Serializer>();
	auto& sceneTextures = this->getComponent<pod::SceneTextures>();
	// initialize voxel map
	{
		const uint32_t DEFAULT_VOXEL_SIZE = ext::config["engine"]["scenes"]["vxgi"]["size"].as<uint32_t>(256);
		const float DEFAULT_VOXELIZE_LIMITER = ext::config["engine"]["scenes"]["vxgi"]["limiter"].as<float>(0);
		const uint32_t DEFAULT_DISPATCH_SIZE = ext::config["engine"]["scenes"]["vxgi"]["dispatch"].as<uint32_t>(8);
		const uint32_t DEFAULT_CASCADES = ext::config["engine"]["scenes"]["vxgi"]["cascades"].as<uint32_t>(8);

		if ( metadata.voxelSize.x == 0 ) metadata.voxelSize.x = DEFAULT_VOXEL_SIZE;
		if ( metadata.voxelSize.y == 0 ) metadata.voxelSize.y = DEFAULT_VOXEL_SIZE;
		if ( metadata.voxelSize.z == 0 ) metadata.voxelSize.z = DEFAULT_VOXEL_SIZE;
		
		if ( metadata.renderer.limiter == 0 ) metadata.renderer.limiter = DEFAULT_VOXELIZE_LIMITER;

		if ( metadata.dispatchSize.x == 0 ) metadata.dispatchSize.x = DEFAULT_DISPATCH_SIZE;
		if ( metadata.dispatchSize.y == 0 ) metadata.dispatchSize.y = DEFAULT_DISPATCH_SIZE;
		if ( metadata.dispatchSize.z == 0 ) metadata.dispatchSize.z = DEFAULT_DISPATCH_SIZE;

		if ( metadata.cascades == 0 ) metadata.cascades = DEFAULT_CASCADES;

		metadata.extents.min = uf::vector::decode( ext::config["engine"]["scenes"]["vxgi"]["extents"]["min"], pod::Vector3f{-32, -32, -32} );
		metadata.extents.max = uf::vector::decode( ext::config["engine"]["scenes"]["vxgi"]["extents"]["max"], pod::Vector3f{ 32,  32,  32} );

		std::vector<uint8_t> empty(metadata.voxelSize.x * metadata.voxelSize.y * metadata.voxelSize.z * sizeof(uint8_t) * 4);
		
		for ( size_t i = 0; i < metadata.cascades; ++i ) {
			auto& id = sceneTextures.voxels.id.emplace_back();
			auto& uv = sceneTextures.voxels.uv.emplace_back();
			auto& normal = sceneTextures.voxels.normal.emplace_back();
			auto& radiance = sceneTextures.voxels.radiance.emplace_back();

			id.sampler.descriptor.filter.min = VK_FILTER_NEAREST;
			id.sampler.descriptor.filter.mag = VK_FILTER_NEAREST;

			id.fromBuffers( (void*) empty.data(), empty.size(), uf::renderer::enums::Format::R16G16_UINT, metadata.voxelSize.x, metadata.voxelSize.y, metadata.voxelSize.z, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );
			uv.fromBuffers( (void*) empty.data(), empty.size(), uf::renderer::enums::Format::R16G16_SFLOAT, metadata.voxelSize.x, metadata.voxelSize.y, metadata.voxelSize.z, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );
			normal.fromBuffers( (void*) empty.data(), empty.size(), uf::renderer::enums::Format::R16G16_SFLOAT, metadata.voxelSize.x, metadata.voxelSize.y, metadata.voxelSize.z, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );
			radiance.fromBuffers( (void*) empty.data(), empty.size(), uf::renderer::enums::Format::R8G8B8A8_UNORM, metadata.voxelSize.x, metadata.voxelSize.y, metadata.voxelSize.z, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );
		}
	}
	// initialize render mode
	{
		if ( metadata.fragmentSize.x == 0 ) metadata.fragmentSize.x = metadata.voxelSize.x;
		if ( metadata.fragmentSize.y == 0 ) metadata.fragmentSize.y = metadata.voxelSize.y;

		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		metadata.renderModeName = "VXGI:" + std::to_string((int) this->getUid());
		uf::renderer::addRenderMode( &renderMode, metadata.renderModeName );
		renderMode.metadata["type"] = "vxgi";
		renderMode.metadata["samples"] = 2;

		renderMode.blitter.device = &ext::vulkan::device;
		renderMode.width = metadata.fragmentSize.x;
		renderMode.height = metadata.fragmentSize.y;
	#if COMP_SHADER_USED
		renderMode.metadata["shaders"]["compute"] = "/shaders/display/vxgi.comp.spv";
		renderMode.blitter.descriptor.renderMode = metadata.renderModeName;
		renderMode.blitter.descriptor.subpass = -1;
		renderMode.blitter.process = true;
	#else
		renderMode.metadata["shaders"] = false;
		renderMode.blitter.process = false;
	#endif

		for ( auto& t : sceneTextures.voxels.id ) renderMode.blitter.material.textures.emplace_back().aliasTexture(t);
		for ( auto& t : sceneTextures.voxels.uv ) renderMode.blitter.material.textures.emplace_back().aliasTexture(t);
		for ( auto& t : sceneTextures.voxels.normal ) renderMode.blitter.material.textures.emplace_back().aliasTexture(t);
		for ( auto& t : sceneTextures.voxels.radiance ) renderMode.blitter.material.textures.emplace_back().aliasTexture(t);

		renderMode.blitter.material.textures.emplace_back().aliasTexture(sceneTextures.noise);
		renderMode.blitter.material.textures.emplace_back().aliasTexture(sceneTextures.skybox);

		renderMode.bindCallback( renderMode.CALLBACK_BEGIN, [&]( VkCommandBuffer commandBuffer ){
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
	}
#endif
}
void ext::VoxelizerBehavior::tick( uf::Object& self ) {
#if UF_USE_VULKAN
	if ( !this->hasComponent<uf::renderer::RenderTargetRenderMode>() ) return;

	auto& metadata = this->getComponent<ext::VoxelizerBehavior::Metadata>();
	auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
	
	auto& scene = uf::scene::getCurrentScene();
	auto& sceneTextures = scene.getComponent<pod::SceneTextures>();
	auto& controller = scene.getController();
	auto controllerTransform = uf::transform::flatten( controller.getComponent<uf::Camera>().getTransform() );
	renderMode.setTarget("");

	if ( renderMode.executed ) {
		if ( !metadata.initialized ) metadata.initialized = true;

		if ( metadata.renderer.limiter > 0 ) {
			if ( metadata.renderer.timer > metadata.renderer.limiter ) {
				metadata.renderer.timer = 0;
				renderMode.execute = true;
			} else {
				metadata.renderer.timer = metadata.renderer.timer + uf::physics::time::delta;
				renderMode.execute = false;
			}
		}
		if ( renderMode.execute ) {
			pod::Vector3f controllerPosition = controllerTransform.position;
			controllerPosition.x = floor(controllerPosition.x);
			controllerPosition.y = floor(controllerPosition.y);
			controllerPosition.z = -floor(controllerPosition.z);

			pod::Vector3f min = metadata.extents.min + controllerPosition;
			pod::Vector3f max = metadata.extents.max + controllerPosition;
			sceneTextures.voxels.matrix = uf::matrix::ortho<float>( min.x, max.x, min.y, max.y, min.z, max.z );
		
			auto graph = uf::scene::generateGraph();
			for ( auto entity : graph ) {
				if ( !entity->hasComponent<uf::Graphic>() ) continue;
				auto& graphic = entity->getComponent<uf::Graphic>();
				if ( graphic.material.hasShader("geometry", "vxgi") ) {
					auto& shader = graphic.material.getShader("geometry", "vxgi");

					auto& sceneTextures = scene.getComponent<pod::SceneTextures>();
					struct UniformDescriptor {
						alignas(16) pod::Matrix4f matrix;
					};
					auto& uniform = shader.getUniform("UBO");
					auto& uniforms = uniform.get<UniformDescriptor>();
					uniforms.matrix = sceneTextures.voxels.matrix;
					shader.updateUniform( "UBO", uniform );
				}
			}
		}
	}
#if COMP_SHADER_USED
	ext::ExtSceneBehavior::bindBuffers( scene, metadata.renderModeName, true );
	ext::ExtSceneBehavior::bindBuffers( scene );
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