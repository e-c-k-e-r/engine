#include <uf/utils/thread/thread.h>
#include <uf/utils/io/iostream.h>
#include <iostream>
#include <future>

uf::thread::container_t uf::thread::threads;
float uf::thread::limiter = 1.0f / 120.0f;
uint uf::thread::workers = 1;
std::thread::id uf::thread::mainThreadId = std::this_thread::get_id();
bool uf::thread::async = false;
uf::stl::string uf::thread::mainThreadName = "Main";
uf::stl::string uf::thread::workerThreadName = "Worker";
uf::stl::string uf::thread::asyncThreadName = "Async";

#define UF_THREAD_ANNOUNCE(...) UF_MSG_DEBUG(__VA_ARGS__)

void uf::thread::start( pod::Thread& thread ) { if ( thread.running ) return;
	thread.thread = std::thread( uf::thread::tick, std::ref(thread) );
	thread.running = true;
}
void uf::thread::quit( pod::Thread& thread ) { if ( !thread.running ) return;
	thread.running = false;
	thread.conditions.queued.notify_one();

	bool locked = false;
//	if ( thread.mutex != NULL ) locked = thread.mutex->try_lock();
	if ( thread.thread.joinable() ) thread.thread.join();
//	if ( thread.mutex != NULL ) thread.mutex->unlock();

}
void uf::thread::tick( pod::Thread& thread ) {
#if !UF_ENV_DREAMCAST
	bool res = SetThreadAffinityMask(GetCurrentThread(), (1u << thread.affinity));
//	if ( !res ) UF_THREAD_ANNOUNCE("Failed to set affinity of Thread #" << thread.uid << " (" << thread.name << " on core " << pthread_self() << "/" << thread.affinity << ")");
#endif
//	UF_THREAD_ANNOUNCE("Starting Thread #" << thread.uid << " (" << thread.name << " on core " << thread.affinity << ")" << (thread.limiter ? " (Limiter: " + std::to_string(1.0f / thread.limiter) + " FPS)" : ""));
	thread.timer.start();
	
	while ( thread.running ) {
		std::unique_lock<std::mutex> lock(*thread.mutex);
		thread.conditions.queued.wait(lock, [&]{
			return (!thread.container.empty() || !thread.queue.empty()) || !thread.running;
		});

		uf::thread::process( thread );

		if ( thread.limiter > 0 ) {
			long long sleep = (thread.limiter * 1000) - thread.timer.elapsed().asMilliseconds();
			if ( sleep > 0 ) {
			//	UF_THREAD_ANNOUNCE("Thread #" << thread.uid << " (" << thread.name << " on core " << thread.affinity << ") will sleep for " << sleep << "ms");
				std::this_thread::sleep_for(std::chrono::milliseconds(sleep));
			}
			thread.timer.reset();
		}
	}
}

