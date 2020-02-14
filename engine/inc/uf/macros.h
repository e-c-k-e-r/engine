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
#define UF_DEBUG_PRINT() if ( UF_DEBUG ) uf::iostream << __FILE__ << ":" << __FUNCTION__ << "@" << __LINE__ << "\n";