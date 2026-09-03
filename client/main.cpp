#include "main.h"
#include "client/headless.h"

#include <uf/utils/io/iostream.h>
#include <uf/utils/time/time.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/utils/window/payloads.h>

#include <uf/utils/memory/pool.h>
#include <uf/utils/singletons/pre_main.h>
#include <uf/spec/renderer/universal.h>


#include <filesystem>
#include <signal.h>
#include <cstdlib>
#if !UF_ENV_DREAMCAST
#include <thread>
#include <chrono>
#endif

namespace {
	bool killing = true;
	volatile sig_atomic_t signalExit = 0;
	namespace handlers {

		void term( int sig ) {
			signalExit = 1;
		}

		void exit() {
		#if UF_ENV_DREAMCAST
			arch_stk_trace(1);
		#endif

			std::ofstream output;
			output.open(uf::io::root+"/logs/crash.txt");
			for ( const auto& str : uf::iostream.getHistory() ) output << str << "\n";
			output.close();
			
			if ( client::terminated ) return;
			UF_MSG_INFO("Termination via std::atexit()!");
			
			client::ready = false;
			//ext::ready = false;
			uf::ready = false;
			
			client::terminated = true;
			
			ext::terminate();
			uf::terminate();
			client::terminate();
		}

		void abrt( int sig ) {
			UF_MSG_ERROR("Abort detected");
		#if UF_ENV_DREAMCAST
			arch_stk_trace(1);
			exit();
		#else
			if ( ::killing ) {
				std::_Exit(0);
			} else if ( !client::terminated ) {
				::killing = true;
				exit();
			}
		#endif
		}

		void segv( int sig ) {
			UF_MSG_ERROR("Segfault detected");
		#if UF_ENV_DREAMCAST
			arch_stk_trace(1);
			exit();
		#else
			if ( ::killing ) {
				std::_Exit(0);
			} else if ( !client::terminated ) {
				::killing = true;
				exit();
			}
		#endif
		}
	}
}

int main(int argc, char** argv){
	uf::StaticInitialization::runAll();
	for ( size_t i = 0; i < argc; ++i ) {
		char* c_str = argv[i];
		std::string string(argv[i]);
		// uf::arguments.emplace_back(string);
	}

	for ( int i = 1; i < argc; ++i ) if ( std::string( argv[i] ) == "--headless" ) uf::headless = true;
	if ( const char* env = getenv( "UF_HEADLESS" ) ) if ( env[0] == '1' ) uf::headless = true;
	for ( int i = 1; i < argc; ++i ) if ( std::string( argv[i] ) == "--io-socket" ) {
		uf::socketRequested = true;
		if ( i + 1 < argc && std::string( argv[i+1] ) != "--headless" && argv[i+1][0] != '-' ) uf::socketPath = argv[++i];
		else uf::socketPath = "engine.sock";
		break;
	}
	std::atexit(::handlers::exit);
	signal(SIGABRT, ::handlers::abrt);
	signal(SIGSEGV, ::handlers::segv);
	signal(SIGINT, ::handlers::term);
	signal(SIGTERM, ::handlers::term);

	client::initialize();
	uf::initialize();
	ext::initialize();

	// For Multithreaded initialization
	while ( !client::ready || !uf::ready ) {
		static uf::Timer<long long> timer(false);
		static double next = 1;
		if ( !timer.running() ) timer.start();

		if ( timer.elapsed().asDouble() >= next ) {
		//	UF_MSG_INFO("Waiting for " << ( client::ready ? "client" : "extension / engine" ) << " to initialize... Retrying in " << next << " seconds.");
			UF_MSG_INFO("Waiting for {} to initialize; retrying in {} seconds", ( client::ready ? "client" : "extension / engine" ), next);
			next *= 2;
		}
	}

	while ( client::ready && uf::ready && !signalExit ) {
		if ( uf::paused && uf::stepBudget == 0 ) {
			client::tick();
		#if !UF_ENV_DREAMCAST
			std::this_thread::sleep_for( std::chrono::milliseconds( 2 ) );
		#endif
			continue;
		}
		if ( uf::paused ) --uf::stepBudget;
		++uf::time::frame;
		
	#if UF_EXCEPTIONS
		try {	
	#endif
			if ( uf::renderer::settings::experimental::dedicatedThread /*&& !uf::renderer::states::rebuild*/ ) {
			//	auto& thread = uf::thread::fetchWorker();
				auto& thread = uf::thread::get("Render");
				uf::thread::queue(thread, [&]{
					ext::render();
					uf::render();
					client::render();
				});
				
				client::tick();
				ext::tick();
				uf::tick();

				uf::thread::wait( thread );
			} else {
				client::tick();
				uf::tick();
				ext::tick();

				ext::render();
				uf::render();
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
	client::headless::terminate();
	if ( !client::terminated ) {
		client::terminated = true;
		UF_MSG_INFO("Natural termination!");
		ext::terminate();
		uf::terminate();
		client::terminate();
	}
	return 0;
}
