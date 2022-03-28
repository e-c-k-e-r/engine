#define TOKEN__PASTE(x, y) x ## y
#define TOKEN_PASTE(x, y) TOKEN__PASTE(x, y)

#define UF_NS_GET_LAST(name) uf::string::replace( uf::string::split( #name, "::" ).back(), "<>", "" )

/*
-x * 1000, uf::Time<long long>::milliseconds

#define TIMER(x, ...) auto TOKEN_PASTE(TIMER, __LINE__) = []( uf::physics::num_t every = 1 ) {\
		static uf::Timer<long long> timer(false);\
		if ( !timer.running() ) {\
			timer.start(uf::Time<long long>(-1000000));\
		}\
		uf::physics::num_t time = 0;\
		if ( (time = timer.elapsed()) >= every ) {\
			timer.reset();\
		}\
		static bool first = true; if ( first ) { first = false; return every; }\
		return time;\
	};\
	uf::physics::num_t time = 0;\
	if ( __VA_ARGS__ (time = TOKEN_PASTE(TIMER, __LINE__)(x)) >= x )
*/
#define TIMER_LAMBDA(x) []() {\
	static uf::Timer<long long> timer(false);\
	if ( !timer.running() ) timer.start(uf::Time<long long>(-1000000));\
	double time = timer.elapsed();\
	if ( time >= every ) timer.reset();\
	static bool first = true; if ( first ) { first = false; return every; }\
	return time;\
};

#define TIMER(x, ...)\
	static uf::Timer<long long> timer(false);\
	if ( !timer.running() ) timer.start(uf::Time<long long>(-1000000));\
	double time = timer.elapsed();\
	if ( time >= x ) timer.reset();\
	if ( __VA_ARGS__ time >= x )

#define UF_DEBUG 1
#if UF_DEBUG
	#include <iostream>
	#include <iomanip>
#endif

#define UF_IO_COUT std::cout // uf::iostream
#define UF_IO_ENDL std::endl // "\r\n"

#define UF_MSG(Y, X) UF_IO_COUT << "[" << X << "] " << __FILE__ << ":" << __FUNCTION__ << "@" << __LINE__ << ": " << Y << UF_IO_ENDL;

#define UF_MSG_DEBUG(X) if (UF_DEBUG)	UF_MSG(X, "  DEBUG  ");
#define UF_MSG_INFO(X) 					UF_MSG(X, "  INFO   ");
#define UF_MSG_WARNING(X) 				UF_MSG(X, " WARNING ");
#define UF_MSG_ERROR(X) 				UF_MSG(X, "  ERROR  ");

#if UF_NO_EXCEPTIONS
	#define UF_EXCEPTION(X) { UF_MSG_ERROR(X); std::abort(); }
#else
	#define UF_EXCEPTION(X) {\
		uf::stl::stringstream str;\
		str << X;\
		uf::stl::string Y = str.str();\
		UF_MSG_ERROR(Y);\
		throw std::runtime_error(Y);\
	}
#endif

#define UF_ASSERT(condition, ...) if ( !(condition) ) { UF_EXCEPTION("Assert failed: " << #condition); }
#define UF_ASSERT_BREAK(condition, ...) if ( !(condition) ) { UF_MSG_ERROR("Assert failed: " << #condition); break; }
#define UF_ASSERT_SAFE(condition) if ( !(condition) ) { UF_MSG_ERROR("Assertion failed: " << #condition); }

#define UF_ASSERT_MSG(condition, ...) if ( !(condition) ) { UF_EXCEPTION("Assert failed: " << #condition << " " << __VA_ARGS__); }
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
	TIMER_TRACE_CUR = TIMER_TRACE.elapsed().asMilliseconds();\
	UF_MSG_DEBUG(std::setfill(' ') << std::setw(4) << TIMER_TRACE_CUR << " ms\t" << std::setfill(' ') << std::setw(4) << (TIMER_TRACE_CUR - TIMER_TRACE_PREV) << " ms\t" << X);\
	TIMER_TRACE_PREV = TIMER_TRACE_CUR;\
}

#define UF_TIMER_MULTITRACE_END(X) UF_MSG_DEBUG(X);

#include <type_traits>
#define TYPE_SANITIZE(T) std::remove_cv_t<std::remove_reference_t<T>>

#if UF_CTTI
	#include <ctti/type_id.hpp>
	#define TYPE_INDEX_T ctti::type_id_t
	#define TYPE_HASH_T ctti::detail::hash_t
	#define TYPE(T) ctti::type_id<TYPE_SANITIZE(T)>()
	#define TYPE_HASH(T) TYPE(T).hash()
	#define TYPE_NAME(T) ctti::nameof<T>().str()
#else
	#define TYPE_INDEX_T std::type_index
	#define TYPE_HASH_T std::size_t
	#define TYPE(T) typeid(TYPE_SANITIZE(T))
	#define TYPE_HASH(T) TYPE(T).hash_code()
	#define TYPE_NAME(T) TYPE(T).name()
#endif

#define MIN(X, Y) (X) < (Y) ? (X) : (Y)
#define MAX(X, Y) (X) > (Y) ? (X) : (Y)
#define LENGTH_OF(X) *(&X + 1) - X
#define FOR_ARRAY(X) for ( auto i = 0; i < LENGTH_OF(X); ++i )