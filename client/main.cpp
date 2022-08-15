#include "main.h"

#include <uf/utils/io/iostream.h>
#include <uf/utils/time/time.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/utils/window/payloads.h>

#include <uf/utils/memory/pool.h>
#include <uf/spec/renderer/universal.h>


#include <filesystem>
#include <signal.h>

namespace {
	namespace handlers {
		void exit() {
		#if UF_ENV_DREAMCAST
			arch_stk_trace(1);
		#endif
			if ( client::terminated ) return;
			UF_MSG_INFO("Termination via std::atexit()!");
			ext::ready = false;
			client::ready = false;
			client::terminated = true;
			ext::terminate();
			client::terminate();
		}

		void abrt( int sig ) {
			ext::ready = false;
			UF_MSG_ERROR("Abort detected");
		#if UF_ENV_DREAMCAST
			arch_stk_trace(1);
		#endif
		}

		void segv( int sig ) {
			ext::ready = false;
			UF_MSG_ERROR("Segfault detected");
		#if UF_ENV_DREAMCAST
			arch_stk_trace(1);
		#endif
		}
	}
}

bool hasDispatchedThread = false;
void renderProc() {
	while ( !client::ready || !ext::ready ) {
		ext::render();
		client::render();
	}
}

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
		//	UF_MSG_INFO("Waiting for " << ( client::ready ? "client" : "extension / engine" ) << " to initialize... Retrying in " << next << " seconds.");
			UF_MSG_INFO("Waiting for {} to initialize; retrying in {} seconds", ( client::ready ? "client" : "extension / engine" ), next);
			next *= 2;
		}
	}

//	auto& renderer = uf::thread::fetchWorker();
	auto& renderer = uf::thread::get("Render");
	
/*
	UF_MSG_DEBUG("?");
	std::thread dispatchedThread;
	UF_MSG_DEBUG("!");
*/
	while ( client::ready && ext::ready ) {
	#if UF_EXCEPTIONS
		try {	
	#endif
	/*
		client::tick();
		ext::tick();

		if ( !hasDispatchedThread ) {
			if ( uf::renderer::settings::experimental::dedicatedThread ) {
				hasDispatchedThread = true;
				dispatchedThread = std::thread(renderProc);
			} else {
				ext::render();
				client::render();
			}
		}
	*/
		#if 1
			if ( uf::renderer::settings::experimental::dedicatedThread ) {
				uf::thread::queue(renderer, [&]{
					ext::render();
					client::render();
				});
				
				client::tick();
				ext::tick();

				uf::thread::wait(renderer);
			} else
		#endif
			{
				client::tick();
				ext::tick();

				ext::render();
				client::render();
			}
	#if UF_EXCEPTIONS
		} catch ( std::runtime_error& e ) {
			UF_MSG_ERROR("RUNTIME ERROR: {}", e.what());
			abort();
		} catch ( std::exception& e ) {
			UF_MSG_ERROR("EXCEPTION ERROR: {}", e.what());
			abort();
		} catch ( bool handled ) {
			if (!handled) {
				UF_MSG_ERROR("UNHANDLED ERROR: {}", "???");
				abort();
			}
		} catch ( ... ) {
			UF_MSG_ERROR("UNKNOWN ERROR: {}", "???");
			abort();
		}
	#endif
	}

/*
	if ( hasDispatchedThread ) {
		dispatchedThread.join();
	}
*/

	if ( !client::terminated ) {
		client::terminated = true;
		UF_MSG_INFO("Natural termination!");
		ext::terminate();
		client::terminate();
	}
	return 0;
}