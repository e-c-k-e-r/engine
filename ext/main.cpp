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

#include <uf/engine/entity/entity.h>

#include <sys/stat.h>

#include <string>
#include <fstream>
#include <iostream>

#include "ext.h"
#include "asset/asset.h"

#include <uf/ext/vulkan/graphics/compute.h>
#include <uf/ext/vulkan/graphics/mesh.h>
#include <uf/ext/discord/discord.h>

namespace {
	struct {
		uf::String input;
		std::ofstream output;

		struct {
			std::string output = "log/output.txt";
		} filenames;
	} io;

	struct {
		struct {
			pod::Vector2ui size;
			uf::String title;
		} window;
		struct {
			uint state = 0;
		} opengl;
		struct {
			std::string mode = "Readable";
		} hook;
	} persistent;

	struct {	
		struct {
			pod::Vector4f ambient = {};
		} light;
	} gl;

	struct {
		spec::Time::time_t 	epoch;
		uf::Timer<> 		sys = uf::Timer<>(false);
		uf::Timer<> 		delta = uf::Timer<>(false);
		double prevTime = 0;
		double curTime = 0;
		double deltaTime = 0;
	} times;

	struct {
		ext::World master;
	} world;
}
bool ext::ready = false;

#include <uf/utils/http/http.h>

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
		#include "./inits/persistence.inl"
	}
	/* Initialize Vulkan */ {
		uf::Serializer config = ext::getConfig();
		ext::vulkan::width = config["window"]["size"]["x"].asInt();
		ext::vulkan::height = config["window"]["size"]["y"].asInt();
		ext::vulkan::initialize();
	}
	/* */ {
		pod::Thread& threadMain = uf::thread::has("Main") ? uf::thread::get("Main") : uf::thread::create( "Main", false, true );
		pod::Thread& threadAux = uf::thread::has("Aux") ? uf::thread::get("Aux") : uf::thread::create( "Aux", true, false );
		pod::Thread& threadPhysics = uf::thread::has("Physics") ? uf::thread::get("Physics") : uf::thread::create( "Physics", true, false );
	}
	/* Discord */ {
		ext::discord::initialize();
	}
	/* Create entities */ {
		::world.master.initialize();
	}

	ext::ready = true;
	uf::iostream << "EXT took " << times.sys.elapsed().asDouble() << " seconds to initialize!" << "\n";

	{
		uf::thread::add( uf::thread::fetchWorker(), [&]() -> int {
			ext::Asset& assetLoader = ::world.master.getComponent<ext::Asset>();
			assetLoader.processQueue();
		return 0;}, false );
	}

