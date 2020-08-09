template<typename T>
void uf::BaseMesh<T>::initialize( bool compress ) {
	this->destroy(false);

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
	graphic.device = &ext::vulkan::device;
	graphic.description.size = sizeof(vertex_t);
	graphic.description.attributes = vertex_t::descriptor;
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
	graphic.indices = indices.size();
	this->generated = true;
	// wait for shaders and uniforms
}
template<typename T>
void uf::BaseMesh<T>::destroy( bool clear ) {
	if ( this->generated ) this->graphic.destroy();
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
