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
#if 0
	if ( metadata.eyes == 0 ) metadata.eyes = 1;
	{
		float width = this->width > 0 ? this->width : ext::vulkan::settings::width;
		float height = this->height > 0 ? this->height : ext::vulkan::settings::height;
		auto& swapchainRender = ext::vulkan::getRenderMode("Swapchain");
		if ( swapchainRender.renderTarget.width != width || swapchainRender.renderTarget.height != height ) {
			settings::invariant::deferredAliasOutputToSwapchain = false;
		}
	}
	if ( settings::pipelines::bloom ) settings::invariant::deferredAliasOutputToSwapchain = false;
#endif

	// buffers.emplace_back().initialize( NULL, sizeof(pod::Camera::Viewports), uf::renderer::enums::Buffer::UNIFORM );


	auto HDR_FORMAT = VK_FORMAT_R32G32B32A32_SFLOAT;
	auto SDR_FORMAT = VK_FORMAT_R16G16B16A16_SFLOAT;

	ext::vulkan::RenderMode::initialize( device );
	renderTarget.device = &device;
	renderTarget.views = metadata.eyes;
	size_t msaa = ext::vulkan::settings::msaa;

#if 0
	struct {
		size_t id, bary, mips, depth, color, bright, scratch;
	} attachments = {};

	bool blend = true; // !ext::vulkan::settings::invariant::deferredSampling;
	attachments.id = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format = */VK_FORMAT_R16G16_UINT,
		/*.layout = */VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
		/*.blend = */false,
		/*.samples = */msaa,
	});
	attachments.bary = renderTarget.attach(RenderTarget::Attachment::Descriptor{
	//	/*.format = */VK_FORMAT_R16G16_SNORM,
		/*.format = */VK_FORMAT_R16G16_SFLOAT,
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
	attachments.depth = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format = */ext::vulkan::settings::formats::depth,
		/*.layout = */VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		/*.usage = */VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
		/*.blend = */false,
		/*.samples = */msaa,
	});
	attachments.color = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format =*/ ext::vulkan::settings::pipelines::hdr ? HDR_FORMAT : SDR_FORMAT,
		/*.layout = */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		/*.usage =*/ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		/*.blend =*/ blend,
		/*.samples =*/ 1,
	});
	attachments.bright = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format =*/ ext::vulkan::settings::pipelines::hdr ? HDR_FORMAT : SDR_FORMAT,
		/*.layout = */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		/*.usage =*/ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		/*.blend =*/ blend,
		/*.samples =*/ 1,
	});
	attachments.scratch = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format =*/ ext::vulkan::settings::pipelines::hdr ? HDR_FORMAT : SDR_FORMAT,
		/*.layout = */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		/*.usage =*/ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		/*.blend =*/ blend,
		/*.samples =*/ 1,
	});
	for ( size_t eye = 0; eye < metadata.eyes; ++eye ) {
		if ( ext::vulkan::settings::invariant::deferredSampling ) {
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
#else
	struct {
		size_t id, bary, mips, normals, uvs, depth, color, bright, scratch, output;
	} attachments = {};

	bool blend = true; // !ext::vulkan::settings::invariant::deferredSampling;
	attachments.id = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format = */VK_FORMAT_R16G16B16A16_UINT,
		/*.layout = */VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
		/*.blend = */false,
		/*.samples = */msaa,
	});
	attachments.bary = renderTarget.attach(RenderTarget::Attachment::Descriptor{
	//	/*.format = */VK_FORMAT_R32G32B32A32_SFLOAT,
		/*.format = */VK_FORMAT_R16G16_SNORM,
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
	attachments.normals = renderTarget.attach(RenderTarget::Attachment::Descriptor{
	//	/*.format = */VK_FORMAT_R16G16_SFLOAT,
		/*.format = */VK_FORMAT_R16G16_SNORM,
		/*.layout = */VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
		/*.blend = */false,
		/*.samples = */msaa,
	});
	attachments.uvs = renderTarget.attach(RenderTarget::Attachment::Descriptor{
	//	/*.format = */VK_FORMAT_R32G32B32A32_SFLOAT,
		/*.format = */VK_FORMAT_R16G16B16A16_SFLOAT,
		/*.layout = */VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		/*.usage = */VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
		/*.blend = */false,
		/*.samples = */msaa,
	});
	attachments.depth = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format = */ext::vulkan::settings::formats::depth,
		/*.layout = */VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		/*.usage = */VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
		/*.blend = */false,
		/*.samples = */msaa,
	});
	attachments.color = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format =*/ ext::vulkan::settings::pipelines::hdr ? HDR_FORMAT : SDR_FORMAT,
		/*.layout = */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		/*.usage =*/ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		/*.blend =*/ blend,
		/*.samples =*/ 1,
	});
	attachments.bright = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format =*/ ext::vulkan::settings::pipelines::hdr ? HDR_FORMAT : SDR_FORMAT,
		/*.layout = */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		/*.usage =*/ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		/*.blend =*/ blend,
		/*.samples =*/ 1,
	});
	attachments.scratch = renderTarget.attach(RenderTarget::Attachment::Descriptor{
		/*.format =*/ ext::vulkan::settings::pipelines::hdr ? HDR_FORMAT : SDR_FORMAT,
		/*.layout = */ VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		/*.usage =*/ VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		/*.blend =*/ blend,
		/*.samples =*/ 1,
	});

	metadata.attachments["id"] = attachments.id;
	metadata.attachments["bary"] = attachments.bary;
	metadata.attachments["mips"] = attachments.mips;
	metadata.attachments["normals"] = attachments.normals;
	metadata.attachments["uvs"] = attachments.uvs;
	metadata.attachments["depth"] = attachments.depth;
	metadata.attachments["color"] = attachments.color;
	metadata.attachments["bright"] = attachments.bright;
	metadata.attachments["scratch"] = attachments.scratch;

	for ( size_t eye = 0; eye < metadata.eyes; ++eye ) {
		// First pass: fill the G-Buffer
		{
			renderTarget.addPass(
				/*.*/ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				/*.colors =*/ { attachments.id, attachments.bary, attachments.mips, attachments.normals, attachments.uvs },
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
				/*.inputs =*/ { attachments.id, attachments.bary, attachments.mips, attachments.normals, attachments.uvs, attachments.depth },
				/*.resolve =*/{},
				/*.depth = */attachments.depth,
				/*.layer = */eye,
				/*.autoBuildPipeline =*/ false
			);
		}
	}
