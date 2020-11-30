#include <uf/ext/gltf/mesh.h>

std::vector<ext::vulkan::VertexDescriptor> ext::gltf::mesh::ID::descriptor = {
	{
		VK_FORMAT_R32G32B32_SFLOAT,
		offsetof(ext::gltf::mesh::ID, position)
	},
	{
		VK_FORMAT_R32G32_SFLOAT,
		offsetof(ext::gltf::mesh::ID, uv)
	},
	{
		VK_FORMAT_R32G32B32_SFLOAT,
		offsetof(ext::gltf::mesh::ID, normal)
	},
	{
		VK_FORMAT_R32G32B32A32_SFLOAT,
		offsetof(ext::gltf::mesh::ID, tangent)
	},
	{
		VK_FORMAT_R32_UINT,
		offsetof(ext::gltf::mesh::ID, id)
	}
};
std::vector<ext::vulkan::VertexDescriptor> ext::gltf::mesh::Skinned::descriptor = {
	{
		VK_FORMAT_R32G32B32_SFLOAT,
		offsetof(ext::gltf::mesh::Skinned, position)
	},
	{
		VK_FORMAT_R32G32_SFLOAT,
		offsetof(ext::gltf::mesh::Skinned, uv)
	},
	{
		VK_FORMAT_R32G32B32_SFLOAT,
		offsetof(ext::gltf::mesh::Skinned, normal)
	},
	{
		VK_FORMAT_R32G32B32A32_SFLOAT,
		offsetof(ext::gltf::mesh::Skinned, tangent)
	},
	{
		VK_FORMAT_R32_UINT,
		offsetof(ext::gltf::mesh::Skinned, id)
	},
	{
		VK_FORMAT_R32G32B32A32_SFLOAT,
		offsetof(ext::gltf::mesh::Skinned, joints)
	},
	{
		VK_FORMAT_R32G32B32A32_SFLOAT,
		offsetof(ext::gltf::mesh::Skinned, weights)
	},
};