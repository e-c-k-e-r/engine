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

#include <sys/stat.h>

#include <string>
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
#include <uf/ext/bullet/bullet.h>

bool ext::ready = false;
std::vector<std::string> ext::arguments;
uf::Serializer ext::config;

namespace {
	struct {
		uf::String input;
		std::ofstream output;

		struct {
			std::string output = uf::io::root+"/logs/output.txt";
		} filenames;
	} io;

	struct {
		spec::Time::time_t 	epoch;
		uf::Timer<> 		sys = uf::Timer<>(false);
		uf::Timer<> 		delta = uf::Timer<>(false);
		double prevTime = 0;
		double curTime = 0;
		double deltaTime = 0;
		size_t frames = 0;
		double limiter = 1.0 / 144.0;
	} times;

	uf::Serializer& config = ext::config;
}

namespace {
	std::string getConfig() {
	#if 1
		return uf::io::readAsString(uf::io::root+"/config.json");
	#else
		struct {
			bool initialized = false;
			uf::Serializer file;
			uf::Serializer fallback;
			uf::Serializer merged;
		} static config;
		if ( config.initialized ) return config.merged;

		struct {
			bool exists = false;
			std::string filename = uf::io::root+"/config.json";
		} file;
		/* Read from file */  {
			file.exists = config.file.readFromFile(file.filename);
		}
		/* Initialize default configuration */ {
			config.fallback["window"]["terminal"]["ncurses"] 			= true;
			config.fallback["window"]["terminal"]["visible"] 			= true;
			config.fallback["window"]["title"] 							= "Grimgram";
			config.fallback["window"]["icon"] 							= "";
			config.fallback["window"]["size"]["x"] 						= 0;
			config.fallback["window"]["size"]["y"] 						= 0;
			config.fallback["window"]["visible"] 						= true;
		//	config.fallback["window"]["fullscreen"] 					= false;
			config.fallback["window"]["mode"] 							= "windowed";
			config.fallback["window"]["cursor"]["visible"] 				= true;
			config.fallback["window"]["cursor"]["center"] 				= false;
			config.fallback["window"]["keyboard"]["repeat"] 			= true;
			
			config.fallback["engine"]["scenes"]["start"]				= "StartMenu";
			config.fallback["engine"]["scenes"]["lights"]["max"] 		= 32;
			config.fallback["engine"]["hook"]["mode"] 					= "Readable";
			config.fallback["engine"]["limiters"]["framerate"] 			= 60;
			config.fallback["engine"]["limiters"]["deltaTime"] 			= 120;
			config.fallback["engine"]["threads"]["workers"] 			= "auto";
			config.fallback["engine"]["threads"]["frame limiter"] 		= 144;
		#if UF_ENV_DREAMCAST
			config.fallback["engine"]["memory pool"]["size"] 			= "1 MiB";
		#else
			config.fallback["engine"]["memory pool"]["size"] 			= "512 MiB";
		#endif
			config.fallback["engine"]["memory pool"]["globalOverride"] 	= false;
			config.fallback["engine"]["memory pool"]["subPools"] 		= true;
		}
		/* Merge */ if ( file.exists ){
			config.merged = config.file;
			config.merged.merge( config.fallback, true );
		} else {
			config.merged = config.fallback;
		}
		/* Write default to file */ if ( false ) {
			config.merged.writeToFile(file.filename);
		}

		config.initialized = true;
		return config.merged;
	#endif
	}
}

