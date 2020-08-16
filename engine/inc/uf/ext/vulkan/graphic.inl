template<typename T, typename U>
void ext::vulkan::Graphic::initializeGeometry( uf::BaseMesh<T, U>& mesh ) {
	if ( mesh.indices.empty() ) mesh.initialize();
	mesh.updateDescriptor();

	// already generated, check if we can just update
	if ( descriptor.indices > 0 ) {
		if ( descriptor.geometry.sizes.vertex == mesh.sizes.vertex && descriptor.geometry.sizes.indices == mesh.sizes.indices && descriptor.indices == mesh.indices.size() ) {
			// too lazy to check if this equals, only matters in pipeline creation anyways
			descriptor.geometry = mesh;

			updateBuffer(
				(void*) mesh.vertices.data(),
				mesh.vertices.size() * mesh.sizes.vertex,
				0,
				false
			);
			updateBuffer(
				(void*) mesh.indices.data(),
				mesh.indices.size() * mesh.sizes.indices,
				0,
				false
			);
			return;
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
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, //VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		false
	);
	initializeBuffer(
		(void*) mesh.indices.data(),
		mesh.indices.size() * mesh.sizes.indices,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, //VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		false
	);
}