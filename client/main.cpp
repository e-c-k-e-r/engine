#include "main.h"

#include <uf/utils/io/iostream.h>
#include <uf/utils/time/time.h>

#include <uf/utils/memory/pool.h>
#include <filesystem>

#if UF_NO_EXCEPTIONS
	#define HANDLE_EXCEPTIONS 0
#else
	#define HANDLE_EXCEPTIONS 1
#endif

#if UFENV_DREAMCAST
	#include <arch/dreamcast/kernel/stack.h>
#endif
#include <signal.h>

namespace {
	namespace handlers {
		void exit() {
		#if UF_ENV_DREAMCAST
			arch_stk_trace(1);
		#endif
			UF_MSG_INFO("Termination via std::atexit()!");
			ext::ready = false;
			client::ready = false;
			client::terminated = true;
			ext::terminate();
			client::terminate();
		}

		void abrt( int sig ) {
			UF_MSG_ERROR("Abort detected");
		#if UF_ENV_DREAMCAST
			arch_stk_trace(1);
		#endif
		}

		void segv( int sig ) {
			UF_MSG_ERROR("Segfault detected");
		#if UF_ENV_DREAMCAST
			arch_stk_trace(1);
		#endif
		}
	}
}
#include <uf/utils/mesh/mesh.h>
int main(int argc, char** argv){
	for ( size_t i = 0; i < argc; ++i ) {
		char* c_str = argv[i];
		std::string string(argv[i]);
		ext::arguments.emplace_back(string);
	}

	std::atexit(::handlers::exit);
	signal(SIGABRT, ::handlers::abrt);
	signal(SIGSEGV, ::handlers::segv);

	client::initialize();
	ext::initialize();

	// For Multithreaded initialization
	while ( !client::ready || !ext::ready ) {
		static uf::Timer<long long> timer(false);
		static double next = 1;
		if ( !timer.running() ) timer.start();

		if ( timer.elapsed().asDouble() >= next ) {
			UF_MSG_INFO("Waiting for " << ( client::ready ? "client" : "extension / engine" ) << " to initialize... Retrying in " << next << " seconds.");
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
			UF_MSG_ERROR("RUNTIME ERROR: " << e.what());
			break;
		} catch ( std::exception& e ) {
			UF_MSG_ERROR("EXCEPTION ERROR: " << e.what());
			throw e;
		} catch ( bool handled ) {
			if (!handled) UF_MSG_ERROR("UNHANDLED ERROR: " << "???");
		} catch ( ... ) {
			UF_MSG_ERROR("UNKNOWN ERROR: " << "???");
		}
	#endif
	}
	if ( !client::terminated ) {
		UF_MSG_INFO("Natural termination!");
		ext::terminate();
		client::terminate();
	}
	return 0;
}