#include "pod.inl"
#include "class.inl"

template<typename T, size_t N>
std::string /*UF_API*/ uf::string::toString( const pod::Vector<T,N>& v ) {
	std::stringstream ss;
	ss << "Vector(";
	for ( size_t i = 0; i < N; ++i ) {
		ss << v[i];
		if ( i + 1 < N )
			ss << ", ";
	}
	ss << ")";
	return ss.str();
}