#include <uf/utils/mempool/mempool.h>
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
		uf::MemoryPool::global.free( node.indices );
	#else
		uf::MemoryPool* memoryPool = uf::MemoryPool::global.size() > 0 ? &uf::MemoryPool::global : NULL;	
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
	buffer = (void*) uf::MemoryPool::global.alloc( data, size );
#else
	uf::MemoryPool* memoryPool = uf::MemoryPool::global.size() > 0 ? &uf::MemoryPool::global : NULL;
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
	return (p1 + p2 + p3) / 3;
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
	UF_DEBUG_MSG(
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
		UF_DEBUG_MSG("Node[" << i << "]: Center: " << uf::vector::toString(node.center) << " | Percentage: " << ( 100.0f * (float) node.indices.size() / (float) total ) << "%" );
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