template<typename T, typename U>
void ext::vulkan::Graphic::initializeMesh( uf::BaseMesh<T, U>& mesh, size_t o ) {
	if ( mesh.indices.empty() ) mesh.initialize( o );
	mesh.updateDescriptor();
	initializeGeometry( mesh );
}