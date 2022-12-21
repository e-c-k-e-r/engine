#include "main.h"

#include <uf/ext/ext.h>
#include <uf/ext/oal/oal.h>

#include <uf/spec/terminal/terminal.h>
#include <uf/spec/controller/controller.h>

#include <uf/utils/time/time.h>
#include <uf/utils/audio/audio.h>
#include <uf/utils/hook/hook.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/string/string.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/image/image.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/http/http.h>

#include <uf/engine/entity/entity.h>
#include <uf/engine/graph/graph.h>
#include <uf/utils/io/inputs.h>

#include <sys/stat.h>

#include <uf/utils/memory/string.h>
#include <fstream>
#include <iostream>

#include <regex>

#include "ext.h"

#include <uf/engine/scene/scene.h>
#include <uf/engine/asset/asset.h>

#include <uf/utils/renderer/renderer.h>
#include <uf/ext/discord/discord.h>
#include <uf/ext/openvr/openvr.h>
#include <uf/ext/lua/lua.h>
#include <uf/ext/ultralight/ultralight.h>
#include <uf/ext/imgui/imgui.h>
#include <uf/ext/ffx/fsr.h>

bool ext::ready = false;
uf::stl::vector<uf::stl::string> ext::arguments;
uf::Serializer ext::config;

namespace {
	struct {
		uf::String input;
		std::ofstream output;

		struct {
			uf::stl::string output = uf::io::root+"/logs/output.txt";
		} filenames;
	} io;

	struct {
		uf::Timer<> sys = uf::Timer<>(false);
		size_t frames = 0;
		float limiter = 1.0 / 144.0;
		struct {
			size_t frames = 0;
			float time = 0;
		} total;
	} times;

	auto& json = ext::config;

	struct {
		struct {
			struct {
				size_t mode;
				bool announce;
				float every;
			} gc;

			struct {
				struct {
					bool enabled;
				} ultralight, discord, imgui;
			} ext;

			struct {
				bool print;
				float every;
			} limiter, fps;
		} engine;
	} config;

	bool requestDedicatedRenderThread = false;
}

void EXT_API ext::load() {
	ext::config.readFromFile(uf::io::root+"/config.json");
}

