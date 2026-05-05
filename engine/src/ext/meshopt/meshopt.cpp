#include <uf/ext/meshopt/meshopt.h>

#if UF_USE_MESHOPT
#include <meshoptimizer.h>
#include <cfloat>

namespace {
	uint32_t readIndex(const uint8_t* ptr, size_t index, size_t size) {
		switch (size) {
			case 1: return ((const uint8_t*)ptr)[index];
			case 2: return ((const uint16_t*)ptr)[index];
			case 4: return ((const uint32_t*)ptr)[index];
			default: return 0;
		}
	}
	void writeIndex(uint8_t* ptr, size_t index, size_t size, uint32_t value) {
		switch (size) {
			case 1: ((uint8_t*)ptr)[index]  = (uint8_t)value; break;
			case 2: ((uint16_t*)ptr)[index] = (uint16_t)value; break;
			case 4: ((uint32_t*)ptr)[index] = (uint32_t)value; break;
		}
	}
}

bool ext::meshopt::optimize( uf::Mesh& mesh, float simplify, size_t o, bool verbose ) {
	if ( mesh.isInterleaved() ) {
		UF_MSG_ERROR("Optimization of interleaved meshes is currently not supported.");
		return false;
	}
	mesh.updateDescriptor();

	const auto& views = mesh.buffer_views;
	if ( views.empty() ) return false;

	pod::DrawCommand* drawCommands = mesh.indirect.count > 0 ? (pod::DrawCommand*) mesh.getBuffer(mesh.indirect).data() : nullptr;
	const uint8_t* srcIndexData = mesh.index.count > 0 ? mesh.getBuffer(mesh.index).data() : nullptr;

	uf::stl::vector<uint32_t> outIndices;
	uf::stl::vector<uf::stl::vector<uint8_t>> outVertices(mesh.vertex.attributes.size());

	uf::Mesh::Attribute positionAttribute;
	for ( auto& attr : mesh.vertex.attributes ) if ( attr.descriptor.name == "position" ) positionAttribute = attr;

	for ( size_t viewIdx = 0; viewIdx < views.size(); ++viewIdx ) {
		const auto& view = views[viewIdx];
		uint32_t cmdIdx = view.indirectIndex;

		uint32_t srcVertexOffset = view.vertex.first;
		uint32_t srcVertexCount  = view.vertex.count;
		uint32_t srcIndexOffset  = view.index.first;
		uint32_t srcIndexCount   = view.index.count;

		if ( srcIndexCount == 0 ) continue;

		// retrieve indices
		uf::stl::vector<uint32_t> localIndices(srcIndexCount);
		if ( srcIndexData ) {
			for ( size_t i = 0; i < srcIndexCount; ++i ) {
				localIndices[i] = readIndex(srcIndexData, srcIndexOffset + i, mesh.index.size);
			}
		} else {
			for ( size_t i = 0; i < srcIndexCount; ++i ) localIndices[i] = i;
		}

		// setup streams
		uf::stl::vector<meshopt_Stream> streams;
		for ( auto& attr : mesh.vertex.attributes ) {
			const uint8_t* basePtr = (const uint8_t*)attr.pointer + srcVertexOffset * attr.stride;
			streams.emplace_back(meshopt_Stream{ basePtr, attr.descriptor.size, attr.stride });
		}

		// deduplicate vertices
		uf::stl::vector<uint32_t> remap(srcVertexCount);
		size_t uniqueVertices = meshopt_generateVertexRemapMulti(
			remap.data(), localIndices.data(), srcIndexCount,
			srcVertexCount, streams.data(), streams.size()
		);
		meshopt_remapIndexBuffer(localIndices.data(), localIndices.data(), srcIndexCount, remap.data());

		// copy position data
		uf::stl::vector<uint8_t> tempPositions(uniqueVertices * positionAttribute.stride);
		const uint8_t* srcPositions = (const uint8_t*)positionAttribute.pointer + srcVertexOffset * positionAttribute.stride;
		meshopt_remapVertexBuffer(tempPositions.data(), srcPositions, srcVertexCount, positionAttribute.stride, remap.data());

		// optimize cache + overdray
		meshopt_optimizeVertexCache(localIndices.data(), localIndices.data(), srcIndexCount, uniqueVertices);
		meshopt_optimizeOverdraw(localIndices.data(), localIndices.data(), srcIndexCount, (const float*)tempPositions.data(), uniqueVertices, positionAttribute.stride, 1.05f);

		// simplify
		size_t optimizedIndexCount = srcIndexCount;
		if ( 0.0f < simplify && simplify < 1.0f ) {
			uf::stl::vector<uint32_t> simplified(srcIndexCount);
			float targetError = 1e-2f / simplify;
			float realError = 0.0f;

			optimizedIndexCount = meshopt_simplify(
				simplified.data(), localIndices.data(), srcIndexCount,
				(const float*)tempPositions.data(), uniqueVertices, positionAttribute.stride,
				srcIndexCount * simplify, targetError, meshopt_SimplifyLockBorder, &realError
			);

			if ( verbose ) UF_MSG_DEBUG("[View {}] Simplified: {} -> {}", viewIdx, srcIndexCount, optimizedIndexCount);
			localIndices.swap(simplified);
			localIndices.resize(optimizedIndexCount);
		}

		// optimize for vertex fetch
		uf::stl::vector<uint32_t> fetchRemap(uniqueVertices);
		size_t finalVertices = meshopt_optimizeVertexFetchRemap(fetchRemap.data(), localIndices.data(), optimizedIndexCount, uniqueVertices);
		meshopt_remapIndexBuffer(localIndices.data(), localIndices.data(), optimizedIndexCount, fetchRemap.data());

		// store to output buffer
		uint32_t outVertexOffset = outVertices[0].size() / mesh.vertex.attributes[0].stride;
		uint32_t outIndexOffset = outIndices.size();

		for ( size_t i = 0; i < localIndices.size(); ++i ) outIndices.emplace_back( localIndices[i] );

		// remap buffers
		for ( size_t a = 0; a < mesh.vertex.attributes.size(); ++a ) {
			auto& attr = mesh.vertex.attributes[a];
			const uint8_t* basePtr = (const uint8_t*) attr.pointer + srcVertexOffset * attr.stride;

			// double remap: source -> unique -> final
			uf::stl::vector<uint8_t> tempBuf(uniqueVertices * attr.stride);
			meshopt_remapVertexBuffer(tempBuf.data(), basePtr, srcVertexCount, attr.stride, remap.data());

			uf::stl::vector<uint8_t> finalBuf(finalVertices * attr.stride);
			meshopt_remapVertexBuffer(finalBuf.data(), tempBuf.data(), uniqueVertices, attr.stride, fetchRemap.data());

			outVertices[a].insert(outVertices[a].end(), finalBuf.begin(), finalBuf.end());
		}

		// update indirect buffer
		if ( drawCommands ) {
			drawCommands[cmdIdx].indexID = outIndexOffset;
			drawCommands[cmdIdx].indices = optimizedIndexCount;
			drawCommands[cmdIdx].vertexID = outVertexOffset;
			drawCommands[cmdIdx].vertices = finalVertices;
		}
	}

	// apply index buffer (if missing)
	if ( mesh.index.attributes.empty() ) {
		mesh.bindIndex<uint32_t>();
		mesh.bind(mesh, mesh.isInterleaved());
	}

	// write indices to buffer
	mesh.index.count = outIndices.size();
	mesh.resizeIndices(mesh.index.count);
	uint8_t* dstIdx = mesh.getBuffer(mesh.index).data();
	for ( size_t i = 0; i < outIndices.size(); ++i ) writeIndex(dstIdx, i, mesh.index.size, outIndices[i]);

	// write vertices to buffer
	mesh.vertex.count = outVertices[0].size() / mesh.vertex.attributes[0].stride;
	for ( size_t a = 0; a < mesh.vertex.attributes.size(); ++a ) {
		auto& attr = mesh.vertex.attributes[a];
		mesh.buffers[attr.buffer].swap(outVertices[a]);
		attr.pointer = mesh.buffers[attr.buffer].data();
	}

	mesh.updateDescriptor();
	return true;
}

