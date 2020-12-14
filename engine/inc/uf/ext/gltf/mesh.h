#pragma once

#include <uf/utils/graphic/mesh.h>

namespace ext {
	namespace gltf {
		enum LoadMode {
			RENDER 					= 0x1 << 0,
			COLLISION 				= 0x1 << 1,
			SEPARATE 				= 0x1 << 2,
			NORMALS 				= 0x1 << 3,
			LOAD 					= 0x1 << 4,
			ATLAS 					= 0x1 << 5,
			SKINNED 				= 0x1 << 6,
			INVERT 					= 0x1 << 7,
			TRANSFORM 				= 0x1 << 8,
		};
		typedef uint16_t load_mode_t;

		namespace mesh {
			struct ID {
				/*alignas(16)*/ pod::Vector3f position;
				/*alignas(8)*/ pod::Vector2f uv;
				/*alignas(16)*/ pod::Vector3f normal;
				/*alignas(16)*/ pod::Vector4f tangent;
				/*alignas(8)*/ pod::Vector2ui id;

				static UF_API std::vector<ext::vulkan::VertexDescriptor> descriptor;
			/*
				bool operator==( const ID& that ) const {
					return 	this->position 	== that.position 	&&
							this->uv 		== that.uv 			&&
							this->normal 	== that.normal 		&&
							this->tangent 	== that.tangent 	&&
							this->id 		== that.id;
				}
				bool operator!=( const ID& that ) const { return !(*this == that); }
			*/
			};
			struct Skinned {
				/*alignas(16)*/ pod::Vector3f position;
				/*alignas(8)*/ pod::Vector2f uv;
				/*alignas(16)*/ pod::Vector3f normal;
				/*alignas(16)*/ pod::Vector4f tangent;
				/*alignas(8)*/ pod::Vector2ui id;
				/*alignas(16)*/ pod::Vector4f joints;
				/*alignas(16)*/ pod::Vector4f weights;

				static UF_API std::vector<ext::vulkan::VertexDescriptor> descriptor;
			/*
				bool operator==( const Skinned& that ) const {
					return 	this->position 	== that.position 	&&
							this->uv 		== that.uv 			&&
							this->normal 	== that.normal 		&&
							this->tangent 	== that.tangent 	&&
							this->joints 	== that.joints 		&&
							this->weights 	== that.weights 	&&
							this->id 		== that.id;
				}
				bool operator!=( const Skinned& that ) const { return !(*this == that); }
			*/
			};
		}
		typedef uf::BaseMesh<ext::gltf::mesh::Skinned, uint32_t> mesh_t;
		typedef uf::BaseMesh<ext::gltf::mesh::Skinned, uint32_t> skinned_mesh_t;
	}
}
/*
namespace std {
	template<> struct hash<ext::gltf::mesh::ID> {
		size_t operator()(ext::gltf::mesh::ID const& vertex) const {
			std::size_t seed = 3 + 2 + 3 + 4 + 2;
			std::hash<float> hasher;
			for ( size_t i = 0; i < 3; ++i ) seed ^= hasher( vertex.position[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 2; ++i ) seed ^= hasher( vertex.uv[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 3; ++i ) seed ^= hasher( vertex.normal[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 4; ++i ) seed ^= hasher( vertex.tangent[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 2; ++i ) seed ^= hasher( (float) vertex.id[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
		}
	};
	template<> struct hash<ext::gltf::mesh::Skinned> {
		size_t operator()(ext::gltf::mesh::Skinned const& vertex) const {
			std::size_t seed = 3 + 2 + 3 + 4 + 4 + 4 + 2;
			std::hash<float> hasher;
			for ( size_t i = 0; i < 3; ++i ) seed ^= hasher( vertex.position[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 2; ++i ) seed ^= hasher( vertex.uv[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 3; ++i ) seed ^= hasher( vertex.normal[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 4; ++i ) seed ^= hasher( vertex.tangent[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 2; ++i ) seed ^= hasher( (float) vertex.id[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 4; ++i ) seed ^= hasher( (float) vertex.joints[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 4; ++i ) seed ^= hasher( vertex.weights[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
		}
	};
}
*/