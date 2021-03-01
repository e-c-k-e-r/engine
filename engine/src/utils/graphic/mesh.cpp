#include <uf/utils/graphic/mesh.h>

// Used for per-vertex colors
UF_VERTEX_DESCRIPTOR(pod::Vertex_3F2F3F4F,
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F4F, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F4F, R32G32_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F4F, R32G32B32_SFLOAT, normal)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F4F, R32_UINT, color)
)
// Used for terrain
UF_VERTEX_DESCRIPTOR(pod::Vertex_3F2F3F32B,
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F32B, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F32B, R32G32_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F32B, R32G32B32_SFLOAT, normal)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F32B, R32_UINT, color)
)
// Used for normal meshses
UF_VERTEX_DESCRIPTOR(pod::Vertex_3F2F3F,
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F, R32G32_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F, R32G32B32_SFLOAT, normal)
)
// (Typically) used for displaying textures
UF_VERTEX_DESCRIPTOR(pod::Vertex_3F2F,
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F, R32G32_SFLOAT, uv)
)
UF_VERTEX_DESCRIPTOR(pod::Vertex_2F2F,
	UF_VERTEX_DESCRIPTION(pod::Vertex_2F2F, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(pod::Vertex_2F2F, R32G32_SFLOAT, uv)
)
// used for texture arrays
UF_VERTEX_DESCRIPTOR(pod::Vertex_3F3F3F,
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F3F3F, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F3F3F, R32G32B32_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F3F3F, R32G32B32_SFLOAT, normal)
)
UF_VERTEX_DESCRIPTOR(pod::Vertex_3F2F3F1UI,
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F1UI, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F1UI, R32G32_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F1UI, R32G32B32_SFLOAT, normal)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F1UI, R32_UINT, id)
)
// Basic
UF_VERTEX_DESCRIPTOR(pod::Vertex_3F,
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F, R32G32B32_SFLOAT, position)
)

uf::BaseGeometry::~BaseGeometry() {}
void uf::BaseGeometry::generateIndices() {
	this->resizeIndices(this->attributes.vertex.length);
	switch ( this->attributes.index.size ) {
		case sizeof(uint32_t):
			for ( size_t i = 0; i < this->attributes.vertex.length; ++i ) ((uint32_t*) this->attributes.index.pointer)[i] = i;
		break;
		case sizeof(uint16_t):
			for ( size_t i = 0; i < this->attributes.vertex.length; ++i ) ((uint16_t*) this->attributes.index.pointer)[i] = i;
		break;
		case sizeof(uint8_t):
			for ( size_t i = 0; i < this->attributes.vertex.length; ++i ) ((uint8_t*) this->attributes.index.pointer)[i] = i;
		break;
	}
}
void uf::BaseGeometry::updateDescriptor() {}
void uf::BaseGeometry::resizeVertices( size_t n ) {}
void uf::BaseGeometry::resizeIndices( size_t n ) {}