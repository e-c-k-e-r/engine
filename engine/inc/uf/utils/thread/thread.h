#pragma once

#include <uf/config.h>
#include <uf/utils/time/time.h>
#include <uf/utils/math/math.h>

#include <queue>
#include <mutex>
#include <thread>
#include <vector>
#include <functional>

namespace pod {
	struct UF_API Thread {
		typedef std::function<int()> function_t;
		typedef std::queue<pod::Thread::function_t> queue_t;
		typedef std::vector<pod::Thread::function_t> container_t;

		uint uid;
		double limiter;
		std::string name;
		bool running, terminates;

		std::mutex* mutex;
		std::thread thread;

		pod::Thread::queue_t temps;
		pod::Thread::container_t consts;

		uf::Timer<long long> timer;
		uint affinity = 0;
	};
}

namespace uf {
	namespace thread {
		extern UF_API double limiter;
		extern UF_API uint workers;

	/* Acts on thread */
		void UF_API start( pod::Thread& );
		void UF_API quit( pod::Thread& );

		void UF_API tick( pod::Thread& );
	//	void UF_API tick( pod::Thread&, const std::function<void()>& = NULL );

		pod::Thread& UF_API fetchWorker( const std::string& name = "Aux" );
		void UF_API add( pod::Thread&, const pod::Thread::function_t&, bool = false );
		void UF_API process( pod::Thread& );

		void UF_API wait( pod::Thread& );

		const std::string& UF_API name( const pod::Thread& );
		uint UF_API uid( const pod::Thread& );
		bool UF_API running( const pod::Thread& );
	/* Acts on global threads */
		typedef std::vector<pod::Thread*> container_t;
		extern UF_API uf::thread::container_t threads;
		

		void UF_API terminate();

		pod::Thread& UF_API create( const std::string& = "", bool = true, bool = true );
		void UF_API destroy( pod::Thread& );
		bool UF_API has( uint );
		bool UF_API has( const std::string& );
		pod::Thread& UF_API get( uint );
		pod::Thread& UF_API get( const std::string& );
	}
}

/*
namespace uf {
	class UF_API Thread {
	public:
		typedef std::function<int()> function_t;
		typedef std::queue<uf::Thread::function_t> queue_t;
		typedef std::vector<uf::Thread::function_t> container_t;

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
		std::string m_name;
		std::thread m_thread;
		uf::Thread::queue_t m_temps;
		uf::Thread::container_t m_consts;
	public:
		static uint fps;
		static uint thread_count;
		Thread( const std::string& = "", bool = true, uint = 0 );
		virtual ~Thread();

		void start();
		virtual void tick(const std::function<void()>& = NULL);
		void add( const uf::Thread::function_t&, uint = 0 );
		void process();
		void quit();

		std::string toString() const;
		const std::string& getName() const;
		uint getUid() const;
		const bool& isRunning() const;
	};
	namespace thread {
		typedef std::unordered_map<std::string, uf::Thread*> container_t;
		extern UF_API uf::thread::container_t threads;

		void UF_API add( uf::Thread* );
		uf::Thread& UF_API get( const std::string& );
		bool UF_API add( const std::string&, const uf::Thread::function_t&, uint = 0 );
		bool UF_API quit( const std::string& );
	};
}
*/