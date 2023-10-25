#if UF_USE_VULKAN

#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/rendermodes/deferred.h>
#include <uf/ext/vulkan/rendermodes/rendertarget.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/window/window.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/ext/vulkan/graphic.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/math/transform.h>

#include <uf/ext/vulkan/graphic.h>
#include <uf/engine/graph/graph.h>
#include <uf/ext/ext.h>

#define BARYCENTRIC 1
#if BARYCENTRIC
	#ifndef BARYCENTRIC_CALCULATE
		#define BARYCENTRIC_CALCULATE 0
	#endif
#endif

namespace {
	const uf::stl::string DEFERRED_MODE = "compute";
	ext::vulkan::Texture depthPyramid;
	uf::stl::vector<VkImageView> depthPyramidViews;
}

#include "./transition.inl"

const uf::stl::string ext::vulkan::DeferredRenderMode::getType() const {
	return "Deferred";
}

void ext::vulkan::DeferredRenderMode::initialize( Device& device ) {
	ext::vulkan::RenderMode::initialize( device );
	
	uint32_t width = this->width > 0 ? this->width : (ext::vulkan::settings::width * this->scale);
	uint32_t height = this->height > 0 ? this->height : (ext::vulkan::settings::height * this->scale);

	renderTarget.device = &device;
	renderTarget.views = metadata.eyes;
	size_t msaa = ext::vulkan::settings::msaa;

	struct {
		size_t id, bary, depth, uv, normal;
		size_t color, bright, motion, scratch, output;
		size_t depthPyramid;
	} attachments = {};

	bool blend = true; // !ext::vulkan::settings::invariant::deferredSampling;

	// input g-buffers
	attachments.id = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format = */VK_FORMAT_R32G32_UINT,
		/*.layout = */VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
		/*.blend = */false,
		/*.samples = */msaa,
	});

#if BARYCENTRIC
	#if !BARYCENTRIC_CALCULATE
		attachments.bary = renderTarget.attach(RenderTarget::Attachment::Descriptor{
			/*.format = */VK_FORMAT_R16G16_SFLOAT,
			/*.layout = */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
			/*.blend = */false,
			/*.samples = */msaa,
		});
	#endif
#else
	attachments.uv = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format = */VK_FORMAT_R16G16B16A16_SFLOAT,
		/*.layout = */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
		/*.blend = */false,
		/*.samples = */msaa,
	});
	attachments.normal = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format = */VK_FORMAT_R16G16B16A16_SFLOAT,
		/*.layout = */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
		/*.blend = */false,
		/*.samples = */msaa,
	});
#endif
	attachments.depth = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format = */ext::vulkan::settings::formats::depth,
		/*.layout = */VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		/*.usage = */VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		/*.blend = */false,
		/*.samples = */msaa,
		//*.mips = */1,
	});
	// output buffers
	attachments.color = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format =*/ ext::vulkan::settings::pipelines::hdr ? enums::Format::HDR : enums::Format::SDR,
		/*.layout = */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		/*.usage =*/ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		/*.blend =*/ blend,
		/*.samples =*/ 1,
	});
	attachments.bright = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format =*/ ext::vulkan::settings::pipelines::hdr ? enums::Format::HDR : enums::Format::SDR,
		/*.layout = */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		/*.usage =*/ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		/*.blend =*/ blend,
		/*.samples =*/ 1,
	});
	attachments.scratch = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format =*/ ext::vulkan::settings::pipelines::hdr ? enums::Format::HDR : enums::Format::SDR,
		/*.layout = */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		/*.usage =*/ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		/*.blend =*/ blend,
		/*.samples =*/ 1,
	});
	attachments.motion = renderTarget.attach(RenderTarget::Attachment::Descriptor{
	//	/*.format = */VK_FORMAT_R32G32B32A32_SFLOAT,
		/*.format = */VK_FORMAT_R16G16_SFLOAT,
		/*.layout = */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		/*.blend = */false,
		/*.samples = */1,
	});
	attachments.depthPyramid = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format = */VK_FORMAT_R32_SFLOAT,
		/*.layout = */ VK_IMAGE_LAYOUT_GENERAL,
		/*.usage = */ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		/*.blend = */false,
		/*.samples = */1,
		/*.mips = */1
	});

	metadata.attachments["id"] = attachments.id;

#if BARYCENTRIC
	#if !BARYCENTRIC_CALCULATE
		metadata.attachments["bary"] = attachments.bary;
	#endif
#else
	metadata.attachments["uv"] = attachments.uv;
	metadata.attachments["normal"] = attachments.normal;
