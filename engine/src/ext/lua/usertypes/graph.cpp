#include <uf/ext/lua/lua.h>
#if UF_USE_LUA
#include <uf/engine/graph/graph.h>

namespace binds {
	uf::stl::string getMaterialName( pod::Graph& self, size_t id ) {
		return uf::graph::getMaterialName( self, id );
	}
	pod::Material getMaterial( pod::Graph& self, size_t id ) {
		return uf::graph::getMaterial( self, id );
	}
	pod::Primitive getPrimitive( pod::Graph& self, size_t id ) {
		return uf::graph::getPrimitive( self, id );
	}
	pod::Instance getInstance( pod::Graph& self, size_t id ) {
		return uf::graph::getInstance( self, id );
	}
}

#include <uf/ext/lua/component.h>
UF_LUA_REGISTER_USERTYPE(pod::DrawCommand,
    UF_LUA_REGISTER_USERTYPE_MEMBER(pod::DrawCommand::indices),
    UF_LUA_REGISTER_USERTYPE_MEMBER(pod::DrawCommand::instances),
    UF_LUA_REGISTER_USERTYPE_MEMBER(pod::DrawCommand::indexID),
    UF_LUA_REGISTER_USERTYPE_MEMBER(pod::DrawCommand::vertexID),
    UF_LUA_REGISTER_USERTYPE_MEMBER(pod::DrawCommand::instanceID),
    UF_LUA_REGISTER_USERTYPE_MEMBER(pod::DrawCommand::materialID)
)

UF_LUA_REGISTER_USERTYPE(pod::Instance,
    UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Instance::materialID),
    UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Instance::primitiveID),
    UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Instance::objectID)
)

UF_LUA_REGISTER_USERTYPE(pod::Primitive,
    UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Primitive::drawCommand),
    UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Primitive::instance)
)

UF_LUA_REGISTER_USERTYPE(pod::Material,
    UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Material::colorBase),
    UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Material::colorEmissive),
    UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Material::factorMetallic),
    UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Material::factorRoughness),
    UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Material::factorOcclusion),
    UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Material::factorAlphaCutoff),
    UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Material::indexAlbedo),
    UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Material::indexNormal),
    UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Material::indexEmissive),
    UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Material::indexOcclusion),
    UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Material::indexMetallicRoughness),
    UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Material::modeCull),
    UF_LUA_REGISTER_USERTYPE_MEMBER(pod::Material::modeAlpha)
)

UF_LUA_REGISTER_USERTYPE_AND_COMPONENT(pod::Graph,
	UF_LUA_REGISTER_USERTYPE_DEFINE( getMaterialName, UF_LUA_C_FUN( ::binds::getMaterialName ) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( getMaterial, UF_LUA_C_FUN( ::binds::getMaterial ) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( getPrimitive, UF_LUA_C_FUN( ::binds::getPrimitive ) ),
	UF_LUA_REGISTER_USERTYPE_DEFINE( getInstance, UF_LUA_C_FUN( ::binds::getInstance ) )
)
#endif