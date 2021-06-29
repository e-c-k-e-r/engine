#include "pod.inl"
#include "class.inl"

template<typename T, size_t R, size_t C>
uf::stl::string /*UF_API*/ uf::string::toString( const pod::Matrix<T,R,C>& m ) {
	return uf::matrix::toString(m);
}

template<typename T, size_t R, size_t C>
ext::json::Value /*UF_API*/ ext::json::encode( const pod::Matrix<T,R,C>& v ) {
	return uf::matrix::encode(v);
}

template<typename T, size_t R, size_t C>
pod::Matrix<T,R,C>& /*UF_API*/ ext::json::decode( const ext::json::Value& json, pod::Matrix<T,R,C>& m ) {
	return uf::matrix::decode<T,R,C>(json, m);
}
template<typename T, size_t R, size_t C>
pod::Matrix<T,R,C> /*UF_API*/ ext::json::decode( const ext::json::Value& json, const pod::Matrix<T,R,C>& m ) {
	return uf::matrix::decode<T,R,C>(json, m);
}