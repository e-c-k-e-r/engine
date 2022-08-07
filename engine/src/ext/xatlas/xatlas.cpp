#include <uf/ext/xatlas/xatlas.h>
#if UF_USE_XATLAS
#include <xatlas/xatlas.h>

#define UF_XATLAS_UNWRAP_MULTITHREAD 1
#define UF_XATLAS_UNWRAP_SERIAL 1 // really big scenes will gorge on memory

size_t ext::xatlas::unwrap( pod::Graph& graph ) {
	return graph.metadata["exporter"]["unwrap lazy"].as<bool>(false) ? unwrapLazy( graph ) : unwrapExperimental( graph );
}
size_t ext::xatlas::unwrapExperimental( pod::Graph& graph ) {
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

	// copy source meshes
	// create mesh decls for passing to xatlas
	for ( auto index = 0; index < graph.meshes.size(); ++index ) {
		auto& name = graph.meshes[index];
		auto& mesh = /*graph.storage*/uf::graph::storage.meshes[name];
		auto& source = sources[index];

		if ( mesh.isInterleaved() ) {
			UF_EXCEPTION("unwrapping interleaved mesh is not supported");
		}

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
					size_t atlasID = drawCommands[i].auxID;

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
		auto& mesh = /*graph.storage*/uf::graph::storage.meshes[name];
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
			auto& mesh = /*graph.storage*/uf::graph::storage.meshes[name];
			auto& source = sources[entry.index];

			if ( source.vertex.count == 0 ) continue;

			source.updateDescriptor();

			// draw commands
			if ( !mesh.indirect.count ) continue;

			auto& primitives = /*graph.storage*/uf::graph::storage.primitives[name];
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
		auto& mesh = /*graph.storage*/uf::graph::storage.meshes[name];
		auto& source = sources[index];

		if ( source.vertex.count == 0 ) continue;
		if ( !mesh.indirect.count ) continue;

		auto& primitives = /*graph.storage*/uf::graph::storage.primitives[name];
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

		size_t vertexIDOffset = 0;
		for ( auto i = 0; i < atlas.pointer->meshCount; i++ ) {
			auto& xmesh = atlas.pointer->meshes[i];
			auto& entry = atlas.entries[i];
			auto& name = graph.meshes[entry.index];
			auto& mesh = /*graph.storage*/uf::graph::storage.meshes[name];
			auto& source = sources[entry.index];

			if ( source.vertex.count == 0 ) continue;

			// draw commands
			if ( mesh.indirect.count ) {
				auto srcInput = source.remapVertexInput( entry.commandID );
				auto dstInput = mesh.remapVertexInput( entry.commandID );

				auto& primitives = /*graph.storage*/uf::graph::storage.primitives[name];
				pod::DrawCommand* drawCommands = (pod::DrawCommand*) mesh.getBuffer(mesh.indirect).data();

				auto& drawCommand = drawCommands[entry.commandID];
				auto& primitive = primitives[entry.commandID];

				for ( auto j = 0; j < xmesh.vertexCount; ++j ) {
					auto& vertex = xmesh.vertexArray[j];
					auto ref = vertex.xref;
					
					for ( auto _ = 0; _ < srcInput.attributes.size(); ++_ ) {
						auto srcAttribute = srcInput.attributes[_];
						auto dstAttribute = dstInput.attributes[_];

						memcpy(
							static_cast<uint8_t*>(dstAttribute.pointer) + dstAttribute.stride * (j + dstInput.first),
							static_cast<uint8_t*>(srcAttribute.pointer) + srcAttribute.stride * (ref + srcInput.first),
							srcAttribute.stride
						);
					}
				}

				for ( auto j = 0; j < xmesh.vertexCount; ++j ) {
					auto& vertex = xmesh.vertexArray[j];
					auto ref = vertex.xref;
					
					for ( auto _ = 0; _ < srcInput.attributes.size(); ++_ ) {
						auto dstAttribute = dstInput.attributes[_];
						if ( dstAttribute.descriptor.name != "st" ) continue;
						pod::Vector2f& st = *(pod::Vector2f*) ( static_cast<uint8_t*>(dstAttribute.pointer) + dstAttribute.stride * (j + dstInput.first) );
						st = pod::Vector2f{ vertex.uv[0] / atlas.pointer->width, vertex.uv[1] / atlas.pointer->height };;
					}
				}

				// indices
				if ( mesh.index.count ) {
					uf::Mesh::Input indexInput = mesh.remapIndexInput( entry.commandID );
					uf::Mesh::Attribute indexAttribute = indexInput.attributes.front();
					uint8_t* pointer = (uint8_t*) static_cast<uint8_t*>(indexAttribute.pointer) + indexAttribute.stride * indexInput.first;
					for ( auto index = 0; index < xmesh.indexCount; ++index ) {
						switch ( mesh.index.size ) {
							case 1: (( uint8_t*) pointer)[index] = xmesh.indexArray[index]; break;
							case 2: ((uint16_t*) pointer)[index] = xmesh.indexArray[index]; break;
							case 4: ((uint32_t*) pointer)[index] = xmesh.indexArray[index]; break;
						}
					}
				}
			} else {
				uf::Mesh::Attribute stAttribute;
				for ( auto& attribute : mesh.vertex.attributes ) if ( attribute.descriptor.name == "st" ) stAttribute = attribute;
				UF_ASSERT( stAttribute.descriptor.name == "st" );

				// vertices
				for ( auto j = 0; j < xmesh.vertexCount; ++j ) {
					auto& vertex = xmesh.vertexArray[j];
					auto ref = vertex.xref;

					for ( auto k = 0; k < mesh.vertex.attributes.size(); ++k ) {
						auto srcAttribute = source.vertex.attributes[k];
						auto dstAttribute = mesh.vertex.attributes[k];

						if ( dstAttribute.descriptor.name == "st" ) {
							pod::Vector2f& st = *(pod::Vector2f*) ( ((uint8_t*) dstAttribute.pointer) + dstAttribute.stride * (mesh.vertex.first + j));
							st = pod::Vector2f{ vertex.uv[0] / atlas.pointer->width, vertex.uv[1] / atlas.pointer->height };
						} else {
							memcpy(  static_cast<uint8_t*>(dstAttribute.pointer) + dstAttribute.stride * j,  static_cast<uint8_t*>(srcAttribute.pointer) + srcAttribute.stride * ref, srcAttribute.stride );
						}
					}
				}
				// indices
				if ( mesh.index.count ) {
				//	uint8_t* pointer = (uint8_t*) mesh.buffers[mesh.isInterleaved(mesh.index.interleaved) ? mesh.index.interleaved : mesh.index.attributes.front().buffer].data();
					uint8_t* pointer = (uint8_t*) mesh.getBuffer(mesh.index).data();
					for ( auto index = 0; index < xmesh.indexCount; ++index ) {
						switch ( mesh.index.size ) {
							case 1: (( uint8_t*) pointer)[index] = xmesh.indexArray[index]; break;
							case 2: ((uint16_t*) pointer)[index] = xmesh.indexArray[index]; break;
							case 4: ((uint32_t*) pointer)[index] = xmesh.indexArray[index]; break;
						}
					}
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

size_t ext::xatlas::unwrapLazy( pod::Graph& graph ) {
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

	uf::stl::unordered_map<size_t, Atlas> atlases;
	atlases.reserve(graph.meshes.size());

	// copy source meshes
	// create mesh decls for passing to xatlas
	for ( auto index = 0; index < graph.meshes.size(); ++index ) {
		auto& name = graph.meshes[index];
		auto& mesh = /*graph.storage*/uf::graph::storage.meshes[name];

		if ( mesh.isInterleaved() ) {
			UF_EXCEPTION("unwrapping interleaved mesh is not supported");
		}

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
					size_t atlasID = drawCommands[i].auxID;

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
#if UF_XATLAS_UNWRAP_SERIAL
	size_t atlasCount = 0;
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

		::xatlas::Generate(atlas.pointer, chartOptions, packOptions);

		for ( auto i = 0; i < atlas.pointer->meshCount; i++ ) {
			auto& xmesh = atlas.pointer->meshes[i];
			auto& entry = atlas.entries[i];
			auto& name = graph.meshes[entry.index];
			auto& mesh = /*graph.storage*/uf::graph::storage.meshes[name];

			// draw commands
			if ( mesh.indirect.count ) {
				auto vertexInput = mesh.remapVertexInput( entry.commandID );
				for ( auto j = 0; j < xmesh.vertexCount; ++j ) {
					auto& vertex = xmesh.vertexArray[j];
					auto ref = vertex.xref;
					
					for ( auto k = 0; k < vertexInput.attributes.size(); ++k ) {
						auto dstAttribute = vertexInput.attributes[k];
						if ( dstAttribute.descriptor.name != "st" ) continue;
						pod::Vector2f& st = *(pod::Vector2f*) ( static_cast<uint8_t*>(dstAttribute.pointer) + dstAttribute.stride * (ref + vertexInput.first) );
						st = pod::Vector2f{ vertex.uv[0] / atlas.pointer->width, vertex.uv[1] / atlas.pointer->height };;
					}
				}
			} else {
				for ( auto j = 0; j < xmesh.vertexCount; ++j ) {
					auto& vertex = xmesh.vertexArray[j];
					auto ref = vertex.xref;
					
					for ( auto k = 0; k < mesh.vertex.attributes.size(); ++k ) {
						auto dstAttribute = mesh.vertex.attributes[k];
						if ( dstAttribute.descriptor.name != "st" ) continue;
						pod::Vector2f& st = *(pod::Vector2f*) ( static_cast<uint8_t*>(dstAttribute.pointer) + dstAttribute.stride * (ref + mesh.vertex.first) );
						st = pod::Vector2f{ vertex.uv[0] / atlas.pointer->width, vertex.uv[1] / atlas.pointer->height };;
					}
				}
			}
			mesh.updateDescriptor();
		}

		::xatlas::Destroy(atlas.pointer);
		++atlasCount;
	}
#else
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

	// update vertices
	for ( auto& pair : atlases ) {
		auto& atlas = pair.second;

		for ( auto i = 0; i < atlas.pointer->meshCount; i++ ) {
			auto& xmesh = atlas.pointer->meshes[i];
			auto& entry = atlas.entries[i];
			auto& name = graph.meshes[entry.index];
			auto& mesh = /*graph.storage*/uf::graph::storage.meshes[name];

			// draw commands
			if ( mesh.indirect.count ) {
				auto vertexInput = mesh.remapVertexInput( entry.commandID );
				for ( auto j = 0; j < xmesh.vertexCount; ++j ) {
					auto& vertex = xmesh.vertexArray[j];
					auto ref = vertex.xref;
					
					for ( auto k = 0; k < vertexInput.attributes.size(); ++k ) {
						auto dstAttribute = vertexInput.attributes[k];
						if ( dstAttribute.descriptor.name != "st" ) continue;
						pod::Vector2f& st = *(pod::Vector2f*) ( static_cast<uint8_t*>(dstAttribute.pointer) + dstAttribute.stride * (ref + vertexInput.first) );
						st = pod::Vector2f{ vertex.uv[0] / atlas.pointer->width, vertex.uv[1] / atlas.pointer->height };;
					}
				}
			} else {
				for ( auto j = 0; j < xmesh.vertexCount; ++j ) {
					auto& vertex = xmesh.vertexArray[j];
					auto ref = vertex.xref;
					
					for ( auto k = 0; k < mesh.vertex.attributes.size(); ++k ) {
						auto dstAttribute = mesh.vertex.attributes[k];
						if ( dstAttribute.descriptor.name != "st" ) continue;
						pod::Vector2f& st = *(pod::Vector2f*) ( static_cast<uint8_t*>(dstAttribute.pointer) + dstAttribute.stride * (ref + mesh.vertex.first) );
						st = pod::Vector2f{ vertex.uv[0] / atlas.pointer->width, vertex.uv[1] / atlas.pointer->height };;
					}
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
#endif
	return atlasCount;
}
#endif