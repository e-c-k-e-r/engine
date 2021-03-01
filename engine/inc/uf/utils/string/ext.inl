template<typename T>
std::string /*UF_API*/ uf::string::join( const T& container, const std::string& token, bool trailing ) {
	std::stringstream ss;
	size_t len = container.size();
	for ( size_t i = 0; i < len; ++i ) {
		ss << container[i];
		if ( trailing || i + 1 < len ) ss << token;
	}
	return ss.str();
}
template<typename T>
std::string /*UF_API*/ uf::string::toString( const T& var ) {
	std::stringstream ss;
	ss << &var;
	return ss.str();
}