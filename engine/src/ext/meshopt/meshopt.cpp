#include <uf/ext/meshopt/meshopt.h>
#if UF_USE_MESHOPT
#include <meshoptimizer.h>

bool ext::meshopt::optimize( uf::Mesh& mesh, float simplify, size_t o, bool verbose ) {
	if ( mesh.isInterleaved() ) {
		UF_MSG_ERROR("optimization of interleaved meshes is currently not supported. Consider optimizing on meshlets.");
		return false;
	}
	mesh.updateDescriptor();

	struct Remap {
		uf::Mesh::Attribute attribute;
		uf::stl::vector<uint8_t> buffer;
	};

	uf::stl::vector<Remap> attributes;
	uf::stl::vector<meshopt_Stream> streams;
	for ( auto& attribute : mesh.vertex.attributes ) {
		auto& p = attributes.emplace_back();
		p.attribute = attribute;

		auto& stream = streams.emplace_back();
		stream.data = p.attribute.pointer;
		stream.size = p.attribute.descriptor.size;
		stream.stride = p.attribute.stride;
	}

	bool hasIndices = mesh.index.count > 0;
	size_t indicesCount = hasIndices ? mesh.index.count : mesh.vertex.count;
	uf::stl::vector<uint32_t> remap(indicesCount);

	uint32_t* sourceIndicesPointer = NULL;
	uf::stl::vector<uint32_t> sourceIndices(indicesCount);
	if ( hasIndices ) {
		uint8_t* pointer = (uint8_t*) mesh.getBuffer(mesh.index).data();
		for ( auto index = 0; index < indicesCount; ++index ) {
			switch ( mesh.index.size ) {
				case 1: sourceIndices[index] = (( uint8_t*) pointer)[index]; break;
				case 2: sourceIndices[index] = ((uint16_t*) pointer)[index]; break;
				case 4: sourceIndices[index] = ((uint32_t*) pointer)[index]; break;
			}
		}
	}

	size_t verticesCount = meshopt_generateVertexRemapMulti( &remap[0], sourceIndicesPointer, indicesCount, indicesCount, &streams[0], streams.size() );

	// generate new indices, as they're going to be specific to a region of vertices due to drawcommand shittery
	uf::stl::vector<uint32_t> indices(indicesCount);
	meshopt_remapIndexBuffer(&indices[0], sourceIndicesPointer, indicesCount, &remap[0]);

	// 
	for ( auto& p : attributes ) {
		auto& buffer = p.buffer;
		buffer.resize(verticesCount * p.attribute.descriptor.size);

		meshopt_remapVertexBuffer(&buffer[0], p.attribute.pointer, indicesCount, p.attribute.descriptor.size, &remap[0]);
	}
	//
	meshopt_optimizeVertexCache(&indices[0], &indices[0], indicesCount, verticesCount);
	//
	meshopt_optimizeVertexFetchRemap(&remap[0], &indices[0], indicesCount, verticesCount);
	//
	for ( auto& p : attributes ) {
		auto& buffer = p.buffer;
		p.attribute.pointer = &buffer[0];

		meshopt_remapVertexBuffer(p.attribute.pointer, p.attribute.pointer, verticesCount, p.attribute.descriptor.size, &remap[0]);
	}
	// almost always causes ID discontinuities
	if ( 0.0f < simplify && simplify < 1.0f ) {
		uf::stl::vector<uint32_t> indicesSimplified(indicesCount);

		uf::Mesh::Attribute positionAttribute;
		for ( auto& p : attributes ) if ( p.attribute.descriptor.name == "position" ) positionAttribute = p.attribute;

		size_t targetIndices = indicesCount * simplify;
		float targetError = 1e-2f / simplify;

		float realError = 0.0f;
		size_t realIndices = meshopt_simplify(&indicesSimplified[0], &indices[0], indicesCount, (float*) positionAttribute.pointer, verticesCount, positionAttribute.stride, targetError, realError);
	//	size_t realIndices = meshopt_simplifySloppy(&indicesSimplified[0], &indices[0], indicesCount, (float*) positionAttribute.pointer, verticesCount, positionAttribute.stride, targetIndices);
		
		if ( verbose ) UF_MSG_DEBUG("[Simplified] indices: {} -> {} | error: {} -> {}", indicesCount, realIndices, targetError, realError);

		indicesCount = realIndices;
		indices.swap( indicesSimplified );
	}
	// done
	if ( mesh.indirect.count ) {
		bool discontinuityDetected = false;
		size_t lastID = 0;
		struct Remap {
			pod::DrawCommand* drawCommand;
			struct {
				size_t start = SIZE_MAX;
				size_t end = 0;
			} index, vertex;
		};
	
		pod::DrawCommand* drawCommands = (pod::DrawCommand*) mesh.getBuffer(mesh.indirect).data();
		uf::stl::vector<Remap> remappedDrawCommands( mesh.indirect.count );

		uf::Mesh::Attribute idAttribute;
		for ( auto& p : attributes ) if ( p.attribute.descriptor.name == "id" ) idAttribute = p.attribute;
		for ( size_t index = 0; index < indicesCount; ++index ) {
			size_t vertex = indices[index];
			pod::Vector<uint16_t,2>& id = *(pod::Vector<uint16_t,2>*) ( static_cast<uint8_t*>(idAttribute.pointer) + idAttribute.stride * vertex );

			auto& d = remappedDrawCommands[id.x];
			d.drawCommand = &drawCommands[id.x];
			d.vertex.start = std::min( d.vertex.start, vertex );
			d.vertex.end = std::max( d.vertex.end, vertex );
			
			d.index.start = std::min( d.index.start, index );
			d.index.end = std::max( d.index.end, index );

			if ( lastID == id.x ) {

			} else if ( lastID + 1 == id.x )  {
				lastID = id.x;
			} else {
				if ( verbose ) UF_MSG_DEBUG("Discontinuity detected: {} | {} | {} | {}", index, vertex, lastID, id.x);
				discontinuityDetected = true;
				lastID = id.x;
			}
		}

		if ( discontinuityDetected ) {
			UF_MSG_ERROR("Discontinuity detected, bailing...");
			return false;
		}

		for ( auto& d : remappedDrawCommands ) {
			d.drawCommand->indices = d.index.end - d.index.start + 1;
			d.drawCommand->indexID = d.index.start;
			d.drawCommand->vertexID = d.vertex.start;
			d.drawCommand->vertices = d.vertex.end - d.vertex.start + 1;
		}

		for ( size_t index = 0; index < indicesCount; ++index ) {
			auto& vertex = indices[index];
			pod::Vector<uint16_t,2>& id = *(pod::Vector<uint16_t,2>*) ( static_cast<uint8_t*>(idAttribute.pointer) + idAttribute.stride * vertex );

			auto& d = remappedDrawCommands[id.x];
			vertex -= d.vertex.start;
		}
	}

	mesh.index.count = indicesCount;
	mesh.vertex.count = verticesCount;

	// vertices
	for ( size_t i = 0; i < attributes.size(); ++i ) {
		auto& attribute = mesh.vertex.attributes[i];
		auto& remapped = attributes[i];

		mesh.buffers[attribute.buffer].swap( remapped.buffer );
		attribute.pointer = mesh.buffers[attribute.buffer].data();
	}

	// indices
	{
		mesh.resizeIndices( mesh.index.count );
		uint8_t* pointer = (uint8_t*) mesh.getBuffer(mesh.index).data();
		for ( auto index = 0; index < indicesCount; ++index ) {
			switch ( mesh.index.size ) {
				case 1: (( uint8_t*) pointer)[index] = indices[index]; break;
				case 2: ((uint16_t*) pointer)[index] = indices[index]; break;
				case 4: ((uint32_t*) pointer)[index] = indices[index]; break;
			}
		}
	}

	mesh.updateDescriptor();
	return true;
}
#endif