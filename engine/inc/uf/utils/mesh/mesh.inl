#if 0
template<typename T, typename U>
void uf::Mesh<T, U>::initialize( size_t o ) {
	this->optimize(o);
	if ( this->indices.empty() ) this->generateIndices();
	this->updateDescriptor();
}	
template<typename T, typename U>
void uf::Mesh<T, U>::expand( bool check ) {
	if ( this->indices.empty() ) return;
	uf::stl::vector<vertex_t> _vertices = std::move( this->vertices );
	this->vertices.clear();
	this->vertices.reserve( this->indices.size() );
	if ( !check ) {
		for ( auto& index : this->indices ) this->vertices.emplace_back( _vertices[index] );
	} else {
		uf::stl::vector<vertex_t> cache;
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
void uf::Mesh<T, U>::updateDescriptor() {
	attributes.vertex.size = sizeof(vertex_t);
	attributes.vertex.length = this->vertices.size();
	attributes.vertex.pointer = &this->vertices[0];
	attributes.vertex.descriptor = vertex_t::descriptor;

	attributes.index.size = sizeof(indices_t);
	attributes.index.length = this->indices.size();
	attributes.index.pointer = &this->indices[0];
}
template<typename T, typename U>
void uf::Mesh<T, U>::destroy() {
	this->indices.clear();
	this->vertices.clear();
}

template<typename T, typename U>
uf::Mesh<T,U> uf::Mesh<T,U>::simplify( float threshold ) {
	size_t target = size_t(this->indices.size() * threshold);
	float error = 1e-2f;
	uf::Mesh<T,U> lod;
	lod.indices.resize(this->indices.size());
	lod.indices.resize(meshopt_simplify(&lod.indices[0], &this->indices[0], this->indices.size(), (float*) &this->vertices[0].position, this->vertices.size(), sizeof(T), target, error));
	lod.vertices.resize(lod.indices.size() < this->vertices.size() ? lod.indices.size() : this->vertices.size()); // note: this is just to reduce the cost of resize()
	lod.vertices.resize(meshopt_optimizeVertexFetch(&lod.vertices[0], &lod.indices[0], lod.indices.size(), &this->vertices[0], this->vertices.size(), sizeof(T)));
	return lod;
}
template<typename T, typename U>
void uf::Mesh<T,U>::optimize( size_t o ) {
#if UF_USE_MESHOPTIMIZER
	ext::meshopt::optimize( *this, o );
#endif
	this->updateDescriptor();
}
template<typename T, typename U>
void uf::Mesh<T,U>::insert( const uf::Mesh<T,U>& mesh ) {
	size_t startVertices = this->vertices.size();
	size_t startIndices = this->indices.size();

	this->vertices.reserve( startVertices + mesh.vertices.size() );
	this->indices.reserve( startIndices + mesh.indices.size() );
	for ( auto& v : mesh.vertices ) this->vertices.emplace_back( v );
	for ( auto& i : mesh.indices ) this->indices.emplace_back( i + startVertices );
	this->updateDescriptor();
}

template<typename T, typename U>
void uf::Mesh<T,U>::resizeVertices( size_t n ) {
	this->vertices.resize(n);
	this->updateDescriptor();
}
template<typename T, typename U>
void uf::Mesh<T,U>::resizeIndices( size_t n ) {
	this->indices.resize(n);
	this->updateDescriptor();
}

//

template<typename T, typename U>
bool pod::VaryingMesh::is() const {
#if 0
	return uf::pointeredUserdata::is<uf::Mesh<T,U>>();
#else
	if ( attributes.vertex.descriptor.size() != T::descriptor.size() ||
	// vertex types are the same size
	attributes.vertex.size != sizeof(T) ||
	// index types are the same size
	attributes.index.size != sizeof(U) ) return false;

	for ( size_t i = 0; i < attributes.vertex.descriptor.size(); ++i ) {
		if ( attributes.vertex.descriptor[i].offset != attributes.vertex.descriptor[i].offset || attributes.vertex.descriptor[i].size != attributes.vertex.descriptor[i].size || attributes.vertex.descriptor[i].components != attributes.vertex.descriptor[i].components ) return false;
	}
	return true;
#if 0
return 
	// descriptors are the same size
	attributes.vertex.descriptor.size() == T::descriptor.size() && 
	// vertex types are the same size
	attributes.vertex.size == sizeof(T) && 
	// index types are the same size
	attributes.index.size == sizeof(U) && 
	// attributes are the same
	std::equal( attributes.vertex.descriptor.begin(), attributes.vertex.descriptor.end(), T::descriptor.begin() );
#endif
#endif
}

template<typename T, typename U>
uf::Mesh<T,U>& pod::VaryingMesh::get() {
	if ( !userdata.data ) set<T,U>();
	return uf::pointeredUserdata::get<uf::Mesh<T,U>>( userdata );
}

template<typename T, typename U>
const uf::Mesh<T,U>& pod::VaryingMesh::get() const {
	UF_ASSERT( userdata.data );
	return uf::pointeredUserdata::get<uf::Mesh<T,U>>( userdata );
}

template<typename T, typename U>
void pod::VaryingMesh::set( const uf::Mesh<T, U>& mesh ) {
	if ( userdata.data ) uf::pointeredUserdata::destroy( userdata );
	userdata = uf::pointeredUserdata::create( mesh );
	attributes = mesh.attributes;
}

template<typename T, typename U>
void pod::VaryingMesh::insert( const pod::VaryingMesh& mesh ) {
	get<T,U>().insert( mesh.get<T,U>() );
	updateDescriptor();
}
template<typename T, typename U>
void pod::VaryingMesh::insert( const uf::Mesh<T, U>& mesh ) {
	get<T,U>().insert( mesh );
	updateDescriptor();
}
#endif