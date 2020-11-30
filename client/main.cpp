#include "main.h"

#include <uf/utils/io/iostream.h>
#include <uf/utils/time/time.h>

#include <uf/utils/mempool/mempool.h>

#define HANDLE_EXCEPTIONS 0

int main(int argc, char** argv){
	for ( size_t i = 0; i < argc; ++i ) {
		char* c_str = argv[i];
		std::string string(argv[i]);
		ext::arguments.emplace_back(string);
	}

	std::atexit([]{
		uf::iostream << "Termination via std::atexit()!" << "\n";
		client::terminated = !(client::ready = ext::ready = false);
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
	#if defined(HANDLE_EXCEPTIONS)
		try {	
	#endif
			static bool first = false; if ( !first ) { first = true;
				uf::Serializer json;
				std::string hook = "window:Resized";
				json["type"] = hook;
				json["invoker"] = "ext";

				json["window"]["size"]["x"] = client::config["window"]["size"]["x"];
				json["window"]["size"]["y"] = client::config["window"]["size"]["y"];
				uf::hooks.call(hook, json);
			}
			client::tick();
			ext::tick();
			client::render();
			ext::render();
	#if defined(HANDLE_EXCEPTIONS)
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
	}
	ext::terminate();
	client::terminate();
	return 0;
}