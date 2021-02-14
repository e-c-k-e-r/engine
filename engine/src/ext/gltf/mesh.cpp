#include <uf/ext/gltf/mesh.h>
#include <uf/utils/graphic/descriptor.h>

UF_VERTEX_DESCRIPTOR(ext::gltf::mesh::ID,
	UF_VERTEX_DESCRIPTION(ext::gltf::mesh::ID, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(ext::gltf::mesh::ID, R32G32_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(ext::gltf::mesh::ID, R32G32B32_SFLOAT, normal)
	UF_VERTEX_DESCRIPTION(ext::gltf::mesh::ID, R32G32B32A32_SFLOAT, tangent)
	UF_VERTEX_DESCRIPTION(ext::gltf::mesh::ID, R32G32_SINT, id)
);
UF_VERTEX_DESCRIPTOR(ext::gltf::mesh::Skinned,
	UF_VERTEX_DESCRIPTION(ext::gltf::mesh::Skinned, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(ext::gltf::mesh::Skinned, R32G32_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(ext::gltf::mesh::Skinned, R32G32B32_SFLOAT, normal)
	UF_VERTEX_DESCRIPTION(ext::gltf::mesh::Skinned, R32G32B32A32_SFLOAT, tangent)
	UF_VERTEX_DESCRIPTION(ext::gltf::mesh::Skinned, R32G32_SINT, id)
	UF_VERTEX_DESCRIPTION(ext::gltf::mesh::Skinned, R32G32B32A32_SFLOAT, joints)
	UF_VERTEX_DESCRIPTION(ext::gltf::mesh::Skinned, R32G32B32A32_SFLOAT, weights)
);