void EXT_API ext::initialize() {
	/* Setup deferred Main thread */ {
		uf::thread::get(uf::thread::mainThreadName);
	}
	/* set JSON implicit preferences */ {
		ext::json::PREFERRED_ENCODING = ::json["engine"]["ext"]["json"]["encoding"].as(ext::json::PREFERRED_ENCODING);
		ext::json::PREFERRED_COMPRESSION = ::json["engine"]["ext"]["json"]["compression"].as(ext::json::PREFERRED_COMPRESSION);

		UF_MSG_DEBUG("Setting JSON implicit preference: {}.{}", ext::json::PREFERRED_ENCODING, ext::json::PREFERRED_COMPRESSION);
	}

	/* Arguments */ {
		bool modified = false;
		auto& arguments = ::json["arguments"];
		for ( auto& arg : ext::arguments ) {
			// store raw argument
			int i = arguments.size();
			arguments[i] = arg;
			// parse --key=value
			{
				std::regex regex("^--(.+?)=(.+?)$");
				std::smatch match;
				if ( std::regex_search( arg, match, regex ) ) {
					uf::stl::string keyString = match[1].str();
					uf::stl::string valueString = match[2].str();
					uf::Serializer value; value.deserialize(valueString);
					::json.path(keyString) = value;
					modified = true;
				}
			}
		}
	//	UF_MSG_DEBUG("Arguments: {}", uf::Serializer(arguments));
		if ( modified ) UF_MSG_DEBUG("New config: {}", ::json.serialize());
	}
	/* Seed */ {
		srand(time(NULL));
	}
	/* Open output file */ {
		io.output.open(io.filenames.output);
	}
	/* Initialize timers */ {
		times.sys.start();
	}
	/* Read persistent data */ {
		// #include "./inits/persistence.inl"
	}

	/* Set memory pool sizes */ {
		auto& configMemoryPoolJson = ::json["engine"]["memory pool"];

		// check if we are even allowed to use memory pools
		bool enabled = configMemoryPoolJson["enabled"].as(true);
		auto deduceSize = [enabled]( const ext::json::Value& value )->size_t{
			if ( !enabled ) return 0;
			if ( value.is<size_t>() ) return value.as<size_t>();
			if ( value.is<uf::stl::string>() ) {
				uf::stl::string str = value.as<uf::stl::string>();
				std::regex regex("^(\\d+) ?((?:K|M|G)?(?:i?B)?)$");
				std::smatch match;
				if ( std::regex_search( str, match, regex ) ) {
					size_t requested = std::stoi( match[1].str() );
					uf::stl::string prefix = match[2].str();
					switch ( prefix.at(0) ) {
						case 'K': return requested * 1024;
						case 'M': return requested * 1024 * 1024;
						case 'G': return requested * 1024 * 1024 * 1024;
					}
					return requested;
				}
			}
			return 0;
		};
		// set memory pool alignment requirements
		uf::memoryPool::alignment = configMemoryPoolJson["alignment"].as( uf::memoryPool::alignment );
		// set memory pool sizes
		size_t size = deduceSize( configMemoryPoolJson["size"] );
		UF_MSG_DEBUG("Requesting {} bytes for global memory pool: {}", (size_t) size, (void*) &uf::memoryPool::global);
		uf::memoryPool::global.initialize( size );
		uf::memoryPool::subPool = configMemoryPoolJson["subPools"].as( uf::memoryPool::subPool );
		if ( size <= 0 || uf::memoryPool::subPool ) {
			{
				size_t size = deduceSize( configMemoryPoolJson["pools"]["component"] );
				UF_MSG_DEBUG("Requesting {} bytes for component memory pool: {}", (int) size, (void*) &uf::component::memoryPool);
				uf::component::memoryPool.initialize( size );
			}
			{
				size_t size = deduceSize( configMemoryPoolJson["pools"]["userdata"] );
				UF_MSG_DEBUG("Requesting {} bytes for userdata memory pool: {}", (int) size, (void*) &uf::userdata::memoryPool);
				uf::userdata::memoryPool.initialize( size );
			}
			{
				size_t size = deduceSize( configMemoryPoolJson["pools"]["entity"] );
				UF_MSG_DEBUG("Requesting {} bytes for entity memory pool: {}", (int) size, (void*) &uf::Entity::memoryPool);
				uf::Entity::memoryPool.initialize( size );
			}
		}
		uf::allocator::override = configMemoryPoolJson["override"].as( uf::allocator::override );
	}
	/* Ext config */ {
		::config.engine.gc.every = ::json["engine"]["debug"]["garbage collection"]["every"].as(::config.engine.gc.every);
		::config.engine.gc.mode = ::json["engine"]["debug"]["garbage collection"]["mode"].as(::config.engine.gc.mode);
		::config.engine.gc.announce = ::json["engine"]["debug"]["garbage collection"]["announce"].as(::config.engine.gc.announce);

		::config.engine.ext.ultralight.enabled = ::json["engine"]["ext"]["ultralight"]["enabled"].as(::config.engine.ext.ultralight.enabled);
		::config.engine.ext.discord.enabled = ::json["engine"]["ext"]["discord"]["enabled"].as(::config.engine.ext.discord.enabled);
		::config.engine.ext.imgui.enabled = ::json["engine"]["ext"]["imgui"]["enabled"].as(::config.engine.ext.imgui.enabled);

		::config.engine.limiter.print = ::json["engine"]["debug"]["framerate"]["print"].as(::config.engine.limiter.print);

		::config.engine.fps.print = ::json["engine"]["debug"]["framerate"]["print"].as(::config.engine.fps.print);
		::config.engine.fps.every = ::json["engine"]["debug"]["framerate"]["every"].as(::config.engine.fps.every);
	}
	{
		uf::Mesh::defaultInterleaved = ::json["engine"]["scenes"]["meshes"]["interleaved"].as( uf::Mesh::defaultInterleaved );
	#if UF_USE_OPENGL
		uf::matrix::reverseInfiniteProjection = false;
	#else
		uf::matrix::reverseInfiniteProjection = ::json["engine"]["scenes"]["matrix"]["reverseInfinite"].as( uf::matrix::reverseInfiniteProjection );
	#endif

		uf::graph::initialBufferElements = ::json["engine"]["graph"]["initial buffer elements"].as(uf::graph::initialBufferElements);
	}

	/* Create initial scene (kludge) */ {
		uf::Scene& scene = uf::instantiator::instantiate<uf::Scene>(); //new uf::Scene;
		uf::scene::scenes.emplace_back(&scene);
		auto& metadata = scene.getComponent<uf::Serializer>();
		metadata["system"]["config"] = ::json;
	}

	{
		uf::Entity::deleteChildrenOnDestroy = ::json["engine"]["debug"]["entity"]["delete children on destroy"].as( uf::Entity::deleteChildrenOnDestroy );
		uf::Entity::deleteComponentsOnDestroy = ::json["engine"]["debug"]["entity"]["delete components on destroy"].as( uf::Entity::deleteComponentsOnDestroy );

		uf::Object::assertionLoad = ::json["engine"]["debug"]["loader"]["assert"].as( uf::Object::assertionLoad );
		uf::asset::assertionLoad = ::json["engine"]["debug"]["loader"]["assert"].as( uf::asset::assertionLoad );
		
		uf::userdata::autoDestruct = ::json["engine"]["debug"]["userdata"]["auto destruct"].as( uf::userdata::autoDestruct );
		uf::userdata::autoValidate = ::json["engine"]["debug"]["userdata"]["auto validate"].as( uf::userdata::autoValidate );
		
		uf::Object::deferLazyCalls = ::json["engine"]["debug"]["hooks"]["defer lazy calls"].as( uf::Object::deferLazyCalls );
		uf::scene::printTaskCalls = ::json["engine"]["debug"]["scene"]["print task calls"].as( uf::scene::printTaskCalls );
	}

	{
		auto& configEngineLimitersJson = ::json["engine"]["limiters"];

		if ( configEngineLimitersJson["framerate"].as<uf::stl::string>() == "auto" && ::json["window"]["refresh rate"].is<size_t>() ) {
			float scale = 1.0;
			size_t refreshRate = ::json["window"]["refresh rate"].as<size_t>();
			configEngineLimitersJson["framerate"] = refreshRate * scale;
			UF_MSG_DEBUG("Setting framerate cap to {}", (int) refreshRate * scale);
		}

		/* Frame limiter */ {
			size_t limit = configEngineLimitersJson["framerate"].as<size_t>();
			::times.limiter = limit != 0 ? 1.0 / limit : 0;
			UF_MSG_DEBUG("Limiter set to {} ms", ::times.limiter);
		}
		/* Max delta time */{
			size_t limit = configEngineLimitersJson["deltaTime"].as<size_t>();
			uf::physics::time::clamp = limit != 0 ? 1.0 / limit : 0;
		}
		
	}
	{
		auto& configEngineThreadJson = ::json["engine"]["threads"];

		if ( configEngineThreadJson["frame limiter"].as<uf::stl::string>() == "auto" && ::json["window"]["refresh rate"].is<size_t>() ) {
			float scale = 2.0;
			size_t refreshRate = ::json["window"]["refresh rate"].as<size_t>();
			configEngineThreadJson["frame limiter"] = refreshRate * scale;
			UF_MSG_DEBUG("Setting thread frame limiter to {}", (int) refreshRate * scale);
		}

		/* Thread frame limiter */ {
			size_t limit = configEngineThreadJson["frame limiter"].as<size_t>();
			uf::thread::limiter = limit != 0 ? 1.0 / limit : 0;
		}

		// Set worker threads
		if ( configEngineThreadJson["workers"].as<uf::stl::string>() == "auto" ) {
			auto threads = std::max( 1, (int) std::thread::hardware_concurrency() - 1 ) / 2;
			configEngineThreadJson["workers"] = threads;
			uf::thread::workers = configEngineThreadJson["workers"].as<size_t>();
			UF_MSG_DEBUG("Using {} worker threads", threads);
		} else if ( configEngineThreadJson["workers"].is<size_t>() ) {
			auto threads = configEngineThreadJson["workers"].as<size_t>();
			uf::thread::workers = threads;
			UF_MSG_DEBUG("Using {} worker threads", threads);
		}
	}
	// Audio settings
	{	
		auto& configEngineAudioJson = ::json["engine"]["audio"];

		uf::audio::muted = configEngineAudioJson["mute"].as( uf::audio::muted );
		uf::audio::streamsByDefault = configEngineAudioJson["streams by default"].as( uf::audio::streamsByDefault );
		uf::audio::bufferSize = configEngineAudioJson["buffers"]["size"].as( uf::audio::bufferSize );
		uf::audio::buffers = configEngineAudioJson["buffers"]["count"].as( uf::audio::buffers );
	#if UF_AUDIO_MAPPED_VOLUMES
		ext::json::forEach( configEngineAudioJson["volumes"], []( const uf::stl::string& key, ext::json::Value& value ){
			float volume; volume = value.as(volume);
			uf::audio::volumes[key] = volume;
		});
	#else
		if ( ext::json::isObject( configEngineAudioJson["volumes"] ) ) {
			uf::audio::volumes::bgm = configEngineAudioJson["volumes"]["bgm"].as(uf::audio::volumes::bgm);
			uf::audio::volumes::sfx = configEngineAudioJson["volumes"]["sfx"].as(uf::audio::volumes::sfx);
			uf::audio::volumes::voice = configEngineAudioJson["volumes"]["voice"].as(uf::audio::volumes::voice);
		}
	#endif
	}

#if UF_USE_OPENAL
	/* Initialize OpenAL */ {
		ext::al::initialize();
	}
#endif

	// set physics parameters
#if UF_USE_REACTPHYSICS
	{
		auto& configEngineReactJson = ::json["engine"]["ext"]["reactphysics"];

		ext::reactphysics::timescale = configEngineReactJson["timescale"].as( ext::reactphysics::timescale );
		ext::reactphysics::interpolate = configEngineReactJson["interpolate"].as( ext::reactphysics::interpolate );
		ext::reactphysics::shared = configEngineReactJson["shared"].as( ext::reactphysics::shared );
		
		if ( configEngineReactJson["gravity"]["mode"].is<uf::stl::string>() ) {
			const auto mode = uf::string::lowercase( configEngineReactJson["gravity"]["mode"].as<uf::stl::string>() );
			if ( mode == "default" ) ext::reactphysics::gravity::mode = ext::reactphysics::gravity::Mode::DEFAULT;
			else if ( mode == "per-object" ) ext::reactphysics::gravity::mode = ext::reactphysics::gravity::Mode::PER_OBJECT;
			else if ( mode == "universal" ) ext::reactphysics::gravity::mode = ext::reactphysics::gravity::Mode::UNIVERSAL;
			else UF_MSG_WARNING("Invalid Gravity::Mode enum string specified: {}", mode);
		}
		ext::reactphysics::gravity::constant = configEngineReactJson["gravity"]["constant"].as( ext::reactphysics::gravity::constant );

		ext::reactphysics::debugDraw::enabled = configEngineReactJson["debug draw"]["enabled"].as( ext::reactphysics::debugDraw::enabled );
		ext::reactphysics::debugDraw::rate = configEngineReactJson["debug draw"]["rate"].as( ext::reactphysics::debugDraw::rate );
		ext::reactphysics::debugDraw::layer = configEngineReactJson["debug draw"]["layer"].as( ext::reactphysics::debugDraw::layer );
		ext::reactphysics::debugDraw::lineWidth = configEngineReactJson["debug draw"]["line width"].as( ext::reactphysics::debugDraw::lineWidth );
	}
#endif

	// renderer settings
	{
	#if UF_USE_VULKAN
		auto& configRenderJson = ::json["engine"]["ext"]["vulkan"];
	#elif UF_USE_OPENGL
		auto& configRenderJson = ::json["engine"]["ext"]["opengl"];
	#else
		auto& configRenderJson = ::json["engine"]["ext"]["software"];
	#endif
		auto& configRenderInvariantJson = configRenderJson["invariant"];
		auto& configRenderExperimentalJson = configRenderJson["experimental"];
		auto& configRenderPipelinesJson = configRenderJson["pipelines"];

		uf::renderer::settings::validation = configRenderJson["validation"]["enabled"].as( uf::renderer::settings::validation );
		uf::renderer::settings::msaa = configRenderJson["framebuffer"]["msaa"].as( uf::renderer::settings::msaa );
		uf::renderer::settings::defaultStageBuffers = configRenderJson["invariant"]["default stage buffers"].as( uf::renderer::settings::defaultStageBuffers );
		uf::renderer::settings::defaultDeferBufferDestroy = configRenderJson["invariant"]["default defer buffer destroy"].as( uf::renderer::settings::defaultDeferBufferDestroy );
		uf::renderer::settings::defaultCommandBufferWait = configRenderJson["invariant"]["default command buffer wait"].as( uf::renderer::settings::defaultCommandBufferWait );

		if ( configRenderJson["framebuffer"]["size"].is<float>() ) {
		//	float scale = configRenderJson["framebuffer"]["size"].as(1.0f);
		//	uf::renderer::settings::width *= scale;
		//	uf::renderer::settings::height *= scale;
		} else if ( ext::json::isArray( configRenderJson["framebuffer"]["size"] ) ) {
			uf::renderer::settings::width = configRenderJson["framebuffer"]["size"][0].as(uf::renderer::settings::width);
			uf::renderer::settings::height = configRenderJson["framebuffer"]["size"][1].as(uf::renderer::settings::height);
			uf::stl::string filter =  uf::string::lowercase( configRenderJson["framebuffer"]["size"][2].as<uf::stl::string>() );

			if ( filter == "nearest" ) uf::renderer::settings::swapchainUpscaleFilter = uf::renderer::enums::Filter::NEAREST;
			else if ( filter == "linear" ) uf::renderer::settings::swapchainUpscaleFilter = uf::renderer::enums::Filter::LINEAR;
		}

	#if UF_USE_VULKAN
		uf::renderer::settings::version = configRenderJson["version"].as<float>(1.3);

		if ( configRenderJson["gpu"].as<uf::stl::string>() == "auto" ) {
			uf::renderer::settings::gpuID = -1;
		} else {
			uf::renderer::settings::gpuID = configRenderJson["gpu"].as(uf::renderer::settings::gpuID);
		}
		for ( int i = 0; i < configRenderJson["validation"]["filters"].size(); ++i ) {
			uf::renderer::settings::validationFilters.emplace_back( configRenderJson["validation"]["filters"][i].as<uf::stl::string>() );
		}

		#define VK_LOAD_VERSION_LEVEL(VERSION) if ( VERSION <= uf::renderer::settings::version ) {\
			auto& configVersionLevel = configRenderJson["versions"][#VERSION];\
			for ( int i = 0; i < configVersionLevel["extensions"]["device"].size(); ++i ) {\
				uf::renderer::settings::requestedDeviceExtensions.emplace_back( configVersionLevel["extensions"]["device"][i].as<uf::stl::string>() );\
			}\
			for ( int i = 0; i < configVersionLevel["extensions"]["instance"].size(); ++i ) {\
				uf::renderer::settings::requestedInstanceExtensions.emplace_back( configVersionLevel["extensions"]["instance"][i].as<uf::stl::string>() );\
			}\
			for ( int i = 0; i < configVersionLevel["features"].size(); ++i ) {\
				uf::renderer::settings::requestedDeviceFeatures.emplace_back( configVersionLevel["features"][i].as<uf::stl::string>() );\
			}\
			for ( int i = 0; i < configVersionLevel["featureChain"].size(); ++i ) {\
				uf::stl::string key = configVersionLevel["featureChain"][i].as<uf::stl::string>();\
				uf::renderer::settings::requestedFeatureChain[key] = true;\
			}\
		}

		VK_LOAD_VERSION_LEVEL(1.0);
		VK_LOAD_VERSION_LEVEL(1.1);
		VK_LOAD_VERSION_LEVEL(1.2);
		VK_LOAD_VERSION_LEVEL(1.3);
		
		ext::vulkan::settings::defaultStageBuffers = configRenderInvariantJson["default stage buffers"].as( uf::renderer::settings::defaultStageBuffers );
		uf::renderer::settings::invariant::deviceAddressing = uf::renderer::settings::requestedFeatureChain["physicalDeviceVulkan12"].as<bool>(false) || uf::renderer::settings::requestedFeatureChain["bufferDeviceAddress"].as<bool>(false);
		uf::renderer::settings::experimental::batchQueueSubmissions = configRenderExperimentalJson["batch queue submissions"].as( uf::renderer::settings::experimental::batchQueueSubmissions );
		uf::renderer::settings::experimental::memoryBudgetBit = configRenderExperimentalJson["memory budget"].as( uf::renderer::settings::experimental::memoryBudgetBit );
	#if 1
		uf::renderer::settings::experimental::dedicatedThread = false;
		::requestDedicatedRenderThread = configRenderExperimentalJson["dedicated thread"].as( uf::renderer::settings::experimental::dedicatedThread );
	#else
		uf::renderer::settings::experimental::dedicatedThread = configRenderExperimentalJson["dedicated thread"].as( uf::renderer::settings::experimental::dedicatedThread );
	#endif
	#endif

		uf::renderer::settings::experimental::rebuildOnTickBegin = configRenderExperimentalJson["rebuild on tick begin"].as( uf::renderer::settings::experimental::rebuildOnTickBegin );

		uf::renderer::settings::invariant::multithreadedRecording = configRenderInvariantJson["multithreaded recording"].as( uf::renderer::settings::invariant::multithreadedRecording );
		uf::renderer::settings::invariant::waitOnRenderEnd = configRenderInvariantJson["wait on render end"].as( uf::renderer::settings::invariant::waitOnRenderEnd );
		uf::renderer::settings::invariant::individualPipelines = configRenderInvariantJson["individual pipelines"].as( uf::renderer::settings::invariant::individualPipelines );
		uf::renderer::settings::invariant::deferredMode = configRenderInvariantJson["deferred mode"].as( uf::renderer::settings::invariant::deferredMode );
	
		uf::renderer::settings::pipelines::vsync = configRenderPipelinesJson["vsync"].as( uf::renderer::settings::pipelines::vsync );
		uf::renderer::settings::pipelines::culling = configRenderPipelinesJson["culling"].as( uf::renderer::settings::pipelines::culling );

	#if UF_USE_VULKAN
		uf::renderer::settings::pipelines::deferred = configRenderPipelinesJson["deferred"].as( uf::renderer::settings::pipelines::deferred );
		uf::renderer::settings::pipelines::hdr = configRenderPipelinesJson["hdr"].as( uf::renderer::settings::pipelines::hdr );
		uf::renderer::settings::pipelines::vxgi = configRenderPipelinesJson["vxgi"].as( uf::renderer::settings::pipelines::vxgi );
		uf::renderer::settings::pipelines::bloom = configRenderPipelinesJson["bloom"].as( uf::renderer::settings::pipelines::bloom );
		uf::renderer::settings::pipelines::rt = configRenderPipelinesJson["rt"].as( uf::renderer::settings::pipelines::rt );
		uf::renderer::settings::pipelines::postProcess = configRenderPipelinesJson["postProcess"].as( uf::renderer::settings::pipelines::postProcess );
		uf::renderer::settings::pipelines::fsr = configRenderPipelinesJson["fsr"].as( uf::renderer::settings::pipelines::fsr );

	#if UF_USE_FFX_FSR
	//	ext::fsr::enabled = uf::renderer::settings::pipelines::fsr;
		ext::fsr::preset = ::json["engine"]["ext"]["fsr"]["preset"].as(ext::fsr::preset);
		ext::fsr::jitterScale = ::json["engine"]["ext"]["fsr"]["jitter scale"].as(ext::fsr::jitterScale);
		ext::fsr::sharpness = ::json["engine"]["ext"]["fsr"]["sharpness"].as(ext::fsr::sharpness);
	#endif
	
		if ( uf::renderer::settings::pipelines::rt ) {
		//	uf::renderer::settings::pipelines::vxgi = false;
		//	uf::renderer::settings::pipelines::culling = false;
			::json["engine"]["scenes"]["lights"]["shadows"]["enabled"] = false;
		}
	#define JSON_TO_FORMAT( key ) if ( configRenderJson["formats"][#key].is<uf::stl::string>() ) {\
			uf::stl::string format = configRenderJson["formats"][#key].as<uf::stl::string>();\
			format = uf::string::replace( uf::string::uppercase(format), " ", "_" );\
			uf::renderer::settings::formats::key = uf::renderer::formatFromString( format );\
		}

		JSON_TO_FORMAT(color);
		JSON_TO_FORMAT(depth);
	#endif
	}

	/* Init controllers */ {
		spec::controller::initialize();
	}

#if UF_USE_LUA
	/* Lua */ {
		auto& configLuaJson = ::json["engine"]["ext"]["lua"];

		ext::lua::enabled = configLuaJson["enabled"].as(ext::lua::enabled);
		ext::lua::main = configLuaJson["main"].as(ext::lua::main);
		ext::json::forEach( configLuaJson["modules"], []( const uf::stl::string& key, ext::json::Value& value ){
			ext::lua::modules[key] = value.as<uf::stl::string>();
		});
		ext::lua::initialize();
	}
#endif

	/* Physics */ {
		uf::physics::initialize();
	}

#if UF_USE_OPENVR
	{	
		auto& configVrJson = ::json["engine"]["ext"]["vr"];

		ext::openvr::enabled = configVrJson["enable"].as( ext::openvr::enabled );
		ext::openvr::swapEyes = configVrJson["swap eyes"].as( ext::openvr::swapEyes );


		if ( configVrJson["dominant eye"].is<int>() ) {
			ext::openvr::dominantEye = configVrJson["dominant eye"].as<int>();
		} else if ( configVrJson["dominant eye"].as<uf::stl::string>() == "left" ) ext::openvr::dominantEye = 0;
		else if ( configVrJson["dominant eye"].as<uf::stl::string>() == "right" ) ext::openvr::dominantEye = 1;

		ext::openvr::driver.manifest = configVrJson["manifest"].as(ext::openvr::driver.manifest);

		if ( ext::openvr::enabled ) ::json["engine"]["render modes"]["stereo deferred"] = true;
	}
#endif

	/* Initialize Vulkan */ {
		// setup render mode
		if ( ::json["engine"]["render modes"]["deferred"].as<bool>(true) ) {
			auto* renderMode = new uf::renderer::DeferredRenderMode;
			
			renderMode->blitter.descriptor.renderMode = "Swapchain";
			renderMode->blitter.descriptor.subpass = 0;
		#if UF_USE_FFX_FSR
			if ( uf::renderer::settings::pipelines::fsr ) {
				auto mode = uf::string::lowercase( ext::fsr::preset );
				if ( mode == "native" ) renderMode->scale = 1;
				else if ( mode == "quality" ) renderMode->scale = 1.0f / (1.5f);
				else if ( mode == "balanced" ) renderMode->scale = 1.0f / (1.7f);
				else if ( mode == "performance" ) renderMode->scale = 1.0f / (2.0f);
				else if ( mode == "ultra" ) renderMode->scale = 1.0f / (3.0f);
				else {
					renderMode->scale = 1;
					UF_MSG_WARNING("Invalid FFX FSR preset enum string specified: {}", mode);
				}
				UF_MSG_DEBUG("Using FFX FSR Preset: {} ({:.3f}% render scale)", mode, (1.0f / renderMode->scale));
			} else
		#endif
			renderMode->scale = ::json["engine"]["ext"]["vulkan"]["framebuffer"]["size"].as(1.0f);
			UF_MSG_DEBUG("Geometry render scale: {:.3f}", renderMode->scale);

			if ( ::json["engine"]["render modes"]["stereo deferred"].as<bool>() ) {
				renderMode->metadata.eyes = 2;
			}
		#if UF_USE_VULKAN
			if ( uf::renderer::settings::pipelines::deferred ) {
				renderMode->metadata.pipelines.emplace_back(uf::renderer::settings::pipelines::names::deferred);
			}
			if ( uf::renderer::settings::pipelines::culling ) {
				renderMode->metadata.pipelines.emplace_back(uf::renderer::settings::pipelines::names::culling);
			}
		#endif
			
			uf::renderer::addRenderMode( renderMode, "" );
		}

#if UF_USE_VULKAN
		/* Callbacks for 2KHR stuffs */ {
			uf::hooks.addHook("vulkan:Instance.ExtensionsEnabled", []( const ext::json::Value& json ) {
			//	UF_MSG_DEBUG("vulkan:Instance.ExtensionsEnabled: " << json);
			});
			uf::hooks.addHook("vulkan:Device.ExtensionsEnabled", []( const ext::json::Value& json ) {
			//	UF_MSG_DEBUG("vulkan:Device.ExtensionsEnabled: " << json);
			});
			uf::hooks.addHook("vulkan:Device.FeaturesEnabled", []( const ext::json::Value& json ) {
			//	UF_MSG_DEBUG("vulkan:Device.FeaturesEnabled: " << json);

				VkPhysicalDeviceFeatures2KHR deviceFeatures2{};
				VkPhysicalDeviceMultiviewFeaturesKHR extFeatures{};
				extFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES_KHR;
				deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
				deviceFeatures2.pNext = &extFeatures;
				PFN_vkGetPhysicalDeviceFeatures2KHR vkGetPhysicalDeviceFeatures2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2KHR>(vkGetInstanceProcAddr(uf::renderer::device.instance, "vkGetPhysicalDeviceFeatures2KHR"));
				vkGetPhysicalDeviceFeatures2KHR(uf::renderer::device.physicalDevice, &deviceFeatures2);
			//	UF_MSG_DEBUG("Multiview features:" );
			//	UF_MSG_DEBUG("\tmultiview = " << extFeatures.multiview );
			//	UF_MSG_DEBUG("\tmultiviewGeometryShader = " << extFeatures.multiviewGeometryShader );
			//	UF_MSG_DEBUG("\tmultiviewTessellationShader = " << extFeatures.multiviewTessellationShader );

				VkPhysicalDeviceProperties2KHR deviceProps2{};
				VkPhysicalDeviceMultiviewPropertiesKHR extProps{};
				extProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES_KHR;
				deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
				deviceProps2.pNext = &extProps;
				PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2KHR>(vkGetInstanceProcAddr(uf::renderer::device.instance, "vkGetPhysicalDeviceProperties2KHR"));
				vkGetPhysicalDeviceProperties2KHR(uf::renderer::device.physicalDevice, &deviceProps2);
			//	UF_MSG_DEBUG("Multiview properties:");
			//	UF_MSG_DEBUG("\tmaxMultiviewViewCount = " << extProps.maxMultiviewViewCount);
			//	UF_MSG_DEBUG("\tmaxMultiviewInstanceIndex = " << extProps.maxMultiviewInstanceIndex);
			});
		}
#endif
	}
#if UF_USE_OPENVR
	if ( ext::openvr::enabled ) {
		ext::openvr::initialize();
	
		uint32_t width, height;
		ext::openvr::recommendedResolution( width, height );

		auto& renderMode = uf::renderer::getRenderMode("Deferred", true);
		renderMode.width = width;
		renderMode.height = height;

		UF_MSG_DEBUG("Recommended VR Resolution: {}, {}", renderMode.width, renderMode.height); 

		if ( ::json["engine"]["ext"]["vr"]["scale"].is<float>() ) {
			float scale = ::json["engine"]["ext"]["vr"]["scale"].as<float>();
			renderMode.width *= scale;
			renderMode.height *= scale;
			
			UF_MSG_DEBUG("VR Resolution: {}, {}", renderMode.width, renderMode.height);
		}
	}
#endif

	/* Initialize Vulkan/OpenGL */ {
		uf::renderer::initialize();
	}

	pod::Thread& threadMain = uf::thread::get(uf::thread::mainThreadName);
#if UF_USE_DISCORD
	/* Discord */ if ( ::config.engine.ext.discord.enabled ) {
		ext::discord::initialize();
	}
#endif
#if UF_USE_ULTRALIGHT
	/* Ultralight-UX */ if ( ::config.engine.ext.ultralight.enabled ) {
		ext::ultralight::scale = ::json["engine"]["ext"]["ultralight"]["scale"].as( ext::ultralight::scale );
		ext::ultralight::log = ::json["engine"]["ext"]["ultralight"]["log"].as( ext::ultralight::log );
		ext::ultralight::initialize();
	}
#endif
#if UF_USE_IMGUI
	if ( ::config.engine.ext.imgui.enabled ) {
		ext::imgui::initialize();
	}
#endif
	/* Add hooks */ {
		uf::hooks.addHook( "game:Scene.Load", [&](ext::json::Value& json){
			auto function = [json]() -> int {
				uf::renderer::synchronize();

				uf::hooks.call("game:Scene.Cleanup");

				auto& scene = uf::scene::loadScene( json["scene"].as<uf::stl::string>() );
				auto& metadata = scene.getComponent<uf::Serializer>();
				metadata["system"]["config"] = ::json;

				return 0;
			};
			
			if ( json["immediate"].as<bool>() ) function();
			else uf::thread::queue( uf::thread::get(uf::thread::mainThreadName), function );
		});

		uf::hooks.addHook( "game:Scene.Cleanup", [&](){
			uf::scene::unloadScene();

			// reset physics world
			{
				uf::physics::terminate();
				uf::physics::initialize();
			}
			// reset graph state
			{
				// cleanup graphic handles
				for ( auto pair : uf::graph::storage.texture2Ds.map ) pair.second.destroy();
				for ( auto& t : uf::graph::storage.shadow2Ds ) t.destroy();
				for ( auto& t : uf::graph::storage.shadowCubes ) t.destroy();

				for ( auto pair : uf::graph::storage.atlases.map ) pair.second.clear();
				for ( auto pair : uf::graph::storage.images.map ) pair.second.clear();
				for ( auto pair : uf::graph::storage.meshes.map ) pair.second.destroy();

				// cleanup storage cache
				uf::graph::storage.instances.clear();
				uf::graph::storage.instanceAddresses.clear();
				uf::graph::storage.primitives.clear();
				uf::graph::storage.drawCommands.clear();
				uf::graph::storage.meshes.clear();
				uf::graph::storage.images.clear();
				uf::graph::storage.materials.clear();
				uf::graph::storage.textures.clear();
				uf::graph::storage.samplers.clear();
				uf::graph::storage.skins.clear();
				uf::graph::storage.animations.clear();
				uf::graph::storage.atlases.clear();
				uf::graph::storage.joints.clear();
				uf::graph::storage.texture2Ds.clear();
				uf::graph::storage.entities.clear();
				uf::graph::storage.shadow2Ds.clear();
				uf::graph::storage.shadowCubes.clear();
			}

			uf::renderer::states::rebuild = true;
		});
		uf::hooks.addHook( "system:Quit", [&](ext::json::Value& json){
			if ( json["message"].is<uf::stl::string>() ) {
				UF_MSG_DEBUG( "{}", json["message"].as<uf::stl::string>() );
			}
			ext::ready = false;
		});
	}

	/* Initialize root scene*/ {
		ext::json::Value payload;
		payload["scene"] = ::json["engine"]["scenes"]["start"];
		payload["immediate"] = true;
		uf::hooks.call("game:Scene.Load", payload);
	}
	
/*
	uf::thread::add( uf::thread::fetchWorker(), [&]{
		uf::asset::processQueue();
	});
*/
	
	ext::ready = true;
	UF_MSG_INFO("EXT took {} seconds to initialize", times.sys.elapsed().asDouble());

}

