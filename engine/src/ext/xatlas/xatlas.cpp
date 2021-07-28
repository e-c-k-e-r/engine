#include <uf/ext/xatlas/xatlas.h>
#if UF_USE_XATLAS
	#include <xatlas/xatlas.h>
#endif
#if 0
pod::Vector2ui UF_API ext::xatlas::unwrap( uf::stl::vector<uf::graph::mesh::Skinned>& vertices, uf::stl::vector<uint32_t>& indices ) {
#if UF_USE_XATLAS
	uf::stl::vector<uf::graph::mesh::Skinned> source = std::move(vertices);
	::xatlas::Atlas* atlas = ::xatlas::Create();

	::xatlas::MeshDecl decl;
	decl.vertexCount = source.size();
	decl.vertexPositionData = source.data() + offsetof(uf::graph::mesh::Skinned, position);
	decl.vertexPositionStride = sizeof(uf::graph::mesh::Skinned);
	decl.vertexUvData = source.data() + offsetof(uf::graph::mesh::Skinned, uv);
	decl.vertexUvStride = sizeof(uf::graph::mesh::Skinned);

	decl.indexCount = indices.size();
	decl.indexData = indices.data();
	decl.indexFormat = ::xatlas::IndexFormat::UInt32;

	::xatlas::AddMeshError error = ::xatlas::AddMesh(atlas, decl, 1);
	if (error != ::xatlas::AddMeshError::Success) {
		::xatlas::Destroy(atlas);
		UF_EXCEPTION(::xatlas::StringForEnum(error));
	}

	auto& xmesh = atlas->meshes[0];
	vertices.resize( xmesh.vertexCount );

	for ( auto i = 0; i < xmesh.vertexCount; ++i ) {
		auto& vertex = xmesh.vertexArray[i];

		vertices[i] = source[vertex.xref];
		vertices[i].st = { vertex.uv[0] / atlas->width, vertex.uv[1] / atlas->height };
	}
	for ( auto i = 0; i < xmesh.indexCount; ++i ) indices[i] = xmesh.indexArray[i];

	pod::Vector2ui size = pod::Vector2ui{ atlas->width, atlas->height };
	::xatlas::Destroy(atlas);
	
	return size;
#endif
}
#endif
pod::Vector2ui UF_API ext::xatlas::unwrap( pod::Graph& graph ) {
#if UF_USE_XATLAS
	struct Pair {
		size_t index = 0;
		size_t command = 0;
		::xatlas::MeshDecl decl;
	};

	uf::stl::vector<Pair> entries;
	entries.reserve(graph.meshes.size());
	
	uf::stl::vector<uf::Mesh> sources;
	sources.reserve(graph.meshes.size());

	::xatlas::Atlas* atlas = ::xatlas::Create();
	for ( auto index = 0; index < graph.meshes.size(); ++index ) {
		auto& name = graph.meshes[index];
		auto& mesh = /*graph.storage*/uf::graph::storage.meshes[name];
		sources.emplace_back(mesh).updateDescriptor();

		if ( mesh.index.count ) {
			uf::Mesh::Attribute positionAttribute;
			uf::Mesh::Attribute uvAttribute;
			uf::Mesh::Attribute stAttribute;
			for ( auto& attribute : mesh.vertex.attributes ) {
				if ( attribute.descriptor.name == "position" ) positionAttribute = attribute;
				else if ( attribute.descriptor.name == "uv" ) uvAttribute = attribute;
				else if ( attribute.descriptor.name == "st" ) stAttribute = attribute;
			}
			UF_ASSERT( positionAttribute.descriptor.name == "position" && uvAttribute.descriptor.name == "uv" && stAttribute.descriptor.name == "st" );

			auto& indexAttribute = mesh.index.attributes.front();
			::xatlas::IndexFormat indexType = ::xatlas::IndexFormat::UInt32;
			switch ( mesh.index.stride ) {
				case sizeof(uint16_t): indexType = ::xatlas::IndexFormat::UInt16; break;
				case sizeof(uint32_t): indexType = ::xatlas::IndexFormat::UInt32; break;
				default: UF_EXCEPTION("unsupported index type"); break;
			}

			if ( mesh.indirect.count ) {
				uf::Mesh::Attribute remappedPositionAttribute;
				uf::Mesh::Attribute remappedUvAttribute;
				uf::Mesh::Attribute remappedIndexAttribute;
				for ( auto i = 0; i < mesh.indirect.count; ++i ) {
					remappedPositionAttribute = mesh.remapVertexAttribute( positionAttribute, i );
					remappedUvAttribute = mesh.remapVertexAttribute( uvAttribute, i );
					remappedIndexAttribute = mesh.remapIndexAttribute( indexAttribute, i );

					auto& entry = entries.emplace_back();
					entry.index = index;
					entry.command = i;

					auto& decl = entry.decl;
					decl.vertexCount = remappedUvAttribute.length;
					decl.vertexPositionData = remappedPositionAttribute.pointer;
					decl.vertexPositionStride = remappedPositionAttribute.stride;
					decl.vertexUvData = remappedUvAttribute.pointer;
					decl.vertexUvStride = remappedUvAttribute.stride;

					decl.indexCount = remappedIndexAttribute.length;
					decl.indexData = remappedIndexAttribute.pointer;
					decl.indexFormat = indexType;
				}
			} else {
				auto& entry = entries.emplace_back();
				entry.index = index;

				auto& decl = entry.decl;
				decl.vertexCount = uvAttribute.length;
				decl.vertexPositionData = positionAttribute.pointer;
				decl.vertexPositionStride = positionAttribute.stride;
				decl.vertexUvData = uvAttribute.pointer;
				decl.vertexUvStride = uvAttribute.stride;

				decl.indexCount = indexAttribute.length;
				decl.indexData = indexAttribute.pointer;
				decl.indexFormat = indexType;
				decl.indexFormat = indexType;

			}
		} else UF_EXCEPTION("to-do: not require indices for meshes");
	}
	for ( auto& mesh : entries ) {
		::xatlas::AddMeshError error = ::xatlas::AddMesh(atlas, mesh.decl, entries.size());
		if (error != ::xatlas::AddMeshError::Success) {
			::xatlas::Destroy(atlas);
			UF_EXCEPTION(::xatlas::StringForEnum(error));
		}
	}

	::xatlas::ChartOptions chartOptions{};
//	chartOptions.useInputMeshUvs = true;

	::xatlas::PackOptions packOptions{};
	packOptions.bruteForce = true;
//	packOptions.resolution = resolution;
//	packOptions.texelsPerUnit = 64.0f;
	packOptions.blockAlign = true;
	packOptions.bilinear = true;

	::xatlas::Generate(atlas, chartOptions, packOptions);

	uf::stl::vector<size_t> sizes( graph.meshes.size(), 0 );
	for ( auto i = 0; i < atlas->meshCount; ++i ) {
		auto& xmesh = atlas->meshes[i];
		auto& entry = entries[i];
		sizes[entry.index] += xmesh.vertexCount;
	}
	for ( auto i = 0; i < graph.meshes.size(); ++i ) {
		auto& name = graph.meshes[i];
		auto& mesh = /*graph.storage*/uf::graph::storage.meshes[name];

		mesh.resizeVertices( sizes[i] );
		mesh.updateDescriptor();

		if ( mesh.indirect.count ) {
			auto& primitive = /*graph.storage*/uf::graph::storage.primitives[name];
			pod::DrawCommand* drawCommands = (pod::DrawCommand*) mesh.buffers[mesh.isInterleaved(mesh.indirect.interleaved) ? mesh.indirect.interleaved : mesh.indirect.attributes.front().buffer].data();

			size_t vertexOffset = 0;
			for ( auto j = 0; j < atlas->meshCount; ++j ) {
				auto& entry = entries[j];
				if ( entry.index != i ) continue;

				auto vertexCount = atlas->meshes[j].vertexCount;

				drawCommands[entry.command].vertices = vertexCount;
				drawCommands[entry.command].vertexID = vertexOffset;
				
				primitive[entry.command].drawCommand.vertices = vertexCount;
				primitive[entry.command].drawCommand.vertexID = vertexOffset;

				vertexOffset += vertexCount;
			}
		}
	}

	for ( auto i = 0; i < atlas->meshCount; i++ ) {
		auto& xmesh = atlas->meshes[i];
		auto& entry = entries[i];
		auto& name = graph.meshes[entry.index];
		auto& mesh = /*graph.storage*/uf::graph::storage.meshes[name];
		auto& source = sources[entry.index];

		// draw commands
		if ( mesh.indirect.count ) {
			// vertices
			for ( auto j = 0; j < xmesh.vertexCount; ++j ) {
				auto& vertex = xmesh.vertexArray[j];
				auto ref = vertex.xref;
				for ( auto k = 0; k < mesh.vertex.attributes.size(); ++k ) {
					auto srcAttribute = source.remapVertexAttribute( source.vertex.attributes[k], entry.command );
					auto dstAttribute = mesh.remapVertexAttribute( mesh.vertex.attributes[k], entry.command );

					if ( dstAttribute.descriptor.name == "st" ) {
						pod::Vector2f& st = *(pod::Vector2f*) ( ((uint8_t*) dstAttribute.pointer) + dstAttribute.stride * j);
						st = pod::Vector2f{ vertex.uv[0] / atlas->width, vertex.uv[1] / atlas->height };
					} else {
						memcpy(  ((uint8_t*) dstAttribute.pointer) + dstAttribute.stride * j,  ((uint8_t*) srcAttribute.pointer) + srcAttribute.stride * ref, srcAttribute.stride );
					}
				}
			}
			// indices
			if ( mesh.index.count ) {
				uf::Mesh::Attribute indexAttribute = mesh.remapIndexAttribute( mesh.index.attributes.front(), entry.command );
				uint8_t* pointer = (uint8_t*) indexAttribute.pointer;
				for ( auto index = 0; index < xmesh.indexCount; ++index ) {
					switch ( mesh.index.stride ) {
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

				if ( mesh.isInterleaved( mesh.vertex.interleaved ) ) {
					uint8_t* srcAttribute = source.buffers[mesh.vertex.interleaved].data() + j * mesh.vertex.stride;
					uint8_t* dstAttribute = mesh.buffers[mesh.vertex.interleaved].data() + j * mesh.vertex.stride;

					memcpy( dstAttribute, srcAttribute, mesh.vertex.stride );

					pod::Vector2f& st = *(pod::Vector2f*) (dstAttribute + stAttribute.descriptor.offset);
					st = { vertex.uv[0] / atlas->width, vertex.uv[1] / atlas->height };
				} else for ( auto& attribute : mesh.vertex.attributes ) {
					uint8_t* srcAttribute = source.buffers[attribute.buffer].data() + j * attribute.descriptor.size;
					uint8_t* dstAttribute = mesh.buffers[attribute.buffer].data() + j * attribute.descriptor.size;
					
					if ( attribute.descriptor.name == "st" ) {
						pod::Vector2f& st = *(pod::Vector2f*) dstAttribute;
						st = { vertex.uv[0] / atlas->width, vertex.uv[1] / atlas->height };
					} else {
						memcpy( dstAttribute, srcAttribute, attribute.descriptor.size );
					}
				}
			}
			// indices
			if ( mesh.index.count ) {
				uint8_t* pointer = (uint8_t*) mesh.buffers[mesh.isInterleaved(mesh.index.interleaved) ? mesh.index.interleaved : mesh.index.attributes.front().buffer].data();
				for ( auto index = 0; index < xmesh.indexCount; ++index ) {
					switch ( mesh.index.stride ) {
						case 1: (( uint8_t*) pointer)[index] = xmesh.indexArray[index]; break;
						case 2: ((uint16_t*) pointer)[index] = xmesh.indexArray[index]; break;
						case 4: ((uint32_t*) pointer)[index] = xmesh.indexArray[index]; break;
					}
				}
			}
		}

		mesh.updateDescriptor();
	}

	pod::Vector2ui size = pod::Vector2ui{ atlas->width, atlas->height };
	::xatlas::Destroy(atlas);
	
	return size;
#endif
}