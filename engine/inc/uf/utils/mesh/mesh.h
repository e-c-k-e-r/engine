#pragma once

#include <uf/utils/math/vector.h>
#include <uf/utils/math/matrix.h>
#include <uf/utils/math/quant.h>
#include <uf/utils/math/shapes.h>
#include <uf/utils/string/hash.h>

#include <functional>
#include <type_traits>
#include <uf/utils/memory/unordered_map.h>

#if UF_USE_VULKAN
#include <uf/ext/vulkan/enums.h>
#define RENDERER vulkan
#elif UF_USE_OPENGL
#include <uf/ext/opengl/enums.h>
#define RENDERER opengl
#endif

#if UF_USE_VULKAN
	namespace uf {
		namespace renderer = ext::vulkan;
	}
#elif UF_USE_OPENGL
	namespace uf {
		namespace renderer = ext::opengl;
	}
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
			uf::renderer::enums::Format::type_t format = uf::renderer::enums::Format::UNDEFINED;
			// not as essential
			uf::stl::string name = "";
			uf::renderer::enums::Type::type_t type = 0;
			size_t components = 0;
			
			bool operator==( const AttributeDescriptor& right ) const { return name == right.name;
			/*
				offset == right.offset && 
				size == right.size && 
				format == right.format &&
				name == right.name &&
				type == right.type && 
				components == right.components;
			*/
			}
			bool operator!=( const AttributeDescriptor& right ) const { return !(*this == right); };
		};
	}
}

namespace pod {
	// stores information for a draw call
	// used for GPU-driven indirection
	// to-do: probably repurpose auxID and materialIDs
	struct UF_API DrawCommand {
		alignas(4) uint32_t indices = 0; // triangle count
		alignas(4) uint32_t instances = 0; // instance count
		alignas(4) uint32_t indexID = 0; // starting triangle position
		alignas(4) uint32_t vertexID = 0; // starting vertex position
		alignas(4) uint32_t instanceID = 0; // starting instance position
		// extra data for padding
		alignas(4) uint32_t auxID = 0; // used for storing which grid this belongs to when slicing, otherwise unused
		alignas(4) uint32_t materialID = 0; // unused
		alignas(4) uint32_t vertices = 0; // stores vertex count, should be unused
	};

	// stores index offsets for LODs
	struct UF_API LODMetadata {
		struct Level {
			alignas(4) uint32_t indices = 0;
			alignas(4) uint32_t indexID = 0;
			alignas(4) uint32_t vertexID = 0;
			alignas(4) uint32_t vertices = 0;
		} levels[4];
	};

	// stores information about how to transform a draw call
	// to-do: clean up this mess
	struct UF_API Instance {
		alignas(4) uint32_t materialID = 0; // index for material information
		alignas(4) uint32_t primitiveID = 0; // index to reference the primitive(?)
		alignas(4) uint32_t meshID = 0; // unused
		alignas(4) uint32_t objectID = 0; // index for the object buffer

		alignas(4)  int32_t jointID = -1; // offset for skins(?)
		alignas(4)  int32_t lightmapID = -1; // index for lightmap to use
		alignas(4) uint32_t imageID = 0; // unused?
		alignas(4) uint32_t auxID = 0; // also the lightmap ID?

		// AABB for this primitive
		// should be for the specific draw call itself, rather than the mesh(let) entirely
		struct Bounds {
			pod::Vector3f min = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
			alignas(4) float padding1 = 0;
			pod::Vector3f max = { -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() };
			alignas(4) float padding2 = 0;
		} bounds;

		// stores "pointers" on the GPU side for buffer locations, used for RT / recalculating barycentrics
		struct UF_API Addresses {
			alignas(8) uint64_t vertex{};
			alignas(8) uint64_t index{};
			
			alignas(8) uint64_t indirect{};
			alignas(4) uint32_t drawID{};
			alignas(4) uint32_t padding0{};
			
			alignas(8) uint64_t position{};
			alignas(8) uint64_t uv{};

			alignas(8) uint64_t color{};
			alignas(8) uint64_t st{};

			alignas(8) uint64_t normal{};
			alignas(8) uint64_t tangent{};

			alignas(8) uint64_t joints{};
			alignas(8) uint64_t weights{};
		};

		struct UF_API Object {
			pod::Matrix4f model;
			pod::Matrix4f previous;

			pod::Vector4f color = { 1, 1, 1, 1 };
		};
	//	Addresses addresses = {};
	};


	struct Primitive {
		pod::DrawCommand drawCommand;
		pod::Instance instance;
		pod::LODMetadata lod;
		pod::Instance::Addresses addresses;
	};
}

namespace uf {
	struct UF_API Mesh {
	public:
		typedef uf::stl::vector<uint8_t> buffer_t;
		struct Attribute {
			uf::renderer::AttributeDescriptor descriptor;
			 int32_t buffer = -1;
			size_t offset = 0;

			size_t stride = 0;
			size_t length = 0;
		//	void* pointer = NULL;
			uint8_t* pointer = NULL;
		};
		struct Input {
			uf::stl::vector<Attribute> attributes;
			size_t count = 0; // how many elements is the input using
			size_t first = 0; // base index to start from
			size_t size = 0; // size of one element in the input's buffer
			size_t offset = 0; // bytes to offset from within the associated buffer
		} vertex, index, instance, indirect;

		struct AttributeView {
			Attribute attribute;

			const void* data(size_t first = 0) const {
				if ( !valid() ) return NULL;
				return static_cast<const uint8_t*>(attribute.pointer) + attribute.stride * first;
			}

			template<typename T>
			const T* get(size_t first = 0) const {
				return reinterpret_cast<const T*>(data(first));
			}

			bool valid() const { return attribute.pointer != NULL; }
			size_t stride() const { return attribute.stride; }
			size_t components() const { return attribute.descriptor.components; }
			uf::renderer::enums::Type::type_t type() const { return attribute.descriptor.type; }
		};

		struct View {
			uf::Mesh::Input vertex;
			uf::Mesh::Input index;
			int32_t indirectIndex = -1;
			
			uf::stl::unordered_map<uint32_t, uf::Mesh::AttributeView> attributes;


