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

const uf::stl::string ext::vulkan::DeferredRenderMode::getType() const {
	return "Deferred";
}
const size_t ext::vulkan::DeferredRenderMode::blitters() const {
	return 1;
}
ext::vulkan::Graphic* ext::vulkan::DeferredRenderMode::getBlitter( size_t i ) {
	return &this->blitter;
}
uf::stl::vector<ext::vulkan::Graphic*> ext::vulkan::DeferredRenderMode::getBlitters() {
	return { &this->blitter };
}

void ext::vulkan::DeferredRenderMode::initialize( Device& device ) {
	if ( metadata.eyes == 0 ) metadata.eyes = 1;
	{
		float width = this->width > 0 ? this->width : ext::vulkan::settings::width;
		float height = this->height > 0 ? this->height : ext::vulkan::settings::height;
		auto& swapchainRender = ext::vulkan::getRenderMode("Swapchain");
		if ( swapchainRender.renderTarget.width != width || swapchainRender.renderTarget.height != height ) {
			settings::experimental::deferredAliasOutputToSwapchain = false;
		}
	}

	if ( settings::experimental::bloom ) settings::experimental::deferredAliasOutputToSwapchain = false;

	auto HDR_FORMAT = VK_FORMAT_R32G32B32A32_SFLOAT;
	auto SDR_FORMAT = VK_FORMAT_R16G16B16A16_SFLOAT;

	ext::vulkan::RenderMode::initialize( device );
	renderTarget.device = &device;
	renderTarget.views = metadata.eyes;
	size_t msaa = ext::vulkan::settings::msaa;

	struct {
		size_t id, normals, uvs, mips, albedo, depth, color, bright, scratch, output;
	} attachments = {};

	bool blend = true; // !ext::vulkan::settings::experimental::deferredSampling;
	attachments.id = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format = */VK_FORMAT_R16G16_UINT,
		/*.layout = */VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
		/*.blend = */false,
		/*.samples = */msaa,
	});
	attachments.normals = renderTarget.attach(RenderTarget::Attachment::Descriptor{
	//	/*.format = */VK_FORMAT_R16G16_SFLOAT,
		/*.format = */VK_FORMAT_R16G16_SNORM,
		/*.layout = */VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
		/*.blend = */false,
		/*.samples = */msaa,
	});
	if ( ext::vulkan::settings::experimental::deferredSampling ) {
		attachments.uvs = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		//	/*.format = */VK_FORMAT_R32G32B32A32_SFLOAT,
			/*.format = */VK_FORMAT_R16G16B16A16_SFLOAT,
			/*.layout = */VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
			/*.blend = */false,
			/*.samples = */msaa,
		});
		attachments.mips = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		//	/*.format = */VK_FORMAT_R32G32B32A32_SFLOAT,
			/*.format = */VK_FORMAT_R16G16_SFLOAT,
			/*.layout = */VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
			/*.blend = */false,
			/*.samples = */msaa,
		});
	} else {
		attachments.albedo = renderTarget.attach(RenderTarget::Attachment::Descriptor{
			/*.format = */ext::vulkan::settings::experimental::hdr ? HDR_FORMAT : VK_FORMAT_R8G8B8A8_UNORM,
			/*.layout = */VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
			/*.blend = */blend,
			/*.samples = */msaa,
		});
	}
	attachments.depth = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format = */ext::vulkan::settings::formats::depth,
		/*.layout = */VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		/*.usage = */VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
		/*.blend = */false,
		/*.samples = */msaa,
	});
	attachments.color = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format =*/ ext::vulkan::settings::experimental::hdr ? HDR_FORMAT : SDR_FORMAT,
		/*.layout = */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		/*.usage =*/ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		/*.blend =*/ blend,
		/*.samples =*/ 1,
	});
	attachments.bright = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format =*/ ext::vulkan::settings::experimental::hdr ? HDR_FORMAT : SDR_FORMAT,
		/*.layout = */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		/*.usage =*/ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		/*.blend =*/ blend,
		/*.samples =*/ 1,
	});
	attachments.scratch = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format =*/ ext::vulkan::settings::experimental::hdr ? HDR_FORMAT : SDR_FORMAT,
		/*.layout = */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		/*.usage =*/ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		/*.blend =*/ blend,
		/*.samples =*/ 1,
	});

	// Attach swapchain's image as output
	if ( settings::experimental::deferredAliasOutputToSwapchain ) {
		attachments.output = renderTarget.attachments.size();
		auto& swapchainAttachment = renderTarget.attachments.emplace_back();
		swapchainAttachment.descriptor.format = ext::vulkan::settings::formats::color; //device.formats.color;
		swapchainAttachment.descriptor.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapchainAttachment.descriptor.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		swapchainAttachment.descriptor.aliased = true;
		{
			VkBool32 blendEnabled = blend;
			VkColorComponentFlags writeMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			VkPipelineColorBlendAttachmentState blendAttachmentState = ext::vulkan::initializers::pipelineColorBlendAttachmentState(
				writeMask,
				blendEnabled
			);
			if ( blendEnabled == VK_TRUE ) {
				blendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
				blendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				blendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
				blendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
				blendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
				blendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
			}
			swapchainAttachment.blendState = blendAttachmentState;
		}
	} else {
		attachments.output = renderTarget.attach(RenderTarget::Attachment::Descriptor{
			/*.format =*/ ext::vulkan::settings::experimental::hdr ? HDR_FORMAT : SDR_FORMAT,
			/*.layout = */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			/*.usage =*/ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
			/*.blend =*/ blend,
			/*.samples =*/ 1,
		});
	}
	metadata.outputs.emplace_back(attachments.color);
