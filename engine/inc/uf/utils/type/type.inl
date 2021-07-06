template<typename T>
const uf::typeInfo::index_t uf::typeInfo::getIndex() {
	return TYPE(T);
}
template<typename T>
void uf::typeInfo::registerType( const uf::stl::string& pretty ) {
	return registerType(getIndex<T>(), sizeof(T), pretty);
}
template<typename T>
const pod::TypeInfo& uf::typeInfo::getType() {
	return getType(getIndex<T>());
}