			bool has( uint32_t hash ) const {
				return attributes.count( hash ) > 0;
			}
			const AttributeView& operator[]( uint32_t hash ) const {
				if ( auto it = attributes.find( hash ); it != attributes.end() ) return it->second;
				UF_EXCEPTION("invalid view hash: {}", hash);
			}
			// support legacy code
			bool has( const uf::stl::string_view name ) const {
				return has( uf::string::fnv1a( name ) );
			}
			const AttributeView& operator[]( const uf::stl::string_view name ) const {
				return operator[]( uf::string::fnv1a( name ) );
			}
		};
		typedef uf::stl::vector<uf::Mesh::View> views_t;

		uf::stl::vector<buffer_t> buffers;

		// crunge, but it's better this way for streaming in mesh data
		uf::stl::vector<uf::stl::string> buffer_paths;
		// mega cringe, but i'd like to have a way to cache it
		uf::stl::vector<uf::Mesh::View> buffer_views;
	protected:
		void _destroy( uf::Mesh::Input& input );
		void _bind();
		void _updateDescriptor( uf::Mesh::Input& input );
		void _updateViews();
		uf::Mesh::Attribute _remapAttribute( const uf::Mesh::Input& input, const uf::Mesh::Attribute& attribute, size_t i = 0 ) const;

		bool _hasV( const uf::Mesh::Input& input, const uf::stl::vector<uf::renderer::AttributeDescriptor>& descriptors ) const;
		bool _hasV( const uf::Mesh::Input& input, const uf::Mesh::Input& src ) const;
		void _bindV( uf::Mesh::Input& input, const uf::stl::vector<uf::renderer::AttributeDescriptor>& descriptors );
		void _resizeVs( uf::Mesh::Input& input, size_t count );
		void _reserveVs( uf::Mesh::Input& input, size_t count );
		void _insertV( uf::Mesh::Input& input, const void* data );
		void _insertVs( uf::Mesh::Input& input, const void* data, size_t size );
		void _insertVs( uf::Mesh::Input& input, const uf::Mesh& mesh, const uf::Mesh::Input& srcInput );
		
		template<typename T> inline bool _hasV( const uf::Mesh::Input& input ) const { return _hasV( input, T::descriptor ); }
		template<typename T> inline void _bindV( uf::Mesh::Input& input ) { return _bindV( input, T::descriptor ); }
		template<typename T> inline void _insertV( uf::Mesh::Input& input, const T& vertex ) { return _insertV( input, (const void*) &vertex ); }
		template<typename T> inline void _insertVs( uf::Mesh::Input& input, const uf::stl::vector<T>& vs ) { return _insertVs( input, (const void*) vs.data(), vs.size() ); }

		void _bindI( uf::Mesh::Input& input, size_t size, uf::renderer::enums::Type::type_t type, size_t count = 1 );
		void _reserveIs( uf::Mesh::Input& input, size_t count, size_t i = 0 );
		void _resizeIs( uf::Mesh::Input& input, size_t count, size_t i = 0 );
		void _insertI( uf::Mesh::Input& input, const void* data, size_t i );
		void _insertIs( uf::Mesh::Input& input, const void* data, size_t size, size_t i );
		void _insertIs( uf::Mesh::Input& input, const uf::Mesh& mesh, const uf::Mesh::Input& srcInput );
		
		template<typename U> inline void _bindI( uf::Mesh::Input& input, size_t indices = 1 ) { return _bindI( input, sizeof(U), uf::renderer::typeToEnum<U>(), indices ); }
		template<typename U> inline void _insertI( uf::Mesh::Input& input, U index, size_t i = 0 ) { return _insertI( input, (const void*) &index, i ); }
		template<typename U> inline void _insertIs( uf::Mesh::Input& input, const uf::stl::vector<U>& is, size_t i = 0 ) { return _insertIs( input, (const void*) is.data(), is.size(), i ); }
	public:
		Mesh() = default;
		Mesh( const Mesh& m ) { copy( m ); }
		Mesh& operator=( const Mesh& m ) { return copy( m ); }
		Mesh( Mesh&& ) noexcept = default;
		Mesh& operator=( Mesh&& ) noexcept = default;

		void initialize();
		void destroy();

		uf::Mesh& copy( const uf::Mesh& );
		uf::Mesh copy() const;
		uf::Mesh alias() const;
		uf::Mesh expand();

		void updateDescriptor();
		
		void bind( const uf::Mesh& );
		void insert( const uf::Mesh& );
		
		void generateIndices();
		void generateIndirect();

		// API hell
		template<typename T> inline void compile( const uf::stl::vector<T>& meshlets, uf::stl::vector<pod::Primitive>& primitives );
		template<typename K, typename V> inline void compile( const uf::stl::unordered_map<K, V>& meshlets, uf::stl::vector<pod::Primitive>& primitives );
		template<typename T> inline uf::stl::vector<pod::Primitive> compile( const uf::stl::vector<T>& meshlets );
		template<typename K, typename V> inline uf::stl::vector<pod::Primitive> compile( const uf::stl::unordered_map<K, V>& meshlets );

		buffer_t& getBuffer( const uf::Mesh::Input&, size_t = 0 );
		buffer_t& getBuffer( const uf::Mesh::Input&, const uf::Mesh::Attribute& );
		
		const buffer_t& getBuffer( const uf::Mesh::Input&, size_t = 0 ) const;
		const buffer_t& getBuffer( const uf::Mesh::Input&, const uf::Mesh::Attribute& ) const;

		void clearAttribute( uf::Mesh::Input&, const uf::Mesh::Attribute& );
		void clearAttribute( uf::Mesh::Input&, size_t );
		void clear();

		uf::Mesh::Input remapInput( const uf::Mesh::Input&, size_t = 0, size_t = 0 ) const;
		uf::Mesh::Input remapVertexInput( size_t i = 0, size_t = 0 ) const;
		uf::Mesh::Input remapIndexInput( size_t i = 0, size_t = 0 ) const;

