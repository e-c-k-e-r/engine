#include <uf/utils/thread/thread.h>

#if UF_USE_KOS_THREAD
struct KOSLock {
	mutex_t* m;
	KOSLock(mutex_t& mutex) : m(&mutex) { mutex_lock(m); }
	~KOSLock() { mutex_unlock(m); }
};

#define LOCK_MUTEX(m) KOSLock lock_##__LINE__(m)

uf::thread::container_t uf::thread::threads;
float uf::thread::limiter = 1.0f / 120.0f;
uint32_t uf::thread::workers = 1;
uf::thread::id_t uf::thread::mainThreadId = nullptr;
bool uf::thread::async = false;
uf::stl::string uf::thread::mainThreadName = "Main";
uf::stl::string uf::thread::workerThreadName = "Worker";
uf::stl::string uf::thread::asyncThreadName = "Async";

namespace {
	mutex_t global_mutex = MUTEX_INITIALIZER;
}

static void* kos_thread_bootstrap(void* arg) {
	pod::Thread* t = static_cast<pod::Thread*>(arg);
	uf::thread::tick(*t);
	return nullptr;
}

void uf::thread::start( pod::Thread& thread ) { if ( thread.running ) return;
	thread.running = true;
	thread.thread = thd_create(0, kos_thread_bootstrap, &thread);
	if ( thread.thread ) {
		if (!thread.name.empty()) {
			thd_set_label(thread.thread, thread.name.c_str());
		}

		if (thread.name != uf::thread::mainThreadName) {
			thd_set_prio(thread.thread, PRIO_DEFAULT);
		}
	}
}

void uf::thread::quit( pod::Thread& thread ) { if ( !thread.running ) return;
	{
		LOCK_MUTEX(thread.mutex);
		thread.running = false;
	}
	cond_broadcast(&thread.conditions.queued);
	if ( thread.thread ) {
		thd_join(thread.thread, nullptr);
		thread.thread = nullptr;
	}
}

void uf::thread::tick( pod::Thread& thread ) {
	thread.timer.start();
	while ( thread.running ) {
		uf::thread::process( thread );
	}
}

pod::Thread& uf::thread::fetchWorker( const uf::stl::string& name ) {
	static volatile int current = 0;
	int limit = uf::thread::workers;
	int tries = 0;

	while ( tries++ < limit ) {
		int val = __atomic_fetch_add(&current, 1, __ATOMIC_RELAXED);
		auto workerName = FMT_FORMAT("{} {}", name, val % limit);
		auto& pod = uf::thread::get( workerName );
		if ( thd_get_current() == pod.thread ) continue;
		return pod;
	}
	UF_EXCEPTION("cannot find free worker");
}

