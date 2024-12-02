#include <uf/utils/mesh/grid.h>

namespace {
	inline bool isInside( const pod::Vector3f& p, const pod::Vector3f& min, const pod::Vector3f& max ) {
	//	return ( min.x <= p.x && p.x <= max.x && min.y <= p.y && p.y <= max.y && min.z <= p.z && p.z <= max.z );
		return (p.x <= max.x && p.x >= min.x) && (p.y <= max.y && p.y >= min.y) && (p.z <= max.z && p.z >= min.z);
	}
}

void uf::meshgrid::print( const uf::meshgrid::Grid& grid ) {
	UF_MSG_DEBUG( "== == == ==");
	float total = 0.0f;
	for ( auto& pair : grid.nodes ) { auto& node = pair.second;
		size_t indices = 0;
		for ( auto& pair2 : node.meshlets ) indices += pair2.second.indices.size();
		float percentage = 100.0f * indices / grid.indices;
		total += percentage;
	/*
		UF_MSG_DEBUG( 
			"[" << node.id.x << "," << node.id.y << "," << node.id.z << "] "
			"[" << percentage << "%|" << total << "%] "
			"Min: " << uf::vector::toString( node.extents.min ) << " | "
			"Max: " << uf::vector::toString( node.extents.max ) << " | "
			"Meshlets: " << node.meshlets.size() << " | "
			"Indices: " << indices
		);
	*/
	}
/*
	UF_MSG_DEBUG( "== == == ==");
	UF_MSG_DEBUG( "Min: {}", uf::vector::toString( grid.extents.min ) );
	UF_MSG_DEBUG( "Max: {}", uf::vector::toString( grid.extents.max ) );
	UF_MSG_DEBUG( "Center: {}", uf::vector::toString( grid.extents.center ) );
	UF_MSG_DEBUG( "Corner: {}", uf::vector::toString( grid.extents.corner ) );
	UF_MSG_DEBUG( "Size: {}", uf::vector::toString( grid.extents.size ) );
	UF_MSG_DEBUG( "Piece: {}", uf::vector::toString( grid.extents.piece ) );
	UF_MSG_DEBUG( "Divisions: {}", uf::vector::toString( grid.divisions ) );
	UF_MSG_DEBUG( "Nodes: {}", grid.nodes.size() );
	UF_MSG_DEBUG( "Indices: {}", grid.indices );
	UF_MSG_DEBUG( "== == == ==");
*/
}
void uf::meshgrid::cleanup( uf::meshgrid::Grid& grid ) {
	uf::stl::vector<pod::Vector3ui> eraseNodes;

	for ( auto& pair : grid.nodes ) { auto& node = pair.second;
		
		uf::stl::vector<size_t> eraseMeshlets;
		for ( auto& pair2 : node.meshlets ) { auto& meshlet = pair2.second;
			if ( !meshlet.indices.empty() ) continue;
			eraseMeshlets.emplace_back(pair2.first);
		}
		for ( auto& e : eraseMeshlets ) node.meshlets.erase(e);
		
		if ( !node.meshlets.empty() ) continue;
		eraseNodes.emplace_back(pair.first);
	}
	
	for ( auto& e : eraseNodes ) grid.nodes.erase(e);

/*
	for ( auto it = grid.nodes.begin(); it != grid.nodes.end(); ++it ) {
		auto& node = it->second;
		
		uf::stl::vector<size_t> eraseMeshlets;
		for ( auto it2 = node.meshlets.begin(); it2 != node.meshlets.end(); ++it2 ) {
			auto& meshlet = it2->second;

			if ( !meshlet.indices.empty() ) continue;
			UF_MSG_DEBUG("Erase: " << it2->first);
			it2 = node.meshlets.erase(it2);
		}

		if ( !node.meshlets.empty() ) continue;
		UF_MSG_DEBUG("Erase: " << uf::vector::toString(it->first));
		it = grid.nodes.erase(it);
	}
*/
/*
	for ( auto& pair : grid.nodes ) { auto& node = pair.second;
		for ( auto& pair2 : node.meshlets ) { auto& meshlet = pair2.second;
			if ( !meshlet.indices.empty() ) continue;
	//		node.meshlets.erase(pair2.first);
	//		UF_MSG_DEBUG("Erase: " << pair2.first);
		}
		if ( !node.meshlets.empty() ) continue;
		UF_MSG_DEBUG("Erase: " << uf::vector::toString(pair.first));
		grid.nodes.erase(pair.first);
	}
*/
}