		uf::Mesh::View makeView( const uf::stl::vector<uf::stl::string>& wanted = {}, size_t index = 0 ) const;
		uf::Mesh::View makeView( size_t commandIndex, const uf::stl::vector<uf::stl::string>& wanted = {}, size_t index = 0 ) const;
		uf::stl::vector<uf::Mesh::View> makeViews( const uf::stl::vector<uf::stl::string>& wanted = {}, size_t index = 0 ) const;

		inline bool hasVertex( const uf::stl::vector<uf::renderer::AttributeDescriptor>& descriptors ) const { return _hasV( vertex, descriptors ); }
		inline bool hasVertex( const uf::Mesh& mesh ) const { return _hasV( vertex, mesh.vertex ); }
		inline void bindVertex( const uf::stl::vector<uf::renderer::AttributeDescriptor>& descriptors ) { return _bindV( vertex, descriptors ); }
		inline void resizeVertices( size_t count ) { return _resizeVs( vertex, count ); }
		inline void reserveVertices( size_t count ) { return _reserveVs( vertex, count ); }
		inline void insertVertex( const void* data ) { return _insertV( vertex, data ); }
		inline void insertVertices( const void* data, size_t size ) { return _insertVs( vertex, data, size ); }
		inline void insertVertices( const uf::Mesh& mesh ) { return _insertVs( vertex, mesh, mesh.vertex ); }
		inline void updateVertexDescriptor() { return _updateDescriptor( vertex ); }
		inline uf::Mesh::Attribute remapVertexAttribute( const uf::Mesh::Attribute& attribute, size_t i = 0 ) const { return _remapAttribute( vertex, attribute, i ); }

		template<typename T> inline bool hasVertex() const { return _hasV( vertex, T::descriptor ); }
		template<typename T> inline void bindVertex() { return _bindV( vertex, T::descriptor ); }
		template<typename T> inline void insertVertex( const T& v ) { return _insertV( vertex, (const void*) &v ); }
		template<typename T> inline void insertVertices( const uf::stl::vector<T>& vertices ) { return _insertVs( vertex, (const void*) vertices.data(), vertices.size() ); }

		inline void bindIndex( size_t size, uf::renderer::enums::Type::type_t type, size_t count = 1 ) { return _bindI( index, size, type, count ); }
		inline void reserveIndices( size_t count, size_t i = 0 ) { return _reserveIs( index, count, i ); }
		inline void resizeIndices( size_t count, size_t i = 0 ) { return _resizeIs( index, count, i ); }
		inline void insertIndex( const void* data, size_t i = 0 ) { return _insertI( index, data, i ); }
		inline void insertIndices( const void* data, size_t size, size_t i = 0 ) { return _insertIs( index, data, size, i ); }
		inline void insertIndices( const uf::Mesh& mesh ) { return _insertIs( index, mesh, mesh.index ); }
		inline void updateIndexDescriptor() { return _updateDescriptor( index ); }
		inline uf::Mesh::Attribute remapIndexAttribute( const uf::Mesh::Attribute& attribute, size_t i = 0 ) const { return _remapAttribute( index, attribute, i ); }

		template<typename U> inline void bindIndex( size_t count = 1 ) { return _bindI( index, sizeof(U), uf::renderer::typeToEnum<U>(), count ); }
		template<typename U> inline void insertIndex( U I, size_t i = 0 ) { return _insertI( index, (const void*) &I, i ); }
		template<typename U> inline void insertIndices( const uf::stl::vector<U>& indices, size_t i = 0 ) { return _insertIs( index, (const void*) indices.data(), indices.size(), i ); }

		inline bool hasInstance( const uf::stl::vector<uf::renderer::AttributeDescriptor>& descriptors ) const { return _hasV( instance, descriptors ); }
		inline bool hasInstance( const uf::Mesh& mesh ) const { return _hasV( instance, mesh.instance ); }
		inline void bindInstance( const uf::stl::vector<uf::renderer::AttributeDescriptor>& descriptors ) { return _bindV( instance, descriptors ); }
		inline void resizeInstances( size_t count ) { return _resizeVs( instance, count ); }
		inline void reserveInstances( size_t count ) { return _reserveVs( instance, count ); }
		inline void insertInstance( const void* data ) { return _insertV( instance, data ); }
		inline void insertInstances( const void* data, size_t size ) { return _insertVs( instance, data, size ); }
		inline void insertInstances( const uf::Mesh& mesh ) { return _insertVs( instance, mesh, mesh.instance ); }
		inline void updateInstanceDescriptor() { return _updateDescriptor( instance ); }

		template<typename T> inline bool hasInstance() const { return _hasV( instance, T::descriptor ); }
		template<typename T> inline void bindInstance() { return _bindV( instance, T::descriptor ); }
		template<typename T> inline void insertInstance( const T& v ) { return _insertV( instance, (const void*) &v ); }
		template<typename T> inline void insertInstances( const uf::stl::vector<T>& instances ) { return _insertVs( instance, (const void*) instances.data(), instances.size() ); }

		inline void bindIndirect( size_t size, uf::renderer::enums::Type::type_t type, size_t count = 1 ) { return _bindI( indirect, size, type, count ); }
		inline void reserveIndirects( size_t count, size_t i = 0 ) { return _reserveIs( indirect, count, i ); }
		inline void resizeIndirects( size_t count, size_t i = 0 ) { return _resizeIs( indirect, count, i ); }
		inline void insertIndirect( const void* data, size_t i = 0 ) { return _insertI( indirect, data, i ); }
		inline void insertIndirects( const void* data, size_t size, size_t i = 0 ) { return _insertIs( indirect, data, size, i ); }
		inline void insertIndirects( const uf::Mesh& mesh ) { return _insertIs( indirect, mesh, mesh.indirect ); }
		inline void updateIndirectDescriptor() { return _updateDescriptor( indirect ); }

		template<typename U> inline void bindIndirect( size_t i = 1 ) { return _bindI( indirect, sizeof(U), uf::renderer::typeToEnum<U>(), i ); }
		template<typename U> inline void insertIndirect( U v, size_t i = 0 ) { return _insertI( indirect, (const void*) &v, i ); }
		template<typename U> inline void insertIndirects( const uf::stl::vector<U>& indirects, size_t i = 0 ) { return _insertIs( indirect, (const void*) indirects.data(), indirects.size(), i ); }