#endif

	metadata.outputs.emplace_back(attachments.color);
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
		auto& scene = uf::scene::getCurrentScene();
		auto& sceneMetadataJson = scene.getComponent<uf::Serializer>();

		blitter.descriptor.renderMode = "Swapchain";
		blitter.descriptor.subpass = 0;
		blitter.descriptor.depth.test = false;
		blitter.descriptor.depth.write = false;

		blitter.initialize( "Swapchain" );
		blitter.initializeMesh( mesh );

		{
			uf::stl::string vertexShaderFilename = uf::io::root+"/shaders/display/renderTarget.vert.spv";
			uf::stl::string fragmentShaderFilename = uf::io::root+"/shaders/display/renderTarget.frag.spv";
			{
				std::pair<bool, uf::stl::string> settings[] = {
					{ !settings::pipelines::rt, "postProcess.frag" },
				//	{ msaa > 1, "msaa.frag" },
				};
				FOR_ARRAY( settings ) if ( settings[i].first ) fragmentShaderFilename = uf::string::replace( fragmentShaderFilename, "frag", settings[i].second );
			}
			blitter.material.initializeShaders({
				{uf::io::resolveURI(vertexShaderFilename), uf::renderer::enums::Shader::VERTEX},
				{uf::io::resolveURI(fragmentShaderFilename), uf::renderer::enums::Shader::FRAGMENT}
			});
		}

		{
			size_t index = metadata.attachments["color"]; // uf::renderer::settings::pipelines::deferred ? metadata.attachments["color"] : metadata.attachments["albedo"]; // deferred ? attachments.color : attachments.albedo;
			auto& shader = blitter.material.getShader("fragment");
			shader.textures.clear();
			shader.textures.emplace_back().aliasAttachment( renderTarget.attachments[index] ); // attachments.color
		}

		if ( settings::pipelines::deferred ) {
			uf::stl::string vertexShaderFilename = uf::io::root+"/shaders/display/subpass.vert.spv";
			uf::stl::string fragmentShaderFilename = uf::io::root+"/shaders/display/subpass.frag.spv"; {
				std::pair<bool, uf::stl::string> settings[] = {
					{ uf::renderer::settings::pipelines::vxgi, "vxgi.frag" },
					{ msaa > 1, "msaa.frag" },
					{ uf::renderer::settings::pipelines::rt, "rt.frag" },
				};
				FOR_ARRAY( settings ) if ( settings[i].first ) fragmentShaderFilename = uf::string::replace( fragmentShaderFilename, "frag", settings[i].second );
			}

			blitter.material.attachShader(uf::io::resolveURI(vertexShaderFilename), uf::renderer::enums::Shader::VERTEX, "deferred");
			blitter.material.attachShader(uf::io::resolveURI(fragmentShaderFilename), uf::renderer::enums::Shader::FRAGMENT, "deferred");
		}
	#if 0
		if ( settings::pipelines::deferred ) {
			uf::stl::string computeShaderFilename = uf::io::root+"/shaders/display/subpass.comp.spv"; {
				std::pair<bool, uf::stl::string> settings[] = {
					{ uf::renderer::settings::pipelines::vxgi, "vxgi.comp" },
					{ msaa > 1, "msaa.comp" },
					{ uf::renderer::settings::pipelines::rt, "rt.comp" },
				};
				FOR_ARRAY( settings ) if ( settings[i].first ) computeShaderFilename = uf::string::replace( computeShaderFilename, "comp", settings[i].second );
			}

			blitter.material.attachShader(uf::io::resolveURI(computeShaderFilename), uf::renderer::enums::Shader::COMPUTE, "deferred-compute");
		}
	#endif
		
		if ( settings::pipelines::bloom ) {
			uf::stl::string computeShaderFilename = uf::io::resolveURI(uf::io::root+"/shaders/display/bloom.comp.spv");
			blitter.material.attachShader(computeShaderFilename, uf::renderer::enums::Shader::COMPUTE, "bloom");

			auto& shader = blitter.material.getShader("compute", "bloom");
			shader.textures.clear();
			shader.textures.emplace_back().aliasAttachment( renderTarget.attachments[metadata.attachments["color"]/*attachments.color*/], (size_t) 0 );
			shader.textures.emplace_back().aliasAttachment( renderTarget.attachments[metadata.attachments["bright"]/*attachments.bright*/], (size_t) 0 );
			shader.textures.emplace_back().aliasAttachment( renderTarget.attachments[metadata.attachments["scratch"]/*attachments.scratch*/], (size_t) 0 );
			for ( auto& texture : shader.textures ) {
				texture.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
				texture.descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			}
		}
		
		if ( settings::pipelines::deferred ) {
			auto& shader = blitter.material.getShader("fragment", "deferred");

			size_t maxLights = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["lights"]["max"].as<size_t>(512);
			size_t maxTextures2D = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["textures"]["max"]["2D"].as<size_t>(512);
			size_t maxTexturesCube = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["textures"]["max"]["cube"].as<size_t>(128);
			size_t maxTextures3D = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["textures"]["max"]["3D"].as<size_t>(128);
			size_t maxCascades = sceneMetadataJson["system"]["config"]["engine"]["scenes"]["vxgi"]["cascades"].as<size_t>(16);

		//	shader.buffers.emplace_back( uf::graph::storage.buffers.camera.alias() );
		//	shader.buffers.emplace_back( uf::graph::storage.buffers.joint.alias() );
			shader.buffers.emplace_back( uf::graph::storage.buffers.drawCommands.alias() );
			shader.buffers.emplace_back( uf::graph::storage.buffers.instance.alias() );
			shader.buffers.emplace_back( uf::graph::storage.buffers.instanceAddresses.alias() );
			shader.buffers.emplace_back( uf::graph::storage.buffers.material.alias() );
			shader.buffers.emplace_back( uf::graph::storage.buffers.texture.alias() );
			shader.buffers.emplace_back( uf::graph::storage.buffers.light.alias() );

			if ( ext::vulkan::settings::pipelines::vxgi ) {
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

		if ( !blitter.hasPipeline( blitter.descriptor ) ){
			blitter.initializePipeline( blitter.descriptor );
		}
		
		{
			ext::vulkan::GraphicDescriptor descriptor = blitter.descriptor;
			descriptor.renderMode = "";
			
			if ( settings::pipelines::deferred ) {
				for ( size_t eye = 0; eye < metadata.eyes; ++eye ) {
					descriptor.pipeline = "deferred";
					descriptor.subpass = 1; // (renderTarget.passes.size() / metadata.eyes) * eye + 1;
					if ( !blitter.hasPipeline( descriptor ) ) {
						blitter.initializePipeline( descriptor );
					}
				}
			#if 0
				for ( size_t eye = 0; eye < metadata.eyes; ++eye ) {
					descriptor.pipeline = "deferred-compute";
					descriptor.subpass = 0;
					if ( !blitter.hasPipeline( descriptor ) ) {
						blitter.initializePipeline( descriptor );
					}
				}
			#endif
			}

			if ( settings::pipelines::bloom ) {
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

	// update post processing
	if ( blitter.material.hasShader("fragment") && !settings::pipelines::rt ) {
		auto& shader = blitter.material.getShader("fragment");

		struct {
			float curTime = 0;
		} uniforms;

		uniforms.curTime = uf::time::current;

		shader.updateBuffer( (const void*) &uniforms, sizeof(uniforms), shader.getUniformBuffer("UBO") );
	}

	if ( resized ) {
		renderTarget.initialize( *renderTarget.device );

		if ( settings::pipelines::bloom ) {
			auto& shader = blitter.material.getShader("compute", "bloom");
			shader.textures.clear();
			shader.textures.emplace_back().aliasAttachment( renderTarget.attachments[metadata.attachments["color"]/*metadata.outputs[0]+0*/], (size_t) 0 ); // attachments.color
			shader.textures.emplace_back().aliasAttachment( renderTarget.attachments[metadata.attachments["bright"]/*metadata.outputs[0]+1*/], (size_t) 0 ); // attachments.bright
			shader.textures.emplace_back().aliasAttachment( renderTarget.attachments[metadata.attachments["scratch"]/*metadata.outputs[0]+2*/], (size_t) 0 ); // attachments.scratch
			for ( auto& texture : shader.textures ) {
				texture.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
				texture.descriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			}
		}
	}
	// update blitter descriptor set
	if ( rebuild && blitter.initialized ) {
		{
		#if UF_USE_FFX_FSR
			size_t index = metadata.attachments["scratch"];
		#else
			size_t index = metadata.attachments["color"]; // uf::renderer::settings::pipelines::deferred ? metadata.outputs[0] : metadata.outputs[0] - 2; // deferred ? attachments.color : attachments.albedo;
		#endif
			auto& shader = blitter.material.getShader("fragment");
			shader.textures.clear();
			shader.textures.emplace_back().aliasAttachment( renderTarget.attachments[index], (size_t) 0 ); // attachments.color
		}

		if ( blitter.hasPipeline( blitter.descriptor ) ){
			blitter.getPipeline( blitter.descriptor ).update( blitter, blitter.descriptor );
		}
		{
			ext::vulkan::GraphicDescriptor descriptor = blitter.descriptor;
			descriptor.renderMode = "";
			if ( settings::pipelines::deferred ) {
				for ( size_t eye = 0; eye < metadata.eyes; ++eye ) {
					descriptor.pipeline = "deferred";
					descriptor.subpass = 1; // (renderTarget.passes.size() / metadata.eyes) * eye + 1;
					if ( blitter.hasPipeline( descriptor ) ) {
						blitter.getPipeline( descriptor ).update( blitter, descriptor );
					}
				}
			}
		#if 0
			if ( settings::pipelines::deferred ) {
				for ( size_t eye = 0; eye < metadata.eyes; ++eye ) {
					descriptor.pipeline = "deferred-compute";
					descriptor.subpass = 0;
					if ( blitter.hasPipeline( descriptor ) ) {
						blitter.getPipeline( descriptor ).update( blitter, descriptor );
					}
				}
			}
		#endif

			if ( settings::pipelines::bloom ) {
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

	for (size_t i = 0; i < commands.size(); ++i) {
		VK_CHECK_RESULT(vkBeginCommandBuffer(commands[i], &cmdBufInfo));
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

			// transition layers for read
			for ( auto layer : layers ) {
				layer->pipelineBarrier( commands[i], 0 );
			}

			for ( auto& pipeline : metadata.pipelines ) {
				if ( pipeline == metadata.pipeline ) continue;
				for ( auto graphic : graphics ) {
					if ( graphic->descriptor.renderMode != this->getName() ) continue;
					ext::vulkan::GraphicDescriptor descriptor = bindGraphicDescriptor(graphic->descriptor, currentSubpass);
					descriptor.pipeline = pipeline;
					graphic->record( commands[i], descriptor, 0, metadata.eyes );
				}
			}
		/*
			for ( auto& pipeline : metadata.pipelines ) {
				if ( pipeline == metadata.pipeline || pipeline == "deferred" ) continue;
				for ( auto graphic : graphics ) {
					if ( graphic->descriptor.renderMode != this->getName() ) continue;
					ext::vulkan::GraphicDescriptor descriptor = bindGraphicDescriptor(graphic->descriptor, currentSubpass);
					descriptor.pipeline = pipeline;
					graphic->record( commands[i], descriptor, 0, metadata.eyes );
				}
			}
		*/

			// pre-renderpass commands
			if ( commandBufferCallbacks.count(CALLBACK_BEGIN) > 0 ) commandBufferCallbacks[CALLBACK_BEGIN]( commands[i], i );

			vkCmdBeginRenderPass(commands[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
				vkCmdSetViewport(commands[i], 0, 1, &viewport);
				vkCmdSetScissor(commands[i], 0, 1, &scissor);
				// render to geometry buffers
				for ( size_t eye = 0; eye < metadata.eyes; ++eye ) {
					size_t currentPass = 0;
					size_t currentDraw = 0;
					if ( !settings::pipelines::rt ) for ( auto graphic : graphics ) {
						// only draw graphics that are assigned to this type of render mode
						if ( graphic->descriptor.renderMode != this->getName() ) continue;
						ext::vulkan::GraphicDescriptor descriptor = bindGraphicDescriptor(graphic->descriptor, currentSubpass);
						graphic->record( commands[i], descriptor, eye, currentDraw++ );
					}
				vkCmdNextSubpass(commands[i], VK_SUBPASS_CONTENTS_INLINE); ++currentPass; ++currentSubpass;
					// deferred post-processing lighting pass
					if ( !settings::pipelines::rt ) {
						ext::vulkan::GraphicDescriptor descriptor = blitter.descriptor; // = bindGraphicDescriptor(blitter.descriptor, currentSubpass);
						descriptor.renderMode = "";
						descriptor.pipeline = "deferred";
						descriptor.subpass = 1;
						blitter.record(commands[i], descriptor, eye, currentDraw++);
					}
					if ( eye + 1 < metadata.eyes ) vkCmdNextSubpass(commands[i], VK_SUBPASS_CONTENTS_INLINE); ++currentSubpass;
				}
			vkCmdEndRenderPass(commands[i]);

		#if 0
			// fill GBuffer
			vkCmdBeginRenderPass(commands[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
				vkCmdSetViewport(commands[i], 0, 1, &viewport);
				vkCmdSetScissor(commands[i], 0, 1, &scissor);
				// render to geometry buffers
				for ( size_t eye = 0; eye < metadata.eyes; ++eye ) {
					size_t currentPass = 0;
					size_t currentDraw = 0;
					if ( !settings::pipelines::rt ) for ( auto graphic : graphics ) {
						// only draw graphics that are assigned to this type of render mode
						if ( graphic->descriptor.renderMode != this->getName() ) continue;
						ext::vulkan::GraphicDescriptor descriptor = bindGraphicDescriptor(graphic->descriptor, currentSubpass);
						graphic->record( commands[i], descriptor, eye, currentDraw++ );
					}
				//	if ( eye + 1 < metadata.eyes ) vkCmdNextSubpass(commands[i], VK_SUBPASS_CONTENTS_INLINE); ++currentSubpass;
			vkCmdEndRenderPass(commands[i]);

			// parse GBuffer
			vkCmdBeginRenderPass(commands[i], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
				vkCmdSetViewport(commands[i], 0, 1, &viewport);
				vkCmdSetScissor(commands[i], 0, 1, &scissor);

				ext::vulkan::GraphicDescriptor descriptor = blitter.descriptor;
				descriptor.pipeline = "deferred";
				blitter.record(commands[i], descriptor, 0, 0);
			vkCmdEndRenderPass(commands[i]);
		#endif

			// post-renderpass commands
			if ( commandBufferCallbacks.count(CALLBACK_END) > 0 ) commandBufferCallbacks[CALLBACK_END]( commands[i], i );

		#if 1
			if ( settings::pipelines::bloom ) {
				ext::vulkan::GraphicDescriptor descriptor = blitter.descriptor;
				descriptor.pipeline = "bloom";
				descriptor.inputs.dispatch = { (width / 8) + 1, (height / 8) + 1, 1 };
				descriptor.subpass = 0;

				auto& shader = blitter.material.getShader("compute", "bloom");

				imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;

			#if 0
				if ( device->queueFamilyIndices.graphics != device->queueFamilyIndices.compute ) {
					imageMemoryBarrier.srcQueueFamilyIndex = device->queueFamilyIndices.graphics;
					imageMemoryBarrier.dstQueueFamilyIndex = device->queueFamilyIndices.compute;
				} else {
					imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
					imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				}
			#endif

				for ( auto& attachment : shader.textures ) {
					imageMemoryBarrier.image = attachment.image;
					vkCmdPipelineBarrier( commands[i], VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_FLAGS_NONE, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
				}

				imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

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

		#if 0
			if ( device->queueFamilyIndices.graphics != device->queueFamilyIndices.compute ) {
				imageMemoryBarrier.srcQueueFamilyIndex = device->queueFamilyIndices.compute;
				imageMemoryBarrier.dstQueueFamilyIndex = device->queueFamilyIndices.graphics;
			}
		#endif

				for ( auto& attachment : shader.textures ) {
					imageMemoryBarrier.image = attachment.image;
					vkCmdPipelineBarrier( commands[i], VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_FLAGS_NONE, 0, NULL, 0, NULL, 1, &imageMemoryBarrier );
				}
			}
		#endif

			for ( auto layer : layers ) {
				layer->pipelineBarrier( commands[i], 1 );
			}
		}

		VK_CHECK_RESULT(vkEndCommandBuffer(commands[i]));
	}
}

#endif