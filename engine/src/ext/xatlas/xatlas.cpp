#include <uf/ext/xatlas/xatlas.h>
#if UF_USE_XATLAS
	#include <xatlas/xatlas.h>
#endif
pod::Vector2ui UF_API ext::xatlas::unwrap( pod::Graph& graph ) {
#if UF_USE_XATLAS
	::xatlas::Atlas* atlas = ::xatlas::Create();
#if 0
	for ( auto& name : graph.meshes ) {
		auto& mesh = uf::graph::storage.meshes[name];
		mesh.updateDescriptor();

		size_t indices = mesh.attributes.index.length;
		size_t indexStride = mesh.attributes.index.size;
		uint8_t* indexPointer = (uint8_t*) mesh.attributes.index.pointer;

		size_t vertices = mesh.attributes.vertex.length;
		size_t vertexStride = mesh.attributes.vertex.size;
		uint8_t* vertexPointer = (uint8_t*) mesh.attributes.vertex.pointer;

		uf::renderer::AttributeDescriptor 	vertexAttributePosition, 
										vertexAttributeNormal,
										vertexAttributeUv,
										vertexAttributeSt,
										vertexAttributeId;

		for ( auto& attribute : mesh.attributes.vertex.descriptor ) {
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

	for ( size_t i = 0; i < atlas->meshCount; i++ ) {
		auto& xmesh = atlas->meshes[i];
		auto& mesh = uf::graph::storage.meshes[graph.meshes[i]];

		size_t indices = mesh.attributes.index.length;
		size_t indexStride = mesh.attributes.index.size;
		uint8_t* indexPointer = (uint8_t*) mesh.attributes.index.pointer;

		uf::stl::vector<uint8_t> oldVertexBuffer( (uint8_t*) mesh.attributes.vertex.pointer, (uint8_t*) mesh.attributes.vertex.pointer + mesh.attributes.vertex.length * mesh.attributes.vertex.size );

		size_t vertices = xmesh.vertexCount;
		size_t vertexStride = mesh.attributes.vertex.size;
		
		mesh.resizeVertices( vertices );

		uint8_t* oldVertexPointer = oldVertexBuffer.data();
		uint8_t* newvertexPointer = (uint8_t*) mesh.attributes.vertex.pointer;

		uf::renderer::AttributeDescriptor 	vertexAttributePosition, 
										vertexAttributeNormal,
										vertexAttributeUv,
										vertexAttributeSt;

		for ( auto& attribute : mesh.attributes.vertex.descriptor ) {
			if ( attribute.name == "position" ) vertexAttributePosition = attribute;
			else if ( attribute.name == "normal" ) vertexAttributeNormal = attribute;
			else if ( attribute.name == "uv" ) vertexAttributeUv = attribute;
			else if ( attribute.name == "st" ) vertexAttributeSt = attribute;
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
	
	return size;
#endif
#endif
}