void EXT_API ext::load() {
	ext::config = getConfig();
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
					std::string keyString = match[1].str();
					std::string valueString = match[2].str();
					uf::Serializer value; value.deserialize(valueString);
					::config.path(keyString) = value;
					modified = true;
				}
			}
		}
		uf::iostream << "Arguments: " << uf::Serializer(arguments) << "\n";
		if ( modified ) uf::iostream << "New config: " << ::config << "\n";
	}
	/* Seed */ {
		srand(time(NULL));
	}
	/* Open output file */ {
		io.output.open(io.filenames.output);
	}
	/* Initialize timers */ {
		times.epoch = spec::time.getTime();
		times.sys.start();
		times.delta.start();

		times.prevTime = times.sys.elapsed().asDouble();
		times.curTime = times.sys.elapsed().asDouble();
	}
	/* Read persistent data */ {
		// #include "./inits/persistence.inl"
	}
	/* Testing scope */ {
	
	}

	/* Parse config */ {
		/* Set memory pool sizes */ {
			auto deduceSize = []( const ext::json::Value& value )->size_t{
				if ( value.is<size_t>() ) return value.as<size_t>();
				if ( value.is<std::string>() ) {
					std::string str = value.as<std::string>();
					std::regex regex("^(\\d+) ?((?:K|M|G)?(?:i?B)?)$");
					std::smatch match;
					if ( std::regex_search( str, match, regex ) ) {
						size_t requested = std::stoi( match[1].str() );
						std::string prefix = match[2].str();
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
			{
				size_t size = deduceSize( ::config["engine"]["memory pool"]["size"] );
				uf::MemoryPool::globalOverride = ::config["engine"]["memory pool"]["globalOverride"].as<bool>();
				uf::iostream << "Requesting " << (size_t) size << " bytes for global memory pool: " << &uf::MemoryPool::global << "\n";
				uf::MemoryPool::global.initialize( size );
				uf::MemoryPool::subPool = ::config["engine"]["memory pool"]["subPools"].as<bool>();
				if ( size <= 0 || uf::MemoryPool::subPool ) {
					{
						size_t size = deduceSize( ::config["engine"]["memory pool"]["pools"]["component"] );
						uf::iostream << "Requesting " << (int) size << " bytes for component memory pool: " << &uf::component::memoryPool << "\n";
						uf::component::memoryPool.initialize( size );
					}
					{
						size_t size = deduceSize( ::config["engine"]["memory pool"]["pools"]["userdata"] );
						uf::iostream << "Requesting " << (int) size << " bytes for userdata memory pool: " << &uf::userdata::memoryPool << "\n";
						uf::userdata::memoryPool.initialize( size );
					}
					{
						size_t size = deduceSize( ::config["engine"]["memory pool"]["pools"]["entity"] );
						uf::iostream << "Requesting " << (int) size << " bytes for entity memory pool: " << &uf::Entity::memoryPool << "\n";
						uf::Entity::memoryPool.initialize( size );
					}
				}
			}
		}

		if ( ::config["engine"]["limiters"]["framerate"].as<std::string>() == "auto" && ::config["window"]["refresh rate"].is<size_t>() ) {
			double scale = 1.0;
			size_t refreshRate = ::config["window"]["refresh rate"].as<size_t>();
			::config["engine"]["limiters"]["framerate"] = refreshRate * scale;
			uf::iostream << "Setting framerate cap to " << (int) refreshRate * scale << "\n";
		}
		if ( ::config["engine"]["threads"]["frame limiter"].as<std::string>() == "auto" && ::config["window"]["refresh rate"].is<size_t>() ) {
			double scale = 2.0;
			size_t refreshRate = ::config["window"]["refresh rate"].as<size_t>();
			::config["engine"]["threads"]["frame limiter"] = refreshRate * scale;
			uf::iostream << "Setting thread frame limiter to " << (int) refreshRate * scale << "\n";
		}
		/* Frame limiter */ {
			double limit = ::config["engine"]["limiters"]["framerate"].as<double>();
			if ( limit != 0 )
				::times.limiter = 1.0 / ::config["engine"]["limiters"]["framerate"].as<double>();
			else ::times.limiter = 0;
		}
		/* Thread frame limiter */ {
			double limit = ::config["engine"]["threads"]["frame limiter"].as<double>();
			if ( limit != 0 ) 
				uf::thread::limiter = 1.0 / ::config["engine"]["threads"]["frame limiter"].as<double>();
			else
				uf::thread::limiter = 0;
		}
		/* Max delta time */{
			double limit = ::config["engine"]["limiters"]["deltaTime"].as<double>();
			if ( limit != 0 )
				uf::physics::time::clamp = 1.0 / ::config["engine"]["limiters"]["deltaTime"].as<double>();
			else uf::physics::time::clamp = 0;
		}
		// Mute audio
		if ( ::config["engine"]["audio"]["mute"].is<bool>() )
			uf::Audio::mute = ::config["engine"]["audio"]["mute"].as<bool>();

		// Set worker threads
		if ( ::config["engine"]["threads"]["workers"].as<std::string>() == "auto" ) {
			auto threads = std::max( 1, (int) std::thread::hardware_concurrency() - 1 );
			::config["engine"]["threads"]["workers"] = threads;
			uf::iostream << "Using " << threads << " worker threads" << "\n";
		}
		if ( ::config["engine"]["scenes"]["use graph"].is<bool>() ) {
			uf::scene::useGraph = ::config["engine"]["scenes"]["use graph"].as<bool>();
		}

	#if UF_USE_BULLET
		// set bullet parameters
		ext::bullet::debugDrawEnabled = ::config["engine"]["ext"]["bullet"]["debug draw"]["enabled"].as<bool>();
		ext::bullet::debugDrawRate = ::config["engine"]["ext"]["bullet"]["debug draw"]["rate"].as<float>();
		if ( ::config["engine"]["ext"]["bullet"]["iterations"].is<size_t>() ) {
			ext::bullet::iterations = ::config["engine"]["ext"]["bullet"]["iterations"].as<size_t>();
		}
		if ( ::config["engine"]["ext"]["bullet"]["substeps"].is<size_t>() ) {
			ext::bullet::substeps = ::config["engine"]["ext"]["bullet"]["substeps"].as<size_t>();
		}
		if ( ::config["engine"]["ext"]["bullet"]["timescale"].as<float>() ) {
			ext::bullet::timescale = ::config["engine"]["ext"]["bullet"]["timescale"].as<float>();
		}
		if ( ::config["engine"]["ext"]["bullet"]["pool size"]["max collision algorithm"].as<size_t>() ) {
			ext::bullet::defaultMaxCollisionAlgorithmPoolSize = ::config["engine"]["ext"]["bullet"]["pool size"]["max collision algorithm"].as<size_t>();
		}
		if ( ::config["engine"]["ext"]["bullet"]["pool size"]["max persistent manifold"].as<size_t>() ) {
			ext::bullet::defaultMaxPersistentManifoldPoolSize = ::config["engine"]["ext"]["bullet"]["pool size"]["max persistent manifold"].as<size_t>();
		}
	#endif
		uf::thread::workers = ::config["engine"]["threads"]["workers"].as<size_t>();
		// Enable valiation layer
	{
	#if UF_USE_VULKAN
		std::string RENDERER = "vulkan";
	#elif UF_USE_OPENGL
		std::string RENDERER = "opengl";
	#else
		std::string RENDERER = "software";
	#endif
		uf::renderer::settings::validation = ::config["engine"]["ext"][RENDERER]["validation"]["enabled"].as<bool>();
		
		if ( ::config["engine"]["ext"][RENDERER]["framebuffer"]["msaa"].is<double>() )
			uf::renderer::settings::msaa = ::config["engine"]["ext"][RENDERER]["framebuffer"]["msaa"].as<size_t>();

		if ( ::config["engine"]["ext"][RENDERER]["framebuffer"]["size"].is<double>() ) {
			float scale = ::config["engine"]["ext"][RENDERER]["framebuffer"]["size"].as<float>();
			uf::renderer::settings::width *= scale;
			uf::renderer::settings::height *= scale;
		} else if ( ext::json::isArray( ::config["engine"]["ext"][RENDERER]["framebuffer"]["size"] ) ) {
			uf::renderer::settings::width = ::config["engine"]["ext"][RENDERER]["framebuffer"]["size"][0].as<float>();
			uf::renderer::settings::height = ::config["engine"]["ext"][RENDERER]["framebuffer"]["size"][1].as<float>();
			std::string filter = ::config["engine"]["ext"][RENDERER]["framebuffer"]["size"][2].as<std::string>();
			if ( uf::string::lowercase( filter )  == "nearest" ) uf::renderer::settings::swapchainUpscaleFilter = uf::renderer::enums::Filter::NEAREST;
			else if ( uf::string::lowercase( filter )  == "linear" ) uf::renderer::settings::swapchainUpscaleFilter = uf::renderer::enums::Filter::LINEAR;
		}

		if ( ::config["engine"]["debug"]["entity"]["delete children on destroy"].is<bool>() ) uf::Entity::deleteChildrenOnDestroy = ::config["engine"]["debug"]["entity"]["delete children on destroy"].as<bool>();
		if ( ::config["engine"]["debug"]["entity"]["delete components on destroy"].is<bool>() ) uf::Entity::deleteComponentsOnDestroy = ::config["engine"]["debug"]["entity"]["delete components on destroy"].as<bool>();
		
		for ( int i = 0; i < ::config["engine"]["ext"][RENDERER]["validation"]["filters"].size(); ++i ) {
			uf::renderer::settings::validationFilters.push_back( ::config["engine"]["ext"][RENDERER]["validation"]["filters"][i].as<std::string>() );
		}
		for ( int i = 0; i < ::config["engine"]["ext"][RENDERER]["extensions"]["device"].size(); ++i ) {
			uf::renderer::settings::requestedDeviceExtensions.push_back( ::config["engine"]["ext"][RENDERER]["extensions"]["device"][i].as<std::string>() );
		}
		for ( int i = 0; i < ::config["engine"]["ext"][RENDERER]["extensions"]["instance"].size(); ++i ) {
			uf::renderer::settings::requestedInstanceExtensions.push_back( ::config["engine"]["ext"][RENDERER]["extensions"]["instance"][i].as<std::string>() );
		}
		for ( int i = 0; i < ::config["engine"]["ext"][RENDERER]["features"].size(); ++i ) {
			uf::renderer::settings::requestedDeviceFeatures.push_back( ::config["engine"]["ext"][RENDERER]["features"][i].as<std::string>() );
		}
	#if 1
		if ( ::config["engine"]["ext"][RENDERER]["experimental"]["rebuild on tick begin"].is<bool>() )
			uf::renderer::settings::experimental::rebuildOnTickBegin = ::config["engine"]["ext"][RENDERER]["experimental"]["rebuild on tick begin"].as<bool>();
		if ( ::config["engine"]["ext"][RENDERER]["experimental"]["wait on render end"].is<bool>() )
			uf::renderer::settings::experimental::waitOnRenderEnd = ::config["engine"]["ext"][RENDERER]["experimental"]["wait on render end"].as<bool>();
		if ( ::config["engine"]["ext"][RENDERER]["experimental"]["individual pipelines"].is<bool>() )
			uf::renderer::settings::experimental::individualPipelines = ::config["engine"]["ext"][RENDERER]["experimental"]["individual pipelines"].as<bool>();
		if ( ::config["engine"]["ext"][RENDERER]["experimental"]["multithreaded command recording"].is<bool>() )
			uf::renderer::settings::experimental::multithreadedCommandRecording = ::config["engine"]["ext"][RENDERER]["experimental"]["multithreaded command recording"].as<bool>();
		if ( ::config["engine"]["ext"][RENDERER]["experimental"]["deferred mode"].is<std::string>() )
			uf::renderer::settings::experimental::deferredMode = ::config["engine"]["ext"][RENDERER]["experimental"]["deferred mode"].as<std::string>();
		if ( ::config["engine"]["ext"][RENDERER]["experimental"]["deferred reconstruct position"].is<bool>() )
			uf::renderer::settings::experimental::deferredReconstructPosition = ::config["engine"]["ext"][RENDERER]["experimental"]["deferred reconstruct position"].as<bool>();
		if ( ::config["engine"]["ext"][RENDERER]["experimental"]["deferred alias output to swapchain"].is<bool>() )
			uf::renderer::settings::experimental::deferredAliasOutputToSwapchain = ::config["engine"]["ext"][RENDERER]["experimental"]["deferred alias output to swapchain"].as<bool>();
		if ( ::config["engine"]["ext"][RENDERER]["experimental"]["vsync"].is<bool>() )
			uf::renderer::settings::experimental::vsync = ::config["engine"]["ext"][RENDERER]["experimental"]["vsync"].as<bool>();
		if ( ::config["engine"]["ext"][RENDERER]["experimental"]["hdr"].is<bool>() )
			uf::renderer::settings::experimental::hdr = ::config["engine"]["ext"][RENDERER]["experimental"]["hdr"].as<bool>();
	#else
		uf::renderer::settings::experimental::rebuildOnTickBegin = ::config["engine"]["ext"][RENDERER]["experimental"]["rebuild on tick begin"].as<bool>();
		uf::renderer::settings::experimental::waitOnRenderEnd = ::config["engine"]["ext"][RENDERER]["experimental"]["wait on render end"].as<bool>();
		uf::renderer::settings::experimental::individualPipelines = ::config["engine"]["ext"][RENDERER]["experimental"]["individual pipelines"].as<bool>();
		uf::renderer::settings::experimental::multithreadedCommandRecording = ::config["engine"]["ext"][RENDERER]["experimental"]["multithreaded command recording"].as<bool>();
		uf::renderer::settings::experimental::deferredMode = ::config["engine"]["ext"][RENDERER]["experimental"]["deferred mode"].as<std::string>();
		uf::renderer::settings::experimental::deferredReconstructPosition = ::config["engine"]["ext"][RENDERER]["experimental"]["deferred reconstruct position"].as<bool>();
		uf::renderer::settings::experimental::deferredAliasOutputToSwapchain = ::config["engine"]["ext"][RENDERER]["experimental"]["deferred alias output to swapchain"].as<bool>();
		uf::renderer::settings::experimental::vsync = ::config["engine"]["ext"][RENDERER]["experimental"]["vsync"].as<bool>();
		uf::renderer::settings::experimental::hdr = ::config["engine"]["ext"][RENDERER]["experimental"]["hdr"].as<bool>();
	#endif
	#define JSON_TO_VKFORMAT( key ) if ( ::config["engine"]["ext"][RENDERER]["formats"][#key].is<std::string>() ) {\
			std::string format = ::config["engine"]["ext"][RENDERER]["formats"][#key].as<std::string>();\
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
		ext::lua::main = ::config["engine"]["ext"]["lua"]["main"].as<std::string>();
		for ( auto it  = ::config["engine"]["ext"]["lua"]["modules"].begin(); it != ::config["engine"]["ext"]["lua"]["modules"].end(); ++it ) {
			std::string key = it.key();
			ext::lua::modules[key] = ::config["engine"]["ext"]["lua"]["modules"][key].as<std::string>();
		}
		ext::lua::initialize();
	}
#endif

	#if UF_USE_BULLET
	/* Bullet */ {
		ext::bullet::initialize();
	}
#endif

	#if UF_USE_OPENVR
		ext::openvr::enabled = ::config["engine"]["ext"]["vr"]["enable"].as<bool>();
		ext::openvr::swapEyes = ::config["engine"]["ext"]["vr"]["swap eyes"].as<bool>();
		
		
		if ( ::config["engine"]["ext"]["vr"]["dominant eye"].is<int>() ) {
			ext::openvr::dominantEye = ::config["engine"]["ext"]["vr"]["dominant eye"].as<int>();
		} else if ( ::config["engine"]["ext"]["vr"]["dominant eye"].as<std::string>() == "left" ) ext::openvr::dominantEye = 0;
		else if ( ::config["engine"]["ext"]["vr"]["dominant eye"].as<std::string>() == "right" ) ext::openvr::dominantEye = 1;

		
		ext::openvr::driver.manifest = ::config["engine"]["ext"]["vr"]["manifest"].as<std::string>();
		
		if ( ext::openvr::enabled ) {
			::config["engine"]["render modes"]["stereo deferred"] = true;
		}
	#endif
	}

	/* Create initial scene (kludge) */ {
		uf::Scene& scene = uf::instantiator::instantiate<uf::Scene>(); //new uf::Scene;
		uf::scene::scenes.push_back(&scene);
		auto& metadata = scene.getComponent<uf::Serializer>();
		metadata["system"]["config"] = ::config;
	}

	/* Initialize Vulkan */ {
	#if UF_USE_VULKAN
		// setup render mode
		if ( ::config["engine"]["render modes"]["gui"].as<bool>() ) {
			auto* renderMode = new uf::renderer::RenderTargetRenderMode;
			uf::renderer::addRenderMode( renderMode, "Gui" );
			renderMode->blitter.descriptor.subpass = 1;
		}
		if ( ::config["engine"]["render modes"]["deferred"].as<bool>() ) {
			uf::renderer::addRenderMode( new uf::renderer::DeferredRenderMode, "" );
			if ( ::config["engine"]["render modes"]["stereo deferred"].as<bool>() ) {
				auto& renderMode = uf::renderer::getRenderMode("Deferred", true);
				renderMode.metadata["eyes"] = 2;
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

			uf::iostream << "Recommended VR Resolution: " << renderMode.width << ", " << renderMode.height << "\n";

			if ( ::config["engine"]["ext"]["vr"]["scale"].is<double>() ) {
				float scale = ::config["engine"]["ext"]["vr"]["scale"].as<float>();
				renderMode.width *= scale;
				renderMode.height *= scale;
				
				uf::iostream << "VR Resolution: " << renderMode.width << ", " << renderMode.height << "\n";
			}
		}
	#endif
	#if UF_USE_VULKAN
		/* Callbacks for 2KHR stuffs */ if ( false ) {
			uf::hooks.addHook("vulkan:Instance.ExtensionsEnabled", []( const ext::json::Value& json ) {
			//	std::cout << "vulkan:Instance.ExtensionsEnabled: " << json << std::endl;
			});
			uf::hooks.addHook("vulkan:Device.ExtensionsEnabled", []( const ext::json::Value& json ) {
			//	std::cout << "vulkan:Device.ExtensionsEnabled: " << json << std::endl;
			});
			uf::hooks.addHook("vulkan:Device.FeaturesEnabled", []( const ext::json::Value& json ) {
			//	std::cout << "vulkan:Device.FeaturesEnabled: " << json << std::endl;

				VkPhysicalDeviceFeatures2KHR deviceFeatures2{};
				VkPhysicalDeviceMultiviewFeaturesKHR extFeatures{};
				extFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES_KHR;
				deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
				deviceFeatures2.pNext = &extFeatures;
				PFN_vkGetPhysicalDeviceFeatures2KHR vkGetPhysicalDeviceFeatures2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2KHR>(vkGetInstanceProcAddr(uf::renderer::device.instance, "vkGetPhysicalDeviceFeatures2KHR"));
				vkGetPhysicalDeviceFeatures2KHR(uf::renderer::device.physicalDevice, &deviceFeatures2);
				std::cout << "Multiview features:" << std::endl;
				std::cout << "\tmultiview = " << extFeatures.multiview << std::endl;
				std::cout << "\tmultiviewGeometryShader = " << extFeatures.multiviewGeometryShader << std::endl;
				std::cout << "\tmultiviewTessellationShader = " << extFeatures.multiviewTessellationShader << std::endl;
				std::cout << std::endl;

				VkPhysicalDeviceProperties2KHR deviceProps2{};
				VkPhysicalDeviceMultiviewPropertiesKHR extProps{};
				extProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES_KHR;
				deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
				deviceProps2.pNext = &extProps;
				PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2KHR>(vkGetInstanceProcAddr(uf::renderer::device.instance, "vkGetPhysicalDeviceProperties2KHR"));
				vkGetPhysicalDeviceProperties2KHR(uf::renderer::device.physicalDevice, &deviceProps2);
				std::cout << "Multiview properties:" << std::endl;
				std::cout << "\tmaxMultiviewViewCount = " << extProps.maxMultiviewViewCount << std::endl;
				std::cout << "\tmaxMultiviewInstanceIndex = " << extProps.maxMultiviewInstanceIndex << std::endl;
			});
		}
	#endif

		uf::renderer::initialize();
	}

	/* */ {
		pod::Thread& threadMain = uf::thread::has("Main") ? uf::thread::get("Main") : uf::thread::create( "Main", false );
		pod::Thread& threadPhysics = uf::thread::has("Physics") ? uf::thread::get("Physics") : uf::thread::create( "Physics", true );
	}
#if UF_USE_DISCORD
	/* Discord */ if ( ::config["engine"]["ext"]["discord"]["enabled"].as<bool>() ) {
		ext::discord::initialize();
	}
#endif
#if UF_USE_ULTRALIGHT
	{
		auto& ultralight = ::config["engine"]["ext"]["ultralight"];
		/* Ultralight-UX */ if ( ultralight["enabled"].as<bool>() ) {
			if ( ultralight["scale"].is<double>() ) {
				ext::ultralight::scale = ultralight["scale"].as<double>();
			}
			if ( ultralight["log"].is<size_t>() ) {
				ext::ultralight::log = ultralight["log"].as<size_t>();
			}
			ext::ultralight::initialize();
		}
	}
#endif

//	#define DEBUG_PRINT() std::cout << __FILE__ << ":" __FUNCTION__ << "@" << __LINE__ << std::endl;	
	/* Add hooks */ {
		uf::hooks.addHook( "game:Scene.Load", [&](ext::json::Value& json){
			auto function = [json]() -> int {
				uf::renderer::synchronize();

				uf::hooks.call("game:Scene.Cleanup");
				// uf::scene::unloadScene();

				auto& scene = uf::scene::loadScene( json["scene"].as<std::string>() );
				auto& metadata = scene.getComponent<uf::Serializer>();
				metadata["system"]["config"] = ::config;

				return 0;
			};
			
			if ( json["immediate"].as<bool>() ) function(); else uf::thread::add( uf::thread::get("Main"), function, true );
		});

		uf::hooks.addHook( "game:Scene.Cleanup", [&](ext::json::Value& json){
			for ( auto* scene : uf::scene::scenes ) {
				if ( scene->hasComponent<uf::Audio>() ) {
					scene->getComponent<uf::Audio>().destroy();
				}
			}
			uf::scene::unloadScene();
		});
		uf::hooks.addHook( "system:Quit", [&](ext::json::Value& json){
		//	uf::iostream << "system:Quit: " << event << "\n";
			ext::ready = false;
		});
	}

	/* Initialize root scene*/ {
		uf::Serializer payload;
		payload["scene"] = ::config["engine"]["scenes"]["start"];
		payload["immediate"] = true;
		uf::hooks.call("game:Scene.Load", payload);
	}
	
	ext::ready = true;
	uf::iostream << "EXT took " << times.sys.elapsed().asDouble() << " seconds to initialize!" << "\n";

	{
		uf::thread::add( uf::thread::fetchWorker(), [&]() -> int {
			auto& scene = uf::scene::getCurrentScene();
			auto& assetLoader = scene.getComponent<uf::Asset>();
			assetLoader.processQueue();
		return 0;}, false );
	}
}

void EXT_API ext::tick() {
	/* Timer */ {
		times.prevTime = times.curTime;
		times.curTime = times.sys.elapsed().asDouble();
		times.deltaTime = times.curTime - times.prevTime;
	}

	/* Tick controllers */ {
		spec::controller::tick();
	}
#if UF_USE_OPENVR
	/* OpenVR */ if ( ext::openvr::context ) {
		ext::openvr::tick();
	}
#endif
#if 0
	/* Print Entity Information */  {
		TIMER(1, uf::Window::isKeyPressed("P") && ) {
	    //	uf::iostream << uf::renderer::allocatorStats() << "\n";
			uf::iostream << "==== Memory Pool Information ====";
			if ( uf::MemoryPool::global.size() > 0 ) uf::iostream << "\nGlobal Memory Pool: " << uf::MemoryPool::global.stats();
			if ( uf::Entity::memoryPool.size() > 0 ) uf::iostream << "\nEntity Memory Pool: " << uf::Entity::memoryPool.stats();
			if ( uf::component::memoryPool.size() > 0 ) uf::iostream << "\nComponents Memory Pool: " << uf::component::memoryPool.stats();
			if ( uf::userdata::memoryPool.size() > 0 ) uf::iostream << "\nUserdata Memory Pool: " << uf::userdata::memoryPool.stats();
			uf::iostream << "\n";
		}
	}
	/* Attempt to reset VR position */  {
		TIMER(1, uf::Window::isKeyPressed("Z") && ) {
	    	uf::hooks.call("VR:Seat.Reset");
		}
	}
	/* Print controller position */ if ( false ) {
		TIMER(1, uf::Window::isKeyPressed("Z") && ) {
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
#if UF_USE_BULLET
	auto& bulletThread = uf::thread::get("Physics");
	/* Update bullet */ {
	#if 1
		//UF_TIMER_TRACE_INIT();
		ext::bullet::tick();
		//UF_TIMER_TRACE("[TICK] BULLET");
	#else
		if ( ::config["engine"]["ext"]["bullet"]["multithreaded"].as<bool>() ) {
			uf::thread::add( bulletThread, [&]() -> int { ext::bullet::tick(); return 0;}, true );
		} else {
			ext::bullet::tick();
		}
	#endif
	}
#endif
	//UF_TIMER_TRACE("ticking physics");
	/* Update entities */ {
		uf::scene::tick();
	}
	//UF_TIMER_TRACE("ticking scene");
	/* Tick Main Thread Queue */ {
		pod::Thread& thread = uf::thread::get("Main");
		uf::thread::process( thread );
	}
	{
		auto& gc = ::config["engine"]["debug"]["garbage collection"];
		/* Garbage collection */ if ( gc["enabled"].as<bool>() ) {
			double every = gc["every"].as<double>();
			uint8_t mode = gc["mode"].as<uint64_t>();
			bool announce = gc["announce"].as<bool>();
			TIMER( every ) {
				size_t collected = uf::instantiator::collect( mode );
				if ( announce && collected > 0 ) UF_DEBUG_MSG("GC collected " << (int) collected << " unused entities");
			}
		}
	}
#if UF_USE_ULTRALIGHT
	/* Ultralight-UX */ if ( ::config["engine"]["ext"]["ultralight"]["enabled"].as<bool>() ) {
		ext::ultralight::tick();
	}
#endif
	/* Update vulkan */ {
		//UF_TIMER_TRACE_INIT();
		uf::renderer::tick();
		//UF_TIMER_TRACE("[TICK] RENDERER");
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
			double every = fps["every"].as<double>();
			TIMER( every ) {
	//			UF_DEBUG_MSG("Framerate: " << (1.0/times.deltaTime) << " FPS | Frametime: " << (times.deltaTime * 1000) << "ms");
				UF_DEBUG_MSG("System: " << (every * 1000.0/::times.frames) << " ms/frame | Time: " << time << " | Frames: " << ::times.frames << " | FPS: " << ::times.frames / time);
			#if UF_ENV_DREAMCAST
			//	pvr_stats_t stats;
			//	pvr_get_stats(&stats);
			//	UF_DEBUG_MSG("PVR stats: " << stats.frame_last_time << " ms | " << stats.frame_rate << " fps | " << stats.reg_last_time << " ms | " << stats.rnd_last_time << " ms | " << stats.vtx_buffer_used << " bytes | " << stats.vtx_buffer_used_max << " bytes | " << stats.buf_last_time << " ms" );
			#endif

				::times.frames = 0;
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
		/*
			if ( ::config["engine"]["debug"]["framerate"]["print"].as<bool>() ) {
				uf::iostream << "Frame limiting: " << elapsed << "ms exceeds limit, sleeping for " << elapsed << "ms" << "\n";
			}
		*/
			std::this_thread::sleep_for(std::chrono::milliseconds(sleep));
		}
		timer.reset();
	}
#endif
#if 0
	if ( ::config["engine"]["ext"]["bullet"]["multithreaded"].as<bool>() ) {
		uf::thread::wait( bulletThread );
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
#if UF_USE_BULLET
	/* Kill bullet */ {
		ext::bullet::terminate();
	}
#endif
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
	uf::scene::destroy();

	/* Garbage collection */ if ( false ) {
		uint8_t mode = ::config["engine"]["debug"]["garbage collection"]["mode"].as<uint64_t>();
		bool announce = ::config["engine"]["debug"]["garbage collection"]["announce"].as<bool>();
		size_t collected = uf::instantiator::collect( mode );
		if ( announce && collected > 0 ) {
			uf::iostream << "GC collected " << (int) collected << " unused entities" << "\n";
		}
	}

	/* Close vulkan */ {
		uf::renderer::destroy();
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
			std::string filename = uf::io::root+"/persistent.json";
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

#if 0
std::string EXT_API ext::getConfig() {
	struct {
		bool initialized = false;
		uf::Serializer file;
		uf::Serializer fallback;
		uf::Serializer merged;
	} static config;
	if ( config.initialized ) return config.merged;

	struct {
		bool exists = false;
		std::string filename = uf::io::root+"/config.json";
	} file;
	/* Read from file */  {
		file.exists = config.file.readFromFile(file.filename);
	}
	/* Initialize default configuration */ {
		config.fallback["window"]["terminal"]["ncurses"] 			= true;
		config.fallback["window"]["terminal"]["visible"] 			= true;
		config.fallback["window"]["title"] 							= "Grimgram";
		config.fallback["window"]["icon"] 							= "";
		config.fallback["window"]["size"]["x"] 						= 0;
		config.fallback["window"]["size"]["y"] 						= 0;
		config.fallback["window"]["visible"] 						= true;
	//	config.fallback["window"]["fullscreen"] 					= false;
		config.fallback["window"]["mode"] 							= "windowed";
		config.fallback["window"]["cursor"]["visible"] 				= true;
		config.fallback["window"]["cursor"]["center"] 				= false;
		config.fallback["window"]["keyboard"]["repeat"] 			= true;
		
		config.fallback["engine"]["scenes"]["start"]				= "StartMenu";
		config.fallback["engine"]["scenes"]["lights"]["max"] 			= 32;
		config.fallback["engine"]["hook"]["mode"] 					= "Readable";
		config.fallback["engine"]["limiters"]["framerate"] 			= 60;
		config.fallback["engine"]["limiters"]["deltaTime"] 			= 120;
		config.fallback["engine"]["threads"]["workers"] 			= "auto";
		config.fallback["engine"]["threads"]["frame limiter"] 		= 144;
		config.fallback["engine"]["memory pool"]["size"] 			= "512 MiB";
		config.fallback["engine"]["memory pool"]["globalOverride"] 	= false;
		config.fallback["engine"]["memory pool"]["subPools"] 		= true;
	}
	/* Merge */ if ( file.exists ){
		config.merged = config.file;
		config.merged.merge( config.fallback, true );
	} else {
		config.merged = config.fallback;
	}
	/* Write default to file */ if ( false ) {
		config.merged.writeToFile(file.filename);
	}

	config.initialized = true;
	return config.merged;
}
#endif