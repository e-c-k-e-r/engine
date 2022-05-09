#pragma once

#include <uf/config.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/engine/graph/graph.h>
#include <limits>


namespace uf {
	namespace meshgrid {
		struct Node {
			struct {
				pod::Vector3f min = {};
				pod::Vector3f max = {};
			} extents;
			struct {
				pod::Vector3f min = {  std::numeric_limits<float>::max(),  std::numeric_limits<float>::max(),  std::numeric_limits<float>::max() };
				pod::Vector3f max = { -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() };
			} effectiveExtents;

			pod::Vector3ui id = {};
			uf::stl::unordered_map<size_t, uf::Meshlet_T<>> meshlets;
		};
		struct Grid {
			pod::Vector3ui divisions = {1,1,1};
			int indices = 0;

			struct {
				pod::Vector3f min = {  std::numeric_limits<float>::max(),  std::numeric_limits<float>::max(),  std::numeric_limits<float>::max() };
				pod::Vector3f max = { -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() };
				
				pod::Vector3f center = {};
				pod::Vector3f corner = {};
				
				pod::Vector3f size = {};
				pod::Vector3f piece = {};
			} extents;

		//	uf::stl::vector<Node> nodes;
			uf::stl::unordered_map<pod::Vector3ui, Node> nodes;
		};

		void UF_API print( const Grid& );
		void UF_API cleanup( Grid& );

		void UF_API calculate( uf::meshgrid::Grid&, float = std::numeric_limits<float>::epsilon() );
		void UF_API calculate( uf::meshgrid::Grid&, int, float = std::numeric_limits<float>::epsilon() );
		void UF_API calculate( uf::meshgrid::Grid&, const pod::Vector3ui&, float = std::numeric_limits<float>::epsilon() );

		void UF_API partition( uf::meshgrid::Grid&, const void*, size_t, const void*, size_t, const pod::Primitive& );
		void UF_API partition( uf::meshgrid::Grid&, uf::Mesh&, const pod::Primitive& );
		
		template<typename T, typename U = uint32_t>
		void UF_API partition( uf::meshgrid::Grid& grid, const uf::stl::vector<T>& vertices, const uf::stl::vector<U>& indices, const pod::Primitive& primitive ) {
			auto vertexDescriptor = T::descriptor.front();

			for ( auto& descriptor : T::descriptor ) if ( descriptor.name == "position" ) { vertexDescriptor = descriptor; break; }
			UF_ASSERT( vertexDescriptor.name == "position" );

			return partition( grid, 
				vertices.data(), sizeof(T),
				indices.data(), sizeof(U),
				primitive
			);
		}

		template<typename T, typename U = uint32_t>
		uf::stl::vector<uf::Meshlet_T<T,U>> UF_API partition( uf::meshgrid::Grid& grid, const uf::stl::vector<uf::Meshlet_T<T,U>>& meshlets, float eps = std::numeric_limits<float>::epsilon() ) {
			uf::stl::vector<uf::Meshlet_T<T,U>> partitioned;
			for ( auto& meshlet : meshlets ) {
				grid.extents.min = uf::vector::min( grid.extents.min, meshlet.primitive.instance.bounds.min );
				grid.extents.max = uf::vector::max( grid.extents.max, meshlet.primitive.instance.bounds.max );
			}

			uf::meshgrid::calculate( grid, eps );

			for ( auto& meshlet : meshlets ) {
				uf::meshgrid::partition<T,U>( grid, meshlet.vertices, meshlet.indices, meshlet.primitive );
			}
			
			uf::meshgrid::cleanup( grid );

			size_t atlasID = 0;
			for ( auto& pair : grid.nodes ) { auto& node = pair.second;
				++atlasID;
				for ( auto& pair2 : node.meshlets ) { auto& mlet = pair2.second;
					if ( mlet.indices.empty() ) continue;

					auto& meshlet = meshlets[mlet.primitive.instance.primitiveID];
					auto& slice = partitioned.emplace_back();
					slice.vertices.reserve( mlet.indices.size() );
					slice.indices.reserve( mlet.indices.size() );
					for ( auto idx : mlet.indices ) {
						slice.vertices.emplace_back( meshlet.vertices[idx] );
						slice.indices.emplace_back( slice.indices.size() );
					}

					slice.primitive.instance.materialID = meshlet.primitive.instance.materialID;
					slice.primitive.instance.primitiveID = partitioned.size() - 1;
					slice.primitive.instance.objectID = 0;
					slice.primitive.instance.auxID = atlasID;
					slice.primitive.instance.bounds.min = node.effectiveExtents.min;
					slice.primitive.instance.bounds.max = node.effectiveExtents.max;

					slice.primitive.drawCommand.indices = slice.indices.size();
					slice.primitive.drawCommand.instances = 1;
					slice.primitive.drawCommand.indexID = 0;
					slice.primitive.drawCommand.vertexID = 0;
					slice.primitive.drawCommand.instanceID = 0;
					slice.primitive.drawCommand.auxID = atlasID; // meshlet.primitive.instance.meshID;
					slice.primitive.drawCommand.vertices = slice.vertices.size();
				}
			}

			return partitioned;
		}
	}
}

#if 0
namespace uf {
	class UF_API MeshGrid {
	protected:
		struct Node {
			pod::Vector3f center = {}; // not necessary since we can easily calculate this on the fly in get()
			pod::Vector3f extent = {}; // not necessary since this can be pre-computed on initialization()
	#if 0
			size_t count = 0;
			uint32_t* indices = NULL;
	#else
			uf::stl::vector<uint32_t> indices;
	#endif
		};
		pod::Vector3ui m_size = {1,1,1};
		pod::Vector3f m_center = {};
		pod::Vector3f m_extent = {};
		uf::stl::vector<Node> m_nodes;

		void initialize( const pod::Vector3f&, const pod::Vector3f&, const pod::Vector3ui& );
		void destroy();
		
		void allocate( Node&, size_t );
		Node& at( const pod::Vector3f& );
		const Node& at( const pod::Vector3f& ) const;
		pod::Vector3f centroid( const pod::Vector3f&, const pod::Vector3f&, const pod::Vector3f& ) const;

		pod::Vector3i wrapIndex( size_t index );
		size_t unwrapIndex( const pod::Vector3i& );

		void insert( uint32_t, uint32_t, uint32_t, const pod::Vector3f&, const pod::Vector3f&, const pod::Vector3f& );
	public:
		~MeshGrid();
		template<typename T, typename U>
		void initialize( const uf::Mesh<T,U>& mesh, size_t = 1 );

		template<typename T, typename U>
		void initialize( const uf::Mesh<T,U>& mesh, const pod::Vector3ui& );
		
		template<typename U = uint32_t>
		uf::stl::vector<U> get() const;

		template<typename U = uint32_t>
		const uf::stl::vector<U>& get( const pod::Vector3f& ) const;

		const pod::Vector3f& center() const;
		const pod::Vector3f& extent() const;

		void analyze() const;
	};
};

#include "grid.inl"
#endif