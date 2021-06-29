template<typename T>
uf::stl::string /*UF_API*/ uf::string::join( const T& container, const uf::stl::string& token, bool trailing ) {
	uf::stl::stringstream ss;
	size_t len = container.size();
	for ( size_t i = 0; i < len; ++i ) {
		ss << container[i];
		if ( trailing || i + 1 < len ) ss << token;
	}
	return ss.str();
}
template<typename T>
uf::stl::string /*UF_API*/ uf::string::toString( const T& var ) {
	uf::stl::stringstream ss;
	ss << &var;
	return ss.str();
}