		template<typename T, typename U = uf::renderer::index_t>
		void bind( size_t indices = 1 ) {
			bindVertex<T>();
			bindIndex<U>( indices );
			_bind();
		}

		template<typename From, typename To>
		void convert() {
			auto fromEnum = uf::renderer::typeToEnum<From>();
			auto toEnum = uf::renderer::typeToEnum<To>();
			if ( toEnum == fromEnum ) return;

			for ( auto& attribute : this->vertex.attributes ) {
				if ( attribute.descriptor.type == toEnum ) continue;
				if ( attribute.descriptor.type != fromEnum ) continue;

				size_t elements = this->vertex.count * attribute.descriptor.components;
				size_t bytes = elements * sizeof(To);
				
				auto& srcBuffer = this->buffers[attribute.buffer];

				if ( srcBuffer.empty() ) continue;

				uf::stl::vector<uint8_t> dstBuffer( bytes );

				From* srcPtr = (From*) (srcBuffer.data());
				To* dstPtr = (To*) (dstBuffer.data());
				
				if ( toEnum == uf::renderer::enums::Type::USHORT ) {
					for ( size_t i = 0; i < elements; ++i ) dstPtr[i] = (To) uf::quant::quantize_f32u16(srcPtr[i]);
				} else if ( fromEnum == uf::renderer::enums::Type::USHORT ) {
					for ( size_t i = 0; i < elements; ++i ) dstPtr[i] = (To) uf::quant::dequantize_u16f32(srcPtr[i]);
				} else {
					for ( size_t i = 0; i < elements; ++i ) dstPtr[i] = (To) srcPtr[i];
				}
				
				srcBuffer.swap( dstBuffer );

				attribute.pointer = (uint8_t*) ( srcBuffer.data() );
				attribute.descriptor.type = toEnum;
				attribute.descriptor.size = sizeof(To) * attribute.descriptor.components;
				attribute.length = sizeof(To) * elements;

				if ( toEnum == uf::renderer::enums::Type::FLOAT ) {
					switch ( attribute.descriptor.components ) {
						case 1: attribute.descriptor.format = uf::renderer::enums::Format::R32_SFLOAT; break;
						case 2: attribute.descriptor.format = uf::renderer::enums::Format::R32G32_SFLOAT; break;
						case 3: attribute.descriptor.format = uf::renderer::enums::Format::R32G32B32_SFLOAT; break;
						case 4: attribute.descriptor.format = uf::renderer::enums::Format::R32G32B32A32_SFLOAT; break;
					}
				} else if ( toEnum == uf::renderer::enums::Type::FLOAT16 ) {
					switch ( attribute.descriptor.components ) {
						case 1: attribute.descriptor.format = uf::renderer::enums::Format::R16_SFLOAT; break;
						case 2: attribute.descriptor.format = uf::renderer::enums::Format::R16G16_SFLOAT; break;
						case 3: attribute.descriptor.format = uf::renderer::enums::Format::R16G16B16_SFLOAT; break;
						case 4: attribute.descriptor.format = uf::renderer::enums::Format::R16G16B16A16_SFLOAT; break;
					}
				} else if ( toEnum == uf::renderer::enums::Type::USHORT ) {
					switch ( attribute.descriptor.components ) {
						case 1: attribute.descriptor.format = uf::renderer::enums::Format::R16_UINT; break;
						case 2: attribute.descriptor.format = uf::renderer::enums::Format::R16G16_UINT; break;
						case 3: attribute.descriptor.format = uf::renderer::enums::Format::R16G16B16_UINT; break;
						case 4: attribute.descriptor.format = uf::renderer::enums::Format::R16G16B16A16_UINT; break;
					}
				}
			}
		}
	};
}

namespace ext {
	namespace RENDERER {
		struct UF_API GraphicDescriptor {
			typedef size_t hash_t;
			
			uf::stl::string renderMode = "";
			uf::stl::string pipeline = "";
			hash_t material = {};

			uint32_t renderTarget = 0;
			uint32_t subpass = 0;
			uint32_t aux = 0;
			
			struct {
				size_t width = 1;
				size_t height = 1;
				size_t depth = 1;
				uint32_t point = 0;
			} bind;

			struct {
				uf::Mesh::Input vertex, index, instance, indirect;
				size_t bufferOffset = 0;
			} inputs;

			uf::renderer::enums::PrimitiveTopology::type_t topology = uf::renderer::enums::PrimitiveTopology::TRIANGLE_LIST;
			uf::renderer::enums::PolygonMode::type_t fill = uf::renderer::enums::PolygonMode::FILL;
			uf::renderer::enums::CullMode::type_t cullMode = uf::renderer::enums::CullMode::BACK;
			uf::renderer::enums::Face::type_t frontFace = uf::renderer::enums::Face::CW;
			float lineWidth = 1.0f;

			struct {
				bool test = true;
				bool write = true;
				uf::renderer::enums::Compare::type_t operation = uf::renderer::enums::Compare::GREATER_OR_EQUAL;
				struct {
					bool enable = false;
					float constant = 0;
					float slope = 0;
					float clamp = 0;
				} bias;
			} depth;

		// to-do: port the rest
			struct {
				bool enabled = true;
		#if UF_USE_VULKAN
				VkBlendFactor			srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
				VkBlendFactor			dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				VkBlendOp				colorBlendOp = VK_BLEND_OP_ADD;
				VkBlendFactor			srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
				VkBlendFactor			dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
				VkBlendOp				alphaBlendOp = VK_BLEND_OP_ADD;
				VkColorComponentFlags	colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		#endif
			} blend;
			
			bool invalidated = false;

			hash_t hash() const;
			void parse( ext::json::Value& );
			bool operator==( const GraphicDescriptor& right ) const { return this->hash() == right.hash(); }
			bool operator!=( const GraphicDescriptor& right ) const { return this->hash() != right.hash(); }
		};
	}
}

namespace std {
	template <>
	struct hash<uf::renderer::GraphicDescriptor> {
		size_t operator()(const uf::renderer::GraphicDescriptor& descriptor) const { return descriptor.hash(); }
	};
}

