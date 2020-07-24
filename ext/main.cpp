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
#include <uf/utils/mesh/mesh.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/http/http.h>

#include <uf/engine/entity/entity.h>

#include <sys/stat.h>

#include <string>
#include <fstream>
#include <iostream>

#include "ext.h"

#include <uf/engine/scene/scene.h>
#include <uf/engine/asset/asset.h>

#include <uf/ext/vulkan/rendermodes/deferred.h>
#include <uf/ext/vulkan/rendermodes/multiview.h>
#include <uf/ext/vulkan/rendermodes/rendertarget.h>
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

void EXT_API ext::initialize() {
	/**/ {
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
	
	::config = ext::getConfig();

	/* Parse config */ {
		/* Frame limiter */ {
			double limit = ::config["engine"]["frame limit"].asDouble();
			if ( limit != 0 ) 
				uf::thread::limiter = 1.0 / ::config["engine"]["frame limit"].asDouble();
			else uf::thread::limiter = 0;
		}
		/* Max delta time */{
			double limit = ::config["engine"]["delta limit"].asDouble();
			if ( limit != 0 ) 
				uf::physics::time::clamp = 1.0 / ::config["engine"]["delta limit"].asDouble();
			else uf::physics::time::clamp = 0;
		}
		// Set worker threads
		uf::thread::workers = ::config["engine"]["worker threads"].asUInt64();
		// Enable valiation layer
		ext::vulkan::validation = ::config["engine"]["ext"]["vulkan"]["validation"].asBool();
	}

	/* Initialize Vulkan */ {
		ext::vulkan::width = ::config["window"]["size"]["x"].asInt();
		ext::vulkan::height = ::config["window"]["size"]["y"].asInt();

		// setup render mode
		if ( ::config["engine"]["render modes"]["deferred"].asBool() )
			ext::vulkan::addRenderMode( new ext::vulkan::DeferredRenderMode, "" );
		if ( ::config["engine"]["render modes"]["gui"].asBool() )
			ext::vulkan::addRenderMode( new ext::vulkan::RenderTargetRenderMode, "Gui" );
		ext::vulkan::initialize();
	}
	/* */ {
		pod::Thread& threadMain = uf::thread::has("Main") ? uf::thread::get("Main") : uf::thread::create( "Main", false, true );
		pod::Thread& threadAux = uf::thread::has("Aux") ? uf::thread::get("Aux") : uf::thread::create( "Aux", true, false );
		pod::Thread& threadPhysics = uf::thread::has("Physics") ? uf::thread::get("Physics") : uf::thread::create( "Physics", true, false );
	}

	/* Discord */ if ( ::config["engine"]["ex"]["discord"]["enabled"].asBool() ) {
		ext::discord::initialize();
	}

	/* Initialize root scene*/ {
		uf::scene::loadScene( ::config["engine"]["scenes"]["start"].asString() );
	}

	/* Add hooks */ {
		uf::hooks.addHook( "game:LoadScene", [&](const std::string& event)->std::string{
			uf::Serializer json = event;
			uf::scene::unloadScene();
			uf::scene::loadScene( json["scene"].asString() );
			return "true";
		});

		uf::hooks.addHook( "system:Quit", [&](const std::string& event)->std::string{
			std::cout << event << std::endl;
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
	{
		uf::thread::add( uf::thread::fetchWorker(), [&]() -> int {
			/* OpenVR */ if ( ::config["engine"]["ext"]["vr"]["enable"].asBool() ) {
				ext::openvr::initialize();
				uint32_t width, height;
				ext::openvr::recommendedResolution( width, height );
				uf::Serializer payload;
				payload["window"]["size"]["x"] = width;
				payload["window"]["size"]["y"] = height;
			/*
				ext::vulkan::command->width = width;
				ext::vulkan::command->height = height;

				std::cout << ext::vulkan::command->width << ", " << ext::vulkan::command->height << std::endl;
				std::cout << width << ", " << height << std::endl;
				ext::vulkan::swapchain.rebuild = true;
			*/
				if ( ::config["engine"]["ext"]["vr"]["resize"].asBool() )
					uf::hooks.call("window:Resized", payload);
			}
		return 0;}, true );
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
					uf::iostream << " (" << t.position.x << ", " << t.position.y << ", " << t.position.z << ")";
				}
				uf::iostream << "\n";
			};
			for ( uf::Scene* scene : ext::vulkan::scenes ) {
				if ( !scene ) continue;
				std::cout << "Scene: " << scene->getName() << ": " << scene << std::endl;
				scene->process(filter, 1);
			}
		}
	}
	/* Print Entity Information */  {
		static uf::Timer<long long> timer(false);
		if ( !timer.running() ) timer.start();
		if ( uf::Window::isKeyPressed("P") && timer.elapsed().asDouble() >= 1 ) { timer.reset();
	    	uf::iostream << ext::vulkan::allocatorStats() << "\n";
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
		ext::vulkan::tick();
	}
	/* Discord */ if ( ::config["engine"]["ext"]["discord"]["enable"].asBool() ) {
		ext::discord::tick();
	}
	/* OpenVR */ if ( ext::openvr::context ) {
		ext::openvr::tick();
	}
	
	/* Frame limiter of sorts I guess */ {
		long long sleep = (uf::thread::limiter * 1000) - timer.elapsed().asMilliseconds();
		if ( sleep > 0 ) {
		//	std::cout << "Frame limiting: " << sleep << ", " << timer.elapsed().asMilliseconds() << std::endl;
			std::this_thread::sleep_for(std::chrono::milliseconds(sleep));
		}
		timer.reset();
	}
}
void EXT_API ext::render() {
	uf::scene::render();

	ext::vulkan::render();

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
		config.fallback["window"]["terminal"]["ncurses"] 	= true;
		config.fallback["window"]["terminal"]["visible"] 	= true;
		config.fallback["window"]["title"] 					= "Grimgram";
		config.fallback["window"]["icon"] 					= "";
		config.fallback["window"]["size"]["x"] 				= 1280;
		config.fallback["window"]["size"]["y"] 				= 720;
		config.fallback["window"]["visible"] 				= true;
		config.fallback["window"]["fullscreen"] 			= false;
		config.fallback["window"]["cursor"]["visible"] 		= true;
		config.fallback["window"]["cursor"]["center"] 		= false;
		config.fallback["window"]["keyboard"]["repeat"] 	= true;
		
		config.fallback["engine"]["scenes"]["start"]		= "StartMenu";
		config.fallback["engine"]["hook"]["mode"] 			= "Readable";
		config.fallback["engine"]["frame limit"] 			= 60;
		config.fallback["engine"]["delta limit"] 			= 120;
		config.fallback["engine"]["worker threads"] 		= 1;
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