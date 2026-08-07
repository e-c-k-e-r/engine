#include <uf/utils/thread/thread.h>
#include <uf/utils/io/iostream.h>
#include <future>

uf::thread::container_t uf::thread::threads;
float uf::thread::limiter = 1.0f / 120.0f;
uint32_t uf::thread::workers = 1;
std::thread::id uf::thread::mainThreadId = std::this_thread::get_id();
bool uf::thread::async = false;
uf::stl::string uf::thread::mainThreadName = "Main";
uf::stl::string uf::thread::workerThreadName = "Worker";
uf::stl::string uf::thread::asyncThreadName = "Async";

namespace {
	std::mutex mutex;
}

#define UF_THREAD_ANNOUNCE(...) UF_MSG_DEBUG(__VA_ARGS__)

void uf::thread::start( pod::Thread& thread ) { if ( thread.running ) return;
	thread.thread = std::thread( uf::thread::tick, std::ref(thread) );
	thread.running = true;
}
void uf::thread::quit( pod::Thread& thread ) { if ( !thread.running ) return;
	{
		std::lock_guard<std::mutex> lock(thread.mutex);
		thread.running = false;
	}
	thread.conditions.queued.notify_all();
	if ( thread.thread.joinable() ) thread.thread.join();
}
void uf::thread::tick( pod::Thread& thread ) {
#if UF_ENV_WINDOWS
	bool res = SetThreadAffinityMask(GetCurrentThread(), (1ull << thread.affinity));
#endif
	thread.timer.start();
	
	while ( thread.running ) {
		uf::thread::process( thread );
	}
}

pod::Thread& uf::thread::fetchWorker( const uf::stl::string& name ) {
	static std::atomic<int> current = 0;
	int limit = uf::thread::workers;
	int tries = 0;

	while ( tries++ < limit ) {
		int val = current.fetch_add(1, std::memory_order_relaxed);
		auto workerName = FMT_FORMAT("{} {}", name, val % limit);
		auto& pod = uf::thread::get( workerName );
		if ( std::this_thread::get_id() == pod.thread.get_id() ) continue; // do not queue to current thread
		return pod;
	}
	UF_EXCEPTION("cannot find free worker");
}
pod::Thread::Tasks uf::thread::schedule( bool async, bool wait ) {
	return schedule( async ? uf::thread::workerThreadName : uf::thread::mainThreadName, wait );
}
pod::Thread::Tasks uf::thread::schedule( const uf::stl::string& name, bool wait ) {
	pod::Thread::Tasks tasks = {
		.name = name,
		.waits = wait,
	};

	return tasks;
}
std::shared_ptr<pod::Thread::Tasks::Tracker> uf::thread::execute( pod::Thread::Tasks& tasks ) {
	auto tracker = std::make_shared<pod::Thread::Tasks::Tracker>();
	if ( tasks.container.empty() ) return tracker;

	tracker->pending.store( tasks.container.size(), std::memory_order_relaxed );

	if ( tasks.name == uf::thread::mainThreadName ) {
	#if UF_THREAD_METRICS
		auto& thread = uf::thread::get( uf::thread::mainThreadName );
		uint32_t tasksThisFrame = 0;
		for ( auto& task : tasks.container ) {
			task();
			++tasksThisFrame;
		}
		thread.metrics.tasksProcessed.store(tasksThisFrame, std::memory_order_relaxed);
	#else
		for ( auto& task : tasks.container ) task();
	#endif
		tasks.container.clear();
		tracker->pending.store(0, std::memory_order_release);
	} else {
		for ( auto& task : tasks.container ) {
			auto& worker = uf::thread::fetchWorker( tasks.name );

			uf::thread::queue( worker, [task, tracker]() {
				struct Decrementer {
					std::shared_ptr<pod::Thread::Tasks::Tracker> t;
					~Decrementer() {
						if ( t->pending.fetch_sub(1, std::memory_order_release) == 1 ) {
							std::lock_guard<std::mutex> lock(t->mutex);
							t->cv.notify_all();
						}
					}
				} dec{ tracker };

				task();
			});
		}
		tasks.container.clear();
		if ( tasks.waits ) uf::thread::wait( tracker );
	}
	return tracker;
}
void uf::thread::wait( std::shared_ptr<pod::Thread::Tasks::Tracker> tracker ) {
	if ( !tracker ) return;
	std::unique_lock<std::mutex> lock(tracker->mutex);
	tracker->cv.wait(lock, [&]{ return tracker->pending.load(std::memory_order_acquire) == 0; });
}

