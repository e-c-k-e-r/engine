#include <uf/ext/xatlas/xatlas.h>
#if UF_USE_XATLAS
	#include <xatlas/xatlas.h>
#endif
pod::Vector2ui UF_API ext::xatlas::unwrap( pod::Graph& graph ) {
#if UF_USE_XATLAS
	::xatlas::Atlas* atlas = ::xatlas::Create();
	for ( auto& mesh : graph.meshes ) {
		mesh.updateDescriptor();

		size_t indices = mesh.attributes.index.length;
		size_t indexStride = mesh.attributes.index.size;
		uint8_t* indexPointer = (uint8_t*) mesh.attributes.index.pointer;

		size_t vertices = mesh.attributes.vertex.length;
		size_t vertexStride = mesh.attributes.vertex.size;
		uint8_t* vertexPointer = (uint8_t*) mesh.attributes.vertex.pointer;

		uf::renderer::VertexDescriptor 	vertexAttributePosition, 
										vertexAttributeNormal,
										vertexAttributeUv,
										vertexAttributeSt,
										vertexAttributeId;

		for ( auto& attribute : mesh.attributes.descriptor ) {
			if ( attribute.name == "position" ) vertexAttributePosition = attribute;
			else if ( attribute.name == "normal" ) vertexAttributeNormal = attribute;
			else if ( attribute.name == "uv" ) vertexAttributeUv = attribute;
			else if ( attribute.name == "st" ) vertexAttributeSt = attribute;
			else if ( attribute.name == "id" ) vertexAttributeId = attribute;
		}
		UF_ASSERT( vertexAttributePosition.name != "" );

		::xatlas::MeshDecl meshDecl;
		meshDecl.vertexCount = vertices; // mesh.vertices.size();
		meshDecl.vertexPositionData = (uint8_t*) mesh.attributes.vertex.pointer + vertexAttributePosition.offset; /*((uint8_t*) mesh.vertices.data()) + vertexAttributePosition.offset*/
		meshDecl.vertexPositionStride = vertexStride;
		if ( vertexAttributeUv.name != "" ) {
			meshDecl.vertexUvData = (uint8_t*) mesh.attributes.vertex.pointer + vertexAttributeUv.offset;// ((uint8_t*) mesh.vertices.data()) + vertexAttributeUv.offset;
			meshDecl.vertexUvStride = vertexStride;
		}
	/*
		if ( vertexAttributeNormal.name != "" ) {
			meshDecl.vertexNormalData = ((uint8_t*) mesh.vertices.data()) + vertexAttributeNormal.offset;
			meshDecl.vertexNormalStride = mesh.attributes.vertex.size;
		}
	*/
		meshDecl.indexCount = indices; // mesh.indices.size();
		meshDecl.indexData = mesh.attributes.index.pointer; // mesh.indices.data();
		meshDecl.indexFormat = indexStride == 4 ? ::xatlas::IndexFormat::UInt32 : ::xatlas::IndexFormat::UInt16;

		::xatlas::AddMeshError error = ::xatlas::AddMesh(atlas, meshDecl, graph.meshes.size());
		if (error != ::xatlas::AddMeshError::Success) {
			::xatlas::Destroy(atlas);
			UF_MSG_ERROR(::xatlas::StringForEnum(error));
			return {};
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

#if UF_GRAPH_EXPERIMENTAL
	for ( size_t i = 0; i < atlas->meshCount; i++ ) {
		auto& xmesh = atlas->meshes[i];
		auto& mesh = graph.meshes[i];

		size_t indices = mesh.attributes.index.length;
		size_t indexStride = mesh.attributes.index.size;
		uint8_t* indexPointer = (uint8_t*) mesh.attributes.index.pointer;

		uf::stl::vector<uint8_t> oldVertexBuffer( (uint8_t*) mesh.attributes.vertex.pointer, (uint8_t*) mesh.attributes.vertex.pointer + mesh.attributes.vertex.length * mesh.attributes.vertex.size );

		size_t vertices = xmesh.vertexCount;
		size_t vertexStride = mesh.attributes.vertex.size;
		
		mesh.resizeVertices( vertices );

		uint8_t* oldVertexPointer = oldVertexBuffer.data();
		uint8_t* newvertexPointer = (uint8_t*) mesh.attributes.vertex.pointer;

		uf::renderer::VertexDescriptor 	vertexAttributePosition, 
										vertexAttributeNormal,
										vertexAttributeUv,
										vertexAttributeSt,
										vertexAttributeId;

		for ( auto& attribute : mesh.attributes.descriptor ) {
			if ( attribute.name == "position" ) vertexAttributePosition = attribute;
			else if ( attribute.name == "normal" ) vertexAttributeNormal = attribute;
			else if ( attribute.name == "uv" ) vertexAttributeUv = attribute;
			else if ( attribute.name == "st" ) vertexAttributeSt = attribute;
			else if ( attribute.name == "id" ) vertexAttributeId = attribute;
		}
		UF_ASSERT( vertexAttributePosition.name != "" );

		for ( size_t v = 0; v < xmesh.vertexCount; v++) {
			auto& xvertex = xmesh.vertexArray[v];
			uint8_t* oldVertexSrc = oldVertexPointer + (xvertex.xref * vertexStride);
			uint8_t* newVertexSrc = newvertexPointer + (v * vertexStride);

			memcpy( newVertexSrc, oldVertexSrc, vertexStride );

			pod::Vector2f& st = *((pod::Vector2f*) (newVertexSrc + vertexAttributeSt.offset));
			st = { xvertex.uv[0] / atlas->width, xvertex.uv[1] / atlas->height };
		}

		for ( size_t currentIndex = 0; currentIndex < xmesh.indexCount; ++currentIndex ) {
			uint32_t index = xmesh.indexArray[currentIndex];
			uint8_t* indexSrc = indexPointer + (currentIndex * indexStride);
			switch ( indexStride ) {
				case sizeof( uint8_t): *(( uint8_t*) indexSrc) = index; break;
				case sizeof(uint16_t): *((uint16_t*) indexSrc) = index; break;
				case sizeof(uint32_t): *((uint32_t*) indexSrc) = index; break;
			}
		}
	}

	pod::Vector2ui size = pod::Vector2ui{ atlas->width, atlas->height };
	::xatlas::Destroy(atlas);
#else
#if 0
	for (size_t i = 0; i < atlas->meshCount; i++) {
		auto& mesh = graph.meshes[i];
		auto& xmesh = atlas->meshes[i];
		for ( size_t v = 0; v < xmesh.vertexCount; v++) {
			auto& xvertex = xmesh.vertexArray[v];
			auto& vertex = mesh.vertices[xvertex.xref];
			vertex.st = { xvertex.uv[0] / atlas->width, xvertex.uv[1] / atlas->height };
		}
	}
#else
	auto oldMeshes = std::move( graph.meshes );
	graph.meshes.resize( oldMeshes.size() );
	for (size_t i = 0; i < atlas->meshCount; i++) {
		auto& xmesh = atlas->meshes[i];
		auto& oldMesh = oldMeshes[i];
		auto& newMesh = graph.meshes[i];		
		newMesh.vertices.reserve( xmesh.vertexCount );
		newMesh.indices.reserve( xmesh.indexCount );
		for ( size_t v = 0; v < xmesh.vertexCount; v++) {
			auto& xvertex = xmesh.vertexArray[v];
			auto& oldVertex = oldMesh.vertices[xvertex.xref];
			auto& newVertex = newMesh.vertices.emplace_back(oldVertex);
			newVertex.st = { xvertex.uv[0] / atlas->width, xvertex.uv[1] / atlas->height };
		}
		for ( size_t i = 0; i < xmesh.indexCount; ++i ) newMesh.indices.emplace_back( xmesh.indexArray[i] );
		newMesh.updateDescriptor();
	}
#endif

	pod::Vector2ui size = pod::Vector2ui{ atlas->width, atlas->height };
	::xatlas::Destroy(atlas);

	for ( auto& node : graph.nodes ) {
		if ( !(0 <= node.mesh && node.mesh < graph.meshes.size()) ) continue;
		auto& nodeMesh = graph.meshes[node.mesh];
		for ( auto& vertex : nodeMesh.vertices ) {
			vertex.id.x = node.index;
		// in almost-software mode, material ID is actually its albedo ID, as we can't use any other forms of mapping
		#if UF_USE_OPENGL_FIXED_FUNCTION
			if ( !(graph.mode & uf::graph::LoadMode::ATLAS) ) {
				auto& material = graph.materials[vertex.id.y];
				auto& texture = graph.textures[material.storage.indexAlbedo];
				vertex.id.y = texture.storage.index;
			}
		#endif
		}
	}
#endif
	return size;
#else
	return pod::Vector2ui{};
#endif
}