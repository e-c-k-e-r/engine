template<typename T, typename U>
void uf::BaseMesh<T, U>::initialize( size_t o ) {
	this->optimize(o);
	if ( this->indices.empty() ) this->generateIndices();
	this->updateDescriptor();
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
				UF_MSG_DEBUG("Invalid index: Max: " << _vertices.size() << "\tGot: " << index);
				valid = false;
				cache.emplace_back( cache.empty() ? vertex_t{} : cache.back() );
				continue;
			}
			// fill cache
			cache.emplace_back( _vertices[index] );
		}
	}
	this->generateIndices();
	this->updateDescriptor();
}
template<typename T, typename U>
void uf::BaseMesh<T, U>::updateDescriptor() {
	attributes.vertex.size = sizeof(vertex_t);
	attributes.index.size = sizeof(indices_t);
	attributes.vertex.pointer = &this->vertices[0];
	attributes.index.pointer = &this->indices[0];
	attributes.vertex.length = this->vertices.size();
	attributes.index.length = this->indices.size();
	attributes.descriptor = vertex_t::descriptor;
}
template<typename T, typename U>
void uf::BaseMesh<T, U>::destroy() {
	this->indices.clear();
	this->vertices.clear();
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
#if UF_USE_MESHOPTIMIZER
	ext::meshopt::optimize( *this, o );
#else
	this->indices.clear();
	this->indices.reserve(vertices.size());
	for ( size_t i = 0; i < vertices.size(); ++i )
		this->indices.emplace_back(i);
#endif
	this->updateDescriptor();
}
template<typename T, typename U>
void uf::BaseMesh<T,U>::insert( const uf::BaseMesh<T,U>& mesh ) {
	size_t startVertices = this->vertices.size();
	size_t startIndices = this->indices.size();

	this->vertices.reserve( startVertices + mesh.vertices.size() );
	this->indices.reserve( startIndices + mesh.indices.size() );
	for ( auto& v : mesh.vertices ) this->vertices.emplace_back( v );
	for ( auto& i : mesh.indices ) this->indices.emplace_back( i + startVertices );
}

template<typename T, typename U>
void uf::BaseMesh<T,U>::resizeVertices( size_t n ) {
	this->vertices.resize(n);
	this->updateDescriptor();
}
template<typename T, typename U>
void uf::BaseMesh<T,U>::resizeIndices( size_t n ) {
	this->indices.resize(n);
	this->updateDescriptor();
}