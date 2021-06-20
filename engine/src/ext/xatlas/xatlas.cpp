#include <uf/ext/xatlas/xatlas.h>
#if UF_USE_XATLAS
	#include <xatlas/xatlas.h>
#endif
pod::Vector2ui UF_API ext::xatlas::unwrap( pod::Graph& graph ) {
#if UF_USE_XATLAS
	::xatlas::Atlas* atlas = ::xatlas::Create();
	for ( auto& mesh : graph.meshes ) {
		mesh.updateDescriptor();
		uf::renderer::VertexDescriptor 	vertexAttributePosition, 
										vertexAttributeNormal,
										vertexAttributeUv;
		for ( auto& attribute : mesh.attributes.descriptor ) {
			if ( attribute.name == "position" ) vertexAttributePosition = attribute;
			else if ( attribute.name == "normal" ) vertexAttributeNormal = attribute;
			else if ( attribute.name == "uv" ) vertexAttributeUv = attribute;
		}
		if ( vertexAttributePosition.name == "" ) continue;

		::xatlas::MeshDecl meshDecl;
		meshDecl.vertexCount = mesh.vertices.size();
		meshDecl.vertexPositionData = ((uint8_t*) mesh.vertices.data()) + vertexAttributePosition.offset;
		meshDecl.vertexPositionStride = mesh.attributes.vertex.size;
		if ( vertexAttributeUv.name != "" ) {
			meshDecl.vertexUvData = ((uint8_t*) mesh.vertices.data()) + vertexAttributeUv.offset;
			meshDecl.vertexUvStride = mesh.attributes.vertex.size;
		}
	/*
		if ( vertexAttributeNormal.name != "" ) {
			meshDecl.vertexNormalData = ((uint8_t*) mesh.vertices.data()) + vertexAttributeNormal.offset;
			meshDecl.vertexNormalStride = mesh.attributes.vertex.size;
		}
	*/
		meshDecl.indexCount = mesh.indices.size();
		meshDecl.indexData = mesh.indices.data();
		meshDecl.indexFormat = ::xatlas::IndexFormat::UInt32;

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
		for ( size_t i = 0; i < xmesh.indexCount; ++i ) {
			newMesh.indices.emplace_back( xmesh.indexArray[i] );
		}
		newMesh.updateDescriptor();
	}
/*
	for (size_t i = 0; i < atlas->meshCount; i++) {
		auto& mesh = graph.meshes[i];
		auto& aMesh = atlas->meshes[i];
		for ( size_t v = 0; v < aMesh.vertexCount; v++) {
			auto& aVertex = aMesh.vertexArray[v];
			auto& vertex = mesh.vertices[aVertex.xref];
			vertex.st[0] = (float) aVertex.uv[0] / (float) atlas->width;
			vertex.st[1] = (float) aVertex.uv[1] / (float) atlas->height;
		}
	}
*/
	pod::Vector2ui size = pod::Vector2ui{ atlas->width, atlas->height };
	::xatlas::Destroy(atlas);

	for ( auto& node : graph.nodes ) {
		if ( !(0 <= node.mesh && node.mesh < graph.meshes.size()) ) continue;
		auto& nodeMesh = graph.meshes[node.mesh];
		for ( auto& vertex : nodeMesh.vertices ) {
			vertex.id.x = node.index;
		// in almost-software mode, material ID is actually its albedo ID, as we can't use any other forms of mapping
		#if UF_USE_OPENGL_FIXED_FUNCTION
			if ( !(graph.mode & ext::gltf::LoadMode::ATLAS) ) {
				auto& material = graph.materials[vertex.id.y];
				auto& texture = graph.textures[material.storage.indexAlbedo];
				vertex.id.y = texture.storage.index;
			}
		#endif
		}
	}
	return size;
#else
	return pod::Vector2ui{};
#endif
}