/*
	{
	
		std::vector<pod::Primitive> cubes;
		for ( int x = 0; x < 4; ++x )
		for ( int y = 0; y < 4; ++y )
		for ( int z = 0; z < 4; ++z ) {
			cubes.push_back( { pod::Primitive::CUBE, { x, y, z }, 0.5f } );
		}
	
		std::vector<pod::Tree> trees = uf::primitive::populateEntirely( cubes );
	}

	uf::iostream << "TEST took " << times.sys.elapsed().asDouble() << " seconds to initialize!" << "\n";
*/
}
void EXT_API ext::tick() {
	/* Timer */ {
		times.prevTime = times.curTime;
		times.curTime = times.sys.elapsed().asDouble();
		times.deltaTime = times.curTime - times.prevTime;
	}
	static uf::Timer<long long> timer(false);
	if ( !timer.running() ) timer.start();

	/* Update physics timer */ {
		uf::physics::tick();
	}
	/* Update entities */ {
		::world.master.tick();
	}

	/* Tick Main Thread Queue */ {
		pod::Thread& thread = uf::thread::has("Main") ? uf::thread::get("Main") : uf::thread::create( "Main", false, true );
		uf::thread::process( thread );
	}

	/* Update vulkan */ {
		ext::vulkan::tick();
	}
	/* Discord */ {
		ext::discord::tick();
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
//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BITz);
	/* Draw geometry */ {
		::world.master.render();
	}
	ext::vulkan::render();
}
void EXT_API ext::terminate() {
	/* Flush input buffer */ {
		io.output << io.input << std::endl;
		for ( const auto& str : uf::iostream.getHistory() ) io.output << str << std::endl;
		io.output << "\nTerminated after " << times.sys.elapsed().asDouble() << " seconds" << std::endl;
		io.output.close();
	}

	::world.master.destroy();

	/* Write persistent data */ {
		struct {
			bool exists = false;
			std::string filename = "cfg/persistent.json";
		} file;
		struct {
			uf::Serializer file;
		} config;
		/* Read from file */  {
			file.exists = config.file.readFromFile(file.filename);
		}
	//	config.file["window"]["size"]["x"] = persistent.window.size.x;
	//	config.file["window"]["size"]["y"] = persistent.window.size.y;
	//	config.file["window"]["title"] = std::string(persistent.window.title);
	//	config.file["OpenGL"]["state"] = persistent.opengl.state;

		config.file["meta"]["version"] = "2018.04.09";
		config.file["meta"]["time"]["initialized"] = times.sys.getStarting().asDouble();
		config.file["meta"]["time"]["terminated"] = times.sys.getEnding().asDouble();
		config.file["meta"]["time"]["elapsed"] = times.sys.elapsed().asDouble();

		/* Write persistent data */ {
//			config.file.writeToFile(file.filename);
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
		std::string filename = "cfg/ext.json";
	} file;
	/* Read from file */  {
		file.exists = config.file.readFromFile(file.filename);
	}

	/* Initialize fallback */ {
		config.fallback["terminal"]["visible"] 				= true;
		config.fallback["terminal"]["ncurses"] 				= true;
		config.fallback["window"]["title"] 					= "[uf] Grimgram - Extended";
		config.fallback["window"]["size"]["x"] 				= 640;
		config.fallback["window"]["size"]["y"] 				= 480;
		config.fallback["window"]["visible"] 				= true;

		config.fallback["cursor"]["visible"] 				= true;
		config.fallback["keyboard"]["repeat"] 				= true;
		
		config.fallback["hook"]["mode"] 					= "Readable";

		config.fallback["light"]["ambient"]["r"] 			= ::gl.light.ambient.x = 0.0f;
		config.fallback["light"]["ambient"]["g"] 			= ::gl.light.ambient.y = 0.0f;
		config.fallback["light"]["ambient"]["b"] 			= ::gl.light.ambient.z = 0.0f;
		config.fallback["light"]["ambient"]["a"] 			= ::gl.light.ambient.w = 1.0f;

		config.fallback["context"]["depthBits"] 			= 24;
		config.fallback["context"]["stencilBits"] 			= 4;
		config.fallback["context"]["bitsPerPixel"] 			= 8;
		config.fallback["context"]["antialiasingLevel"] 	= 0;
		config.fallback["context"]["majorVersion"] 			= 3;
		config.fallback["context"]["minorVersion"] 			= 0;
	}
	/* Merge */ if ( file.exists ){
		config.merged = config.file;
		config.merged.merge( config.fallback, true );
	} else {
		config.merged = config.fallback;
	}
	/* Write default to file */ {
		config.merged.writeToFile(file.filename);
	}
	/* Update title */ {
		persistent.window.title = config.merged["window"]["title"].asString();
	}
	/* Update hook mode */ {
		persistent.hook.mode = config.merged["hook"]["mode"].asString();
	}
	/* Update ambient color */ {
		::gl.light.ambient.x = config.merged["light"]["ambient"]["r"].asDouble();
		::gl.light.ambient.y = config.merged["light"]["ambient"]["g"].asDouble();
		::gl.light.ambient.z = config.merged["light"]["ambient"]["b"].asDouble();
		::gl.light.ambient.w = config.merged["light"]["ambient"]["a"].asDouble();
	}

	config.initialized = true;
	return config.merged;
}