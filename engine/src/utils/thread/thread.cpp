#include <uf/utils/thread/thread.h>
#include <uf/utils/io/iostream.h>
#include <iostream>

uf::thread::container_t uf::thread::threads;
double uf::thread::limiter = 1.0f / 120.0f;
uint uf::thread::workers = 1;
std::thread::id uf::thread::mainThreadId = std::this_thread::get_id();

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
/*
	bool res = SetThreadAffinityMask(GetCurrentThread(), (1u << thread.affinity));
	if ( !res ) UF_THREAD_ANNOUNCE("Failed to set affinity of Thread #" << thread.uid << " (" << thread.name << " on ID " << pthread_self() << "/" << thread.affinity << ")");
	if ( !thread.timer.running() ) thread.timer.start();
*/
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

pod::Thread& UF_API uf::thread::fetchWorker( const std::string& name ) {
	static int current = 0;
	static int limit = uf::thread::workers;
	static uint threads = std::thread::hardware_concurrency();
	if ( ++current >= limit ) current = 0;
	std::string thread = name;
	if ( current > 0 ) thread += " " + std::to_string(current);
	bool exists = uf::thread::has(thread);
	auto& pod = exists ? uf::thread::get(thread) : uf::thread::create( thread, true );
	if ( !exists ) {
/*
		pod.affinity = (current + 1) % threads;
		std::cout << "Create: " << thread << ": " << pod.affinity << std::endl;
		BOOL res = SetThreadAffinityMask((HANDLE) pod.thread.native_handle(), 1u << pod.affinity );
		if ( res ) if ( UF_THREAD_ANNOUNCE ) uf::iostream << "Bound worker thread #" << current << " to ID " << pod.affinity << "\n";
		else if ( UF_THREAD_ANNOUNCE ) uf::iostream << "Failed to bind worker thread #" << current << " to ID " << pod.affinity << "\n";
*/
	}
	return pod;
}
void UF_API uf::thread::batchWorkers( const std::vector<pod::Thread::function_t>& functions, bool wait, const std::string& name ) {
	std::vector<pod::Thread*> workers;
	for ( auto& function : functions ) {
		auto& worker = uf::thread::fetchWorker( name );
		workers.emplace_back(&worker);
		uf::thread::add( worker, function, true );
	}
	if ( wait )
		for ( auto& worker : workers )
			uf::thread::wait( *worker );
}
/*
void UF_API uf::thread::tick( pod::Thread& thread, const std::function<void()>& callback ) {
	while ( thread.running ) {
		uf::thread::process( thread );
		if ( callback ) callback();
		if ( thread.terminates && thread.temps.empty() && thread.consts.empty() ) uf::thread::quit( thread );
	}
}
*/
void UF_API uf::thread::add( pod::Thread& thread, const pod::Thread::function_t& function, bool temporary ) {
	if ( thread.mutex != NULL ) thread.mutex->lock();
	temporary ? thread.temps.push( function ) : thread.consts.push_back( function );
	if ( thread.mutex != NULL ) thread.mutex->unlock();
}
void UF_API uf::thread::assertExecute( pod::Thread& thread, const pod::Thread::function_t& f ) {
	std::cout << "ASSERT EXECUTION: " << thread.name << "\t" << uf::thread::isMain() << "\t" << std::this_thread::get_id() << "\t" << thread.thread.get_id() << std::endl;
	if ( thread.name == "Main" && uf::thread::isMain() ) f();
	else if ( std::this_thread::get_id() == thread.thread.get_id() ) f();
	else uf::thread::add( thread, f, true );
}
void UF_API uf::thread::process( pod::Thread& thread ) { if ( !uf::thread::has(uf::thread::uid(thread)) ) { UF_THREAD_ANNOUNCE("Bad Thread: " << thread.uid << " " << thread.name); return; } //ops
/*
	std::function<int()> temps = [&] {
		int i = 0;
		while ( !thread.temps.empty() ) {
			auto& function = thread.temps.front();
			if ( function ) function();
			thread.temps.pop();
			++i;
		}
		return i;
	};
	std::function<int()> consts = [&] {
		int i = 0;
		for ( auto function : thread.consts ) {
			function();
			++i;
		}
		return i;
	};
	if ( thread.mutex != NULL ) thread.mutex->lock();
	temps();
	consts();
	if ( thread.mutex != NULL ) thread.mutex->unlock();
*/
	size_t temps = 0;
	size_t consts = 0;
	while ( !thread.temps.empty() ) {
		auto& function = thread.temps.front();
	#if UF_NO_EXCEPTIONS
		if ( function ) function();
	#else
		if ( function ) try {
			function();
		} catch ( std::exception& e ) {
			uf::iostream << "Thread " << thread.name << " (UID: " << thread.uid << ") caught exception: " << e.what() << "\n";
		}
	#endif
		thread.temps.pop();
		++temps;
	}
	for ( auto function : thread.consts ) {
	#if UF_NO_EXCEPTIONS
		if ( function ) function();
	#else
		if ( function ) try {
			function();
		} catch ( std::exception& e ) {
			uf::iostream << "Thread " << thread.name << " (UID: " << thread.uid << ") caught exception: " << e.what() << "\n";
		}
	#endif
		++consts;
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
/*
	if ( thread.mutex != NULL ) {
		thread.mutex->lock();
		thread.mutex->unlock();
		return;
	}
	while ( !thread.temps.empty() );
*/
}

const std::string& UF_API uf::thread::name( const pod::Thread& thread ) {
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
pod::Thread& UF_API uf::thread::create( const std::string& name, bool start, bool locks ) {
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
bool UF_API uf::thread::has( const std::string& name ) {
	for ( const pod::Thread* thread : uf::thread::threads ) if ( uf::thread::name(*thread) == name ) return true;
	return false;
}
pod::Thread& UF_API uf::thread::get( uint uid ) {
	for ( pod::Thread* thread : uf::thread::threads ) if ( uf::thread::uid(*thread) == uid ) return *thread;
	UF_EXCEPTION("invalid call to uf::thread::get");
}
pod::Thread& UF_API uf::thread::get( const std::string& name ) {
	for ( pod::Thread* thread : uf::thread::threads ) if ( uf::thread::name(*thread) == name ) return *thread;
	UF_EXCEPTION("invalid call to uf::thread::get");
}
bool UF_API uf::thread::isMain() {
	return uf::thread::mainThreadId == std::this_thread::get_id();
}