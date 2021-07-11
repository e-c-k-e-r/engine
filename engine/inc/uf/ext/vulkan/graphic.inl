#if 0
template<typename T, typename U>
void ext::vulkan::Graphic::initializeMesh( uf::Mesh<T, U>& mesh, size_t o ) {
	if ( mesh.indices.empty() ) mesh.initialize( o );
	mesh.updateDescriptor();
	initializeAttributes( mesh.attributes );
}
#endif