#undef UF_RENDERER
#define UF_VERTEX_DESCRIPTION( TYPE, FORMAT, ATTRIBUTE ) {\
		.offset = offsetof(TYPE, ATTRIBUTE),\
		.size = sizeof(decltype(TYPE::ATTRIBUTE)),\
		.format = uf::renderer::enums::Format::FORMAT,\
		.name = #ATTRIBUTE,\
		.type = uf::renderer::typeToEnum<decltype(TYPE::ATTRIBUTE)::type_t>(),\
		.components = decltype(TYPE::ATTRIBUTE)::size,\
	},

#define UF_VERTEX_DESCRIPTOR( TYPE, ... )\
	uf::stl::vector<uf::renderer::AttributeDescriptor> TYPE::descriptor = { __VA_ARGS__ };

#define UF_VERTEX_INTERPOLATE( TYPE, ... )\
	TYPE UF_API TYPE::interpolate( const TYPE& p1, const TYPE& p2, float t ) __VA_ARGS__

namespace pod {
	struct /*UF_API*/ Vertex_3F2F3F4F {
		pod::Vector3f position;
		pod::Vector2f uv;
		pod::Vector3f normal;
		pod::Vector4f color;

		static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
		static UF_API Vertex_3F2F3F4F interpolate( const Vertex_3F2F3F4F& p1, const Vertex_3F2F3F4F& p2, float t );
	};
	struct /*UF_API*/ Vertex_3F2F3F32B {
		pod::Vector3f position;
		pod::Vector2f uv;
		pod::Vector3f normal;
		pod::Vector4t<uint8_t> color;

		static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
		static UF_API Vertex_3F2F3F32B interpolate( const Vertex_3F2F3F32B& p1, const Vertex_3F2F3F32B& p2, float t );
	};
	struct /*UF_API*/ Vertex_3F3F3F {
		pod::Vector3f position;
		pod::Vector3f uv;
		pod::Vector3f normal;

		static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
		static UF_API Vertex_3F3F3F interpolate( const Vertex_3F3F3F& p1, const Vertex_3F3F3F& p2, float t );
	};
	struct /*UF_API*/ Vertex_3F2F3F1UI {
		pod::Vector3f position;
		pod::Vector2f uv;
		pod::Vector3f normal;
		pod::Vector1ui id;

		static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
		static UF_API Vertex_3F2F3F1UI interpolate( const Vertex_3F2F3F1UI& p1, const Vertex_3F2F3F1UI& p2, float t );
	};
	struct /*UF_API*/ Vertex_3F2F3F {
		pod::Vector3f position;
		pod::Vector2f uv;
		pod::Vector3f normal;

		static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
		static UF_API Vertex_3F2F3F interpolate( const Vertex_3F2F3F& p1, const Vertex_3F2F3F& p2, float t );
	};
	struct /*UF_API*/ Vertex_3F2F {
		pod::Vector3f position;
		pod::Vector2f uv;

		static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
		static UF_API Vertex_3F2F interpolate( const Vertex_3F2F& p1, const Vertex_3F2F& p2, float t );
	};
	struct /*UF_API*/ Vertex_2F2F {
		pod::Vector2f position;
		pod::Vector2f uv;

		static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
		static UF_API Vertex_2F2F interpolate( const Vertex_2F2F& p1, const Vertex_2F2F& p2, float t );
	};
	struct /*UF_API*/ Vertex_3F {
		pod::Vector3f position;

		static UF_API uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
		static UF_API Vertex_3F interpolate( const Vertex_3F& p1, const Vertex_3F& p2, float t );
	};
}

namespace uf {
	// ???
	template<typename T = pod::Vertex_3F, typename U = uf::renderer::index_t>
	struct UF_API Mesh_T {
		typedef T vertex_t;
		typedef U index_t;

		uf::stl::vector<vertex_t> vertices;
		uf::stl::vector<index_t> indices;
		uf::stl::vector<pod::Primitive> primitives;
	};

	template<typename T = pod::Vertex_3F, typename U = uf::renderer::index_t>
	struct UF_API Meshlet_T {
		typedef T vertex_t;
		typedef U index_t;

		uf::stl::vector<vertex_t> vertices;
		uf::stl::vector<index_t> indices;
		pod::Primitive primitive;
	};

	namespace mesh {
		template <typename T, typename = void> struct has_tangent : std::false_type {};
		template <typename T> struct has_tangent<T, std::void_t<decltype(std::declval<T>().tangent)>> : std::true_type {};

		template <typename T, typename = void> struct has_normal : std::false_type {};
		template <typename T> struct has_normal<T, std::void_t<decltype(std::declval<T>().normal)>> : std::true_type {};

		size_t UF_API fetchIndex( const void* pointer, size_t stride, size_t index );
		pod::Vector3f UF_API fetchVertex( const uf::Mesh::View& view, const uf::Mesh::AttributeView& positions, size_t index );
		pod::Triangle UF_API fetchTriangle( const uf::Mesh::View& view, const uf::Mesh::AttributeView& indices, const uf::Mesh::AttributeView& positions, size_t triID );
		pod::TriangleWithNormal UF_API fetchTriangle( const uf::Mesh& mesh, size_t triID );

		template<typename T> size_t windingOrder( uf::stl::vector<T>& vertices );
		template<typename T, typename U> size_t windingOrder( uf::stl::vector<T>& vertices, uf::stl::vector<U>& indices );
		template<typename T> void normals( uf::stl::vector<T>& vertices );
		template<typename T, typename U = uint32_t> void normals( uf::stl::vector<T>& vertices, const uf::stl::vector<U>& indices );
		// specifically refuses to work properly
		template<typename T> void tangents( uf::stl::vector<T>& vertices );
		template<typename T, typename U = uint32_t> void tangents( uf::stl::vector<T>& vertices, const uf::stl::vector<U>& indices );

