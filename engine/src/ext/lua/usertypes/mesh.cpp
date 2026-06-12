#include <uf/ext/lua/lua.h>
#if UF_USE_LUA
#include <uf/utils/mesh/mesh.h>

namespace binds {
	void updateDescriptor( uf::Mesh& self ) {
		self.updateDescriptor();
	}

	std::tuple<const uf::Mesh::View*, size_t> fetchView( uf::Mesh& self, size_t triID ) {
		const auto* view = uf::mesh::fetchView( self, triID );
		return std::make_tuple( view, triID );
	}
	const pod::DrawCommand& fetchDrawCommand( uf::Mesh& mesh, size_t triID ) {
		return uf::mesh::fetchDrawCommand( mesh, triID );
	}

	size_t fetchIndex( const uf::Mesh::View& view, const uf::stl::string& name, size_t index ) {
		return uf::mesh::fetchIndex( view, name, index );
	}

	uf::stl::vector<float> fetchVertexAttribute( const uf::Mesh::View& view, const uf::stl::string& name, size_t index ) {
		uf::stl::vector<float> res;
		if ( !view.has( name ) ) return res;
		const auto& attr = view[name];
		if ( !attr.valid() ) return res;

		size_t comps = attr.components();
		auto type = attr.type();

		const uint8_t* ptr = static_cast<const uint8_t*>(attr.data(view.vertex.first + index));

		for ( size_t i = 0; i < comps; ++i ) {
			switch ( type ) {
				case uf::renderer::enums::Type::BYTE: res.emplace_back( (float)( reinterpret_cast<const int8_t*>(ptr)[i] ) ); break;
				case uf::renderer::enums::Type::UBYTE: res.emplace_back( (float)( reinterpret_cast<const uint8_t*>(ptr)[i] ) ); break;
				case uf::renderer::enums::Type::SHORT: res.emplace_back( (float)( reinterpret_cast<const int16_t*>(ptr)[i] ) ); break;
				case uf::renderer::enums::Type::USHORT: res.emplace_back( (float)( reinterpret_cast<const uint16_t*>(ptr)[i] ) ); break;
				case uf::renderer::enums::Type::INT: res.emplace_back( (float)( reinterpret_cast<const int32_t*>(ptr)[i] ) ); break;
				case uf::renderer::enums::Type::UINT: res.emplace_back( (float)( reinterpret_cast<const uint32_t*>(ptr)[i] ) ); break;
				case uf::renderer::enums::Type::FLOAT: res.emplace_back( (float)( reinterpret_cast<const float*>(ptr)[i] ) ); break;
				case uf::renderer::enums::Type::DOUBLE: res.emplace_back( (float)( reinterpret_cast<const double*>(ptr)[i] ) ); break;
				default: res.emplace_back( 0.0 ); break;
			}
		}

		return res;
	}
}

#include <uf/ext/lua/component.h>
UF_LUA_REGISTER_USERTYPE(uf::Mesh::View,
	UF_LUA_REGISTER_USERTYPE_MEMBER(uf::Mesh::View::indirectIndex)
)
UF_LUA_REGISTER_USERTYPE_AND_COMPONENT(uf::Mesh,
	UF_LUA_REGISTER_USERTYPE_DEFINE( updateDescriptor, UF_LUA_C_FUN( ::binds::updateDescriptor ) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( fetchView, UF_LUA_C_FUN( ::binds::fetchView ) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( fetchDrawCommand, UF_LUA_C_FUN( ::binds::fetchDrawCommand ) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( fetchIndex, UF_LUA_C_FUN( ::binds::fetchIndex ) )
)
#endif