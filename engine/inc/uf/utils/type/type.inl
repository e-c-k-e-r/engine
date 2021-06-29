template<typename T>
const std::type_index uf::typeInfo::getIndex() {
	return typeid(T);
}
template<typename T>
void uf::typeInfo::registerType( const uf::stl::string& pretty ) {
	return registerType(getIndex<T>(), sizeof(T), pretty);
}
template<typename T>
const pod::TypeInfo& uf::typeInfo::getType() {
	return getType(getIndex<T>());
}