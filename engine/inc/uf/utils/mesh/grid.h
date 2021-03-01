#pragma once

#include <uf/config.h>
#include <uf/utils/mesh/mesh.h>
#include <limits>

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
			std::vector<uint32_t> indices;
	#endif
		};
		pod::Vector3ui m_size = {1,1,1};
		pod::Vector3f m_center = {};
		pod::Vector3f m_extent = {};
		std::vector<Node> m_nodes;

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
		void initialize( const uf::BaseMesh<T,U>& mesh, size_t = 1 );

		template<typename T, typename U>
		void initialize( const uf::BaseMesh<T,U>& mesh, const pod::Vector3ui& );
		
		template<typename U = uint32_t>
		std::vector<U> get() const;

		template<typename U = uint32_t>
		const std::vector<U>& get( const pod::Vector3f& ) const;

		const pod::Vector3f& center() const;
		const pod::Vector3f& extent() const;

		void analyze() const;
	};
};

#include "grid.inl"