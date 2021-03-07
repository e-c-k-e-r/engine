#if UF_USE_OPENGL

#include <uf/ext/opengl/opengl.h>
#include <uf/ext/opengl/initializers.h>
#include <uf/ext/opengl/graphic.h>
#include <uf/ext/opengl/rendermode.h>
#include <uf/utils/graphic/graphic.h>

#include <uf/ext/openvr/openvr.h>
#include <uf/ext/gltf/graph.h>

#include <ostream>
#include <fstream>
#include <atomic>

namespace {
	float fract( float n ) {
		float whole, fractional;
		return std::modf(n, &whole);
	}
}

uint32_t ext::opengl::settings::width = 1280;
uint32_t ext::opengl::settings::height = 720;
uint8_t ext::opengl::settings::msaa = 1;
bool ext::opengl::settings::validation = true;
// constexpr size_t ext::opengl::settings::maxViews = 6;
size_t ext::opengl::settings::viewCount = 2;

std::vector<std::string> ext::opengl::settings::validationFilters;
std::vector<std::string> ext::opengl::settings::requestedDeviceFeatures;
std::vector<std::string> ext::opengl::settings::requestedDeviceExtensions;
std::vector<std::string> ext::opengl::settings::requestedInstanceExtensions;

ext::opengl::enums::Filter::type_t ext::opengl::settings::swapchainUpscaleFilter = ext::opengl::enums::Filter::LINEAR;

bool ext::opengl::settings::experimental::rebuildOnTickBegin = false;
bool ext::opengl::settings::experimental::waitOnRenderEnd = false;
bool ext::opengl::settings::experimental::individualPipelines = true;
bool ext::opengl::settings::experimental::multithreadedCommandRecording = true;
std::string ext::opengl::settings::experimental::deferredMode = "";
bool ext::opengl::settings::experimental::deferredReconstructPosition = false;
bool ext::opengl::settings::experimental::deferredAliasOutputToSwapchain = true;
bool ext::opengl::settings::experimental::multiview = true;
bool ext::opengl::settings::experimental::vsync = true;
bool ext::opengl::settings::experimental::hdr = true;

GLhandle(VkColorSpaceKHR) ext::opengl::settings::formats::colorSpace;
ext::opengl::enums::Format::type_t ext::opengl::settings::formats::color = ext::opengl::enums::Format::R8G8B8A8_UNORM;
ext::opengl::enums::Format::type_t ext::opengl::settings::formats::depth = ext::opengl::enums::Format::D32_SFLOAT;
ext::opengl::enums::Format::type_t ext::opengl::settings::formats::normal = ext::opengl::enums::Format::R16G16B16A16_SFLOAT;
ext::opengl::enums::Format::type_t ext::opengl::settings::formats::position = ext::opengl::enums::Format::R16G16B16A16_SFLOAT;

ext::opengl::Device ext::opengl::device;
std::mutex ext::opengl::mutex;
std::mutex ext::opengl::immediateModeMutex;

bool ext::opengl::states::resized = false;
bool ext::opengl::states::rebuild = false;
uint32_t ext::opengl::states::currentBuffer = 0;

std::vector<uf::Scene*> ext::opengl::scenes;
ext::opengl::RenderMode* ext::opengl::currentRenderMode = NULL;
std::vector<ext::opengl::RenderMode*> ext::opengl::renderModes = {
	new ext::opengl::BaseRenderMode,
};

std::string UF_API ext::opengl::errorString() {
	return ext::opengl::errorString(glGetError());
}
std::string UF_API ext::opengl::errorString( GLenum error ) {
	std::string str = "";
	if (error == GL_NO_ERROR) return str;
#if UF_ENV_DREAMCAST
#else
	const GLubyte* bytes = gluErrorString(error);
	str = reinterpret_cast<const char*>(bytes);
#endif
	return str;
}

/////
bool ext::opengl::hasRenderMode( const std::string& name, bool isName ) {
	for ( auto& renderMode: ext::opengl::renderModes ) {
		if ( isName ) {
			if ( renderMode->getName() == name ) return true;
		} else {
			if ( renderMode->getType() == name ) return true;
		}
	}
	return false;
}

ext::opengl::RenderMode& ext::opengl::addRenderMode( ext::opengl::RenderMode* mode, const std::string& name ) {
	mode->metadata["name"] = name;
	renderModes.push_back(mode);
	if ( ext::opengl::settings::validation ) uf::iostream << "Adding RenderMode: " << name << ": " << mode->getType() << "\n";
	// reorder
	ext::opengl::states::rebuild = true;
	return *mode;
}
ext::opengl::RenderMode& ext::opengl::getRenderMode( const std::string& name, bool isName ) {
	RenderMode* target = renderModes[0];
	for ( auto& renderMode: renderModes ) {
		if ( isName ) {
			if ( renderMode->getName() == "" )  target = renderMode;
			if ( renderMode->getName() == name ) {
				target = renderMode;
				break;
			}
		} else {
			if ( renderMode->getType() == name ) {
				target = renderMode;
				break;
			}
		}
	}
//	if ( ext::opengl::settings::validation ) uf::iostream << "Requesting RenderMode `" << name << "`, got `" << target->getName() << "` (" << target->getType() << ")" << "\n";
	return *target;
}
std::vector<ext::opengl::RenderMode*> ext::opengl::getRenderModes( const std::string& name, bool isName ) {
	return ext::opengl::getRenderModes({name}, isName);
}
std::vector<ext::opengl::RenderMode*> ext::opengl::getRenderModes( const std::vector<std::string>& names, bool isName ) {
	std::vector<RenderMode*> targets;
	for ( auto& renderMode: renderModes ) {
		if ( ( isName && std::find(names.begin(), names.end(), renderMode->getName()) != names.end() ) || std::find(names.begin(), names.end(), renderMode->getType()) != names.end() ) {
			targets.push_back(renderMode);
//			if ( ext::opengl::settings::validation ) uf::iostream << "Requestings RenderMode `" << name << "`, got `" << renderMode->getName() << "` (" << renderMode->getType() << ")" << "\n";
		}
	}
	return targets;
}
void UF_API ext::opengl::removeRenderMode( ext::opengl::RenderMode* mode, bool free ) {
	if ( !mode ) return;
	renderModes.erase( std::remove( renderModes.begin(), renderModes.end(), mode ), renderModes.end() );
	mode->destroy();
	if ( free ) delete mode;
	ext::opengl::states::rebuild = true;
}

