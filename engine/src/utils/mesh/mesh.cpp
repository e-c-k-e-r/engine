#include <uf/utils/mesh/mesh.h>

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

pod::Mesh::~Mesh() {}
void pod::Mesh::generateIndices() {
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
void pod::Mesh::updateDescriptor() {}
void pod::Mesh::resizeVertices( size_t n ) {}
void pod::Mesh::resizeIndices( size_t n ) {}

pod::Mesh& pod::VaryingMesh::get() {
	return uf::pointeredUserdata::get<pod::Mesh>( userdata, false );
}
const pod::Mesh& pod::VaryingMesh::get() const {
	return uf::pointeredUserdata::get<pod::Mesh>( userdata, false );
}

void pod::VaryingMesh::destroy() {
	uf::pointeredUserdata::destroy( userdata );
}
void pod::VaryingMesh::updateDescriptor() {
	auto& mesh = get();
	mesh.updateDescriptor();
	attributes = mesh.attributes;
}

void pod::VaryingMesh::resizeVertices( size_t n ) {
	auto& mesh = get();
	mesh.resizeVertices( n );
	attributes = mesh.attributes;
}
void pod::VaryingMesh::resizeIndices( size_t n ) {
	auto& mesh = get();
	mesh.resizeIndices( n );
	attributes = mesh.attributes;
}