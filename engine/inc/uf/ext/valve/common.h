#pragma once

#include <uf/config.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/engine/graph/graph.h>

#define UF_GRAPH_MESH_FORMAT uf::graph::mesh::Base, uint32_t

namespace impl {
	const float sourceToMeters = 0.07f;
	
	typedef uf::Meshlet_T<UF_GRAPH_MESH_FORMAT> Meshlet;

	template<typename T>
	T str2vec( uf::stl::string string ) {
		string = uf::string::join(uf::string::split(string, " "), ","); // replace spaces with commas
		string = ::fmt::format("[{}]", string); // wrap as an array
		ext::json::Value j; ext::json::decode( j, string ); // parse JSON string
		return uf::vector::decode( j, T{} ); // parse JSON object
	}

	inline pod::Vector3f convertPos( const pod::Vector3f& vertex, float scale = impl::sourceToMeters ) {
		return pod::Vector3f{ -vertex.y, vertex.z, vertex.x } * scale;
	}

	bool parseKeyValue( const uf::stl::string& line, uf::stl::string& key, uf::stl::string& value );
}