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
		struct UF_API AttributeDescriptor {
			// essential for vertex input
			size_t offset = 0;
			size_t size = 0;
			ext::RENDERER::enums::Format::type_t format = ext::RENDERER::enums::Format::UNDEFINED;
			// not as essential
			uf::stl::string name = "";
			ext::RENDERER::enums::Type::type_t type = 0;
			size_t components = 0;
			// somewhat essential
			size_t length = 0;
			void* pointer = NULL;
			
			bool operator==( const AttributeDescriptor& right ) const { return 
				offset == right.offset && 
				size == right.size && 
				format == right.format &&
				name == right.name &&
				type == right.type && 
				components == right.components;
			}
			bool operator!=( const AttributeDescriptor& right ) const { return !(*this == right); };
		};
	}
}

namespace uf {
	struct UF_API Mesh {
	public:
		static bool defaultInterleaved;
		typedef uf::stl::vector<uint8_t> buffer_t;
		struct Attribute {
			ext::RENDERER::AttributeDescriptor descriptor;
			 int32_t buffer = -1;
			size_t offset = 0;
		};
		struct Input {
			uf::stl::vector<Attribute> attributes;
			size_t count = 0; // how many elements is the input using
			size_t first = 0; // base index to start from
			size_t stride = 0; // size of one element in the input's buffer
			size_t offset = 0; // bytes to offset from within the associated buffer
			 int32_t interleaved = -1; // index to interleaved buffer if in bounds

			 struct Settings {
			 	bool interleaved = false;
			 } settings;
		} vertex, index, instance, indirect;
		uf::stl::vector<buffer_t> buffers;
	protected:
		void _destroy( uf::Mesh::Input& input );
		void _bind( bool interleaved = uf::Mesh::defaultInterleaved );
		void _updateDescriptor( uf::Mesh::Input& input );

		bool _hasV( const uf::Mesh::Input& input, const uf::stl::vector<ext::RENDERER::AttributeDescriptor>& descriptors ) const;
		bool _hasV( const uf::Mesh::Input& input, const uf::Mesh::Input& src ) const;
		void _bindV( uf::Mesh::Input& input, const uf::stl::vector<ext::RENDERER::AttributeDescriptor>& descriptors );
		void _resizeVs( uf::Mesh::Input& input, size_t count );
		void _reserveVs( uf::Mesh::Input& input, size_t count );
		void _insertV( uf::Mesh::Input& input, const void* data );
		void _insertV( uf::Mesh::Input& input, const uf::Mesh& mesh, const uf::Mesh::Input& srcInput );
		void _insertVs( uf::Mesh::Input& input, const void* data, size_t size );
		
		template<typename T> inline bool _hasV( const uf::Mesh::Input& input ) const { return _hasV( input, T::descriptors ); }
		template<typename T> inline void _bindV( uf::Mesh::Input& input ) { return _bindV( input, T::descriptor ); }
		template<typename T> inline void _insertV( uf::Mesh::Input& input, const T& vertex ) { return _insertV( input, (const void*) &vertex ); }
		template<typename T> inline void _insertVs( uf::Mesh::Input& input, const uf::stl::vector<T>& vs ) { return _insertVs( input, (const void*) vs.data(), vs.size() ); }

		void _bindI( uf::Mesh::Input& input, size_t size, ext::RENDERER::enums::Type::type_t type, size_t count = 1 );
		void _reserveIs( uf::Mesh::Input& input, size_t count, size_t i = 0 );
		void _resizeIs( uf::Mesh::Input& input, size_t count, size_t i = 0 );
		void _insertI( uf::Mesh::Input& input, const void* data, size_t i );
		void _insertI( uf::Mesh::Input& input, const uf::Mesh& mesh, const uf::Mesh::Input& srcInput );
		void _insertIs( uf::Mesh::Input& input, const void* data, size_t size, size_t i );
		
		template<typename U> inline void _bindI( uf::Mesh::Input& input, size_t indices = 1 ) { return _bindI( input, sizeof(U), ext::RENDERER::typeToEnum<U>(), indices ); }
		template<typename U> inline void _insertI( uf::Mesh::Input& input, U index, size_t i = 0 ) { return _insertI( input, (const void*) &index, i ); }
		template<typename U> inline void _insertIs( uf::Mesh::Input& input, const uf::stl::vector<U>& is, size_t i = 0 ) { return _insertIs( input, (const void*) is.data(), is.size(), i ); }
	public:
		void initialize();
		void destroy();

		uf::Mesh interleave() const;
		void updateDescriptor();
		void insert( const uf::Mesh& );
		
