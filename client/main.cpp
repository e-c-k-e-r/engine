#include "main.h"

#include <uf/utils/io/iostream.h>
#include <uf/utils/time/time.h>

#include <uf/utils/memory/pool.h>

#if UF_NO_EXCEPTIONS
	#define HANDLE_EXCEPTIONS 0
#else
	#define HANDLE_EXCEPTIONS 0
#endif
int main(int argc, char** argv){
	for ( size_t i = 0; i < argc; ++i ) {
		char* c_str = argv[i];
		std::string string(argv[i]);
		ext::arguments.emplace_back(string);
	}

	std::atexit([]{
		uf::iostream << "Termination via std::atexit()!" << "\n";
		ext::ready = false;
		client::ready = false;
		client::terminated = true;
		ext::terminate();
		client::terminate();
	});

	client::initialize();
	ext::initialize();

	// For Multithreaded initialization
	while ( !client::ready || !ext::ready ) {
		static uf::Timer<long long> timer(false);
		static double next = 1;
		if ( !timer.running() ) timer.start();

		if ( timer.elapsed().asDouble() >= next ) {
			uf::iostream << "Waiting for " << ( client::ready ? "client" : "extension / engine" ) << " to initialize... Retrying in " << next << " seconds." << "\n";
			next *= 2;
		}
	}
	while ( client::ready && ext::ready ) {
	#if HANDLE_EXCEPTIONS
		try {	
	#endif
			static bool first = false; if ( !first ) { first = true;
				uf::Serializer json;
				std::string hook = "window:Resized";
				json["type"] = hook;
				json["invoker"] = "ext";
				json["window"]["size"] = client::config["window"]["size"];
				uf::hooks.call(hook, json);
			}
		#if UF_ENV_DREAMCAST
			ext::render();
			ext::tick();
		#else
			client::tick();
			client::render();
			ext::tick();
			ext::render();
		#endif
	#if HANDLE_EXCEPTIONS
		} catch ( std::runtime_error& e ) {
			uf::iostream << "RUNTIME ERROR: " << e.what() << "\n";
			break;
		} catch ( std::exception& e ) {
			uf::iostream << "EXCEPTION ERROR: " << e.what() << "\n";
			throw e;
		} catch ( bool handled ) {
			if (!handled) uf::iostream << "UNHANDLED ERROR: " << "???" << "\n";
		} catch ( ... ) {
			uf::iostream << "UNKNOWN ERROR: " << "???" << "\n";
		}
	#endif
	}
	if ( !client::terminated ) {
		uf::iostream << "Natural termination!" << "\n";
		ext::terminate();
		client::terminate();
	}
	return 0;
}