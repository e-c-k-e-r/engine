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

const uf::stl::string ext::vulkan::RenderTargetRenderMode::getTarget() const {
//	auto& metadata = *const_cast<uf::Serializer*>(&this->metadata);
//	return metadata["target"].as<uf::stl::string>();
	return metadata.target;
}
void ext::vulkan::RenderTargetRenderMode::setTarget( const uf::stl::string& target ) {
//	this->metadata["target"] = target;
	metadata.target = target;
}

const uf::stl::string ext::vulkan::RenderTargetRenderMode::getType() const {
	return "RenderTarget";
}
const size_t ext::vulkan::RenderTargetRenderMode::blitters() const {
	return 1;
}
ext::vulkan::Graphic* ext::vulkan::RenderTargetRenderMode::getBlitter( size_t i ) {
	return &this->blitter;
}
uf::stl::vector<ext::vulkan::Graphic*> ext::vulkan::RenderTargetRenderMode::getBlitters() {
	return { &this->blitter };
}

ext::vulkan::GraphicDescriptor ext::vulkan::RenderTargetRenderMode::bindGraphicDescriptor( const ext::vulkan::GraphicDescriptor& reference, size_t pass ) {
	ext::vulkan::GraphicDescriptor descriptor = ext::vulkan::RenderMode::bindGraphicDescriptor(reference, pass);
	descriptor.parse(metadata.json["descriptor"]);

	if ( 0 <= pass && pass < metadata.subpasses && metadata.type == uf::renderer::settings::pipelines::names::vxgi ) {
		descriptor.cullMode = VK_CULL_MODE_NONE;
		descriptor.depth.test = false;
		descriptor.depth.write = false;
	} else if ( metadata.type == "depth" ) {
		descriptor.cullMode = VK_CULL_MODE_NONE;
	}
	// invalidate
	if ( metadata.target != "" && descriptor.renderMode != this->getName() && descriptor.renderMode != metadata.target ) {
		descriptor.invalidated = true;
	} else {
		descriptor.renderMode = this->getName();
	}
	return descriptor;
}

