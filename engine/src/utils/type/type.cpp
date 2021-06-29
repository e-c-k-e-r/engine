#include <uf/utils/type/type.h>
#include <uf/utils/io/iostream.h>

#include <cxxabi.h>
#include <memory>
namespace {
	uf::stl::string demangle( const uf::stl::string& mangled ) {

		int status = -4;
		std::unique_ptr<char, void(*)(void*)> demangled{
	        abi::__cxa_demangle(mangled.c_str(), NULL, NULL, &status),
	        std::free
	    };
	    return ( status == 0 ) ? demangled.get() : mangled;
	/*
		int status;
		char* pointer = abi::__cxa_demangle(mangled.c_str(), 0, 0, &status);
		uf::stl::string demangled;
		demangled.copy( pointer, strlen(pointer) );
		free(pointer);
		return demangled;
	*/
	}
}

uf::stl::unordered_map<uf::typeInfo::index_t, pod::TypeInfo>* uf::typeInfo::types = NULL;
void uf::typeInfo::registerType( const index_t& index, size_t size, const uf::stl::string& pretty ) {
	if ( !uf::typeInfo::types ) uf::typeInfo::types = new uf::stl::unordered_map<uf::typeInfo::index_t, pod::TypeInfo>;
	bool exists = uf::typeInfo::types->count(index) > 0;
	auto& type = (*uf::typeInfo::types)[index];
	if ( !exists || type.size < size ) type.size = size;
	if ( !exists ) type.hash = index.hash_code();
	if ( !exists ) type.name.compiler = index.name();
	if ( !exists || pretty != "" ) type.name.pretty = pretty != "" ? pretty : demangle( type.name.compiler );

	// if ( !exists ) uf::iostream << "Registered type " << type.name.pretty << " (Mangled: " << type.name.compiler << ") of size " << size << "\n";
}
const pod::TypeInfo& UF_API uf::typeInfo::getType( size_t hash ) {
	if ( !uf::typeInfo::types ) uf::typeInfo::types = new uf::stl::unordered_map<uf::typeInfo::index_t, pod::TypeInfo>;
	auto& types = *uf::typeInfo::types;
	for ( auto& pair : types ) {
		if ( pair.second.hash == hash ) return pair.second;
	}
	static pod::TypeInfo null;
	return null;
}
const pod::TypeInfo& uf::typeInfo::getType( const index_t& index ) {
	if ( !uf::typeInfo::types ) uf::typeInfo::types = new uf::stl::unordered_map<uf::typeInfo::index_t, pod::TypeInfo>;
	return (*uf::typeInfo::types)[index];
}