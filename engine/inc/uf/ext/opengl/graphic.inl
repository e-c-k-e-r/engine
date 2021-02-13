template<typename T, typename U>
void ext::opengl::Graphic::initializeGeometry( uf::BaseMesh<T, U>& mesh, size_t o ) {
	if ( mesh.indices.empty() ) mesh.initialize( o );
	mesh.updateDescriptor();

	// already generated, check if we can just update
	if ( descriptor.indices > 0 ) {
		if ( descriptor.geometry.sizes.vertex == mesh.sizes.vertex && descriptor.geometry.sizes.indices == mesh.sizes.indices && descriptor.indices == mesh.indices.size() ) {
			// too lazy to check if this equals, only matters in pipeline creation anyways
			descriptor.geometry = mesh;

			int32_t vertexBuffer = -1;
			int32_t indexBuffer = -1;
			for ( size_t i = 0; i < buffers.size(); ++i ) {
				if ( buffers[i].usageFlags & 1 ) vertexBuffer = i;
				if ( buffers[i].usageFlags & 2 ) indexBuffer = i;
			}

			if ( vertexBuffer > 0 && indexBuffer > 0 ) {
				updateBuffer(
					(void*) mesh.vertices.data(),
					mesh.vertices.size() * mesh.sizes.vertex,
					vertexBuffer
				);
				updateBuffer(
					(void*) mesh.indices.data(),
					mesh.indices.size() * mesh.sizes.indices,
					indexBuffer
				);
				return;
			}
		}
		// can't reuse buffers, re-create buffers
		{
			for ( auto& buffer : buffers ) buffer.destroy();
			buffers.clear();
		}
	}

	descriptor.geometry = mesh;
	descriptor.indices = mesh.indices.size();

	initializeBuffer(
		(void*) mesh.vertices.data(),
		mesh.vertices.size() * mesh.sizes.vertex,
		/*VK_BUFFER_USAGE_VERTEX_BUFFER_BIT*/ 1
	);
	initializeBuffer(
		(void*) mesh.indices.data(),
		mesh.indices.size() * mesh.sizes.indices,
		/*VK_BUFFER_USAGE_INDEX_BUFFER_BIT*/ 2
	);
}