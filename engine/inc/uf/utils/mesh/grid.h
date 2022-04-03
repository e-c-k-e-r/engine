#pragma once

#include <uf/config.h>
#include <uf/utils/mesh/mesh.h>
#include <limits>


namespace uf {
	namespace meshgrid {
		struct UF_API Node {
			struct {
				pod::Vector3f min = {};
				pod::Vector3f max = {};
			} extents;

			pod::Vector3ui id = {};
			uf::stl::vector<uint32_t> indices;
		};
		struct Grid {
			int divisions = 1;

			struct {
				pod::Vector3f min = {  std::numeric_limits<float>::max(),  std::numeric_limits<float>::max(),  std::numeric_limits<float>::max() };
				pod::Vector3f max = { -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() };
				
				pod::Vector3f center = {};
				pod::Vector3f corner = {};
				
				pod::Vector3f size = {};
				pod::Vector3f piece = {};
			} extents;

			uf::stl::vector<Node> nodes;
		};

		void UF_API print( const Grid&, size_t indices );
		Grid UF_API generate( int divisions, void* pPointer, size_t pStride, size_t pFirst, size_t pCount, void* iPointer, size_t iStride, size_t iFirst, size_t iCount );
		Grid UF_API partition( uf::Mesh&, int );
		
		template<typename T, typename U = uint32_t>
		Grid UF_API partition( uf::stl::vector<T>& vertices, uf::stl::vector<U>& indices, int divisions ) {
			auto vertexDescriptor = T::descriptor.front();

			for ( auto& descriptor : T::descriptor ) if ( descriptor.name == "position" ) { vertexDescriptor = descriptor; break; }
			UF_ASSERT( vertexDescriptor.name == "position" );

			return generate( divisions, 
				vertices.data(), sizeof(T), 0, vertices.size(),
				indices.data(), sizeof(U), 0, indices.size()
			);
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