#include "main.h"

#include <uf/utils/io/iostream.h>
#include <uf/utils/time/time.h>

int main(int argc, char** argv){
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
		try {	
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
		} catch ( std::runtime_error& e ) {
			uf::iostream << "RUNTIME ERROR: " << e.what() << "\n";
			std::abort();
			raise(SIGSEGV);
			throw e;
		} catch ( std::exception& e ) {
			uf::iostream << "EXCEPTION ERROR: " << e.what() << "\n";
		} catch ( bool handled ) {
			if (!handled) uf::iostream << "UNHANDLED ERROR: " << "???" << "\n";
		} catch ( ... ) {
			uf::iostream << "UNKNOWN ERROR: " << "???" << "\n";
		}
	}

	if ( !client::terminated ) {
		uf::iostream << "Natural termination!" << "\n";
		ext::terminate();
		client::terminate();
	}
	return 0;
}