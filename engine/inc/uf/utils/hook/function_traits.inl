/*
template<typename T> 
struct function_traits;  

template<typename R, typename ...Args> 
struct function_traits<std::function<R(Args...)>> {
	static const size_t nargs = sizeof...(Args);

	typedef R result_type;

	template <size_t i>
	struct arg {
		typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
	};
};
*/

// For generic types that are functors, delegate to its 'operator()'
template <typename T>
struct function_traits : public function_traits<decltype(&T::operator())> {};

// for pointers to member function
template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...) const> {
    //enum { arity = sizeof...(Args) };
    static const size_t arguments = sizeof...(Args);
    template <size_t i>
	struct Arg {
		typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
	};
    typedef ReturnType return_type;
    typedef std::function<ReturnType (Args...)> type;
};

// for pointers to member function
template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...) > {
	static const size_t arguments = sizeof...(Args);
	template <size_t i>
	struct Arg {
		typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
	};
    typedef ReturnType return_type;
    typedef std::function<ReturnType (Args...)> type;
};

// for function pointers
template <typename ReturnType, typename... Args>
struct function_traits<ReturnType (*)(Args...)>  {
	static const size_t arguments = sizeof...(Args);
	template <size_t i>
	struct Arg {
		typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
	};
  typedef ReturnType return_type;
  typedef std::function<ReturnType (Args...)> type;
};