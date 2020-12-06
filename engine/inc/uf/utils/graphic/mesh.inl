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
void uf::BaseMesh<T, U>::expand( bool check ) {
	if ( this->indices.empty() ) return;
	std::vector<vertex_t> _vertices = std::move( this->vertices );
	this->vertices.clear();
	this->vertices.reserve( this->indices.size() );
	if ( !check ) {
		for ( auto& index : this->indices ) this->vertices.emplace_back( _vertices[index] );
	} else {
		std::vector<vertex_t> cache;
		bool valid = true;
		cache.reserve(3);
		for ( auto& index : this->indices ) {
			// flush cache
			if ( cache.size() == 3 ) {
				if ( valid ) {
					this->vertices.emplace_back(cache[0]);
					this->vertices.emplace_back(cache[1]);
					this->vertices.emplace_back(cache[2]);
				}
				cache.clear();
				valid = true;
			}
			// invalid index, mark cache as invalid
			if ( index >= _vertices.size() ) {
				std::cout << "Invalid index: Max: " << _vertices.size() << "\tGot: " << index << std::endl;
				valid = false;
				cache.emplace_back( cache.empty() ? vertex_t{} : cache.back() );
				continue;
			}
			// fill cache
			cache.emplace_back( _vertices[index] );
		}
	}
	this->indices.clear();
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
