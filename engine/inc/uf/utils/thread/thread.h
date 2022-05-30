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
		std::condition_variable condition;
		std::thread thread;

		pod::Thread::queue_t temps;
		pod::Thread::container_t consts;

		uf::Timer<long long> timer;
		uint affinity = 0;
	};
}

namespace uf {
	namespace thread {
		extern UF_API float limiter;
		extern UF_API uint workers;
		extern UF_API std::thread::id mainThreadId;
		extern UF_API bool async;

	/* Acts on thread */
		void UF_API start( pod::Thread& );
		void UF_API quit( pod::Thread& );

		void UF_API tick( pod::Thread& );
	//	void UF_API tick( pod::Thread&, const std::function<void()>& = NULL );

		pod::Thread& UF_API fetchWorker( const uf::stl::string& name = "Aux" );
		void UF_API batchWorker( const pod::Thread::function_t&, const uf::stl::string& name = "Aux" );
		void UF_API batchWorkers( const uf::stl::vector<pod::Thread::function_t>&, bool = true, const uf::stl::string& name = "Aux" );
		void UF_API batchWorkers_Async( const uf::stl::vector<pod::Thread::function_t>&, bool = true, const uf::stl::string& name = "Aux" );
		void UF_API add( pod::Thread&, const pod::Thread::function_t&, bool = false );
		void UF_API process( pod::Thread& );

		void UF_API wait( pod::Thread& );

		const uf::stl::string& UF_API name( const pod::Thread& );
		uint UF_API uid( const pod::Thread& );
		bool UF_API running( const pod::Thread& );
	/* Acts on global threads */
		typedef uf::stl::vector<pod::Thread*> container_t;
		extern UF_API uf::thread::container_t threads;
		

		void UF_API terminate();

		pod::Thread& UF_API create( const uf::stl::string& = "", bool = true, bool = true );
		void UF_API destroy( pod::Thread& );
		bool UF_API has( uint );
		bool UF_API has( const uf::stl::string& );
		pod::Thread& UF_API get( uint );
		pod::Thread& UF_API get( const uf::stl::string& );

		bool UF_API isMain();
	}
}

/*
namespace uf {
	class UF_API Thread {
	public:
		typedef std::function<int()> function_t;
		typedef std::queue<uf::Thread::function_t> queue_t;
		typedef uf::stl::vector<uf::Thread::function_t> container_t;

		enum {
			TEMP = 0,
			CONSTANT = 1,
		};
	protected:
		uint m_uid;
		uint m_mode;
		bool m_running;
		bool m_shouldLock;
		bool m_terminateOnEmpty;
		std::mutex m_mutex;
		uf::stl::string m_name;
		std::thread m_thread;
		uf::Thread::queue_t m_temps;
		uf::Thread::container_t m_consts;
	public:
		static uint fps;
		static uint thread_count;
		Thread( const uf::stl::string& = "", bool = true, uint = 0 );
		virtual ~Thread();

		void start();
		virtual void tick(const std::function<void()>& = NULL);
		void add( const uf::Thread::function_t&, uint = 0 );
		void process();
		void quit();

		uf::stl::string toString() const;
		const uf::stl::string& getName() const;
		uint getUid() const;
		const bool& isRunning() const;
	};
	namespace thread {
		typedef uf::stl::unordered_map<uf::stl::string, uf::Thread*> container_t;
		extern UF_API uf::thread::container_t threads;

		void UF_API add( uf::Thread* );
		uf::Thread& UF_API get( const uf::stl::string& );
		bool UF_API add( const uf::stl::string&, const uf::Thread::function_t&, uint = 0 );
		bool UF_API quit( const uf::stl::string& );
	};
}
*/