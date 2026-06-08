#include <uf/ext/xatlas/xatlas.h>
#if UF_USE_XATLAS
#include <xatlas/xatlas.h>

#define UF_XATLAS_UNWRAP_MULTITHREAD 1

size_t ext::xatlas::unwrap( pod::Graph& graph ) {
	struct Entry {
		size_t index = 0;
		size_t commandID = 0;
		::xatlas::MeshDecl decl;
	};
	struct Atlas {
		::xatlas::Atlas* pointer = NULL;
		uf::stl::vector<Entry> entries;
		size_t vertexOffset = 0;
	};

	uf::stl::vector<uf::Mesh> sources(graph.meshes.size());

	uf::stl::unordered_map<size_t, Atlas> atlases;
	atlases.reserve(graph.meshes.size());

	uf::stl::unordered_map<size_t, size_t> sizesVertex;
	uf::stl::unordered_map<size_t, size_t> sizesIndex;

	auto& storage = uf::graph::getStorage( graph );

	// copy source meshes
	// create mesh decls for passing to xatlas
	for ( auto index = 0; index < graph.meshes.size(); ++index ) {
		auto& name = graph.meshes[index];
		auto& mesh = /*graph.storage*/storage.meshes[name];
		auto& source = sources[index];

		bool should = false;
		if ( graph.metadata["exporter"]["unwrap"].is<bool>() && graph.metadata["exporter"]["unwrap"].as<bool>() ) {
			should = true;
		} else {
			ext::json::forEach( graph.metadata["tags"], [&]( const uf::stl::string& key, ext::json::Value& value ) {
				if ( uf::string::isRegex( key ) ) {
					if ( !uf::string::matched( name, key ) ) return;
				} else if ( name != key ) return;

				if ( ext::json::isNull( value["unwrap mesh"] ) ) return;
				if ( !value["unwrap mesh"].as<bool>(false) ) return;

				should = true;
			});
		}
		if ( !should ) continue;

		source = mesh;
		source.updateDescriptor();

		for ( size_t viewIdx = 0; viewIdx < source.buffer_views.size(); ++viewIdx ) {
			const auto& view = source.buffer_views[viewIdx];

			size_t atlasID = 0;
			if ( view.indirectIndex != -1 ) {
				pod::DrawCommand* drawCommands = (pod::DrawCommand*) mesh.getBuffer(mesh.indirect).data();
				atlasID = drawCommands[view.indirectIndex].auxID;
			}

			auto& atlas = atlases[atlasID];
			auto& entry = atlas.entries.emplace_back();
			entry.index = index;
			entry.commandID = viewIdx;

			auto& decl = entry.decl;
			auto posView = view["position"_hash];
			auto uvView  = view["uv"_hash];
			auto idxView = view["index"_hash];

			UF_ASSERT( posView.valid() && uvView.valid() );

			decl.vertexCount = view.vertex.count;
			decl.vertexPositionData = posView.data(view.vertex.first);
			decl.vertexPositionStride = posView.stride();
			decl.vertexUvData = uvView.data(view.vertex.first);
			decl.vertexUvStride = uvView.stride();

			if ( idxView.valid() ) {
				decl.indexCount = view.index.count;
				decl.indexData = idxView.data(view.index.first);

				switch ( idxView.attribute.descriptor.size ) {
					case 1: UF_EXCEPTION("xatlas does not support 8-bit indices"); break;
					case 2: decl.indexFormat = ::xatlas::IndexFormat::UInt16; break;
					case 4: decl.indexFormat = ::xatlas::IndexFormat::UInt32; break;
					default: UF_EXCEPTION("unsupported index type"); break;
				}
			} else {
				decl.indexCount = 0;
			}
		}
	}

	::xatlas::ChartOptions chartOptions{};
	chartOptions.useInputMeshUvs = graph.metadata["baking"]["settings"]["useInputMeshUvs"].as(chartOptions.useInputMeshUvs);
	chartOptions.maxIterations = graph.metadata["baking"]["settings"]["maxIterations"].as(chartOptions.maxIterations);

	::xatlas::PackOptions packOptions{};
	packOptions.maxChartSize = graph.metadata["baking"]["settings"]["maxChartSize"].as(packOptions.maxChartSize);
	packOptions.padding = graph.metadata["baking"]["settings"]["padding"].as(packOptions.padding);
	packOptions.texelsPerUnit = graph.metadata["baking"]["settings"]["texelsPerUnit"].as(packOptions.texelsPerUnit);
	packOptions.bilinear = graph.metadata["baking"]["settings"]["bilinear"].as(packOptions.bilinear);
	packOptions.blockAlign = graph.metadata["baking"]["settings"]["blockAlign"].as(packOptions.blockAlign);
	packOptions.bruteForce = graph.metadata["baking"]["settings"]["bruteForce"].as(packOptions.bruteForce);
	packOptions.createImage = graph.metadata["baking"]["settings"]["createImage"].as(packOptions.createImage);
	packOptions.rotateChartsToAxis = graph.metadata["baking"]["settings"]["rotateChartsToAxis"].as(packOptions.rotateChartsToAxis);
	packOptions.rotateCharts = graph.metadata["baking"]["settings"]["rotateCharts"].as(packOptions.rotateCharts);
	packOptions.resolution = graph.metadata["baking"]["resolution"].as(packOptions.resolution);

	// add mesh decls to mesh atlases
	// done after the fact since we'll know the total amount of meshes added
	for ( auto& pair : atlases ) {
		auto& atlas = pair.second;
		if ( !atlas.pointer ) atlas.pointer = ::xatlas::Create();

		for ( auto& entry : atlas.entries ) {
			::xatlas::AddMeshError error = ::xatlas::AddMesh(atlas.pointer, entry.decl, atlas.entries.size());
			if (error != ::xatlas::AddMeshError::Success) {
				::xatlas::Destroy(atlas.pointer);
				UF_EXCEPTION("{}", ::xatlas::StringForEnum(error));
			}
		}
	}

	// pack
#if UF_XATLAS_UNWRAP_MULTITHREAD
	auto tasks = uf::thread::schedule(true);
#else
	auto tasks = uf::thread::schedule(false);
#endif
	for ( auto& pair : atlases ) {
		tasks.queue([&]{
			auto& atlas = pair.second;
			::xatlas::Generate(atlas.pointer, chartOptions, packOptions);
		});
	}
	uf::thread::execute( tasks );

	for ( auto& pair : atlases ) {
		auto& atlas = pair.second;
		// get vertices size ahead of time
		for ( auto i = 0; i < atlas.pointer->meshCount; ++i ) {
			auto& xmesh = atlas.pointer->meshes[i];
			auto& entry = atlas.entries[i];

			if ( sizesVertex.count(entry.index) == 0 ) sizesVertex[entry.index] = 0;
			if ( sizesIndex.count(entry.index) == 0 ) sizesIndex[entry.index] = 0;

			sizesVertex[entry.index] += xmesh.vertexCount;
			sizesIndex[entry.index] += xmesh.indexCount;
		}
	}

	// resize vertices
	for ( auto i = 0; i < graph.meshes.size(); ++i ) {
		auto& name = graph.meshes[i];
		auto& mesh = /*graph.storage*/storage.meshes[name];
		auto& source = sources[i];
		if ( source.vertex.count == 0 ) continue;

		if ( sizesVertex[i] != mesh.vertex.count ) {
			mesh.resizeVertices( sizesVertex[i] );
		}
		if ( sizesIndex[i] != mesh.index.count ) {
			mesh.resizeIndices( sizesIndex[i] );
		}
		mesh.updateDescriptor();
	}

	// update vertices count
	for ( auto& pair : atlases ) {
		auto& atlas = pair.second;
		for ( auto i = 0; i < atlas.pointer->meshCount; i++ ) {
			auto& xmesh = atlas.pointer->meshes[i];
			auto& entry = atlas.entries[i];
			auto& name = graph.meshes[entry.index];
			auto& mesh = /*graph.storage*/storage.meshes[name];
			auto& source = sources[entry.index];

			if ( source.vertex.count == 0 ) continue;

			source.updateDescriptor();

			// draw commands
			if ( !mesh.indirect.count ) continue;

			auto& primitives = /*graph.storage*/storage.primitives[name];
			pod::DrawCommand* drawCommands = (pod::DrawCommand*) mesh.getBuffer(mesh.indirect).data();
			
			auto& primitive = primitives[entry.commandID];
			auto& drawCommand = drawCommands[entry.commandID];

			drawCommand.vertices = xmesh.vertexCount;
			drawCommand.indices = xmesh.indexCount;
		}
	}
	
	// update vertexID offsets for indirect commands
	for ( auto index = 0; index < graph.meshes.size(); ++index ) {
		auto& name = graph.meshes[index];
		auto& mesh = /*graph.storage*/storage.meshes[name];
		auto& source = sources[index];

		if ( source.vertex.count == 0 ) continue;
		if ( !mesh.indirect.count ) continue;

		auto& primitives = /*graph.storage*/storage.primitives[name];
		pod::DrawCommand* drawCommands = (pod::DrawCommand*) mesh.getBuffer(mesh.indirect).data();
	
		size_t vertexID = 0;
		size_t indexID = 0;

		for ( auto i = 0; i < mesh.indirect.count; ++i ) {
			auto& primitive = primitives[i];
			auto& drawCommand = drawCommands[i];

			drawCommand.vertexID = vertexID;
			drawCommand.indexID = indexID;
			primitive.drawCommand = drawCommand;

			vertexID += drawCommand.vertices;
			indexID += drawCommand.indices;
		}
	}

	// update vertices
	for ( auto& pair : atlases ) {
		auto& atlas = pair.second;

		for ( auto i = 0; i < atlas.pointer->meshCount; i++ ) {
			auto& xmesh = atlas.pointer->meshes[i];
			auto& entry = atlas.entries[i];

			auto& name = graph.meshes[entry.index];
			auto& mesh = storage.meshes[name];
			auto& source = sources[entry.index];

			if ( source.vertex.count == 0 ) continue;

			const auto& srcView = source.buffer_views[entry.commandID];
			const auto& dstView = mesh.buffer_views[entry.commandID];

			size_t dstVertexFirst = 0;
			size_t dstIndexFirst = 0;
			if ( mesh.indirect.count > 0 ) {
				pod::DrawCommand* drawCommands = (pod::DrawCommand*) mesh.getBuffer(mesh.indirect).data();
				dstVertexFirst = drawCommands[entry.commandID].vertexID;
				dstIndexFirst  = drawCommands[entry.commandID].indexID;
			}

			auto stView = dstView["st"];
			int stAttributeIndex = -1;
			for ( auto attrIdx = 0; attrIdx < mesh.vertex.attributes.size(); ++attrIdx ) {
				if ( mesh.vertex.attributes[attrIdx].descriptor.name == "st" ) {
					stAttributeIndex = attrIdx;
					break;
				}
			}

			for ( auto j = 0; j < xmesh.vertexCount; ++j ) {
				auto& vertex = xmesh.vertexArray[j];
				uint32_t ref = vertex.xref; // original vertex index

				for ( auto attrIdx = 0; attrIdx < mesh.vertex.attributes.size(); ++attrIdx ) {
					auto srcAttribute = srcView.vertex.attributes[attrIdx];
					auto dstAttribute = mesh.vertex.attributes[attrIdx];

					if ( attrIdx == stAttributeIndex ) {
						auto& st = uf::mesh::getVertexAttribute<pod::Vector2f>( dstView, stView, dstVertexFirst + j );
						st.x = vertex.uv[0] / atlas.pointer->width;
						st.y = vertex.uv[1] / atlas.pointer->height;
					} else {
						uint8_t* dstPtr = static_cast<uint8_t*>(dstAttribute.pointer) + dstAttribute.stride * (dstVertexFirst + j);
						const uint8_t* srcPtr = static_cast<const uint8_t*>(srcAttribute.pointer) + srcAttribute.stride * (srcView.vertex.first + ref);
						std::memcpy(dstPtr, srcPtr, srcAttribute.descriptor.size);
					}
				}
			}

			if ( mesh.index.count ) {
				uf::Mesh::Attribute indexAttribute = mesh.index.attributes.front();
				uint8_t* dstIndexPtr = static_cast<uint8_t*>(indexAttribute.pointer) + indexAttribute.stride * dstIndexFirst;

				for ( auto idx = 0; idx < xmesh.indexCount; ++idx ) {
					uf::mesh::setIndex(dstIndexPtr, mesh.index.size, idx, xmesh.indexArray[idx]);
				}
			}
			mesh.updateDescriptor();
		}
	}

	// cleanup
	size_t atlasCount = 0;
	for ( auto& pair : atlases ) {
		auto& atlas = pair.second;
		::xatlas::Destroy(atlas.pointer);
		++atlasCount;
	}
	return atlasCount;
}
#endif