void EXT_API ext::tick() {
	/* Tick controllers */ {
		spec::controller::tick();
	}
#if UF_USE_OPENVR
	/* OpenVR */ if ( ext::openvr::context ) {
		ext::openvr::tick();
	}
#endif
	/* Print Memory Pool Information */  {
		TIMER(1, uf::inputs::kbm::states::P ) {
	    //	uf::iostream << uf::renderer::allocatorStats() << "\n";
			UF_MSG_DEBUG("==== Memory Pool Information ====");
			if ( uf::memoryPool::global.size() > 0 ) UF_MSG_DEBUG("Global Memory Pool: {}", uf::memoryPool::global.stats());
			if ( uf::Entity::memoryPool.size() > 0 ) UF_MSG_DEBUG("Entity Memory Pool: {}", uf::Entity::memoryPool.stats());
			if ( uf::component::memoryPool.size() > 0 ) UF_MSG_DEBUG("Components Memory Pool: {}", uf::component::memoryPool.stats());
			if ( uf::userdata::memoryPool.size() > 0 ) UF_MSG_DEBUG("Userdata Memory Pool: {}", uf::userdata::memoryPool.stats());
		}
	}
#if 0
	/* Attempt to reset VR position */  {
		TIMER(1, uf::inputs::kbm::states::Z ) {
	    	uf::hooks.call("VR:Seat.Reset");
		}
	}
	/* Print controller position */ if ( false ) {
		TIMER(1, uf::inputs::kbm::states::Z ) {
			auto& scene = uf::scene::getCurrentScene();
			auto& controller = scene.getController();
			auto& camera = controller.getComponent<uf::Camera>();
			auto& t = camera.getTransform(); //controller->getComponent<pod::Transform<>>();
			uf::iostream << "Viewport position: (" << t.position.x << ", " << t.position.y << ", " << t.position.z << ") (" << t.orientation.x << ", " << t.orientation.y << ", " << t.orientation.z << ", " << t.orientation.w << ")";
			uf::iostream << "\n";
			if ( false ) {
			//	uf::Entity* light = scene.findByUid(scene.loadChildUid("/light.json", true));
				uf::Object& light = scene.loadChild("/light.json", true);
				auto& lTransform = light.getComponent<pod::Transform<>>();
				auto& lMetadata = light.getComponent<uf::Serializer>();
				lTransform.position = t.position;
				lTransform.orientation = t.orientation;
				if ( !ext::json::isArray( lMetadata["light"] ) ) {
					lMetadata["light"]["color"][0] = (rand() % 100) / 100.0;
					lMetadata["light"]["color"][1] = (rand() % 100) / 100.0;
					lMetadata["light"]["color"][2] = (rand() % 100) / 100.0;
				}
				auto& sMetadata = scene.getComponent<uf::Serializer>();
				sMetadata["light"]["should"] = true;
			}
		}
	}
#endif
	/* Update physics timer */ {
		uf::physics::tick();
	}
	/* Update graph */ {
		uf::graph::tick();
	}
	/* Update entities */ {
		uf::scene::tick();
	}

	/* Tick Main Thread Queue */ {
		uf::thread::process( uf::thread::get(uf::thread::mainThreadName) );
	}
#if UF_USE_ULTRALIGHT
	/* Ultralight-UX */ if ( ::config.engine.ext.ultralight.enabled ) {
		ext::ultralight::tick();
	}
#endif
	/* Update vulkan */ {
		uf::renderer::tick();
	}
	//UF_TIMER_TRACE("ticking renderer");
#if UF_USE_DISCORD
	/* Discord */ if ( ::config.engine.ext.discord.enabled ) {
		ext::discord::tick();
	}
#endif
#if UF_USE_IMGUI
	if ( ::config.engine.ext.imgui.enabled ) {
		ext::imgui::tick();
	}
#endif
	/* FPS Print */ if ( ::config.engine.fps.print ) {
		++::times.frames;
		++::times.total.frames;
		TIMER( ::config.engine.fps.every ) {
			UF_MSG_DEBUG("System: {:.3f} ms/frame | Time: {:.3f} | Frames: {} | FPS: {:.3f}", (time * 1000.0/::times.frames), time, ::times.frames, ::times.frames / time);
		#if UF_ENV_DREAMCAST
			DC_STATS();
		#endif
			::times.frames = 0;
		}
	}
	{	
		TIMER( ::config.engine.gc.every ) {
			size_t collected = uf::instantiator::collect( ::config.engine.gc.mode );
			if ( collected > 0 ) {
				if ( ::config.engine.gc.announce ) UF_MSG_DEBUG("GC collected {} unused entities", (int) collected);
			//	uf::renderer::states::rebuild = true;
			}
		}
	}
#if !UF_ENV_DREAMCAST
	if ( ::times.limiter > 0 ) {
		static uf::Timer<long long> timer(false);
		if ( !timer.running() ) timer.start();
		auto elapsed = timer.elapsed().asMilliseconds();
		long long sleep = (::times.limiter * 1000) - elapsed;
		if ( sleep > 0 ) {
		 	if ( ::config.engine.limiter.print ) UF_MSG_DEBUG("Frame limiting: sleeping for {}ms (from {})", elapsed, ::times.limiter * 1000);
			std::this_thread::sleep_for(std::chrono::milliseconds(sleep));
		}
		timer.reset();
	}

	if ( ::requestDedicatedRenderThread && uf::scene::getCurrentScene().getController().getName() == "Player" ) {
		::requestDedicatedRenderThread = false;
		uf::renderer::settings::experimental::dedicatedThread = true;
	//	uf::renderer::settings::experimental::rebuildOnTickBegin = true;
		UF_MSG_DEBUG("Dedicated render requested");
	}
#endif
}
void EXT_API ext::render() {
	if ( uf::scene::scenes.empty() ) return;

#if UF_USE_ULTRALIGHT
	/* Ultralight-UX */ if ( ::config.engine.ext.ultralight.enabled ) {
		ext::ultralight::render();
	}
#endif
#if UF_USE_OPENVR
	if ( ext::openvr::context ) {
		vr::VRCompositor()->SubmitExplicitTimingData();
	}
#endif
	/* Render scene */ {
	//	uf::renderer::tick();
		uf::renderer::render();
	}
#if UF_USE_OPENVR
	if ( ext::openvr::context ) {
		ext::openvr::synchronize();
		ext::openvr::submit();
	}
#endif
}
void EXT_API ext::terminate() {
	/* Kill threads */ {
		uf::thread::terminate();
	}
	/* Terminate controllers */ {
		spec::controller::terminate();
	}
#if UF_USE_IMGUI
	if ( ::config.engine.ext.imgui.enabled ) {
		ext::imgui::terminate();
	}
#endif
#if UF_USE_ULTRALIGHT
	/* Ultralight-UX */ if ( ::config.engine.ext.ultralight.enabled ) {
		ext::ultralight::terminate();
	}
#endif
#if UF_USE_OPENVR
	/* OpenVR */ if ( ext::openvr::context ) {
		ext::openvr::terminate();
	}
#endif
	{
		uf::hooks.removeHooks();
	}
#if UF_USE_LUA
	{
		ext::lua::terminate();
	}
#endif
	{
		uf::graph::destroy();
	}
	{
		uf::scene::destroy();
	}
	/* Kill physics */ {
		uf::physics::terminate();
	}
	/* Garbage collection */ if ( false ) { // segfaults, for some reason
		size_t collected = uf::instantiator::collect( ::config.engine.gc.mode );
		if ( ::config.engine.gc.announce && collected > 0 ) UF_MSG_DEBUG("GC collected {} unused entities", (int) collected);
	}

	/* Close vulkan */ {
		uf::renderer::destroy();
	}

	#if UF_USE_OPENAL
		/* Initialize OpenAL */ {
			ext::al::destroy();
		}
	#endif

	/* Print system stats */ {
		::times.total.time = times.sys.elapsed().asDouble();
		UF_MSG_DEBUG("System: Total Time: {} | Total Frames: {} | Average FPS: {}", ::times.total.time, ::times.total.frames, ::times.total.frames / ::times.total.time);
	}

	/* Flush input buffer */ {
		io.output << io.input << "\n";
		for ( const auto& str : uf::iostream.getHistory() ) io.output << str << "\n";
		io.output << "\nTerminated after " << times.sys.elapsed().asDouble() << " seconds" << "\n";
		io.output.close();
	}

	/* Write persistent data */ if ( false ) {
		struct {
			bool exists = false;
			uf::stl::string filename = uf::io::root+"/persistent.json";
		} file;
		struct {
			uf::Serializer file;
		} config;
		/* Read from file */  {
			file.exists = config.file.readFromFile(file.filename);
		}
		/* Write persistent data */ {
			config.file.writeToFile(file.filename);
		}
	}
}