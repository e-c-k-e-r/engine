#include "pod.inl"

template<typename T, size_t N>
uf::stl::string /*UF_API*/ uf::string::toString( const pod::Vector<T,N>& v ) {
	return uf::vector::toString(v);
}

template<typename T, size_t N>
ext::json::Value /*UF_API*/ ext::json::encode( const pod::Vector<T,N>& v ) {
	return uf::vector::encode(v);
}

template<typename T, size_t N>
pod::Vector<T,N>& /*UF_API*/ ext::json::decode( const ext::json::Value& json, pod::Vector<T,N>& v ) {
	return uf::vector::decode<T,N>(json, v);
}
template<typename T, size_t N>
pod::Vector<T,N> /*UF_API*/ ext::json::decode( const ext::json::Value& json, const pod::Vector<T,N>& v ) {
	return uf::vector::decode<T,N>(json, v);
}

template<typename T, size_t N>
size_t std::hash<pod::Vector<T,N>>::operator()( const pod::Vector<T,N>& v ) const {
	return uf::vector::hash( v );
}