void uf::thread::add( pod::Thread& thread, const pod::Thread::function_t& function ) {
	std::lock_guard<std::mutex> lock(thread.mutex);
	thread.container.emplace_back( function );
}
void uf::thread::queue( const pod::Thread::container_t& functions ) {
	for ( auto& function : functions )
		uf::thread::queue( uf::thread::fetchWorker(), function ); // dispatch tasks across all worker threads
}
void uf::thread::queue( const pod::Thread::function_t& function ) {
	return uf::thread::queue( uf::thread::fetchWorker(), function );
}
void uf::thread::queue( pod::Thread& thread, const pod::Thread::function_t& function ) {
	{
		std::lock_guard<std::mutex> lock(thread.mutex);
		thread.queue.emplace_back( function );
		thread.pending.fetch_add(1);
	}
	thread.conditions.queued.notify_one();
}
void uf::thread::process( pod::Thread& thread ) { if ( !uf::thread::has(thread.name) ) return; // ops
	STATIC_THREAD_LOCAL(pod::Thread::container_t, local_queue);
	STATIC_THREAD_LOCAL(pod::Thread::container_t, local_container);

	// hardcoded cringe
	if ( thread.name == uf::thread::mainThreadName ) {
		if ( thread.queue.empty() ) return;
		std::unique_lock<std::mutex> lock(thread.mutex);
		std::swap( local_queue, thread.queue );
		for ( auto& function : local_queue ) function();
		return;
	}

#if UF_THREAD_METRICS
	uint32_t tasksThisFrame = 0;
	auto frameStart = std::chrono::high_resolution_clock::now();
	auto idleStart = std::chrono::high_resolution_clock::now();
#endif

	// wait for work
	{
		std::unique_lock<std::mutex> lock(thread.mutex);
		if ( thread.limiter > 0 ) {
			long long sleep_ms = (thread.limiter * 1000.0f) - thread.timer.elapsed().asMilliseconds();
			if ( sleep_ms > 0 ) {
				thread.conditions.queued.wait_for(lock, std::chrono::milliseconds(sleep_ms), [&]{
					return !thread.queue.empty() || !thread.running;
				});
			}
			thread.timer.reset();
		} else {
			thread.conditions.queued.wait(lock, [&]{
				return (!thread.container.empty() || !thread.queue.empty()) || !thread.running;
			});
		}

		if ( !thread.running ) return;
		std::swap( local_queue, thread.queue );
	}

	// update stats
#if UF_THREAD_METRICS
	{	
		std::chrono::duration<float, std::milli> idleTime = std::chrono::high_resolution_clock::now() - idleStart;
		thread.metrics.idleTimeMs.store(idleTime.count(), std::memory_order_relaxed);
	}
	auto activeStart = std::chrono::high_resolution_clock::now();
#endif

	// iterate through queued work
	for ( auto& function : local_queue ) {
	#if UF_EXCEPTIONS
		try {
	#endif
			function();
	#if UF_EXCEPTIONS
		} catch ( std::exception& e ) {
			UF_MSG_ERROR("Thread {} (UID: {}) caught exception: {}", thread.name, thread.uid, e.what());
		}
	#endif
		if ( thread.pending.fetch_sub(1) == 1 ) {
			thread.conditions.finished.notify_all();
		}
	#if UF_THREAD_METRICS
		++tasksThisFrame;
	#endif
	}

	// buffer persistent work
	{
		std::lock_guard<std::mutex> lock(thread.mutex);
		local_container = thread.container;
	}

	// iterate through persistent work
	for ( auto& function : local_container ) {
	#if UF_EXCEPTIONS
		try {
	#endif
			function();
	#if UF_EXCEPTIONS
		} catch ( std::exception& e ) {
			UF_MSG_ERROR("Thread {} (UID: {}) caught exception: {}", thread.name, thread.uid, e.what());
		}
	#endif
	#if UF_THREAD_METRICS
		++tasksThisFrame;
	#endif
	}

	{
		std::lock_guard<std::mutex> lock(thread.mutex);
		thread.conditions.finished.notify_all();
	}

	// update metrics
#if UF_THREAD_METRICS
	{
		std::chrono::duration<float, std::milli> activeTime = std::chrono::high_resolution_clock::now() - activeStart;
		std::chrono::duration<float, std::milli> frameTime = std::chrono::high_resolution_clock::now() - frameStart;

		thread.metrics.activeTimeMs.store(activeTime.count(), std::memory_order_relaxed);
		thread.metrics.totalFrameTimeMs.store(frameTime.count(), std::memory_order_relaxed);
		thread.metrics.tasksProcessed.store(tasksThisFrame, std::memory_order_relaxed);
	}
#endif
}
void uf::thread::wait( pod::Thread& thread ) {
	std::unique_lock<std::mutex> lock(thread.mutex);
	thread.conditions.finished.wait(lock, [&]{ return thread.pending.load() == 0; });
//	while ( thread.pending.load() > 0 ) std::this_thread::yield();
}

