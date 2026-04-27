#include <uf/ext/meshopt/meshopt.h>
#if UF_USE_MESHOPT
#include <meshoptimizer.h>

bool ext::meshopt::optimize( uf::Mesh& mesh, float simplify, size_t o, bool verbose ) {
	if ( mesh.isInterleaved() ) {
		UF_MSG_ERROR("Optimization of interleaved meshes is currently not supported. Consider optimizing on meshlets.");
		return false;
	}
	mesh.updateDescriptor();

	const auto& views = mesh.buffer_views;
	if ( views.empty() ) {
		UF_MSG_ERROR("No buffer views found. Cannot optimize per-submesh.");
		return false;
	}

	uf::stl::vector<uint32_t> optIndices;
	pod::DrawCommand* drawCommands = mesh.indirect.count > 0 ? (pod::DrawCommand*) mesh.getBuffer(mesh.indirect).data() : nullptr;

	for ( size_t viewIdx = 0; viewIdx < views.size(); ++viewIdx ) {
		const auto& view = views[viewIdx];
		auto& indicesView = view["index"];
		auto& positionsView = view["position"];

		if ( !indicesView.valid() || !positionsView.valid() ) continue;

		size_t indicesCount = view.index.count;
		size_t vertexCount = view.vertex.count;

		uf::stl::vector<uint32_t> submeshIndices(indicesCount);
		for ( size_t i = 0; i < indicesCount; ++i ) {
			size_t global_i = view.index.first + i;
			switch ( indicesView.attribute.descriptor.size ) {
				case 1: submeshIndices[i] = indicesView.get<uint8_t>(global_i)[0]; break;
				case 2: submeshIndices[i] = indicesView.get<uint16_t>(global_i)[0]; break;
				case 4: submeshIndices[i] = indicesView.get<uint32_t>(global_i)[0]; break;
			}
		}

		meshopt_optimizeVertexCache(&submeshIndices[0], &submeshIndices[0], indicesCount, mesh.vertex.count);

		meshopt_optimizeOverdraw(
			&submeshIndices[0],
			&submeshIndices[0],
			indicesCount,
			(const float*) positionsView.data(),
			mesh.vertex.count,
			positionsView.stride(),
			1.05f
		);

		if ( 0.0f < simplify && simplify < 1.0f ) {
			uf::stl::vector<uint32_t> indicesSimplified(indicesCount);

			float targetError = 0.1; // 1e-2f / simplify;
			float realError = 0.0f;

			size_t realIndices = meshopt_simplify(
				&indicesSimplified[0],
				&submeshIndices[0],
				indicesCount,
				(const float*) positionsView.data(),
				mesh.vertex.count,
				positionsView.stride(),
				indicesCount * simplify,
				targetError
				//,0, &realError
			);

			if ( verbose ) {
				UF_MSG_DEBUG("[View {} Simplified] indices: {} -> {} | error: {} -> {}", viewIdx, indicesCount, realIndices, targetError, realError);
			}

			indicesCount = realIndices;
			submeshIndices.swap(indicesSimplified);
			submeshIndices.resize(indicesCount);
		}

		size_t newIndexStart = optIndices.size();
		optIndices.insert(optIndices.end(), submeshIndices.begin(), submeshIndices.end());

		if ( drawCommands ) {
			drawCommands[view.indirectIndex].indexID = newIndexStart;
			drawCommands[view.indirectIndex].indices = indicesCount;
		}
	}

	mesh.index.count = optIndices.size();
	mesh.resizeIndices( mesh.index.count );

	uint8_t* dstPointer = (uint8_t*) mesh.getBuffer(mesh.index).data();
	for ( size_t i = 0; i < optIndices.size(); ++i ) {
		switch ( mesh.index.size ) {
			case 1: (( uint8_t*) dstPointer)[i] = (uint8_t)  optIndices[i]; break;
			case 2: ((uint16_t*) dstPointer)[i] = (uint16_t) optIndices[i]; break;
			case 4: ((uint32_t*) dstPointer)[i] = (uint32_t) optIndices[i]; break;
		}
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
	if ( views.empty() ) return lodMetadata;

	size_t numLODs = std::min(lodFactors.size(), (size_t)4);
	lodMetadata.resize(mesh.indirect.count);

	uf::stl::vector<uf::stl::vector<uint32_t>> lodBlocks(numLODs);
	pod::DrawCommand* drawCommands = mesh.indirect.count > 0 ? (pod::DrawCommand*) mesh.getBuffer(mesh.indirect).data() : nullptr;

	for ( size_t viewIdx = 0; viewIdx < views.size(); ++viewIdx ) {
		const auto& view = views[viewIdx];
		uint32_t cmdIdx = view.indirectIndex;

		auto& indicesView = view["index"];
		auto& positionsView = view["position"];

		size_t baseIndicesCount = view.index.count;
		uf::stl::vector<uint32_t> baseIndices(baseIndicesCount);

		for ( size_t i = 0; i < baseIndicesCount; ++i ) {
			size_t global_i = view.index.first + i;
			switch ( indicesView.attribute.descriptor.size ) {
				case 1: baseIndices[i] = indicesView.get<uint8_t>(global_i)[0]; break;
				case 2: baseIndices[i] = indicesView.get<uint16_t>(global_i)[0]; break;
				case 4: baseIndices[i] = indicesView.get<uint32_t>(global_i)[0]; break;
			}
		}

		meshopt_optimizeVertexCache(&baseIndices[0], &baseIndices[0], baseIndicesCount, mesh.vertex.count);

		for ( size_t lodIdx = 0; lodIdx < numLODs; ++lodIdx ) {
			float simplify = lodFactors[lodIdx];
			uf::stl::vector<uint32_t> lodIndices = baseIndices;
			size_t currentIndicesCount = baseIndicesCount;

			if ( simplify < 1.0f ) {
				float targetError = 0.1; // 1e-2f / simplify;
				float realError = 0.0f;
				currentIndicesCount = meshopt_simplify(
					&lodIndices[0], &baseIndices[0], baseIndicesCount,
					(const float*)positionsView.data(0), mesh.vertex.count, positionsView.stride(),
					baseIndicesCount * simplify, targetError
					//, 0, &realError
				);

				if ( baseIndicesCount == currentIndicesCount ) {
					continue;
				}

				if ( verbose ) {
					UF_MSG_DEBUG("[View {} Simplified LOD {}] indices: {} -> {} | error: {} -> {}", viewIdx, lodIdx, baseIndicesCount, currentIndicesCount, targetError, realError);
				}


				lodIndices.resize(currentIndicesCount);
			}

			lodMetadata[cmdIdx].levels[lodIdx].indexID = lodBlocks[lodIdx].size();
			lodMetadata[cmdIdx].levels[lodIdx].indices = currentIndicesCount;

			lodBlocks[lodIdx].insert(lodBlocks[lodIdx].end(), lodIndices.begin(), lodIndices.end());
		}
	}

	uf::stl::vector<uint32_t> unifiedIndices;
	size_t currentGlobalOffset = 0;

	for ( size_t lodIdx = 0; lodIdx < numLODs; ++lodIdx ) {
		for ( size_t viewIdx = 0; viewIdx < views.size(); ++viewIdx ) {
			uint32_t cmdIdx = views[viewIdx].indirectIndex;
			lodMetadata[cmdIdx].levels[lodIdx].indexID += currentGlobalOffset;

			if ( lodIdx == 0 && drawCommands ) {
				drawCommands[cmdIdx].indexID = lodMetadata[cmdIdx].levels[0].indexID;
				drawCommands[cmdIdx].indices = lodMetadata[cmdIdx].levels[0].indices;
			}
		}

		unifiedIndices.insert(unifiedIndices.end(), lodBlocks[lodIdx].begin(), lodBlocks[lodIdx].end());
		currentGlobalOffset = unifiedIndices.size();
	}

	mesh.index.count = unifiedIndices.size();
	mesh.resizeIndices( mesh.index.count );
	uint8_t* dstPointer = (uint8_t*) mesh.getBuffer(mesh.index).data();
	for ( size_t i = 0; i < unifiedIndices.size(); ++i ) {
		switch ( mesh.index.size ) {
			case 1: (( uint8_t*) dstPointer)[i] = (uint8_t)  unifiedIndices[i]; break;
			case 2: ((uint16_t*) dstPointer)[i] = (uint16_t) unifiedIndices[i]; break;
			case 4: ((uint32_t*) dstPointer)[i] = (uint32_t) unifiedIndices[i]; break;
		}
	}

	mesh.updateDescriptor();
	
	return lodMetadata;
}

#endif