//	metadata.outputs.emplace_back(attachments.output);

	for ( size_t eye = 0; eye < metadata.eyes; ++eye ) {
		if ( ext::vulkan::settings::experimental::deferredSampling ) {
			// First pass: fill the G-Buffer
			{
				renderTarget.addPass(
					/*.*/ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					/*.colors =*/ { attachments.id, attachments.normals, attachments.uvs, attachments.mips },
					/*.inputs =*/ {},
					/*.resolve =*/{},
					/*.depth = */ attachments.depth,
					/*.layer = */eye,
					/*.autoBuildPipeline =*/ true
				);
			}
			// Second pass: write to color
			{
				renderTarget.addPass(
					/*.*/ VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
					/*.colors =*/ { attachments.color, attachments.bright },
					/*.inputs =*/ { attachments.id, attachments.normals, attachments.uvs, attachments.depth, attachments.mips },
					/*.resolve =*/{},
					/*.depth = */attachments.depth,
					/*.layer = */eye,
					/*.autoBuildPipeline =*/ false
				);
			}
		} else {
			// First pass: fill the G-Buffer
			{
				renderTarget.addPass(
					/*.*/ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					/*.colors =*/ { attachments.id, attachments.normals, attachments.albedo },
					/*.inputs =*/ {},
					/*.resolve =*/{},
					/*.depth = */ attachments.depth,
					/*.layer = */eye,
					/*.autoBuildPipeline =*/ true
				);
			}
			// Second pass: write to color
			{
				renderTarget.addPass(
					/*.*/ VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
					/*.colors =*/ { attachments.color, attachments.bright },
					/*.inputs =*/ { attachments.id, attachments.normals, attachments.albedo, attachments.depth },
					/*.resolve =*/{},
					/*.depth = */ attachments.depth,
					/*.layer = */eye,
					/*.autoBuildPipeline =*/ false
				);
			}
		}
	}

	renderTarget.initialize( device );

	{
		uf::Mesh mesh;
		mesh.vertex.count = 3;
	/*
		mesh.bind<pod::Vertex_2F2F, uint16_t>();
		mesh.insertVertices<pod::Vertex_2F2F>({
			{ {-1.0f, 1.0f}, {0.0f, 1.0f}, },
			{ {-1.0f, -1.0f}, {0.0f, 0.0f}, },
			{ {1.0f, -1.0f}, {1.0f, 0.0f}, },
			{ {1.0f, 1.0f}, {1.0f, 1.0f}, }
		});
		mesh.insertIndices<uint16_t>({
			0, 1, 2, 2, 3, 0
		});
	*/

		blitter.descriptor.subpass = 1;
		blitter.descriptor.depth.test = false;
		blitter.descriptor.depth.write = false;

		blitter.initialize( this->getName() );
		blitter.initializeMesh( mesh );

		uf::stl::string vertexShaderFilename = uf::io::root+"/shaders/display/subpass.vert.spv";
		uf::stl::string fragmentShaderFilename = uf::io::root+"/shaders/display/subpass.frag.spv"; {
			std::pair<bool, uf::stl::string> settings[] = {
				{ uf::renderer::settings::experimental::vxgi, "vxgi.frag" },
				{ msaa > 1, "msaa.frag" },
				{ uf::renderer::settings::experimental::deferredSampling, "deferredSampling.frag" },
			};
			FOR_ARRAY( settings ) if ( settings[i].first ) fragmentShaderFilename = uf::string::replace( fragmentShaderFilename, "frag", settings[i].second );
		}

		blitter.material.initializeShaders({
			{uf::io::resolveURI(vertexShaderFilename), VK_SHADER_STAGE_VERTEX_BIT},
			{uf::io::resolveURI(fragmentShaderFilename), VK_SHADER_STAGE_FRAGMENT_BIT}
		});
		
		auto& scene = uf::scene::getCurrentScene();
		auto& sceneMetadataJson = scene.getComponent<uf::Serializer>();
		
		if ( settings::experimental::bloom ) {
			uf::stl::string computeShaderFilename = uf::io::resolveURI(uf::io::root+"/shaders/display/bloom.comp.spv");
			blitter.material.attachShader(computeShaderFilename, uf::renderer::enums::Shader::COMPUTE, "bloom");

			auto& shader = blitter.material.getShader("compute", "bloom");
			shader.textures.clear();
			shader.textures.emplace_back().aliasAttachment( renderTarget.attachments[attachments.color], (size_t) 0 );
			shader.textures.emplace_back().aliasAttachment( renderTarget.attachments[attachments.bright], (size_t) 0 );
			shader.textures.emplace_back().aliasAttachment( renderTarget.attachments[attachments.scratch], (size_t) 0 );
			for ( auto& texture : shader.textures ) {
				texture.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
				texture.descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			}
		}
		
		{
			auto& shader = blitter.material.getShader("fragment");

			size_t maxLights = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["lights"]["max"].as<size_t>(512);
			size_t maxTextures2D = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["textures"]["max"]["2D"].as<size_t>(512);
			size_t maxTexturesCube = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["textures"]["max"]["cube"].as<size_t>(128);
			size_t maxTextures3D = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["textures"]["max"]["3D"].as<size_t>(128);
			size_t maxCascades = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["vxgi"]["cascades"].as<size_t>(16);

		//	shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.camera );
		//	shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.joint );
			shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.instance );
			shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.material );
			shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.texture );
			shader.buffers.emplace_back().aliasBuffer( uf::graph::storage.buffers.light );

			if ( ext::vulkan::settings::experimental::vxgi ) {
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
			} else {
				uint32_t* specializationConstants = (uint32_t*) (void*) shader.specializationConstants;
				for ( auto pair : shader.metadata.definitions.specializationConstants ) {
					auto& sc = pair.second;
					if ( sc.name == "TEXTURES" ) sc.value.ui = (specializationConstants[sc.index] = maxTextures2D);
					else if ( sc.name == "CUBEMAPS" ) sc.value.ui = (specializationConstants[sc.index] = maxTexturesCube);
				}
				for ( auto pair : shader.metadata.definitions.textures ) {
					auto& tx = pair.second;
					for ( auto& layout : shader.descriptorSetLayoutBindings ) {
						if ( layout.binding != tx.binding ) continue;
						if ( tx.name == "samplerTextures" ) layout.descriptorCount = maxTextures2D;
						else if ( tx.name == "samplerCubemaps" ) layout.descriptorCount = maxTexturesCube;
					}
				}
			}
		}
		for ( size_t eye = 0; eye < metadata.eyes; ++eye ) {
			auto descriptor = blitter.descriptor;
			descriptor.subpass = (renderTarget.passes.size() / metadata.eyes) * eye + 1;
			if ( !blitter.hasPipeline( descriptor ) ) blitter.initializePipeline( descriptor );

			if ( settings::experimental::bloom ) {
				descriptor.inputs.dispatch = { (width / 8) + 1, (height / 8) + 1, 1 };
				descriptor.pipeline = "bloom";
				descriptor.subpass = 0;
				if ( !blitter.hasPipeline( descriptor ) ) {
					blitter.initializePipeline( descriptor );
				}
			}
		}
	}
}
void ext::vulkan::DeferredRenderMode::tick() {
	ext::vulkan::RenderMode::tick();
	
	bool resized = this->width == 0 && this->height == 0 && ext::vulkan::states::resized;
	bool rebuild = resized || ext::vulkan::states::rebuild || this->rebuild;

	if ( resized ) {
		renderTarget.initialize( *renderTarget.device );

		if ( settings::experimental::bloom ) {
			auto& shader = blitter.material.getShader("compute", "bloom");
		#if 1
			shader.textures.clear();
			shader.textures.emplace_back().aliasAttachment( renderTarget.attachments[renderTarget.attachments.size() - 5], (size_t) 0 ); // attachments.color
			shader.textures.emplace_back().aliasAttachment( renderTarget.attachments[renderTarget.attachments.size() - 4], (size_t) 0 ); // attachments.bright
			shader.textures.emplace_back().aliasAttachment( renderTarget.attachments[renderTarget.attachments.size() - 3], (size_t) 0 ); // attachments.scratch
			for ( auto& texture : shader.textures ) {
				texture.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
				texture.descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			}
		#endif
		}
	}
	// update blitter descriptor set
	if ( rebuild && blitter.initialized ) {
		for ( size_t eye = 0; eye < metadata.eyes; ++eye ) {
			auto descriptor = blitter.descriptor;
			descriptor.subpass = (renderTarget.passes.size() / metadata.eyes) * eye + 1;
			if ( blitter.hasPipeline( blitter.descriptor ) ) blitter.getPipeline( blitter.descriptor ).update( blitter, blitter.descriptor );
			
			if ( settings::experimental::bloom ) {
				descriptor.inputs.dispatch = { (width / 8) + 1, (height / 8) + 1, 1 };
				descriptor.pipeline = "bloom";
				descriptor.subpass = 0;
				if ( blitter.hasPipeline( descriptor ) ) {
					blitter.getPipeline( descriptor ).update( blitter, descriptor );
				}
			}
		}
	}
}
void ext::vulkan::DeferredRenderMode::destroy() {
	ext::vulkan::RenderMode::destroy();
	blitter.destroy();
}

