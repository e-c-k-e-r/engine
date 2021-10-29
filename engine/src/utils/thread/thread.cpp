#include <uf/utils/thread/thread.h>
#include <uf/utils/io/iostream.h>
#include <iostream>
#include <future>

uf::thread::container_t uf::thread::threads;
float uf::thread::limiter = 1.0f / 120.0f;
uint uf::thread::workers = 1;
std::thread::id uf::thread::mainThreadId = std::this_thread::get_id();
bool uf::thread::async = false;

#define UF_THREAD_ANNOUNCE(x)\
	//uf::iostream << x << "\n";

void UF_API uf::thread::start( pod::Thread& thread ) { if ( thread.running ) return;
	thread.thread = std::thread( uf::thread::tick, std::ref(thread) );
	thread.running = true;
}
void UF_API uf::thread::quit( pod::Thread& thread ) { // if ( !thread.running ) return;
	thread.running = false;

	if ( thread.mutex != NULL ) thread.mutex->lock();
	if ( thread.thread.joinable() ) thread.thread.join();
	if ( thread.mutex != NULL ) thread.mutex->unlock();

}
void UF_API uf::thread::tick( pod::Thread& thread ) {
#if !UF_ENV_DREAMCAST
	bool res = SetThreadAffinityMask(GetCurrentThread(), (1u << thread.affinity));
	if ( !res ) UF_THREAD_ANNOUNCE("Failed to set affinity of Thread #" << thread.uid << " (" << thread.name << " on ID " << pthread_self() << "/" << thread.affinity << ")");
#endif
	UF_THREAD_ANNOUNCE("Starting Thread #" << thread.uid << " (" << thread.name << " on ID " << thread.affinity << ") (Limiter: " << (1.0f / thread.limiter) << " FPS)");
	thread.timer.start();
	
	while ( thread.running ) {
		uf::thread::process( thread );
		if ( thread.terminates && thread.temps.empty() && thread.consts.empty() ) uf::thread::quit( thread );

		if ( thread.limiter > 0 ) {
			long long sleep = (thread.limiter * 1000) - thread.timer.elapsed().asMilliseconds();
			if ( sleep > 0 ) {
				std::this_thread::sleep_for(std::chrono::milliseconds(sleep));
			}
			thread.timer.reset();
		}
	}
}

pod::Thread& UF_API uf::thread::fetchWorker( const uf::stl::string& name ) {
	static int current = 0;
	static int limit = uf::thread::workers;
	static uint threads = std::thread::hardware_concurrency();
	int tries = 8;
	while ( --tries >= 0 ) {
		if ( ++current >= limit ) current = 0;
		uf::stl::string thread = name;
		if ( current > 0 ) thread += " " + std::to_string(current);
		bool exists = uf::thread::has(thread);
		auto& pod = exists ? uf::thread::get(thread) : uf::thread::create(thread, true);
		if ( std::this_thread::get_id() != pod.thread.get_id() ) return pod;
	}
	
	return uf::thread::has("Main") ? uf::thread::get("Main") : uf::thread::create("Main", true);
}
void UF_API uf::thread::batchWorker( const pod::Thread::function_t& function, const uf::stl::string& name ) {
	return batchWorkers( { function }, false, name );
}
void UF_API uf::thread::batchWorkers( const uf::stl::vector<pod::Thread::function_t>& functions, bool wait, const uf::stl::string& name ) {
	if ( uf::thread::async ) {
		uf::stl::vector<std::future<void>> futures;
		futures.reserve(functions.size());
		for ( auto& function : functions ) futures.emplace_back(std::async( std::launch::async, function ));
		if ( wait ) for ( auto& future : futures ) future.wait();
		return;
	}

	uf::stl::vector<pod::Thread*> workers;
	for ( auto& function : functions ) {
		auto& worker = uf::thread::fetchWorker( name );
		workers.emplace_back(&worker);
		uf::thread::add( worker, function, true );
	}
	if ( wait ) for ( auto& worker : workers ) uf::thread::wait( *worker );
}
void UF_API uf::thread::add( pod::Thread& thread, const pod::Thread::function_t& function, bool temporary ) {
	if ( thread.mutex != NULL ) thread.mutex->lock();
	temporary ? thread.temps.push( function ) : thread.consts.push_back( function );
	if ( thread.mutex != NULL ) thread.mutex->unlock();
}
void UF_API uf::thread::process( pod::Thread& thread ) { if ( !uf::thread::has(uf::thread::uid(thread)) ) { UF_THREAD_ANNOUNCE("Bad Thread: " << thread.uid << " " << thread.name); return; } //ops
	while ( !thread.temps.empty() ) {
		auto& function = thread.temps.front();
		if ( function )
	#if UF_NO_EXCEPTIONS
			function();
	#else
		try {
			function();
		} catch ( std::exception& e ) {
			UF_MSG_ERROR("Thread " << thread.name << " (UID: " << thread.uid << ") caught exception: " << e.what());
		}
	#endif
		thread.temps.pop();
	}
	for ( auto function : thread.consts ) {
		if ( function )
	#if UF_NO_EXCEPTIONS
			function();
	#else
		try {
			function();
		} catch ( std::exception& e ) {
			UF_MSG_ERROR("Thread " << thread.name << " (UID: " << thread.uid << ") caught exception: " << e.what());
		}
	#endif
	}
	thread.condition.notify_one();
}
void UF_API uf::thread::wait( pod::Thread& thread ) {
	if ( thread.mutex != NULL ) {
		std::unique_lock<std::mutex> lock(*thread.mutex);
    	thread.condition.wait(lock, [&]{return thread.temps.empty();});
    	return;
    }
	while ( !thread.temps.empty() );
}