		template<typename T> T fetchVertexAttribute( const uf::Mesh::View& view, const uf::Mesh::AttributeView& attributeView, size_t index );
		template<typename T>
		T& getVertexAttribute( const uf::Mesh::View& view, const uf::Mesh::AttributeView& attributeView, size_t index ) {
			UF_ASSERT( uf::renderer::typeToEnum<typename T::type_t>() == attributeView.type() && T::size == attributeView.components() );
			return *(T*) attributeView.data( view.vertex.first + index );
		}
		//
		template<typename U> inline U& getIndex( void* pointer, size_t index ) {
			return ((U*) pointer)[index];
		}
		template<typename U> inline U& getIndex( const uf::Mesh::View& view, const uf::Mesh::AttributeView& indices, size_t index ) {
			return uf::mesh::getIndex<U>( indices.data(view.index.first), index );
		}
		template<typename U> inline U& getIndex( const uf::Mesh::View& view, size_t index ) {
			return uf::mesh::getIndex<U>( view, view["indices"_hash], index );
		}
		template<typename U> inline U& getIndex( const uf::Mesh::View& view, const uf::stl::string& indices, size_t index ) {
			return uf::mesh::getIndex<U>( view, view[indices], index );
		}
		
		void UF_API setIndex( void* pointer, size_t stride, size_t index, size_t value );
		
		//
		template<typename T, typename U> void compile( uf::Mesh& mesh, const uf::stl::vector<uf::Meshlet_T<T, U>>& meshlets, uf::stl::vector<pod::Primitive>& primitives );
		
		template<typename K, typename V> inline uf::stl::vector<pod::Primitive> compile( uf::Mesh& mesh, const uf::stl::unordered_map<K, V>& meshlets );
		template<typename T> inline uf::stl::vector<pod::Primitive> compile( uf::Mesh& mesh, const uf::stl::vector<T>& meshlets );
		template<typename K, typename V> inline uf::stl::vector<pod::Primitive> compile( uf::Mesh& mesh, const uf::stl::unordered_map<K, V>& meshlets );

		//
		static inline size_t fetchIndex( const uf::Mesh::View& view, const uf::Mesh::AttributeView& indices, size_t index ) {
			return uf::mesh::fetchIndex( indices.data(view.index.first), indices.stride(), index );
		}
		static inline size_t fetchIndex( const uf::Mesh::View& view, size_t index ) {
			return uf::mesh::fetchIndex( view, view["indices"_hash], index );
		}
		static inline size_t fetchIndex( const uf::Mesh::View& view, const uf::stl::string& indices, size_t index ) {
			return uf::mesh::fetchIndex( view, view[indices], index );
		}
		static inline pod::Vector3f fetchVertex( const uf::Mesh::View& view, size_t index ) {
			return uf::mesh::fetchVertex( view, view["positions"_hash], index );
		}
		static inline pod::Vector3f fetchVertex( const uf::Mesh::View& view, const uf::stl::string& positions, size_t index ) {
			return uf::mesh::fetchVertex( view, view[positions], index );
		}
		static inline pod::Triangle fetchTriangle( const uf::Mesh::View& view, const uf::stl::string& indices, const uf::stl::string& positions, size_t triID ) {
			return uf::mesh::fetchTriangle( view, view[indices], view[positions], triID );
		}
		static inline pod::Triangle fetchTriangle( const uf::Mesh::View& view, size_t triID ) {
			return uf::mesh::fetchTriangle( view, view["index"_hash], view["position"_hash], triID );
		}

		static inline void setIndex( const uf::Mesh::View& view, const uf::Mesh::AttributeView& indices, size_t index, size_t value ) {
			return uf::mesh::setIndex( const_cast<void*>(indices.data(view.index.first)), indices.stride(), index, value );
		}
		static inline void setIndex( const uf::Mesh::View& view, size_t index, size_t value ) {
			return uf::mesh::setIndex( view, view["indices"_hash], index, value );
		}
	}
}

template<typename T>
T uf::mesh::fetchVertexAttribute( const uf::Mesh::View& view, const uf::Mesh::AttributeView& attributeView, size_t index ) {
	#define CAST_VERTEX(type) {\
		const type* vertices = (type*) attributeView.data(view.vertex.first + index);\
		for ( auto i = 0; i < T::size; ++i ) res[i] = vertices[i];\
		return res;\
	}
	#define DEQUANTIZE_VERTEX(type) {\
		const type* vertices = (type*) attributeView.data(view.vertex.first + index);\
		for ( auto i = 0; i < T::size; ++i ) res[i] = uf::quant::dequantize(vertices[i]);\
		return res;\
	}

	// direct copy
	if ( uf::renderer::typeToEnum<typename T::type_t>() == attributeView.type() && T::size == attributeView.components() ) {
		return uf::vector::copy<typename T::type_t, T::size>( (typename T::type_t*) attributeView.data( view.vertex.first + index ) );
	}

	// implicit copy
	T res;
	switch ( attributeView.type() ) {
		// dequantize
		case uf::renderer::enums::Type::USHORT:
		case uf::renderer::enums::Type::SHORT: {
			DEQUANTIZE_VERTEX(uint16_t);
		} break;
		case uf::renderer::enums::Type::FLOAT: {
			CAST_VERTEX(float);
		} break;
	#if UF_USE_FLOAT16
		case uf::renderer::enums::Type::HALF: {
			CAST_VERTEX(std::float16_t);
		} break;
	#endif
	#if UF_USE_BFLOAT16
		case uf::renderer::enums::Type::BFLOAT: {
			CAST_VERTEX(std::bfloat16_t);
		} break;
	#endif
		default: UF_EXCEPTION("unsupported attribute type: {}", attributeView.attribute.descriptor.type); break;
	}
}

template<typename T>
size_t uf::mesh::windingOrder( uf::stl::vector<T>& vertices ) {
	if constexpr ( !uf::mesh::has_normal<T>::value ) return 0;

	size_t corrected = 0;
	for ( size_t i = 0; i < vertices.size() / 3; ++i ) {
		pod::Vector3f position[3] = {
			vertices[i * 3 + 0].position,
			vertices[i * 3 + 1].position,
			vertices[i * 3 + 2].position,
		};
		pod::Vector3f normal = uf::vector::normalize( uf::vector::cross((position[0] - position[1]), (position[1] - position[2])) );

		if ( uf::vector::dot( vertices[i * 3 + 0].normal, normal ) < 0.0f ) {
			std::swap( vertices[i * 3 + 0], vertices[i * 3 + 2] );
			++corrected;
		}
	}
	return corrected;
}

