#pragma once

#include <uf/utils/math/vector.h>
#include <uf/utils/math/matrix.h>

#include <functional>
#include <uf/utils/memory/unordered_map.h>

#if UF_USE_VULKAN
#include <uf/ext/vulkan/enums.h>
#define RENDERER vulkan
#elif UF_USE_OPENGL
#include <uf/ext/opengl/enums.h>
#define RENDERER opengl
#endif

namespace ext {
	namespace RENDERER {
	#if UF_ENV_DREAMCAST && !UF_USE_OPENGL_GLDC
		typedef uint16_t index_t;
	#else
		typedef uint32_t index_t;
	#endif
		
		struct /*UF_API*/ VertexDescriptor {
			ext::RENDERER::enums::Format::type_t format = ext::RENDERER::enums::Format::UNDEFINED; // VK_FORMAT_R32G32B32_SFLOAT
			size_t offset = 0; // offsetof(Vertex, position)
			size_t size = 0; // sizeof(Vertex::position::type_t)
			size_t components = 0; // Vertex::position::size
			ext::RENDERER::enums::Type::type_t type = 0; // ext::RENDERER::typeToEnum<Vertex::position::type_t>()
			uf::stl::string name = "";

			bool operator==( const VertexDescriptor& right ) const { return format == right.format && offset == right.offset && size == right.size && components == right.components && type == right.type && name == right.name; }
			bool operator!=( const VertexDescriptor& right ) const { return !(*this == right); };
		};
	}
}

namespace pod {
	struct UF_API Mesh {
	public:
		struct Attributes {
			struct {
				size_t size = 0;
				size_t length = 0;
				void* pointer = NULL;
			} vertex, index;

			typedef uf::stl::vector<ext::RENDERER::VertexDescriptor> descriptors_t;
			descriptors_t descriptor;
		} attributes;

		void generateIndices();
		virtual ~Mesh();
		virtual void updateDescriptor();
		virtual void resizeVertices( size_t );
		virtual void resizeIndices( size_t );
	};
}

namespace ext {
	namespace RENDERER {
		struct UF_API GraphicDescriptor {
		#if UF_GRAPHIC_DESCRIPTOR_USE_STRING
			typedef uf::stl::string hash_t;
		#else
			typedef size_t hash_t;
		#endif

			uf::stl::string renderMode = "";
			uf::stl::string pipeline = "";

			uint32_t renderTarget = 0;
			uint32_t subpass = 0;

			pod::Mesh::Attributes attributes;
			size_t indices = 0;

			struct {
				size_t vertex = 0;
				size_t index = 0;
			} offsets;

			ext::RENDERER::enums::PrimitiveTopology::type_t topology = ext::RENDERER::enums::PrimitiveTopology::TRIANGLE_LIST;
			ext::RENDERER::enums::PolygonMode::type_t fill = ext::RENDERER::enums::PolygonMode::FILL;
			ext::RENDERER::enums::CullMode::type_t cullMode = ext::RENDERER::enums::CullMode::BACK;
			ext::RENDERER::enums::Face::type_t frontFace = ext::RENDERER::enums::Face::CW;
			float lineWidth = 1.0f;

			struct {
				bool test = true;
				bool write = true;
				ext::RENDERER::enums::Compare::type_t operation = ext::RENDERER::enums::Compare::GREATER_OR_EQUAL;
				struct {
					bool enable = false;
					float constant = 0;
					float slope = 0;
					float clamp = 0;
				} bias;
			} depth;
			bool invalidated = false;
			hash_t hash() const;
			void parse( ext::json::Value& );
			bool operator==( const GraphicDescriptor& right ) const { return this->hash() == right.hash(); }
			bool operator!=( const GraphicDescriptor& right ) const { return this->hash() != right.hash(); }
		};
	}
}

#undef UF_RENDERER
#define UF_VERTEX_DESCRIPTION( TYPE, FORMAT, ATTRIBUTE ) {\
		uf::renderer::enums::Format::FORMAT,\
		offsetof(TYPE, ATTRIBUTE),\
		sizeof(decltype(TYPE::ATTRIBUTE)::type_t),\
		decltype(TYPE::ATTRIBUTE)::size,\
		ext::RENDERER::typeToEnum<decltype(TYPE::ATTRIBUTE)::type_t>(),\
		#ATTRIBUTE\
	},

#define UF_VERTEX_DESCRIPTOR( TYPE, ... )\
	uf::stl::vector<uf::renderer::VertexDescriptor> TYPE::descriptor = { __VA_ARGS__ };


#if UF_USE_VULKAN
	namespace uf {
		namespace renderer = ext::vulkan;
	}
#elif UF_USE_OPENGL
	namespace uf {
		namespace renderer = ext::opengl;
	}
#endif

namespace uf {
	template<typename T, typename U = uf::renderer::index_t>
	class /*UF_API*/ Mesh : public pod::Mesh {
	public:
		typedef T vertex_t;
		typedef U index_t;
		typedef vertex_t vertices_t;
		typedef index_t indices_t;
		uf::stl::vector<vertex_t> vertices;
		uf::stl::vector<index_t> indices;

