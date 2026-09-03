#pragma once

#include <uf/config.h>
#include <uf/utils/time/time.h>
#include <uf/utils/math/math.h>

#include <uf/utils/memory/vector.h>
#include <uf/utils/memory/string.h>
#include <uf/utils/memory/map.h>
#include <uf/utils/memory/unordered_map.h>
#include <uf/utils/memory/queue.h>

#include <functional>

#if UF_ENV_DREAMCAST
	#define UF_USE_KOS_THREAD 1
#endif

#if UF_USE_KOS_THREAD
	#include <kos/thread.h>
	#include <kos/mutex.h>
	#include <kos/cond.h>
	
	#define UF_THREAD_METRICS 0

	#include "kos.h"
#else
	#include <thread>
	#include <mutex>
	#include <condition_variable>
	#include <atomic>
	#define UF_THREAD_METRICS 1

	namespace uf {
		namespace stl {
			template <typename T>
			using atomic = std::atomic<T>;

			using atomic_bool = std::atomic<bool>;
			using atomic_int = std::atomic<int>;
			using atomic_uint = std::atomic<unsigned int>;
		}
	}
#endif

namespace uf {
	namespace thread {
		inline constexpr const char* mainThreadName = "Main";
		inline constexpr const char* workerThreadName = "Worker";
		inline constexpr const char* asyncThreadName = "Async";
	}
}

namespace pod {
	struct UF_API Thread {
		typedef uint16_t id_t;
		typedef std::function<void()> function_t;
		typedef uf::stl::vector<pod::Thread::function_t> container_t;

		struct UF_API Tasks {
			struct UF_API Tracker {
			#if UF_USE_KOS_THREAD
				volatile uint32_t pending{0};
				mutex_t mutex;
				condvar_t cv;

				Tracker() {
					mutex_init(&mutex, MUTEX_TYPE_NORMAL);
					cond_init(&cv);
				}
				~Tracker() {
					mutex_destroy(&mutex);
					cond_destroy(&cv);
				}
			#else
				uf::stl::atomic<uint32_t> pending{0};
				std::mutex mutex;
				std::condition_variable cv;
			#endif
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

	#if UF_USE_KOS_THREAD
		mutex_t mutex;
		struct {
			condvar_t queued;
			condvar_t finished;
		} conditions;
		volatile int pending{0};
		volatile bool running;

		kthread_t* thread = nullptr;
	#else
		std::mutex mutex;
		struct {
			std::condition_variable queued;
			std::condition_variable finished;
		} conditions;
		uf::stl::atomic<int> pending{0};
		uf::stl::atomic<bool> running;

		std::thread thread;
	#endif

		pod::Thread::container_t queue;
		pod::Thread::container_t container;

		uint32_t affinity = 0;
	#if UF_THREAD_METRICS
		struct Performance {
			typedef std::tuple<float, float, float, uint32_t> tuple_t;

			uf::stl::atomic<float> activeTimeMs{0.0f};
			uf::stl::atomic<float> idleTimeMs{0.0f};
			uf::stl::atomic<float> totalFrameTimeMs{0.0f};
			uf::stl::atomic<uint32_t> tasksProcessed{0};

			inline tuple_t collect() {
				return std::make_tuple( activeTimeMs.load(), idleTimeMs.load(), totalFrameTimeMs.load(), tasksProcessed.load() );
			}
		} metrics;
	#endif
	};
}

namespace uf {
	namespace thread {
	#if UF_USE_KOS_THREAD
		typedef kthread_t* id_t;
	#else
		typedef std::thread::id id_t;
	#endif
		extern UF_API float limiter;
		extern UF_API uint32_t workers;
		extern UF_API uf::thread::id_t mainThreadId;
		extern UF_API bool async;

	/* 	Easy to use async helper functions */
		pod::Thread& UF_API fetchWorker( const uf::stl::string& name = uf::thread::workerThreadName );
		inline pod::Thread& UF_API fetchWorker( const char* s ) { return fetchWorker( uf::stl::string( s ) ); } // ugh
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
		bool UF_API has( uf::thread::id_t );
		bool UF_API has( const uf::stl::string& );
		pod::Thread& UF_API get( pod::Thread::id_t );
		pod::Thread& UF_API get( uf::thread::id_t );
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

		uf::stl::string UF_API name( uf::thread::id_t id );
		const uf::stl::string& UF_API name( const pod::Thread& );
		uf::thread::id_t UF_API id( const pod::Thread& );
		pod::Thread::id_t UF_API uid( const pod::Thread& );
		bool UF_API running( const pod::Thread& );

		uf::thread::id_t UF_API current_id();

	#if UF_THREAD_METRICS
		uf::stl::unordered_map<uf::stl::string, pod::Thread::Performance::tuple_t> collectStats();
	#endif
	}
}