template<typename T, typename U>
size_t uf::mesh::windingOrder( uf::stl::vector<T>& vertices, uf::stl::vector<U>& indices ) {
	if constexpr ( !uf::mesh::has_normal<T>::value ) return 0;
	if ( indices.empty() ) return uf::mesh::windingOrder( vertices );

	size_t corrected = 0;
	for ( size_t i = 0; i < indices.size() / 3; ++i ) {
		size_t idx[3] = {
			indices[i * 3 + 0],
			indices[i * 3 + 1],
			indices[i * 3 + 2],
		};
		pod::Vector3f position[3] = {
			vertices[idx[0]].position,
			vertices[idx[1]].position,
			vertices[idx[2]].position,
		};
		pod::Vector3f normal = uf::vector::normalize( uf::vector::cross((position[0] - position[1]), (position[1] - position[2])) );

		if ( uf::vector::dot( vertices[idx[0]].normal, normal ) < 0.0f ) {
			indices[i * 3 + 0] = idx[2];
			indices[i * 3 + 2] = idx[0];
			++corrected;
		}
	}
	return corrected;
}

template<typename T>
void uf::mesh::tangents( uf::stl::vector<T>& vertices ) {
	if constexpr ( !uf::mesh::has_tangent<T>::value ) return;

	for ( size_t i = 0; i < vertices.size(); i += 3 ) {
		size_t idx[3] = { i + 0, i + 1, i + 2 };

		auto p0 = vertices[idx[0]].position;
		auto p1 = vertices[idx[1]].position;
		auto p2 = vertices[idx[2]].position;

		auto uv0 = vertices[idx[0]].uv;
		auto uv1 = vertices[idx[1]].uv;
		auto uv2 = vertices[idx[2]].uv;

		auto p10 = p1 - p0;
		auto p20 = p2 - p0;

		auto uv10 = uv1 - uv0;
		auto uv20 = uv2 - uv0;

		auto det = (uv10.x * uv20.y - uv10.y * uv20.x);
		float r = 1.0f / det;
		auto t = (p10 * uv20.y - p20 * uv10.y) * r;
		auto b = (p20 * uv10.x - p10 * uv20.x) * r;

		for ( auto j = 0; j < 3; ++j ) {
			auto& n = vertices[idx[j]].normal;
			auto& tangent = vertices[idx[j]].tangent;
			tangent = uf::vector::normalize(t - n * uf::vector::dot(n, t));
			if ( uf::vector::dot( uf::vector::cross(n, tangent), b) < 0.0f ) tangent = -tangent;
		}
	/*
		pod::Vector3f position[3] = {
			vertices[idx[0]].position, vertices[idx[1]].position, vertices[idx[2]].position
		};
		pod::Vector2f uv[3] = {
			vertices[idx[0]].uv, vertices[idx[1]].uv, vertices[idx[2]].uv
		};

		pod::Vector3f dPosition[2] = { position[1] - position[0], position[2] - position[0] };
		pod::Vector2f dUV[2] = { uv[1] - uv[0], uv[2] - uv[0] };

		float det = (dUV[0].x * dUV[1].y - dUV[0].y * dUV[1].x);
		if ( det == 0.0f ) continue;
		float r = 1.0f / det;

		auto t = (dPosition[0] * dUV[1].y - dPosition[1] * dUV[0].y) * r;
		auto b = (dPosition[1] * dUV[0].x - dPosition[0] * dUV[1].x) * r;

		for ( auto j = 0; j < 3; ++j ) {
			auto& normal = vertices[idx[j]].normal;
			auto& tangent = vertices[idx[j]].tangent;
			tangent = uf::vector::normalize(t - normal * uf::vector::dot(normal, t));
			if ( uf::vector::dot(uf::vector::cross(normal, tangent), b) < 0.0f ) tangent = -tangent;
		}
	*/
	}
}

template<typename T, typename U>
void uf::mesh::tangents( uf::stl::vector<T>& vertices, const uf::stl::vector<U>& indices ) {
	if constexpr ( !uf::mesh::has_tangent<T>::value ) return;
	if ( indices.empty() ) return tangents( vertices );

	for ( size_t i = 0; i < indices.size(); i += 3 ) {
		size_t idx[3] = { indices[i + 0], indices[i + 1], indices[i + 2] };

		auto p0 = vertices[idx[0]].position;
		auto p1 = vertices[idx[1]].position;
		auto p2 = vertices[idx[2]].position;

		auto uv0 = vertices[idx[0]].uv;
		auto uv1 = vertices[idx[1]].uv;
		auto uv2 = vertices[idx[2]].uv;

		auto p10 = p1 - p0;
		auto p20 = p2 - p0;

		auto uv10 = uv1 - uv0;
		auto uv20 = uv2 - uv0;

		auto det = (uv10.x * uv20.y - uv10.y * uv20.x);
		float r = 1.0f / det;
		auto t = (p10 * uv20.y - p20 * uv10.y) * r;
		auto b = (p20 * uv10.x - p10 * uv20.x) * r;

		for ( auto j = 0; j < 3; ++j ) {
			auto& n = vertices[idx[j]].normal;
			auto& tangent = vertices[idx[j]].tangent;
			tangent = uf::vector::normalize(t - n * uf::vector::dot(n, t));
			if ( uf::vector::dot( uf::vector::cross(n, tangent), b) < 0.0f ) tangent = -tangent;
		}
	/*
		pod::Vector3f position[3] = { vertices[idx[0]].position, vertices[idx[1]].position, vertices[idx[2]].position };
		pod::Vector2f uv[3] = { vertices[idx[0]].uv, vertices[idx[1]].uv, vertices[idx[2]].uv };
		pod::Vector3f dPosition[2] = { position[1] - position[0], position[2] - position[0] };
		pod::Vector2f dUV[2] = { uv[1] - uv[0], uv[2] - uv[0] };

		float det = (dUV[0].x * dUV[1].y - dUV[0].y * dUV[1].x);
		if ( det == 0.0f ) continue;
		float r = 1.0f / det;

		auto t = (dPosition[0] * dUV[1].y - dPosition[1] * dUV[0].y) * r;
		auto b = (dPosition[1] * dUV[0].x - dPosition[0] * dUV[1].x) * r;

		for ( auto j = 0; j < 3; ++j ) {
			auto& normal = vertices[idx[j]].normal;
			auto& tangent = vertices[idx[j]].tangent;
			tangent = uf::vector::normalize(t - normal * uf::vector::dot(normal, t));
			if ( uf::vector::dot(uf::vector::cross(normal, tangent), b) < 0.0f ) tangent = -tangent;
		}
	*/
	}
}

