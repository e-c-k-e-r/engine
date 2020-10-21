#include "main.h"

#include <uf/ext/ext.h>
#include <uf/ext/vulkan/vulkan.h>
#include <uf/ext/oal/oal.h>
#include <uf/ext/assimp/assimp.h>

#include <uf/spec/terminal/terminal.h>

#include <uf/utils/time/time.h>
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

namespace {
	struct {
		uf::String input;
		std::ofstream output;

		struct {
			std::string output = "log/output.txt";
		} filenames;
	} io;

	struct {
		spec::Time::time_t 	epoch;
		uf::Timer<> 		sys = uf::Timer<>(false);
		uf::Timer<> 		delta = uf::Timer<>(false);
		double prevTime = 0;
		double curTime = 0;
		double deltaTime = 0;
	} times;

	uf::Serializer config;
}
bool ext::ready = false;
std::vector<std::string> ext::arguments;

void EXT_API ext::initialize() {
	::config = ext::getConfig();
	/* Arguments */ {
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
					auto keys = uf::string::split(keyString, ".");
					uf::Serializer value; value.deserialize(valueString);
					Json::Value* traversal = &::config;
					for ( auto& key : keys ) {
						traversal = &((*traversal)[key]);
					}
					*traversal = value;
				}
			}
		}
		uf::iostream << "Arguments: " << uf::Serializer(arguments) << "\n";
		uf::iostream << "New config: " << ::config << "\n";
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
	
	/* Parse config */ {
		// Set memory pool sizes
			{
				auto deduceSize = []( auto& value )->size_t{
					if ( value.isNumeric() ) return value.asUInt64();
					if ( value.isString() ) {
						std::string str = value.asString();
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
					uf::MemoryPool::globalOverride = ::config["engine"]["memory pool"]["globalOverride"].asBool();
					std::cout << "Requesting " << (int) size << " bytes for global memory pool: " << &uf::MemoryPool::global << std::endl;
					uf::MemoryPool::global.initialize( size );
					uf::MemoryPool::subPool = ::config["engine"]["memory pool"]["subPools"].asBool();
					if ( size <= 0 || uf::MemoryPool::subPool ) {
						{
							size_t size = deduceSize( ::config["engine"]["memory pools"]["component"] );
							std::cout << "Requesting " << (int) size << " bytes for component memory pool: " << &uf::component::memoryPool << std::endl;
							uf::component::memoryPool.initialize( size );
						}
						{
							size_t size = deduceSize( ::config["engine"]["memory pools"]["userdata"] );
							std::cout << "Requesting " << (int) size << " bytes for userdata memory pool: " << &uf::userdata::memoryPool << std::endl;
							uf::userdata::memoryPool.initialize( size );
						}
						{
							size_t size = deduceSize( ::config["engine"]["memory pools"]["entity"] );
							std::cout << "Requesting " << (int) size << " bytes for entity memory pool: " << &uf::Entity::memoryPool << std::endl;
							uf::Entity::memoryPool.initialize( size );
						}
					}
				}
			}
		/* Frame limiter */ {
			double limit = ::config["engine"]["limiters"]["framerate"].asDouble();
			if ( limit != 0 ) 
				uf::thread::limiter = 1.0 / ::config["engine"]["limiters"]["framerate"].asDouble();
			else uf::thread::limiter = 0;
		}
		/* Max delta time */{
			double limit = ::config["engine"]["limiters"]["deltaTime"].asDouble();
			if ( limit != 0 ) 
				uf::physics::time::clamp = 1.0 / ::config["engine"]["limiters"]["deltaTime"].asDouble();
			else uf::physics::time::clamp = 0;
		}
		/* Thread frame limiter */ {
			double limit = ::config["engine"]["threads"]["frame limiter"].asDouble();
			if ( limit != 0 ) 
				uf::thread::limiter = 1.0 / ::config["engine"]["threads"]["frame limiter"].asDouble();
		}
		// Set worker threads
		uf::thread::workers = ::config["engine"]["threads"]["workers"].asUInt64();
		// Enable valiation layer
		uf::renderer::validation = ::config["engine"]["ext"]["vulkan"]["validation"]["enabled"].asBool();
		if ( ::config["engine"]["ext"]["vulkan"]["validation"]["enabled"].isNumeric() )
			uf::renderer::msaa = ::config["engine"]["ext"]["vulkan"]["msaa"].asUInt64();
		
		uf::renderer::rebuildOnTickStart = ::config["engine"]["ext"]["vulkan"]["validation"]["rebuild on tick begin"].asBool();
		uf::renderer::waitOnRenderEnd = ::config["engine"]["ext"]["vulkan"]["validation"]["wait on render end"].asBool();

		for ( int i = 0; i < ::config["engine"]["ext"]["vulkan"]["validation"]["filters"].size(); ++i ) {
			uf::renderer::validationFilters.push_back( ::config["engine"]["ext"]["vulkan"]["validation"]["filters"][i].asString() );
		}
		for ( int i = 0; i < ::config["engine"]["ext"]["vulkan"]["extensions"]["device"].size(); ++i ) {
			uf::renderer::requestedDeviceExtensions.push_back( ::config["engine"]["ext"]["vulkan"]["extensions"]["device"][i].asString() );
		}
		for ( int i = 0; i < ::config["engine"]["ext"]["vulkan"]["extensions"]["instance"].size(); ++i ) {
			uf::renderer::requestedInstanceExtensions.push_back( ::config["engine"]["ext"]["vulkan"]["extensions"]["instance"][i].asString() );
		}
		for ( int i = 0; i < ::config["engine"]["ext"]["vulkan"]["features"].size(); ++i ) {
			uf::renderer::requestedDeviceFeatures.push_back( ::config["engine"]["ext"]["vulkan"]["features"][i].asString() );
		}
		ext::openvr::enabled = ::config["engine"]["ext"]["vr"]["enable"].asBool();
		ext::openvr::swapEyes = ::config["engine"]["ext"]["vr"]["swap eyes"].asBool();
		if ( ::config["engine"]["ext"]["vr"]["dominatEye"].isNumeric() )
			ext::openvr::dominantEye = ::config["engine"]["ext"]["vr"]["dominatEye"].asUInt64();
		else if ( ::config["engine"]["ext"]["vr"]["dominatEye"].asString() == "left" ) ext::openvr::dominantEye = 0;
		else if ( ::config["engine"]["ext"]["vr"]["dominatEye"].asString() == "right" ) ext::openvr::dominantEye = 1;
		ext::openvr::driver.manifest = ::config["engine"]["ext"]["vr"]["manifest"].asString();
		if ( ext::openvr::enabled ) {
			::config["engine"]["render modes"]["stereo deferred"] = true;
		}
	}


	/* Create initial scene (kludge) */ {
		uf::Scene& scene = uf::instantiator::instantiate<uf::Scene>(); //new uf::Scene;
		uf::scene::scenes.push_back(&scene);
		auto& metadata = scene.getComponent<uf::Serializer>();
		metadata["system"]["config"] = ::config;
	}

	/* Initialize Vulkan */ {
		// uf::renderer::width = ::config["window"]["size"]["x"].asInt();
		// uf::renderer::height = ::config["window"]["size"]["y"].asInt();

		// setup render mode
		if ( ::config["engine"]["render modes"]["gui"].asBool() ) {
			auto* renderMode = new uf::renderer::RenderTargetRenderMode;
			uf::renderer::addRenderMode( renderMode, "Gui" );
			renderMode->blitter.descriptor.subpass = 1;
		}
		if ( ::config["engine"]["render modes"]["stereo deferred"].asBool() )
			uf::renderer::addRenderMode( new uf::renderer::StereoscopicDeferredRenderMode, "" );
		else if ( ::config["engine"]["render modes"]["deferred"].asBool() )
			uf::renderer::addRenderMode( new uf::renderer::DeferredRenderMode, "" );

	//	if ( ::config["engine"]["render modes"]["compute"].asBool() )
	//		uf::renderer::addRenderMode( new uf::renderer::ComputeRenderMode, "C:RT:0" );

		if ( ext::openvr::enabled ) {
			ext::openvr::initialize();
		
			uint32_t width, height;
			ext::openvr::recommendedResolution( width, height );

			auto& renderMode = uf::renderer::getRenderMode("Stereoscopic Deferred", true);
			renderMode.width = width;
			renderMode.height = height;

			std::cout << "Recommended VR Resolution: " << width << ", " << height << std::endl;
		}

		uf::renderer::initialize();
	}
	/* */ {
		pod::Thread& threadMain = uf::thread::has("Main") ? uf::thread::get("Main") : uf::thread::create( "Main", false );
		pod::Thread& threadPhysics = uf::thread::has("Physics") ? uf::thread::get("Physics") : uf::thread::create( "Physics", true );
	}

	/* Discord */ if ( ::config["engine"]["ex"]["discord"]["enabled"].asBool() ) {
		ext::discord::initialize();
	}

	/* Initialize root scene*/ {
		uf::scene::unloadScene();
		auto& scene = uf::scene::loadScene( ::config["engine"]["scenes"]["start"].asString() );
		auto& metadata = scene.getComponent<uf::Serializer>();
		metadata["system"]["config"] = ::config;
	}

	/* Add hooks */ {
		uf::hooks.addHook( "game:LoadScene", [&](const std::string& event)->std::string{
			uf::Serializer json = event;
			std::cout << "SCENE CHANGE: " << event << std::endl;
			uf::renderer::mutex.lock();
			uf::renderer::mutex.unlock();
			uf::scene::unloadScene();
			auto& scene = uf::scene::loadScene( json["scene"].asString() );
			auto& metadata = scene.getComponent<uf::Serializer>();
			metadata["system"]["config"] = ::config;
			return "true";
		});

		uf::hooks.addHook( "system:Quit", [&](const std::string& event)->std::string{
			std::cout << "system:Quit: " << event << std::endl;
			ext::ready = false;
			return "true";
		});
	}

	ext::ready = true;
	uf::iostream << "EXT took " << times.sys.elapsed().asDouble() << " seconds to initialize!" << "\n";

	{
		uf::thread::add( uf::thread::fetchWorker(), [&]() -> int {
			uf::Asset& assetLoader = uf::scene::getCurrentScene().getComponent<uf::Asset>();
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
	static uf::Timer<long long> timer(false);
	if ( !timer.running() ) timer.start();

	/* Print World Tree */ {
		static uf::Timer<long long> timer(false);
		if ( !timer.running() ) timer.start();
		if ( uf::Window::isKeyPressed("U") && timer.elapsed().asDouble() >= 1 ) { timer.reset();
			std::function<void(uf::Entity*, int)> filter = []( uf::Entity* entity, int indent ) {
				for ( int i = 0; i < indent; ++i ) uf::iostream << "\t";
				uf::iostream << entity->getName() << ": " << entity->getUid();
				if ( entity->hasComponent<pod::Transform<>>() ) {
					pod::Transform<> t = uf::transform::flatten(entity->getComponent<pod::Transform<>>());
					uf::iostream << " (" << t.position.x << ", " << t.position.y << ", " << t.position.z << ") (" << t.orientation.x << ", " << t.orientation.y << ", " << t.orientation.z << ", " << t.orientation.w << ")";
				}
				uf::iostream << "\n";
			};
			for ( uf::Scene* scene : uf::renderer::scenes ) {
				if ( !scene ) continue;
				std::cout << "Scene: " << scene->getName() << ": " << scene << std::endl;
				scene->process(filter, 1);
			}
		}
	}
	/* Print World Tree */ {
		static uf::Timer<long long> timer(false);
		if ( !timer.running() ) timer.start();
		if ( uf::Window::isKeyPressed("U") && timer.elapsed().asDouble() >= 1 ) { timer.reset();
			std::function<void(uf::Entity*, int)> filter = []( uf::Entity* entity, int indent ) {
				for ( int i = 0; i < indent; ++i ) uf::iostream << "\t";
				uf::iostream << entity->getName() << ": " << entity->getUid() << " [";
				for ( auto& behavior : entity->getBehaviors() ) {
					std::cout << uf::instantiator::behaviors->names[behavior.type] << ", ";
				}
				uf::iostream << "]\n";
			};
			for ( uf::Scene* scene : uf::renderer::scenes ) {
				if ( !scene ) continue;
				std::cout << "Scene: " << scene->getName() << ": " << scene << std::endl;
				scene->process(filter, 1);
			}
			uf::Serializer instantiator;
			{
				int i = 0;
				for ( auto& pair : uf::instantiator::objects->names ) {
					instantiator["objects"][i++] = pair.second;
				}
			}
			{
				int i = 0;
				for ( auto& pair : uf::instantiator::behaviors->names ) {
					instantiator["behaviors"][i++] = pair.second;
				}
			}
			std::cout << instantiator << std::endl;
		}
	}
	/* Print Entity Information */  {
		static uf::Timer<long long> timer(false);
		if ( !timer.running() ) timer.start();
		if ( uf::Window::isKeyPressed("P") && timer.elapsed().asDouble() >= 1 ) { timer.reset();
	    //	uf::iostream << uf::renderer::allocatorStats() << "\n";
			if ( uf::MemoryPool::global.size() > 0 ) uf::iostream << "Global Memory Pool:\n" << uf::MemoryPool::global.stats() << "\n";
			if ( uf::Entity::memoryPool.size() > 0 ) uf::iostream << "Entity Memory Pool:\n" << uf::Entity::memoryPool.stats() << "\n";
			if ( uf::component::memoryPool.size() > 0 ) uf::iostream << "Components Memory Pool:\n" << uf::component::memoryPool.stats() << "\n";
			if ( uf::userdata::memoryPool.size() > 0 ) uf::iostream << "Userdata Memory Pool:\n" << uf::userdata::memoryPool.stats() << "\n";
		}
	}
	/* Attempt to reset VR position */  {
		static uf::Timer<long long> timer(false);
		if ( !timer.running() ) timer.start();
		if ( uf::Window::isKeyPressed("Z") && timer.elapsed().asDouble() >= 1 ) { timer.reset();
	    	uf::hooks.call("VR:Seat.Reset");
		}
	}
	/* Print controller position */ if ( false ) {
		static uf::Timer<long long> timer(false);
		if ( !timer.running() ) timer.start();
		if ( uf::Window::isKeyPressed("Z") && timer.elapsed().asDouble() >= 1 ) { timer.reset();
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
				if ( !lMetadata["light"].isArray() ) {
					lMetadata["light"]["color"][0] = (rand() % 100) / 100.0;
					lMetadata["light"]["color"][1] = (rand() % 100) / 100.0;
					lMetadata["light"]["color"][2] = (rand() % 100) / 100.0;
				}
				auto& sMetadata = scene.getComponent<uf::Serializer>();
				sMetadata["light"]["should"] = true;
			}
		}
	}

	/* Update physics timer */ {
		uf::physics::tick();
	}
	/* Update entities */ {
		uf::scene::tick();
	}

	/* Tick Main Thread Queue */ {
		pod::Thread& thread = uf::thread::has("Main") ? uf::thread::get("Main") : uf::thread::create( "Main", false, true );
		uf::thread::process( thread );
	}

	/* Update vulkan */ {
		uf::renderer::tick();
	}
	/* Discord */ if ( ::config["engine"]["ext"]["discord"]["enable"].asBool() ) {
		ext::discord::tick();
	}
	/* OpenVR */ if ( ext::openvr::context ) {
		ext::openvr::tick();
	}

	/* FPS Print */ if ( ::config["engine"]["debug"]["framerate"]["print"].asBool() ) {
		static uf::Timer<long long> timer(false);
		if ( !timer.running() ) timer.start();
		if ( timer.elapsed().asDouble() >= ::config["engine"]["debug"]["framerate"]["every"].asDouble() ) { timer.reset();
			std::cout << "Framerate: " << (1.0/times.deltaTime) << " FPS | Frametime: " << (times.deltaTime * 1000) << "ms" << std::endl;
		}
	}
	
	/* Frame limiter of sorts I guess */ if ( uf::thread::limiter > 0 ) {
		auto elapsed = timer.elapsed().asMilliseconds();
		long long sleep = (uf::thread::limiter * 1000) - elapsed;
		if ( sleep > 0 ) {
			if ( ::config["engine"]["debug"]["framerate"]["print"].asBool() ) {
				std::cout << "Frame limiting: " << elapsed << "ms exceeds limit, sleeping for " << elapsed << "ms" << std::endl;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(sleep));
		}
		timer.reset();
	}
}
void EXT_API ext::render() {
	// uf::scene::render();

	uf::renderer::render();

	/* OpenVR */ if ( ext::openvr::context ) {
		ext::openvr::submit();
	}
}
void EXT_API ext::terminate() {
	/* Kill threads */ {
		uf::thread::terminate();
	}

	/* OpenVR */ if ( ext::openvr::context ) {
		ext::openvr::terminate();
	}
	
	/* Close vulkan */ {
		uf::renderer::destroy();
	}


	/* Flush input buffer */ {
		io.output << io.input << std::endl;
		for ( const auto& str : uf::iostream.getHistory() ) io.output << str << std::endl;
		io.output << "\nTerminated after " << times.sys.elapsed().asDouble() << " seconds" << std::endl;
		io.output.close();
	}

	uf::scene::destroy();

	/* Write persistent data */ if ( false ) {
		struct {
			bool exists = false;
			std::string filename = "./data/persistent.json";
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
		std::string filename = "./data/config.json";
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
		config.fallback["engine"]["scenes"]["max lights"] 			= 32;
		config.fallback["engine"]["hook"]["mode"] 					= "Readable";
		config.fallback["engine"]["limiters"]["framerate"] 			= 60;
		config.fallback["engine"]["limiters"]["deltaTime"] 			= 120;
		config.fallback["engine"]["threads"]["workers"] 			= 1;
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