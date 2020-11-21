#pragma once

#include <uf/utils/graphic/mesh.h>

namespace ext {
	namespace gltf {
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
				alignas(16) pod::Vector4ui joints;
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