#include <uf/ext/valve/common.h>

bool impl::parseKeyValue( const uf::stl::string& line, uf::stl::string& key, uf::stl::string& value ) {
	size_t q1 = line.find('"'); if ( q1 == uf::stl::string::npos ) return false;
	size_t q2 = line.find('"', q1 + 1); if ( q2 == uf::stl::string::npos ) return false;
	size_t q3 = line.find('"', q2 + 1); if ( q3 == uf::stl::string::npos ) return false;
	size_t q4 = line.find('"', q3 + 1); if ( q4 == uf::stl::string::npos ) return false;

	key = line.substr( q1 + 1, q2 - q1 - 1 );
	value = line.substr( q3 + 1, q4 - q3 - 1 );
	return true;
}

uf::stl::string impl::readString( std::ifstream& file ) {
	uf::stl::string str;
	char c;
	while ( file.get(c) && c != '\0' ) str += c;
	return str;
}

size_t impl::addMaterial( pod::Graph& graph, const uf::stl::string& name ) {
	auto& storage = uf::graph::getStorage( graph );

	size_t imageID = graph.images.size();
	auto imgKeyName = graph.images.emplace_back(name);
	auto& image = storage.images[imgKeyName].data;

	size_t textureID = graph.textures.size();
	auto texKeyName = graph.textures.emplace_back(name);
	storage.textures[texKeyName].index = imageID;

	size_t materialID = graph.materials.size();
	auto matKeyName = graph.materials.emplace_back(name);
	auto& material = storage.materials[matKeyName];
	material.indexAlbedo = textureID;
	material.colorBase = {1.0f, 1.0f, 1.0f, 1.0f};
	material.factorMetallic = 0.0f;
	material.factorRoughness = 1.0f;
	material.factorOcclusion = 1.0f;

	return materialID;
}

ext::json::Value impl::processValue( const uf::stl::string& v ) {
	if ( v.empty() ) return ext::json::Value(v);

	char* end = nullptr;
	long intVal = std::strtol( v.c_str(), &end, 10 );
	if ( end == v.c_str() + v.size() ) return ext::json::Value( intVal );

	float floatVal = std::strtof( v.c_str(), &end );
	if ( end == v.c_str() + v.size() ) return ext::json::Value( floatVal );

	return ext::json::Value( v );
}