#if UF_USE_VULKAN

#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/vulkan/rendermodes/rendertarget.h>
#include <uf/ext/vulkan/initializers.h>
#include <uf/utils/window/window.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/ext/vulkan/graphic.h>
#include <uf/engine/graph/graph.h>
#include <uf/utils/camera/camera.h>

const uf::stl::string ext::vulkan::RenderTargetRenderMode::getType() const {
	return "RenderTarget";
}

ext::vulkan::GraphicDescriptor ext::vulkan::RenderTargetRenderMode::bindGraphicDescriptor( const ext::vulkan::GraphicDescriptor& reference, size_t pass ) {
	ext::vulkan::GraphicDescriptor descriptor = ext::vulkan::RenderMode::bindGraphicDescriptor(reference, pass);
/*
	descriptor.parse(metadata.json["descriptor"]);
	// invalidate
	if ( metadata.target != "" && descriptor.renderMode != this->getName() && descriptor.renderMode != metadata.target ) {
		descriptor.invalidated = true;
	} else {
		descriptor.renderMode = this->getName();
	}
*/

	if ( 0 <= pass && pass < metadata.subpasses && metadata.type == uf::renderer::settings::pipelines::names::vxgi ) {
		descriptor.cullMode = VK_CULL_MODE_NONE;
		descriptor.depth.test = false;
		descriptor.depth.write = false;
	} else if ( metadata.type == "depth" ) {
		descriptor.cullMode = VK_CULL_MODE_NONE;
	}
	return descriptor;
}

