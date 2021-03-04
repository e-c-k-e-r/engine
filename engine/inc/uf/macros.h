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
#endif
#define UF_PRINT_MSG(X) std::cout << __FILE__ << ":" << __FUNCTION__ << "@" << __LINE__ << ": " << X << std::endl;
#define UF_DEBUG_MSG(X) if ( UF_DEBUG ) UF_PRINT_MSG(X);
#define UF_ERROR_MSG(X) UF_PRINT_MSG(X);
#define UF_DEBUG_PRINT_MARKER() UF_DEBUG_MSG("")

#if UF_NO_EXCEPTIONS
	#define UF_EXCEPTION(X) UF_ERROR_MSG(X)
#else
	#define UF_EXCEPTION(X) throw std::runtime_error(X)
#endif

#define UF_TIMER_TRACE_INIT() uf::Timer<long long> TIMER_TRACE;

#define UF_TIMER_TRACE(X) {\
	auto elapsed = TIMER_TRACE.elapsed().asMilliseconds();\
	if ( elapsed > 0 ) UF_DEBUG_MSG(X << " | " << TIMER_TRACE.elapsed().asMilliseconds() << "ms");\
}

#define UF_TIMER_TRACE_RESET(X) {\
	auto elapsed = TIMER_TRACE.elapsed().asMilliseconds();\
	if ( elapsed > 0 ) UF_DEBUG_MSG(X << " | " << TIMER_TRACE.elapsed().asMilliseconds() << "ms");\
	TIMER_TRACE.reset();\
}