const uf::stl::string& UF_API uf::thread::name( const pod::Thread& thread ) {
	return thread.name;
}
uint UF_API uf::thread::uid( const pod::Thread& thread ) {
	return thread.uid;
}
bool UF_API uf::thread::running( const pod::Thread& thread ) {
	return thread.running;
}

void UF_API uf::thread::terminate() {
	while ( !uf::thread::threads.empty() ) {
		pod::Thread& thread = **uf::thread::threads.begin();
		uf::thread::destroy( thread );
	}
}
pod::Thread& UF_API uf::thread::create( const uf::stl::string& name, bool start, bool locks ) {
	pod::Thread* pointer = NULL;
	uf::thread::threads.emplace_back(pointer = new pod::Thread);
	pod::Thread& thread = *pointer;

	static uint uids = 0;
	static int limit = uf::thread::workers;
	static uint threads = std::thread::hardware_concurrency();
	thread.name = name;
	thread.uid = uids++;
	thread.terminates = false;
	thread.running = false;
	thread.mutex = NULL;
	thread.mutex = locks ? new std::mutex() : NULL;
	thread.limiter = uf::thread::limiter;
	thread.affinity = (thread.uid % limit) + 1;

	UF_THREAD_ANNOUNCE("Creating Thread #" << thread.uid << " (" << name << ") " << &thread << " (Affinity: " << thread.affinity << ") (Limiter: " << (1.0f / thread.limiter) << " FPS)");

	if ( start ) uf::thread::start( thread );

	return thread;
}
void UF_API uf::thread::destroy( pod::Thread& thread ) {
	if ( !uf::thread::has( uf::thread::uid(thread) ) ) return; // oops

	UF_THREAD_ANNOUNCE("Quitting Thread #" << thread.uid << " (" << thread.name << ")");
	uf::thread::quit( thread );
	UF_THREAD_ANNOUNCE("Quitted Thread #" << thread.uid << " (" << thread.name << ")");

	if ( thread.mutex != NULL ) delete thread.mutex;

	for ( auto it = uf::thread::threads.begin(); it != uf::thread::threads.end(); ++it )
		if ( uf::thread::uid( **it ) == uf::thread::uid(thread) ) {
			delete *it;
			uf::thread::threads.erase( it );
			break;
		}
}
bool UF_API uf::thread::has( uint uid ) {
	for ( const pod::Thread* thread : uf::thread::threads ) if ( uf::thread::uid(*thread) == uid ) return true;
	return false;
}
bool UF_API uf::thread::has( const uf::stl::string& name ) {
	for ( const pod::Thread* thread : uf::thread::threads ) if ( uf::thread::name(*thread) == name ) return true;
	return false;
}
pod::Thread& UF_API uf::thread::get( uint uid ) {
	for ( pod::Thread* thread : uf::thread::threads ) if ( uf::thread::uid(*thread) == uid ) return *thread;
	UF_EXCEPTION("Thread error: invalid call");
}
pod::Thread& UF_API uf::thread::get( const uf::stl::string& name ) {
	for ( pod::Thread* thread : uf::thread::threads ) if ( uf::thread::name(*thread) == name ) return *thread;
	UF_EXCEPTION("Thread error: invalid call");
}
bool UF_API uf::thread::isMain() {
	return uf::thread::mainThreadId == std::this_thread::get_id();
}