pod::Thread::Tasks uf::thread::schedule( bool async, bool wait ) {
	return schedule( async ? uf::thread::asyncThreadName : uf::thread::mainThreadName, wait );
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

	__atomic_store_n(&tracker->pending, tasks.container.size(), __ATOMIC_RELEASE);

	if ( tasks.name == uf::thread::mainThreadName ) {
		for ( auto& task : tasks.container ) task();
		tasks.container.clear();
		__atomic_store_n(&tracker->pending, 0, __ATOMIC_RELEASE);
	} else {
		for ( auto& task : tasks.container ) {
			auto& worker = uf::thread::fetchWorker( tasks.name );

			uf::thread::queue( worker, [task, tracker]() {
				struct Decrementer {
					std::shared_ptr<pod::Thread::Tasks::Tracker> t;
					~Decrementer() {
						if ( __atomic_sub_fetch(&t->pending, 1, __ATOMIC_RELEASE) == 0 ) {
							mutex_lock(&t->mutex);
							cond_broadcast(&t->cv);
							mutex_unlock(&t->mutex);
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
	mutex_lock(&tracker->mutex);
	while (__atomic_load_n(&tracker->pending, __ATOMIC_ACQUIRE) > 0) {
		cond_wait(&tracker->cv, &tracker->mutex);
	}
	mutex_unlock(&tracker->mutex);
}

void uf::thread::add( pod::Thread& thread, const pod::Thread::function_t& function ) {
	LOCK_MUTEX(thread.mutex);
	thread.container.emplace_back( function );
}

void uf::thread::queue( const pod::Thread::container_t& functions ) {
	for ( auto& function : functions )
		uf::thread::queue( uf::thread::fetchWorker(), function );
}

void uf::thread::queue( const pod::Thread::function_t& function ) {
	return uf::thread::queue( uf::thread::fetchWorker(), function );
}

void uf::thread::queue( pod::Thread& thread, const pod::Thread::function_t& function ) {
	{
		LOCK_MUTEX(thread.mutex);
		thread.queue.emplace_back( function );
		__atomic_add_fetch(&thread.pending, 1, __ATOMIC_SEQ_CST);
	}
	cond_signal(&thread.conditions.queued);
}

void uf::thread::process( pod::Thread& thread ) { if ( !uf::thread::has(thread.name) ) return;
	STATIC_THREAD_LOCAL(pod::Thread::container_t, local_queue);
	STATIC_THREAD_LOCAL(pod::Thread::container_t, local_container);

	if ( thread.name == uf::thread::mainThreadName ) {
		if ( thread.queue.empty() ) return;
		mutex_lock(&thread.mutex);
		std::swap( local_queue, thread.queue );
		mutex_unlock(&thread.mutex);
		for ( auto& function : local_queue ) function();
		return;
	}

	if ( thread.limiter > 0 ) {
		long long sleep_ms = (thread.limiter * 1000.0f) - thread.timer.elapsed().asMilliseconds();
		if ( sleep_ms > 0 ) {
		//	thd_pass();
			thd_sleep(sleep_ms);
		}
		thread.timer.reset();
	}

	{
		mutex_lock(&thread.mutex);
		if ( thread.limiter <= 0 ) {
			while (thread.queue.empty() && thread.container.empty() && thread.running) {
				cond_wait(&thread.conditions.queued, &thread.mutex);
			}
		}

		if ( !thread.running ) {
			mutex_unlock(&thread.mutex);
			return;
		}
		std::swap( local_queue, thread.queue );
		mutex_unlock(&thread.mutex);
	}

	// iterate through queued work
	for ( auto& function : local_queue ) {
		function();
		if ( __atomic_sub_fetch(&thread.pending, 1, __ATOMIC_SEQ_CST) == 0 ) {
			mutex_lock(&thread.mutex);
			cond_broadcast(&thread.conditions.finished);
			mutex_unlock(&thread.mutex);
		}
	}

	// buffer persistent work
	{
		LOCK_MUTEX(thread.mutex);
		local_container = thread.container;
	}

	// iterate through persistent work
	for ( auto& function : local_container ) {
		function();
	}

	{
		LOCK_MUTEX(thread.mutex);
		cond_broadcast(&thread.conditions.finished);
	}
}

void uf::thread::wait( pod::Thread& thread ) {
	mutex_lock(&thread.mutex);
	while (__atomic_load_n(&thread.pending, __ATOMIC_ACQUIRE) > 0) {
		cond_wait(&thread.conditions.finished, &thread.mutex);
	}
	mutex_unlock(&thread.mutex);
}

uf::stl::string uf::thread::name( id_t id ) {
	if ( id == uf::thread::mainThreadId ) return uf::thread::mainThreadName;
	for ( auto& [ name, thread ] : uf::thread::threads ) {
		if ( thread->thread == id ) return name;
	}
	return "?";
}

const uf::stl::string& uf::thread::name( const pod::Thread& thread ) {
	return thread.name;
}

uf::thread::id_t uf::thread::id( const pod::Thread& thread ) {
	if ( thread.name == uf::thread::mainThreadName ) return uf::thread::mainThreadId;
	return thread.thread;
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
		LOCK_MUTEX(::global_mutex);
		std::swap( local_threads, uf::thread::threads );
	}

	for ( auto& [ key, thread ] : local_threads ) {
		uf::thread::quit( *thread );
		mutex_destroy(&thread->mutex);
		cond_destroy(&thread->conditions.queued);
		cond_destroy(&thread->conditions.finished);
		delete thread;
	}
}

pod::Thread& uf::thread::create( const uf::stl::string& name, bool start, bool locks ) {
	if ( !uf::thread::mainThreadId ) {
		uf::thread::mainThreadId = thd_get_current();
	}

	if ( name == uf::thread::mainThreadName ) start = false;

	pod::Thread* pointer = NULL;
	{
		LOCK_MUTEX(::global_mutex);
		uf::thread::threads[name] = (pointer = new pod::Thread);
	}
	pod::Thread& thread = *pointer;

	static auto limit = uf::thread::workers;
	static pod::Thread::id_t uids = 0;

	mutex_init(&thread.mutex, MUTEX_TYPE_NORMAL);
	cond_init(&thread.conditions.queued);
	cond_init(&thread.conditions.finished);

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
		LOCK_MUTEX(::global_mutex);
		uf::thread::threads.erase( name );
	}
	delete &thread; // shortcut, references from threads should always be from the map anyways
}
bool uf::thread::has( const uf::stl::string& name ) {
	LOCK_MUTEX(::global_mutex);
	return uf::thread::threads.count( name ) > 0;
}

pod::Thread& uf::thread::get( const uf::stl::string& name ) {
	if ( !uf::thread::has(name) ) return uf::thread::create(name);
	return *uf::thread::threads[name];
}

uf::thread::id_t uf::thread::current_id() {
	return thd_get_current();
}

#endif