		void generateIndices();
		void generateIndirect();

		bool isInterleaved( size_t ) const;

		void print() const;

		inline bool hasVertex( const uf::stl::vector<ext::RENDERER::AttributeDescriptor>& descriptors ) const { return _hasV( vertex, descriptors ); }
		inline bool hasVertex( const uf::Mesh& mesh ) const { return _hasV( vertex, mesh.vertex ); }
		inline void bindVertex( const uf::stl::vector<ext::RENDERER::AttributeDescriptor>& descriptors ) { return _bindV( vertex, descriptors ); }
		inline void resizeVertices( size_t count ) { return _resizeVs( vertex, count ); }
		inline void reserveVertices( size_t count ) { return _reserveVs( vertex, count ); }
		inline void insertVertex( const void* data ) { return _insertV( vertex, data ); }
		inline void insertVertices( const void* data, size_t size ) { return _insertVs( vertex, data, size ); }
		inline void insertVertices( const uf::Mesh& mesh ) { return _insertV( vertex, mesh, mesh.vertex ); }
		inline void updateVertexDescriptor() { return _updateDescriptor( vertex ); }

		template<typename T> inline bool hasVertex() const { return _hasV( vertex, T::descriptors ); }
		template<typename T> inline void bindVertex() { return _bindV( vertex, T::descriptor ); }
		template<typename T> inline void insertVertex( const T& v ) { return _insertV( vertex, (const void*) &v ); }
		template<typename T> inline void insertVertices( const uf::stl::vector<T>& vertices ) { return _insertVs( vertex, (const void*) vertices.data(), vertices.size() ); }

		inline void bindIndex( size_t size, ext::RENDERER::enums::Type::type_t type, size_t count = 1 ) { return _bindI( index, size, type, count ); }
		inline void reserveIndices( size_t count, size_t i = 0 ) { return _reserveIs( index, count, i ); }
		inline void resizeIndices( size_t count, size_t i = 0 ) { return _resizeIs( index, count, i ); }
		inline void insertIndex( const void* data, size_t i = 0 ) { return _insertI( index, data, i ); }
		inline void insertIndices( const void* data, size_t size, size_t i = 0 ) { return _insertIs( index, data, size, i ); }
		inline void insertIndices( const uf::Mesh& mesh ) { return _insertI( index, mesh, mesh.index ); }
		inline void updateIndexDescriptor() { return _updateDescriptor( index ); }

		template<typename U> inline void bindIndex( size_t count = 1 ) { return _bindI( index, sizeof(U), ext::RENDERER::typeToEnum<U>(), count ); }
		template<typename U> inline void insertIndex( U I, size_t i = 0 ) { return _insertI( index, (const void*) &I, i ); }
		template<typename U> inline void insertIndices( const uf::stl::vector<U>& indices, size_t i = 0 ) { return _insertIs( index, (const void*) indices.data(), indices.size(), i ); }

		inline bool hasInstance( const uf::stl::vector<ext::RENDERER::AttributeDescriptor>& descriptors ) const { return _hasV( instance, descriptors ); }
		inline bool hasInstance( const uf::Mesh& mesh ) const { return _hasV( instance, mesh.instance ); }
		inline void bindInstance( const uf::stl::vector<ext::RENDERER::AttributeDescriptor>& descriptors ) { return _bindV( instance, descriptors ); }
		inline void resizeInstances( size_t count ) { return _resizeVs( instance, count ); }
		inline void reserveInstances( size_t count ) { return _reserveVs( instance, count ); }
		inline void insertInstance( const void* data ) { return _insertV( instance, data ); }
		inline void insertInstances( const void* data, size_t size ) { return _insertVs( instance, data, size ); }
		inline void insertInstances( const uf::Mesh& mesh ) { return _insertV( instance, mesh, mesh.instance ); }
		inline void updateInstanceDescriptor() { return _updateDescriptor( instance ); }

		template<typename T> inline bool hasInstance() const { return _hasV( instance, T::descriptors ); }
		template<typename T> inline void bindInstance() { return _bindV( instance, T::descriptor ); }
		template<typename T> inline void insertInstance( const T& v ) { return _insertV( instance, (const void*) &v ); }
		template<typename T> inline void insertInstances( const uf::stl::vector<T>& instances ) { return _insertVs( instance, (const void*) instances.data(), instances.size() ); }

