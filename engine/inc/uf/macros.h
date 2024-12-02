#define TOKEN__PASTE(x, y) x ## y
#define TOKEN_PASTE(x, y) TOKEN__PASTE(x, y)

#define UF_NS_GET_LAST(name) uf::string::replace( uf::string::split( #name, "::" ).back(), "<>", "" )

#define TIMER(x, ...)\
	static bool first = true;\
	static uf::Timer<long long> timer(false);\
	if ( !timer.running() ) timer.start(uf::Time<long long>(-1000000));\
	struct { bool should = true; } timerState = { __VA_ARGS__ };\
	double time = timer.elapsed();\
	if ( first ) {\
		time = x;\
		first = false;\
	}\
	if ( time >= x && timerState.should && (timer.reset(), true) )

#define UF_DEBUG 1
#if UF_DEBUG
	#include <uf/utils/io/fmt.h>
#endif

#if UF_USE_FMT
	#define UF_MSG(CATEGORY, ...) uf::io::log(CATEGORY, __FILE__, __FUNCTION__, __LINE__, ::fmt::format(__VA_ARGS__));
#else
	#define UF_MSG(CATEGORY, ...) uf::io::log(CATEGORY, __FILE__, __FUNCTION__, __LINE__, #__VA_ARGS__);
#endif

#define UF_MSG_DEBUG(...) if (UF_DEBUG)		UF_MSG("DEBUG", __VA_ARGS__);
#define UF_MSG_INFO(...) 					UF_MSG("INFO", __VA_ARGS__);
#define UF_MSG_WARNING(...) 				UF_MSG("WARNING", __VA_ARGS__);
#define UF_MSG_ERROR(...) 					UF_MSG("ERROR", __VA_ARGS__);

#if UF_NO_EXCEPTIONS
	#define UF_EXCEPTIONS 0
	#define UF_EXCEPTION(...) {\
		UF_MSG_ERROR(__VA_ARGS__);\
		std::abort();\
	}
#else
	#define UF_NO_EXCEPTIONS 0
	#define UF_EXCEPTIONS 1
	#define UF_EXCEPTION(...) {\
		auto msg = UF_MSG_ERROR(__VA_ARGS__);\
		throw std::runtime_error(msg);\
	}
#endif

#define UF_ASSERT(condition, ...) if ( !(condition) ) { UF_EXCEPTION("Assert failed: {}", #condition); }
#define UF_ASSERT_BREAK(condition, ...) if ( !(condition) ) { UF_MSG_ERROR("Assert failed: {}", #condition); break; }
#define UF_ASSERT_SAFE(condition) if ( !(condition) ) { UF_MSG_ERROR("Assertion failed: {}", #condition); }

#define UF_ASSERT_MSG(condition, ...) if ( !(condition) ) { UF_EXCEPTION("Assert failed: {} {}", #condition, ::fmt::format(__VA_ARGS__)); }
#define UF_ASSERT_BREAK_MSG(condition, ...) if ( !(condition) ) { UF_MSG_ERROR("Assert failed: {} {}", #condition, ::fmt::format(__VA_ARGS__)); break; }
#define UF_ASSERT_SAFE_MSG(condition, ...) if ( !(condition) ) { UF_MSG_ERROR("Assertion failed: {} {}", #condition, ::fmt::format(__VA_ARGS__)); }

#define UF_TIMER_TRACE_INIT() uf::Timer<long long> TIMER_TRACE;

#define UF_TIMER_TRACE(...) {\
	auto elapsed = TIMER_TRACE.elapsed().asMicroseconds();\
	if ( elapsed > 0 ) UF_MSG_DEBUG("{} us\t{}", TIMER_TRACE.elapsed().asMicroseconds(), ::fmt::format(__VA_ARGS__));\
}

#define UF_TIMER_TRACE_RESET(...) {\
	auto elapsed = TIMER_TRACE.elapsed().asMicroseconds();\
	if ( elapsed > 0 ) UF_MSG_DEBUG("{} us\t{}", TIMER_TRACE.elapsed().asMicroseconds(), ::fmt::format(__VA_ARGS__));\
	TIMER_TRACE.reset();\
}

#define UF_TIMER_MULTITRACE_START(...)\
	UF_TIMER_TRACE_INIT();\
	long long TIMER_TRACE_PREV = 0, TIMER_TRACE_CUR = 0;\
	UF_MSG_DEBUG(__VA_ARGS__);\

#define UF_TIMER_MULTITRACE(...) {\
	TIMER_TRACE_CUR = TIMER_TRACE.elapsed().asMicroseconds();\
	UF_MSG_DEBUG("{} us\t{} us\t{}", TIMER_TRACE_CUR, (TIMER_TRACE_CUR - TIMER_TRACE_PREV), ::fmt::format(__VA_ARGS__));\
	TIMER_TRACE_PREV = TIMER_TRACE_CUR;\
}

#define UF_TIMER_MULTITRACE_END(...) UF_MSG_DEBUG(__VA_ARGS__);

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
#define CLAMP(X, LO, HI) MAX(LO, MIN(HI, X))
#define LENGTH_OF(X) *(&X + 1) - X
#define FOR_ARRAY(X) for ( auto i = 0; i < LENGTH_OF(X); ++i )

#define ALIGNED_SIZE(V, A) ((V + A - 1) & ~(A - 1))

#define UF_MSG_PEEK(X) #X": {}", X

#if UF_ENV_DREAMCAST
	#define DC_STATS() {\
		UF_MSG_DEBUG(spec::dreamcast::malloc_stats());\
		UF_MSG_DEBUG(spec::dreamcast::pvr_malloc_stats());\
	}
#endif

#if __STDCPP_FLOAT16_T__ && !defined(UF_USE_FLOAT16)
	#define UF_USE_FLOAT16 1
#endif
#if __STDCPP_BFLOAT16_T__ && !defined(UF_USE_BFLOAT16)
	#define UF_USE_BFLOAT16 1
#endif

#if UF_USE_FLOAT16 || UF_USE_BFLOAT16
	#include <stdfloat>
	using float16_t = std::float16_t;
	using bfloat16_t = std::bfloat16_t;

	using float16 = float16_t;
	using bfloat16 = bfloat16_t;
#endif