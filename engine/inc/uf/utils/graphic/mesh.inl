template<typename T, typename U>
void uf::BaseMesh<T, U>::initialize( bool compress ) {
	// this->destroy(false);
	this->updateDescriptor();

	if ( compress ) {
		std::unordered_map<vertex_t, indices_t> unique;
		std::vector<vertex_t> _vertices = std::move( this->vertices );
		
		this->indices.clear();
		this->vertices.clear();
		this->indices.reserve(_vertices.size());
		this->vertices.reserve(_vertices.size());

		for ( vertex_t& vertex : _vertices ) {
			if ( unique.count(vertex) == 0 ) {
				unique[vertex] = static_cast<indices_t>(this->vertices.size());
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
}	
template<typename T, typename U>
void uf::BaseMesh<T, U>::updateDescriptor() {
	sizes.vertex = sizeof(vertex_t);
	sizes.indices = sizeof(indices_t);
	attributes = vertex_t::descriptor;
}
template<typename T, typename U>
void uf::BaseMesh<T, U>::destroy() {
	this->indices.clear();
	this->vertices.clear();
}

template<typename T, typename U>
uf::BaseMesh<T, U>::~BaseMesh() {
	this->destroy();
}
