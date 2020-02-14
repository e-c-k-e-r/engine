#include <uf/ext/gltf/mesh.h>
#include <uf/utils/graphic/descriptor.h>

std::vector<uf::renderer::VertexDescriptor> ext::gltf::mesh::ID::descriptor = {
	{
		uf::renderer::enums::Format::R32G32B32_SFLOAT,
		offsetof(ext::gltf::mesh::ID, position)
	},
	{
		uf::renderer::enums::Format::R32G32_SFLOAT,
		offsetof(ext::gltf::mesh::ID, uv)
	},
	{
		uf::renderer::enums::Format::R32G32B32_SFLOAT,
		offsetof(ext::gltf::mesh::ID, normal)
	},
	{
		uf::renderer::enums::Format::R32G32B32A32_SFLOAT,
		offsetof(ext::gltf::mesh::ID, tangent)
	},
	{
		uf::renderer::enums::Format::R32G32_SINT,
		offsetof(ext::gltf::mesh::ID, id)
	}
};
std::vector<uf::renderer::VertexDescriptor> ext::gltf::mesh::Skinned::descriptor = {
	{
		uf::renderer::enums::Format::R32G32B32_SFLOAT,
		offsetof(ext::gltf::mesh::Skinned, position)
	},
	{
		uf::renderer::enums::Format::R32G32_SFLOAT,
		offsetof(ext::gltf::mesh::Skinned, uv)
	},
	{
		uf::renderer::enums::Format::R32G32B32_SFLOAT,
		offsetof(ext::gltf::mesh::Skinned, normal)
	},
	{
		uf::renderer::enums::Format::R32G32B32A32_SFLOAT,
		offsetof(ext::gltf::mesh::Skinned, tangent)
	},
	{
		uf::renderer::enums::Format::R32G32_SINT,
		offsetof(ext::gltf::mesh::Skinned, id)
	},
	{
		uf::renderer::enums::Format::R32G32B32A32_SFLOAT,
		offsetof(ext::gltf::mesh::Skinned, joints)
	},
	{
		uf::renderer::enums::Format::R32G32B32A32_SFLOAT,
		offsetof(ext::gltf::mesh::Skinned, weights)
	},
};