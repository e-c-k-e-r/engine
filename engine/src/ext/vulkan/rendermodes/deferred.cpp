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
#include <uf/utils/io/fmt.h>

#include <uf/ext/vulkan/graphic.h>
#include <uf/engine/graph/graph.h>
#include <uf/engine/ext.h>
#include <uf/ext/ffx/fsr.h>

#define BARYCENTRIC 1
#if BARYCENTRIC
	// 0 keeps a buffer for barycentric coordinates, 1 will reconstruct in the deferred pass
	#ifndef BARYCENTRIC_CALCULATE
		#define BARYCENTRIC_CALCULATE 0
	#endif
#endif

namespace {
	const uf::stl::string DEFERRED_MODE = "compute";

	namespace postprocesses {
		struct {
			ext::vulkan::Buffer atomicCounter;
			uf::stl::vector<VkImageView> views;
		} depthPyramid, bloom, dof;
	}
	
	struct AtomicCounter {
		uint32_t counter;
	};
	struct PushConstants {
		uint32_t mips;
		uint32_t numWorkGroups;
		uint32_t workGroupOffset;
	};

	void destroyImageView( ext::vulkan::Device& device, VkImageView view ) {
		ext::vulkan::mutex.lock();
		auto& texture = device.transient.textures.emplace_back();
		ext::vulkan::mutex.unlock();

		texture.device = &device;
		texture.view = view;
	/*
		vkDestroyImageView(device.logicalDevice, view, nullptr);
		VK_UNREGISTER_HANDLE(view);
	*/
	}

	void buildMippedViews( ext::vulkan::Shader& shader, ext::vulkan::Texture2D& source, uf::stl::vector<VkImageView>& views, size_t mips ) {
		auto& device = *shader.device;

		for ( auto& view : views ) ::destroyImageView( device, view );
		views.clear();
		views.resize( mips );

		shader.textures.clear();

		for ( auto i = 0; i < mips; ++i ) {
			auto& view = views[i];

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
			VK_REGISTER_HANDLE(view);

			auto& texture = shader.textures.emplace_back();
			texture.aliasTexture( source );
			texture.view = view;
			texture.layout = VK_IMAGE_LAYOUT_GENERAL;
			texture.updateDescriptors();
		}
	}
}

#include "./transition.inl"

const uf::stl::string ext::vulkan::DeferredRenderMode::getType() const {
	return "Deferred";
}