		void initialize( size_t = SIZE_MAX );
		void destroy();

		void expand( bool = true );
		uf::Mesh<T,U> simplify( float = 0.2f );
		void optimize( size_t = SIZE_MAX );
		void insert( const uf::Mesh<T,U>& mesh );

		virtual void updateDescriptor();
		virtual void resizeVertices( size_t );
		virtual void resizeIndices( size_t );
	};
}

namespace pod {
	struct /*UF_API*/ Vertex_3F2F3F4F {
		/*alignas(16)*/ pod::Vector3f position;
		/*alignas(8)*/ pod::Vector2f uv;
		/*alignas(16)*/ pod::Vector3f normal;
		/*alignas(16)*/ pod::Vector4f color;

		static UF_API uf::stl::vector<uf::renderer::VertexDescriptor> descriptor;
	};
	struct /*UF_API*/ Vertex_3F2F3F32B {
		/*alignas(16)*/ pod::Vector3f position;
		/*alignas(8)*/ pod::Vector2f uv;
		/*alignas(16)*/ pod::Vector3f normal;
		/*alignas(16)*/ pod::Vector4t<uint8_t> color;

		static UF_API uf::stl::vector<uf::renderer::VertexDescriptor> descriptor;
	};
	struct /*UF_API*/ Vertex_3F3F3F {
		/*alignas(16)*/ pod::Vector3f position;
		/*alignas(16)*/ pod::Vector3f uv;
		/*alignas(16)*/ pod::Vector3f normal;

		static UF_API uf::stl::vector<uf::renderer::VertexDescriptor> descriptor;
	};
	struct /*UF_API*/ Vertex_3F2F3F1UI {
		/*alignas(16)*/ pod::Vector3f position;
		/*alignas(8)*/ pod::Vector2f uv;
		/*alignas(16)*/ pod::Vector3f normal;
		/*alignas(4)*/ pod::Vector1ui id;

		static UF_API uf::stl::vector<uf::renderer::VertexDescriptor> descriptor;
	};
	struct /*UF_API*/ Vertex_3F2F3F {
		/*alignas(16)*/ pod::Vector3f position;
		/*alignas(8)*/ pod::Vector2f uv;
		/*alignas(16)*/ pod::Vector3f normal;

		static UF_API uf::stl::vector<uf::renderer::VertexDescriptor> descriptor;
	};
	struct /*UF_API*/ Vertex_3F2F {
		/*alignas(16)*/ pod::Vector3f position;
		/*alignas(8)*/ pod::Vector2f uv;

		static UF_API uf::stl::vector<uf::renderer::VertexDescriptor> descriptor;
	};
	struct /*UF_API*/ Vertex_2F2F {
		/*alignas(8)*/ pod::Vector2f position;
		/*alignas(8)*/ pod::Vector2f uv;

		static UF_API uf::stl::vector<uf::renderer::VertexDescriptor> descriptor;
	};
	struct /*UF_API*/ Vertex_3F {
		/*alignas(16)*/ pod::Vector3f position;

		static UF_API uf::stl::vector<uf::renderer::VertexDescriptor> descriptor;
	};
}

#if 0
namespace uf {
	typedef BaseMesh<pod::Vertex_3F2F3F32B> ColoredMesh;
	typedef BaseMesh<pod::Vertex_3F2F3F> Mesh;
	typedef BaseMesh<pod::Vertex_3F> LineMesh;
	template<size_t N = 6>
	struct MeshDescriptor {
		struct {
			/*alignas(16)*/ pod::Matrix4f model;
			/*alignas(16)*/ pod::Matrix4f view[N];
			/*alignas(16)*/ pod::Matrix4f projection[N];
		} matrices;
		/*alignas(16)*/ pod::Vector4f color = { 1, 1, 1, 0 };
	};
}
#endif

#include <uf/utils/userdata/userdata.h>
namespace pod {
	struct UF_API VaryingMesh : public pod::Mesh {	
		pod::PointeredUserdata userdata;

		pod::Mesh& get();
		const pod::Mesh& get() const;
		void destroy();
		
		virtual void updateDescriptor();
		virtual void resizeVertices( size_t );
		virtual void resizeIndices( size_t );

		template<typename T, typename U = uf::renderer::index_t>
		bool is() const;

		template<typename T, typename U = uf::renderer::index_t>
		uf::Mesh<T,U>& get();
		
		template<typename T, typename U = uf::renderer::index_t>
		const uf::Mesh<T,U>& get() const;

		template<typename T, typename U = uf::renderer::index_t>
		void set( const uf::Mesh<T, U>& = {} );

		template<typename T, typename U = uf::renderer::index_t>
		void insert( const pod::VaryingMesh& mesh );

		template<typename T, typename U = uf::renderer::index_t>
		void insert( const uf::Mesh<T, U>& mesh );
	};
}

#include <uf/ext/meshopt/meshopt.h>
#include "mesh.inl"