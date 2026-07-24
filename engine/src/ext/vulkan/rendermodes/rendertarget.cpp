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
#include <uf/engine/ext.h>
#include <uf/utils/io/fmt.h>
#include <uf/ext/openvr/openvr.h>

const uf::stl::string ext::vulkan::RenderTargetRenderMode::getType() const {
	return "RenderTarget";
}

ext::vulkan::GraphicDescriptor ext::vulkan::RenderTargetRenderMode::bindGraphicDescriptor( const ext::vulkan::GraphicDescriptor& reference, size_t pass ) {
	ext::vulkan::GraphicDescriptor descriptor = ext::vulkan::RenderMode::bindGraphicDescriptor(reference, pass);

	if ( 0 <= pass && pass < metadata.subpasses && metadata.type == uf::renderer::settings::pipelines::names::vxgi ) {
		descriptor.cullMode = VK_CULL_MODE_NONE;
		descriptor.depth.test = false;
		descriptor.depth.write = false;
	} else if ( metadata.type == "depth" ) {
		descriptor.cullMode = VK_CULL_MODE_NONE;
		if ( metadata.json["reverse depth"].as<bool>(uf::matrix::reverseInfiniteProjection) ) {
			descriptor.depth.operation = ext::vulkan::enums::Compare::GREATER_OR_EQUAL;
			descriptor.depth.min = 1.0f;
			descriptor.depth.max = 0.0f;
		} else {
			descriptor.depth.operation = ext::vulkan::enums::Compare::LESS;
			descriptor.depth.min = 0.0f;
			descriptor.depth.max = 1.0f;
		}
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
		UF_EXCEPTION("unimplemented");
	} else if ( metadata.type == "full" ) {
		UF_EXCEPTION("unimplemented");
	} else {
		struct {
			size_t color, depth;
		} attachments = {};

		attachments.color = renderTarget.attach(RenderTarget::Attachment::Descriptor{
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

		metadata.attachments["color"] = attachments.color;
		metadata.attachments["depth"] = attachments.depth;

		for ( size_t currentPass = 0; currentPass < metadata.subpasses; ++currentPass ) {
			renderTarget.addPass(
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				{ attachments.color },
				{},
				{},
				attachments.depth,
				0,
				true
			);
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
				if ( filename == "" ) return;
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
				if ( filename == "" ) return;
				blitter.material.attachShader( uf::io::root+filename, type, pipeline );
			});
		} else if ( metadata.json["shaders"].is<bool>() && !ext::json::isNull( metadata.json["shaders"] ) ) {
			// do not attach if we're requesting no blitter shaders
			blitter.process = metadata.json["shaders"].as<bool>();
		} else {
			uf::stl::string vertexShaderFilename = uf::io::root+"/shaders/display/renderTarget/vert.spv";
			uf::stl::string fragmentShaderFilename = uf::io::root+"/shaders/display/renderTarget/frag.spv"; {
				uf::stl::string postProcess = FMT_FORMAT("{}.frag", metadata.json["postProcess"].as<uf::stl::string>("postProcess"));
				
				std::pair<bool, uf::stl::string> settings[] = {
					{ settings::pipelines::postProcess, postProcess },
				//	{ msaa > 1, "msaa.frag" },
				};
				FOR_ARRAY( settings ) if ( settings[i].first ) fragmentShaderFilename = uf::string::replace( fragmentShaderFilename, "frag", settings[i].second );
			}
			blitter.material.initializeShaders({
				{uf::io::resolveURI(vertexShaderFilename), VK_SHADER_STAGE_VERTEX_BIT},
				{uf::io::resolveURI(fragmentShaderFilename), VK_SHADER_STAGE_FRAGMENT_BIT}
			});
		}

		if ( metadata.type == uf::renderer::settings::pipelines::names::vxgi ) {
			// handled by its rendermode
		} else if ( metadata.type == uf::renderer::settings::pipelines::names::rt ) {
			// handled by its rendermode
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
	#if UF_USE_OPENVR
		if ( ext::openvr::enabled && metadata.json["vr"].as<bool>() ) {
			uf::stl::string vertexShaderFilename = uf::io::resolveURI(FMT_FORMAT("{}/shaders/display/vr/{}.vert.spv", uf::io::root, metadata.json["stereo"].as<bool>(metadata.views == 2) ? "stereo" : "flat"));
			uf::stl::string fragmentShaderFilename = uf::io::resolveURI(FMT_FORMAT("{}/shaders/display/vr/{}.frag.spv", uf::io::root, metadata.json["stereo"].as<bool>(metadata.views == 2) ? "stereo" : "flat"));

			blitter.material.attachShader(vertexShaderFilename, uf::renderer::enums::Shader::VERTEX, "vr");
			blitter.material.attachShader(fragmentShaderFilename, uf::renderer::enums::Shader::FRAGMENT, "vr");

			{
				auto& shader = blitter.material.getShader("fragment", "vr");
				shader.aliasAttachment("color", this);
			}

			{
				pod::Transform<> transform = {};
				transform.position = {0, 0, -3};
				transform.scale = { 1, -1, 1 };
				transform.orientation = {0, 0, 0, 1};
				metadata.camera.setTransform(transform);
				metadata.camera.setProjection( ext::openvr::hmdProjectionMatrix(0, 0.001f, 0.0f), 0 );
				metadata.camera.setProjection( ext::openvr::hmdProjectionMatrix(1, 0.001f, 0.0f), 1 );
			}
		}
	#endif
	}

	this->build(true);
}

void ext::vulkan::RenderTargetRenderMode::build( bool resized ) {
	ext::vulkan::RenderMode::build();

	uint32_t width = this->width > 0 ? this->width : (ext::vulkan::settings::width * this->scale);
	uint32_t height = this->height > 0 ? this->height : (ext::vulkan::settings::height * this->scale);
	auto mips = uf::vector::mips( pod::Vector2ui{ width, height } );
	
	auto& scene = uf::scene::getCurrentScene();
	auto& sceneMetadataJson = scene.getComponent<uf::Serializer>();
	auto& storage = uf::graph::getStorage( scene );

	// (re)bind aliases
	if ( metadata.type == uf::renderer::settings::pipelines::names::vxgi ) {
		auto& shader = blitter.material.getShader("compute");

	//	shader.aliasBuffer( storage.buffers.camera );
	//	shader.aliasBuffer( storage.buffers.joint );
		shader.aliasBuffer( storage.buffers.drawCommands );
		shader.aliasBuffer( storage.buffers.instance );
		shader.aliasBuffer( storage.buffers.addresses );
		shader.aliasBuffer( storage.buffers.object );
		shader.aliasBuffer( storage.buffers.material );
		shader.aliasBuffer( storage.buffers.texture );
		shader.aliasBuffer( storage.buffers.light );
	}

	// (re)initialize pipelines
	if ( blitter.process ) {
		blitter.descriptor.bind.width = width;
		blitter.descriptor.bind.height = height;

		blitter.update( blitter.descriptor );
	}
	
	if ( metadata.type == uf::renderer::settings::pipelines::names::vxgi ) {
		auto descriptor = blitter.descriptor;
		//descriptor.pipeline = "lighting";
		descriptor.subpass = -1;
		descriptor.bind.point = VK_PIPELINE_BIND_POINT_COMPUTE;
		blitter.update( descriptor );
	}

	if ( metadata.type == uf::renderer::settings::pipelines::names::vxgi ) {
		auto descriptor = blitter.descriptor;
		descriptor.pipeline = "mipmap";
		descriptor.subpass = -1;
		descriptor.bind.point = VK_PIPELINE_BIND_POINT_COMPUTE;
		blitter.update( descriptor );
	}

	if ( ext::openvr::enabled && metadata.json["vr"].as<bool>() ) {
		auto descriptor = blitter.descriptor;
		descriptor.pipeline = "vr";
		descriptor.renderMode = "VR";
		descriptor.bind.point = VK_PIPELINE_BIND_POINT_GRAPHICS;
		descriptor.depth.test = false;
		descriptor.cullMode = uf::renderer::enums::CullMode::NONE;
		blitter.update( descriptor );
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
		this->build( resized );
	}
	
	if ( ext::openvr::enabled && metadata.json["vr"].as<bool>() ) {
		auto& shader = blitter.material.getShader("vertex", "vr");
		metadata.camera.update();
		if ( shader.hasUniform("UBO") ) {
			shader.updateBuffer( (const void*) &metadata.camera.data().viewport, sizeof(pod::Camera::Viewports), shader.getUniformBuffer("UBO") );
		}
	}
}
void ext::vulkan::RenderTargetRenderMode::destroy() {
	ext::vulkan::RenderMode::destroy();
}

void ext::vulkan::RenderTargetRenderMode::render() {
	if ( this->commands.container().empty() ) return;

	VK_COMMAND_BUFFER_CALLBACK( EXECUTE_BEGIN, VkCommandBuffer{}, 0, {} );

	VkSubmitInfo submitInfo = this->queue();
	VkQueue queue = device->getQueue( QueueEnum::GRAPHICS );
	VkResult res = vkQueueSubmit( queue, 1, &submitInfo, /*VK_NULL_HANDLE*/fences[states::currentBuffer]);
	VK_CHECK_QUEUE_CHECKPOINT( queue, res );
	VK_COMMAND_BUFFER_CALLBACK( EXECUTE_END, VkCommandBuffer{}, 0, {} );

	this->executed = true;
}
void ext::vulkan::RenderTargetRenderMode::createCommandBuffers( const uf::stl::vector<ext::vulkan::Graphic*>& graphics ) {
	// destroy if exists
	uint32_t width = this->width > 0 ? this->width : (ext::vulkan::settings::width * this->scale);
	uint32_t height = this->height > 0 ? this->height : (ext::vulkan::settings::height * this->scale);

	VkCommandBufferBeginInfo cmdBufInfo = ext::vulkan::initializers::commandBufferBeginInfo();
	cmdBufInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	auto& scene = uf::scene::getCurrentScene();
	auto& sceneMetadataJson = scene.getComponent<uf::Serializer>();

	uf::stl::vector<VkClearValue> clearValues;
	float depthClear = uf::matrix::reverseInfiniteProjection ? 0.0f : 1.0f;
	for ( auto graphic : graphics ) {
		auto descriptor = bindGraphicDescriptor(graphic->descriptor);
		depthClear = descriptor.depth.max;
		break;
	}
	for ( auto& attachment : renderTarget.attachments ) {
		auto& clearValue = clearValues.emplace_back();
		if ( attachment.descriptor.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT ) {
			clearValue.color = { { 0.0f, 0.0f, 0.0f, 0.0f } };
		} else if ( attachment.descriptor.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ) {
			clearValue.depthStencil = { depthClear, 0 };
		}
	}

	auto& commands = getCommands();
	for (size_t frame = 0; frame < commands.size(); ++frame) {
		auto& commandBuffer = commands[frame];
		VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo));
		device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::BEGIN, "begin" );
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

			size_t subpasses = renderTarget.passes.size();
			size_t currentPass = 0;

			// pre-renderpass commands
			VK_COMMAND_BUFFER_CALLBACK( CALLBACK_BEGIN, commandBuffer, frame, {
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "callback[begin]" );
			} );

			if ( this->getName() == "Compute" ) {
				for ( auto graphic : graphics ) {
					if ( graphic->descriptor.renderMode != this->getTarget() ) continue;
					if ( graphic->descriptor.pipeline != uf::renderer::settings::pipelines::names::rt ) continue;
					device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, FMT_FORMAT("graphic[{}]", graphic->descriptor.pipeline) );
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
						device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, FMT_FORMAT("graphic[{}]", pipeline) );
						graphic->record( commandBuffer, descriptor, 0, metadata.type == uf::renderer::settings::pipelines::names::vxgi ? 0 : MIN(subpasses,6), frame );
					}
				}

				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::BEGIN, "renderPass[begin]" );
				vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
					vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
					vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
					for ( ; currentPass < subpasses; ++currentPass ) {
						size_t currentDraw = 0;
						for ( auto graphic : graphics ) {
							if ( graphic->descriptor.renderMode != this->getTarget() ) continue;
							ext::vulkan::GraphicDescriptor descriptor = bindGraphicDescriptor(graphic->descriptor, currentPass);
							device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, FMT_FORMAT("graphic[{}]", currentDraw) );
							graphic->record( commandBuffer, descriptor, currentPass, currentDraw++, frame );
						}

						VK_COMMAND_BUFFER_CALLBACK( currentPass, commandBuffer, frame, {
							device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, FMT_FORMAT("callback[{}]", currentPass) );
						} );

						if ( currentPass + 1 < subpasses ) {
							device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "nextSubpass" );
							vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
						}
					}
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::END, "renderPass[end]" );
				vkCmdEndRenderPass(commandBuffer);
			}
			
			// post-renderpass commands
			VK_COMMAND_BUFFER_CALLBACK( CALLBACK_END, commandBuffer, frame, {
				device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::GENERIC, "callback[end]" );
			} );

		#if 0 && UF_USE_OPENVR
			if ( ext::vulkan::hasRenderMode("VR") ) {
			// transition outputs
				auto& outputAttachment = this->getAttachment("color");

				VkImageSubresourceRange range = {};
				range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				range.baseMipLevel = 0;
				range.levelCount = 1;
				range.baseArrayLayer = 0;
				range.layerCount = 1;

				uf::renderer::Texture::setImageLayout( commandBuffer, outputAttachment.image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, range );
			}
		#endif
		}
		device->UF_CHECKPOINT_MARK( commandBuffer, pod::Checkpoint::END, "end" );
		VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
	}
}

#endif