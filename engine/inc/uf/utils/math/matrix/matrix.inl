#include "pod.inl"
#include "class.inl"

template<typename T, size_t R, size_t C>
std::string /*UF_API*/ uf::string::toString( const pod::Matrix<T,R,C>& m ) {
	std::stringstream ss;
	ss << "Matrix(\n\t";
	for ( size_t c = 0; c < C; ++c ) {
		for ( size_t r = 0; r < R; ++r ) {
			ss << m[r+c*C] << ", ";
		}
		if ( c + 1 < C ) ss << "\n\t";
	}
	ss << "\n)";
	return ss.str();
}