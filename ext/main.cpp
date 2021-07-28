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
#include <uf/ext/bullet/bullet.h>

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
		spec::Time::time_t 	epoch;
		uf::Timer<> 		sys = uf::Timer<>(false);
		uf::Timer<> 		delta = uf::Timer<>(false);
		double prevTime = 0;
		double curTime = 0;
		double deltaTime = 0;
		size_t frames = 0;
		double limiter = 1.0 / 144.0;
		struct {
			size_t frames = 0;
			double time = 0;
		} total;
	} times;

	uf::Serializer& config = ext::config;
}

namespace {
	uf::stl::string getConfig() {
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
			uf::stl::string filename = uf::io::root+"/config.json";
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
			// check if we are even allowed to use memory pools
			bool enabled = ::config["engine"]["memory pool"]["enabled"].as(true);
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
			uf::memoryPool::alignment = ::config["engine"]["memory pool"]["alignment"].as( uf::memoryPool::alignment );
			// set memory pool sizes
			size_t size = deduceSize( ::config["engine"]["memory pool"]["size"] );
			UF_MSG_DEBUG("Requesting " << (size_t) size << " bytes for global memory pool: " << &uf::memoryPool::global);
			uf::memoryPool::global.initialize( size );
			uf::memoryPool::subPool = ::config["engine"]["memory pool"]["subPools"].as( uf::memoryPool::subPool );
			if ( size <= 0 || uf::memoryPool::subPool ) {
				{
					size_t size = deduceSize( ::config["engine"]["memory pool"]["pools"]["component"] );
					UF_MSG_DEBUG("Requesting " << (int) size << " bytes for component memory pool: " << &uf::component::memoryPool);
					uf::component::memoryPool.initialize( size );
				}
				{
					size_t size = deduceSize( ::config["engine"]["memory pool"]["pools"]["userdata"] );
					UF_MSG_DEBUG("Requesting " << (int) size << " bytes for userdata memory pool: " << &uf::userdata::memoryPool);
					uf::userdata::memoryPool.initialize( size );
				}
				{
					size_t size = deduceSize( ::config["engine"]["memory pool"]["pools"]["entity"] );
					UF_MSG_DEBUG("Requesting " << (int) size << " bytes for entity memory pool: " << &uf::Entity::memoryPool);
					uf::Entity::memoryPool.initialize( size );
				}
			}
			uf::allocator::override = ::config["engine"]["memory pool"]["override"].as( uf::allocator::override );
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

		if ( ::config["engine"]["limiters"]["framerate"].as<uf::stl::string>() == "auto" && ::config["window"]["refresh rate"].is<size_t>() ) {
			double scale = 1.0;
			size_t refreshRate = ::config["window"]["refresh rate"].as<size_t>();
			::config["engine"]["limiters"]["framerate"] = refreshRate * scale;
			UF_MSG_DEBUG("Setting framerate cap to " << (int) refreshRate * scale);
		}
		if ( ::config["engine"]["threads"]["frame limiter"].as<uf::stl::string>() == "auto" && ::config["window"]["refresh rate"].is<size_t>() ) {
			double scale = 2.0;
			size_t refreshRate = ::config["window"]["refresh rate"].as<size_t>();
			::config["engine"]["threads"]["frame limiter"] = refreshRate * scale;
			UF_MSG_DEBUG("Setting thread frame limiter to " << (int) refreshRate * scale);
		}
		/* Frame limiter */ {
			double limit = ::config["engine"]["limiters"]["framerate"].as<double>();
			::times.limiter = limit != 0 ? 1.0 / limit : 0;
		}
		/* Thread frame limiter */ {
			double limit = ::config["engine"]["threads"]["frame limiter"].as<double>();
			uf::thread::limiter = limit != 0 ? 1.0 / limit : 0;
		}
		/* Max delta time */{
			double limit = ::config["engine"]["limiters"]["deltaTime"].as<double>();
			uf::physics::time::clamp = limit != 0 ? 1.0 / limit : 0;
		}

		// Set worker threads
		if ( ::config["engine"]["threads"]["workers"].as<uf::stl::string>() == "auto" ) {
			auto threads = std::max( 1, (int) std::thread::hardware_concurrency() - 1 );
			::config["engine"]["threads"]["workers"] = threads;
			UF_MSG_DEBUG("Using " << threads << " worker threads");
		}
		// Mute audio
		uf::audio::muted = ::config["engine"]["audio"]["mute"].as( uf::audio::muted );
		uf::audio::streamsByDefault = ::config["engine"]["audio"]["streams by default"].as( uf::audio::streamsByDefault );
		uf::audio::bufferSize = ::config["engine"]["audio"]["buffers"]["size"].as( uf::audio::bufferSize );
		uf::audio::buffers = ::config["engine"]["audio"]["buffers"]["count"].as( uf::audio::buffers );
	#if UF_AUDIO_MAPPED_VOLUMES
		ext::json::forEach( ::config["engine"]["audio"]["volumes"], []( const uf::stl::string& key, ext::json::Value& value ){
			float volume; volume = value.as(volume);
			uf::audio::volumes[key] = volume;
		});
	#else
		uf::audio::volumes::bgm = ::config["engine"]["audio"]["volumes"]["bgm"].as(uf::audio::volumes::bgm);
		uf::audio::volumes::sfx = ::config["engine"]["audio"]["volumes"]["sfx"].as(uf::audio::volumes::sfx);
		uf::audio::volumes::voice = ::config["engine"]["audio"]["volumes"]["voice"].as(uf::audio::volumes::voice);
	#endif
	#if UF_USE_OPENAL
		/* Initialize OpenAL */ {
			ext::al::initialize();
		}
	#endif

	#if UF_USE_BULLET
		// set bullet parameters
		ext::bullet::iterations = ::config["engine"]["ext"]["bullet"]["iterations"].as( ext::bullet::iterations );
		ext::bullet::substeps = ::config["engine"]["ext"]["bullet"]["substeps"].as( ext::bullet::substeps );
		ext::bullet::timescale = ::config["engine"]["ext"]["bullet"]["timescale"].as( ext::bullet::timescale );
		ext::bullet::defaultMaxCollisionAlgorithmPoolSize = ::config["engine"]["ext"]["bullet"]["pool size"]["max collision algorithm"].as( ext::bullet::defaultMaxCollisionAlgorithmPoolSize );
		ext::bullet::defaultMaxPersistentManifoldPoolSize = ::config["engine"]["ext"]["bullet"]["pool size"]["max persistent manifold"].as( ext::bullet::defaultMaxPersistentManifoldPoolSize );
		
		ext::bullet::debugDrawEnabled = ::config["engine"]["ext"]["bullet"]["debug draw"]["enabled"].as( ext::bullet::debugDrawEnabled );
		ext::bullet::debugDrawRate = ::config["engine"]["ext"]["bullet"]["debug draw"]["rate"].as( ext::bullet::debugDrawRate );
		ext::bullet::debugDrawLayer = ::config["engine"]["ext"]["bullet"]["debug draw"]["layer"].as( ext::bullet::debugDrawLayer );
	#endif
		uf::thread::workers = ::config["engine"]["threads"]["workers"].as<size_t>();
	{
	#if UF_USE_VULKAN
		uf::stl::string RENDERER = "vulkan";
	#elif UF_USE_OPENGL
		uf::stl::string RENDERER = "opengl";
	#else
		uf::stl::string RENDERER = "software";
	#endif
		uf::renderer::settings::validation = ::config["engine"]["ext"][RENDERER]["validation"]["enabled"].as( uf::renderer::settings::validation );
		uf::renderer::settings::msaa = ::config["engine"]["ext"][RENDERER]["framebuffer"]["msaa"].as( uf::renderer::settings::msaa );

		if ( ::config["engine"]["ext"][RENDERER]["framebuffer"]["size"].is<float>() ) {
			float scale = ::config["engine"]["ext"][RENDERER]["framebuffer"]["size"].as(1.0f);
			uf::renderer::settings::width *= scale;
			uf::renderer::settings::height *= scale;
		} else if ( ext::json::isArray( ::config["engine"]["ext"][RENDERER]["framebuffer"]["size"] ) ) {
			uf::renderer::settings::width = ::config["engine"]["ext"][RENDERER]["framebuffer"]["size"][0].as<float>();
			uf::renderer::settings::height = ::config["engine"]["ext"][RENDERER]["framebuffer"]["size"][1].as<float>();
			uf::stl::string filter = ::config["engine"]["ext"][RENDERER]["framebuffer"]["size"][2].as<uf::stl::string>();
			if ( uf::string::lowercase( filter ) == "nearest" ) uf::renderer::settings::swapchainUpscaleFilter = uf::renderer::enums::Filter::NEAREST;
			else if ( uf::string::lowercase( filter ) == "linear" ) uf::renderer::settings::swapchainUpscaleFilter = uf::renderer::enums::Filter::LINEAR;
		}

		uf::Entity::deleteChildrenOnDestroy = ::config["engine"]["debug"]["entity"]["delete children on destroy"].as( uf::Entity::deleteChildrenOnDestroy );
		uf::Entity::deleteComponentsOnDestroy = ::config["engine"]["debug"]["entity"]["delete components on destroy"].as( uf::Entity::deleteComponentsOnDestroy );
		
		for ( int i = 0; i < ::config["engine"]["ext"][RENDERER]["validation"]["filters"].size(); ++i ) {
			uf::renderer::settings::validationFilters.emplace_back( ::config["engine"]["ext"][RENDERER]["validation"]["filters"][i].as<uf::stl::string>() );
		}
		for ( int i = 0; i < ::config["engine"]["ext"][RENDERER]["extensions"]["device"].size(); ++i ) {
			uf::renderer::settings::requestedDeviceExtensions.emplace_back( ::config["engine"]["ext"][RENDERER]["extensions"]["device"][i].as<uf::stl::string>() );
		}
		for ( int i = 0; i < ::config["engine"]["ext"][RENDERER]["extensions"]["instance"].size(); ++i ) {
			uf::renderer::settings::requestedInstanceExtensions.emplace_back( ::config["engine"]["ext"][RENDERER]["extensions"]["instance"][i].as<uf::stl::string>() );
		}
		for ( int i = 0; i < ::config["engine"]["ext"][RENDERER]["features"].size(); ++i ) {
			uf::renderer::settings::requestedDeviceFeatures.emplace_back( ::config["engine"]["ext"][RENDERER]["features"][i].as<uf::stl::string>() );
		}

		uf::renderer::settings::experimental::rebuildOnTickBegin = ::config["engine"]["ext"][RENDERER]["experimental"]["rebuild on tick begin"].as( uf::renderer::settings::experimental::rebuildOnTickBegin );
		uf::renderer::settings::experimental::waitOnRenderEnd = ::config["engine"]["ext"][RENDERER]["experimental"]["wait on render end"].as( uf::renderer::settings::experimental::waitOnRenderEnd );
		uf::renderer::settings::experimental::individualPipelines = ::config["engine"]["ext"][RENDERER]["experimental"]["individual pipelines"].as( uf::renderer::settings::experimental::individualPipelines );
		uf::renderer::settings::experimental::multithreadedCommandRecording = ::config["engine"]["ext"][RENDERER]["experimental"]["multithreaded command recording"].as( uf::renderer::settings::experimental::multithreadedCommandRecording );
		uf::renderer::settings::experimental::multithreadedCommandRendering = ::config["engine"]["ext"][RENDERER]["experimental"]["multithreaded command rendering"].as( uf::renderer::settings::experimental::multithreadedCommandRendering );
		uf::renderer::settings::experimental::deferredMode = ::config["engine"]["ext"][RENDERER]["experimental"]["deferred mode"].as( uf::renderer::settings::experimental::deferredMode );
		uf::renderer::settings::experimental::deferredReconstructPosition = ::config["engine"]["ext"][RENDERER]["experimental"]["deferred reconstruct position"].as( uf::renderer::settings::experimental::deferredReconstructPosition );
		uf::renderer::settings::experimental::deferredAliasOutputToSwapchain = ::config["engine"]["ext"][RENDERER]["experimental"]["deferred alias output to swapchain"].as( uf::renderer::settings::experimental::deferredAliasOutputToSwapchain );
		uf::renderer::settings::experimental::vsync = ::config["engine"]["ext"][RENDERER]["experimental"]["vsync"].as( uf::renderer::settings::experimental::vsync );
		uf::renderer::settings::experimental::hdr = ::config["engine"]["ext"][RENDERER]["experimental"]["hdr"].as( uf::renderer::settings::experimental::hdr );
		uf::renderer::settings::experimental::vxgi = ::config["engine"]["ext"][RENDERER]["experimental"]["vxgi"].as( uf::renderer::settings::experimental::vxgi );
		uf::renderer::settings::experimental::deferredSampling = ::config["engine"]["ext"][RENDERER]["experimental"]["deferred sampling"].as( uf::renderer::settings::experimental::deferredSampling );
		uf::renderer::settings::experimental::culling = ::config["engine"]["ext"][RENDERER]["experimental"]["culling"].as( uf::renderer::settings::experimental::culling );
		uf::renderer::settings::experimental::bloom = ::config["engine"]["ext"][RENDERER]["experimental"]["bloom"].as( uf::renderer::settings::experimental::bloom );
	#define JSON_TO_VKFORMAT( key ) if ( ::config["engine"]["ext"][RENDERER]["formats"][#key].is<uf::stl::string>() ) {\
			uf::stl::string format = ::config["engine"]["ext"][RENDERER]["formats"][#key].as<uf::stl::string>();\
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
		ext::lua::enabled = ::config["engine"]["ext"]["lua"]["enabled"].as(ext::lua::enabled);
		ext::lua::main = ::config["engine"]["ext"]["lua"]["main"].as(ext::lua::main);
		ext::json::forEach( ::config["engine"]["ext"]["lua"]["modules"], []( const uf::stl::string& key, ext::json::Value& value ){
			ext::lua::modules[key] = value.as<uf::stl::string>();
		});
		ext::lua::initialize();
	}
#endif

	#if UF_USE_BULLET
	/* Bullet */ {
		ext::bullet::initialize();
	}
#endif

	#if UF_USE_OPENVR
		ext::openvr::enabled = ::config["engine"]["ext"]["vr"]["enable"].as( ext::openvr::enabled );
		ext::openvr::swapEyes = ::config["engine"]["ext"]["vr"]["swap eyes"].as( ext::openvr::swapEyes );
		
		
		if ( ::config["engine"]["ext"]["vr"]["dominant eye"].is<int>() ) {
			ext::openvr::dominantEye = ::config["engine"]["ext"]["vr"]["dominant eye"].as<int>();
		} else if ( ::config["engine"]["ext"]["vr"]["dominant eye"].as<uf::stl::string>() == "left" ) ext::openvr::dominantEye = 0;
		else if ( ::config["engine"]["ext"]["vr"]["dominant eye"].as<uf::stl::string>() == "right" ) ext::openvr::dominantEye = 1;

		ext::openvr::driver.manifest = ::config["engine"]["ext"]["vr"]["manifest"].as<uf::stl::string>();
		
		if ( ext::openvr::enabled ) ::config["engine"]["render modes"]["stereo deferred"] = true;
	#endif
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

			if ( ::config["engine"]["ext"]["vr"]["scale"].is<double>() ) {
				float scale = ::config["engine"]["ext"]["vr"]["scale"].as<float>();
				renderMode.width *= scale;
				renderMode.height *= scale;
				
				UF_MSG_DEBUG("VR Resolution: " << renderMode.width << ", " << renderMode.height);
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
				UF_MSG_DEBUG("Multiview features:" );
				UF_MSG_DEBUG("\tmultiview = " << extFeatures.multiview );
				UF_MSG_DEBUG("\tmultiviewGeometryShader = " << extFeatures.multiviewGeometryShader );
				UF_MSG_DEBUG("\tmultiviewTessellationShader = " << extFeatures.multiviewTessellationShader );

				VkPhysicalDeviceProperties2KHR deviceProps2{};
				VkPhysicalDeviceMultiviewPropertiesKHR extProps{};
				extProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES_KHR;
				deviceProps2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
				deviceProps2.pNext = &extProps;
				PFN_vkGetPhysicalDeviceProperties2KHR vkGetPhysicalDeviceProperties2KHR = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2KHR>(vkGetInstanceProcAddr(uf::renderer::device.instance, "vkGetPhysicalDeviceProperties2KHR"));
				vkGetPhysicalDeviceProperties2KHR(uf::renderer::device.physicalDevice, &deviceProps2);
				UF_MSG_DEBUG("Multiview properties:");
				UF_MSG_DEBUG("\tmaxMultiviewViewCount = " << extProps.maxMultiviewViewCount);
				UF_MSG_DEBUG("\tmaxMultiviewInstanceIndex = " << extProps.maxMultiviewInstanceIndex);
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
			ext::ultralight::scale = ultralight["scale"].as<double>( ext::ultralight::scale );
			ext::ultralight::log = ultralight["log"].as<size_t>( ext::ultralight::log );
			ext::ultralight::initialize();
		}
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

		uf::thread::add( uf::thread::fetchWorker(), [&]() -> int {
			auto& scene = uf::scene::getCurrentScene();
			auto& assetLoader = scene.getComponent<uf::Asset>();
			assetLoader.processQueue();
		return 0;}, false );
	}
	
	ext::ready = true;
	UF_MSG_INFO("EXT took " << times.sys.elapsed().asDouble() << " seconds to initialize!");

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
	/* Print Memory Pool Information */  {
		TIMER(1, uf::Window::isKeyPressed("P") && ) {
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
	/* Update graph */ {
		uf::graph::tick();
	}
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
				if ( collected > 0 ) {
					if ( announce ) UF_MSG_DEBUG("GC collected " << (int) collected << " unused entities");
				//	uf::renderer::states::rebuild = true;
				}
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
			++::times.total.frames;
			double every = fps["every"].as<double>();
			TIMER( every ) {
				UF_MSG_DEBUG("System: " << (every * 1000.0/::times.frames) << " ms/frame | Time: " << time << " | Frames: " << ::times.frames << " | FPS: " << ::times.frames / time);
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
		// 	if ( ::config["engine"]["debug"]["framerate"]["print"].as<bool>() ) UF_MSG_DEBUG("Frame limiting: " << elapsed << "ms exceeds limit, sleeping for " << elapsed << "ms");
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