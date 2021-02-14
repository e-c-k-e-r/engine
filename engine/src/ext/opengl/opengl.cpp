#if UF_USE_OPENGL

#include <uf/ext/opengl/opengl.h>
#include <uf/ext/opengl/initializers.h>
#include <uf/ext/opengl/graphic.h>
#include <uf/ext/opengl/rendermode.h>
#include <uf/utils/graphic/graphic.h>

#include <uf/ext/openvr/openvr.h>

#include <ostream>
#include <fstream>
#include <atomic>

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
bool ext::opengl::settings::experimental::individualPipelines = false;
bool ext::opengl::settings::experimental::multithreadedCommandRecording = false;
std::string ext::opengl::settings::experimental::deferredMode = "";
bool ext::opengl::settings::experimental::deferredReconstructPosition = false;
bool ext::opengl::settings::experimental::deferredAliasOutputToSwapchain = true;
bool ext::opengl::settings::experimental::multiview = true;
bool ext::opengl::settings::experimental::hdr = true;

GLhandle(VkColorSpaceKHR) ext::opengl::settings::formats::colorSpace;
ext::opengl::enums::Format::type_t ext::opengl::settings::formats::color = ext::opengl::enums::Format::R8G8B8A8_UNORM;
ext::opengl::enums::Format::type_t ext::opengl::settings::formats::depth = ext::opengl::enums::Format::D32_SFLOAT;
ext::opengl::enums::Format::type_t ext::opengl::settings::formats::normal = ext::opengl::enums::Format::R16G16B16A16_SFLOAT;
ext::opengl::enums::Format::type_t ext::opengl::settings::formats::position = ext::opengl::enums::Format::R16G16B16A16_SFLOAT;

ext::opengl::Device ext::opengl::device;
std::mutex ext::opengl::mutex;

bool ext::opengl::states::resized = false;
bool ext::opengl::states::rebuild = false;
uint32_t ext::opengl::states::currentBuffer = 0;

std::vector<uf::Scene*> ext::opengl::scenes;
ext::opengl::RenderMode* ext::opengl::currentRenderMode = NULL;
std::vector<ext::opengl::RenderMode*> ext::opengl::renderModes = {
	new ext::opengl::BaseRenderMode,
};

std::string UF_API ext::opengl::errorString() {
#if UF_ENV_DREAMCAST
#else
	GLenum error;
	const GLubyte* bytes = NULL;
	if ((error = glGetError()) != GL_NO_ERROR) bytes = gluErrorString(error);
	std::string str = (const char*) bytes;
	return str;
#endif
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
	if ( !jobs.empty() ) {
		uf::thread::batchWorkers( jobs );
	}
	{
		std::vector<uint8_t> pixels = { 
			255,   0, 255, 255,      0,   0,   0, 255,
			  0,   0,   0, 255,    255,   0, 255, 255,
		};
		Texture2D::empty.sampler.descriptor.filter.min = uf::renderer::enums::Filter::NEAREST;
		Texture2D::empty.sampler.descriptor.filter.mag = uf::renderer::enums::Filter::NEAREST;
		Texture2D::empty.fromBuffers( (void*) &pixels[0], pixels.size(), uf::renderer::enums::Format::R8G8B8A8_UNORM, 2, 2 );
	}
}
void UF_API ext::opengl::tick(){
	ext::opengl::mutex.lock();
	if ( ext::opengl::states::resized || ext::opengl::settings::experimental::rebuildOnTickBegin ) {
		ext::opengl::states::rebuild = true;
	}

	std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
		if ( !entity->hasComponent<uf::Graphic>() ) return;
		ext::opengl::Graphic& graphic = entity->getComponent<uf::Graphic>();
		if ( graphic.initialized || !graphic.process || graphic.initialized ) return;
		graphic.initializePipeline();
		ext::opengl::states::rebuild = true;
	};
	for ( uf::Scene* scene : uf::scene::scenes ) {
		if ( !scene ) continue;
		scene->process(filter);
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
			if ( settings::experimental::multithreadedCommandRecording ) {
				jobs.emplace_back([&]{
					renderMode->createCommandBuffers();
					return 0;
				});
			} else {
				renderMode->createCommandBuffers();
			}
		}
	}
	if ( !jobs.empty() ) {
		uf::thread::batchWorkers( jobs );
	}
	
	ext::opengl::states::rebuild = false;
	ext::opengl::states::resized = false;
	ext::opengl::mutex.unlock();
}
void UF_API ext::opengl::render(){
	ext::opengl::mutex.lock();
	if ( hasRenderMode("Gui", true) ) {
		RenderMode& primary = getRenderMode("Gui", true);
		auto it = std::find( renderModes.begin(), renderModes.end(), &primary );
		if ( it + 1 != renderModes.end() ) std::rotate( it, it + 1, renderModes.end() );
	}
	if ( hasRenderMode("", true) ) {
		RenderMode& primary = getRenderMode("", true);
		auto it = std::find( renderModes.begin(), renderModes.end(), &primary );
		if ( it + 1 != renderModes.end() ) std::rotate( it, it + 1, renderModes.end() );
	}

	for ( auto& renderMode : renderModes ) {
		if ( !renderMode ) continue;
		if ( !renderMode->execute ) continue;
		ext::opengl::currentRenderMode = renderMode;
		for ( uf::Scene* scene : uf::scene::scenes ) scene->render();
		renderMode->render();
	}

	ext::opengl::currentRenderMode = NULL;
	if ( ext::opengl::settings::experimental::waitOnRenderEnd ) {
		synchronize();
	}
#if UF_USE_OPENVR
/*
	if ( ext::openvr::context ) {
		ext::openvr::postSubmit();
	}
*/
#endif
	ext::opengl::mutex.unlock();
}
void UF_API ext::opengl::destroy() {
	ext::opengl::mutex.lock();
	synchronize();

	Texture2D::empty.destroy();

	std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
		if ( !entity->hasComponent<uf::Graphic>() ) return;
		uf::Graphic& graphic = entity->getComponent<uf::Graphic>();
		graphic.destroy();
	};
	for ( uf::Scene* scene : uf::scene::scenes ) {
		if ( !scene ) continue;
		scene->process(filter);
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
ext::opengl::enums::Format::type_t UF_API ext::opengl::formatFromString( const std::string& ){
	return ext::opengl::enums::Format::R8G8B8A8_UNORM;
}

#endif