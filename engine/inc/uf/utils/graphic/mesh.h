#pragma once

#include <uf/utils/math/vector.h>
#include <uf/utils/math/matrix.h>
#include <uf/ext/vulkan/vulkan.h>
#include <uf/utils/graphic/descriptor.h>

#include <functional>
#include <unordered_map>
#include <meshoptimizer.h>

namespace pod {
	struct /*UF_API*/ Vertex_3F2F3F4F {
		alignas(16) pod::Vector3f position;
		alignas(8) pod::Vector2f uv;
		alignas(16) pod::Vector3f normal;
		alignas(16) pod::Vector4f color;

		static UF_API std::vector<ext::vulkan::VertexDescriptor> descriptor;
	/*
		bool operator==( const Vertex_3F2F3F4F& that ) const {
			return 	this->position 	== that.position 	&&
					this->normal 	== that.normal 		&&
					this->uv 		== that.uv 		&&
					this->color 	== that.color;
		}
		bool operator!=( const Vertex_3F2F3F4F& that ) const { return !(*this == that); }
	*/
	};
	struct /*UF_API*/ Vertex_3F2F3F32B {
		alignas(16) pod::Vector3f position;
		alignas(8) pod::Vector2f uv;
		alignas(16) pod::Vector3f normal;
		alignas(16) pod::Vector4t<uint8_t> color;

		static UF_API std::vector<ext::vulkan::VertexDescriptor> descriptor;
	/*
		bool operator==( const Vertex_3F2F3F32B& that ) const {
			return 	this->position 	== that.position 	&&
					this->normal 	== that.normal 		&&
					this->uv 		== that.uv 		&&
					this->color 	== that.color;
		}
		bool operator!=( const Vertex_3F2F3F32B& that ) const { return !(*this == that); }
	*/
	};
	struct /*UF_API*/ Vertex_3F3F3F {
		alignas(16) pod::Vector3f position;
		alignas(16) pod::Vector3f uv;
		alignas(16) pod::Vector3f normal;

		static UF_API std::vector<ext::vulkan::VertexDescriptor> descriptor;
	/*
		bool operator==( const Vertex_3F3F3F& that ) const {
			return 	this->position 	== that.position 	&&
					this->normal 	== that.normal 		&&
					this->uv 		== that.uv;
		}
		bool operator!=( const Vertex_3F3F3F& that ) const { return !(*this == that); }
	*/
	};
	struct /*UF_API*/ Vertex_3F2F3F1UI {
		alignas(16) pod::Vector3f position;
		alignas(8) pod::Vector2f uv;
		alignas(16) pod::Vector3f normal;
		alignas(4) uint32_t id;

		static UF_API std::vector<ext::vulkan::VertexDescriptor> descriptor;
	/*
		bool operator==( const Vertex_3F2F3F1UI& that ) const {
			return 	this->position 	== that.position 	&&
					this->normal 	== that.normal 		&&
					this->uv 		== that.uv 			&&
					this->id 		== that.id;
		}
		bool operator!=( const Vertex_3F2F3F1UI& that ) const { return !(*this == that); }
	*/
	};
	struct /*UF_API*/ Vertex_3F2F3F {
		alignas(16) pod::Vector3f position;
		alignas(8) pod::Vector2f uv;
		alignas(16) pod::Vector3f normal;

		static UF_API std::vector<ext::vulkan::VertexDescriptor> descriptor;
	/*
		bool operator==( const Vertex_3F2F3F& that ) const {
			return 	this->position 	== that.position 	&&
					this->normal 	== that.normal 		&&
					this->uv 		== that.uv;
		}
		bool operator!=( const Vertex_3F2F3F& that ) const { return !(*this == that); }
	*/
	};
	struct /*UF_API*/ Vertex_3F2F {
		alignas(16) pod::Vector3f position;
		alignas(8) pod::Vector2f uv;

		static UF_API std::vector<ext::vulkan::VertexDescriptor> descriptor;
	/*
		bool operator==( const Vertex_3F2F& that ) const {
			return 	this->position 	== that.position 	&&
					this->uv 		== that.uv;
		}
		bool operator!=( const Vertex_3F2F& that ) const { return !(*this == that); }
	*/
	};
	struct /*UF_API*/ Vertex_2F2F {
		alignas(8) pod::Vector2f position;
		alignas(8) pod::Vector2f uv;

		static UF_API std::vector<ext::vulkan::VertexDescriptor> descriptor;
	/*
		bool operator==( const Vertex_2F2F& that ) const {
			return 	this->position 	== that.position 	&&
					this->uv 		== that.uv;
		}
		bool operator!=( const Vertex_2F2F& that ) const { return !(*this == that); }
	*/
	};
	struct /*UF_API*/ Vertex_3F {
		alignas(16) pod::Vector3f position;

		static UF_API std::vector<ext::vulkan::VertexDescriptor> descriptor;
	/*
		bool operator==( const Vertex_3F& that ) const {
			return 	this->position 	== that.position;
		}
		bool operator!=( const Vertex_3F& that ) const { return !(*this == that); }
	*/
	};
}

namespace uf {
	template<typename T, typename U = uint32_t>
	class /*UF_API*/ BaseMesh : public BaseGeometry {
	public:
		typedef T vertex_t;
		typedef U indices_t;
		std::vector<vertex_t> vertices;
		std::vector<indices_t> indices;

