template<typename T, typename U>
void uf::MeshGrid::initialize( const uf::BaseMesh<T,U>& mesh, size_t divisions ) {
	return initialize<T,U>( mesh, pod::Vector3ui{ divisions, divisions, divisions } );
}
template<typename T, typename U>
void uf::MeshGrid::initialize( const uf::BaseMesh<T,U>& mesh, const pod::Vector3ui& size ) {
	// calculate our extents
	pod::Vector3f min = {  std::numeric_limits<float>::max(),  std::numeric_limits<float>::max(),  std::numeric_limits<float>::max() };
	pod::Vector3f max = { -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() };
	for ( auto& vertex : mesh.vertices ) {
		auto& position = vertex.position;
		min.x = std::min( position.x, min.x );
		min.y = std::min( position.z, min.y );
		min.z = std::min( position.y, min.z );

		max.x = std::max( position.x, max.x );
		max.y = std::max( position.z, max.y );
		max.z = std::max( position.y, max.z );
	}
	pod::Vector3f center = (max + min) / 2;
	pod::Vector3f extent = (max - min) / 2;
	// pre-generate nodes
	initialize( center, extent, size );
	// fill our nodes
	for ( size_t i = 0; i < mesh.indices.size() / 3; ++i ) {
		// triangle face
		auto& i1 = mesh.indices[i*3+0];
		auto& i2 = mesh.indices[i*3+1];
		auto& i3 = mesh.indices[i*3+2];
		// triangle positions
		auto& v1 = mesh.vertices[i1].position;
		auto& v2 = mesh.vertices[i2].position;
		auto& v3 = mesh.vertices[i3].position;

		insert(i1, i2, i3, v1, v2, v3);
	}
}

template<typename U = uint32_t>
std::vector<U> uf::MeshGrid::get() const {
#if 0
	auto& node = get( point );
	std::vector<U> indices(node.count);
	for ( size_t i = 0; i < node.count; ++i ) indices[i] = node.indices[i];
	return indices;
#else
	std::vector<U> indices;
	for ( auto& node : this->m_nodes ) {
		indices.insert( indices.end(), node.indices.begin(), node.indices.end() );
	}
	return indices;
#endif
}
		
template<typename U = uint32_t>
const std::vector<U>& uf::MeshGrid::get( const pod::Vector3f& point ) const {
#if 0
	auto& node = get( point );
	std::vector<U> indices(node.count);
	for ( size_t i = 0; i < node.count; ++i ) indices[i] = node.indices[i];
	return indices;
#else
	return at( point ).indices;
#endif
}