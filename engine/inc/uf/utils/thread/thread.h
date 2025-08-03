#pragma once

#include <uf/config.h>
#include <uf/utils/time/time.h>
#include <uf/utils/math/math.h>

#include <mutex>
#include <thread>

#include <uf/utils/memory/vector.h>
#include <uf/utils/memory/string.h>
#include <uf/utils/memory/map.h>
#include <uf/utils/memory/unordered_map.h>
#include <uf/utils/memory/queue.h>

#include <functional>
#include <condition_variable>

namespace uf {
	namespace thread {
		extern UF_API uf::stl::string mainThreadName;
		extern UF_API uf::stl::string workerThreadName;
		extern UF_API uf::stl::string asyncThreadName;
	}
}

namespace pod {
	struct UF_API Thread {
		typedef std::function<void()> function_t;
		typedef uf::stl::queue<pod::Thread::function_t> queue_t;
		typedef uf::stl::vector<pod::Thread::function_t> container_t;

		uint uid;
		float limiter;
		uf::stl::string name;
		bool running, terminates;

		std::mutex* mutex;
		struct {
			std::condition_variable queued;
			std::condition_variable finished;
		} conditions;
		std::thread thread;

		pod::Thread::queue_t queue;
		pod::Thread::container_t container;

		uf::Timer<long long> timer;
		uint affinity = 0;

		struct UF_API Tasks {
			uf::stl::string name = uf::thread::workerThreadName;
			bool waits = true;

			pod::Thread::queue_t container;

			inline void add( const pod::Thread::function_t& fun ) { container.emplace(fun); }
			inline void emplace( const pod::Thread::function_t& fun ) { container.emplace(fun); }
			inline void queue( const pod::Thread::function_t& fun ) { container.emplace(fun); }
			inline void clear() { container = {}; }
		};
	};

}

namespace uf {
	namespace thread {
		extern UF_API float limiter;
		extern UF_API uint workers;
		extern UF_API std::thread::id mainThreadId;
		extern UF_API bool async;

	/* 	Easy to use async helper functions */
		pod::Thread& UF_API fetchWorker( const uf::stl::string& name = uf::thread::workerThreadName );
		pod::Thread::Tasks UF_API schedule( bool multithread, bool waits = true );
		pod::Thread::Tasks UF_API schedule( const uf::stl::string& name = uf::thread::workerThreadName, bool waits = true );
		uf::stl::vector<pod::Thread*> UF_API execute( pod::Thread::Tasks& tasks );
		void UF_API wait( uf::stl::vector<pod::Thread*>& );
		void UF_API wait( const uf::stl::vector<pod::Thread*>& );

	/* Acts on global threads */
		typedef uf::stl::vector<pod::Thread*> container_t;
		extern UF_API uf::thread::container_t threads;

		void UF_API terminate();

		pod::Thread& UF_API create( const uf::stl::string& = "", bool = true, bool = true );
		void UF_API destroy( pod::Thread& );
		bool UF_API has( uint );
		bool UF_API has( std::thread::id );
		bool UF_API has( const uf::stl::string& );
		pod::Thread& UF_API get( uint );
		pod::Thread& UF_API get( std::thread::id );
		pod::Thread& UF_API get( const uf::stl::string& );

		bool UF_API isMain();
		pod::Thread& UF_API currentThread();

	/* Acts on thread */
		void UF_API start( pod::Thread& );
		void UF_API quit( pod::Thread& );

		void UF_API tick( pod::Thread& );

		// schedules to worker thread
		void UF_API queue( const pod::Thread::function_t& fun );
		void UF_API queue( const pod::Thread::container_t& funs );
		// schedules to target thread
		void UF_API queue( pod::Thread& thread, const pod::Thread::function_t& fun );
		void UF_API add( pod::Thread& thread, const pod::Thread::function_t& fun );
		// schedules to named thread
		inline void queue( const uf::stl::string& name, const pod::Thread::function_t& fun ) { return uf::thread::queue( uf::thread::get(name), fun ); }
		inline void add( const uf::stl::string& name, const pod::Thread::function_t& fun ) { return uf::thread::add( uf::thread::get(name), fun ); }
		
		void UF_API process( pod::Thread& );

		void UF_API wait( pod::Thread& );

		const uf::stl::string& UF_API name( const pod::Thread& );
		std::thread::id UF_API id( const pod::Thread& );
		uint UF_API uid( const pod::Thread& );
		bool UF_API running( const pod::Thread& );
	}
}