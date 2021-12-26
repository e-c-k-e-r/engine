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

	uf::Serializer& config = ext::config;
}

void EXT_API ext::load() {
	ext::config = uf::io::readAsString(uf::io::root+"/config.json");
}
void EXT_API ext::initialize() {

	/* Arguments */ {
		bool modified = false;
		auto& arguments = ::config["arguments"];
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
					::config.path(keyString) = value;
					modified = true;
				}
			}
		}
		UF_MSG_DEBUG("Arguments: " << uf::Serializer(arguments));
		if ( modified ) UF_MSG_DEBUG("New config: " << ::config);
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
	/* Testing scope */ {
	
	}

	/* Set memory pool sizes */ {
		auto& configMemoryPoolJson = ::config["engine"]["memory pool"];

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
		UF_MSG_DEBUG("Requesting " << (size_t) size << " bytes for global memory pool: " << &uf::memoryPool::global);
		uf::memoryPool::global.initialize( size );
		uf::memoryPool::subPool = configMemoryPoolJson["subPools"].as( uf::memoryPool::subPool );
		if ( size <= 0 || uf::memoryPool::subPool ) {
			{
				size_t size = deduceSize( configMemoryPoolJson["pools"]["component"] );
				UF_MSG_DEBUG("Requesting " << (int) size << " bytes for component memory pool: " << &uf::component::memoryPool);
				uf::component::memoryPool.initialize( size );
			}
			{
				size_t size = deduceSize( configMemoryPoolJson["pools"]["userdata"] );
				UF_MSG_DEBUG("Requesting " << (int) size << " bytes for userdata memory pool: " << &uf::userdata::memoryPool);
				uf::userdata::memoryPool.initialize( size );
			}
			{
				size_t size = deduceSize( configMemoryPoolJson["pools"]["entity"] );
				UF_MSG_DEBUG("Requesting " << (int) size << " bytes for entity memory pool: " << &uf::Entity::memoryPool);
				uf::Entity::memoryPool.initialize( size );
			}
		}
		uf::allocator::override = configMemoryPoolJson["override"].as( uf::allocator::override );
	}
	{
		uf::Mesh::defaultInterleaved = ::config["engine"]["scenes"]["meshes"]["interleaved"].as( uf::Mesh::defaultInterleaved );
	#if 0 && UF_USE_OPENGL
		uf::matrix::reverseInfiniteProjection = false;
	#else
		uf::matrix::reverseInfiniteProjection = ::config["engine"]["scenes"]["matrix"]["reverseInfinite"].as( uf::matrix::reverseInfiniteProjection );
	#endif
	}

	/* Create initial scene (kludge) */ {
		uf::Scene& scene = uf::instantiator::instantiate<uf::Scene>(); //new uf::Scene;
		uf::scene::scenes.emplace_back(&scene);
		auto& metadata = scene.getComponent<uf::Serializer>();
		metadata["system"]["config"] = ::config;
	}

	{
		uf::Entity::deleteChildrenOnDestroy = ::config["engine"]["debug"]["entity"]["delete children on destroy"].as( uf::Entity::deleteChildrenOnDestroy );
		uf::Entity::deleteComponentsOnDestroy = ::config["engine"]["debug"]["entity"]["delete components on destroy"].as( uf::Entity::deleteComponentsOnDestroy );
	}

	{
		auto& configEngineLimitersJson = ::config["engine"]["limiters"];

		/* Frame limiter */ {
			float limit = configEngineLimitersJson["framerate"].as<float>();
			::times.limiter = limit != 0 ? 1.0 / limit : 0;
		}
		/* Max delta time */{
			float limit = configEngineLimitersJson["deltaTime"].as<float>();
			uf::physics::time::clamp = limit != 0 ? 1.0 / limit : 0;
		}
		
		if ( configEngineLimitersJson["framerate"].as<uf::stl::string>() == "auto" && ::config["window"]["refresh rate"].is<size_t>() ) {
			float scale = 1.0;
			size_t refreshRate = ::config["window"]["refresh rate"].as<size_t>();
			configEngineLimitersJson["framerate"] = refreshRate * scale;
			UF_MSG_DEBUG("Setting framerate cap to " << (int) refreshRate * scale);
		}
	}
	{
		auto& configEngineThreadJson = ::config["engine"]["threads"];

		if ( configEngineThreadJson["frame limiter"].as<uf::stl::string>() == "auto" && ::config["window"]["refresh rate"].is<size_t>() ) {
			float scale = 2.0;
			size_t refreshRate = ::config["window"]["refresh rate"].as<size_t>();
			configEngineThreadJson["frame limiter"] = refreshRate * scale;
			UF_MSG_DEBUG("Setting thread frame limiter to " << (int) refreshRate * scale);
		}
		/* Thread frame limiter */ {
			float limit = configEngineThreadJson["frame limiter"].as<float>();
			uf::thread::limiter = limit != 0 ? 1.0 / limit : 0;
		}

		// Set worker threads
		if ( configEngineThreadJson["workers"].as<uf::stl::string>() == "async" ) {
			uf::thread::async = true;
			auto threads = std::max( 1, (int) std::thread::hardware_concurrency() - 1 );
			configEngineThreadJson["workers"] = threads;
			uf::thread::workers = configEngineThreadJson["workers"].as<size_t>();
			UF_MSG_DEBUG("Using async worker threads");
		} else if ( configEngineThreadJson["workers"].as<uf::stl::string>() == "auto" ) {
			auto threads = std::max( 1, (int) std::thread::hardware_concurrency() - 1 );
			configEngineThreadJson["workers"] = threads;
			uf::thread::workers = configEngineThreadJson["workers"].as<size_t>();
			UF_MSG_DEBUG("Using " << threads << " worker threads");
		} else if ( configEngineThreadJson["workers"].is<size_t>() ) {
			auto threads = configEngineThreadJson["workers"].as<size_t>();
			uf::thread::workers = threads;
			UF_MSG_DEBUG("Using " << threads << " worker threads");
		}
	}
	// Audio settings
	{	
		auto& configEngineAudioJson = ::config["engine"]["audio"];

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
		uf::audio::volumes::bgm = configEngineAudioJson["volumes"]["bgm"].as(uf::audio::volumes::bgm);
		uf::audio::volumes::sfx = configEngineAudioJson["volumes"]["sfx"].as(uf::audio::volumes::sfx);
		uf::audio::volumes::voice = configEngineAudioJson["volumes"]["voice"].as(uf::audio::volumes::voice);
	#endif
	}

