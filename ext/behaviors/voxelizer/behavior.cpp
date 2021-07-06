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


UF_BEHAVIOR_REGISTER_CPP(ext::VoxelizerBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::VoxelizerBehavior, ticks = true, renders = false, multithread = false)
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
		const float DEFAULT_CASCADE_POWER = ext::config["engine"]["scenes"]["vxgi"]["cascadePower"].as<float>(1.5);
		const float DEFAULT_GRANULARITY = ext::config["engine"]["scenes"]["vxgi"]["granularity"].as<float>(2.0);
		const float DEFAULT_SHADOWS = ext::config["engine"]["scenes"]["vxgi"]["shadows"].as<size_t>(8);

		if ( metadata.voxelSize.x == 0 ) metadata.voxelSize.x = DEFAULT_VOXEL_SIZE;
		if ( metadata.voxelSize.y == 0 ) metadata.voxelSize.y = DEFAULT_VOXEL_SIZE;
		if ( metadata.voxelSize.z == 0 ) metadata.voxelSize.z = DEFAULT_VOXEL_SIZE;
		
		if ( metadata.renderer.limiter == 0 ) metadata.renderer.limiter = DEFAULT_VOXELIZE_LIMITER;

		if ( metadata.dispatchSize.x == 0 ) metadata.dispatchSize.x = DEFAULT_DISPATCH_SIZE;
		if ( metadata.dispatchSize.y == 0 ) metadata.dispatchSize.y = DEFAULT_DISPATCH_SIZE;
		if ( metadata.dispatchSize.z == 0 ) metadata.dispatchSize.z = DEFAULT_DISPATCH_SIZE;

		if ( metadata.cascades == 0 ) metadata.cascades = DEFAULT_CASCADES;
		if ( metadata.cascadePower == 0 ) metadata.cascadePower = DEFAULT_CASCADE_POWER;
		if ( metadata.granularity == 0 ) metadata.granularity = DEFAULT_GRANULARITY;
		if ( metadata.shadows == 0 ) metadata.shadows = DEFAULT_SHADOWS;

		metadata.extents.min = uf::vector::decode( ext::config["engine"]["scenes"]["vxgi"]["extents"]["min"], pod::Vector3f{-32, -32, -32} );
		metadata.extents.max = uf::vector::decode( ext::config["engine"]["scenes"]["vxgi"]["extents"]["max"], pod::Vector3f{ 32,  32,  32} );

	//	uf::stl::vector<uint8_t> empty(metadata.voxelSize.x * metadata.voxelSize.y * metadata.voxelSize.z * sizeof(uint8_t) * 4);	
		const bool HDR = false;
		for ( size_t i = 0; i < metadata.cascades; ++i ) {
			auto& id = sceneTextures.voxels.id.emplace_back();
		//	auto& uv = sceneTextures.voxels.uv.emplace_back();
			auto& normal = sceneTextures.voxels.normal.emplace_back();
			auto& radiance = sceneTextures.voxels.radiance.emplace_back();
		//	auto& depth = sceneTextures.voxels.depth.emplace_back();

			id.sampler.descriptor.filter.min = VK_FILTER_NEAREST;
			id.sampler.descriptor.filter.mag = VK_FILTER_NEAREST;

			id.fromBuffers( NULL, 0, uf::renderer::enums::Format::R16G16_UINT, metadata.voxelSize.x, metadata.voxelSize.y, metadata.voxelSize.z, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );
		//	uv.fromBuffers( NULL, 0, uf::renderer::enums::Format::R16G16_SFLOAT, metadata.voxelSize.x, metadata.voxelSize.y, metadata.voxelSize.z, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );
			normal.fromBuffers( NULL, 0, uf::renderer::enums::Format::R16G16_SFLOAT, metadata.voxelSize.x, metadata.voxelSize.y, metadata.voxelSize.z, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );
			radiance.fromBuffers( NULL, 0, HDR ? uf::renderer::enums::Format::R16G16B16A16_SFLOAT : uf::renderer::enums::Format::R8G8B8A8_UNORM, metadata.voxelSize.x, metadata.voxelSize.y, metadata.voxelSize.z, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );
		//	depth.fromBuffers( (void*) empty.data(), empty.size(), uf::renderer::enums::Format::R16_SFLOAT, metadata.voxelSize.x, metadata.voxelSize.y, metadata.voxelSize.z, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );
		}
	}
	// initialize render mode
	{
		if ( metadata.fragmentSize.x == 0 ) metadata.fragmentSize.x = metadata.voxelSize.x;
		if ( metadata.fragmentSize.y == 0 ) metadata.fragmentSize.y = metadata.voxelSize.y;

		auto& renderMode = this->getComponent<uf::renderer::RenderTargetRenderMode>();
		metadata.renderModeName = "VXGI:" + std::to_string((int) this->getUid());
		uf::renderer::addRenderMode( &renderMode, metadata.renderModeName );

		renderMode.metadata.type = "vxgi";
		renderMode.metadata.samples = 1;
		renderMode.metadata.subpasses = metadata.cascades;

		renderMode.blitter.device = &ext::vulkan::device;
		renderMode.width = metadata.fragmentSize.x;
		renderMode.height = metadata.fragmentSize.y;

		uf::stl::string computeShaderFilename = "/shaders/display/vxgi.comp.spv";
		if ( renderMode.metadata.samples > 1 ) {
			computeShaderFilename = uf::string::replace( computeShaderFilename, "frag", "msaa.frag" );
		}
		if ( uf::renderer::settings::experimental::deferredSampling ) {
			computeShaderFilename = uf::string::replace( computeShaderFilename, "frag", "deferredSampling.frag" );
		}
		renderMode.metadata.json["shaders"]["compute"] = computeShaderFilename;
		renderMode.blitter.descriptor.renderMode = metadata.renderModeName;
		renderMode.blitter.descriptor.subpass = -1;
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
			for ( auto& t : sceneTextures.voxels.depth ) vkCmdClearColorImage( commandBuffer, t.image, t.imageLayout, &clearColor, 1, &subresourceRange );
		});
		renderMode.bindCallback( renderMode.CALLBACK_END, [&]( VkCommandBuffer commandBuffer ){
			// parse voxel lighting
			if ( renderMode.blitter.initialized ) {
				auto& pipeline = renderMode.blitter.getPipeline();
				pipeline.record(renderMode.blitter, commandBuffer);
				vkCmdDispatch(commandBuffer, metadata.voxelSize.x / metadata.dispatchSize.x, metadata.voxelSize.y / metadata.dispatchSize.y, metadata.voxelSize.z / metadata.dispatchSize.z);
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
					graphic.updateBuffer( uniforms, shader.getUniformBuffer("UBO") );
				#endif
				}
			}
		}
	}
	ext::ExtSceneBehavior::bindBuffers( scene, metadata.renderModeName, true );
	ext::ExtSceneBehavior::bindBuffers( scene );
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
void ext::VoxelizerBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ){}
void ext::VoxelizerBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ){}
#undef this