void uf::meshgrid::calculate( uf::meshgrid::Grid& grid, int divisions, float eps ){
	grid.divisions = {divisions, divisions, divisions};
	return calculate( grid, eps );
}
void uf::meshgrid::calculate( uf::meshgrid::Grid& grid, const pod::Vector3ui& divisions, float eps ){
	grid.divisions = divisions; 
	return calculate( grid, eps );
}
void uf::meshgrid::calculate( uf::meshgrid::Grid& grid, float eps ){
	// calculate extents
	constexpr float epsilon = std::numeric_limits<float>::epsilon();
	grid.extents.min -= pod::Vector3f{ epsilon, epsilon, epsilon }; // dilate
	grid.extents.max += pod::Vector3f{ epsilon, epsilon, epsilon }; // dilate
	grid.extents.size = uf::vector::abs(grid.extents.max - grid.extents.min);
	grid.extents.piece = grid.extents.size / grid.divisions;

	grid.extents.center = (grid.extents.max + grid.extents.min) * 0.5f;
	grid.extents.corner = grid.extents.size * 0.5f;


// initialize
	grid.nodes.reserve( grid.divisions.x * grid.divisions.y * grid.divisions.z );
	for ( int z = 0; z < grid.divisions.z; ++z ) {
		for ( int y = 0; y < grid.divisions.y; ++y ) {
			for ( int x = 0; x < grid.divisions.x; ++x ) {
				auto id = pod::Vector3ui{ x, y, z };
				grid.nodes[id] = {
					.extents = {
						.min = grid.extents.min + grid.extents.piece * pod::Vector3f{ x, y, z },
						.max = grid.extents.min + grid.extents.piece * pod::Vector3f{ x+1, y+1, z+1 },
					},
					.id = id
				};
			}
		}
	}
}
void uf::meshgrid::partition( uf::meshgrid::Grid& grid,
	const void* pPointer, size_t pStride,
	const void* iPointer, size_t iStride,
	const pod::Primitive& primitive
) {
	size_t pFirst = primitive.drawCommand.vertexID;
	size_t pCount = primitive.drawCommand.vertices;
	size_t iFirst = primitive.drawCommand.indexID;
	size_t iCount = primitive.drawCommand.indices;

	const float oneOverThree = 1.0f / 3.0f;

	struct Triangle {
		pod::Vector3ui indices;
		pod::Vector3f center;
		pod::Vector3f vertices[3];
	};
	uf::stl::vector<Triangle> rejects;

	for ( auto& pair : grid.nodes ) { auto& node = pair.second;
		auto& meshlet = node.meshlets[primitive.instance.primitiveID];
		meshlet.primitive = primitive;
		meshlet.primitive.instance.bounds.min = node.extents.min;
		meshlet.primitive.instance.bounds.max = node.extents.max;
	}

	// iterate
	for ( size_t i = 0; i < iCount; i+=3 ) {
		Triangle tri{};
		const uint8_t* indexPointers[3] = {
			(static_cast<const uint8_t*>(iPointer) + iStride * (iFirst + i + 0)),
			(static_cast<const uint8_t*>(iPointer) + iStride * (iFirst + i + 1)),
			(static_cast<const uint8_t*>(iPointer) + iStride * (iFirst + i + 2)),
		};

		switch ( iStride ) {
			case  sizeof(uint8_t): {
				tri.indices[0] = *( const uint8_t*) indexPointers[0];
				tri.indices[1] = *( const uint8_t*) indexPointers[1];
				tri.indices[2] = *( const uint8_t*) indexPointers[2];
			} break;
			case sizeof(uint16_t): {
				tri.indices[0] = *(const uint16_t*) indexPointers[0];
				tri.indices[1] = *(const uint16_t*) indexPointers[1];
				tri.indices[2] = *(const uint16_t*) indexPointers[2];
			} break;
			case sizeof(uint32_t): {
				tri.indices[0] = *(const uint32_t*) indexPointers[0];
				tri.indices[1] = *(const uint32_t*) indexPointers[1];
				tri.indices[2] = *(const uint32_t*) indexPointers[2];
			} break;
		}

		tri.vertices[0] = *(const pod::Vector3f*) (static_cast<const uint8_t*>(pPointer) + pStride * (pFirst + tri.indices[0]));
		tri.vertices[1] = *(const pod::Vector3f*) (static_cast<const uint8_t*>(pPointer) + pStride * (pFirst + tri.indices[1]));
		tri.vertices[2] = *(const pod::Vector3f*) (static_cast<const uint8_t*>(pPointer) + pStride * (pFirst + tri.indices[2]));
		
		tri.center = (tri.vertices[0] + tri.vertices[1] + tri.vertices[2]) * oneOverThree;

		bool found = false;
		for ( auto& pair : grid.nodes ) { auto& node = pair.second;
			auto& meshlet = node.meshlets[primitive.instance.primitiveID];

			if ( isInside( tri.vertices[0], node.extents.min, node.extents.max ) || isInside( tri.vertices[1], node.extents.min, node.extents.max ) || isInside( tri.vertices[2], node.extents.min, node.extents.max ) ) {
		//	if ( isInside( tri.center, node.extents.min, node.extents.max ) ) {
				meshlet.indices.emplace_back( tri.indices[0] );
				meshlet.indices.emplace_back( tri.indices[1] );
				meshlet.indices.emplace_back( tri.indices[2] );

				#pragma unroll // GCC unroll N
				for ( uint_fast8_t _ = 0; _ < 3; ++_ ) {
					node.effectiveExtents.min = uf::vector::min( node.effectiveExtents.min, tri.vertices[_] );
					node.effectiveExtents.max = uf::vector::max( node.effectiveExtents.max, tri.vertices[_] );
				}
				found = true;
				break;
			}
		//	if ( isInside( tri.vertices[0], node.extents.min, node.extents.max ) || isInside( tri.vertices[1], node.extents.min, node.extents.max ) || isInside( tri.vertices[2], node.extents.min, node.extents.max ) ) {
			if ( isInside( tri.center, node.extents.min, node.extents.max ) ) {
				meshlet.indices.emplace_back( tri.indices[0] );
				meshlet.indices.emplace_back( tri.indices[1] );
				meshlet.indices.emplace_back( tri.indices[2] );

				#pragma unroll // GCC unroll N
				for ( uint_fast8_t _ = 0; _ < 3; ++_ ) {
					node.effectiveExtents.min = uf::vector::min( node.effectiveExtents.min, tri.vertices[_] );
					node.effectiveExtents.max = uf::vector::max( node.effectiveExtents.max, tri.vertices[_] );
				}
				found = true;
				break;
			}
		}
		if ( found ) {
			continue;
		}
		rejects.emplace_back(tri);
	}

	for ( auto& tri : rejects ) {
		uf::meshgrid::Node* closest = NULL;
		float closestDistance = std::numeric_limits<float>::max();

		for ( auto& pair : grid.nodes ) { auto& node = pair.second;
			auto nodeCenter = (node.extents.max + node.extents.min) * 0.5f;
			float curDistance = uf::vector::distanceSquared( nodeCenter, tri.center );
			if ( curDistance < closestDistance ) {
				closestDistance = curDistance;
				closest = &node;
			}
		}
		if ( closest ) {
			auto& meshlet = closest->meshlets[primitive.instance.primitiveID];
			meshlet.indices.emplace_back( tri.indices[0] );
			meshlet.indices.emplace_back( tri.indices[1] );
			meshlet.indices.emplace_back( tri.indices[2] );

			#pragma unroll // GCC unroll N
			for ( uint_fast8_t _ = 0; _ < 3; ++_ ) {
				closest->effectiveExtents.min = uf::vector::min( closest->effectiveExtents.min, tri.vertices[_] );
				closest->effectiveExtents.max = uf::vector::max( closest->effectiveExtents.max, tri.vertices[_] );
			}
		} else {
		}
	}

	grid.indices += iCount;
}
void uf::meshgrid::partition( uf::meshgrid::Grid& grid, uf::Mesh& mesh, const pod::Primitive& primitive ) {
	uf::Mesh::Input vertexInput = mesh.vertex;
	uf::Mesh::Input indexInput = mesh.index;

	uf::Mesh::Attribute vertexAttribute = mesh.vertex.attributes.front();
	uf::Mesh::Attribute indexAttribute = mesh.index.attributes.front();

	for ( auto& attribute : mesh.vertex.attributes ) if ( attribute.descriptor.name == "position" ) { vertexAttribute = attribute; break; }
	UF_ASSERT( vertexAttribute.descriptor.name == "position" );

	return partition( grid, 
		static_cast<uint8_t*>(vertexAttribute.pointer) + vertexAttribute.offset, vertexAttribute.stride,
		static_cast<uint8_t*>(indexAttribute.pointer) + indexAttribute.offset, indexAttribute.stride,
		primitive
	);
}