#if UF_USE_OPENAL
	/* Initialize OpenAL */ {
		ext::al::initialize();
	}
#endif

	// set bullet parameters
#if UF_USE_BULLET
	{
		auto& configEngineBulletJson = ::config["engine"]["ext"]["bullet"];

		ext::bullet::iterations = configEngineBulletJson["iterations"].as( ext::bullet::iterations );
		ext::bullet::substeps = configEngineBulletJson["substeps"].as( ext::bullet::substeps );
		ext::bullet::timescale = configEngineBulletJson["timescale"].as( ext::bullet::timescale );
		ext::bullet::defaultMaxCollisionAlgorithmPoolSize = configEngineBulletJson["pool size"]["max collision algorithm"].as( ext::bullet::defaultMaxCollisionAlgorithmPoolSize );
		ext::bullet::defaultMaxPersistentManifoldPoolSize = configEngineBulletJson["pool size"]["max persistent manifold"].as( ext::bullet::defaultMaxPersistentManifoldPoolSize );
		
		ext::bullet::debugDrawEnabled = configEngineBulletJson["debug draw"]["enabled"].as( ext::bullet::debugDrawEnabled );
		ext::bullet::debugDrawRate = configEngineBulletJson["debug draw"]["rate"].as( ext::bullet::debugDrawRate );
		ext::bullet::debugDrawLayer = configEngineBulletJson["debug draw"]["layer"].as( ext::bullet::debugDrawLayer );
		ext::bullet::debugDrawLineWidth = configEngineBulletJson["debug draw"]["line width"].as( ext::bullet::debugDrawLineWidth );
	}