uf::stl::vector<float> ext::meshopt::computeLODs( size_t count, size_t maxLODs, size_t minIndices ) {
	uf::stl::vector<float> factors;
	factors.reserve(maxLODs);
	factors.emplace_back(1.0f);

	float factor = 1.0f;
	for (size_t i = 1; i < maxLODs; ++i) {
		factor *= 0.5f;
		if ( count * factor < minIndices ) break;
		factors.emplace_back(factor);
	}

	return factors;
}

uf::stl::vector<pod::LODMetadata> ext::meshopt::generateLODs( uf::Mesh& mesh, const uf::stl::vector<float>& lodFactors, bool verbose ) {
	uf::stl::vector<pod::LODMetadata> lodMetadata;

	if ( mesh.isInterleaved() ) {
		UF_MSG_ERROR("Cannot generate LODs on interleaved meshes.");
		return lodMetadata;
	}
	mesh.updateDescriptor();

	const auto& views = mesh.buffer_views;
	if ( views.empty() || lodFactors.empty() ) return lodMetadata;

	size_t numLODs = std::min(lodFactors.size(), (size_t)4);
	lodMetadata.resize(mesh.indirect.count);

	pod::DrawCommand* drawCommands = mesh.indirect.count > 0 ? (pod::DrawCommand*) mesh.getBuffer(mesh.indirect).data() : nullptr;
	if ( !drawCommands ) return lodMetadata;

	// store LOD0 as-is
	uf::stl::vector<uint32_t> outIndices(mesh.index.count);
	const uint8_t* srcIndexData = mesh.getBuffer(mesh.index).data();
	for ( size_t i = 0; i < mesh.index.count; ++i ) {
		outIndices[i] = readIndex(srcIndexData, i, mesh.index.size);
	}
	// write LOD0 data
	for ( size_t viewIdx = 0; viewIdx < views.size(); ++viewIdx ) {
		uint32_t cmdIdx = views[viewIdx].indirectIndex;
		auto& cmd = drawCommands[cmdIdx];
		lodMetadata[cmdIdx].levels[0].indexID  = cmd.indexID;
		lodMetadata[cmdIdx].levels[0].indices  = cmd.indices;
		lodMetadata[cmdIdx].levels[0].vertexID = cmd.vertexID;
		lodMetadata[cmdIdx].levels[0].vertices = cmd.vertices;
	}

	// copy position attribute
	int posAttrIdx = -1;
	uf::stl::vector<uf::stl::vector<uint8_t>> outVertices(mesh.vertex.attributes.size());
	for ( size_t a = 0; a < mesh.vertex.attributes.size(); ++a ) {
		auto& attr = mesh.vertex.attributes[a];
		if ( attr.descriptor.name == "position" ) posAttrIdx = a;

		auto& buf = mesh.buffers[attr.buffer];
		outVertices[a].assign(buf.begin(), buf.end());
	}


	// generate LOD1=>N
	for ( size_t lodIdx = 1; lodIdx < numLODs; ++lodIdx ) {
		float simplify = lodFactors[lodIdx];

		for ( size_t viewIdx = 0; viewIdx < views.size(); ++viewIdx ) {
			uint32_t cmdIdx = views[viewIdx].indirectIndex;

			// source from LOD0
			auto& cmd0 = lodMetadata[cmdIdx].levels[0];
			size_t previousIndicesCount = lodMetadata[cmdIdx].levels[lodIdx - 1].indices;

			uf::stl::vector<uint32_t> baseIndices(cmd0.indices);
			for ( size_t i = 0; i < cmd0.indices; ++i ) baseIndices[i] = outIndices[cmd0.indexID + i];

			// generate LOD
			if ( 0.0f < simplify && simplify < 1.0f ) {
				float targetError = 1e-2f / simplify;
				float realError = 0.0f;
				size_t currentIndicesCount = cmd0.indices;
				uf::stl::vector<uint32_t> lodIndices = baseIndices;

				const float* basePositions = (const float*) (outVertices[posAttrIdx].data() + cmd0.vertexID * mesh.vertex.attributes[posAttrIdx].stride);

				currentIndicesCount = meshopt_simplify(
					lodIndices.data(), baseIndices.data(), cmd0.indices,
					basePositions, cmd0.vertices, mesh.vertex.attributes[posAttrIdx].stride,
					cmd0.indices * simplify, targetError, meshopt_SimplifyLockBorder, &realError
				);

				// couldn't simplify further, use previous LOD
				if ( currentIndicesCount == previousIndicesCount ) {
					lodMetadata[cmdIdx].levels[lodIdx] = lodMetadata[cmdIdx].levels[lodIdx - 1];
					continue;
				}

				if ( verbose ) UF_MSG_DEBUG("[View {}] LOD {}: {} -> {}", viewIdx, lodIdx, cmd0.indices, currentIndicesCount);

				lodIndices.resize(currentIndicesCount);

				// optimize and pack vertices for this specific LOD
				uf::stl::vector<uint32_t> fetchRemap(cmd0.vertices);
				size_t uniqueVertices = meshopt_optimizeVertexFetchRemap(fetchRemap.data(), lodIndices.data(), currentIndicesCount, cmd0.vertices);
				meshopt_remapIndexBuffer(lodIndices.data(), lodIndices.data(), currentIndicesCount, fetchRemap.data());

				// record the new offsets appended at the end of the global buffers
				uint32_t lodVertexOffset = outVertices[0].size() / mesh.vertex.attributes[0].stride;
				uint32_t lodIndexOffset = outIndices.size();

				lodMetadata[cmdIdx].levels[lodIdx].indexID  = lodIndexOffset;
				lodMetadata[cmdIdx].levels[lodIdx].indices  = currentIndicesCount;
				lodMetadata[cmdIdx].levels[lodIdx].vertexID = lodVertexOffset;
				lodMetadata[cmdIdx].levels[lodIdx].vertices = uniqueVertices;

				// append indices
				for ( size_t i = 0; i < currentIndicesCount; ++i ) outIndices.emplace_back(lodIndices[i]);
				// append vertices
				for ( size_t a = 0; a < mesh.vertex.attributes.size(); ++a ) {
					auto& attr = mesh.vertex.attributes[a];
					const uint8_t* srcPtr = outVertices[a].data() + cmd0.vertexID * attr.stride;

					uf::stl::vector<uint8_t> packed(uniqueVertices * attr.stride);
					meshopt_remapVertexBuffer(packed.data(), srcPtr, cmd0.vertices, attr.stride, fetchRemap.data());
					outVertices[a].insert(outVertices[a].end(), packed.begin(), packed.end());
				}
			} else {
				// no simplification, just use LOD0 (shouldn't happen)
				lodMetadata[cmdIdx].levels[lodIdx] = lodMetadata[cmdIdx].levels[0];
			}
		}
	}

	// write indices to mesh
	mesh.index.count = outIndices.size();
	mesh.resizeIndices(mesh.index.count);
	uint8_t* dstIdx = mesh.getBuffer(mesh.index).data();
	for ( size_t i = 0; i < outIndices.size(); ++i ) writeIndex(dstIdx, i, mesh.index.size, outIndices[i]);

	// write vertices to mesh
	mesh.vertex.count = outVertices[0].size() / mesh.vertex.attributes[0].stride;
	for ( size_t a = 0; a < mesh.vertex.attributes.size(); ++a ) {
		auto& attr = mesh.vertex.attributes[a];
		mesh.buffers[attr.buffer].swap(outVertices[a]);
		attr.pointer = mesh.buffers[attr.buffer].data();
	}

	mesh.updateDescriptor();
	return lodMetadata;
}
#endif
