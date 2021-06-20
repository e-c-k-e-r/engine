#pragma once 	

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
			std::string name = "";
		};
	}
}

namespace uf {
	struct UF_API BaseGeometry {
	public:
		struct {
			struct {
				size_t size = 0;
				void* pointer = NULL;
				size_t length = 0;
			} vertex, index;
			std::vector<ext::RENDERER::VertexDescriptor> descriptor;
		} attributes;

		void generateIndices();
		virtual ~BaseGeometry();
		virtual void updateDescriptor();
		virtual void resizeVertices( size_t );
		virtual void resizeIndices( size_t );
	};
}

namespace ext {
	namespace RENDERER {
		struct UF_API GraphicDescriptor {
		#if UF_GRAPHIC_DESCRIPTOR_USE_STRING
			typedef std::string hash_t;
		#else
			typedef size_t hash_t;
		#endif

			std::string renderMode = "";
			std::string pipeline = "";

			uint32_t renderTarget = 0;
			uint32_t subpass = 0;

			uf::BaseGeometry geometry;
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
	std::vector<uf::renderer::VertexDescriptor> TYPE::descriptor = { __VA_ARGS__ };
	