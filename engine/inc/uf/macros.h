#define TOKEN__PASTE(x, y) x ## y
#define TOKEN_PASTE(x, y) TOKEN__PASTE(x, y)

#define UF_NS_GET_LAST(name) uf::string::replace( uf::string::split( #name, "::" ).back(), "<>", "" )

#define TIMER(x, ...) auto TOKEN_PASTE(TIMER, __LINE__) = []( double every = 1 ) {\
		static uf::Timer<long long> timer(false);\
		if ( !timer.running() ) timer.start();\
		double time = 0;\
		if ( (time = timer.elapsed().asDouble()) >= every ) {\
			timer.reset();\
		}\
		return time;\
	};\
	double time = 0;\
	if ( __VA_ARGS__ (time = TOKEN_PASTE(TIMER, __LINE__)(x)) >= x )

#define UF_DEBUG 1
#if UF_DEBUG
	#include <iostream>
	#include <iomanip>
#endif
#define UF_MSG(X) std::cout << __FILE__ << ":" << __FUNCTION__ << "@" << __LINE__ << ": " << X << std::endl;

#define UF_MSG_DEBUG(X) if ( UF_DEBUG ) UF_MSG(X);
#define UF_DEBUG_PRINT_MARKER() UF_MSG_DEBUG("");

#define UF_MSG_INFO(X) 		UF_MSG_DEBUG("[ INFO  ] " << X);
#define UF_MSG_WARNING(X) 	UF_MSG_DEBUG("[WARNING] " << X);
#define UF_MSG_ERROR(X) 	UF_MSG_DEBUG("[ ERROR ] " << X);

#if UF_NO_EXCEPTIONS
	#define UF_EXCEPTION(X) UF_ERROR_MSG(X)
#else
	#define UF_EXCEPTION(X) throw std::runtime_error(X)
#endif

#define UF_TIMER_TRACE_INIT() uf::Timer<long long> TIMER_TRACE;

#define UF_TIMER_TRACE(X) {\
	auto elapsed = TIMER_TRACE.elapsed().asMilliseconds();\
	if ( elapsed > 0 ) UF_MSG_DEBUG(TIMER_TRACE.elapsed().asMilliseconds() << "ms\t" << X);\
}

#define UF_TIMER_TRACE_RESET(X) {\
	auto elapsed = TIMER_TRACE.elapsed().asMilliseconds();\
	if ( elapsed > 0 ) UF_MSG_DEBUG(TIMER_TRACE.elapsed().asMilliseconds() << "ms\t" << X);\
	TIMER_TRACE.reset();\
}

#define UF_TIMER_MULTITRACE_START(X)\
	UF_TIMER_TRACE_INIT();\
	long long TIMER_TRACE_PREV = 0, TIMER_TRACE_CUR = 0;\
	UF_MSG_DEBUG(X);\

#define UF_TIMER_MULTITRACE(X) {\
	TIMER_TRACE_CUR = TIMER_TRACE.elapsed().asMicroseconds();\
	UF_MSG_DEBUG(std::setfill(' ') << std::setw(6) << TIMER_TRACE_CUR << " us\t" << std::setfill(' ') << std::setw(6) << (TIMER_TRACE_CUR - TIMER_TRACE_PREV) << " us\t" << X);\
	TIMER_TRACE_PREV = TIMER_TRACE_CUR;\
}

#define UF_TIMER_MULTITRACE_END(X) UF_MSG_DEBUG(X);

// alias
#define UF_DEBUG_MSG(X) UF_MSG_DEBUG(X);

#define MIN(X, Y) X < Y ? X : Y 
#define MAX(X, Y) X > Y ? X : Y 