#elif UF_USE_REACTPHYSICS
	{
		auto& configEngineReactJson = ::config["engine"]["ext"]["reactphysics"];

		ext::reactphysics::timescale = configEngineReactJson["timescale"].as( ext::reactphysics::timescale );
		ext::reactphysics::debugDrawEnabled = configEngineReactJson["debug draw"]["enabled"].as( ext::reactphysics::debugDrawEnabled );
		ext::reactphysics::debugDrawRate = configEngineReactJson["debug draw"]["rate"].as( ext::reactphysics::debugDrawRate );
		ext::reactphysics::debugDrawLayer = configEngineReactJson["debug draw"]["layer"].as( ext::reactphysics::debugDrawLayer );
		ext::reactphysics::debugDrawLineWidth = configEngineReactJson["debug draw"]["line width"].as( ext::reactphysics::debugDrawLineWidth );
	}
#endif

	// renderer settings
	{
	#if UF_USE_VULKAN
		auto& configRenderJson = ::config["engine"]["ext"]["vulkan"];
	#elif UF_USE_OPENGL
		auto& configRenderJson = ::config["engine"]["ext"]["opengl"];
	#else
		auto& configRenderJson = ::config["engine"]["ext"]["software"];
	#endif
		auto& configRenderExperimentalJson = configRenderJson["experimental"];

		uf::renderer::settings::validation = configRenderJson["validation"]["enabled"].as( uf::renderer::settings::validation );
		uf::renderer::settings::msaa = configRenderJson["framebuffer"]["msaa"].as( uf::renderer::settings::msaa );

		if ( configRenderJson["framebuffer"]["size"].is<float>() ) {
			float scale = configRenderJson["framebuffer"]["size"].as(1.0f);
			uf::renderer::settings::width *= scale;
			uf::renderer::settings::height *= scale;
		} else if ( ext::json::isArray( configRenderJson["framebuffer"]["size"] ) ) {
			uf::renderer::settings::width = configRenderJson["framebuffer"]["size"][0].as<float>();
			uf::renderer::settings::height = configRenderJson["framebuffer"]["size"][1].as<float>();
			uf::stl::string filter =  uf::string::lowercase( configRenderJson["framebuffer"]["size"][2].as<uf::stl::string>() );

			if ( filter == "nearest" ) uf::renderer::settings::swapchainUpscaleFilter = uf::renderer::enums::Filter::NEAREST;
			else if ( filter == "linear" ) uf::renderer::settings::swapchainUpscaleFilter = uf::renderer::enums::Filter::LINEAR;
		}
		
		for ( int i = 0; i < configRenderJson["validation"]["filters"].size(); ++i ) {
			uf::renderer::settings::validationFilters.emplace_back( configRenderJson["validation"]["filters"][i].as<uf::stl::string>() );
		}
		for ( int i = 0; i < configRenderJson["extensions"]["device"].size(); ++i ) {
			uf::renderer::settings::requestedDeviceExtensions.emplace_back( configRenderJson["extensions"]["device"][i].as<uf::stl::string>() );
		}
		for ( int i = 0; i < configRenderJson["extensions"]["instance"].size(); ++i ) {
			uf::renderer::settings::requestedInstanceExtensions.emplace_back( configRenderJson["extensions"]["instance"][i].as<uf::stl::string>() );
		}
		for ( int i = 0; i < configRenderJson["features"].size(); ++i ) {
			uf::renderer::settings::requestedDeviceFeatures.emplace_back( configRenderJson["features"][i].as<uf::stl::string>() );
		}

		uf::renderer::settings::experimental::rebuildOnTickBegin = configRenderExperimentalJson["rebuild on tick begin"].as( uf::renderer::settings::experimental::rebuildOnTickBegin );
		uf::renderer::settings::experimental::waitOnRenderEnd = configRenderExperimentalJson["wait on render end"].as( uf::renderer::settings::experimental::waitOnRenderEnd );
		uf::renderer::settings::experimental::individualPipelines = configRenderExperimentalJson["individual pipelines"].as( uf::renderer::settings::experimental::individualPipelines );
		uf::renderer::settings::experimental::multithreadedCommandRecording = configRenderExperimentalJson["multithreaded command recording"].as( uf::renderer::settings::experimental::multithreadedCommandRecording );
		uf::renderer::settings::experimental::multithreadedCommandRendering = configRenderExperimentalJson["multithreaded command rendering"].as( uf::renderer::settings::experimental::multithreadedCommandRendering );
		uf::renderer::settings::experimental::deferredMode = configRenderExperimentalJson["deferred mode"].as( uf::renderer::settings::experimental::deferredMode );
		uf::renderer::settings::experimental::deferredReconstructPosition = configRenderExperimentalJson["deferred reconstruct position"].as( uf::renderer::settings::experimental::deferredReconstructPosition );
		uf::renderer::settings::experimental::deferredAliasOutputToSwapchain = configRenderExperimentalJson["deferred alias output to swapchain"].as( uf::renderer::settings::experimental::deferredAliasOutputToSwapchain );
		uf::renderer::settings::experimental::vsync = configRenderExperimentalJson["vsync"].as( uf::renderer::settings::experimental::vsync );
		uf::renderer::settings::experimental::hdr = configRenderExperimentalJson["hdr"].as( uf::renderer::settings::experimental::hdr );
		uf::renderer::settings::experimental::vxgi = configRenderExperimentalJson["vxgi"].as( uf::renderer::settings::experimental::vxgi );
		uf::renderer::settings::experimental::deferredSampling = configRenderExperimentalJson["deferred sampling"].as( uf::renderer::settings::experimental::deferredSampling );
		uf::renderer::settings::experimental::culling = configRenderExperimentalJson["culling"].as( uf::renderer::settings::experimental::culling );
		uf::renderer::settings::experimental::bloom = configRenderExperimentalJson["bloom"].as( uf::renderer::settings::experimental::bloom );
	
	#define JSON_TO_VKFORMAT( key ) if ( configRenderJson["formats"][#key].is<uf::stl::string>() ) {\
			uf::stl::string format = configRenderJson["formats"][#key].as<uf::stl::string>();\
			format = uf::string::replace( uf::string::uppercase(format), " ", "_" );\
			uf::renderer::settings::formats::key = uf::renderer::formatFromString( format );\
		}

		JSON_TO_VKFORMAT(color);
		JSON_TO_VKFORMAT(depth);
		JSON_TO_VKFORMAT(normal);
		JSON_TO_VKFORMAT(position);
	}

	/* Init controllers */ {
		spec::controller::initialize();
	}