uf::stl::string uf::thread::name( std::thread::id id ) {
	if ( id == uf::thread::mainThreadId ) return uf::thread::mainThreadName;
	for ( auto& [ name, thread ] : uf::thread::threads ) {
		if ( thread->thread.get_id() == id ) return name;
	}
	return "?";
}
const uf::stl::string& uf::thread::name( const pod::Thread& thread ) {
	return thread.name;
}
std::thread::id uf::thread::id( const pod::Thread& thread ) {
	if ( thread.name == uf::thread::mainThreadName ) return uf::thread::mainThreadId;
	return thread.thread.get_id();
}
pod::Thread::id_t uf::thread::uid( const pod::Thread& thread ) {
	return thread.uid;
}
bool uf::thread::running( const pod::Thread& thread ) {
	return thread.running;
}

void uf::thread::terminate() {
	uf::thread::container_t local_threads;
	{
		std::unique_lock<std::mutex> lock(::mutex);
		std::swap( local_threads, uf::thread::threads );
	}

	for ( auto& [ key, thread ] : local_threads ) {
		uf::thread::quit( *thread );
		delete thread;
	}
}
pod::Thread& uf::thread::create( const uf::stl::string& name, bool start, bool locks ) {
	if ( name == uf::thread::mainThreadName ) start = false;

	pod::Thread* pointer = NULL;
	{
		std::unique_lock<std::mutex> lock(::mutex);
		uf::thread::threads[name] = (pointer = new pod::Thread);
	}
	pod::Thread& thread = *pointer;

	static auto limit = uf::thread::workers;
	static pod::Thread::id_t uids = 0;
	static pod::Thread::id_t threads = std::thread::hardware_concurrency();

	thread.name = name;
	thread.uid = uids++;
	thread.running = false;
	thread.limiter = uf::thread::limiter;
	thread.affinity = (thread.uid % limit) + 1;

	if ( start ) uf::thread::start( thread );

	return thread;
}
void uf::thread::destroy( pod::Thread& thread ) {
	auto& name = thread.name;
	if ( !uf::thread::has( name ) ) return; // oops

	uf::thread::quit( thread );
	{
		std::unique_lock<std::mutex> lock(::mutex);
		uf::thread::threads.erase( name );
	}
	delete &thread; // shortcut, references from threads should always be from the map anyways
}
bool uf::thread::has( const uf::stl::string& name ) {
	std::unique_lock<std::mutex> lock(::mutex);
	return uf::thread::threads.count( name ) > 0;
}

pod::Thread& uf::thread::get( const uf::stl::string& name ) {
	if ( !uf::thread::has(name) ) return uf::thread::create(name);
	return *uf::thread::threads[name];
}

#if UF_THREAD_METRICS
uf::stl::unordered_map<uf::stl::string, pod::Thread::Performance::tuple_t> uf::thread::collectStats() {
	uf::stl::unordered_map<uf::stl::string, pod::Thread::Performance::tuple_t> stats;
	// possible mutex issue
	for ( auto& [ key, thread ] : uf::thread::threads ) stats[thread->name] = thread->metrics.collect();
	return stats;
}
#endif