#include <uf/ext/xatlas/xatlas.h>
#if UF_USE_XATLAS
#include <xatlas/xatlas.h>

#define UF_XATLAS_UNWRAP_MULTITHREAD 1
#define UF_XATLAS_LAZY 1 // i do not understand why it needs to insert extra vertices for it to not even be used in the indices buffer, this flag avoids having to account for it

size_t UF_API ext::xatlas::unwrap( pod::Graph& graph, bool combined ) {
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

	uf::stl::vector<uf::Mesh> sources;
	sources.reserve(graph.meshes.size());

	uf::stl::unordered_map<size_t, Atlas> atlases;
	atlases.reserve(graph.meshes.size());

	uf::stl::vector<size_t> sizes( graph.meshes.size(), 0 );

	// copy source meshes
	// create mesh decls for passing to xatlas
	for ( auto index = 0; index < graph.meshes.size(); ++index ) {
		auto& name = graph.meshes[index];
		auto& mesh = /*graph.storage*/uf::graph::storage.meshes[name];

		if ( mesh.isInterleaved() ) {
			UF_EXCEPTION("unwrapping interleaved mesh is not supported");
		}
		sources.emplace_back(mesh).updateDescriptor();

		uf::Mesh::Input vertexInput = mesh.vertex;

		uf::Mesh::Attribute positionAttribute;
		uf::Mesh::Attribute uvAttribute;
		uf::Mesh::Attribute stAttribute;

		for ( auto& attribute : mesh.vertex.attributes ) {
			if ( attribute.descriptor.name == "position" ) positionAttribute = attribute;
			else if ( attribute.descriptor.name == "uv" ) uvAttribute = attribute;
			else if ( attribute.descriptor.name == "st" ) stAttribute = attribute;
		}
		UF_ASSERT( positionAttribute.descriptor.name == "position" && uvAttribute.descriptor.name == "uv" && stAttribute.descriptor.name == "st" );

		if ( mesh.index.count ) {
			uf::Mesh::Input indexInput = mesh.index;
			uf::Mesh::Attribute indexAttribute = mesh.index.attributes.front();

			::xatlas::IndexFormat indexType = ::xatlas::IndexFormat::UInt32;
			switch ( mesh.index.size ) {
				case sizeof(uint16_t): indexType = ::xatlas::IndexFormat::UInt16; break;
				case sizeof(uint32_t): indexType = ::xatlas::IndexFormat::UInt32; break;
				default: UF_EXCEPTION("unsupported index type"); break;
			}

			if ( mesh.indirect.count ) {
				auto& primitives = /*graph.storage*/uf::graph::storage.primitives[name];
				pod::DrawCommand* drawCommands = (pod::DrawCommand*) mesh.getBuffer(mesh.indirect).data();

				for ( auto i = 0; i < mesh.indirect.count; ++i ) {
					size_t atlasID = combined ? 0 : drawCommands[i].auxID;

					vertexInput = mesh.remapVertexInput( i );
					indexInput = mesh.remapIndexInput( i );

					auto& atlas = atlases[atlasID];
					auto& entry = atlas.entries.emplace_back();
					entry.index = index;
					entry.commandID = i;

					auto& decl = entry.decl;
					
					decl.vertexPositionData = static_cast<uint8_t*>(positionAttribute.pointer) + positionAttribute.stride * vertexInput.first;
					decl.vertexPositionStride = positionAttribute.stride;

					decl.vertexUvData = static_cast<uint8_t*>(uvAttribute.pointer) + uvAttribute.stride * vertexInput.first;
					decl.vertexUvStride = uvAttribute.stride;

					decl.vertexCount = vertexInput.count;

					decl.indexCount = indexInput.count;
					decl.indexData = static_cast<uint8_t*>(indexAttribute.pointer) + indexAttribute.stride * indexInput.first;
					decl.indexFormat = indexType;
				}
			} else {
				size_t atlasID = 0;
				auto& atlas = atlases[atlasID];
				auto& entry = atlas.entries.emplace_back();
				entry.index = index;

				auto& decl = entry.decl;
				decl.vertexPositionData = static_cast<uint8_t*>(positionAttribute.pointer) + positionAttribute.stride * vertexInput.first;
				decl.vertexPositionStride = positionAttribute.stride;

				decl.vertexUvData = static_cast<uint8_t*>(uvAttribute.pointer) + uvAttribute.stride * vertexInput.first;
				decl.vertexUvStride = uvAttribute.stride;

				decl.vertexCount = vertexInput.count;

				decl.indexCount = indexInput.count;
				decl.indexData = static_cast<uint8_t*>(indexAttribute.pointer) + indexAttribute.stride * indexInput.first;
				decl.indexFormat = indexType;
			}
		} else UF_EXCEPTION("to-do: not require indices for meshes");
	}

	// add mesh decls to mesh atlases
	// done after the fact since we'll know the total amount of meshes added
	for ( auto& pair : atlases ) {
		auto& atlas = pair.second;
		if ( !atlas.pointer ) atlas.pointer = ::xatlas::Create();

		for ( auto& entry : atlas.entries ) {
			::xatlas::AddMeshError error = ::xatlas::AddMesh(atlas.pointer, entry.decl, atlas.entries.size());
			if (error != ::xatlas::AddMeshError::Success) {
				::xatlas::Destroy(atlas.pointer);
				UF_EXCEPTION(::xatlas::StringForEnum(error));
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

	// pack
	pod::Thread::container_t jobs;

	for ( auto& pair : atlases ) {
		jobs.emplace_back([&]{
			auto& atlas = pair.second;
			::xatlas::Generate(atlas.pointer, chartOptions, packOptions);
		/*
			// get vertices size ahead of time
			for ( auto i = 0; i < atlas.pointer->meshCount; ++i ) {
				auto& xmesh = atlas.pointer->meshes[i];
				auto& entry = atlas.entries[i];
			//	atlas.vertices += xmesh.vertexCount;
				sizes[entry.index] += xmesh.vertexCount;
			}
		*/
		});
	}

#if UF_XATLAS_UNWRAP_MULTITHREAD
	if ( !jobs.empty() ) uf::thread::batchWorkers_Async( jobs );
#else
	for ( auto& job : jobs ) job();
#endif

#if !UF_XATLAS_LAZY
	for ( auto& pair : atlases ) {
		auto& atlas = pair.second;
		// get vertices size ahead of time
		for ( auto i = 0; i < atlas.pointer->meshCount; ++i ) {
			auto& xmesh = atlas.pointer->meshes[i];
			auto& entry = atlas.entries[i];
			sizes[entry.index] += xmesh.vertexCount;
		}
	}

	// resize vertices
	for ( auto i = 0; i < graph.meshes.size(); ++i ) {
		auto& name = graph.meshes[i];
		auto& mesh = /*graph.storage*/uf::graph::storage.meshes[name];
		if ( sizes[i] != mesh.vertex.count ) {
			mesh.resizeVertices( sizes[i] );
			mesh.updateDescriptor();
		}
	}
#endif

	// update vertices
	for ( auto& pair : atlases ) {
		size_t vertexIDOffset = 0;
		auto& atlas = pair.second;
		for ( auto i = 0; i < atlas.pointer->meshCount; i++ ) {
			auto& xmesh = atlas.pointer->meshes[i];
			auto& entry = atlas.entries[i];
			auto& name = graph.meshes[entry.index];
			auto& mesh = /*graph.storage*/uf::graph::storage.meshes[name];
			auto& source = sources[entry.index];


			// draw commands
			if ( mesh.indirect.count ) {
				auto srcInput = source.remapVertexInput( entry.commandID );
				auto dstInput = mesh.remapVertexInput( entry.commandID );

			#if UF_XATLAS_LAZY
				for ( auto j = 0; j < xmesh.vertexCount; ++j ) {
					auto& vertex = xmesh.vertexArray[j];
					auto ref = vertex.xref;
					
					for ( auto k = 0; k < srcInput.attributes.size(); ++k ) {
						auto dstAttribute = dstInput.attributes[k];
						if ( dstAttribute.descriptor.name != "st" ) continue;
						pod::Vector2f& st = *(pod::Vector2f*) ( static_cast<uint8_t*>(dstAttribute.pointer) + dstAttribute.stride * (ref + dstInput.first) );
						st = pod::Vector2f{ vertex.uv[0] / atlas.pointer->width, vertex.uv[1] / atlas.pointer->height };;
					}
				}
			#else
				auto& primitives = /*graph.storage*/uf::graph::storage.primitives[name];
				pod::DrawCommand* drawCommands = (pod::DrawCommand*) mesh.getBuffer(mesh.indirect).data();

				bool mismatched = xmesh.vertexCount != drawCommands[entry.commandID].vertices;
				vertexIDOffset += xmesh.vertexCount - drawCommands[entry.commandID].vertices;

				drawCommands[entry.commandID].vertices = xmesh.vertexCount;
				primitives[entry.commandID].drawCommand.vertices = xmesh.vertexCount;
				drawCommands[entry.commandID].vertexID += vertexIDOffset;
				primitives[entry.commandID].drawCommand.vertexID += vertexIDOffset;

				for ( auto j = 0; j < xmesh.vertexCount; ++j ) {
					auto& vertex = xmesh.vertexArray[j];
					auto ref = vertex.xref;
					
					for ( auto k = 0; k < srcInput.attributes.size(); ++k ) {
						auto srcAttribute = srcInput.attributes[k];
						auto dstAttribute = dstInput.attributes[k];

						if ( dstAttribute.descriptor.name == "st" ) {
							pod::Vector2f& st = *(pod::Vector2f*) ( static_cast<uint8_t*>(dstAttribute.pointer) + dstAttribute.stride * (j + dstInput.first) );
							st = pod::Vector2f{ vertex.uv[0] / atlas.pointer->width, vertex.uv[1] / atlas.pointer->height };;
						} else {
							memcpy(  static_cast<uint8_t*>(dstAttribute.pointer) + dstAttribute.stride * (j + dstInput.first),  static_cast<uint8_t*>(srcAttribute.pointer) + srcAttribute.stride * (ref + srcInput.first), srcAttribute.stride );
						}
					}
				}
				// indices
				if ( mesh.index.count ) {
					uf::Mesh::Input indexInput = mesh.remapIndexInput( entry.commandID );
					uf::Mesh::Attribute indexAttribute = mesh.index.attributes.front();
				//	uf::Mesh::Attribute indexAttribute = mesh.remapIndexAttribute( mesh.index.attributes.front(), entry.commandID );
					uint8_t* pointer = (uint8_t*) static_cast<uint8_t*>(indexAttribute.pointer) + indexAttribute.stride * indexInput.first;
					for ( auto index = 0; index < xmesh.indexCount; ++index ) {
						switch ( mesh.index.size ) {
							case 1: (( uint8_t*) pointer)[index] = xmesh.indexArray[index]; break;
							case 2: ((uint16_t*) pointer)[index] = xmesh.indexArray[index]; break;
							case 4: ((uint32_t*) pointer)[index] = xmesh.indexArray[index]; break;
						}
					}
				}
			#endif
			} else {
				uf::Mesh::Attribute stAttribute;
				for ( auto& attribute : mesh.vertex.attributes ) if ( attribute.descriptor.name == "st" ) stAttribute = attribute;
				UF_ASSERT( stAttribute.descriptor.name == "st" );

				// vertices
			#if UF_XATLAS_LAZY
				auto srcInput = source.vertex;
				auto dstInput = mesh.vertex;

				for ( auto j = 0; j < xmesh.vertexCount; ++j ) {
					auto& vertex = xmesh.vertexArray[j];
					auto ref = vertex.xref;
					
					for ( auto k = 0; k < srcInput.attributes.size(); ++k ) {
						auto dstAttribute = dstInput.attributes[k];
						if ( dstAttribute.descriptor.name != "st" ) continue;
						pod::Vector2f& st = *(pod::Vector2f*) ( static_cast<uint8_t*>(dstAttribute.pointer) + dstAttribute.stride * (ref + dstInput.first) );
						st = pod::Vector2f{ vertex.uv[0] / atlas.pointer->width, vertex.uv[1] / atlas.pointer->height };;
					}
				}
			#else
				for ( auto j = 0; j < xmesh.vertexCount; ++j ) {
					auto& vertex = xmesh.vertexArray[j];
					auto ref = vertex.xref;

					for ( auto k = 0; k < mesh.vertex.attributes.size(); ++k ) {
						auto srcAttribute = source.vertex.attributes[k];
						auto dstAttribute = mesh.vertex.attributes[k];

						if ( dstAttribute.descriptor.name == "st" ) {
							pod::Vector2f& st = *(pod::Vector2f*) ( ((uint8_t*) dstAttribute.pointer) + dstAttribute.stride * j);
							st = pod::Vector2f{ vertex.uv[0] / atlas.pointer->width, vertex.uv[1] / atlas.pointer->height };
						} else {
							memcpy(  static_cast<uint8_t*>(dstAttribute.pointer) + dstAttribute.stride * j,  static_cast<uint8_t*>(srcAttribute.pointer) + srcAttribute.stride * ref, srcAttribute.stride );
						}
					}
				/*
					if ( mesh.isInterleaved( mesh.vertex.interleaved ) ) {
						uint8_t* srcAttribute = source.buffers[mesh.vertex.interleaved].data() + j * mesh.vertex.size;
						uint8_t* dstAttribute = mesh.buffers[mesh.vertex.interleaved].data() + j * mesh.vertex.size;

						memcpy( dstAttribute, srcAttribute, mesh.vertex.size );

						pod::Vector2f& st = *(pod::Vector2f*) (dstAttribute + stAttribute.descriptor.offset);
						st = { vertex.uv[0] / atlas.pointer->width, vertex.uv[1] / atlas.pointer->height };
					} else for ( auto& attribute : mesh.vertex.attributes ) {
						uint8_t* srcAttribute = source.buffers[attribute.buffer].data() + j * attribute.descriptor.size;
						uint8_t* dstAttribute = mesh.buffers[attribute.buffer].data() + j * attribute.descriptor.size;
						
						if ( attribute.descriptor.name == "st" ) {
							pod::Vector2f& st = *(pod::Vector2f*) dstAttribute;
							st = { vertex.uv[0] / atlas.pointer->width, vertex.uv[1] / atlas.pointer->height };
						} else {
							memcpy( dstAttribute, srcAttribute, attribute.descriptor.size );
						}
					}
				*/
				}
				// indices
				if ( mesh.index.count ) {
					uint8_t* pointer = (uint8_t*) mesh.buffers[mesh.isInterleaved(mesh.index.interleaved) ? mesh.index.interleaved : mesh.index.attributes.front().buffer].data();
					for ( auto index = 0; index < xmesh.indexCount; ++index ) {
						switch ( mesh.index.size ) {
							case 1: (( uint8_t*) pointer)[index] = xmesh.indexArray[index]; break;
							case 2: ((uint16_t*) pointer)[index] = xmesh.indexArray[index]; break;
							case 4: ((uint32_t*) pointer)[index] = xmesh.indexArray[index]; break;
						}
					}
				}
			#endif
			}
			mesh.updateDescriptor();
		}
	}

#if !UF_XATLAS_LAZY
	// update vertexID offsets for indirect commands
	for ( auto index = 0; index < graph.meshes.size(); ++index ) {
		auto& name = graph.meshes[index];
		auto& mesh = /*graph.storage*/uf::graph::storage.meshes[name];
		
		if ( !mesh.indirect.count ) continue;

		auto& primitives = /*graph.storage*/uf::graph::storage.primitives[name];
		pod::DrawCommand* drawCommands = (pod::DrawCommand*) mesh.getBuffer(mesh.indirect).data();

	
		size_t vertexID = 0;
		for ( auto i = 0; i < mesh.indirect.count; ++i ) {
			auto& primitive = primitives[i];
			auto& drawCommand = drawCommands[i];

			drawCommand.vertexID = vertexID;
			primitive.drawCommand.vertexID = vertexID;

			vertexID += drawCommand.vertices;
		}
	}
#endif

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