void ext::vulkan::DeferredRenderMode::initialize( Device& device ) {
	ext::vulkan::RenderMode::initialize( device );
	
	uint32_t width = this->width > 0 ? this->width : (ext::vulkan::settings::width * this->scale);
	uint32_t height = this->height > 0 ? this->height : (ext::vulkan::settings::height * this->scale);
	uint32_t mips = uf::vector::mips( pod::Vector2ui{ width, height } );

	renderTarget.device = &device;
	renderTarget.views = metadata.eyes;
	size_t msaa = ext::vulkan::settings::msaa;

	struct {
		size_t id, bary, depth, uv, normal;
		size_t color, scratch, motion, output;
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
		/*.usage = */VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		/*.blend = */false,
		/*.samples = */msaa,
		//*.mips = */1,
	});
	// output buffers
	attachments.color = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format =*/ ext::vulkan::settings::pipelines::hdr ? enums::Format::HDR : enums::Format::SDR,
		/*.layout = */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		/*.usage =*/ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		/*.blend =*/ false,
		/*.samples =*/ 1,
	});
	attachments.scratch = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format =*/ ext::vulkan::settings::pipelines::hdr ? enums::Format::HDR : enums::Format::SDR,
		/*.layout = */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		/*.usage =*/ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		/*.blend =*/ false,
		/*.samples =*/ 1,
		/*.mips =*/ mips,
	});
	attachments.motion = renderTarget.attach(RenderTarget::Attachment::Descriptor{
	//	/*.format = */VK_FORMAT_R32G32B32A32_SFLOAT,
		/*.format = */VK_FORMAT_R16G16_SFLOAT,
		/*.layout = */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		/*.blend = */false,
		/*.samples = */1,
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
	metadata.attachments["color"] = attachments.color;
	metadata.attachments["scratch"] = attachments.scratch;
	metadata.attachments["motion"] = attachments.motion;

	metadata.attachments["output"] = attachments.color;

	// First pass: fill the G-Buffer
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
		/*.layer = */0,
		/*.autoBuildPipeline =*/ true
	);
	if ( DEFERRED_MODE == "fragment" ) {
		renderTarget.addPass(
			/*.*/ VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
			/*.colors =*/ { attachments.color, attachments.scratch, attachments.motion },
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
			/*.layer = */0,
			/*.autoBuildPipeline =*/ false
		);
	}

	// metadata.outputs.emplace_back(metadata.attachments["output"]);
	renderTarget.initialize( device );

	{
		uf::Mesh mesh;
		mesh.vertex.count = 3;

		blitter.descriptor.renderMode = "Swapchain";
		blitter.descriptor.subpass = 0;
		blitter.descriptor.depth.test = false;
		blitter.descriptor.depth.write = false;
		blitter.descriptor.blend.enabled = false;

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
			if ( !settings::pipelines::fsr || !ext::fsr::frameUpscale ) {
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

			size_t maxLights = uf::config["engine"]["scenes"]["lights"]["max"].as<size_t>(512);
			size_t maxTextures2D = uf::config["engine"]["scenes"]["textures"]["max"]["2D"].as<size_t>(512);
			size_t maxTexturesCube = uf::config["engine"]["scenes"]["textures"]["max"]["cube"].as<size_t>(128);
			size_t maxTextures3D = uf::config["engine"]["scenes"]["textures"]["max"]["3D"].as<size_t>(128);
			size_t maxCascades = uf::config["engine"]["scenes"]["vxgi"]["cascades"].as<size_t>(16);

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
			shader.aliasAttachment("scratch", this, VK_IMAGE_LAYOUT_GENERAL);
			shader.aliasAttachment("motion", this, VK_IMAGE_LAYOUT_GENERAL);
		}
		
		if ( settings::pipelines::bloom ) {
			uf::stl::string computeShaderFilename = uf::io::resolveURI(uf::io::root+"/shaders/display/bloom/down.comp.spv");
			blitter.material.attachShader(computeShaderFilename, uf::renderer::enums::Shader::COMPUTE, "bloom-down");

			auto& shader = blitter.material.getShader("compute", "bloom-down");

			shader.aliasAttachment("color", this, VK_IMAGE_LAYOUT_GENERAL);
			shader.aliasAttachment("scratch", this, VK_IMAGE_LAYOUT_GENERAL);

			// atomic counter buffer
			::postprocesses::bloom.atomicCounter.initialize( (const void*) nullptr, sizeof(::AtomicCounter) * 1, uf::renderer::enums::Buffer::STORAGE );
			shader.aliasBuffer("atomicCounterBloom", ::postprocesses::bloom.atomicCounter);	
		}

		if ( settings::pipelines::bloom ) {
			uf::stl::string computeShaderFilename = uf::io::resolveURI(uf::io::root+"/shaders/display/bloom/up.comp.spv");
			blitter.material.metadata.autoInitializeUniformBuffers = false;
			blitter.material.attachShader(computeShaderFilename, uf::renderer::enums::Shader::COMPUTE, "bloom-up");
			blitter.material.metadata.autoInitializeUniformBuffers = true;

			auto& shader = blitter.material.getShader("compute", "bloom-up");

			shader.aliasAttachment("color", this, VK_IMAGE_LAYOUT_GENERAL);
			shader.aliasAttachment("scratch", this, VK_IMAGE_LAYOUT_GENERAL);

			{
				auto& shaderDown = blitter.material.getShader("compute", "bloom-down");
				auto& ubo = shaderDown.getUniformBuffer("UBO");

				shader.aliasBuffer("ubo", ubo);
			}
		}

		if ( settings::pipelines::dof ) {
			uf::stl::string computeShaderFilename = uf::io::resolveURI(uf::io::root+"/shaders/display/dof/down.comp.spv");
			blitter.material.attachShader(computeShaderFilename, uf::renderer::enums::Shader::COMPUTE, "dof-down");

			auto& shader = blitter.material.getShader("compute", "dof-down");

			shader.aliasAttachment("color", this, VK_IMAGE_LAYOUT_GENERAL);
			shader.aliasAttachment("depth", this);
			shader.aliasAttachment("scratch", this, VK_IMAGE_LAYOUT_GENERAL);

			// atomic counter buffer
			::postprocesses::dof.atomicCounter.initialize( (const void*) nullptr, sizeof(::AtomicCounter) * 1, uf::renderer::enums::Buffer::STORAGE );
			shader.aliasBuffer("atomicCounterBloom", ::postprocesses::dof.atomicCounter);
		}

		if ( settings::pipelines::dof ) {
			uf::stl::string computeShaderFilename = uf::io::resolveURI(uf::io::root+"/shaders/display/dof/up.comp.spv");
			blitter.material.metadata.autoInitializeUniformBuffers = false;
			blitter.material.attachShader(computeShaderFilename, uf::renderer::enums::Shader::COMPUTE, "dof-up");
			blitter.material.metadata.autoInitializeUniformBuffers = true;

			auto& shader = blitter.material.getShader("compute", "dof-up");

			shader.aliasAttachment("color", this, VK_IMAGE_LAYOUT_GENERAL);
			shader.aliasAttachment("depth", this);
			shader.aliasAttachment("scratch", this, VK_IMAGE_LAYOUT_GENERAL);

			{
				auto& shaderDown = blitter.material.getShader("compute", "dof-down");
				auto& ubo = shaderDown.getUniformBuffer("UBO");

				shader.aliasBuffer("ubo", ubo);
			}
		}

		if ( settings::pipelines::culling ) {
			uf::stl::string computeShaderFilename = uf::io::resolveURI(uf::io::root+"/shaders/display/depth-pyramid/comp.spv");
			blitter.material.attachShader(computeShaderFilename, uf::renderer::enums::Shader::COMPUTE, "depth-pyramid");

			auto& shader = blitter.material.getShader("compute", "depth-pyramid");

			shader.aliasAttachment("depth", this);

			// atomic counter buffer
			::postprocesses::depthPyramid.atomicCounter.initialize( (const void*) nullptr, sizeof(::AtomicCounter) * 1, uf::renderer::enums::Buffer::STORAGE );
			shader.aliasBuffer("atomicCounterDepth", ::postprocesses::depthPyramid.atomicCounter);
		}
	}

	this->build(true);
}

