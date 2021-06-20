#define TOKEN__PASTE(x, y) x ## y
#define TOKEN_PASTE(x, y) TOKEN__PASTE(x, y)

#define UF_NS_GET_LAST(name) uf::string::replace( uf::string::split( #name, "::" ).back(), "<>", "" )

#define TIMER(x, ...) auto TOKEN_PASTE(TIMER, __LINE__) = []( double every = 1 ) {\
		static uf::Timer<long long> timer(false);\
		if ( !timer.running() ) {\
			timer.start(uf::Time<long long>(-x, uf::Time<long long>::seconds));\
		}\
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

// uf::iostream
#define UF_IO_COUT std::cout 
// "\n"
#define UF_IO_ENDL std::endl

#define UF_MSG(Y, X) UF_IO_COUT << "[" << X << "] " << __FILE__ << ":" << __FUNCTION__ << "@" << __LINE__ << ": " << Y << UF_IO_ENDL;

#define UF_MSG_DEBUG(X) if (UF_DEBUG)	UF_MSG(X, "  DEBUG  ");
#define UF_MSG_INFO(X) 					UF_MSG(X, "  INFO   ");
#define UF_MSG_WARNING(X) 				UF_MSG(X, " WARNING ");
#define UF_MSG_ERROR(X) 				UF_MSG(X, "  ERROR  ");

#if UF_NO_EXCEPTIONS
	#define UF_EXCEPTION(X) UF_MSG_ERROR(X)
#else
	#define UF_EXCEPTION(X) { UF_MSG_ERROR(X); throw std::runtime_error(X); }
#endif

#define UF_ASSERT_BREAK(condition, ...) if ( !(condition) ) { UF_MSG_ERROR("Assert failed: " << #condition); break; }
#define UF_ASSERT_SAFE(condition) if ( !(condition) ) { UF_MSG_ERROR("Assertion failed: " << #condition); }

#define UF_ASSERT_BREAK_MSG(condition, ...) if ( !(condition) ) { UF_MSG_ERROR("Assert failed: " << #condition << " " << __VA_ARGS__); break; }
#define UF_ASSERT_SAFE_MSG(condition, ...) if ( !(condition) ) { UF_MSG_ERROR("Assertion failed: " << #condition << " " << __VA_ARGS__); }

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

#define MIN(X, Y) X < Y ? X : Y 
#define MAX(X, Y) X > Y ? X : Y 