#endif
	
	metadata.attachments["depth"] = attachments.depth;
	metadata.attachments["depthPyramid"] = attachments.depthPyramid;
	metadata.attachments["color"] = attachments.color;
	metadata.attachments["bright"] = attachments.bright;
	metadata.attachments["scratch"] = attachments.scratch;
	metadata.attachments["motion"] = attachments.motion;

	metadata.attachments["output"] = attachments.color;

	// First pass: fill the G-Buffer
	for ( size_t eye = 0; eye < metadata.eyes; ++eye ) {
		renderTarget.addPass(
			/*.*/ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		#if BARYCENTRIC
			#if !BARYCENTRIC_CALCULATE
				/*.colors =*/ { attachments.id, attachments.bary },
			#else
				/*.colors =*/ { attachments.id },
			#endif
		#else
			/*.colors =*/ { attachments.id, attachments.uv, attachments.normal },
		#endif
			/*.inputs =*/ {},
			/*.resolve =*/{},
			/*.depth = */ attachments.depth,
			/*.layer = */eye,
			/*.autoBuildPipeline =*/ true
		);
	}
	if ( DEFERRED_MODE == "fragment" ) {
		for ( size_t eye = 0; eye < metadata.eyes; ++eye ) {
			renderTarget.addPass(
				/*.*/ VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
				/*.colors =*/ { attachments.color, attachments.bright, attachments.motion },
			#if BARYCENTRIC
				#if !BARYCENTRIC_CALCULATE
					/*.inputs =*/ { attachments.id, attachments.depth, attachments.bary },
				#else
					/*.inputs =*/ { attachments.id, attachments.depth },
				#endif
			#else
				/*.inputs =*/ { attachments.id, attachments.depth, attachments.uv, attachments.normal },
			#endif
				/*.resolve =*/{},
				/*.depth = */attachments.depth,
				/*.layer = */eye,
				/*.autoBuildPipeline =*/ false
			);
		}
	}

	// metadata.outputs.emplace_back(metadata.attachments["output"]);
	renderTarget.initialize( device );

	{
		uf::Mesh mesh;
		mesh.vertex.count = 3;

		auto& scene = uf::scene::getCurrentScene();
		auto& sceneMetadataJson = scene.getComponent<uf::Serializer>();

		blitter.descriptor.renderMode = "Swapchain";
		blitter.descriptor.subpass = 0;
		blitter.descriptor.depth.test = false;
		blitter.descriptor.depth.write = false;

		blitter.initialize( "Swapchain" );
		blitter.initializeMesh( mesh );

		{
			uf::stl::string vertexShaderFilename = uf::io::root+"/shaders/display/renderTarget/vert.spv";
			uf::stl::string fragmentShaderFilename = uf::io::root+"/shaders/display/renderTarget/frag.spv";

			uf::stl::string postProcess = ::fmt::format("{}.frag", metadata.json["postProcess"].as<uf::stl::string>("postProcess"));
			{
				std::pair<bool, uf::stl::string> settings[] = {
					{ settings::pipelines::postProcess /*&& !settings::pipelines::rt*/, postProcess },
				//	{ msaa > 1, "msaa.frag" },
				};
				FOR_ARRAY( settings ) if ( settings[i].first ) fragmentShaderFilename = uf::string::replace( fragmentShaderFilename, "frag", settings[i].second );
			}
			blitter.material.initializeShaders({
				{uf::io::resolveURI(vertexShaderFilename), uf::renderer::enums::Shader::VERTEX},
				{uf::io::resolveURI(fragmentShaderFilename), uf::renderer::enums::Shader::FRAGMENT}
			});

			auto& shader = blitter.material.getShader("fragment");
			if ( !settings::pipelines::fsr ) {
				shader.aliasAttachment("output", this);
			}
		}

		if ( settings::pipelines::deferred ) {
			if ( DEFERRED_MODE == "compute" ) {
				uf::stl::string computeShaderFilename = "comp.spv"; {
					std::pair<bool, uf::stl::string> settings[] = {
						{ uf::renderer::settings::pipelines::rt, "rt.comp" },
						{ uf::renderer::settings::pipelines::vxgi, "vxgi.comp" },
						{ msaa > 1, "msaa.comp" },
					};
					FOR_ARRAY( settings ) if ( settings[i].first ) computeShaderFilename = uf::string::replace( computeShaderFilename, "comp", settings[i].second );
				}
				computeShaderFilename = uf::io::root+"/shaders/display/deferred/comp/" + computeShaderFilename;
				blitter.material.attachShader(uf::io::resolveURI(computeShaderFilename), uf::renderer::enums::Shader::COMPUTE, "deferred");
				UF_MSG_DEBUG("Using deferred shader: {}", computeShaderFilename);
			} else if ( DEFERRED_MODE == "fragment" )  {
				uf::stl::string vertexShaderFilename = "vert.spv";
				uf::stl::string fragmentShaderFilename = "frag.spv"; {
					std::pair<bool, uf::stl::string> settings[] = {
						{ uf::renderer::settings::pipelines::rt, "rt.frag" },
						{ uf::renderer::settings::pipelines::vxgi, "vxgi.frag" },
						{ msaa > 1, "msaa.frag" },
					};
					FOR_ARRAY( settings ) if ( settings[i].first ) fragmentShaderFilename = uf::string::replace( fragmentShaderFilename, "frag", settings[i].second );
				}
				fragmentShaderFilename = uf::io::root+"/shaders/display/deferred/frag/" + fragmentShaderFilename;
				blitter.material.attachShader(uf::io::resolveURI(fragmentShaderFilename), uf::renderer::enums::Shader::FRAGMENT, "deferred");
				UF_MSG_DEBUG("Using deferred shader: {}", fragmentShaderFilename);
			}

			auto& shader = blitter.material.getShader(DEFERRED_MODE, "deferred");

			size_t maxLights = ext::config["engine"]["scenes"]["lights"]["max"].as<size_t>(512);
			size_t maxTextures2D = ext::config["engine"]["scenes"]["textures"]["max"]["2D"].as<size_t>(512);
			size_t maxTexturesCube = ext::config["engine"]["scenes"]["textures"]["max"]["cube"].as<size_t>(128);
			size_t maxTextures3D = ext::config["engine"]["scenes"]["textures"]["max"]["3D"].as<size_t>(128);
			size_t maxCascades = ext::config["engine"]["scenes"]["vxgi"]["cascades"].as<size_t>(16);

			shader.setSpecializationConstants({
				{ "TEXTURES", maxTextures2D },
				{ "CUBEMAPS", maxTexturesCube },
				{ "CASCADES", maxCascades },
			});
			shader.setDescriptorCounts({
				{ "samplerTextures", maxTextures2D },
				{ "samplerCubemaps", maxTexturesCube },
				{ "voxelId", maxCascades },
				{ "voxelUv", maxCascades },
				{ "voxelNormal", maxCascades },
				{ "voxelRadiance", maxCascades },
			});

		//	shader.buffers.emplace_back( uf::graph::storage.buffers.camera.alias() );
		//	shader.buffers.emplace_back( uf::graph::storage.buffers.joint.alias() );
			shader.buffers.emplace_back( uf::graph::storage.buffers.drawCommands.alias() );
			shader.buffers.emplace_back( uf::graph::storage.buffers.instance.alias() );
			shader.buffers.emplace_back( uf::graph::storage.buffers.instanceAddresses.alias() );
			shader.buffers.emplace_back( uf::graph::storage.buffers.material.alias() );
			shader.buffers.emplace_back( uf::graph::storage.buffers.texture.alias() );
			shader.buffers.emplace_back( uf::graph::storage.buffers.light.alias() );

			shader.aliasAttachment("id", this);
		#if BARYCENTRIC
			#if !BARYCENTRIC_CALCULATE
				shader.aliasAttachment("bary", this);
			#endif
		#else
			shader.aliasAttachment("uv", this);
			shader.aliasAttachment("normal", this);
		#endif
			shader.aliasAttachment("depth", this);
			
			shader.aliasAttachment("color", this, VK_IMAGE_LAYOUT_GENERAL);
			shader.aliasAttachment("bright", this, VK_IMAGE_LAYOUT_GENERAL);
			shader.aliasAttachment("motion", this, VK_IMAGE_LAYOUT_GENERAL);
		}
		
		if ( settings::pipelines::bloom ) {
			uf::stl::string computeShaderFilename = uf::io::resolveURI(uf::io::root+"/shaders/display/bloom/comp.spv");
			blitter.material.attachShader(computeShaderFilename, uf::renderer::enums::Shader::COMPUTE, "bloom");

			auto& shader = blitter.material.getShader("compute", "bloom");
			shader.aliasAttachment("color", this, VK_IMAGE_LAYOUT_GENERAL);
			shader.aliasAttachment("bright", this, VK_IMAGE_LAYOUT_GENERAL);
			shader.aliasAttachment("scratch", this, VK_IMAGE_LAYOUT_GENERAL);
		}

		if ( false && settings::pipelines::culling ) {
			uf::stl::string computeShaderFilename = uf::io::resolveURI(uf::io::root+"/shaders/display/depth-pyramid/comp.spv");
			blitter.material.attachShader(computeShaderFilename, uf::renderer::enums::Shader::COMPUTE, "depth-pyramid");

			auto& shader = blitter.material.getShader("compute", "depth-pyramid");
			auto attachment = this->getAttachment("depth");
			auto mips = uf::vector::mips( pod::Vector2ui{ width, height } );
			shader.setSpecializationConstants({
				{ "MIPS", mips - 1 },
			});
			shader.setDescriptorCounts({
				{ "inImage", mips - 1 },
				{ "outImage", mips - 1 },
			});

			shader.aliasAttachment("depth", this);

			ext::vulkan::Texture2D source; source.aliasAttachment( this->getAttachment("depthPyramid") );
			source.sampler.descriptor.reduction.enabled = true;
			source.sampler.descriptor.reduction.mode = VK_SAMPLER_REDUCTION_MODE_MIN;
			
			for ( auto& view : ::depthPyramidViews ) {
				vkDestroyImageView(device.logicalDevice, view, nullptr);
			}
			::depthPyramidViews.clear();
			::depthPyramidViews.resize(mips-1);
			shader.textures.clear();
			
			for ( auto i = 1; i < mips; ++i ) {
				auto& view = ::depthPyramidViews[i-1];
				VkImageViewCreateInfo viewCreateInfo = {};
				viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				viewCreateInfo.pNext = NULL;
				viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
				viewCreateInfo.subresourceRange.baseMipLevel = i;
				viewCreateInfo.subresourceRange.layerCount = 1;
				viewCreateInfo.subresourceRange.levelCount = 1;
				viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				viewCreateInfo.viewType = source.viewType;
				viewCreateInfo.format = source.format;
				viewCreateInfo.image = source.image;
				VK_CHECK_RESULT(vkCreateImageView(device.logicalDevice, &viewCreateInfo, nullptr, &view));

				if ( i + 1 < mips ) {
					auto& texture = shader.textures.emplace_back();
					texture.aliasTexture( source );
					texture.view = view;
					texture.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
					texture.updateDescriptors();
				}
			}
			for ( auto i = 1; i < mips; ++i ) {
				auto& view = ::depthPyramidViews[i-1];
				auto& texture = shader.textures.emplace_back();
				texture.aliasTexture( source );
				texture.view = view;
				texture.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
				texture.updateDescriptors();
			}
		}
	
		if ( !blitter.hasPipeline( blitter.descriptor ) ){
			blitter.initializePipeline( blitter.descriptor );
		}
		
		{
			ext::vulkan::GraphicDescriptor descriptor = blitter.descriptor;
			descriptor.renderMode = "";
			descriptor.bind.width = width;
			descriptor.bind.height = height;
			descriptor.bind.depth = 1;
			
			if ( settings::pipelines::deferred && blitter.material.hasShader(DEFERRED_MODE, "deferred") ) {
				descriptor.pipeline = "deferred";
				if ( DEFERRED_MODE == "fragment" ) {
					descriptor.subpass = 1;
					descriptor.bind.point = VK_PIPELINE_BIND_POINT_GRAPHICS;
				} else if ( DEFERRED_MODE == "compute" ) {
					descriptor.subpass = 0;
					descriptor.bind.point = VK_PIPELINE_BIND_POINT_COMPUTE;
				}
				if ( !blitter.hasPipeline( descriptor ) ) {
					blitter.initializePipeline( descriptor );
				}
			}

			if ( settings::pipelines::bloom ) {
				descriptor.pipeline = "bloom";
				descriptor.subpass = 0;
				descriptor.bind.point = VK_PIPELINE_BIND_POINT_COMPUTE;
				if ( !blitter.hasPipeline( descriptor ) ) {
					blitter.initializePipeline( descriptor );
				}
			}

			if ( false && settings::pipelines::culling ) {
				descriptor.aux = uf::vector::mips( pod::Vector2ui{ width, height } );
				descriptor.pipeline = "depth-pyramid";
				descriptor.subpass = 0;
				descriptor.bind.point = VK_PIPELINE_BIND_POINT_COMPUTE;
				if ( !blitter.hasPipeline( descriptor ) ) {
					blitter.initializePipeline( descriptor );
				}
			}
		}
	}
}
void ext::vulkan::DeferredRenderMode::tick() {
	ext::vulkan::RenderMode::tick();
	
	bool resized = (this->width == 0 && this->height == 0 && ext::vulkan::states::resized) || this->resized;
	bool rebuild = resized || ext::vulkan::states::rebuild || this->rebuild;

	uint32_t width = this->width > 0 ? this->width : (ext::vulkan::settings::width * this->scale);
	uint32_t height = this->height > 0 ? this->height : (ext::vulkan::settings::height * this->scale);

	if ( resized ) {
		this->resized = false;
		rebuild = true;
		renderTarget.initialize( *renderTarget.device );

		if ( false && settings::pipelines::culling ) {
			auto& shader = blitter.material.getShader("compute", "depth-pyramid");
			auto attachment = this->getAttachment("depth");
			auto mips = uf::vector::mips( pod::Vector2ui{ width, height } );
			shader.setSpecializationConstants({
				{ "MIPS", mips - 1 },
			});
			shader.setDescriptorCounts({
				{ "inImage", mips - 1 },
				{ "outImage", mips - 1 },
			});

			ext::vulkan::Texture2D source; source.aliasAttachment( this->getAttachment("depthPyramid") );
			source.sampler.descriptor.reduction.enabled = true;
			source.sampler.descriptor.reduction.mode = VK_SAMPLER_REDUCTION_MODE_MIN;
			
			for ( auto& view : ::depthPyramidViews ) {
				vkDestroyImageView(device->logicalDevice, view, nullptr);
			}
			::depthPyramidViews.clear();
			::depthPyramidViews.resize(mips-1);
			shader.textures.clear();
			
			for ( auto i = 1; i < mips; ++i ) {
				auto& view = ::depthPyramidViews[i-1];
				VkImageViewCreateInfo viewCreateInfo = {};
				viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				viewCreateInfo.pNext = NULL;
				viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
				viewCreateInfo.subresourceRange.baseMipLevel = i;
				viewCreateInfo.subresourceRange.layerCount = 1;
				viewCreateInfo.subresourceRange.levelCount = 1;
				viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				viewCreateInfo.viewType = source.viewType;
				viewCreateInfo.format = source.format;
				viewCreateInfo.image = source.image;
				VK_CHECK_RESULT(vkCreateImageView(device->logicalDevice, &viewCreateInfo, nullptr, &view));

				if ( i + 1 < mips ) {
					auto& texture = shader.textures.emplace_back();
					texture.aliasTexture( source );
					texture.view = view;
					texture.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
					texture.updateDescriptors();
				}
			}
			for ( auto i = 1; i < mips; ++i ) {
				auto& view = ::depthPyramidViews[i-1];
				auto& texture = shader.textures.emplace_back();
				texture.aliasTexture( source );
				texture.view = view;
				texture.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
				texture.updateDescriptors();
			}
		}
	}
	// update blitter descriptor set
	if ( rebuild && blitter.initialized ) {
		if ( true ) {
			auto& shader = blitter.material.getShader(DEFERRED_MODE, "deferred");

			auto moved = std::move( shader.buffers );
			for ( auto& buffer : moved ) {
				if ( buffer.aliased ) continue;
				shader.buffers.emplace_back( buffer );
				buffer.aliased = true;
			}

			shader.buffers.emplace_back( uf::graph::storage.buffers.drawCommands.alias() );
			shader.buffers.emplace_back( uf::graph::storage.buffers.instance.alias() );
			shader.buffers.emplace_back( uf::graph::storage.buffers.instanceAddresses.alias() );
			shader.buffers.emplace_back( uf::graph::storage.buffers.material.alias() );
			shader.buffers.emplace_back( uf::graph::storage.buffers.texture.alias() );
			shader.buffers.emplace_back( uf::graph::storage.buffers.light.alias() );
		}
		
		if ( blitter.hasPipeline( blitter.descriptor ) ){
			blitter.getPipeline( blitter.descriptor ).update( blitter, blitter.descriptor );
		} else {
			blitter.initializePipeline( blitter.descriptor );
		}

		{
			ext::vulkan::GraphicDescriptor descriptor = blitter.descriptor;
			descriptor.renderMode = "";
			descriptor.bind.width = width;
			descriptor.bind.height = height;
			descriptor.bind.depth = 1;

			if ( settings::pipelines::deferred && blitter.material.hasShader(DEFERRED_MODE, "deferred") ) {
				descriptor.pipeline = "deferred";
				if ( DEFERRED_MODE == "fragment" ) {
					descriptor.subpass = 1;
					descriptor.bind.point = VK_PIPELINE_BIND_POINT_GRAPHICS;
				} else if ( DEFERRED_MODE == "compute" ) {
					descriptor.subpass = 0;
					descriptor.bind.point = VK_PIPELINE_BIND_POINT_COMPUTE;
				}
				if ( blitter.hasPipeline( descriptor ) ) {
					blitter.getPipeline( descriptor ).update( blitter, descriptor );
				} else {
					blitter.initializePipeline( descriptor );
				}
			}

			if ( settings::pipelines::bloom ) {
				descriptor.pipeline = "bloom";
				descriptor.subpass = 0;
				descriptor.bind.point = VK_PIPELINE_BIND_POINT_COMPUTE;
				if ( blitter.hasPipeline( descriptor ) ) {
					blitter.getPipeline( descriptor ).update( blitter, descriptor );
				} else {
					blitter.initializePipeline( descriptor );
				}
			}

			if ( false && settings::pipelines::culling ) {
				descriptor.aux = uf::vector::mips( pod::Vector2ui{ width, height } );
				descriptor.pipeline = "depth-pyramid";
				descriptor.subpass = 0;
				descriptor.bind.point = VK_PIPELINE_BIND_POINT_COMPUTE;
				if ( blitter.hasPipeline( descriptor ) ) {
					blitter.getPipeline( descriptor ).update( blitter, descriptor );
				} else {
					blitter.initializePipeline( descriptor );
				}
			}
		}
	}
}
VkSubmitInfo ext::vulkan::DeferredRenderMode::queue() {
	//lockMutex( this->mostRecentCommandPoolId );
	auto& commands = getCommands( this->mostRecentCommandPoolId );
	// Pipeline stage at which the queue submission will wait (via pWaitSemaphores)
	static VkPipelineStageFlags waitStageMask[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	// The submit info structure specifices a command buffer queue submission batch
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pWaitDstStageMask = waitStageMask;									// Pointer to the list of pipeline stages that the semaphore waits will occur at
	submitInfo.pWaitSemaphores = &swapchain.presentCompleteSemaphore;				// Semaphore(s) to wait upon before the submitted command buffer starts executing
	submitInfo.waitSemaphoreCount = 1;												// One wait semaphore																				
	submitInfo.pSignalSemaphores = &renderCompleteSemaphore;						// Semaphore(s) to be signaled when command buffers have completed
	submitInfo.signalSemaphoreCount = 1;											// One signal semaphore
	submitInfo.pCommandBuffers = &commands[states::currentBuffer];					// Command buffers(s) to execute in this batch (submission)
	submitInfo.commandBufferCount = 1;

	return submitInfo;
}
void ext::vulkan::DeferredRenderMode::render() {
//	if ( this->executed ) return;

	if ( commandBufferCallbacks.count(EXECUTE_BEGIN) > 0 ) commandBufferCallbacks[EXECUTE_BEGIN]( VkCommandBuffer{}, 0 );

	//lockMutex( this->mostRecentCommandPoolId );
	auto& commands = getCommands( this->mostRecentCommandPoolId );
	// Submit commands
	// Use a fence to ensure that command buffer has finished executing before using it again
	VK_CHECK_RESULT(vkWaitForFences( *device, 1, &fences[states::currentBuffer], VK_TRUE, VK_DEFAULT_FENCE_TIMEOUT ));
	VK_CHECK_RESULT(vkResetFences( *device, 1, &fences[states::currentBuffer] ));

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pWaitDstStageMask = NULL; 								// Pointer to the list of pipeline stages that the semaphore waits will occur at
	submitInfo.pWaitSemaphores = NULL;									// Semaphore(s) to wait upon before the submitted command buffer starts executing
	submitInfo.waitSemaphoreCount = 0;									// One wait semaphore																				
	submitInfo.pSignalSemaphores = NULL;								// Semaphore(s) to be signaled when command buffers have completed
	submitInfo.signalSemaphoreCount = 0;								// One signal semaphore
	submitInfo.pCommandBuffers = &commands[states::currentBuffer];		// Command buffers(s) to execute in this batch (submission)
	submitInfo.commandBufferCount = 1;

//	VK_CHECK_RESULT(vkQueueSubmit(device->getQueue( QueueEnum::GRAPHICS ), 1, &submitInfo, fences[states::currentBuffer]));
	VkQueue queue = device->getQueue( QueueEnum::GRAPHICS );
	VkResult res = vkQueueSubmit( queue, 1, &submitInfo, fences[states::currentBuffer]);
	VK_CHECK_QUEUE_CHECKPOINT( queue, res );
	if ( commandBufferCallbacks.count(EXECUTE_END) > 0 ) commandBufferCallbacks[EXECUTE_END]( VkCommandBuffer{}, 0 );

	this->executed = true;
	//unlockMutex( this->mostRecentCommandPoolId );
}
void ext::vulkan::DeferredRenderMode::destroy() {
	ext::vulkan::RenderMode::destroy();
	blitter.destroy();
}

ext::vulkan::GraphicDescriptor ext::vulkan::DeferredRenderMode::bindGraphicDescriptor( const ext::vulkan::GraphicDescriptor& reference, size_t pass ) {
	ext::vulkan::GraphicDescriptor descriptor = ext::vulkan::RenderMode::bindGraphicDescriptor(reference, pass);
	if ( descriptor.renderMode != "" ) descriptor.invalidated = true;
	return descriptor;
}
void ext::vulkan::DeferredRenderMode::createCommandBuffers( const uf::stl::vector<ext::vulkan::Graphic*>& graphics ) {
	uint32_t width = this->width > 0 ? this->width : (ext::vulkan::settings::width * this->scale);
	uint32_t height = this->height > 0 ? this->height : (ext::vulkan::settings::height * this->scale);

	VkCommandBufferBeginInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufInfo.pNext = nullptr;

	VkImageMemoryBarrier imageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device->queueFamilyIndices.graphics; //VK_QUEUE_FAMILY_IGNORED
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device->queueFamilyIndices.graphics; //VK_QUEUE_FAMILY_IGNORED
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemoryBarrier.subresourceRange.layerCount = 1;
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	uf::stl::vector<RenderMode*> layers = ext::vulkan::getRenderModes(uf::stl::vector<uf::stl::string>{"RenderTarget", "Compute"}, false);
	
	auto& scene = uf::scene::getCurrentScene();
	auto& sceneMetadataJson = scene.getComponent<uf::Serializer>();

	auto& commands = getCommands();
//	auto& swapchainRender = ext::vulkan::getRenderMode("Swapchain");
	uf::stl::vector<VkClearValue> clearValues;
	for ( auto& attachment : renderTarget.attachments ) {
		pod::Vector4f clearColor = uf::vector::decode( sceneMetadataJson["system"]["renderer"]["clear values"][(int) clearValues.size()], pod::Vector4f{0, 0, 0, 0} );
		auto& clearValue = clearValues.emplace_back();
		if ( attachment.descriptor.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ) {
			clearValue.color.float32[0] = clearColor[0];
			clearValue.color.float32[1] = clearColor[1];
			clearValue.color.float32[2] = clearColor[2];
			clearValue.color.float32[3] = clearColor[3];
		} else if ( attachment.descriptor.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ) {
			if ( uf::matrix::reverseInfiniteProjection ) {
				clearValue.depthStencil = { 0.0f, 0 };
			} else {
				clearValue.depthStencil = { 1.0f, 0 };
			}
		}
	}
	bool shouldRecord = true; // ( settings::pipelines::rt && !ext::config["engine"]["scenes"]["rt"]["full"].as<bool>() ) || !settings::pipelines::rt;
	for (size_t i = 0; i < commands.size(); ++i) {
		auto commandBuffer = commands[i];
		VK_CHECK_RESULT( vkBeginCommandBuffer(commandBuffer, &cmdBufInfo) );
		device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::BEGIN, "begin" );

		// Fill GBuffer
		{
			VkRenderPassBeginInfo renderPassBeginInfo = {};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.pNext = nullptr;
			renderPassBeginInfo.renderArea.offset.x = 0;
			renderPassBeginInfo.renderArea.offset.y = 0;
			renderPassBeginInfo.renderArea.extent.width = width;
			renderPassBeginInfo.renderArea.extent.height = height;
			renderPassBeginInfo.clearValueCount = clearValues.size();
			renderPassBeginInfo.pClearValues = &clearValues[0];
			renderPassBeginInfo.renderPass = renderTarget.renderPass;
			renderPassBeginInfo.framebuffer = renderTarget.framebuffers[i];

			// Update dynamic viewport state
			VkViewport viewport = {};
			viewport.width = (float) width;
			viewport.height = (float) height;
			viewport.minDepth = (float) 0.0f;
			viewport.maxDepth = (float) 1.0f;
			
			// Update dynamic scissor state
			VkRect2D scissor = {};
			scissor.extent.width = width;
			scissor.extent.height = height;
			scissor.offset.x = 0;
			scissor.offset.y = 0;
			
			size_t currentSubpass = 0;

		/*
			// transition layers for read
			for ( auto layer : layers ) {
				layer->pipelineBarrier( commandBuffer, 0 );
			}
		*/
		// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		// VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL

		// VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
		// VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL

		#if 1
			for ( auto& attachment : renderTarget.attachments ) {
				// transition attachments to general attachments for imageStore
				VkImageSubresourceRange subresourceRange;
				subresourceRange.baseMipLevel = 0;
				subresourceRange.baseArrayLayer = 0;
				subresourceRange.levelCount = attachment.descriptor.mips;
				subresourceRange.layerCount = renderTarget.views;
				subresourceRange.aspectMask = attachment.descriptor.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "setImageLayout" );
				uf::renderer::Texture::setImageLayout( commandBuffer, attachment.image, VK_IMAGE_LAYOUT_UNDEFINED, attachment.descriptor.layout, subresourceRange );
			}
		#endif

			for ( auto& pipeline : metadata.pipelines ) {
				if ( pipeline == metadata.pipeline ) continue;
				for ( auto graphic : graphics ) {
					if ( graphic->descriptor.renderMode != this->getName() ) continue;
					ext::vulkan::GraphicDescriptor descriptor = bindGraphicDescriptor(graphic->descriptor, currentSubpass);
					descriptor.pipeline = pipeline;
					if ( pipeline == uf::renderer::settings::pipelines::names::culling ) {
						descriptor.bind.width = graphic->descriptor.inputs.indirect.count;
						descriptor.bind.height = 1;
						descriptor.bind.depth = 1;
						descriptor.bind.point = VK_PIPELINE_BIND_POINT_COMPUTE;
					} else if ( pipeline == "skinning" ) {
						descriptor.bind.width = graphic->descriptor.inputs.vertex.count;
						descriptor.bind.height = 1;
						descriptor.bind.depth = 1;
						descriptor.bind.point = VK_PIPELINE_BIND_POINT_COMPUTE;
					}
					device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, ::fmt::format("graphic[{}]", pipeline) );
					graphic->record( commandBuffer, descriptor, 0, metadata.eyes );
				}
			}

			// pre-renderpass commands
			if ( commandBufferCallbacks.count(CALLBACK_BEGIN) > 0 ) {
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "callback[begin]" );
				commandBufferCallbacks[CALLBACK_BEGIN]( commandBuffer, i );
			}

			device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::BEGIN, "renderPass[begin]" ) ;
			vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
				vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
				vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
				// render to geometry buffers
				for ( size_t eye = 0; eye < metadata.eyes; ++eye ) {		
					size_t currentPass = 0;
					size_t currentDraw = 0;
					for ( auto graphic : graphics ) {
						// only draw graphics that are assigned to this type of render mode
						if ( graphic->descriptor.renderMode != this->getName() ) continue;
						ext::vulkan::GraphicDescriptor descriptor = bindGraphicDescriptor(graphic->descriptor, currentSubpass);
						device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, ::fmt::format("graphic[{}]", currentDraw) );
						graphic->record( commandBuffer, descriptor, eye, currentDraw++ );
					}
					if ( eye + 1 < metadata.eyes ) {
						device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "nextSubpass" );
						vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE); ++currentSubpass;
					}
				}
				// skip deferred pass if RT is enabled, we still process geometry for a depth buffer
				if ( DEFERRED_MODE == "fragment" ) for ( size_t eye = 0; eye < metadata.eyes; ++eye ) {		
					size_t currentPass = 0;
					size_t currentDraw = 0;
					{
						ext::vulkan::GraphicDescriptor descriptor = blitter.descriptor; // = bindGraphicDescriptor(blitter.descriptor, currentSubpass);
						descriptor.renderMode = "";
						descriptor.pipeline = "deferred";
						descriptor.subpass = currentSubpass;
						descriptor.bind.point = VK_PIPELINE_BIND_POINT_GRAPHICS;
						device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "deferred" );
						blitter.record(commandBuffer, descriptor, eye, currentDraw++);
					}
					if ( eye + 1 < metadata.eyes ) {
						device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "nextSubpass" );
						vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE); ++currentSubpass;
					}
				}
			device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::END, "renderPass[end]" );
			vkCmdEndRenderPass(commandBuffer);

		#if 1
			if ( settings::pipelines::deferred && DEFERRED_MODE == "compute" && blitter.material.hasShader(DEFERRED_MODE, "deferred") ) {
				auto& shader = blitter.material.getShader(DEFERRED_MODE, "deferred");
				ext::vulkan::GraphicDescriptor descriptor = blitter.descriptor;
				descriptor.renderMode = "";
				descriptor.pipeline = "deferred";
				descriptor.bind.width = width;
				descriptor.bind.height = height;
				descriptor.bind.depth = metadata.eyes;
				descriptor.bind.point = VK_PIPELINE_BIND_POINT_COMPUTE;
				descriptor.subpass = 0;

				// transition attachments to general attachments for imageStore
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "setImageLayout" );
				::transitionAttachmentsTo( this, shader, commandBuffer );

				// dispatch compute shader				
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "deferred" );
				blitter.record(commandBuffer, descriptor, 0, 0);

				// transition attachments back to shader read layouts
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "setImageLayout" );
				::transitionAttachmentsFrom( this, shader, commandBuffer );
			}

			if ( settings::pipelines::bloom && blitter.material.hasShader("compute", "bloom") ) {
				auto& shader = blitter.material.getShader("compute", "bloom");
				ext::vulkan::GraphicDescriptor descriptor = blitter.descriptor;
				descriptor.renderMode = "";
				descriptor.pipeline = "bloom";
				descriptor.bind.width = width;
				descriptor.bind.height = height;
				descriptor.bind.depth = metadata.eyes;
				descriptor.bind.point = VK_PIPELINE_BIND_POINT_COMPUTE;
				descriptor.subpass = 0;

				// transition attachments to general attachments for imageStore
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "setImageLayout" );
				::transitionAttachmentsTo( this, shader, commandBuffer );

				// dispatch compute shader				
				VkMemoryBarrier memoryBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
				memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				vkCmdPipelineBarrier( commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_FLAGS_NONE, 1, &memoryBarrier, 0, NULL, 0, NULL );
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "bloom[1]" );
				blitter.record(commandBuffer, descriptor, 0, 1);
				vkCmdPipelineBarrier( commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_FLAGS_NONE, 1, &memoryBarrier, 0, NULL, 0, NULL );
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "bloom[2]" );
				blitter.record(commandBuffer, descriptor, 0, 2);
				vkCmdPipelineBarrier( commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_FLAGS_NONE, 1, &memoryBarrier, 0, NULL, 0, NULL );
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "bloom[3]" );
				blitter.record(commandBuffer, descriptor, 0, 3);

				// transition attachments back to shader read layouts
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "setImageLayout" );
				::transitionAttachmentsFrom( this, shader, commandBuffer );
			}

			// construct depth-pyramid
		#if 0
			if ( settings::pipelines::culling && blitter.material.hasShader("compute", "depth-pyramid") ) {
				auto& shader = blitter.material.getShader("compute", "depth-pyramid");
			//	auto mips = attachment.descriptor.mips; // uf::vector::mips( pod::Vector2ui{ renderTarget.width, renderTarget.height } );
				auto mips = uf::vector::mips( pod::Vector2ui{ width, height } );

				ext::vulkan::GraphicDescriptor descriptor = blitter.descriptor;
				descriptor.renderMode = "";
				descriptor.aux = uf::vector::mips( pod::Vector2ui{ width, height } );
				descriptor.pipeline = "depth-pyramid";
				descriptor.bind.width = width;
				descriptor.bind.height = height;
				descriptor.bind.depth = metadata.eyes;
				descriptor.bind.point = VK_PIPELINE_BIND_POINT_COMPUTE;
				descriptor.subpass = 0;

			/*
				// transition attachments to general attachments for imageStore
				VkImageSubresourceRange subresourceRange;
				subresourceRange.baseMipLevel = 0;
				subresourceRange.levelCount = 1;
				subresourceRange.baseArrayLayer = 0;
				subresourceRange.layerCount = mips;
				subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				subresourceRange.layerCount = this->metadata.eyes;

				auto& attachment = this->getAttachment("depth");
				uf::renderer::Texture::setImageLayout( commandBuffer, attachment.image, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL, subresourceRange );
			*/

				// dispatch compute shader				
				VkMemoryBarrier memoryBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
				memoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				memoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

				for ( auto i = 0; i < mips - 1; ++i ) {
					descriptor.bind.width = width >> i;
					descriptor.bind.height = height >> i;
					if ( descriptor.bind.width < 1 ) descriptor.bind.width = 1;
					if ( descriptor.bind.height < 1 ) descriptor.bind.height = 1;

					blitter.record(commandBuffer, descriptor, 0, i);
					vkCmdPipelineBarrier( commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_FLAGS_NONE, 1, &memoryBarrier, 0, NULL, 0, NULL );
				}

			/*
				// transition attachments to general attachments for imageStore
				uf::renderer::Texture::setImageLayout( commandBuffer, attachment.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, subresourceRange );
			*/
			}
		#endif
			// post-renderpass commands
			if ( commandBufferCallbacks.count(CALLBACK_END) > 0 ) {
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "callback[end]" );
				commandBufferCallbacks[CALLBACK_END]( commandBuffer, i );
			}

		#if 0
			if ( this->hasAttachment("depth") ) {
				auto& attachment = this->getAttachment("depth");
				ext::vulkan::Texture texture; texture.aliasAttachment( attachment );
				texture.width = width;
				texture.height = height;
				texture.depth = 1;

				texture.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
				texture.descriptor.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			#if 1
				imageMemoryBarrier.subresourceRange.layerCount = metadata.eyes;
				imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
				uf::renderer::Texture::setImageLayout( commandBuffer, attachment.image, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, imageMemoryBarrier.subresourceRange );
			#endif


				for ( size_t eye = 0; eye < metadata.eyes; ++eye ) {
					texture.generateMipmaps(commandBuffer, eye);
				}

			#if 1
				uf::renderer::Texture::setImageLayout( commandBuffer, attachment.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, imageMemoryBarrier.subresourceRange );	
				imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				imageMemoryBarrier.subresourceRange.layerCount = 1;
			#endif
			}
		#endif
		#endif

		/*
			for ( auto layer : layers ) {
				layer->pipelineBarrier( commandBuffer, 1 );
			}
		*/
		}

		device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::END, "end" );
		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
	}
}

#endif