#include <uf/utils/thread/thread.h>
#include <iostream>

uf::thread::container_t uf::thread::threads;

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
	while ( thread.running ) {
		uf::thread::process( thread );
		if ( thread.terminates && thread.temps.empty() && thread.consts.empty() ) uf::thread::quit( thread );
	}
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
void UF_API uf::thread::process( pod::Thread& thread ) { if ( !uf::thread::has(uf::thread::uid(thread)) ) { std::cout << "Bad Thread: " << thread.uid << " " << thread.name << std::endl; return; } //ops
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
}
void UF_API uf::thread::wait( pod::Thread& thread ) {
	if ( thread.mutex != NULL ) {
		thread.mutex->lock();
		thread.mutex->unlock();
	}
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
	thread.name = name;
	thread.uid = uids++;
	thread.terminates = false;
	thread.running = false;
	thread.mutex = NULL;
	thread.mutex = locks ? new std::mutex() : NULL;

	std::cout << "Creating Thread #" << thread.uid << " (" << name << ") " << &thread << std::endl;

	if ( start ) uf::thread::start( thread );

	return thread;
}
void UF_API uf::thread::destroy( pod::Thread& thread ) {
	if ( !uf::thread::has( uf::thread::uid(thread) ) ) return; // oops

	uf::thread::quit( thread );
	std::cout << "Quitted Thread #" << thread.uid << " (" << thread.name << ")" << std::endl;

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
	throw "invalid call to uf::thread::get";
}
pod::Thread& UF_API uf::thread::get( const std::string& name ) {
	for ( pod::Thread* thread : uf::thread::threads ) if ( uf::thread::name(*thread) == name ) return *thread;
	throw "invalid call to uf::thread::get";
}

/*
#include <sstream>

uint uf::Thread::fps = 60;
uint uf::Thread::threads = 0;

UF_API_CALL uf::Thread::Thread( const std::string& name, bool start, uint mode ) {
	static int uid = 0;
	this->m_mode = mode;
	this->m_name = name;
	this->m_running = false;
	this->m_shouldLock = false;
	this->m_terminateOnEmpty = false;
	this->m_uid = start ? ++uid : 0;

	if ( uf::Thread::threads != 0 && this->m_uid > uf::Thread::threads ) start = false;
	if ( start ) this->start();
}
UF_API_CALL uf::Thread::~Thread() {
	this->m_quit();
}

void UF_API_CALL uf::Thread::start() {
	if ( this->m_running ) return;
	
	this->m_running = true;
	this->m_thread = std::thread( [&]{ this->tick(); } );
}

void UF_API_CALL uf::Thread::tick( const std::function<void()>& callback ) {
	double curTime = uf::time::curTime;
	double prevTime = uf::time::prevTime;
	double deltaTime = curTime - prevTime;
	double tempTime = 0;
	double totalTime = 0;
	double framerate = 1 / uf::Thread::fps;

	bool should_call = (bool) callback;
	while ( this->m_running ) {
		prevTime = curTime;
		curTime = uf::time::curTime;
		deltaTime = curTime - prevTime;
		tempTime += deltaTime;
		totalTime += deltaTime;

		if ( tempTime > framerate ) {
			this->process();
			tempTime = 0.0;
			if ( should_call ) callback();
			if ( this->m_terminateOnEmpty && this->m_temps.empty() && this->m_consts.empty() ) {
				this->m_running = false;
			}
		}
	}
}

void UF_API_CALL uf::Thread::add( const uf::Thread::function_t& function, uint mode ) {
	if ( this->m_shouldLock ) this->m_mutex.lock();
	switch ( mode ) {
		case uf::Thread::TEMP: this->m_temps.push(function); break;
		case uf::Thread::CONSTANT: this->m_consts.push_back(function); break;
	}
	if ( this->m_shouldLock ) this->m_mutex.unlock();
}

void UF_API_CALL uf::Thread::process() {
	std::function<int()> temps = [&] {
		int i = 0;
		while ( !this->m_temps.empty() ) {
			uf::Thread::function_t& function = this->m_temps.front();
			if ( function() != -1 ) this->m_temps.pop();
			i++;
		}
		return i;
	};
	std::function<int()> consts = [&] {
		int i = 0;
		for ( uf::Thread::container_t::iterator it = this->m_consts.begin(); it != this->m_consts.end(); ++it ) {
			uf::Thread::function_t& function = *it;
			function();
			i++;
		}
		return i;
	};

	if ( this->m_shouldLock ) this->m_mutex.lock();
	try {
		switch ( this->m_mode ) {
			case 0:
				temps();
				consts();
			break;
			case 1:
				consts();
				temps();
			break;
		}
	}
	catch ( ... ) {
	//	std::cout << "error" << std::endl;
	//	uf::iostream << "error" << "\n";
	}
	if ( this->m_shouldLock ) this->m_mutex.unlock();
}

void UF_API_CALL uf::Thread::quit() {
	this->m_running = false;
	this->m_thread.join();

//	uf::log << uf::Log::Entree( { "basic", { this->m_toString() + " killed", __LINE__ - 1, __FILE__, __FUNCTION__, 0 } } );
}

std::string UF_API_CALL uf::Thread::toString() const {
//	std::stringstream str;
//	str << "Thread(Name: " << this->m_name << ")";
//	return str.str();
	return "Thread";
}
const std::string& UF_API_CALL uf::Thread::getName() const {
	return this->m_name;
}
uint UF_API_CALL uf::Thread::getUid() const {
	return this->m_uid;
}
const bool& UF_API_CALL uf::Thread::isRunning() const {
	return this->m_running;
}
//
uf::thread::container_t uf::thread::threads;

void UF_API_CALL uf::thread::add( uf::Thread* thread ) {
	std::string name = thread->getName();
	uf::thread::threads[name] = thread;
}
uf::Thread& UF_API_CALL uf::thread::get( const std::string& name ) {
	static uf::Thread null( "NULL", false );
	try {
		return *uf::thread::threads.at(name);
	} catch ( std::out_of_range oor ) {
		return null;
	}
}
bool UF_API_CALL uf::thread::add( const std::string& name, const uf::Thread::function_t& function, uint mode  ) {
	uf::Thread& thread = uf::thread::get(name);
	if ( thread.getName() == "NULL" || ( uf::Thread::threads != 0 && thread.getUid() > uf::Thread::threads ) ) {
		switch ( mode ) {
			case uf::Thread::TEMP: function(); break;
		}
		return false;
	}
	thread.add( function, mode );

	return true;
}
bool UF_API_CALL uf::thread::quit( const std::string& name ) {
	uf::Thread& thread = uf::thread::get(name);
	if ( thread.getName() == "NULL" ) return false;

	thread.quit();
	return true;
}
*/