template<typename T>
void uf::mesh::normals( uf::stl::vector<T>& vertices ) {
	if constexpr ( !uf::mesh::has_normal<T>::value ) return;

	for ( size_t i = 0; i < vertices.size(); i += 3 ) {
		auto& v0 = vertices[i + 0];
		auto& v1 = vertices[i + 1];
		auto& v2 = vertices[i + 2];

		pod::Vector3f normal = uf::vector::normalize(uf::vector::cross(v1.position - v0.position, v2.position - v0.position));
		v0.normal = normal;
		v1.normal = normal;
		v2.normal = normal;
	}
}

template<typename T, typename U>
void uf::mesh::normals( uf::stl::vector<T>& vertices, const uf::stl::vector<U>& indices ) {
	if constexpr ( !uf::mesh::has_normal<T>::value ) return;
	if ( indices.empty() ) return normals( vertices );

	for ( auto& v : vertices ) v.normal = {};
	for ( size_t i = 0; i < indices.size(); i += 3 ) {
		auto& v0 = vertices[indices[i + 0]];
		auto& v1 = vertices[indices[i + 1]];
		auto& v2 = vertices[indices[i + 2]];

		pod::Vector3f normal = uf::vector::cross(v1.position - v0.position, v2.position - v0.position);
		v0.normal += normal;
		v1.normal += normal;
		v2.normal += normal;
	}

	for ( auto& v : vertices ) v.normal = uf::vector::normalize( v.normal );
}

template<typename T> uf::stl::vector<pod::Primitive> uf::Mesh::compile( const uf::stl::vector<T>& meshlets ) {
	uf::stl::vector<pod::Primitive> primitives;
	uf::mesh::compile( *this, meshlets, primitives );
	return primitives;
}
template<typename K, typename V> uf::stl::vector<pod::Primitive> uf::Mesh::compile( const uf::stl::unordered_map<K, V>& meshlets ) {
	uf::stl::vector<pod::Primitive> primitives;
	uf::mesh::compile( *this, uf::stl::values( meshlets ), primitives );
	return primitives;
}

template<typename T> void uf::Mesh::compile( const uf::stl::vector<T>& meshlets, uf::stl::vector<pod::Primitive>& primitives ) {
	return uf::mesh::compile( *this, meshlets, primitives );
}
template<typename K, typename V> void uf::Mesh::compile( const uf::stl::unordered_map<K, V>& meshlets, uf::stl::vector<pod::Primitive>& primitives ) {
	return uf::mesh::compile( *this, uf::stl::values( meshlets ), primitives );
}
//
template<typename T> uf::stl::vector<pod::Primitive> uf::mesh::compile( uf::Mesh& mesh, const uf::stl::vector<T>& meshlets ) {
	uf::stl::vector<pod::Primitive> primitives;
	uf::mesh::compile( mesh, meshlets, primitives );
	return primitives;
}
template<typename K, typename V> uf::stl::vector<pod::Primitive> uf::mesh::compile( uf::Mesh& mesh, const uf::stl::unordered_map<K, V>& meshlets ) {
	uf::stl::vector<pod::Primitive> primitives;
	uf::mesh::compile( mesh, uf::stl::values( meshlets ), primitives );
	return primitives;
}
template<typename K, typename V> void uf::mesh::compile( uf::Mesh& mesh, const uf::stl::unordered_map<K, V>& meshlets, uf::stl::vector<pod::Primitive>& primitives ) {
	return uf::mesh::compile( mesh, uf::stl::values( meshlets ), primitives );
}
//
template<typename T, typename U> void uf::mesh::compile( uf::Mesh& mesh, const uf::stl::vector<uf::Meshlet_T<T, U>>& meshlets, uf::stl::vector<pod::Primitive>& primitives ) {
	mesh.bindIndirect<pod::DrawCommand>();
	mesh.bind<T, U>();

	size_t indexID = 0;
	size_t vertexID = 0;
	size_t instanceID = 0;

	uf::stl::vector<pod::DrawCommand> drawCommands;
	drawCommands.reserve( meshlets.size() );
	primitives.reserve( primitives.size() + meshlets.size() );

	for ( auto& meshlet : meshlets ) {
		if ( meshlet.indices.empty() ) continue;
		auto& primitive = primitives.emplace_back(meshlet.primitive);
		// write draw command
		primitive.drawCommand = {
			.indices = meshlet.indices.size(),
			.instances = MAX(1, primitive.drawCommand.instances),
			.indexID = indexID,
			.vertexID = vertexID,
			.instanceID = instanceID,
			.auxID = 0,
			.materialID = 0,
			.vertices = meshlet.vertices.size(),
		};
		// write LOD0
		primitive.lod.levels[0] = {
			.indices = meshlet.indices.size(),
			.indexID = indexID,
			.vertexID = vertexID,
			.vertices = meshlet.vertices.size(),
		};
		// sync draw command with primitive
		drawCommands.emplace_back(primitive.drawCommand);
		// increase IDs
		indexID += primitive.drawCommand.indices;
		vertexID += primitive.drawCommand.vertices;
		instanceID += primitive.drawCommand.instances;
		// insert
		mesh.insertVertices( meshlet.vertices );
		mesh.insertIndices( meshlet.indices );
	}

	mesh.insertIndirects(drawCommands);
	mesh.updateDescriptor();
}