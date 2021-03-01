#include <uf/utils/mesh/mesh.h>
#if UF_USE_MESHOPTIMIZER
	#include <meshoptimizer.h>
#endif

void ext::meshopt::optimize( uf::BaseGeometry& mesh, size_t o ) {
#if 0
	mesh.updateDescriptor();

	void* vertices = NULL;
	void* indices = NULL;
	size_t verticesCount = mesh.attributes.vertex.length;
	size_t indicesCount = mesh.attributes.index.length;

	if ( !indicesCount ) mesh.generateIndices();
	if ( o == 0 ) {
		if ( !indicesCount ) {
			mesh.resizeIndices(verticesCount);
			switch ( mesh.attributes.index.size ) {
				case sizeof(uint32_t):
					for ( size_t i = 0; i < verticesCount; ++i ) ((uint32_t*) mesh.attributes.index.pointer)[i] = i;
				break;
				case sizeof(uint16_t):
					for ( size_t i = 0; i < verticesCount; ++i ) ((uint16_t*) mesh.attributes.index.pointer)[i] = i;
				break;
				case sizeof(uint8_t):
					for ( size_t i = 0; i < verticesCount; ++i ) ((uint8_t*) mesh.attributes.index.pointer)[i] = i;
				break;
			}
		}
		return;
	}
	{
		vertices = malloc(verticesCount * mesh.attributes.vertex.size);
		memcpy( vertices, mesh.attributes.vertex.pointer, verticesCount );
	}
	if ( indicesCount > 0 ) {
		indices = malloc(indicesCount);
		memcpy( indices, mesh.attributes.index.pointer, indicesCount );
	}

	std::vector<uint32_t> remap(verticesCount);
	verticesCount = meshopt_generateVertexRemap(&remap[0], (uint32_t*) indices, indicesCount ? indicesCount : verticesCount, (float*) vertices, verticesCount, mesh.attributes.vertex.size);
	
	mesh.resizeIndices(indicesCount);
	mesh.resizeVertices(verticesCount);

	meshopt_remapIndexBuffer((uint32_t*) mesh.attributes.index.pointer, (uint32_t*) indices, indicesCount, &remap[0]);
	meshopt_remapVertexBuffer(mesh.attributes.vertex.pointer, (float*) vertices, verticesCount, mesh.attributes.vertex.size, &remap[0]);

	if ( vertices ) free(vertices);
	if ( indices ) free(indices);
#endif
#if 0
	auto vertices = std::move( this->vertices );
	size_t valueCount = vertices.size();

	std::vector<uint32_t> remap(valueCount);
	size_t verticesCount = meshopt_generateVertexRemap(&remap[0], NULL, valueCount, &vertices[0], valueCount, sizeof(T));
	
	this->indices.resize(valueCount);
	meshopt_remapIndexBuffer(&this->indices[0], NULL, valueCount, &remap[0]);
	//meshopt_remapIndexBuffer(&this->indices[0], (const U*) NULL, indices, &remap[0]);
	this->vertices.resize(verticesCount);
	meshopt_remapVertexBuffer(&this->vertices[0], &vertices[0], valueCount, sizeof(T), &remap[0]);

	size_t verticesCount = this->vertices.size();
	size_t indicesCount = this->indices.size();
	if ( indicesCount == 0 ) indicesCount = verticesCount;

	auto vertices = std::move( this->vertices );
	auto indices = std::move( this->indices );

	std::vector<uint32_t> remap(indicesCount);
	verticesCount = meshopt_generateVertexRemap(&remap[0], &indices[0], indicesCount, &vertices[0], verticesCount, sizeof(T));
	
	this->indices.resize(indicesCount);
	this->vertices.resize(verticesCount);

	meshopt_remapIndexBuffer(&this->indices[0], &indices[0], indicesCount, &remap[0]);
	meshopt_remapVertexBuffer(&this->vertices[0], &vertices[0], verticesCount, sizeof(T), &remap[0]);
#endif
#if 0
	// optimize for cache
	if ( o >= 2 ) {
		meshopt_optimizeVertexCache(mesh.attributes.index.pointer, mesh.attributes.index.pointer, mesh.attributes.index.length, mesh.attributes.vertex.length);
	}
	// optimize for overdraw
	if ( o >= 3 ) {
		const float kOverdrawThreshold = 3.f;
		meshopt_optimizeOverdraw(mesh.attributes.index.pointer, mesh.attributes.index.pointer, mesh.attributes.index.length, (float*) mesh.attributes.vertex.pointer.position, mesh.attributes.vertex.length, sizeof(T), kOverdrawThreshold);
	}
	// optimize for fetch
	if ( o >= 4 ) {
		meshopt_optimizeVertexFetch(mesh.attributes.vertex.pointer, mesh.attributes.index.pointer, mesh.attributes.index.length, mesh.attributes.vertex.pointer, mesh.attributes.vertex.length, mesh.attributes.vertex.size);
	}
#endif
}


/*
auto vertices = std::move( this->vertices );
U indices = vertices.size();

std::vector<uint32_t> remap(indices);
size_t verticesCount = meshopt_generateVertexRemap(&remap[0], NULL, indices, &vertices[0], indices, sizeof(T));

this->indices.resize(indices);
//meshopt_remapIndexBuffer(&this->indices[0], NULL, indices, &remap[0]);
meshopt_remapIndexBuffer(&this->indices[0], (const U*) NULL, indices, &remap[0]);
this->vertices.resize(verticesCount);
meshopt_remapVertexBuffer(&this->vertices[0], &vertices[0], indices, sizeof(T), &remap[0]);
// optimize for cache
if ( o >= 1 ) {
	meshopt_optimizeVertexCache(&this->indices[0], &this->indices[0], this->indices.size(), this->vertices.size());
}
// optimize for overdraw
if ( o >= 2 ) {
	const float kOverdrawThreshold = 3.f;
	meshopt_optimizeOverdraw(&this->indices[0], &this->indices[0], this->indices.size(), (float*) &this->vertices[0].position, this->vertices.size(), sizeof(T), kOverdrawThreshold);
}
// optimize for fetch
if ( o >= 3 ) {
	meshopt_optimizeVertexFetch(&this->vertices[0], &this->indices[0], this->indices.size(), &this->vertices[0], this->vertices.size(), sizeof(T));
}
*/