void ext::vulkan::RenderTargetRenderMode::initialize( Device& device ) {
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
	} else {
		for ( size_t currentPass = 0; currentPass < metadata.subpasses; ++currentPass ) {
			if ( metadata.type == "single" ) {
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
			} else {
				struct {
					size_t id, normals, uvs, albedo, depth, output;
				} attachments = {};

				attachments.id = renderTarget.attach(RenderTarget::Attachment::Descriptor{
					/*.format = */VK_FORMAT_R16G16_UINT,
					/*.layout = */VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
					/*.blend = */false,
					/*.samples = */msaa,
				});
				attachments.normals = renderTarget.attach(RenderTarget::Attachment::Descriptor{
					/*.format = */VK_FORMAT_R16G16_SFLOAT,
					/*.layout = */VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
					/*.blend = */false,
					/*.samples = */msaa,
				});
				if ( !true && ext::vulkan::settings::invariant::deferredMode != "" ) {
					attachments.uvs = renderTarget.attach(RenderTarget::Attachment::Descriptor{
						/*.format = */VK_FORMAT_R16G16B16A16_UNORM,
						/*.layout = */VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
						/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
						/*.blend = */false,
						/*.samples = */msaa,
					});
				} else {
					attachments.albedo = renderTarget.attach(RenderTarget::Attachment::Descriptor{
						/*.format = */VK_FORMAT_R8G8B8A8_UNORM,
						/*.layout = */VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
						/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
						/*.blend = */true,
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
				attachments.output = renderTarget.attach(RenderTarget::Attachment::Descriptor{
					/*.format =*/ VK_FORMAT_R8G8B8A8_UNORM,
					/*.layout = */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					/*.usage =*/ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
					/*.blend =*/ true,
					/*.samples =*/ 1,
				});
				if ( !true && ext::vulkan::settings::invariant::deferredMode != "" ) {
					// First pass: fill the G-Buffer
					{
						renderTarget.addPass(
							VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
							{ attachments.id, attachments.normals, attachments.uvs },
							{},
							{},
							attachments.depth,
							0,
							true
						);
					}
					// Second pass: write to output
					{
						renderTarget.addPass(
							VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
							{ attachments.output },
							{ attachments.id, attachments.normals, attachments.uvs, attachments.depth },
							{},
							attachments.depth,
							0,
							false
						);
					}
				} else {
					// First pass: fill the G-Buffer
					{
						renderTarget.addPass(
							VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
							{ attachments.id, attachments.normals, attachments.albedo },
							{},
							{},
							attachments.depth,
							0,
							true
						);
					}
					// Second pass: write to output
					{
						renderTarget.addPass(
							VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
							{ attachments.output },
							{ attachments.id, attachments.normals, attachments.albedo, attachments.depth },
							{},
							attachments.depth,
							0,
							false
						);
					}
				}
				metadata.outputs.emplace_back(attachments.output);
			}
		}
	}

	renderTarget.initialize( device );

	if ( blitter.process ) {
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
			uf::stl::string vertexShaderFilename = uf::io::root+"/shaders/display/renderTarget.vert.spv";
			uf::stl::string fragmentShaderFilename = uf::io::root+"/shaders/display/renderTarget.frag.spv"; {
				std::pair<bool, uf::stl::string> settings[] = {
					{ msaa > 1, "msaa.frag" },
				// I don't actually have support for deferred sampling within a render target
				//	{ uf::renderer::settings::invariant::deferredSampling, "deferredSampling.frag" },
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
			size_t maxLights = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["lights"]["max"].as<size_t>(512);
			size_t maxTextures2D = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["textures"]["max"]["2D"].as<size_t>(512);
			size_t maxTexturesCube = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["textures"]["max"]["cube"].as<size_t>(128);
			size_t maxTextures3D = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["textures"]["max"]["3D"].as<size_t>(1);
			size_t maxCascades = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["vxgi"]["cascades"].as<size_t>(16);

			auto& shader = blitter.material.getShader("compute");
		//	shader.buffers.emplace_back( uf::graph::storage.buffers.camera.alias() );
		//	shader.buffers.emplace_back( uf::graph::storage.buffers.joint.alias() );
			shader.buffers.emplace_back( uf::graph::storage.buffers.drawCommands.alias() );
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
		} else if ( metadata.type != uf::renderer::settings::pipelines::names::rt ) {
			for ( auto& attachment : renderTarget.attachments ) {
				if ( !(attachment.descriptor.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ) continue;

				Texture2D& texture = blitter.material.textures.emplace_back();
				enums::Filter::type_t filter = VK_FILTER_NEAREST;

				texture.sampler.descriptor.filter.min = filter;
				texture.sampler.descriptor.filter.mag = filter;
				texture.aliasAttachment(attachment);
			}
		}
	/*
		if ( blitter.process ) {
			auto& mainRenderMode = ext::vulkan::getRenderMode("");
			size_t eyes = mainRenderMode.metadata["eyes"].as<size_t>();
			for ( size_t eye = 0; eye < eyes; ++eye ) {
				ext::vulkan::GraphicDescriptor descriptor = blitter.descriptor;
				descriptor.subpass = 2 * eye + 1;
				if ( blitter.hasPipeline( descriptor ) ) continue;
				blitter.initializePipeline( descriptor );
			}
		}
	*/
	}
}

void ext::vulkan::RenderTargetRenderMode::tick() {
	ext::vulkan::RenderMode::tick();

	bool resized = this->width == 0 && this->height == 0 && ext::vulkan::states::resized;
	bool rebuild = resized || ext::vulkan::states::rebuild || this->rebuild;

	if ( metadata.type == uf::renderer::settings::pipelines::names::vxgi ) {
		if ( resized ) {
			renderTarget.initialize( *renderTarget.device );
		}
		if ( rebuild && blitter.process ) {
			blitter.getPipeline().update( blitter );
		}
		return;
	}
	if ( resized ) {
		renderTarget.initialize( *renderTarget.device );
		if ( metadata.type != uf::renderer::settings::pipelines::names::rt ) {
			blitter.material.textures.clear();
			for ( auto& attachment : renderTarget.attachments ) {
				if ( !(attachment.descriptor.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ) continue;
				Texture2D& texture = blitter.material.textures.emplace_back();
				enums::Filter::type_t filter = VK_FILTER_NEAREST;

				texture.sampler.descriptor.filter.min = filter;
				texture.sampler.descriptor.filter.mag = filter;
				texture.aliasAttachment(attachment);
			}
		}
	}
	if ( rebuild && blitter.process ) {
		auto& mainRenderMode = ext::vulkan::getRenderMode("");
		size_t eyes = mainRenderMode.metadata.eyes;
		for ( size_t eye = 0; eye < eyes; ++eye ) {
			ext::vulkan::GraphicDescriptor descriptor = blitter.descriptor;
			descriptor.subpass = 2 * eye + 1;
			if ( !blitter.hasPipeline( descriptor ) ) {
				blitter.initializePipeline( descriptor );
			} else {
				blitter.getPipeline( descriptor ).update( blitter, descriptor );
			}
		}
	}

	if ( metadata.limiter.frequency > 0 ) {
		if ( metadata.limiter.timer > metadata.limiter.frequency ) {
			metadata.limiter.timer = 0;
			metadata.limiter.execute = true;
		} else {
			metadata.limiter.timer = metadata.limiter.timer + uf::physics::time::delta;
			metadata.limiter.execute = false;
		}
	}
}
void ext::vulkan::RenderTargetRenderMode::destroy() {
	ext::vulkan::RenderMode::destroy();
	blitter.destroy();
}

void ext::vulkan::RenderTargetRenderMode::render() {
	if ( commandBufferCallbacks.count(EXECUTE_BEGIN) > 0 ) commandBufferCallbacks[EXECUTE_BEGIN]( VkCommandBuffer{} );

	//lockMutex( this->mostRecentCommandPoolId );
	auto& commands = getCommands( this->mostRecentCommandPoolId );
	// Submit commands
	// Use a fence to ensure that command buffer has finished executing before using it again
	VK_CHECK_RESULT(vkWaitForFences( *device, 1, &fences[states::currentBuffer], VK_TRUE, UINT64_MAX ));
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

	VK_CHECK_RESULT(vkQueueSubmit(device->getQueue( Device::QueueEnum::GRAPHICS ), 1, &submitInfo, fences[states::currentBuffer]));

	if ( commandBufferCallbacks.count(EXECUTE_END) > 0 ) commandBufferCallbacks[EXECUTE_END]( VkCommandBuffer{} );

	this->executed = true;
	//unlockMutex( this->mostRecentCommandPoolId );
}
void ext::vulkan::RenderTargetRenderMode::pipelineBarrier( VkCommandBuffer commandBuffer, uint8_t state ) {
	VkImageMemoryBarrier imageMemoryBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.graphics; //VK_QUEUE_FAMILY_IGNORED
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // ext::vulkan::device.queueFamilyIndices.graphics; //VK_QUEUE_FAMILY_IGNORED
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
	imageMemoryBarrier.subresourceRange.levelCount = 1;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
	imageMemoryBarrier.subresourceRange.layerCount = 1;
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	for ( auto& attachment : renderTarget.attachments ) {
		if ( !(attachment.descriptor.usage & VK_IMAGE_USAGE_SAMPLED_BIT) ) continue;
		if (  (attachment.descriptor.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) ) continue;
		
		VkPipelineStageFlags srcStageMask, dstStageMask;
		imageMemoryBarrier.image = attachment.image;
		imageMemoryBarrier.oldLayout = attachment.descriptor.layout;
		imageMemoryBarrier.newLayout = attachment.descriptor.layout;
	
		switch ( state ) {
			case 0: {
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			} break;
			case 1: {
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				imageMemoryBarrier.newLayout = attachment.descriptor.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			} break;
			// ensure 
			default: {
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
				dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			}
		}
		
		vkCmdPipelineBarrier( commandBuffer,
			srcStageMask, dstStageMask,
			VK_FLAGS_NONE,
			0, NULL,
			0, NULL,
			1, &imageMemoryBarrier
		);

		attachment.descriptor.layout = imageMemoryBarrier.newLayout;
	}
}
void ext::vulkan::RenderTargetRenderMode::createCommandBuffers( const uf::stl::vector<ext::vulkan::Graphic*>& graphics ) {
	// destroy if exists
	float width = this->width > 0 ? this->width : ext::vulkan::settings::width;
	float height = this->height > 0 ? this->height : ext::vulkan::settings::height;

	VkCommandBufferBeginInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufInfo.pNext = nullptr;
	auto& commands = getCommands();
	for (size_t i = 0; i < commands.size(); ++i) {
		VK_CHECK_RESULT(vkBeginCommandBuffer(commands[i], &cmdBufInfo));
		{
			uf::stl::vector<VkClearValue> clearValues;
			for ( size_t j = 0; j < renderTarget.views; ++j ) {
				for ( auto& attachment : renderTarget.attachments ) {
					VkClearValue clearValue;
					if ( attachment.descriptor.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ) {
						clearValue.color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
					} else if ( attachment.descriptor.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ) {
						if ( uf::matrix::reverseInfiniteProjection ) {
							clearValue.depthStencil = { 0.0f, 0 };
						} else {
							clearValue.depthStencil = { 1.0f, 0 };
						}
					}
					clearValues.push_back(clearValue);
				}
			}

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
			// pre-renderpass commands
			if ( commandBufferCallbacks.count(CALLBACK_BEGIN) > 0 ) commandBufferCallbacks[CALLBACK_BEGIN]( commands[i] );
			
			if ( this->getName() == "Compute" ) {
				for ( auto graphic : graphics ) {
					if ( graphic->descriptor.renderMode != this->getTarget() ) continue;
					if ( graphic->descriptor.pipeline != uf::renderer::settings::pipelines::names::rt ) continue;
					graphic->record( commands[i] );
				}
			} else {
				for ( auto& pipeline : metadata.pipelines ) {
					if ( pipeline == metadata.pipeline ) continue;
					for ( auto graphic : graphics ) {
						if ( graphic->descriptor.renderMode != this->getTarget() ) continue;
						ext::vulkan::GraphicDescriptor descriptor = bindGraphicDescriptor(graphic->descriptor, currentPass);
						descriptor.pipeline = pipeline;
						graphic->record( commands[i], descriptor, 0, metadata.type == uf::renderer::settings::pipelines::names::vxgi ? 0 : MIN(subpasses,6) );
					}
				}

				vkCmdBeginRenderPass(commands[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
					vkCmdSetViewport(commands[i], 0, 1, &viewport);
					vkCmdSetScissor(commands[i], 0, 1, &scissor);
					for ( ; currentPass < subpasses; ++currentPass ) {
						size_t currentDraw = 0;
						for ( auto graphic : graphics ) {
							if ( graphic->descriptor.renderMode != this->getTarget() ) continue;
							ext::vulkan::GraphicDescriptor descriptor = bindGraphicDescriptor(graphic->descriptor, currentPass);
							graphic->record( commands[i], descriptor, currentPass, currentDraw++ );
						}
						if ( commandBufferCallbacks.count( currentPass ) > 0 ) commandBufferCallbacks[currentPass]( commands[i] );
						if ( currentPass + 1 < subpasses ) vkCmdNextSubpass(commands[i], VK_SUBPASS_CONTENTS_INLINE);
					}
				vkCmdEndRenderPass(commands[i]);
			}
			
			// post-renderpass commands
			if ( commandBufferCallbacks.count(CALLBACK_END) > 0 ) commandBufferCallbacks[CALLBACK_END]( commands[i] );
		}

		VK_CHECK_RESULT(vkEndCommandBuffer(commands[i]));
	}
}

#endif