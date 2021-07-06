template<typename T, typename U>
void ext::opengl::Graphic::initializeMesh( uf::Mesh<T, U>& mesh, size_t o ) {
	if ( mesh.indices.empty() ) mesh.initialize( o );
	mesh.updateDescriptor();
	initializeAttributes( mesh.attributes );
}