#pragma once

#include "gjk.h"
#include <uf/utils/graphic/mesh.h>

namespace uf {
	class UF_API MeshCollider : public pod::Collider {
	protected:
		std::vector<pod::Vector3> m_positions;
	public:
		MeshCollider( const pod::Transform<>& = {}, const std::vector<pod::Vector3>& = {} );

	 	std::vector<pod::Vector3>& getPositions();
	 	const std::vector<pod::Vector3>& getPositions() const;
		
		void setPositions( const std::vector<pod::Vector3>& );
		
		template<typename T, typename U>
		void setPositions( const uf::BaseMesh<T, U>& mesh ) {
			this->m_positions.clear();
			this->m_positions.reserve( std::max( mesh.vertices.size(), mesh.indices.size() ) );
			if ( !mesh.indices.empty() ) {
				for ( auto& index : mesh.indices ) this->m_positions.push_back( mesh.vertices[index].position );
			} else {
				for ( auto& vertex : mesh.vertices ) this->m_positions.push_back( vertex.position );
			}
		}

		virtual std::string type() const;
		virtual pod::Vector3* expand() const;
		virtual pod::Vector3 support( const pod::Vector3& ) const;
	};
}