#pragma once

#include <uf/config.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/engine/graph/graph.h>

namespace impl {
	const float sourceToMeters = 0.07f;
	
	typedef uf::Meshlet_T<uf::graph::mesh::Skinned, uint32_t> Meshlet;

	template<typename T>
	T str2vec( uf::stl::string string ) {
		string = uf::string::replace(string, " ", ","); // replace spaces with commas
		string = ::fmt::format("[{}]", string); // wrap as an array
		ext::json::Value j; ext::json::decode( j, string ); // parse JSON string
		return uf::vector::decode( j, T{} ); // parse JSON object
	}

	inline pod::Vector3f convertPos( const pod::Vector3f& vertex, float scale = impl::sourceToMeters ) {
		return pod::Vector3f{ -vertex.y, vertex.z, vertex.x } * scale;
	}

	ext::json::Value processValue( const uf::stl::string& v );
	uf::stl::string readString( std::ifstream& file );
	bool parseKeyValue( const uf::stl::string& line, uf::stl::string& key, uf::stl::string& value );
	size_t addMaterial( pod::Graph& graph, const uf::stl::string& name, int32_t& textureID );
	inline size_t addMaterial( pod::Graph& graph, const uf::stl::string& name ) {
		int32_t textureID = -1;
		return impl::addMaterial( graph, name, textureID );
	}
}