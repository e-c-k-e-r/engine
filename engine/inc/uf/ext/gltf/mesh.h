#pragma once

#include <uf/utils/graphic/mesh.h>

namespace ext {
	namespace gltf {
		enum LoadMode {
			GENERATE_NORMALS 		= 0x1 << 0,
			APPLY_TRANSFORMS 		= 0x1 << 1,
			SEPARATE_MESHES 		= 0x1 << 2,
			RENDER 					= 0x1 << 3,
			COLLISION 				= 0x1 << 4,
			AABB 					= 0x1 << 5,
			DEFAULT_LOAD 			= 0x1 << 6,
			USE_ATLAS 				= 0x1 << 7,
			SKINNED 				= 0x1 << 8,
			FLIP_XY 				= 0x1 << 9,
		};
		typedef uint16_t load_mode_t;
		namespace mesh {
			struct ID {
				alignas(16) pod::Vector3f position;
				alignas(8) pod::Vector2f uv;
				alignas(16) pod::Vector3f normal;
				alignas(4) uint32_t id;

				static UF_API std::vector<ext::vulkan::VertexDescriptor> descriptor;

				bool operator==( const ID& that ) const {
					return 	this->position 	== that.position 	&&
							this->normal 	== that.normal 		&&
							this->uv 		== that.uv 			&&
							this->id 		== that.id;
				}
				bool operator!=( const ID& that ) const { return !(*this == that); }
			};
			struct Skinned {
				alignas(16) pod::Vector3f position;
				alignas(8) pod::Vector2f uv;
				alignas(16) pod::Vector3f normal;
				alignas(4) uint32_t id;
				alignas(16) pod::Vector4f joints;
				alignas(16) pod::Vector4f weights;

				static UF_API std::vector<ext::vulkan::VertexDescriptor> descriptor;

				bool operator==( const Skinned& that ) const {
					return 	this->position 	== that.position 	&&
							this->normal 	== that.normal 		&&
							this->uv 		== that.uv 			&&
							this->joints 	== that.joints 		&&
							this->weights 	== that.weights 	&&
							this->id 		== that.id;
				}
				bool operator!=( const Skinned& that ) const { return !(*this == that); }
			};
		}
		typedef uf::BaseMesh<ext::gltf::mesh::Skinned, uint32_t> mesh_t;
		typedef uf::BaseMesh<ext::gltf::mesh::Skinned, uint32_t> skinned_mesh_t;
	}
}

namespace std {
	template<> struct hash<ext::gltf::mesh::ID> {
		size_t operator()(ext::gltf::mesh::ID const& vertex) const {
			std::size_t seed = 3 + 2 + 3 + 1;
			std::hash<float> hasher;
			for ( size_t i = 0; i < 3; ++i ) seed ^= hasher( vertex.position[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 3; ++i ) seed ^= hasher( vertex.normal[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 2; ++i ) seed ^= hasher( vertex.uv[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= hasher( (float) vertex.id ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
		}
	};
	template<> struct hash<ext::gltf::mesh::Skinned> {
		size_t operator()(ext::gltf::mesh::Skinned const& vertex) const {
			std::size_t seed = 3 + 2 + 3 + 4 + 4 + 1;
			std::hash<float> hasher;
			for ( size_t i = 0; i < 3; ++i ) seed ^= hasher( vertex.position[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 3; ++i ) seed ^= hasher( vertex.normal[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 2; ++i ) seed ^= hasher( vertex.uv[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 4; ++i ) seed ^= hasher( (float) vertex.joints[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 4; ++i ) seed ^= hasher( vertex.weights[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= hasher( (float) vertex.id ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
		}
	};
}