void UF_API ext::opengl::initialize() {
	device.initialize();
	// swapchain.initialize( device );
	if ( 0 ) {
		std::vector<uint8_t> pixels = { 
			255,   0, 255, 255,      0,   0,   0, 255,
			  0,   0,   0, 255,    255,   0, 255, 255,
		};
		Texture2D::empty.sampler.descriptor.filter.min = uf::renderer::enums::Filter::NEAREST;
		Texture2D::empty.sampler.descriptor.filter.mag = uf::renderer::enums::Filter::NEAREST;
		Texture2D::empty.fromBuffers( (void*) &pixels[0], pixels.size(), uf::renderer::enums::Format::R8G8B8A8_UNORM, 2, 2 );
	}
	for ( auto& renderMode : renderModes ) {
		if ( !renderMode ) continue;
		renderMode->initialize(device);
	}
	
	std::vector<std::function<int()>> jobs;
	for ( auto& renderMode : renderModes ) {
		if ( !renderMode ) continue;
		if ( settings::experimental::individualPipelines ) renderMode->bindPipelines();
		if ( settings::experimental::multithreadedCommandRecording ) {
			jobs.emplace_back([&]{
				renderMode->createCommandBuffers();
				return 0;
			});
		} else {
			renderMode->createCommandBuffers();
		}
	}
	if ( !jobs.empty() ) uf::thread::batchWorkers( jobs );
	// bind shaders
#if !UF_ENV_DREAMCAST
	{
		ext::opengl::Shader::bind( uf::io::root + "shaders/gltf/instanced.vert.spv", [](const ext::opengl::Shader& shader, const ext::opengl::Graphic& graphic, void* userdata) {
			if ( !userdata ) return;

			ext::gltf::mesh_t::vertex_t* verticesSrc = (ext::gltf::mesh_t::vertex_t*) userdata;

			uf::renderer::Buffer::Descriptor vertexBuffer = {};
			uf::renderer::Buffer::Descriptor instancesBuffer = graphic.getStorage(0);
			uf::renderer::Buffer::Descriptor materialsBuffer = graphic.getStorage(1);
			uf::renderer::Buffer::Descriptor texturesBuffer  = graphic.getStorage(2);

			for ( auto& buffer : graphic.buffers ) {
				if ( (buffer.usage & uf::renderer::enums::Buffer::VERTEX) && !vertexBuffer.buffer ) vertexBuffer = buffer.descriptor;
			}

			if ( !vertexBuffer.buffer || !instancesBuffer.buffer ) return;

			size_t vertexStride = graphic.descriptor.geometry.attributes.vertex.size;
			size_t vertices = vertexBuffer.range / vertexStride;
			
			uf::renderer::VertexDescriptor 	vertexAttributePosition,
											vertexAttributeUv,
											vertexAttributeNormal,
											vertexAttributeId;

			for ( auto& attribute : graphic.descriptor.geometry.attributes.descriptor ) {
				if ( attribute.name == "position" ) vertexAttributePosition = attribute;
				else if ( attribute.name == "normal" ) vertexAttributeNormal = attribute;
				else if ( attribute.name == "uv" ) vertexAttributeUv = attribute;
				else if ( attribute.name == "id" ) vertexAttributeId = attribute;
			}
			if ( vertexAttributePosition.name == "" ||  vertexAttributeUv.name == "" || vertexAttributeId.name == "" ) return;
			// GPU-buffer based command dispatching

			// CPU-buffer based command dispatching
			pod::Matrix4f* instances 			= (pod::Matrix4f*) ( graphic.device->getBuffer( instancesBuffer.buffer ) + instancesBuffer.offset );
			pod::Material::Storage* materials 	= (pod::Material::Storage*) ( graphic.device->getBuffer( materialsBuffer.buffer ) + materialsBuffer.offset );
			pod::Texture::Storage*  textures 	= (pod::Texture::Storage*) ( graphic.device->getBuffer( texturesBuffer.buffer ) + texturesBuffer.offset );
			size_t instancesCount = instancesBuffer.range / sizeof(pod::Matrix4f);
			size_t materialsCount = materialsBuffer.range / sizeof(pod::Material::Storage);
			size_t texturesCount  = texturesBuffer.range / sizeof(pod::Texture::Storage);

			uint8_t* vertexSrcPointer = (uint8_t*) ( verticesSrc );
			uint8_t* vertexDstPointer = (uint8_t*) ( graphic.device->getBuffer( vertexBuffer.buffer ) + vertexBuffer.offset );
			vertexSrcPointer += vertexBuffer.offset;

			for ( size_t currentIndex = 0; currentIndex < vertices; ++currentIndex ) {
				uint8_t* vertexSrc = vertexSrcPointer + (currentIndex * vertexStride);
				uint8_t* vertexDst = vertexDstPointer + (currentIndex * vertexStride);
			
				const pod::Vector3f& position = *((pod::Vector3f*) (vertexSrc + vertexAttributePosition.offset));
				const pod::Vector2ui& id = *((pod::Vector2ui*) (vertexSrc + vertexAttributeId.offset));
				const pod::Vector3f& normal = *((pod::Vector3f*) (vertexSrc + vertexAttributeNormal.offset));
				const pod::Vector2f& uv = *((pod::Vector2f*) (vertexSrc + vertexAttributeUv.offset));

				pod::Vector3f& positionDst 	= *((pod::Vector3f*) (vertexDst + vertexAttributePosition.offset));
				pod::Vector3f& normalDst 	= *((pod::Vector3f*) (vertexDst + vertexAttributeNormal.offset));
				pod::Vector2f& uvDst 		= *((pod::Vector2f*) (vertexDst + vertexAttributeUv.offset));

				auto& model = instances[id.x];
				auto& material = materials[id.y];
				auto& texture = textures[material.indexAlbedo];

				positionDst = uf::matrix::multiply<float>( model, position, 1.0f );
				normalDst = uf::vector::normalize( uf::matrix::multiply<float>( model, normal, 0.0f ) );
			}
		});
		ext::opengl::Shader::bind( uf::io::root + "shaders/gltf/skinned.vert.spv", [](const ext::opengl::Shader& shader, const ext::opengl::Graphic& graphic, void* userdata) {
			if ( !userdata ) return;

			ext::gltf::mesh_t::vertex_t* verticesSrc = (ext::gltf::mesh_t::vertex_t*) userdata;

			uf::renderer::Buffer::Descriptor vertexBuffer = {};
			uf::renderer::Buffer::Descriptor jointsBuffer = graphic.getStorage(0);

			for ( auto& buffer : graphic.buffers ) {
				if ( (buffer.usage & uf::renderer::enums::Buffer::VERTEX) && !vertexBuffer.buffer ) vertexBuffer = buffer.descriptor;
			}
			if ( !vertexBuffer.buffer || !jointsBuffer.buffer ) return;

			size_t vertexStride = graphic.descriptor.geometry.attributes.vertex.size;
			size_t vertices = vertexBuffer.range / vertexStride;
			
			uf::renderer::VertexDescriptor 	vertexAttributePosition,
											vertexAttributeNormal,
											vertexAttributeJoints,
											vertexAttributeWeights;

			for ( auto& attribute : graphic.descriptor.geometry.attributes.descriptor ) {
				if ( attribute.name == "position" ) 		vertexAttributePosition = attribute;
				else if ( attribute.name == "normal" ) 		vertexAttributeNormal = attribute;
				else if ( attribute.name == "joints" ) 		vertexAttributeJoints = attribute;
				else if ( attribute.name == "weights" ) 	vertexAttributeWeights = attribute;
			}
			if ( vertexAttributePosition.name == "" || vertexAttributeJoints.name == "" || vertexAttributeWeights.name == "" ) return;
			// GPU-buffer based command dispatching

			// CPU-buffer based command dispatching
			pod::Matrix4f* jointMatrices = (pod::Matrix4f*) ( graphic.device->getBuffer( jointsBuffer.buffer ) + jointsBuffer.offset );
			size_t jointsCount = jointsBuffer.range / sizeof(pod::Matrix4f);

			uint8_t* vertexSrcPointer = (uint8_t*) ( verticesSrc );
			uint8_t* vertexDstPointer = (uint8_t*) ( graphic.device->getBuffer( vertexBuffer.buffer ) + vertexBuffer.offset );
			vertexSrcPointer += vertexBuffer.offset;
			for ( size_t currentIndex = 0; currentIndex < vertices; ++currentIndex ) {
				uint8_t* vertexSrc = vertexSrcPointer + (currentIndex * vertexStride);
				uint8_t* vertexDst = vertexDstPointer + (currentIndex * vertexStride);
			
				const pod::Vector3f& position = *((pod::Vector3f*) (vertexSrc + vertexAttributePosition.offset));
				const pod::Vector3f& normal = *((pod::Vector3f*) (vertexSrc + vertexAttributeNormal.offset));
				const pod::Vector4ui& joints = *((pod::Vector4ui*) (vertexSrc + vertexAttributeJoints.offset));
				const pod::Vector4f& weights = *((pod::Vector4f*) (vertexSrc + vertexAttributeWeights.offset));

				pod::Vector3f& positionDst 	= *((pod::Vector3f*) (vertexDst + vertexAttributePosition.offset));
				pod::Vector3f& normalDst 	= *((pod::Vector3f*) (vertexDst + vertexAttributeNormal.offset));
				pod::Matrix4f model = jointMatrices[joints[0]] * weights[0] + jointMatrices[joints[1]] * weights[1] + jointMatrices[joints[2]] * weights[2] + jointMatrices[joints[3]] * weights[3];
				
				positionDst = uf::matrix::multiply<float>( model, position, 1.0f );
				normalDst = uf::vector::normalize( uf::matrix::multiply<float>( model, normal, 0.0f ) );
			}
		});
	}
#endif
}
void UF_API ext::opengl::tick(){
	ext::opengl::mutex.lock();
	if ( ext::opengl::states::resized || ext::opengl::settings::experimental::rebuildOnTickBegin ) {
		ext::opengl::states::rebuild = true;
	}
if ( uf::scene::useGraph ) {
	auto graph = uf::scene::generateGraph();
	for ( auto entity : graph ) {
		if ( !entity->hasComponent<uf::Graphic>() ) continue;
		ext::opengl::Graphic& graphic = entity->getComponent<uf::Graphic>();
		if ( graphic.initialized || !graphic.process || graphic.initialized ) continue;
		graphic.initializePipeline();
		ext::opengl::states::rebuild = true;
	}
} else {
	for ( uf::Scene* scene : uf::scene::scenes ) {
		if ( !scene ) continue;
		scene->process([&]( uf::Entity* entity ) {
			if ( !entity->hasComponent<uf::Graphic>() ) return;
			ext::opengl::Graphic& graphic = entity->getComponent<uf::Graphic>();
			if ( graphic.initialized || !graphic.process || graphic.initialized ) return;
			graphic.initializePipeline();
			ext::opengl::states::rebuild = true;
		});
	}
}
	for ( auto& renderMode : renderModes ) {
		if ( !renderMode ) continue;
		if ( !renderMode->device ) {
			renderMode->initialize(ext::opengl::device);
			ext::opengl::states::rebuild = true;
		}
		renderMode->tick();
	}
	std::vector<std::function<int()>> jobs;
	for ( auto& renderMode : renderModes ) {
		if ( !renderMode ) continue;
		if ( ext::opengl::states::rebuild || renderMode->rebuild ) {
			if ( settings::experimental::individualPipelines ) renderMode->bindPipelines();
			if ( settings::experimental::multithreadedCommandRecording ) jobs.emplace_back([&]{ renderMode->createCommandBuffers(); return 0; });
			else renderMode->createCommandBuffers();
		}
	}
	if ( !jobs.empty() ) uf::thread::batchWorkers( jobs );	
/*
	ext::opengl::device.activateContext();
	ext::opengl::device.commandBuffer.end();
	ext::opengl::device.commandBuffer.submit();
	ext::opengl::device.commandBuffer.flush();
	ext::opengl::device.commandBuffer.start();
*/	
	ext::opengl::states::rebuild = false;
	ext::opengl::states::resized = false;
	ext::opengl::mutex.unlock();
}
void UF_API ext::opengl::render(){
	ext::opengl::mutex.lock();
#if !UF_ENV_DREAMCAST
	if ( hasRenderMode("Gui", true) ) {
		RenderMode& primary = getRenderMode("Gui", true);
		auto it = std::find( renderModes.begin(), renderModes.end(), &primary );
		if ( it + 1 != renderModes.end() ) std::rotate( it, it + 1, renderModes.end() );
	}
	if ( hasRenderMode("", true) ) {
		RenderMode& primary = getRenderMode("", true);
		auto it = std::find( renderModes.begin(), renderModes.end(), &primary );
		if ( it + 1 != renderModes.end() ) std::rotate( it, it + 1, renderModes.end() );
	} else {
		RenderMode& primary = getRenderMode("Swapchain", true);
		auto it = std::find( renderModes.begin(), renderModes.end(), &primary );
		if ( it + 1 != renderModes.end() ) std::rotate( it, it + 1, renderModes.end() );
	}
	ext::opengl::device.activateContext();
#endif
	for ( auto& renderMode : renderModes ) {
		if ( !renderMode || !renderMode->execute ) continue;
		ext::opengl::currentRenderMode = renderMode;
		uf::scene::render();
	//	UF_TIMER_TRACE_INIT();
		renderMode->render();
		renderMode->executed = true;
	//	UF_TIMER_TRACE("==== RENDER TIME ==== ");
	}
	ext::opengl::currentRenderMode = NULL;
	if ( ext::opengl::settings::experimental::waitOnRenderEnd ) synchronize();
#if UF_USE_OPENVR
	// if ( ext::openvr::context ) ext::openvr::postSubmit();
#endif
	ext::opengl::mutex.unlock();
}
void UF_API ext::opengl::destroy() {
	ext::opengl::mutex.lock();
	synchronize();

	Texture2D::empty.destroy();
if ( uf::scene::useGraph ) {
	auto graph = uf::scene::generateGraph();
	for ( auto entity : graph ) {
		if ( !entity->hasComponent<uf::Graphic>() ) continue;
		uf::Graphic& graphic = entity->getComponent<uf::Graphic>();
		graphic.destroy();
	}
} else {
	for ( uf::Scene* scene : uf::scene::scenes ) {
		if ( !scene ) continue;
		scene->process([&]( uf::Entity* entity ) {
			if ( !entity->hasComponent<uf::Graphic>() ) return;
			uf::Graphic& graphic = entity->getComponent<uf::Graphic>();
			graphic.destroy();
		});
	}
}
	for ( auto& renderMode : renderModes ) {
		if ( !renderMode ) continue;
		renderMode->destroy();
		renderMode = NULL;
	}
//	swapchain.destroy();
	device.destroy();
	ext::opengl::mutex.unlock();
}
void UF_API ext::opengl::synchronize( uint8_t flag ) {
	if ( flag & 0b01 ) {
		for ( auto& renderMode : renderModes ) {
			if ( !renderMode ) continue;
			if ( !renderMode->execute ) continue;
			renderMode->synchronize();
		}
	}
	if ( flag & 0b10 ) {
	//	vkDeviceWaitIdle( device );
	}
}
std::string UF_API ext::opengl::allocatorStats(){
	return "";
}
ext::opengl::enums::Format::type_t ext::opengl::formatFromString( const std::string& string ) {
	std::unordered_map<std::string,ext::opengl::enums::Format::type_t> table = {
		{ "UNDEFINED", ext::opengl::enums::Format::UNDEFINED },
		{ "R4G4_UNORM_PACK8", ext::opengl::enums::Format::R4G4_UNORM_PACK8 },
		{ "R4G4B4A4_UNORM_PACK16", ext::opengl::enums::Format::R4G4B4A4_UNORM_PACK16 },
		{ "B4G4R4A4_UNORM_PACK16", ext::opengl::enums::Format::B4G4R4A4_UNORM_PACK16 },
		{ "R5G6B5_UNORM_PACK16", ext::opengl::enums::Format::R5G6B5_UNORM_PACK16 },
		{ "B5G6R5_UNORM_PACK16", ext::opengl::enums::Format::B5G6R5_UNORM_PACK16 },
		{ "R5G5B5A1_UNORM_PACK16", ext::opengl::enums::Format::R5G5B5A1_UNORM_PACK16 },
		{ "B5G5R5A1_UNORM_PACK16", ext::opengl::enums::Format::B5G5R5A1_UNORM_PACK16 },
		{ "A1R5G5B5_UNORM_PACK16", ext::opengl::enums::Format::A1R5G5B5_UNORM_PACK16 },
		{ "R8_UNORM", ext::opengl::enums::Format::R8_UNORM },
		{ "R8_SNORM", ext::opengl::enums::Format::R8_SNORM },
		{ "R8_USCALED", ext::opengl::enums::Format::R8_USCALED },
		{ "R8_SSCALED", ext::opengl::enums::Format::R8_SSCALED },
		{ "R8_UINT", ext::opengl::enums::Format::R8_UINT },
		{ "R8_SINT", ext::opengl::enums::Format::R8_SINT },
		{ "R8_SRGB", ext::opengl::enums::Format::R8_SRGB },
		{ "R8G8_UNORM", ext::opengl::enums::Format::R8G8_UNORM },
		{ "R8G8_SNORM", ext::opengl::enums::Format::R8G8_SNORM },
		{ "R8G8_USCALED", ext::opengl::enums::Format::R8G8_USCALED },
		{ "R8G8_SSCALED", ext::opengl::enums::Format::R8G8_SSCALED },
		{ "R8G8_UINT", ext::opengl::enums::Format::R8G8_UINT },
		{ "R8G8_SINT", ext::opengl::enums::Format::R8G8_SINT },
		{ "R8G8_SRGB", ext::opengl::enums::Format::R8G8_SRGB },
		{ "R8G8B8_UNORM", ext::opengl::enums::Format::R8G8B8_UNORM },
		{ "R8G8B8_SNORM", ext::opengl::enums::Format::R8G8B8_SNORM },
		{ "R8G8B8_USCALED", ext::opengl::enums::Format::R8G8B8_USCALED },
		{ "R8G8B8_SSCALED", ext::opengl::enums::Format::R8G8B8_SSCALED },
		{ "R8G8B8_UINT", ext::opengl::enums::Format::R8G8B8_UINT },
		{ "R8G8B8_SINT", ext::opengl::enums::Format::R8G8B8_SINT },
		{ "R8G8B8_SRGB", ext::opengl::enums::Format::R8G8B8_SRGB },
		{ "B8G8R8_UNORM", ext::opengl::enums::Format::B8G8R8_UNORM },
		{ "B8G8R8_SNORM", ext::opengl::enums::Format::B8G8R8_SNORM },
		{ "B8G8R8_USCALED", ext::opengl::enums::Format::B8G8R8_USCALED },
		{ "B8G8R8_SSCALED", ext::opengl::enums::Format::B8G8R8_SSCALED },
		{ "B8G8R8_UINT", ext::opengl::enums::Format::B8G8R8_UINT },
		{ "B8G8R8_SINT", ext::opengl::enums::Format::B8G8R8_SINT },
		{ "B8G8R8_SRGB", ext::opengl::enums::Format::B8G8R8_SRGB },
		{ "R8G8B8A8_UNORM", ext::opengl::enums::Format::R8G8B8A8_UNORM },
		{ "R8G8B8A8_SNORM", ext::opengl::enums::Format::R8G8B8A8_SNORM },
		{ "R8G8B8A8_USCALED", ext::opengl::enums::Format::R8G8B8A8_USCALED },
		{ "R8G8B8A8_SSCALED", ext::opengl::enums::Format::R8G8B8A8_SSCALED },
		{ "R8G8B8A8_UINT", ext::opengl::enums::Format::R8G8B8A8_UINT },
		{ "R8G8B8A8_SINT", ext::opengl::enums::Format::R8G8B8A8_SINT },
		{ "R8G8B8A8_SRGB", ext::opengl::enums::Format::R8G8B8A8_SRGB },
		{ "B8G8R8A8_UNORM", ext::opengl::enums::Format::B8G8R8A8_UNORM },
		{ "B8G8R8A8_SNORM", ext::opengl::enums::Format::B8G8R8A8_SNORM },
		{ "B8G8R8A8_USCALED", ext::opengl::enums::Format::B8G8R8A8_USCALED },
		{ "B8G8R8A8_SSCALED", ext::opengl::enums::Format::B8G8R8A8_SSCALED },
		{ "B8G8R8A8_UINT", ext::opengl::enums::Format::B8G8R8A8_UINT },
		{ "B8G8R8A8_SINT", ext::opengl::enums::Format::B8G8R8A8_SINT },
		{ "B8G8R8A8_SRGB", ext::opengl::enums::Format::B8G8R8A8_SRGB },
		{ "A8B8G8R8_UNORM_PACK32", ext::opengl::enums::Format::A8B8G8R8_UNORM_PACK32 },
		{ "A8B8G8R8_SNORM_PACK32", ext::opengl::enums::Format::A8B8G8R8_SNORM_PACK32 },
		{ "A8B8G8R8_USCALED_PACK32", ext::opengl::enums::Format::A8B8G8R8_USCALED_PACK32 },
		{ "A8B8G8R8_SSCALED_PACK32", ext::opengl::enums::Format::A8B8G8R8_SSCALED_PACK32 },
		{ "A8B8G8R8_UINT_PACK32", ext::opengl::enums::Format::A8B8G8R8_UINT_PACK32 },
		{ "A8B8G8R8_SINT_PACK32", ext::opengl::enums::Format::A8B8G8R8_SINT_PACK32 },
		{ "A8B8G8R8_SRGB_PACK32", ext::opengl::enums::Format::A8B8G8R8_SRGB_PACK32 },
		{ "A2R10G10B10_UNORM_PACK32", ext::opengl::enums::Format::A2R10G10B10_UNORM_PACK32 },
		{ "A2R10G10B10_SNORM_PACK32", ext::opengl::enums::Format::A2R10G10B10_SNORM_PACK32 },
		{ "A2R10G10B10_USCALED_PACK32", ext::opengl::enums::Format::A2R10G10B10_USCALED_PACK32 },
		{ "A2R10G10B10_SSCALED_PACK32", ext::opengl::enums::Format::A2R10G10B10_SSCALED_PACK32 },
		{ "A2R10G10B10_UINT_PACK32", ext::opengl::enums::Format::A2R10G10B10_UINT_PACK32 },
		{ "A2R10G10B10_SINT_PACK32", ext::opengl::enums::Format::A2R10G10B10_SINT_PACK32 },
		{ "A2B10G10R10_UNORM_PACK32", ext::opengl::enums::Format::A2B10G10R10_UNORM_PACK32 },
		{ "A2B10G10R10_SNORM_PACK32", ext::opengl::enums::Format::A2B10G10R10_SNORM_PACK32 },
		{ "A2B10G10R10_USCALED_PACK32", ext::opengl::enums::Format::A2B10G10R10_USCALED_PACK32 },
		{ "A2B10G10R10_SSCALED_PACK32", ext::opengl::enums::Format::A2B10G10R10_SSCALED_PACK32 },
		{ "A2B10G10R10_UINT_PACK32", ext::opengl::enums::Format::A2B10G10R10_UINT_PACK32 },
		{ "A2B10G10R10_SINT_PACK32", ext::opengl::enums::Format::A2B10G10R10_SINT_PACK32 },
		{ "R16_UNORM", ext::opengl::enums::Format::R16_UNORM },
		{ "R16_SNORM", ext::opengl::enums::Format::R16_SNORM },
		{ "R16_USCALED", ext::opengl::enums::Format::R16_USCALED },
		{ "R16_SSCALED", ext::opengl::enums::Format::R16_SSCALED },
		{ "R16_UINT", ext::opengl::enums::Format::R16_UINT },
		{ "R16_SINT", ext::opengl::enums::Format::R16_SINT },
		{ "R16_SFLOAT", ext::opengl::enums::Format::R16_SFLOAT },
		{ "R16G16_UNORM", ext::opengl::enums::Format::R16G16_UNORM },
		{ "R16G16_SNORM", ext::opengl::enums::Format::R16G16_SNORM },
		{ "R16G16_USCALED", ext::opengl::enums::Format::R16G16_USCALED },
		{ "R16G16_SSCALED", ext::opengl::enums::Format::R16G16_SSCALED },
		{ "R16G16_UINT", ext::opengl::enums::Format::R16G16_UINT },
		{ "R16G16_SINT", ext::opengl::enums::Format::R16G16_SINT },
		{ "R16G16_SFLOAT", ext::opengl::enums::Format::R16G16_SFLOAT },
		{ "R16G16B16_UNORM", ext::opengl::enums::Format::R16G16B16_UNORM },
		{ "R16G16B16_SNORM", ext::opengl::enums::Format::R16G16B16_SNORM },
		{ "R16G16B16_USCALED", ext::opengl::enums::Format::R16G16B16_USCALED },
		{ "R16G16B16_SSCALED", ext::opengl::enums::Format::R16G16B16_SSCALED },
		{ "R16G16B16_UINT", ext::opengl::enums::Format::R16G16B16_UINT },
		{ "R16G16B16_SINT", ext::opengl::enums::Format::R16G16B16_SINT },
		{ "R16G16B16_SFLOAT", ext::opengl::enums::Format::R16G16B16_SFLOAT },
		{ "R16G16B16A16_UNORM", ext::opengl::enums::Format::R16G16B16A16_UNORM },
		{ "R16G16B16A16_SNORM", ext::opengl::enums::Format::R16G16B16A16_SNORM },
		{ "R16G16B16A16_USCALED", ext::opengl::enums::Format::R16G16B16A16_USCALED },
		{ "R16G16B16A16_SSCALED", ext::opengl::enums::Format::R16G16B16A16_SSCALED },
		{ "R16G16B16A16_UINT", ext::opengl::enums::Format::R16G16B16A16_UINT },
		{ "R16G16B16A16_SINT", ext::opengl::enums::Format::R16G16B16A16_SINT },
		{ "R16G16B16A16_SFLOAT", ext::opengl::enums::Format::R16G16B16A16_SFLOAT },
		{ "R32_UINT", ext::opengl::enums::Format::R32_UINT },
		{ "R32_SINT", ext::opengl::enums::Format::R32_SINT },
		{ "R32_SFLOAT", ext::opengl::enums::Format::R32_SFLOAT },
		{ "R32G32_UINT", ext::opengl::enums::Format::R32G32_UINT },
		{ "R32G32_SINT", ext::opengl::enums::Format::R32G32_SINT },
		{ "R32G32_SFLOAT", ext::opengl::enums::Format::R32G32_SFLOAT },
		{ "R32G32B32_UINT", ext::opengl::enums::Format::R32G32B32_UINT },
		{ "R32G32B32_SINT", ext::opengl::enums::Format::R32G32B32_SINT },
		{ "R32G32B32_SFLOAT", ext::opengl::enums::Format::R32G32B32_SFLOAT },
		{ "R32G32B32A32_UINT", ext::opengl::enums::Format::R32G32B32A32_UINT },
		{ "R32G32B32A32_SINT", ext::opengl::enums::Format::R32G32B32A32_SINT },
		{ "R32G32B32A32_SFLOAT", ext::opengl::enums::Format::R32G32B32A32_SFLOAT },
		{ "R64_UINT", ext::opengl::enums::Format::R64_UINT },
		{ "R64_SINT", ext::opengl::enums::Format::R64_SINT },
		{ "R64_SFLOAT", ext::opengl::enums::Format::R64_SFLOAT },
		{ "R64G64_UINT", ext::opengl::enums::Format::R64G64_UINT },
		{ "R64G64_SINT", ext::opengl::enums::Format::R64G64_SINT },
		{ "R64G64_SFLOAT", ext::opengl::enums::Format::R64G64_SFLOAT },
		{ "R64G64B64_UINT", ext::opengl::enums::Format::R64G64B64_UINT },
		{ "R64G64B64_SINT", ext::opengl::enums::Format::R64G64B64_SINT },
		{ "R64G64B64_SFLOAT", ext::opengl::enums::Format::R64G64B64_SFLOAT },
		{ "R64G64B64A64_UINT", ext::opengl::enums::Format::R64G64B64A64_UINT },
		{ "R64G64B64A64_SINT", ext::opengl::enums::Format::R64G64B64A64_SINT },
		{ "R64G64B64A64_SFLOAT", ext::opengl::enums::Format::R64G64B64A64_SFLOAT },
		{ "B10G11R11_UFLOAT_PACK32", ext::opengl::enums::Format::B10G11R11_UFLOAT_PACK32 },
		{ "E5B9G9R9_UFLOAT_PACK32", ext::opengl::enums::Format::E5B9G9R9_UFLOAT_PACK32 },
		{ "D16_UNORM", ext::opengl::enums::Format::D16_UNORM },
		{ "X8_D24_UNORM_PACK32", ext::opengl::enums::Format::X8_D24_UNORM_PACK32 },
		{ "D32_SFLOAT", ext::opengl::enums::Format::D32_SFLOAT },
		{ "S8_UINT", ext::opengl::enums::Format::S8_UINT },
		{ "D16_UNORM_S8_UINT", ext::opengl::enums::Format::D16_UNORM_S8_UINT },
		{ "D24_UNORM_S8_UINT", ext::opengl::enums::Format::D24_UNORM_S8_UINT },
		{ "D32_SFLOAT_S8_UINT", ext::opengl::enums::Format::D32_SFLOAT_S8_UINT },
		{ "BC1_RGB_UNORM_BLOCK", ext::opengl::enums::Format::BC1_RGB_UNORM_BLOCK },
		{ "BC1_RGB_SRGB_BLOCK", ext::opengl::enums::Format::BC1_RGB_SRGB_BLOCK },
		{ "BC1_RGBA_UNORM_BLOCK", ext::opengl::enums::Format::BC1_RGBA_UNORM_BLOCK },
		{ "BC1_RGBA_SRGB_BLOCK", ext::opengl::enums::Format::BC1_RGBA_SRGB_BLOCK },
		{ "BC2_UNORM_BLOCK", ext::opengl::enums::Format::BC2_UNORM_BLOCK },
		{ "BC2_SRGB_BLOCK", ext::opengl::enums::Format::BC2_SRGB_BLOCK },
		{ "BC3_UNORM_BLOCK", ext::opengl::enums::Format::BC3_UNORM_BLOCK },
		{ "BC3_SRGB_BLOCK", ext::opengl::enums::Format::BC3_SRGB_BLOCK },
		{ "BC4_UNORM_BLOCK", ext::opengl::enums::Format::BC4_UNORM_BLOCK },
		{ "BC4_SNORM_BLOCK", ext::opengl::enums::Format::BC4_SNORM_BLOCK },
		{ "BC5_UNORM_BLOCK", ext::opengl::enums::Format::BC5_UNORM_BLOCK },
		{ "BC5_SNORM_BLOCK", ext::opengl::enums::Format::BC5_SNORM_BLOCK },
		{ "BC6H_UFLOAT_BLOCK", ext::opengl::enums::Format::BC6H_UFLOAT_BLOCK },
		{ "BC6H_SFLOAT_BLOCK", ext::opengl::enums::Format::BC6H_SFLOAT_BLOCK },
		{ "BC7_UNORM_BLOCK", ext::opengl::enums::Format::BC7_UNORM_BLOCK },
		{ "BC7_SRGB_BLOCK", ext::opengl::enums::Format::BC7_SRGB_BLOCK },
		{ "ETC2_R8G8B8_UNORM_BLOCK", ext::opengl::enums::Format::ETC2_R8G8B8_UNORM_BLOCK },
		{ "ETC2_R8G8B8_SRGB_BLOCK", ext::opengl::enums::Format::ETC2_R8G8B8_SRGB_BLOCK },
		{ "ETC2_R8G8B8A1_UNORM_BLOCK", ext::opengl::enums::Format::ETC2_R8G8B8A1_UNORM_BLOCK },
		{ "ETC2_R8G8B8A1_SRGB_BLOCK", ext::opengl::enums::Format::ETC2_R8G8B8A1_SRGB_BLOCK },
		{ "ETC2_R8G8B8A8_UNORM_BLOCK", ext::opengl::enums::Format::ETC2_R8G8B8A8_UNORM_BLOCK },
		{ "ETC2_R8G8B8A8_SRGB_BLOCK", ext::opengl::enums::Format::ETC2_R8G8B8A8_SRGB_BLOCK },
		{ "EAC_R11_UNORM_BLOCK", ext::opengl::enums::Format::EAC_R11_UNORM_BLOCK },
		{ "EAC_R11_SNORM_BLOCK", ext::opengl::enums::Format::EAC_R11_SNORM_BLOCK },
		{ "EAC_R11G11_UNORM_BLOCK", ext::opengl::enums::Format::EAC_R11G11_UNORM_BLOCK },
		{ "EAC_R11G11_SNORM_BLOCK", ext::opengl::enums::Format::EAC_R11G11_SNORM_BLOCK },
		{ "ASTC_4x4_UNORM_BLOCK", ext::opengl::enums::Format::ASTC_4x4_UNORM_BLOCK },
		{ "ASTC_4x4_SRGB_BLOCK", ext::opengl::enums::Format::ASTC_4x4_SRGB_BLOCK },
		{ "ASTC_5x4_UNORM_BLOCK", ext::opengl::enums::Format::ASTC_5x4_UNORM_BLOCK },
		{ "ASTC_5x4_SRGB_BLOCK", ext::opengl::enums::Format::ASTC_5x4_SRGB_BLOCK },
		{ "ASTC_5x5_UNORM_BLOCK", ext::opengl::enums::Format::ASTC_5x5_UNORM_BLOCK },
		{ "ASTC_5x5_SRGB_BLOCK", ext::opengl::enums::Format::ASTC_5x5_SRGB_BLOCK },
		{ "ASTC_6x5_UNORM_BLOCK", ext::opengl::enums::Format::ASTC_6x5_UNORM_BLOCK },
		{ "ASTC_6x5_SRGB_BLOCK", ext::opengl::enums::Format::ASTC_6x5_SRGB_BLOCK },
		{ "ASTC_6x6_UNORM_BLOCK", ext::opengl::enums::Format::ASTC_6x6_UNORM_BLOCK },
		{ "ASTC_6x6_SRGB_BLOCK", ext::opengl::enums::Format::ASTC_6x6_SRGB_BLOCK },
		{ "ASTC_8x5_UNORM_BLOCK", ext::opengl::enums::Format::ASTC_8x5_UNORM_BLOCK },
		{ "ASTC_8x5_SRGB_BLOCK", ext::opengl::enums::Format::ASTC_8x5_SRGB_BLOCK },
		{ "ASTC_8x6_UNORM_BLOCK", ext::opengl::enums::Format::ASTC_8x6_UNORM_BLOCK },
		{ "ASTC_8x6_SRGB_BLOCK", ext::opengl::enums::Format::ASTC_8x6_SRGB_BLOCK },
		{ "ASTC_8x8_UNORM_BLOCK", ext::opengl::enums::Format::ASTC_8x8_UNORM_BLOCK },
		{ "ASTC_8x8_SRGB_BLOCK", ext::opengl::enums::Format::ASTC_8x8_SRGB_BLOCK },
		{ "ASTC_10x5_UNORM_BLOCK", ext::opengl::enums::Format::ASTC_10x5_UNORM_BLOCK },
		{ "ASTC_10x5_SRGB_BLOCK", ext::opengl::enums::Format::ASTC_10x5_SRGB_BLOCK },
		{ "ASTC_10x6_UNORM_BLOCK", ext::opengl::enums::Format::ASTC_10x6_UNORM_BLOCK },
		{ "ASTC_10x6_SRGB_BLOCK", ext::opengl::enums::Format::ASTC_10x6_SRGB_BLOCK },
		{ "ASTC_10x8_UNORM_BLOCK", ext::opengl::enums::Format::ASTC_10x8_UNORM_BLOCK },
		{ "ASTC_10x8_SRGB_BLOCK", ext::opengl::enums::Format::ASTC_10x8_SRGB_BLOCK },
		{ "ASTC_10x10_UNORM_BLOCK", ext::opengl::enums::Format::ASTC_10x10_UNORM_BLOCK },
		{ "ASTC_10x10_SRGB_BLOCK", ext::opengl::enums::Format::ASTC_10x10_SRGB_BLOCK },
		{ "ASTC_12x10_UNORM_BLOCK", ext::opengl::enums::Format::ASTC_12x10_UNORM_BLOCK },
		{ "ASTC_12x10_SRGB_BLOCK", ext::opengl::enums::Format::ASTC_12x10_SRGB_BLOCK },
		{ "ASTC_12x12_UNORM_BLOCK", ext::opengl::enums::Format::ASTC_12x12_UNORM_BLOCK },
		{ "ASTC_12x12_SRGB_BLOCK", ext::opengl::enums::Format::ASTC_12x12_SRGB_BLOCK }
	};
	return table[string];
}

#endif