#if UF_USE_LUA
	/* Lua */ {
		auto& configLuaJson = ::config["engine"]["ext"]["lua"];

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
		auto& configVrJson = ::config["engine"]["ext"]["vr"];

		ext::openvr::enabled = configVrJson["enable"].as( ext::openvr::enabled );
		ext::openvr::swapEyes = configVrJson["swap eyes"].as( ext::openvr::swapEyes );


		if ( configVrJson["dominant eye"].is<int>() ) {
			ext::openvr::dominantEye = configVrJson["dominant eye"].as<int>();
		} else if ( configVrJson["dominant eye"].as<uf::stl::string>() == "left" ) ext::openvr::dominantEye = 0;
		else if ( configVrJson["dominant eye"].as<uf::stl::string>() == "right" ) ext::openvr::dominantEye = 1;

		ext::openvr::driver.manifest = configVrJson["manifest"].as<uf::stl::string>();

		if ( ext::openvr::enabled ) ::config["engine"]["render modes"]["stereo deferred"] = true;
	}
#endif

	/* Initialize Vulkan */ {
	#if UF_USE_VULKAN
		// setup render mode
		if ( ::config["engine"]["render modes"]["gui"].as<bool>(true) ) {
			auto* renderMode = new uf::renderer::RenderTargetRenderMode;
			uf::renderer::addRenderMode( renderMode, "Gui" );
			renderMode->blitter.descriptor.subpass = 1;
		}
		if ( ::config["engine"]["render modes"]["deferred"].as<bool>(true) ) {
			uf::renderer::addRenderMode( new uf::renderer::DeferredRenderMode, "" );
			auto& renderMode = uf::renderer::getRenderMode("Deferred", true);
			if ( ::config["engine"]["render modes"]["stereo deferred"].as<bool>() ) {
				renderMode.metadata.eyes = 2;
			}
			if ( uf::renderer::settings::experimental::culling ) {
				renderMode.metadata.pipelines.emplace_back("culling");
			}
		}
	#endif

	#if UF_USE_OPENVR
		if ( ext::openvr::enabled ) {
			ext::openvr::initialize();
		
			uint32_t width, height;
			ext::openvr::recommendedResolution( width, height );

			auto& renderMode = uf::renderer::getRenderMode("Deferred", true);
			renderMode.width = width;
			renderMode.height = height;

			UF_MSG_DEBUG("Recommended VR Resolution: " << renderMode.width << ", " << renderMode.height); 

			if ( ::config["engine"]["ext"]["vr"]["scale"].is<float>() ) {
				float scale = ::config["engine"]["ext"]["vr"]["scale"].as<float>();
				renderMode.width *= scale;
				renderMode.height *= scale;
				
				UF_MSG_DEBUG("VR Resolution: " << renderMode.width << ", " << renderMode.height);
			}
		}
	#endif
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

	{
		uf::renderer::initialize();
	}

	pod::Thread& threadMain = uf::thread::has("Main") ? uf::thread::get("Main") : uf::thread::create( "Main", false );
#if UF_USE_DISCORD
	/* Discord */ if ( ::config["engine"]["ext"]["discord"]["enabled"].as<bool>() ) {
		ext::discord::initialize();
	}
#endif
#if UF_USE_ULTRALIGHT
	/* Ultralight-UX */ if ( ::config["engine"]["ext"]["ultralight"]["enabled"].as<bool>() ) {
		ext::ultralight::scale = ::config["engine"]["ext"]["ultralight"]["scale"].as( ext::ultralight::scale );
		ext::ultralight::log = ::config["engine"]["ext"]["ultralight"]["log"].as( ext::ultralight::log );
		ext::ultralight::initialize();
	}
#endif
	/* Add hooks */ {
		uf::hooks.addHook( "game:Scene.Load", [&](ext::json::Value& json){
			auto function = [json]() -> int {
				uf::renderer::synchronize();

				uf::hooks.call("game:Scene.Cleanup");
				// uf::scene::unloadScene();

				auto& scene = uf::scene::loadScene( json["scene"].as<uf::stl::string>() );
				auto& metadata = scene.getComponent<uf::Serializer>();
				metadata["system"]["config"] = ::config;

				return 0;
			};
			
			if ( json["immediate"].as<bool>() ) function(); else uf::thread::add( uf::thread::get("Main"), function, true );
		});

		uf::hooks.addHook( "game:Scene.Cleanup", [&](ext::json::Value& json){
			uf::scene::unloadScene();
		});
		uf::hooks.addHook( "system:Quit", [&](ext::json::Value& json){
			ext::ready = false;
		});
	}

	/* Initialize root scene*/ {
		uf::Serializer payload;
		payload["scene"] = ::config["engine"]["scenes"]["start"];
		payload["immediate"] = true;
		uf::hooks.call("game:Scene.Load", payload);
	}
	
/*
	uf::thread::add( uf::thread::fetchWorker(), [&]() -> int {
		auto& scene = uf::scene::getCurrentScene();
		auto& assetLoader = scene.getComponent<uf::Asset>();
		assetLoader.processQueue();
	return 0;}, false );
*/
	
	ext::ready = true;
	UF_MSG_INFO("EXT took " << times.sys.elapsed().asDouble() << " seconds to initialize!");

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
		TIMER(1, uf::inputs::kbm::states::P && ) {
	    //	uf::iostream << uf::renderer::allocatorStats() << "\n";
			UF_MSG_DEBUG("==== Memory Pool Information ====");
			if ( uf::memoryPool::global.size() > 0 ) UF_MSG_DEBUG("Global Memory Pool: " << uf::memoryPool::global.stats());
			if ( uf::Entity::memoryPool.size() > 0 ) UF_MSG_DEBUG("Entity Memory Pool: " << uf::Entity::memoryPool.stats());
			if ( uf::component::memoryPool.size() > 0 ) UF_MSG_DEBUG("Components Memory Pool: " << uf::component::memoryPool.stats());
			if ( uf::userdata::memoryPool.size() > 0 ) UF_MSG_DEBUG("Userdata Memory Pool: " << uf::userdata::memoryPool.stats());
		}
	}