void ext::vulkan::DeferredRenderMode::build( bool resized ) {
	ext::vulkan::RenderMode::build();

	uint32_t width = this->width > 0 ? this->width : (ext::vulkan::settings::width * this->scale);
	uint32_t height = this->height > 0 ? this->height : (ext::vulkan::settings::height * this->scale);
	auto mips = uf::vector::mips( pod::Vector2ui{ width, height } );

	auto& scene = uf::scene::getCurrentScene();
	auto& sceneMetadataJson = scene.getComponent<uf::Serializer>();
	auto& storage = uf::graph::globalStorage ? uf::graph::storage : scene.getComponent<pod::Graph::Storage>();

	// if resized (or initialized)
	if ( resized ) {
	#if UF_USE_FFX_FSR || UF_USE_FFX_SDK
		if ( settings::pipelines::fsr && ext::fsr::frameUpscale ) {
			auto& shader = blitter.material.getShader("fragment");
			shader.textures.clear();
			shader.textures.emplace_back().aliasTexture( ext::fsr::getRenderTarget() );
		}
	#endif

		if ( settings::pipelines::bloom ) {
			auto& shader = blitter.material.getShader("compute", "bloom-down");
			shader.setSpecializationConstants({
				{ "MIPS", mips },
			});
			shader.setDescriptorCounts({
				{ "outImage", mips },
			});
						
			ext::vulkan::Texture2D source;
			source.aliasAttachment( this->getAttachment("scratch") );
			::buildMippedViews( shader, source, ::postprocesses::bloom.views, mips );
		}

		if ( settings::pipelines::dof ) {
			auto& shader = blitter.material.getShader("compute", "dof-down");
			shader.setSpecializationConstants({
				{ "MIPS", mips },
			});
			shader.setDescriptorCounts({
				{ "outImage", mips },
			});
						
			ext::vulkan::Texture2D source;
			source.aliasAttachment( this->getAttachment("scratch") );
			::buildMippedViews( shader, source, ::postprocesses::dof.views, mips );
		}

		if ( settings::pipelines::culling ) {
			auto& shader = blitter.material.getShader("compute", "depth-pyramid");
			shader.setSpecializationConstants({
				{ "MIPS", mips },
			});
			shader.setDescriptorCounts({
				{ "outImage", mips },
			});

			shader.textures.clear();

			storage.buffers.depthPyramid.destroy(true);
			storage.buffers.depthPyramid.fromBuffers( NULL, 0, VK_FORMAT_R32_SFLOAT, width, height, 1, 1, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_LAYOUT_GENERAL );

			ext::vulkan::Texture2D& source = storage.buffers.depthPyramid;
			source.sampler.descriptor.reduction.enabled = true;
			source.sampler.descriptor.reduction.mode = VK_SAMPLER_REDUCTION_MODE_MIN;
			::buildMippedViews( shader, source, ::postprocesses::depthPyramid.views, mips );
		}
	}

	// (re)bind aliases
	if ( blitter.material.hasShader(DEFERRED_MODE, "deferred") ) {
		auto& shader = blitter.material.getShader(DEFERRED_MODE, "deferred");

		shader.metadata.aliases.buffers.clear();
		shader.aliasBuffer( storage.buffers.camera );
	//	shader.aliasBuffer( storage.buffers.joint );
		shader.aliasBuffer( storage.buffers.drawCommands );
		shader.aliasBuffer( storage.buffers.instance );
		shader.aliasBuffer( storage.buffers.instanceAddresses );
		shader.aliasBuffer( storage.buffers.object );
		shader.aliasBuffer( storage.buffers.material );
		shader.aliasBuffer( storage.buffers.texture );
		shader.aliasBuffer( storage.buffers.light );
	}

	// (re)initialize pipelines
	{
		ext::vulkan::GraphicDescriptor descriptor = blitter.descriptor;

		blitter.update( blitter.descriptor );

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
			blitter.update( descriptor );
		}

		if ( settings::pipelines::bloom ) {
			descriptor.aux = mips;
			descriptor.pipeline = "bloom-down";
			descriptor.subpass = 0;
			descriptor.bind.point = VK_PIPELINE_BIND_POINT_COMPUTE;
			blitter.update( descriptor );
		}

		if ( settings::pipelines::bloom ) {
			descriptor.aux = {};
			descriptor.pipeline = "bloom-up";
			descriptor.subpass = 0;
			descriptor.bind.point = VK_PIPELINE_BIND_POINT_COMPUTE;
			blitter.update( descriptor );
		}

		if ( settings::pipelines::dof ) {
			descriptor.aux = mips;
			descriptor.pipeline = "dof-down";
			descriptor.subpass = 0;
			descriptor.bind.point = VK_PIPELINE_BIND_POINT_COMPUTE;
			blitter.update( descriptor );
		}

		if ( settings::pipelines::dof ) {
			descriptor.aux = {};
			descriptor.pipeline = "dof-up";
			descriptor.subpass = 0;
			descriptor.bind.point = VK_PIPELINE_BIND_POINT_COMPUTE;
			blitter.update( descriptor );
		}

		if ( settings::pipelines::culling ) {
			descriptor.aux = mips;
			descriptor.pipeline = "depth-pyramid";
			descriptor.subpass = 0;
			descriptor.bind.point = VK_PIPELINE_BIND_POINT_COMPUTE;
			blitter.update( descriptor );
		}
	}
}
void ext::vulkan::DeferredRenderMode::tick() {
	ext::vulkan::RenderMode::tick();
	
	bool resized = (this->width == 0 && this->height == 0 && ext::vulkan::states::resized) || this->resized;
	bool rebuild = resized || ext::vulkan::states::rebuild || this->rebuild;

	uint32_t width = this->width > 0 ? this->width : (ext::vulkan::settings::width * this->scale);
	uint32_t height = this->height > 0 ? this->height : (ext::vulkan::settings::height * this->scale);
	auto mips = uf::vector::mips( pod::Vector2ui{ width, height } );

	auto& scene = uf::scene::getCurrentScene();
	auto& storage = uf::graph::globalStorage ? uf::graph::storage : scene.getComponent<pod::Graph::Storage>();

	// rebuild rendertarget
	if ( resized ) {
		this->resized = false;
		rebuild = true;
		renderTarget.initialize( *renderTarget.device );
	}

	// update blitter descriptor set
	if ( rebuild && blitter.initialized ) {
		this->build( resized );
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
	submitInfo.pWaitDstStageMask = waitStageMask;												// Pointer to the list of pipeline stages that the semaphore waits will occur at
	submitInfo.pWaitSemaphores = &swapchain.presentCompleteSemaphores[states::currentBuffer];	// Semaphore(s) to wait upon before the submitted command buffer starts executing
	submitInfo.waitSemaphoreCount = 1;															// One wait semaphore																				
	submitInfo.pSignalSemaphores = &renderCompleteSemaphores[states::currentBuffer];			// Semaphore(s) to be signaled when command buffers have completed
	submitInfo.signalSemaphoreCount = 1;														// One signal semaphore
	submitInfo.pCommandBuffers = &commands[states::currentBuffer];								// Command buffers(s) to execute in this batch (submission)
	submitInfo.commandBufferCount = 1;

	return submitInfo;
}
void ext::vulkan::DeferredRenderMode::render() {
//	if ( this->executed ) return;
	//lockMutex( this->mostRecentCommandPoolId );
	if ( this->commands.container().empty() ) return;
	
	auto& commands = getCommands( this->mostRecentCommandPoolId );

	VK_COMMAND_BUFFER_CALLBACK( EXECUTE_BEGIN, VkCommandBuffer{}, 0, {} );
	// Submit commands
	// Use a fence to ensure that command buffer has finished executing before using it again
	/*
	VK_CHECK_RESULT(vkWaitForFences( *device, 1, &fences[states::currentBuffer], VK_TRUE, VK_DEFAULT_FENCE_TIMEOUT ));
	VK_CHECK_RESULT(vkResetFences( *device, 1, &fences[states::currentBuffer] ));
	*/

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
	VkResult res = vkQueueSubmit( queue, 1, &submitInfo, VK_NULL_HANDLE/*fences[states::currentBuffer]*/);
	VK_CHECK_QUEUE_CHECKPOINT( queue, res );
	VK_COMMAND_BUFFER_CALLBACK( EXECUTE_END, VkCommandBuffer{}, 0, {} );

	this->executed = true;
	//unlockMutex( this->mostRecentCommandPoolId );
}
void ext::vulkan::DeferredRenderMode::destroy() {
	// cleanup
	::postprocesses::depthPyramid.atomicCounter.destroy(false);
	::postprocesses::bloom.atomicCounter.destroy(false);
	::postprocesses::dof.atomicCounter.destroy(false);
	
	for ( auto& view : ::postprocesses::bloom.views ) {
		vkDestroyImageView(device->logicalDevice, view, nullptr);
		VK_UNREGISTER_HANDLE(view);
	}
	::postprocesses::bloom.views.clear();

	for ( auto& view : ::postprocesses::dof.views ) {
		vkDestroyImageView(device->logicalDevice, view, nullptr);
		VK_UNREGISTER_HANDLE(view);
	}
	::postprocesses::dof.views.clear();

	for ( auto& view : ::postprocesses::depthPyramid.views ) {
		vkDestroyImageView(device->logicalDevice, view, nullptr);
		VK_UNREGISTER_HANDLE(view);
	}
	::postprocesses::depthPyramid.views.clear();
	
	ext::vulkan::RenderMode::destroy();
}

ext::vulkan::GraphicDescriptor ext::vulkan::DeferredRenderMode::bindGraphicDescriptor( const ext::vulkan::GraphicDescriptor& reference, size_t pass ) {
	ext::vulkan::GraphicDescriptor descriptor = ext::vulkan::RenderMode::bindGraphicDescriptor(reference, pass);
	if ( descriptor.renderMode != "" ) descriptor.invalidated = true;
	return descriptor;
}
void ext::vulkan::DeferredRenderMode::createCommandBuffers( const uf::stl::vector<ext::vulkan::Graphic*>& graphics ) {
	uint32_t width = this->width > 0 ? this->width : (ext::vulkan::settings::width * this->scale);
	uint32_t height = this->height > 0 ? this->height : (ext::vulkan::settings::height * this->scale);

	VkCommandBufferBeginInfo cmdBufInfo = ext::vulkan::initializers::commandBufferBeginInfo();
	cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	VkImageMemoryBarrier imageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.graphics; //VK_QUEUE_FAMILY_IGNORED
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.graphics; //VK_QUEUE_FAMILY_IGNORED
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
	bool shouldRecord = true; // ( settings::pipelines::rt && !uf::config["engine"]["scenes"]["rt"]["full"].as<bool>() ) || !settings::pipelines::rt;
	for (size_t frame = 0; frame < commands.size(); ++frame) {
		auto commandBuffer = commands[frame];
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
			renderPassBeginInfo.framebuffer = renderTarget.framebuffers[frame];

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
					graphic->record( commandBuffer, descriptor, 0, metadata.eyes, frame );
				}
			}

			// pre-renderpass commands
			VK_COMMAND_BUFFER_CALLBACK( CALLBACK_BEGIN, commandBuffer, frame, {
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "callback[begin]" );
			} );

			device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::BEGIN, "renderPass[begin]" ) ;
			vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
				vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
				vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
				// render to geometry buffers
				{		
					size_t currentPass = 0;
					size_t currentDraw = 0;
					for ( auto graphic : graphics ) {
						// only draw graphics that are assigned to this type of render mode
						if ( graphic->descriptor.renderMode != this->getName() ) continue;
						ext::vulkan::GraphicDescriptor descriptor = bindGraphicDescriptor(graphic->descriptor, currentSubpass);
						device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, ::fmt::format("graphic[{}]", currentDraw) );
						graphic->record( commandBuffer, descriptor, 0, currentDraw++, frame );
					}
				}
				// skip deferred pass if RT is enabled, we still process geometry for a depth buffer
				if ( DEFERRED_MODE == "fragment" ) {		
					device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "nextSubpass" );
					vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE); ++currentSubpass;

					size_t currentPass = 0;
					size_t currentDraw = 0;
					{
						ext::vulkan::GraphicDescriptor descriptor = blitter.descriptor; // = bindGraphicDescriptor(blitter.descriptor, currentSubpass);
						descriptor.renderMode = "";
						descriptor.pipeline = "deferred";
						descriptor.subpass = currentSubpass;
						descriptor.bind.point = VK_PIPELINE_BIND_POINT_GRAPHICS;
						device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "deferred" );
						blitter.record(commandBuffer, descriptor, 0, currentDraw++, frame);
					}
				}
			device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::END, "renderPass[end]" );
			vkCmdEndRenderPass(commandBuffer);

		#if 1
			if ( settings::pipelines::deferred && DEFERRED_MODE == "compute" && blitter.material.hasShader(DEFERRED_MODE, "deferred") ) {
				VkMemoryBarrier computeBarrier = {};
				computeBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
				computeBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				computeBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				vkCmdPipelineBarrier( commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &computeBarrier, 0, nullptr, 0, nullptr );

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
				blitter.record(commandBuffer, descriptor, 0, 0, frame);

				// transition attachments back to shader read layouts
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "setImageLayout" );
				::transitionAttachmentsFrom( this, shader, commandBuffer );
			}

			if ( settings::pipelines::bloom && blitter.material.hasShader("compute", "bloom-down") ) {
				auto& shader = blitter.material.getShader("compute", "bloom-down");
				auto mips = uf::vector::mips( pod::Vector2ui{ width, height } );

				uint32_t dispatchX = (width + 63) / 64;
				uint32_t dispatchY = (height + 63) / 64;
				uint32_t numWorkGroups = dispatchX * dispatchY;
				auto& pushConstant = shader.pushConstants.front().get<::PushConstants>();
				pushConstant = {
					.mips = mips,
					.numWorkGroups = numWorkGroups,
					.workGroupOffset = 0,
				};

				ext::vulkan::GraphicDescriptor descriptor = blitter.descriptor;
				descriptor.renderMode = "";
				descriptor.aux = mips;
				descriptor.pipeline = "bloom-down";
				descriptor.bind.width = dispatchX * 256;
				descriptor.bind.height = dispatchY;
				descriptor.bind.depth = metadata.eyes;
				descriptor.bind.point = VK_PIPELINE_BIND_POINT_COMPUTE;
				descriptor.subpass = 0;

				// reset counter buffer
				vkCmdFillBuffer(commandBuffer, ::postprocesses::bloom.atomicCounter.buffer, 0, 4, 0);
				VkMemoryBarrier counterBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
				counterBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				counterBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &counterBarrier, 0, nullptr, 0, nullptr);

				// transition attachments to general attachments for imageStore
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "setImageLayout" );
				::transitionAttachmentsTo( this, shader, commandBuffer );

				// dispatch compute shader				
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "bloom[down]" );
				blitter.record( commandBuffer, descriptor );
			
			/*
				ext::vulkan::Texture2D source;
				source.aliasAttachment( this->getAttachment("scratch") );
				source.generateMipmaps( commandBuffer );
			*/

				// transition attachments back to shader read layouts
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "setImageLayout" );
				::transitionAttachmentsFrom( this, shader, commandBuffer );
			}

			if ( settings::pipelines::bloom && blitter.material.hasShader("compute", "bloom-up") ) {
				auto& shader = blitter.material.getShader("compute", "bloom-up");
				ext::vulkan::GraphicDescriptor descriptor = blitter.descriptor;
				descriptor.renderMode = "";
				descriptor.pipeline = "bloom-up";
				descriptor.bind.width = width;
				descriptor.bind.height = height;
				descriptor.bind.depth = metadata.eyes;
				descriptor.bind.point = VK_PIPELINE_BIND_POINT_COMPUTE;
				descriptor.subpass = 0;

				// transition attachments to general attachments for imageStore
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "setImageLayout" );
				::transitionAttachmentsTo( this, shader, commandBuffer );

				// dispatch compute shader				
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "bloom[up]" );
				blitter.record( commandBuffer, descriptor );

				// transition attachments back to shader read layouts
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "setImageLayout" );
				::transitionAttachmentsFrom( this, shader, commandBuffer );
			}

			if ( settings::pipelines::dof && blitter.material.hasShader("compute", "dof-down") ) {
				auto& shader = blitter.material.getShader("compute", "dof-down");
				auto mips = uf::vector::mips( pod::Vector2ui{ width, height } );

				uint32_t dispatchX = (width + 63) / 64;
				uint32_t dispatchY = (height + 63) / 64;
				uint32_t numWorkGroups = dispatchX * dispatchY;
				auto& pushConstant = shader.pushConstants.front().get<::PushConstants>();
				pushConstant = {
					.mips = mips,
					.numWorkGroups = numWorkGroups,
					.workGroupOffset = 0,
				};

				ext::vulkan::GraphicDescriptor descriptor = blitter.descriptor;
				descriptor.renderMode = "";
				descriptor.aux = mips;
				descriptor.pipeline = "dof-down";
				descriptor.bind.width = dispatchX * 256;
				descriptor.bind.height = dispatchY;
				descriptor.bind.depth = metadata.eyes;
				descriptor.bind.point = VK_PIPELINE_BIND_POINT_COMPUTE;
				descriptor.subpass = 0;

				// reset counter buffer
				vkCmdFillBuffer(commandBuffer, ::postprocesses::dof.atomicCounter.buffer, 0, 4, 0);
				VkMemoryBarrier counterBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
				counterBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				counterBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &counterBarrier, 0, nullptr, 0, nullptr);

				// transition attachments to general attachments for imageStore
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "setImageLayout" );
				::transitionAttachmentsTo( this, shader, commandBuffer );

				// dispatch compute shader				
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "dof[down]" );
				blitter.record( commandBuffer, descriptor );
			
			/*
				ext::vulkan::Texture2D source;
				source.aliasAttachment( this->getAttachment("scratch") );
				source.generateMipmaps( commandBuffer );
			*/

				// transition attachments back to shader read layouts
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "setImageLayout" );
				::transitionAttachmentsFrom( this, shader, commandBuffer );
			}

			if ( settings::pipelines::dof && blitter.material.hasShader("compute", "dof-up") ) {
				auto& shader = blitter.material.getShader("compute", "dof-up");
				ext::vulkan::GraphicDescriptor descriptor = blitter.descriptor;
				descriptor.renderMode = "";
				descriptor.pipeline = "dof-up";
				descriptor.bind.width = width;
				descriptor.bind.height = height;
				descriptor.bind.depth = metadata.eyes;
				descriptor.bind.point = VK_PIPELINE_BIND_POINT_COMPUTE;
				descriptor.subpass = 0;

				// transition attachments to general attachments for imageStore
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "setImageLayout" );
				::transitionAttachmentsTo( this, shader, commandBuffer );

				// dispatch compute shader				
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "dof[up]" );
				blitter.record( commandBuffer, descriptor );

				// transition attachments back to shader read layouts
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "setImageLayout" );
				::transitionAttachmentsFrom( this, shader, commandBuffer );
			}

			// construct depth-pyramid
			if ( settings::pipelines::culling && blitter.material.hasShader("compute", "depth-pyramid") ) {
				auto& shader = blitter.material.getShader("compute", "depth-pyramid");
				auto mips = uf::vector::mips( pod::Vector2ui{ width, height } );

				uint32_t dispatchX = (width + 63) / 64;
				uint32_t dispatchY = (height + 63) / 64;
				uint32_t numWorkGroups = dispatchX * dispatchY;
				auto& pushConstant = shader.pushConstants.front().get<::PushConstants>();
				pushConstant = {
					.mips = mips,
					.numWorkGroups = numWorkGroups,
					.workGroupOffset = 0,
				};

				ext::vulkan::GraphicDescriptor descriptor = blitter.descriptor;
				descriptor.renderMode = "";
				descriptor.aux = mips;
				descriptor.pipeline = "depth-pyramid";
				descriptor.bind.width = dispatchX * 256;
				descriptor.bind.height = dispatchY;
				descriptor.bind.depth = metadata.eyes;
				descriptor.bind.point = VK_PIPELINE_BIND_POINT_COMPUTE;
				descriptor.subpass = 0;

				// reset counter buffer
				vkCmdFillBuffer(commandBuffer, ::postprocesses::depthPyramid.atomicCounter.buffer, 0, 4, 0);
				VkMemoryBarrier counterBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
				counterBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				counterBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
				vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &counterBarrier, 0, nullptr, 0, nullptr);

				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "setImageLayout" );
				::transitionAttachmentsTo( this, shader, commandBuffer );

				blitter.record(commandBuffer, descriptor);

				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "setImageLayout" );
				::transitionAttachmentsFrom( this, shader, commandBuffer );
			}

			// post-renderpass commands
			VK_COMMAND_BUFFER_CALLBACK( CALLBACK_END, commandBuffer, frame, {
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "callback[end]" );
			} );
		#endif
		}

	/*
	#if UF_USE_FFX_FSR || UF_USE_FFX_SDK
		{
			device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::END, "fsr:start" );
			ext::fsr::render( commandBuffer );
			device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::END, "fsr:end" );
		}
	#endif
	*/

		device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::END, "end" );
		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
	}
}

#endif