void ext::vulkan::RenderTargetRenderMode::initialize( Device& device ) {
	uint32_t width = this->width > 0 ? this->width : (ext::vulkan::settings::width * this->scale);
	uint32_t height = this->height > 0 ? this->height : (ext::vulkan::settings::height * this->scale);
	
	ext::vulkan::RenderMode::initialize( device );

	this->setTarget( this->getName() );
	uint8_t msaa = ext::vulkan::sampleCount(metadata.samples);
	if ( metadata.subpasses == 0 ) metadata.subpasses = 1;
	renderTarget.device = &device;
	renderTarget.views = metadata.views;
	
	//
	if ( metadata.type == "depth" ) {
		buffers.emplace_back().initialize( NULL, sizeof(pod::Camera::Viewports), uf::renderer::enums::Buffer::UNIFORM );
	}

	if ( metadata.type == "depth" || metadata.type == uf::renderer::settings::pipelines::names::vxgi ) {
		renderTarget.views = metadata.subpasses;
		struct {
			size_t depth;
		} attachments = {};

		attachments.depth = renderTarget.attach(RenderTarget::Attachment::Descriptor{
			/*.format = */ ext::vulkan::settings::formats::depth,
			/*.layout = */ VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			/*.usage = */ VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			/*.blend = */ false,
			/*.samples = */ 1,
		});
		metadata.attachments["depth"] = attachments.depth;

		for ( size_t currentPass = 0; currentPass < metadata.subpasses; ++currentPass ) {
			renderTarget.addPass(
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				{},
				{},
				{},
				attachments.depth,
				currentPass,
				true
			);
		}
	} else if ( metadata.type == settings::pipelines::names::rt )  {
	#if 1
		struct {
			size_t depth, color, bright, motion, scratch, output;
		} attachments = {};

		bool blend = true;
		attachments.depth = renderTarget.attach(RenderTarget::Attachment::Descriptor{
			/*.format = */ext::vulkan::settings::formats::depth,
			/*.layout = */VK_IMAGE_LAYOUT_GENERAL,
			/*.usage = */VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			/*.blend = */false,
			/*.samples = */1,
		});
		attachments.color = renderTarget.attach(RenderTarget::Attachment::Descriptor{
			/*.format =*/ ext::vulkan::settings::pipelines::hdr ? enums::Format::HDR : enums::Format::SDR,
			/*.layout = */ VK_IMAGE_LAYOUT_GENERAL,
			/*.usage =*/ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			/*.blend =*/ blend,
			/*.samples =*/ 1,
		});
		attachments.bright = renderTarget.attach(RenderTarget::Attachment::Descriptor{
			/*.format =*/ ext::vulkan::settings::pipelines::hdr ? enums::Format::HDR : enums::Format::SDR,
			/*.layout = */ VK_IMAGE_LAYOUT_GENERAL,
			/*.usage =*/ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			/*.blend =*/ blend,
			/*.samples =*/ 1,
		});
		attachments.scratch = renderTarget.attach(RenderTarget::Attachment::Descriptor{
			/*.format =*/ ext::vulkan::settings::pipelines::hdr ? enums::Format::HDR : enums::Format::SDR,
			/*.layout = */ VK_IMAGE_LAYOUT_GENERAL,
			/*.usage =*/ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			/*.blend =*/ blend,
			/*.samples =*/ 1,
		});
		attachments.motion = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		//	/*.format = */VK_FORMAT_R32G32B32A32_SFLOAT,
			/*.format = */VK_FORMAT_R16G16_SFLOAT,
			/*.layout = */ VK_IMAGE_LAYOUT_GENERAL,
			/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			/*.blend = */false,
			/*.samples = */1,
		});

		renderTarget.addPass(
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			{},
			{},
			{},
			attachments.depth,
			0,
			true
		);

		metadata.attachments["depth"] = attachments.depth;
		metadata.attachments["color"] = attachments.color;
		metadata.attachments["bright"] = attachments.bright;
		metadata.attachments["motion"] = attachments.motion;
		metadata.attachments["scratch"] = attachments.scratch;
		
		metadata.attachments["output"] = attachments.color;
	#endif
	} else if ( metadata.type == "full" ) {
	#if 0
		struct {
			size_t id, position, normal, depth, color;
		} attachments = {};

		// input g-buffers
		attachments.id = renderTarget.attach(RenderTarget::Attachment::Descriptor{
			/*.format = */VK_FORMAT_R32G32_UINT,
			/*.layout = */VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
			/*.blend = */false,
			/*.samples = */msaa,
		});
		attachments.position = renderTarget.attach(RenderTarget::Attachment::Descriptor{
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
		attachments.depth = renderTarget.attach(RenderTarget::Attachment::Descriptor{
			/*.format = */ext::vulkan::settings::formats::depth,
			/*.layout = */VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			/*.usage = */VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
			/*.blend = */false,
			/*.samples = */msaa,
		});
		// output buffers
		attachments.color = renderTarget.attach(RenderTarget::Attachment::Descriptor{
			/*.format =*/ ext::vulkan::settings::pipelines::hdr ? enums::Format::HDR : enums::Format::SDR,
			/*.layout = */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			/*.usage =*/ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			/*.blend =*/ blend,
			/*.samples =*/ 1,
		});

		metadata.attachments["id"] = attachments.id;
		metadata.attachments["position"] = attachments.position;
		metadata.attachments["normal"] = attachments.normal;
		
		metadata.attachments["depth"] = attachments.depth;
		metadata.attachments["color"] = attachments.color;
		metadata.attachments["output"] = attachments.color;

		// First pass: fill the G-Buffer
		for ( size_t eye = 0; eye < metadata.eyes; ++eye ) {
			renderTarget.addPass(
				/*.*/ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				/*.colors =*/ { attachments.id, attachments.uv, attachments.normal },
				/*.inputs =*/ {},
				/*.resolve =*/{},
				/*.depth = */ attachments.depth,
				/*.layer = */eye,
				/*.autoBuildPipeline =*/ true
			);
		}
	#endif
	} else {
		for ( size_t currentPass = 0; currentPass < metadata.subpasses; ++currentPass ) {
			struct {
				size_t albedo, depth;
			} attachments = {};

			attachments.albedo = renderTarget.attach(RenderTarget::Attachment::Descriptor{
				/*.format = */VK_FORMAT_R8G8B8A8_UNORM,
				/*.layout = */VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
				/*.blend = */true,
				/*.samples = */msaa,
			});
			attachments.depth = renderTarget.attach(RenderTarget::Attachment::Descriptor{
				/*.format = */ ext::vulkan::settings::formats::depth,
				/*.layout = */ VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				/*.usage = */ VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				/*.blend = */ false,
				/*.samples = */ 1,
			});
			renderTarget.addPass(
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				{ attachments.albedo },
				{},
				{},
				attachments.depth,
				0,
				true
			);

			metadata.attachments["albedo"] = attachments.albedo;
			metadata.attachments["depth"] = attachments.depth;
		}
	}

	renderTarget.initialize( device );

	if ( blitter.process ) {
		uf::Mesh mesh;
		mesh.vertex.count = 3;

		blitter.device = &device;
		blitter.material.device = &device;
		blitter.initializeMesh( mesh );
		if ( ext::json::isArray( metadata.json["shaders"] ) ) {
			ext::json::forEach( metadata.json["shaders"], [&]( ext::json::Value& value ){
				ext::vulkan::enums::Shader::type_t type{}; 
				uf::stl::string filename = "";
				uf::stl::string pipeline = "";

				if ( value.is<uf::stl::string>() ) {
					filename = value.as<uf::stl::string>();
					auto split = uf::string::split( filename, "." );
					uf::stl::string extension = split.back(); split.pop_back();
					uf::stl::string sType = split.back();
					
					if ( sType == "vert" ) type = ext::vulkan::enums::Shader::VERTEX;
					else if ( sType == "frag" ) type = ext::vulkan::enums::Shader::FRAGMENT;
					else if ( sType == "geom" ) type = ext::vulkan::enums::Shader::GEOMETRY;
					else if ( sType == "comp" ) type = ext::vulkan::enums::Shader::COMPUTE;
				} else {
					filename = value["filename"].as<uf::stl::string>();
					pipeline = value["pipeline"].as<uf::stl::string>();
					uf::stl::string sType = value["type"].as<uf::stl::string>();

					if ( sType == "vertex" ) type = ext::vulkan::enums::Shader::VERTEX;
					else if ( sType == "fragment" ) type = ext::vulkan::enums::Shader::FRAGMENT;
					else if ( sType == "geometry" ) type = ext::vulkan::enums::Shader::GEOMETRY;
					else if ( sType == "compute" ) type = ext::vulkan::enums::Shader::COMPUTE;
				}
				blitter.material.attachShader( uf::io::root+filename, type, pipeline );
			});
		} else if ( ext::json::isObject( metadata.json["shaders"] ) ) {
			ext::json::forEach( metadata.json["shaders"], [&]( const uf::stl::string& key, ext::json::Value& value ){
				ext::vulkan::enums::Shader::type_t type{}; 
				uf::stl::string filename = "";
				uf::stl::string pipeline = "";
				if ( value.is<uf::stl::string>() ) {
					filename = value.as<uf::stl::string>();
				} else {
					filename = value["filename"].as<uf::stl::string>();
					pipeline = value["pipeline"].as<uf::stl::string>();
				}
				if ( key == "vertex" ) type = ext::vulkan::enums::Shader::VERTEX;
				else if ( key == "fragment" ) type = ext::vulkan::enums::Shader::FRAGMENT;
				else if ( key == "geometry" ) type = ext::vulkan::enums::Shader::GEOMETRY;
				else if ( key == "compute" ) type = ext::vulkan::enums::Shader::COMPUTE;
				blitter.material.attachShader( uf::io::root+filename, type, pipeline );
			});
		} else if ( metadata.json["shaders"].is<bool>() && !metadata.json["shaders"].as<bool>() ) {
			// do not attach if we're requesting no blitter shaders
			blitter.process = false;
		} else {
			uf::stl::string vertexShaderFilename = uf::io::root+"/shaders/display/renderTarget/vert.spv";
			uf::stl::string fragmentShaderFilename = uf::io::root+"/shaders/display/renderTarget/frag.spv"; {
				std::pair<bool, uf::stl::string> settings[] = {
					{ settings::pipelines::postProcess, "postProcess.frag" },
				//	{ msaa > 1, "msaa.frag" },
				};
				FOR_ARRAY( settings ) if ( settings[i].first ) fragmentShaderFilename = uf::string::replace( fragmentShaderFilename, "frag", settings[i].second );
			}
			blitter.material.initializeShaders({
				{uf::io::resolveURI(vertexShaderFilename), VK_SHADER_STAGE_VERTEX_BIT},
				{uf::io::resolveURI(fragmentShaderFilename), VK_SHADER_STAGE_FRAGMENT_BIT}
			});
		}
		if ( metadata.type == uf::renderer::settings::pipelines::names::vxgi  ) {
			auto& scene = uf::scene::getCurrentScene();
			auto& sceneMetadataJson = scene.getComponent<uf::Serializer>();

			auto& shader = blitter.material.getShader("compute");
			
			size_t maxLights = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["lights"]["max"].as<size_t>(512);
			size_t maxTextures2D = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["textures"]["max"]["2D"].as<size_t>(512);
			size_t maxTexturesCube = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["textures"]["max"]["cube"].as<size_t>(128);
			size_t maxTextures3D = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["textures"]["max"]["3D"].as<size_t>(1);
			size_t maxCascades = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["vxgi"]["cascades"].as<size_t>(16);

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
		} else if ( metadata.type == uf::renderer::settings::pipelines::names::rt ) {
		#if 0
			auto& shader = blitter.material.getShader("fragment");
			shader.aliasAttachment("output", this);
		#endif
		} else {
			auto& shader = blitter.material.getShader("fragment");
			for ( auto i = 0; i < renderTarget.attachments.size(); ++i ) {
				if ( !(renderTarget.attachments[i].descriptor.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ) continue;

				for ( auto& pair : metadata.attachments ) {
					if ( pair.second != i ) continue;
					shader.aliasAttachment(pair.first, this);
					break;
				}
			}
		}
	}
}

void ext::vulkan::RenderTargetRenderMode::tick() {
	ext::vulkan::RenderMode::tick();

	bool resized = this->width == 0 && this->height == 0 && (ext::vulkan::states::resized || this->resized);
	bool rebuild = resized || ext::vulkan::states::rebuild || this->rebuild;

	uint32_t width = this->width > 0 ? this->width : (ext::vulkan::settings::width * this->scale);
	uint32_t height = this->height > 0 ? this->height : (ext::vulkan::settings::height * this->scale);

	if ( resized ) {
		this->resized = false;
		renderTarget.initialize( *renderTarget.device );
	}
	if ( rebuild && blitter.process ) {
		blitter.descriptor.bind.width = width;
		blitter.descriptor.bind.height = height;

		if ( !blitter.hasPipeline( blitter.descriptor ) ) {
			blitter.initializePipeline( blitter.descriptor );
		} else if ( blitter.hasPipeline( blitter.descriptor ) ){
			blitter.getPipeline( blitter.descriptor ).update( blitter, blitter.descriptor );
		}
	}
/*
	if ( metadata.limiter.frequency > 0 ) {
		if ( metadata.limiter.timer > metadata.limiter.frequency ) {
			metadata.limiter.timer = 0;
			metadata.limiter.execute = true;
		} else {
			metadata.limiter.timer = metadata.limiter.timer + uf::physics::time::delta;
			metadata.limiter.execute = false;
		}
	}
*/
}
void ext::vulkan::RenderTargetRenderMode::destroy() {
	ext::vulkan::RenderMode::destroy();
	blitter.destroy();
}

void ext::vulkan::RenderTargetRenderMode::render() {
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

	VK_CHECK_RESULT(vkQueueSubmit(device->getQueue( QueueEnum::GRAPHICS ), 1, &submitInfo, fences[states::currentBuffer]));

	if ( commandBufferCallbacks.count(EXECUTE_END) > 0 ) commandBufferCallbacks[EXECUTE_END]( VkCommandBuffer{}, 0 );

	this->executed = true;
	//unlockMutex( this->mostRecentCommandPoolId );
}
void ext::vulkan::RenderTargetRenderMode::pipelineBarrier( VkCommandBuffer commandBuffer, uint8_t state ) {
	ext::vulkan::RenderMode::pipelineBarrier( commandBuffer, state );
}
void ext::vulkan::RenderTargetRenderMode::createCommandBuffers( const uf::stl::vector<ext::vulkan::Graphic*>& graphics ) {
	// destroy if exists
	uint32_t width = this->width > 0 ? this->width : (ext::vulkan::settings::width * this->scale);
	uint32_t height = this->height > 0 ? this->height : (ext::vulkan::settings::height * this->scale);

	VkCommandBufferBeginInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufInfo.pNext = nullptr;

	auto& scene = uf::scene::getCurrentScene();
	auto& sceneMetadataJson = scene.getComponent<uf::Serializer>();

	uf::stl::vector<VkClearValue> clearValues;
	for ( size_t j = 0; j < renderTarget.views; ++j ) {
		for ( auto& attachment : renderTarget.attachments ) {
			auto& clearValue = clearValues.emplace_back();
			if ( attachment.descriptor.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ) {
				clearValue.color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
			} else if ( attachment.descriptor.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ) {
				if ( uf::matrix::reverseInfiniteProjection ) {
					clearValue.depthStencil = { 0.0f, 0 };
				} else {
					clearValue.depthStencil = { 1.0f, 0 };
				}
			}
		}
	}

	auto& commands = getCommands();
	for (size_t i = 0; i < commands.size(); ++i) {
		auto& commandBuffer = commands[i];
		VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));
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

			size_t subpasses = renderTarget.passes.size();
			size_t currentPass = 0;

			//
		//	this->pipelineBarrier( commands[i], 1 );

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
				uf::renderer::Texture::setImageLayout( commandBuffer, attachment.image, VK_IMAGE_LAYOUT_UNDEFINED, attachment.descriptor.layout, subresourceRange );
			}
		#endif

			// pre-renderpass commands
			if ( commandBufferCallbacks.count(CALLBACK_BEGIN) > 0 ) commandBufferCallbacks[CALLBACK_BEGIN]( commandBuffer, i );

			if ( this->getName() == "Compute" ) {
				for ( auto graphic : graphics ) {
					if ( graphic->descriptor.renderMode != this->getTarget() ) continue;
					if ( graphic->descriptor.pipeline != uf::renderer::settings::pipelines::names::rt ) continue;
					graphic->record( commandBuffer );
				}
			} else {
				for ( auto& pipeline : metadata.pipelines ) {
					if ( pipeline == metadata.pipeline ) continue;
					for ( auto graphic : graphics ) {
						if ( graphic->descriptor.renderMode != this->getTarget() ) continue;
						ext::vulkan::GraphicDescriptor descriptor = bindGraphicDescriptor(graphic->descriptor, currentPass);
						descriptor.pipeline = pipeline;
						if ( pipeline == uf::renderer::settings::pipelines::names::culling ) {
							descriptor.bind.width = graphic->descriptor.inputs.indirect.count;
							descriptor.bind.height = 1;
							descriptor.bind.depth = 1;
							descriptor.bind.point = VK_PIPELINE_BIND_POINT_COMPUTE;
						} else if ( pipeline == uf::renderer::settings::pipelines::names::rt ) {
							descriptor.bind.width = width;
							descriptor.bind.height = height;
							descriptor.bind.depth = 1;
							descriptor.bind.point = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
						} else {
							UF_MSG_DEBUG("Aux pipeline: {}", pipeline);
						}
						graphic->record( commandBuffer, descriptor, 0, metadata.type == uf::renderer::settings::pipelines::names::vxgi ? 0 : MIN(subpasses,6) );
					}
				}

				vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
					vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
					vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
					for ( ; currentPass < subpasses; ++currentPass ) {
						size_t currentDraw = 0;
						for ( auto graphic : graphics ) {
							if ( graphic->descriptor.renderMode != this->getTarget() ) continue;
							ext::vulkan::GraphicDescriptor descriptor = bindGraphicDescriptor(graphic->descriptor, currentPass);
							graphic->record( commandBuffer, descriptor, currentPass, currentDraw++ );
						}
						if ( commandBufferCallbacks.count( currentPass ) > 0 ) commandBufferCallbacks[currentPass]( commandBuffer, i );
						if ( currentPass + 1 < subpasses ) vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
					}
				vkCmdEndRenderPass(commandBuffer);
			}

			
			// post-renderpass commands
			if ( commandBufferCallbacks.count(CALLBACK_END) > 0 ) commandBufferCallbacks[CALLBACK_END]( commandBuffer, i );

		//	this->pipelineBarrier( commands[i], 1 );
		}

		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
	}
}

#endif