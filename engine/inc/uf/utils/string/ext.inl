template<typename T>
std::string /*UF_API*/ uf::string::toString( const T& var ) {
	std::stringstream ss;
	ss << &var;
	return ss.str();
}