		inline void bindIndirect( size_t size, ext::RENDERER::enums::Type::type_t type, size_t count = 1 ) { return _bindI( indirect, size, type, count ); }
		inline void reserveIndirects( size_t count, size_t i = 0 ) { return _reserveIs( indirect, count, i ); }
		inline void resizeIndirects( size_t count, size_t i = 0 ) { return _resizeIs( indirect, count, i ); }
		inline void insertIndirect( const void* data, size_t i = 0 ) { return _insertI( indirect, data, i ); }
		inline void insertIndirects( const void* data, size_t size, size_t i = 0 ) { return _insertIs( indirect, data, size, i ); }
		inline void insertIndirects( const uf::Mesh& mesh ) { return _insertI( indirect, mesh, mesh.indirect ); }
		inline void updateIndirectDescriptor() { return _updateDescriptor( indirect ); }

		template<typename U> inline void bindIndirect( size_t i = 1 ) { return _bindI( indirect, sizeof(U), ext::RENDERER::typeToEnum<U>(), i ); }
		template<typename U> inline void insertIndirect( U v, size_t i = 0 ) { return _insertI( indirect, (const void*) &v, i ); }
		template<typename U> inline void insertIndirects( const uf::stl::vector<U>& indirects, size_t i = 0 ) { return _insertIs( indirect, (const void*) indirects.data(), indirects.size(), i ); }

		template<typename T, typename U = ext::RENDERER::index_t>
		void bind( bool interleave = uf::Mesh::defaultInterleaved, size_t indices = 1 ) {
			bindVertex<T>();
			bindIndex<U>( indices );
			_bind( interleave );
		}
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

			struct {
				uf::Mesh::Input vertex, index, instance, indirect;
				size_t bufferOffset = 0;
			} inputs;

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
		.offset = offsetof(TYPE, ATTRIBUTE),\
		.size = sizeof(decltype(TYPE::ATTRIBUTE)),\
		.format = uf::renderer::enums::Format::FORMAT,\
		.name = #ATTRIBUTE,\
		.type = ext::RENDERER::typeToEnum<decltype(TYPE::ATTRIBUTE)::type_t>(),\
		.components = decltype(TYPE::ATTRIBUTE)::size,\
	},

#define UF_VERTEX_DESCRIPTOR( TYPE, ... )\
	uf::stl::vector<uf::renderer::AttributeDescriptor> TYPE::descriptor = { __VA_ARGS__ };


#if UF_USE_VULKAN
	namespace uf {
		namespace renderer = ext::vulkan;
	}
#elif UF_USE_OPENGL
	namespace uf {
		namespace renderer = ext::opengl;
	}
#endif

namespace pod {
	struct /*UF_API*/ Vertex_3F2F3F4F {
		/*alignas(16)*/ pod::Vector3f position;
		/*alignas(8)*/ pod::Vector2f uv;
		/*alignas(16)*/ pod::Vector3f normal;
		/*alignas(16)*/ pod::Vector4f color;

		static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
	};
	struct /*UF_API*/ Vertex_3F2F3F32B {
		/*alignas(16)*/ pod::Vector3f position;
		/*alignas(8)*/ pod::Vector2f uv;
		/*alignas(16)*/ pod::Vector3f normal;
		/*alignas(16)*/ pod::Vector4t<uint8_t> color;

		static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
	};
	struct /*UF_API*/ Vertex_3F3F3F {
		/*alignas(16)*/ pod::Vector3f position;
		/*alignas(16)*/ pod::Vector3f uv;
		/*alignas(16)*/ pod::Vector3f normal;

		static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
	};
	struct /*UF_API*/ Vertex_3F2F3F1UI {
		/*alignas(16)*/ pod::Vector3f position;
		/*alignas(8)*/ pod::Vector2f uv;
		/*alignas(16)*/ pod::Vector3f normal;
		/*alignas(4)*/ pod::Vector1ui id;

		static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
	};
	struct /*UF_API*/ Vertex_3F2F3F {
		/*alignas(16)*/ pod::Vector3f position;
		/*alignas(8)*/ pod::Vector2f uv;
		/*alignas(16)*/ pod::Vector3f normal;

		static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
	};
	struct /*UF_API*/ Vertex_3F2F {
		/*alignas(16)*/ pod::Vector3f position;
		/*alignas(8)*/ pod::Vector2f uv;

		static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
	};
	struct /*UF_API*/ Vertex_2F2F {
		/*alignas(8)*/ pod::Vector2f position;
		/*alignas(8)*/ pod::Vector2f uv;

		static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
	};
	struct /*UF_API*/ Vertex_3F {
		/*alignas(16)*/ pod::Vector3f position;

		static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
	};
}


#include <uf/ext/meshopt/meshopt.h>
#include "mesh.inl"