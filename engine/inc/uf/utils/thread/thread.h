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

#define UF_THREAD_METRICS 1

namespace uf {
	namespace thread {
		extern UF_API uf::stl::string mainThreadName;
		extern UF_API uf::stl::string workerThreadName;
		extern UF_API uf::stl::string asyncThreadName;
	}
}

namespace pod {
	struct UF_API Thread {
		typedef uint16_t id_t;
		typedef std::function<void()> function_t;
		typedef uf::stl::vector<pod::Thread::function_t> container_t;
		
		struct UF_API Tasks {
			struct UF_API Tracker {
				std::atomic<uint32_t> pending{0};
				std::mutex mutex;
				std::condition_variable cv;
			};

			uf::stl::string name = uf::thread::workerThreadName;
			bool waits = true;

			pod::Thread::container_t container;

			inline void add( const pod::Thread::function_t& fun ) { container.emplace_back(fun); }
			inline void emplace( const pod::Thread::function_t& fun ) { container.emplace_back(fun); }
			inline void queue( const pod::Thread::function_t& fun ) { container.emplace_back(fun); }
			inline bool empty() { return container.empty(); }
			inline void clear() { container = {}; }
		};

		pod::Thread::id_t uid;
		uf::stl::string name;

		uf::Timer<long long> timer;
		float limiter;
		bool terminates;

		std::mutex mutex;
		struct {
			std::condition_variable queued;
			std::condition_variable finished;
		} conditions;
		std::atomic<int> pending{0};
		std::atomic<bool> running;

		std::thread thread;

		pod::Thread::container_t queue;
		pod::Thread::container_t container;

		uint32_t affinity = 0;
	#if UF_THREAD_METRICS
		struct Performance {
			typedef std::tuple<float, float, float, uint32_t> tuple_t;

			std::atomic<float> activeTimeMs{0.0f};
			std::atomic<float> idleTimeMs{0.0f};
			std::atomic<float> totalFrameTimeMs{0.0f};
			std::atomic<uint32_t> tasksProcessed{0};

			inline tuple_t collect() {
				return std::make_tuple( activeTimeMs.load(), idleTimeMs.load(), totalFrameTimeMs.load(), tasksProcessed.load() );
			}
		} metrics;
	#endif
	};
}

namespace uf {
	namespace thread {
		extern UF_API float limiter;
		extern UF_API uint32_t workers;
		extern UF_API std::thread::id mainThreadId;
		extern UF_API bool async;

	/* 	Easy to use async helper functions */
		pod::Thread& UF_API fetchWorker( const uf::stl::string& name = uf::thread::workerThreadName );
		pod::Thread::Tasks UF_API schedule( bool multithread, bool waits = true );
		pod::Thread::Tasks UF_API schedule( const uf::stl::string& name = uf::thread::workerThreadName, bool waits = true );
		std::shared_ptr<pod::Thread::Tasks::Tracker> UF_API execute( pod::Thread::Tasks& tasks );
		void UF_API wait( std::shared_ptr<pod::Thread::Tasks::Tracker> );

	/* Acts on global threads */
		typedef uf::stl::unordered_map<uf::stl::string, pod::Thread*> container_t;
		extern UF_API uf::thread::container_t threads;

		void UF_API terminate();

		pod::Thread& UF_API create( const uf::stl::string& = "", bool = true, bool = true );
		void UF_API destroy( pod::Thread& );
	
		bool UF_API has( const uf::stl::string& );
		pod::Thread& UF_API get( const uf::stl::string& );
	/*
		bool UF_API has( pod::Thread::id_t );
		bool UF_API has( std::thread::id );
		bool UF_API has( const uf::stl::string& );
		pod::Thread& UF_API get( pod::Thread::id_t );
		pod::Thread& UF_API get( std::thread::id );
		pod::Thread& UF_API get( const uf::stl::string& );

		bool UF_API isMain();
		pod::Thread& UF_API currentThread();
	*/

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

	/*
		template<typename F>
		inline void queue( const uf::stl::string& name, const F& fun ) { return uf::thread::queue( uf::thread::get(name), [=](){ fun(); } ); }
	*/
		void UF_API process( pod::Thread& );

		void UF_API wait( pod::Thread& );

		uf::stl::string UF_API name( std::thread::id id );
		const uf::stl::string& UF_API name( const pod::Thread& );
		std::thread::id UF_API id( const pod::Thread& );
		pod::Thread::id_t UF_API uid( const pod::Thread& );
		bool UF_API running( const pod::Thread& );

	#if UF_THREAD_METRICS
		uf::stl::unordered_map<uf::stl::string, pod::Thread::Performance::tuple_t> collectStats();
	#endif
	}
}