		~BaseMesh();
		void updateDescriptor();
		void initialize( size_t = SIZE_MAX );
		void expand( bool = true );
		void destroy();

		uf::BaseMesh<T,U> simplify( float = 0.2f );
		void optimize( size_t = SIZE_MAX );
	};
}

namespace uf {
	typedef BaseMesh<pod::Vertex_3F2F3F32B> ColoredMesh;
	typedef BaseMesh<pod::Vertex_3F2F3F> Mesh;
	typedef BaseMesh<pod::Vertex_3F2F> GuiMesh;
	typedef BaseMesh<pod::Vertex_3F> LineMesh;

	template<size_t N = ext::vulkan::settings::maxViews>
	struct MeshDescriptor {
		struct {
			alignas(16) pod::Matrix4f model;
			alignas(16) pod::Matrix4f view[N];
			alignas(16) pod::Matrix4f projection[N];
		} matrices;
		alignas(16) pod::Vector4f color = { 1, 1, 1, 0 };
	};
/*
	struct StereoMeshDescriptor {
		struct {
			alignas(16) pod::Matrix4f model;
			alignas(16) pod::Matrix4f view[2];
			alignas(16) pod::Matrix4f projection[2];
		} matrices;
		alignas(16) pod::Vector4f color = { 1, 1, 1, 0 };
	};
*/
}

#include "mesh.inl"
/*
namespace std {
	template<> struct hash<pod::Vertex_3F2F3F4F> {
	    size_t operator()(pod::Vertex_3F2F3F4F const& vertex) const {
	       std::size_t seed = 3 + 3 + 2 + 4;
	       std::hash<float> hasher;
			for ( size_t i = 0; i < 3; ++i ) seed ^= hasher( vertex.position[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 3; ++i ) seed ^= hasher( vertex.normal[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 2; ++i ) seed ^= hasher( vertex.uv[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 4; ++i ) seed ^= hasher( vertex.color[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
	    }
	};
	template<> struct hash<pod::Vertex_3F2F3F32B> {
	    size_t operator()(pod::Vertex_3F2F3F32B const& vertex) const {
	       std::size_t seed = 3 + 3 + 2 + 4;
	       std::hash<float> hasher;
			for ( size_t i = 0; i < 3; ++i ) seed ^= hasher( vertex.position[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 3; ++i ) seed ^= hasher( vertex.normal[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 2; ++i ) seed ^= hasher( vertex.uv[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 4; ++i ) seed ^= hasher( vertex.color[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
	    }
	};
	template<> struct hash<pod::Vertex_3F2F3F1UI> {
	    size_t operator()(pod::Vertex_3F2F3F1UI const& vertex) const {
	       std::size_t seed = 3 + 2 + 3 + 1;
	       std::hash<float> hasher;
			for ( size_t i = 0; i < 3; ++i ) seed ^= hasher( vertex.position[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 3; ++i ) seed ^= hasher( vertex.normal[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 2; ++i ) seed ^= hasher( vertex.uv[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= hasher( (float) vertex.id ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
	    }
	};
	template<> struct hash<pod::Vertex_3F3F3F> {
	    size_t operator()(pod::Vertex_3F3F3F const& vertex) const {
	       std::size_t seed = 3 + 3 + 3;
	       std::hash<float> hasher;
			for ( size_t i = 0; i < 3; ++i ) seed ^= hasher( vertex.position[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 3; ++i ) seed ^= hasher( vertex.normal[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 3; ++i ) seed ^= hasher( vertex.uv[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
	    }
	};
    template<> struct hash<pod::Vertex_3F2F3F> {
	    size_t operator()(pod::Vertex_3F2F3F const& vertex) const {
	       std::size_t seed = 3 + 3 + 2;
	       std::hash<float> hasher;
			for ( size_t i = 0; i < 3; ++i ) seed ^= hasher( vertex.position[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 3; ++i ) seed ^= hasher( vertex.normal[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 2; ++i ) seed ^= hasher( vertex.uv[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
	    }
	};
    template<> struct hash<pod::Vertex_3F2F> {
	    size_t operator()(pod::Vertex_3F2F const& vertex) const {
	       std::size_t seed = 3 + 2;
	       std::hash<float> hasher;
			for ( size_t i = 0; i < 3; ++i ) seed ^= hasher( vertex.position[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 2; ++i ) seed ^= hasher( vertex.uv[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
	    }
    };
    template<> struct hash<pod::Vertex_2F2F> {
	    size_t operator()(pod::Vertex_2F2F const& vertex) const {
	       std::size_t seed = 2 + 2;
	       std::hash<float> hasher;
			for ( size_t i = 0; i < 2; ++i ) seed ^= hasher( vertex.position[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			for ( size_t i = 0; i < 2; ++i ) seed ^= hasher( vertex.uv[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
	    }
    };
    template<> struct hash<pod::Vertex_3F> {
	    size_t operator()(pod::Vertex_3F const& vertex) const {
	       std::size_t seed = 3;
	       std::hash<float> hasher;
			for ( size_t i = 0; i < 3; ++i ) seed ^= hasher( vertex.position[i] ) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
	    }
    };
}
*/