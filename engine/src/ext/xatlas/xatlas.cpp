#include <uf/ext/xatlas/xatlas.h>
#if UF_USE_XATLAS
	#include <xatlas/xatlas.h>
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

	/*
		if ( mesh.isInterleaved() ) {
			sources.emplace_back(mesh.convert()).updateDescriptor();
		} else {
			sources.emplace_back(mesh).updateDescriptor();
		}
	*/
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
				for ( auto i = 0; i < mesh.indirect.count; ++i ) {
					vertexInput = mesh.remapVertexInput( i );
					indexInput = mesh.remapIndexInput( i );

					auto& entry = entries.emplace_back();
					entry.index = index;
					entry.command = i;

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
				auto& entry = entries.emplace_back();
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
			pod::DrawCommand* drawCommands = (pod::DrawCommand*) mesh.getBuffer(mesh.indirect).data();

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
				//	auto srcAttribute = source.remapVertexAttribute( source.vertex.attributes[k], entry.command );
				//	auto dstAttribute = mesh.remapVertexAttribute( mesh.vertex.attributes[k], entry.command );

					auto srcInput = source.remapVertexInput( entry.command );
					auto dstInput = mesh.remapVertexInput( entry.command );

					auto srcAttribute = source.vertex.attributes[k];
					auto dstAttribute = mesh.vertex.attributes[k];

					if ( dstAttribute.descriptor.name == "st" ) {
						pod::Vector2f& st = *(pod::Vector2f*) ( static_cast<uint8_t*>(dstAttribute.pointer) + dstAttribute.stride * (j + dstInput.first) );
						st = pod::Vector2f{ vertex.uv[0] / atlas->width, vertex.uv[1] / atlas->height };
					} else {
						memcpy(  static_cast<uint8_t*>(dstAttribute.pointer) + dstAttribute.stride * (j + dstInput.first),  static_cast<uint8_t*>(srcAttribute.pointer) + srcAttribute.stride * (ref + srcInput.first), srcAttribute.stride );
					}
				}
			}
			// indices
			if ( mesh.index.count ) {
				uf::Mesh::Input indexInput = mesh.remapIndexInput( entry.command );
				uf::Mesh::Attribute indexAttribute = mesh.index.attributes.front();
			//	uf::Mesh::Attribute indexAttribute = mesh.remapIndexAttribute( mesh.index.attributes.front(), entry.command );
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
						pod::Vector2f& st = *(pod::Vector2f*) ( ((uint8_t*) dstAttribute.pointer) + dstAttribute.stride * j);
						st = pod::Vector2f{ vertex.uv[0] / atlas->width, vertex.uv[1] / atlas->height };
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
		}

		mesh.updateDescriptor();
	}

#if 0
		for ( auto i = 0; i < atlas->meshCount; i++ ) {
		auto& xmesh = atlas->meshes[i];
		auto& entry = entries[i];
		auto& name = graph.meshes[entry.index];
		auto& mesh = /*graph.storage*/uf::graph::storage.meshes[name];
		auto& source = sources[entry.index];

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
				for ( auto v = 0; v < xmesh.vertexCount; ++v ) {
					auto& vertex = xmesh.vertexArray[v];
					auto ref = vertex.xref;

					vertexInput = mesh.remapVertexInput( entry.command );

					for ( size_t _ = 0; _ < vertexInput.attributes.size(); ++_ ) {
						auto& srcAttribute = source.vertex.attributes[_];
						auto& dstAttribute = mesh.vertex.attributes[_];

						memcpy( static_cast<uint8_t*>(dstAttribute.pointer) + dstAttribute.stride * (vertexInput.first + v), static_cast<uint8_t*>(srcAttribute.pointer) + srcAttribute.stride * (vertexInput.first + ref), srcAttribute.stride );
					}
					
					pod::Vector2f& st = *(pod::Vector2f*) (static_cast<uint8_t*>(stAttribute.pointer) + stAttribute.stride * (vertexInput.first + v));
					st = { vertex.uv[0] / atlas->width, vertex.uv[1] / atlas->height  };
				}
				// indices

				indexInput = mesh.remapIndexInput( entry.command );
				for ( auto index = 0; index < xmesh.indexCount; ++index ) {
					switch ( mesh.index.size ) {
						case 1: (( uint8_t*) static_cast<uint8_t*>(indexAttribute.pointer) + indexAttribute.stride * indexInput.first)[index] = xmesh.indexArray[index]; break;
						case 2: ((uint16_t*) static_cast<uint8_t*>(indexAttribute.pointer) + indexAttribute.stride * indexInput.first)[index] = xmesh.indexArray[index]; break;
						case 4: ((uint32_t*) static_cast<uint8_t*>(indexAttribute.pointer) + indexAttribute.stride * indexInput.first)[index] = xmesh.indexArray[index]; break;
					}
				}
			} else {
				for ( auto v = 0; v < xmesh.vertexCount; ++v ) {
					auto& vertex = xmesh.vertexArray[v];
					auto ref = vertex.xref;

					for ( size_t _ = 0; _ < vertexInput.attributes.size(); ++_ ) {
						auto& srcAttribute = source.vertex.attributes[_];
						auto& dstAttribute = mesh.vertex.attributes[_];

						memcpy( static_cast<uint8_t*>(dstAttribute.pointer) + dstAttribute.stride * (vertexInput.first + v), static_cast<uint8_t*>(srcAttribute.pointer) + srcAttribute.stride * (vertexInput.first + ref), srcAttribute.stride );
					}
					
					pod::Vector2f& st = *(pod::Vector2f*) (static_cast<uint8_t*>(stAttribute.pointer) + stAttribute.stride * (vertexInput.first + v));
					st = { vertex.uv[0] / atlas->width, vertex.uv[1] / atlas->height  };
				}

				for ( auto index = 0; index < xmesh.indexCount; ++index ) {
					switch ( mesh.index.size ) {
						case 1: (( uint8_t*) static_cast<uint8_t*>(indexAttribute.pointer) + indexAttribute.stride * indexInput.first)[index] = xmesh.indexArray[index]; break;
						case 2: ((uint16_t*) static_cast<uint8_t*>(indexAttribute.pointer) + indexAttribute.stride * indexInput.first)[index] = xmesh.indexArray[index]; break;
						case 4: ((uint32_t*) static_cast<uint8_t*>(indexAttribute.pointer) + indexAttribute.stride * indexInput.first)[index] = xmesh.indexArray[index]; break;
					}
				}
			}
		} else UF_EXCEPTION("to-do: not require indices for meshes");
	}
#endif

	pod::Vector2ui size = pod::Vector2ui{ atlas->width, atlas->height };
	::xatlas::Destroy(atlas);
	
	return size;
#endif
}