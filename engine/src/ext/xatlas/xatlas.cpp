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
		auto& mesh = uf::graph::storage.meshes[name];
		sources.emplace_back(mesh);

		if ( mesh.index.count ) {
			uf::Mesh::Attribute positionAttribute;
			uf::Mesh::Attribute uvAttribute;
			uf::Mesh::Attribute stAttribute;
			for ( auto& attribute : mesh.vertex.attributes ) {
				if ( attribute.descriptor.name == "position" ) positionAttribute = attribute;
				else if ( attribute.descriptor.name == "uv" ) uvAttribute = attribute;
				else if ( attribute.descriptor.name == "st" ) stAttribute = attribute;
			}
			UF_ASSERT( positionAttribute.descriptor.name == "position" && uvAttribute.descriptor.name == "uv" && uvAttribute.descriptor.name == "st" );

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
//	packOptions.bruteForce = true;
//	packOptions.resolution = resolution;
//	packOptions.texelsPerUnit = 64.0f;
//	packOptions.blockAlign = true;
//	packOptions.bilinear = true;

	::xatlas::Generate(atlas, chartOptions, packOptions);

	for ( size_t i = 0; i < atlas->meshCount; i++ ) {
		auto& xmesh = atlas->meshes[i];
		auto& entry = entries[i];
		auto& mesh = graph.meshes[entry.index];
		auto& source = sources[entry.index];
		source.updateDescriptor();


	}

#if 0
	for ( size_t i = 0; i < atlas->meshCount; i++ ) {
		auto& xmesh = atlas->meshes[i];
		auto& mesh = *meshes[i];

		uf::Mesh src;
		src.buffers = mesh.buffers;
		src.vertex = mesh.vertex;
		src.index = mesh.index;
		src.instance = mesh.instance;
		src.indirect = mesh.indirect;

		mesh.resizeVertices( xmesh.vertexCount );

		if ( mesh.isInterleaved( mesh.vertex.interleaved ) ) {
			for ( auto& attribute : mesh.vertex.attributes ) {
				if ( attribute.descriptor.name != "st" ) continue;

				uint8_t* srcBuffer = src.buffers[mesh.vertex.interleaved].data();
				uint8_t* dstBuffer = mesh.buffers[mesh.vertex.interleaved].data();
				for ( auto v = 0; v < xmesh.vertexCount; ++v ) {
					auto& vertex = xmesh.vertexArray[v];
					auto ref = vertex.xref;

					memcpy( dstBuffer + v * mesh.vertex.stride, srcBuffer + ref * mesh.vertex.stride, mesh.vertex.stride );

					pod::Vector2f& st = *((pod::Vector2f*) dstBuffer + v * mesh.vertex.stride + attribute.descriptor.offset);
					st = { vertex.uv[0] / atlas->width, vertex.uv[1] / atlas->height };
				}

				break;
			}
		} else for ( auto& attribute : mesh.vertex.attributes ) {
			uint8_t* srcBuffer = src.buffers[attribute.buffer].data();
			uint8_t* dstBuffer = mesh.buffers[attribute.buffer].data();
			for ( auto v = 0; v < xmesh.vertexCount; ++v ) {
				auto& vertex = xmesh.vertexArray[v];
				auto ref = vertex.xref;

				if ( attribute.descriptor.name == "st" ) {
					pod::Vector2f& st = *((pod::Vector2f*) dstBuffer + v * attribute.descriptor.size);
					st = { vertex.uv[0] / atlas->width, vertex.uv[1] / atlas->height };
				} else {
					memcpy( dstBuffer + v * attribute.descriptor.size, srcBuffer + ref * attribute.descriptor.size, attribute.descriptor.size );
				}
			}
		}

		{
			auto& buffer = mesh.buffers[mesh.isInterleaved(mesh.index.interleaved) ? mesh.index.interleaved : mesh.index.attributes.front().buffer];
			uint8_t* pointer = (uint8_t*) buffer.data();
			for ( auto index = 0; index < mesh.index.count; ++index ) {
				switch ( mesh.index.stride ) {
					case 1: *( uint8_t*) pointer = xmesh.indexArray[index]; break;
					case 2: *(uint16_t*) pointer = xmesh.indexArray[index]; break;
					case 4: *(uint32_t*) pointer = xmesh.indexArray[index]; break;
				}
			}
		}
	}
#endif

	pod::Vector2ui size = pod::Vector2ui{ atlas->width, atlas->height };
	::xatlas::Destroy(atlas);
	
	return size;
#endif
}