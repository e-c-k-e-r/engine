template<typename T>
void uf::BaseMesh<T>::initialize( bool compress ) {
	// this->destroy(false);
	if ( compress ) {
		std::unordered_map<vertex_t, uint32_t> unique;
		std::vector<vertex_t> _vertices = std::move( this->vertices );
		
		this->indices.clear();
		this->vertices.clear();
		this->indices.reserve(_vertices.size());
		this->vertices.reserve(_vertices.size());

		for ( vertex_t& vertex : _vertices ) {
			if ( unique.count(vertex) == 0 ) {
				unique[vertex] = static_cast<uint32_t>(this->vertices.size());
				this->vertices.push_back( vertex );
			}
			this->indices.push_back( unique[vertex] );
		}
	} else {
		this->indices.clear();
		this->indices.reserve(vertices.size());
		for ( size_t i = 0; i < vertices.size(); ++i ) {
			this->indices.push_back(i);
		}
	}
	this->generate();
}	
template<typename T>
void uf::BaseMesh<T>::generate() {
	// already generated, check if we can just update
	if ( graphic.descriptor.indices > 0 ) {
		if ( graphic.descriptor.size == sizeof(vertex_t) && graphic.descriptor.indices == indices.size() ) {
			// too lazy to check if this equals, only matters in pipeline creation anyways
			graphic.descriptor.attributes = vertex_t::descriptor;

			graphic.updateBuffer(
				(void*) vertices.data(),
				vertices.size() * sizeof(vertex_t),
				0,
				false
			);
			graphic.updateBuffer(
				(void*) indices.data(),
				indices.size() * sizeof(uint32_t),
				0,
				false
			);
			return;
		}
		// can't reuse buffers, re-create buffers
		this->destroy(false);
	}

	graphic.device = &ext::vulkan::device;
	graphic.descriptor.size = sizeof(vertex_t);
	graphic.descriptor.attributes = vertex_t::descriptor;
	graphic.descriptor.indices = indices.size();

	graphic.initializeBuffer(
		(void*) vertices.data(),
		vertices.size() * sizeof(vertex_t),
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, //VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		false
	);
	graphic.initializeBuffer(
		(void*) indices.data(),
		indices.size() * sizeof(uint32_t),
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, //VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		false
	);
	this->generated = true;
	// wait for shaders and uniforms
}
template<typename T>
void uf::BaseMesh<T>::destroy( bool clear ) {
	// if ( this->generated ) this->graphic.destroy();
	if ( this->generated ) {
		for ( auto& buffer : graphic.buffers ) buffer.destroy();
		graphic.buffers.clear();
	}
	this->generated = false;
	if ( clear ) {
		this->indices.clear();
		this->vertices.clear();
	}
}

template<typename T>
uf::BaseMesh<T>::~BaseMesh() {
	this->destroy();
}