ext::vulkan::GraphicDescriptor ext::vulkan::DeferredRenderMode::bindGraphicDescriptor( const ext::vulkan::GraphicDescriptor& reference, size_t pass ) {
	ext::vulkan::GraphicDescriptor descriptor = ext::vulkan::RenderMode::bindGraphicDescriptor(reference, pass);
	return descriptor;
}
void ext::vulkan::DeferredRenderMode::createCommandBuffers( const uf::stl::vector<ext::vulkan::Graphic*>& graphics ) {
	float width = this->width > 0 ? this->width : ext::vulkan::settings::width;
	float height = this->height > 0 ? this->height : ext::vulkan::settings::height;

	VkCommandBufferBeginInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufInfo.pNext = nullptr;

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
	auto& sceneMetadata = scene.getComponent<uf::Serializer>();
	auto& commands = getCommands();
	auto& swapchainRender = ext::vulkan::getRenderMode("Swapchain");
	for (size_t i = 0; i < commands.size(); ++i) {
		VK_CHECK_RESULT(vkBeginCommandBuffer(commands[i], &cmdBufInfo));
		// Fill GBuffer
		{
			uf::stl::vector<VkClearValue> clearValues;
			for ( auto& attachment : renderTarget.attachments ) {
				VkClearValue clearValue;
				if ( attachment.descriptor.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ) {
					if ( !ext::json::isNull( sceneMetadata["system"]["renderer"]["clear values"][(int) clearValues.size()] ) ) {
						auto& v = sceneMetadata["system"]["renderer"]["clear values"][(int) clearValues.size()];
						clearValue.color = { {
							v[0].as<float>(),
							v[1].as<float>(),
							v[2].as<float>(),
							v[3].as<float>(),
						} };
					} else {
						clearValue.color = { { 0, 0, 0, 0 } };
					}
				} else if ( attachment.descriptor.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ) {
					if ( uf::matrix::reverseInfiniteProjection ) {
						clearValue.depthStencil = { 0.0f, 0 };
					} else {
						clearValue.depthStencil = { 1.0f, 0 };
					}
				}
				clearValues.push_back(clearValue);
			}
			// uf::matrix::reverseInfiniteProjection
			// descriptor.depth.operation ext::RENDERER::enums::Compare::GREATER_OR_EQUAL

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

			// transition layers for read
			for ( auto layer : layers ) {
				layer->pipelineBarrier( commands[i], 0 );
			}
			
			size_t currentSubpass = 0;

			for ( auto& pipeline : metadata.pipelines ) {
				if ( pipeline == metadata.pipeline ) continue;
				for ( auto graphic : graphics ) {
					if ( graphic->descriptor.renderMode != this->getName() ) continue;
					ext::vulkan::GraphicDescriptor descriptor = bindGraphicDescriptor(graphic->descriptor, currentSubpass);
					descriptor.pipeline = pipeline;
					graphic->record( commands[i], descriptor, 0, metadata.eyes );
				}
			}

			// pre-renderpass commands
			if ( commandBufferCallbacks.count(CALLBACK_BEGIN) > 0 ) commandBufferCallbacks[CALLBACK_BEGIN]( commands[i] );

			vkCmdBeginRenderPass(commands[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
				vkCmdSetViewport(commands[i], 0, 1, &viewport);
				vkCmdSetScissor(commands[i], 0, 1, &scissor);
				// render to geometry buffers
				for ( size_t eye = 0; eye < metadata.eyes; ++eye ) {
					size_t currentPass = 0;
					size_t currentDraw = 0;
					for ( auto graphic : graphics ) {
						// only draw graphics that are assigned to this type of render mode
						if ( graphic->descriptor.renderMode != this->getName() ) continue;
						ext::vulkan::GraphicDescriptor descriptor = bindGraphicDescriptor(graphic->descriptor, currentSubpass);
						graphic->record( commands[i], descriptor, eye, currentDraw++ );
					}
					// blit any RT's that request this subpass
					{
						for ( auto _ : layers ) {
							RenderTargetRenderMode* layer = (RenderTargetRenderMode*) _;
							auto& blitter = layer->blitter;
							if ( !blitter.initialized || !blitter.process || blitter.descriptor.subpass != currentPass ) continue;
							ext::vulkan::GraphicDescriptor descriptor = bindGraphicDescriptor(blitter.descriptor, currentSubpass);
							blitter.record(commands[i], descriptor);
						}
					}
				vkCmdNextSubpass(commands[i], VK_SUBPASS_CONTENTS_INLINE); ++currentPass; ++currentSubpass;
					// deferred post-processing lighting pass
					{
						ext::vulkan::GraphicDescriptor descriptor = bindGraphicDescriptor(blitter.descriptor, currentSubpass);
						blitter.record(commands[i], descriptor, eye, currentDraw++);
					}
					// blit any RT's that request this subpass
					{
						for ( auto _ : layers ) {
							RenderTargetRenderMode* layer = (RenderTargetRenderMode*) _;
							auto& blitter = layer->blitter;
							if ( !blitter.initialized || !blitter.process || blitter.descriptor.subpass != currentPass ) continue;
							ext::vulkan::GraphicDescriptor descriptor = bindGraphicDescriptor(blitter.descriptor, currentSubpass);
							blitter.record(commands[i], descriptor, eye, currentDraw++);
						}
					}
					if ( eye + 1 < metadata.eyes ) vkCmdNextSubpass(commands[i], VK_SUBPASS_CONTENTS_INLINE); ++currentSubpass;
				}
			vkCmdEndRenderPass(commands[i]);

			// post-renderpass commands
			if ( commandBufferCallbacks.count(CALLBACK_END) > 0 ) commandBufferCallbacks[CALLBACK_END]( commands[i] );

			if ( settings::experimental::bloom ) {
				ext::vulkan::GraphicDescriptor descriptor = blitter.descriptor;
				descriptor.inputs.dispatch = { (width / 8) + 1, (height / 8) + 1, 1 };
				descriptor.pipeline = "bloom";
				descriptor.subpass = 0;

				auto& shader = blitter.material.getShader("compute", "bloom");

				imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
				for ( auto& attachment : shader.textures ) {
					imageMemoryBarrier.image = attachment.image;
					vkCmdPipelineBarrier( commands[i], VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_FLAGS_NONE, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
				}
			#if 0
				blitter.record(commands[i], descriptor, 0, 0);
			
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
				for ( auto& attachment : shader.textures ) {
					imageMemoryBarrier.image = attachment.image;
					vkCmdPipelineBarrier( commands[i], VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_FLAGS_NONE, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
				}
			#endif
				blitter.record(commands[i], descriptor, 0, 1);
				for ( auto& attachment : shader.textures ) {
					imageMemoryBarrier.image = attachment.image;
					vkCmdPipelineBarrier( commands[i], VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_FLAGS_NONE, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
				}
				blitter.record(commands[i], descriptor, 0, 2);
				for ( auto& attachment : shader.textures ) {
					imageMemoryBarrier.image = attachment.image;
					vkCmdPipelineBarrier( commands[i], VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_FLAGS_NONE, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
				}
				for ( auto& attachment : shader.textures ) {
					imageMemoryBarrier.image = attachment.image;
					vkCmdPipelineBarrier( commands[i], VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_FLAGS_NONE, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
				}
				blitter.record(commands[i], descriptor, 0, 3);
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				for ( auto& attachment : shader.textures ) {
					imageMemoryBarrier.image = attachment.image;
					vkCmdPipelineBarrier( commands[i], VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_FLAGS_NONE, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
				}
			}

			for ( auto layer : layers ) {
				layer->pipelineBarrier( commands[i], 1 );
			}
			
			if ( !settings::experimental::deferredAliasOutputToSwapchain ) {
				{
					auto& renderTarget = swapchainRender.renderTarget;
					float width = renderTarget.width;
					float height = renderTarget.height;

					uf::stl::vector<VkClearValue> clearValues; clearValues.resize(2);
					clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
					clearValues[1].depthStencil = { 1.0f, 0 };

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

					vkCmdBeginRenderPass(commands[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
					vkCmdEndRenderPass(commands[i]);
				}

				{
					VkImageBlit imageBlitRegion{};
					imageBlitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					imageBlitRegion.srcSubresource.layerCount = 1;
					imageBlitRegion.srcOffsets[1] = { width, height, 1 };
					imageBlitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					imageBlitRegion.dstSubresource.layerCount = 1;
					imageBlitRegion.dstOffsets[1] = {
						swapchainRender.width > 0 ? swapchainRender.width : ext::vulkan::settings::width,
						swapchainRender.height > 0 ? swapchainRender.height : ext::vulkan::settings::height,
						1
					};
					auto& outputAttachment = renderTarget.attachments[metadata.outputs.front()];
					// Transition to KHR
					{
						imageMemoryBarrier.image = outputAttachment.image;
						imageMemoryBarrier.srcAccessMask = 0;
						imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
						imageMemoryBarrier.oldLayout = outputAttachment.descriptor.layout;
						imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
						vkCmdPipelineBarrier( commands[i], VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
					//	outputAttachment.descriptor.layout = imageMemoryBarrier.newLayout;
					}
					{
						imageMemoryBarrier.image = swapchainRender.renderTarget.attachments[i].image;
						imageMemoryBarrier.srcAccessMask = 0;
						imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
						imageMemoryBarrier.oldLayout = swapchainRender.renderTarget.attachments[i].descriptor.layout;
						imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
						vkCmdPipelineBarrier( commands[i], VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
						swapchainRender.renderTarget.attachments[i].descriptor.layout = imageMemoryBarrier.newLayout;
					}

					vkCmdBlitImage(
						commands[i],
						outputAttachment.image,
						VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						swapchainRender.renderTarget.attachments[i].image,
						VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						1,
						&imageBlitRegion,
						settings::swapchainUpscaleFilter
					);

					{
						imageMemoryBarrier.image = outputAttachment.image;
						imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
						imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
						imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL; //outputAttachment.descriptor.layout;
						imageMemoryBarrier.newLayout = outputAttachment.descriptor.layout; //VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
						vkCmdPipelineBarrier( commands[i], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
					//	outputAttachment.descriptor.layout = imageMemoryBarrier.newLayout;
					}
					{
						imageMemoryBarrier.image = swapchainRender.renderTarget.attachments[i].image;
						imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
						imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
						imageMemoryBarrier.oldLayout = swapchainRender.renderTarget.attachments[i].descriptor.layout;
						imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
						vkCmdPipelineBarrier( commands[i], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
						swapchainRender.renderTarget.attachments[i].descriptor.layout = imageMemoryBarrier.newLayout;
					}
				}
			}
		}

		VK_CHECK_RESULT(vkEndCommandBuffer(commands[i]));
	}
}

#endif