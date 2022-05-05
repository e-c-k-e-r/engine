#pragma once

#if UF_USE_UNUSED
#include "gjk.h"
#include <uf/utils/mesh/mesh.h>

namespace uf {
	class UF_API MeshCollider : public pod::Collider {
	protected:
		uf::stl::vector<pod::Vector3> m_positions;
	public:
		MeshCollider( const pod::Transform<>& = {}, const uf::stl::vector<pod::Vector3>& = {} );

	 	uf::stl::vector<pod::Vector3>& getPositions();
	 	const uf::stl::vector<pod::Vector3>& getPositions() const;
		
		void setPositions( const uf::stl::vector<pod::Vector3>& );
		
	#if 0
		template<typename T, typename U>
		void setPositions( const uf::Mesh<T, U>& mesh ) {
			this->m_positions.clear();
			this->m_positions.reserve( std::max( mesh.vertices.size(), mesh.indices.size() ) );
			if ( !mesh.indices.empty() ) {
				for ( auto& index : mesh.indices ) this->m_positions.push_back( mesh.vertices[index].position );
			} else {
				for ( auto& vertex : mesh.vertices ) this->m_positions.push_back( vertex.position );
			}
		}
	#endif

		virtual uf::stl::string type() const;
		virtual pod::Vector3* expand() const;
		virtual pod::Vector3 support( const pod::Vector3& ) const;
	};
}
#endif