#if 0
	/* Attempt to reset VR position */  {
		TIMER(1, uf::inputs::kbm::states::Z && ) {
	    	uf::hooks.call("VR:Seat.Reset");
		}
	}
	/* Print controller position */ if ( false ) {
		TIMER(1, uf::inputs::kbm::states::Z && ) {
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
		uf::thread::process( uf::thread::get("Main") );
	}
#if UF_USE_ULTRALIGHT
	/* Ultralight-UX */ if ( ::config["engine"]["ext"]["ultralight"]["enabled"].as<bool>() ) {
		ext::ultralight::tick();
	}
#endif
	/* Update vulkan */ {
		uf::renderer::tick();
	}
	//UF_TIMER_TRACE("ticking renderer");
#if UF_USE_DISCORD
	/* Discord */ if ( ::config["engine"]["ext"]["discord"]["enable"].as<bool>() ) {
		ext::discord::tick();
	}
#endif
	{
		auto& fps = ::config["engine"]["debug"]["framerate"];
		/* FPS Print */ if ( fps["print"].as<bool>() ) {
			++::times.frames;
			++::times.total.frames;
			float every = fps["every"].as<float>();
			TIMER( every ) {
				UF_MSG_DEBUG("System: " << (every * 1000.0/::times.frames) << " ms/frame | Time: " << time << " | Frames: " << ::times.frames << " | FPS: " << ::times.frames / time);
				::times.frames = 0;
			}
		}
	}
	{
		auto& gc = ::config["engine"]["debug"]["garbage collection"];
		/* Garbage collection */ if ( gc["enabled"].as<bool>() ) {
			float every = gc["every"].as<float>();
			uint8_t mode = gc["mode"].as<uint64_t>();
			bool announce = gc["announce"].as<bool>();
			TIMER( every ) {
				size_t collected = uf::instantiator::collect( mode );
				if ( collected > 0 ) {
					if ( announce ) UF_MSG_DEBUG("GC collected " << (int) collected << " unused entities");
				//	uf::renderer::states::rebuild = true;
				}
			}
		}
	}
#if !UF_ENV_DREAMCAST
	/* Frame limiter of sorts I guess */ if ( ::times.limiter > 0 ) {
		static uf::Timer<long long> timer(false);
		if ( !timer.running() ) timer.start();
		auto elapsed = timer.elapsed().asMilliseconds();
		long long sleep = (::times.limiter * 1000) - elapsed;
		if ( sleep > 0 ) {
		// 	if ( ::config["engine"]["debug"]["framerate"]["print"].as<bool>() ) UF_MSG_DEBUG("Frame limiting: " << elapsed << "ms exceeds limit, sleeping for " << elapsed << "ms");
			std::this_thread::sleep_for(std::chrono::milliseconds(sleep));
		}
		timer.reset();
	}
#endif
}
void EXT_API ext::render() {
#if UF_USE_ULTRALIGHT
	/* Ultralight-UX */ if ( ::config["engine"]["ext"]["ultralight"]["enabled"].as<bool>() ) {
		ext::ultralight::render();
	}
#endif
#if UF_USE_OPENVR
	if ( ext::openvr::context ) {
		vr::VRCompositor()->SubmitExplicitTimingData();
	}
#endif
	/* Render scene */ {
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
	/* Kill physics */ {
		uf::physics::terminate();
	}
#if UF_USE_ULTRALIGHT
	/* Ultralight-UX */ if ( ::config["engine"]["ext"]["ultralight"]["enabled"].as<bool>() ) {
		ext::ultralight::terminate();
	}
#endif
#if UF_USE_OPENVR
	/* OpenVR */ if ( ext::openvr::context ) {
		ext::openvr::terminate();
	}
#endif
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

	/* Garbage collection */ if ( false ) {
		uint8_t mode = ::config["engine"]["debug"]["garbage collection"]["mode"].as<uint64_t>();
		bool announce = ::config["engine"]["debug"]["garbage collection"]["announce"].as<bool>();
		size_t collected = uf::instantiator::collect( mode );
		if ( announce && collected > 0 ) UF_MSG_DEBUG("GC collected " << (int) collected << " unused entities");
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
		UF_MSG_DEBUG("System: Total Time: " << ::times.total.time << " | Total Frames: " << ::times.total.frames << " | Average FPS: " << ::times.total.frames / ::times.total.time);
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