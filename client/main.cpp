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

	{
		uf::Mesh mesh;
		mesh.bind<pod::Vertex_2F2F, uint16_t>();
		mesh.insertVertices<pod::Vertex_2F2F>({
			{ {-1.0f, 1.0f}, {0.0f, 1.0f}, },
			{ {-1.0f, -1.0f}, {0.0f, 0.0f}, },
			{ {1.0f, -1.0f}, {1.0f, 0.0f}, },
			{ {1.0f, 1.0f}, {1.0f, 1.0f}, }
		});
		mesh.insertIndices<uint16_t>({
			0, 1, 2, 2, 3, 0
		});
	/*
		mesh.bind<pod::Vertex_3F2F3F>();
		mesh.insertVertices<pod::Vertex_3F2F3F>({
			{{0.0f, 1.0f, 2.0f}, {3.0f, 4.0f}, {5.0f, 6.0f, 7.0f}},
			{{8.0f, 9.0f, 10.0f}, {11.0f, 12.0f}, {13.0f, 14.0f, 15.0f}},
			{{16.0f, 17.0f, 18.0f}, {19.0f, 20.0f}, {21.0f, 22.0f, 23.0f}},
		});
		mesh.insertIndices<uint32_t>({
			0, 1, 2
		});
	*/

		mesh.print();
	/*
		auto& buffer = mesh.buffers[0 <= mesh.vertex.interleaved && mesh.vertex.interleaved < mesh.buffers.size() ? mesh.vertex.interleaved :];
		uint8_t* pointer = (uint8_t*) buffer.data();
		while ( pointer < (uint8_t*) buffer.data() + buffer.size() ) {
			for ( auto& attribute : mesh.vertex.attributes ) {
				float* vector = pointer + attrribute.descriptor.offset;
				for ( auto i = 0; i < attribute.descriptor.components; ++i ) {
					UF_MSG_DEBUG( attribute.descriptor.name << ": " << vector[i] );
				}
				pointer += attribute.descriptor.size;
			}
		}
	*/
	}

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