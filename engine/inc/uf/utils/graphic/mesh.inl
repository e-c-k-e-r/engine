template<typename T, typename U>
void uf::BaseMesh<T, U>::initialize( size_t o ) {
	// this->destroy(false);
	this->updateDescriptor();

	if ( o > 0 ) {
		this->optimize(o);
	} else {
		this->indices.clear();
		this->indices.reserve(vertices.size());
		for ( size_t i = 0; i < vertices.size(); ++i )
			this->indices.emplace_back(i);
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

template<typename T, typename U>
uf::BaseMesh<T,U> uf::BaseMesh<T,U>::simplify( float threshold ) {
	size_t target = size_t(this->indices.size() * threshold);
	float error = 1e-2f;
	uf::BaseMesh<T,U> lod;
	lod.indices.resize(this->indices.size()); // note: simplify needs space for index_count elements in the destination array, not target
	lod.indices.resize(meshopt_simplify(&lod.indices[0], &this->indices[0], this->indices.size(), (float*) &this->vertices[0].position, this->vertices.size(), sizeof(T), target, error));
	lod.vertices.resize(lod.indices.size() < this->vertices.size() ? lod.indices.size() : this->vertices.size()); // note: this is just to reduce the cost of resize()
	lod.vertices.resize(meshopt_optimizeVertexFetch(&lod.vertices[0], &lod.indices[0], lod.indices.size(), &this->vertices[0], this->vertices.size(), sizeof(T)));
	return lod;
}
template<typename T, typename U>
void uf::BaseMesh<T,U>::optimize( size_t o ) {
	// generate indices
	auto vertices = std::move( this->vertices );
	U indices = vertices.size();

	std::vector<U> remap(indices);
	size_t verticesCount = meshopt_generateVertexRemap(&remap[0], NULL, indices, &vertices[0], indices, sizeof(T));
	
	this->indices.resize(indices);
	meshopt_remapIndexBuffer(&this->indices[0], NULL, indices, &remap[0]);
	this->vertices.resize(verticesCount);
	meshopt_remapVertexBuffer(&this->vertices[0], &vertices[0], indices, sizeof(T), &remap[0]);
	// optimize for cache
	if ( o >= 1 ) {
		meshopt_optimizeVertexCache(&this->indices[0], &this->indices[0], this->indices.size(), this->vertices.size());
	}
	// optimize for overdraw
	if ( o >= 2 ) {
		const float kOverdrawThreshold = 3.f;
		meshopt_optimizeOverdraw(&this->indices[0], &this->indices[0], this->indices.size(), (float*) &this->vertices[0].position, this->vertices.size(), sizeof(T), kOverdrawThreshold);
	}
	// optimize for fetch
	if ( o >= 3 ) {
		meshopt_optimizeVertexFetch(&this->vertices[0], &this->indices[0], this->indices.size(), &this->vertices[0], this->vertices.size(), sizeof(T));
	}
/*
	old nasty way

	mesh.indices.clear();
	mesh.vertices.clear();
	
	std::unordered_map<vertex_t, indices_t> unique;

	std::vector<vertex_t> _vertices = std::move( mesh.vertices );
	for ( vertex_t& vertex : _vertices ) {
		if ( unique.count(vertex) == 0 ) {
			unique[vertex] = static_cast<indices_t>(mesh.vertices.size());
			mesh.vertices.push_back( vertex );
		}
		mesh.indices.push_back( unique[vertex] );
	}
*/
}