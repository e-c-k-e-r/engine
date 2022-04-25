#include <uf/utils/mesh/grid.h>

namespace {
	inline bool isInside( const pod::Vector3f& p, const pod::Vector3f& min, const pod::Vector3f& max ) {
	//	return ( min.x <= p.x && p.x <= max.x && min.y <= p.y && p.y <= max.y && min.z <= p.z && p.z <= max.z );
		return (p.x <= max.x && p.x >= min.x) && (p.y <= max.y && p.y >= min.y) && (p.z <= max.z && p.z >= min.z);
	}

}

void UF_API uf::meshgrid::print( const uf::meshgrid::Grid& grid ) {
	UF_MSG_DEBUG( "== == == ==");
	float total = 0.0f;
	for ( auto& pair : grid.nodes ) { auto& node = pair.second;
		size_t indices = 0;
		for ( auto& pair2 : node.meshlets ) indices += pair2.second.indices.size();
		float percentage = 100.0f * indices / grid.indices;
		total += percentage;
		UF_MSG_DEBUG( 
			"[" << node.id.x << "," << node.id.y << "," << node.id.z << "] "
			"[" << percentage << "%|" << total << "%] "
			"Min: " << uf::vector::toString( node.extents.min ) << " | "
			"Max: " << uf::vector::toString( node.extents.max ) << " | "
			"Meshlets: " << node.meshlets.size() << " | "
			"Indices: " << indices
		);
	}

	UF_MSG_DEBUG( "== == == ==");
	UF_MSG_DEBUG( "Min: " << uf::vector::toString( grid.extents.min ) );
	UF_MSG_DEBUG( "Max: " << uf::vector::toString( grid.extents.max ) );
	UF_MSG_DEBUG( "Center: " << uf::vector::toString( grid.extents.center ) );
	UF_MSG_DEBUG( "Corner: " << uf::vector::toString( grid.extents.corner ) );
	UF_MSG_DEBUG( "Size: " << uf::vector::toString( grid.extents.size ) );
	UF_MSG_DEBUG( "Piece: " << uf::vector::toString( grid.extents.piece ) );
	UF_MSG_DEBUG( "Divisions: " << uf::vector::toString( grid.divisions ) );
	UF_MSG_DEBUG( "Nodes: " << grid.nodes.size() );
	UF_MSG_DEBUG( "Indices: " << grid.indices );
	UF_MSG_DEBUG( "== == == ==");
}
void UF_API uf::meshgrid::cleanup( uf::meshgrid::Grid& grid ) {
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
		}
		if ( found ) continue;
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

		//	UF_MSG_DEBUG("REASSIGNED TRI: " << uf::vector::toString(tri.center) << " -> " << uf::vector::toString(closest->id) << " (" << closestDistance << ")");
		} else {
		//	UF_MSG_DEBUG("REJECT TRI: " << tri.center.x << " " << tri.center.y << " " << tri.center.z);
		}
	}

	grid.indices += iCount;
}
void UF_API uf::meshgrid::partition( uf::meshgrid::Grid& grid, uf::Mesh& mesh, const pod::Primitive& primitive ) {
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

#if 0
#include <uf/utils/memory/pool.h>
#include <uf/utils/mesh/grid.h>
#include <uf/utils/math/collision/mesh.h>
#include <uf/utils/math/collision/boundingbox.h>

uf::MeshGrid::~MeshGrid() {
	this->destroy();
}

void uf::MeshGrid::initialize( const pod::Vector3f& center, const pod::Vector3f& extent, const pod::Vector3ui& size ) {
	float epsilon = 1.0f;
	this->m_center = center;
	this->m_extent = extent + pod::Vector3f{ epsilon, epsilon, epsilon };
	this->m_size = size;

	pod::Vector3f min = center - extent;
	pod::Vector3f max = center + extent;

	this->m_nodes.reserve(size.x * size.y * size.y);
	for ( size_t z = 0; z < size.z; ++z )
	for ( size_t y = 0; y < size.y; ++y )
	for ( size_t x = 0; x < size.x; ++x ) {
		auto& node = this->m_nodes.emplace_back();
		node.extent = pod::Vector3f{ extent.x / size.x, extent.y / size.y, extent.z / size.z };
		node.center = min + node.extent * pod::Vector3f{ 1 + 2 * x, 1 + 2 * y, 1 + 2 * z };
	}
}
void uf::MeshGrid::destroy() {
#if 0 
	for ( auto& node : this->m_nodes ) {
	#if UF_MEMORYPOOL_INVALID_FREE
		uf::memoryPool::global.free( node.indices );
	#else
		uf::MemoryPool* memoryPool = uf::memoryPool::global.size() > 0 ? &uf::memoryPool::global : NULL;	
		if ( memoryPool ) memoryPool->free( node.indices );
		else free( node.indices );
	#endif
		node.size = 0;
		node.indices = NULL;
	}
#endif
	this->m_nodes.clear();
}

void uf::MeshGrid::allocate( uf::MeshGrid::Node& node, size_t indices ) {
#if 0
	size_t size = indices * sizeof(uint32_t);
	uint32_t* buffer = NULL;

#if UF_MEMORYPOOL_INVALID_MALLOC
	buffer = (void*) uf::memoryPool::global.alloc( data, size );
#else
	uf::MemoryPool* memoryPool = uf::memoryPool::global.size() > 0 ? &uf::memoryPool::global : NULL;
	if ( memoryPool )
		buffer = (void*) memoryPool->alloc( data, size );
	else {
		buffer = malloc( size );
		memset( buffer, 0, size );
	}
#endif
	node.count = indices;
	node.indices = buffer;
#else
	node.indices.reserve( indices );
#endif
}

uf::MeshGrid::Node& uf::MeshGrid::at( const pod::Vector3f& point ) {
	// naive way, bound check every node
	for ( auto& node : this->m_nodes ) {
		pod::Vector3f min = node.center - node.extent;
		pod::Vector3f max = node.center + node.extent;
		if ( min.x < point.x && point.x < max.x && min.y < point.y && point.y < max.y && min.z < point.z && point.z < max.z ) return node;
	//	if ( min <= point && point <= max ) return node;
	}
	return this->m_nodes.front();
}
const uf::MeshGrid::Node& uf::MeshGrid::at( const pod::Vector3f& point ) const {
	// naive way, bound check every node
	for ( auto& node : this->m_nodes ) {
		pod::Vector3f min = node.center - node.extent;
		pod::Vector3f max = node.center + node.extent;
		if ( min.x < point.x && point.x < max.x && min.y < point.y && point.y < max.y && min.z < point.z && point.z < max.z ) return node;
	//	if ( min <= point && point <= max ) return node;
	}
	return this->m_nodes.front();
}
pod::Vector3f uf::MeshGrid::centroid( const pod::Vector3f& p1, const pod::Vector3f& p2, const pod::Vector3f& p3 ) const {
	return uf::vector::divide(p1 + p2 + p3, 3.0f);
}

const pod::Vector3f& uf::MeshGrid::center() const { return this->m_center; }
const pod::Vector3f& uf::MeshGrid::extent() const { return this->m_extent; }

void uf::MeshGrid::insert( uint32_t i1, uint32_t i2, uint32_t i3, const pod::Vector3f& v1, const pod::Vector3f& v2, const pod::Vector3f& v3 ) {
#if 0
	bool useStrongest = false;
	int32_t strongestNode = -1;
	pod::Collider::Manifold strongestManifold = {};	
	for ( size_t i = 0; i < this->m_nodes.size(); ++i ) {
		auto& node = this->m_nodes[i];
		pod::Transform<> transform;
		uf::MeshCollider cTriangle( transform );
		cTriangle.setPositions({v1,v2,v3});

		uf::BoundingBox cNode;
		cNode.setOrigin( node.center );
		cNode.setCorner( node.extent );

		auto manifold = cTriangle.intersects( cNode );
		if ( manifold.colliding ) {
			if ( useStrongest && manifold.depth > strongestManifold.depth ) {
				strongestManifold = manifold;
			} else {
				node.indices.emplace_back(i1);
				node.indices.emplace_back(i2);
				node.indices.emplace_back(i3);
			}
		}
	}
	if ( useStrongest && strongestNode >= 0 ) {
		auto& node = this->m_nodes[strongestNode];
		node.indices.emplace_back(i1);
		node.indices.emplace_back(i2);
		node.indices.emplace_back(i3);
	}
#endif
#if 1
	auto center = centroid( v1, v2, v3 );
	for ( size_t i = 0; i < this->m_nodes.size(); ++i ) {
		auto& node = this->m_nodes[i];
		pod::Vector3f min = node.center - node.extent;
		pod::Vector3f max = node.center + node.extent;
		if ( 
			( min.x < v1.x && v1.x < max.x && min.y < v1.y && v1.y < max.y && min.z < v1.z && v1.z < max.z ) ||
			( min.x < v2.x && v2.x < max.x && min.y < v2.y && v2.y < max.y && min.z < v2.z && v2.z < max.z ) ||
			( min.x < v3.x && v3.x < max.x && min.y < v3.y && v3.y < max.y && min.z < v3.z && v3.z < max.z ) ||
			( min.x < center.x && center.x < max.x && min.y < center.y && center.y < max.y && min.z < center.z && center.z < max.z )	
	//		( min <= v1 && v1 <= max ) || ( min <= v2 && v2 <= max ) || ( min <= v3 && v3 <= max ) || ( min <= center && center <= max )
		) {
			node.indices.emplace_back(i1);
			node.indices.emplace_back(i2);
			node.indices.emplace_back(i3);
		}
	}
#endif
#if 0
	// get associated node
	auto& node = at( centroid( v1, v2, v3 ) );
	// naive way: inserting triangle's indices rather than the triangle's position in the indices
	node.indices.emplace_back(i1);
	node.indices.emplace_back(i2);
	node.indices.emplace_back(i3);
#endif
}

void uf::MeshGrid::analyze() const {
	pod::Vector3f min = this->m_center - this->m_extent;
	pod::Vector3f max = this->m_center + this->m_extent;
	UF_MSG_DEBUG(
		"Region count: " << this->m_nodes.size() << " | "
		"Size: " << uf::vector::toString(this->m_size) << " | "
		"Center: " << uf::vector::toString(this->m_center) << " | "
		"Extent: " << uf::vector::toString(this->m_extent) << " | "
		"Min: " << uf::vector::toString(min) << " | "
		"Max: " << uf::vector::toString(max)
	);
	size_t total = 0;
	for ( auto& node : this->m_nodes ) total += node.indices.size();
	for ( size_t i = 0; i < this->m_nodes.size(); ++i ) {
		auto& node = this->m_nodes[i];
		UF_MSG_DEBUG("Node[" << i << "]: Center: " << uf::vector::toString(node.center) << " | Percentage: " << ( 100.0f * (float) node.indices.size() / (float) total ) << "%" );
	}
}

#if 0
namespace {
	enum Swizzle {
		XYZ, YZX, ZXY
	} swizzle = Swizzle::YZX;
}
size_t uf::MeshGrid::wrapPosition( const pod::Vector3f& position ) {
	// ordered from first to change to last to change
	pod::Vector3f input;
	pod::Vector3f size;
	switch ( ::swizzle ) {
		case Swizzle::YZX:
			input = {
				position.x,
				position.z,
				position.y,
			};
			size = {
				this->m_size.x,
				this->m_size.z,
				this->m_size.y,
			};
		break;
		case Swizzle::ZXY:
			input = {
				position.y,
				position.x,
				position.z,
			};
			size = {
				this->m_size.y,
				this->m_size.x,
				this->m_size.z,
			};
		break;
		case Swizzle::XYZ:
			input = {
				position.z,
				position.y,
				position.x,
			};
			size = {
				this->m_size.z,
				this->m_size.y,
				this->m_size.x,
			};
		break;
	}
	return size.y * size.x * input.z + size.x * input.y + input.x;
}
pod::Vector3i uf::MeshGrid::unwrapIndex( size_t i ) {
	// ordered from first to change to last to change
	/*
		result.x = i % this->m_size.x;
		result.y = ( i / this->m_size.x ) % this->m_size.y;
		result.z = i / ( this->m_size.x * this->m_size.y );
	*/
	pod::Vector3ui size;
	switch ( ::swizzle ) {
		case Swizzle::YZX:
			size = {
				this->m_size.x,
				this->m_size.z,
				this->m_size.y,
			};
			return {
				i % size.x,
				i / ( size.x * size.y ),
				( i / size.x ) % size.y,
			};
		break;
		case Swizzle::ZXY:
			size = {
				this->m_size.y,
				this->m_size.x,
				this->m_size.z,
			};
			return {
				( i / size.x ) % size.y,
				i % size.x,
				i / ( size.x * size.y ),
			};
		break;
		case Swizzle::XYZ:
		default:
			size = {
				this->m_size.z,
				this->m_size.y,
				this->m_size.x,
			};
			return {
				( i / size.x ) % size.y,
				i / ( size.x * size.y ),
				i % size.x,
			};
		break;
	}
}
#endif
#endif