pod::Thread& uf::thread::fetchWorker( const uf::stl::string& name ) {
	static int current = 0;
	static int limit = uf::thread::workers;
	int tries = 0;

	while ( tries++ < limit ) {
		if ( ++current >= limit ) current = 0;
		auto& pod = uf::thread::get(name + " " + std::to_string(current));
		if ( std::this_thread::get_id() == pod.thread.get_id() ) continue;
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
uf::stl::vector<pod::Thread*> uf::thread::execute( pod::Thread::Tasks& tasks ) {
	uf::stl::vector<pod::Thread*> workers;
	if ( tasks.container.empty() ) return workers;

	if ( tasks.name == uf::thread::mainThreadName ) {
		while ( !tasks.container.empty() ) {
			auto& task = tasks.container.front();
			task();
			tasks.container.pop();
		}
	} else {
		while ( !tasks.container.empty() ) {
			auto task = tasks.container.front();
			auto& worker = uf::thread::fetchWorker( tasks.name );
			uf::thread::queue( worker, task );
			workers.emplace_back(&worker);
			tasks.container.pop();
		}
		if ( tasks.waits ) uf::thread::wait( workers );
	}
	return workers;
}
void uf::thread::wait( uf::stl::vector<pod::Thread*>& workers ) {
	for ( auto& worker : workers ) uf::thread::wait( *worker );
	workers.clear();
}
void uf::thread::wait( const uf::stl::vector<pod::Thread*>& workers ) {
	for ( auto& worker : workers ) uf::thread::wait( *worker );
}
/*
void uf::thread::batchWorker( const pod::Thread::function_t& function, const uf::stl::string& name ) {
	return batchWorkers( { function }, false, name );
}
void uf::thread::batchWorkers( const uf::stl::vector<pod::Thread::function_t>& functions, bool wait, const uf::stl::string& name ) {
	uf::stl::vector<pod::Thread*> workers;
	for ( auto& function : functions ) {
		auto& worker = uf::thread::fetchWorker( name );
		workers.emplace_back(&worker);
		uf::thread::queue( worker, function );
	}
	if ( wait ) for ( auto& worker : workers ) uf::thread::wait( *worker );
}
void uf::thread::batchWorkers_Async( const uf::stl::vector<pod::Thread::function_t>& functions, bool wait, const uf::stl::string& name ) {
//	if ( uf::thread::async )
	uf::stl::vector<std::future<void>> futures;
	futures.reserve(functions.size());
	for ( auto& function : functions ) futures.emplace_back(std::async( std::launch::async, function ));
	if ( wait ) for ( auto& future : futures ) future.wait();
	return;
}
*/
/*
void uf::thread::add( pod::Thread& thread, bool queued, const pod::Thread::function_t& function ) {
	if ( thread.mutex != NULL ) thread.mutex->lock();
	queue ? thread.queue.push( function ) : thread.container.push_back( function );
	if ( thread.mutex != NULL ) thread.mutex->unlock();
}
*/
void uf::thread::add( pod::Thread& thread, const pod::Thread::function_t& function ) {
	if ( thread.mutex != NULL ) thread.mutex->lock();
	thread.container.emplace_back( function );
	if ( thread.mutex != NULL ) thread.mutex->unlock();
}
void uf::thread::queue( const pod::Thread::container_t& functions ) {
	for ( auto& function : functions )
		uf::thread::queue( uf::thread::fetchWorker(), function );
}
void uf::thread::queue( const pod::Thread::function_t& function ) {
	return uf::thread::queue( uf::thread::fetchWorker(), function );
}
void uf::thread::queue( pod::Thread& thread, const pod::Thread::function_t& function ) {
	if ( thread.mutex != NULL ) thread.mutex->lock();
	thread.queue.emplace( function );
	thread.conditions.queued.notify_one();
	if ( thread.mutex != NULL ) thread.mutex->unlock();
}
void uf::thread::process( pod::Thread& thread ) { if ( !uf::thread::has(uf::thread::uid(thread)) )return; //ops
	while ( !thread.queue.empty() ) {
		auto& function = thread.queue.front();
		if ( function )
	#if UF_EXCEPTIONS
		try {
	#endif
			function();
	#if UF_EXCEPTIONS
		} catch ( std::exception& e ) {
			UF_MSG_ERROR("Thread {} (UID: {}) caught exception: {}", thread.name, thread.uid, e.what());
		}
	#endif
		thread.queue.pop();
	}
	for ( auto function : thread.container ) {
		if ( function )
	#if UF_EXCEPTIONS
		try {
	#endif
			function();
	#if UF_EXCEPTIONS
		} catch ( std::exception& e ) {
			UF_MSG_ERROR("Thread {} (UID: {}) caught exception: {}", thread.name, thread.uid, e.what());
		}
	#endif
	}
	thread.conditions.finished.notify_one();
}
void uf::thread::wait( pod::Thread& thread ) {
	if ( thread.mutex != NULL ) {
		std::unique_lock<std::mutex> lock(*thread.mutex);
		thread.conditions.finished.wait(lock, [&]{return thread.queue.empty();});
    	return;
    }
	while ( !thread.queue.empty() );
}

const uf::stl::string& uf::thread::name( const pod::Thread& thread ) {
	return thread.name;
}
std::thread::id uf::thread::id( const pod::Thread& thread ) {
	if ( thread.name == uf::thread::mainThreadName ) return uf::thread::mainThreadId;
	return thread.thread.get_id();
}
uint uf::thread::uid( const pod::Thread& thread ) {
	return thread.uid;
}
bool uf::thread::running( const pod::Thread& thread ) {
	return thread.running;
}

void uf::thread::terminate() {
	while ( !uf::thread::threads.empty() ) {
		pod::Thread& thread = **uf::thread::threads.begin();
		uf::thread::destroy( thread );
	}
}
pod::Thread& uf::thread::create( const uf::stl::string& name, bool start, bool locks ) {
	if ( name == uf::thread::mainThreadName ) start = false;

	pod::Thread* pointer = NULL;
	uf::thread::threads.emplace_back(pointer = new pod::Thread);
	pod::Thread& thread = *pointer;

	static uint uids = 0;
	static int limit = uf::thread::workers;
	static uint threads = std::thread::hardware_concurrency();
	thread.name = name;
	thread.uid = uids++;
	thread.running = false;
	thread.mutex = locks ? new std::mutex : NULL;
	thread.limiter = uf::thread::limiter;
	thread.affinity = (thread.uid % limit) + 1;

//	UF_THREAD_ANNOUNCE("Creating Thread #" << thread.uid << " (" << thread.name << " on core " << thread.affinity << ")" << (thread.limiter ? " (Limiter: " + std::to_string(1.0f / thread.limiter) + " FPS)" : ""));

	if ( start ) uf::thread::start( thread );

	return thread;
}
void uf::thread::destroy( pod::Thread& thread ) {
	if ( !uf::thread::has( uf::thread::uid(thread) ) ) return; // oops

//	UF_THREAD_ANNOUNCE("Quitting Thread #" << thread.uid << " (" << thread.name << ")");
	uf::thread::quit( thread );

	if ( thread.mutex != NULL ) delete thread.mutex;

	for ( auto it = uf::thread::threads.begin(); it != uf::thread::threads.end(); ++it )
		if ( uf::thread::uid( **it ) == uf::thread::uid(thread) ) {
			delete *it;
			uf::thread::threads.erase( it );
			break;
		}
}
bool uf::thread::has( uint uid ) {
	for ( const pod::Thread* thread : uf::thread::threads ) if ( uf::thread::uid(*thread) == uid ) return true;
	return false;
}
bool uf::thread::has( std::thread::id id ) {
	for ( const pod::Thread* thread : uf::thread::threads ) if ( uf::thread::id(*thread) == id ) return true;
	return false;
}
bool uf::thread::has( const uf::stl::string& name ) {
	for ( const pod::Thread* thread : uf::thread::threads ) if ( uf::thread::name(*thread) == name ) return true;
	return false;
}
pod::Thread& uf::thread::get( uint uid ) {
	for ( pod::Thread* thread : uf::thread::threads ) if ( uf::thread::uid(*thread) == uid ) return *thread;
	UF_EXCEPTION("Thread error: invalid call");
//	return uf::thread::get(uf::thread::mainThreadId);
}
pod::Thread& uf::thread::get( std::thread::id id ) {
	for ( pod::Thread* thread : uf::thread::threads ) if ( uf::thread::id(*thread) == id ) return *thread;
	UF_EXCEPTION("Thread error: invalid call");
//	return uf::thread::get(uf::thread::mainThreadId);
}
pod::Thread& uf::thread::get( const uf::stl::string& name ) {
	if ( !uf::thread::has(name) ) return uf::thread::create(name);
	for ( pod::Thread* thread : uf::thread::threads ) if ( uf::thread::name(*thread) == name ) return *thread;
	UF_EXCEPTION("Thread error: invalid call");
//	return uf::thread::get(uf::thread::mainThreadId);
}

bool uf::thread::isMain() {
	return uf::thread::mainThreadId == std::this_thread::get_id();
}
pod::Thread& uf::thread::currentThread() {
	std::thread::id id = std::this_thread::get_id();
	if ( uf::thread::has(id) ) return uf::thread::get(id);
	UF_MSG_ERROR("Invalid thread call");
	return uf::thread::get(uf::thread::mainThreadId);
}