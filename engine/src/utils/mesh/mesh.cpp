#include <uf/utils/mesh/mesh.h>
#include <uf/engine/graph/graph.h>

// Used for per-vertex colors
UF_VERTEX_DESCRIPTOR(pod::Vertex_3F2F3F4F,
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F4F, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F4F, R32G32_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F4F, R32G32B32_SFLOAT, normal)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F4F, R32_UINT, color)
)
// Used for terrain
UF_VERTEX_DESCRIPTOR(pod::Vertex_3F2F3F32B,
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F32B, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F32B, R32G32_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F32B, R32G32B32_SFLOAT, normal)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F32B, R32_UINT, color)
)
// Used for normal meshses
UF_VERTEX_DESCRIPTOR(pod::Vertex_3F2F3F,
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F, R32G32_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F, R32G32B32_SFLOAT, normal)
)
// (Typically) used for displaying textures
UF_VERTEX_DESCRIPTOR(pod::Vertex_3F2F,
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F, R32G32_SFLOAT, uv)
)
UF_VERTEX_DESCRIPTOR(pod::Vertex_2F2F,
	UF_VERTEX_DESCRIPTION(pod::Vertex_2F2F, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(pod::Vertex_2F2F, R32G32_SFLOAT, uv)
)
// used for texture arrays
UF_VERTEX_DESCRIPTOR(pod::Vertex_3F3F3F,
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F3F3F, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F3F3F, R32G32B32_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F3F3F, R32G32B32_SFLOAT, normal)
)
UF_VERTEX_DESCRIPTOR(pod::Vertex_3F2F3F1UI,
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F1UI, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F1UI, R32G32_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F1UI, R32G32B32_SFLOAT, normal)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F1UI, R32_UINT, id)
)
// Basic
UF_VERTEX_DESCRIPTOR(pod::Vertex_3F,
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F, R32G32B32_SFLOAT, position)
)

bool uf::Mesh::defaultInterleaved = false;

void uf::Mesh::initialize() {
}
void uf::Mesh::destroy() {
	_destroy(vertex);
	_destroy(index);
	_destroy(instance);
	_destroy(indirect);

	buffers.clear();
}
uf::Mesh uf::Mesh::interleave() const {
	uf::Mesh interleaved;
	auto& buffer = interleaved.buffers.emplace_back();

#define PARSE_INPUT_INTERLEAVED(name) {\
	interleaved.name = name;\
	interleaved.name.offset = buffer.size();\
	if ( isInterleaved( name.interleaved ) ) buffer.insert( buffer.end(), buffers[name.interleaved].begin(), buffers[name.interleaved].end() );\
	else for ( auto& attribute : name.attributes ) buffer.insert( buffer.end(), buffers[attribute.buffer].begin(), buffers[attribute.buffer].end() );\
}
	PARSE_INPUT_INTERLEAVED(vertex);
	PARSE_INPUT_INTERLEAVED(index);
	PARSE_INPUT_INTERLEAVED(instance);
	PARSE_INPUT_INTERLEAVED(indirect);

	return interleaved;
}
void uf::Mesh::updateDescriptor() {
	_updateDescriptor(vertex);
	_updateDescriptor(index);
	_updateDescriptor(instance);
	_updateDescriptor(indirect);
}
void uf::Mesh::insert( const uf::Mesh& mesh ) {
	insertVertex(mesh);
	insertIndex(mesh);
	insertInstance(mesh);
	insertIndirect(mesh);
}
/*
uint32_t indices = 0; // triangle count
uint32_t instances = 0; // instance count
uint32_t indexID = 0; // starting triangle position
 int32_t vertexID = 0; // starting vertex position

uint32_t instanceID = 0; // starting instance position
uint32_t materialID = 0;
uint32_t objectID = 0;
uint32_t vertices = 0;
*/
void uf::Mesh::generateIndices() {
	// deduce type
	size_t size = sizeof(uint32_t);
	uf::renderer::enums::Type::type_t type;
	/*if ( vertex.count <= std::numeric_limits<uint8_t>::max() ) { size = sizeof(uint8_t); type = uf::renderer::typeToEnum<uint8_t>(); }
	else*/ if ( vertex.count <= std::numeric_limits<uint16_t>::max() ) { size = sizeof(uint16_t); type = uf::renderer::typeToEnum<uint16_t>(); }
	else if ( vertex.count <= std::numeric_limits<uint32_t>::max() ) { size = sizeof(uint32_t); type = uf::renderer::typeToEnum<uint32_t>(); }

	if ( !index.attributes.empty() ) {
		_destroy( index );
	}
	_bindI( index, size, type );
	_bind( isInterleaved( vertex.interleaved ) );
	
	switch ( size ) {
		case 1: { uf::stl::vector<uint8_t>  indices( vertex.count ); std::iota( indices.begin(), indices.end(), 0 ); insertIndices( indices ); } break;
		case 2: { uf::stl::vector<uint16_t> indices( vertex.count ); std::iota( indices.begin(), indices.end(), 0 ); insertIndices( indices ); } break;
		case 4: { uf::stl::vector<uint32_t> indices( vertex.count ); std::iota( indices.begin(), indices.end(), 0 ); insertIndices( indices ); } break;
	}
}
void uf::Mesh::generateIndirect() {
	if ( index.count == 0 ) generateIndices();

	uf::stl::vector<pod::DrawCommand> commands;
	for ( auto& attribute : index.attributes ) {
		auto& buffer = buffers[isInterleaved(index.interleaved) ? index.interleaved : attribute.buffer];
		auto& command = commands.emplace_back();
		command.indices = buffer.size() / attribute.descriptor.size;
		command.instances = instance.count == 0 && instance.attributes.empty() ? 1 : instance.count;
		command.indexID = 0;
		command.vertexID = 0;
		command.instanceID = 0;
		command.objectID = 0;
		command.vertices = vertex.count;
	}

	_destroy( indirect );
	bindIndirect<pod::DrawCommand>();
	_bind();
	insertIndirects( commands );
}
bool uf::Mesh::isInterleaved( size_t i ) const { return 0 <= i && i < buffers.size(); }
void uf::Mesh::print() const {
	std::stringstream str;
	str << "Buffers: " << buffers.size() << "\n";
	str << "Vertices: " << vertex.count << " | " << (isInterleaved(vertex.interleaved) ? "interleaved" : "deinterleaved") << "\n";
	for ( auto i = 0; i < vertex.count; ++i ) {
		if ( isInterleaved(vertex.interleaved) ) {
			auto& buffer = buffers[vertex.interleaved];
			uint8_t* e = (uint8_t*) &buffer[i * vertex.stride];
			for ( auto& attribute : vertex.attributes ) {
				str << "[" << i << "][" << attribute.descriptor.name << "]: ( ";
				switch ( attribute.descriptor.type ) {
					case uf::renderer::enums::Type::FLOAT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((float*) (e + attribute.descriptor.offset))[j] << " "; break;
					case uf::renderer::enums::Type::UINT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((uint32_t*) (e + attribute.descriptor.offset))[j] << " "; break;
					case uf::renderer::enums::Type::INT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((int32_t*) (e + attribute.descriptor.offset))[j] << " "; break;
					case uf::renderer::enums::Type::USHORT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((uint16_t*) (e + attribute.descriptor.offset))[j] << " "; break;
					case uf::renderer::enums::Type::SHORT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((int16_t*) (e + attribute.descriptor.offset))[j] << " "; break;
					case uf::renderer::enums::Type::UBYTE: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((uint8_t*) (e + attribute.descriptor.offset))[j] << " "; break;
					case uf::renderer::enums::Type::BYTE: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((int8_t*) (e + attribute.descriptor.offset))[j] << " "; break;
				}
				str << ")\n";
			}
		} else for ( auto& attribute : vertex.attributes ) {
			auto& buffer = buffers[attribute.buffer];
			str << "[" << i << "][" << attribute.descriptor.name << "]: ( ";
			switch ( attribute.descriptor.type ) {
				case uf::renderer::enums::Type::FLOAT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((float*) &buffer[i * attribute.descriptor.size])[j] << " "; break;
				case uf::renderer::enums::Type::UINT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((uint32_t*) &buffer[i * attribute.descriptor.size])[j] << " "; break;
				case uf::renderer::enums::Type::INT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((int32_t*) &buffer[i * attribute.descriptor.size])[j] << " "; break;
				case uf::renderer::enums::Type::USHORT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((uint16_t*) &buffer[i * attribute.descriptor.size])[j] << " "; break;
				case uf::renderer::enums::Type::SHORT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((int16_t*) &buffer[i * attribute.descriptor.size])[j] << " "; break;
				case uf::renderer::enums::Type::UBYTE: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((uint8_t*) &buffer[i * attribute.descriptor.size])[j] << " "; break;
				case uf::renderer::enums::Type::BYTE: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((int8_t*) &buffer[i * attribute.descriptor.size])[j] << " "; break;
			}
			str << ")\n";
		}
	}
	str << "Indices: " << index.count << " | " << (isInterleaved(index.interleaved) ? "interleaved" : "deinterleaved") << "\n";
	for ( auto i = 0; i < index.count; ++i ) {
		if ( isInterleaved(index.interleaved) ) {
			auto& buffer = buffers[index.interleaved];
			switch ( index.stride ) {
				case 1: str << "[" << i << "]: " << *(( uint8_t*) &buffer[i * index.stride]) << "\n"; break;
				case 2: str << "[" << i << "]: " << *((uint16_t*) &buffer[i * index.stride]) << "\n"; break;
				case 4: str << "[" << i << "]: " << *((uint32_t*) &buffer[i * index.stride]) << "\n"; break;
			}
		} else for ( auto& attribute : index.attributes ) {
			auto& buffer = buffers[attribute.buffer];
			switch ( attribute.descriptor.size ) {
				case 1: str << "[" << i << "]: " << *(( uint8_t*) &buffer[i * attribute.descriptor.size]) << "\n"; break;
				case 2: str << "[" << i << "]: " << *((uint16_t*) &buffer[i * attribute.descriptor.size]) << "\n"; break;
				case 4: str << "[" << i << "]: " << *((uint32_t*) &buffer[i * attribute.descriptor.size]) << "\n"; break;
			}
		}
	}
	str << "Instances: " << instance.count << " | " << (isInterleaved(instance.interleaved) ? "interleaved" : "deinterleaved") << "\n";
	for ( auto i = 0; i < instance.count; ++i ) {
		if ( isInterleaved(instance.interleaved) ) {
			auto& buffer = buffers[instance.interleaved];
			uint8_t* e = (uint8_t*) &buffer[i * instance.stride];
			for ( auto& attribute : instance.attributes ) {
				str << "[" << i << "][" << attribute.descriptor.name << "]: ( ";
				switch ( attribute.descriptor.type ) {
					case uf::renderer::enums::Type::FLOAT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((float*) (e + attribute.descriptor.offset))[j] << " "; break;
					case uf::renderer::enums::Type::UINT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((uint32_t*) (e + attribute.descriptor.offset))[j] << " "; break;
					case uf::renderer::enums::Type::INT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((int32_t*) (e + attribute.descriptor.offset))[j] << " "; break;
					case uf::renderer::enums::Type::USHORT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((uint16_t*) (e + attribute.descriptor.offset))[j] << " "; break;
					case uf::renderer::enums::Type::SHORT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((int16_t*) (e + attribute.descriptor.offset))[j] << " "; break;
					case uf::renderer::enums::Type::UBYTE: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((uint8_t*) (e + attribute.descriptor.offset))[j] << " "; break;
					case uf::renderer::enums::Type::BYTE: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((int8_t*) (e + attribute.descriptor.offset))[j] << " "; break;
				}
				str << ")\n";
			}
		} else for ( auto& attribute : instance.attributes ) {
			auto& buffer = buffers[attribute.buffer];
			str << "[" << i << "][" << attribute.descriptor.name << "]: ( ";
			switch ( attribute.descriptor.type ) {
				case uf::renderer::enums::Type::FLOAT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((float*) &buffer[i * attribute.descriptor.size])[j] << " "; break;
				case uf::renderer::enums::Type::UINT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((uint32_t*) &buffer[i * attribute.descriptor.size])[j] << " "; break;
				case uf::renderer::enums::Type::INT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((int32_t*) &buffer[i * attribute.descriptor.size])[j] << " "; break;
				case uf::renderer::enums::Type::USHORT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((uint16_t*) &buffer[i * attribute.descriptor.size])[j] << " "; break;
				case uf::renderer::enums::Type::SHORT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((int16_t*) &buffer[i * attribute.descriptor.size])[j] << " "; break;
				case uf::renderer::enums::Type::UBYTE: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((uint8_t*) &buffer[i * attribute.descriptor.size])[j] << " "; break;
				case uf::renderer::enums::Type::BYTE: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((int8_t*) &buffer[i * attribute.descriptor.size])[j] << " "; break;
			}
			str << ")\n";
		}
	}
	str << "Indirect: " << indirect.count << " | " << (isInterleaved(indirect.interleaved) ? "interleaved" : "deinterleaved");
	std::cout << str.str() << std::endl;
}
//
void uf::Mesh::_destroy( uf::Mesh::Input& input ) {
	for ( auto& attribute : input.attributes ) {
		attribute.descriptor.length = 0;
		attribute.descriptor.pointer = NULL;
		attribute.buffer = 0;
	}
	input.attributes.clear();
}
void uf::Mesh::_bind( bool interleave ) {
	size_t buffer = 0;
#define PARSE_INPUT(INPUT, INTERLEAVED){\
	if ( INTERLEAVED ) INPUT.interleaved = buffer;\
	for ( auto i = 0; i < INPUT.attributes.size(); ++i ) INPUT.attributes[i].buffer =  !INTERLEAVED ? buffer++ : buffer;\
	if ( !INPUT.attributes.empty() && INTERLEAVED ) ++buffer;\
}
	
	PARSE_INPUT(vertex, interleave)
	PARSE_INPUT(index, false)
	PARSE_INPUT(instance, interleave)
	PARSE_INPUT(indirect, false)

	buffers.resize( buffer );
	updateDescriptor();

//	for ( auto& attribute : vertex.attributes ) UF_MSG_DEBUG( attribute.descriptor.name << "[" << attribute.descriptor.offset << "]: " << attribute.descriptor.size );
#undef PARSE_INPUT
}
void uf::Mesh::_updateDescriptor( uf::Mesh::Input& input ) {
	input.stride = 0;
	for ( auto& attribute : input.attributes ) {
		auto& buffer = buffers[isInterleaved(input.interleaved) ? input.interleaved : attribute.buffer];
		attribute.descriptor.length = buffer.size();
		attribute.descriptor.pointer = (void*) (buffer.data());
		input.stride += attribute.descriptor.size;
	}
}
void uf::Mesh::_insertV( uf::Mesh::Input& input, const uf::Mesh& mesh, const uf::Mesh::Input& srcInput ) {
	if ( !_hasV( input, srcInput ) ) return;
	_reserveVs( input, input.count += srcInput.count );

	// both meshes are interleaved, just copy directly
	if ( isInterleaved(input.interleaved) && isInterleaved(srcInput.interleaved) ) {
		auto& src = mesh.buffers[srcInput.interleaved];
		auto& dst = buffers[input.interleaved];
		dst.insert( dst.end(), src.begin(), src.end() );
	// both meshes are de-interleaved, just copy directly
	} else if ( !isInterleaved(input.interleaved) && !isInterleaved(srcInput.interleaved) ) {
		for ( auto i = 0; i < input.attributes.size(); ++i ) {
			auto& srcAttribute = srcInput.attributes[i];
			auto& dstAttribute = input.attributes[i];
			auto& src = mesh.buffers[srcAttribute.buffer];
			auto& dst = buffers[dstAttribute.buffer];
			dst.insert( dst.end(), src.begin(), src.end() );
		}
	// not easy to convert, will implement later
	} else {
		UF_EXCEPTION("to be implemented");
	}
	_updateDescriptor( input );
}
void uf::Mesh::_insertI( uf::Mesh::Input& input, const uf::Mesh& mesh, const uf::Mesh::Input& srcInput ) {
//	if ( !_hasI( source ) ) return;
	_reserveIs( input, input.count += srcInput.count );

	// both meshes are interleaved, just copy directly
	if ( isInterleaved(input.interleaved) && isInterleaved(srcInput.interleaved) ) {
		auto& src = mesh.buffers[srcInput.interleaved];
		auto& dst = buffers[input.interleaved];
		dst.insert( dst.end(), src.begin(), src.end() );
	// both meshes are de-interleaved, just copy directly
	} else if ( !isInterleaved(input.interleaved) && !isInterleaved(srcInput.interleaved) ) {
		for ( auto i = 0; i < input.attributes.size(); ++i ) {
			auto& srcAttribute = srcInput.attributes[i];
			auto& dstAttribute = input.attributes[i];
			auto& src = mesh.buffers[srcAttribute.buffer];
			auto& dst = buffers[dstAttribute.buffer];
			dst.insert( dst.end(), src.begin(), src.end() );
		}
	// not easy to convert, will implement later
	} else {
		UF_EXCEPTION("to be implemented");
	}
	_updateDescriptor( input );
}
// Vertices
bool uf::Mesh::_hasV( const uf::Mesh::Input& input, const uf::stl::vector<ext::RENDERER::AttributeDescriptor>& descriptors ) const {
	if ( input.attributes.size() != descriptors.size() ) return false;
	for ( auto i = 0; i < input.attributes.size(); ++i ) if ( input.attributes[i].descriptor != descriptors[i] ) return false;
	return true;
}
bool uf::Mesh::_hasV( const uf::Mesh::Input& input, const uf::Mesh::Input& srcInput ) const {
	if ( input.attributes.size() != srcInput.attributes.size() || input.stride != srcInput.stride ) return false;
	for ( auto i = 0; i < input.attributes.size(); ++i ) if ( input.attributes[i].descriptor != srcInput.attributes[i].descriptor ) return false;
	return true;
}
void uf::Mesh::_bindV( uf::Mesh::Input& input, const uf::stl::vector<uf::renderer::AttributeDescriptor>& descriptors ) {
	input.attributes.resize( descriptors.size() );
	for ( auto i = 0; i < descriptors.size(); ++i ) {
		auto& attribute = input.attributes[i];
		attribute.descriptor = descriptors[i];
		input.stride += attribute.descriptor.size;
	}
}
void uf::Mesh::_reserveVs( uf::Mesh::Input& input, size_t count ) {
	if ( isInterleaved(input.interleaved) ) {
		buffers[input.interleaved].reserve( count * input.stride );
		for ( auto& attribute : input.attributes ) {
			attribute.descriptor.length = buffers[input.interleaved].size();
			attribute.descriptor.pointer = (void*) (buffers[input.interleaved].data());
		}
	} else for ( auto& attribute : input.attributes ) {
		buffers[attribute.buffer].reserve( count * attribute.descriptor.size );
		attribute.descriptor.length = buffers[attribute.buffer].size();
		attribute.descriptor.pointer = (void*) (buffers[attribute.buffer].data());
	}
}
void uf::Mesh::_resizeVs( uf::Mesh::Input& input, size_t count ) {
	if ( isInterleaved(input.interleaved) ) {
		buffers[input.interleaved].resize( count * input.stride );
		for ( auto& attribute : input.attributes ) {
			attribute.descriptor.length = buffers[input.interleaved].size();
			attribute.descriptor.pointer = (void*) (buffers[input.interleaved].data());
		}
	} else for ( auto& attribute : input.attributes ) {
		buffers[attribute.buffer].resize( count * attribute.descriptor.size );
		attribute.descriptor.length = buffers[attribute.buffer].size();
		attribute.descriptor.pointer = (void*) (buffers[attribute.buffer].data());
	}
}
void uf::Mesh::_insertV( uf::Mesh::Input& input, const void* data ) {
	_reserveVs( input, ++input.count );
	const uint8_t* pointer = (const uint8_t*) data;
	if ( isInterleaved(input.interleaved) ) {
		buffers[input.interleaved].insert( buffers[input.interleaved].end(), pointer, pointer + input.stride );
	} else for ( auto& attribute : input.attributes ) {
		buffers[attribute.buffer].insert( buffers[attribute.buffer].end(), pointer + attribute.descriptor.offset, pointer + attribute.descriptor.offset + attribute.descriptor.size );
	}
}
void uf::Mesh::_insertVs( uf::Mesh::Input& input, const void* data, size_t size ) {
#if 0
	const uint8_t* pointer = (const uint8_t*) data;
	for ( auto i = 0; i < size; ++i ) insertV( pointer + i * input.stride  );
#else
	_reserveVs( input, input.count += size );
	const uint8_t* pointer = (const uint8_t*) data;
	if ( isInterleaved(input.interleaved) ) {
		buffers[input.interleaved].insert( buffers[input.interleaved].end(), pointer, pointer + size * input.stride );
	} else for ( const uint8_t* p = pointer; p < pointer + size * input.stride; p += input.stride ) {
		for ( auto& attribute : input.attributes )
			buffers[attribute.buffer].insert( buffers[attribute.buffer].end(), p + attribute.descriptor.offset, p + attribute.descriptor.offset + attribute.descriptor.size );
	}
#endif
}
// Indices
void uf::Mesh::_bindI( uf::Mesh::Input& input, size_t size, ext::RENDERER::enums::Type::type_t type, size_t count ) {
	input.attributes.resize( count );
	input.stride = size;
	for ( auto i = 0; i < count; ++i ) {
		auto& attribute = input.attributes[i];
		attribute.descriptor.offset = 0;
		attribute.descriptor.size = size;
		attribute.descriptor.components = 1;
		attribute.descriptor.type = type;
	}
}
void uf::Mesh::_reserveIs( uf::Mesh::Input& input, size_t count, size_t i ) {
	auto& attribute = input.attributes[i];
	buffers[attribute.buffer].reserve( count * attribute.descriptor.size );
	attribute.descriptor.length = buffers[attribute.buffer].size();
	attribute.descriptor.pointer = (void*) (buffers[attribute.buffer].data());
}
void uf::Mesh::_resizeIs( uf::Mesh::Input& input, size_t count, size_t i ) {
	auto& attribute = input.attributes[i];
	buffers[attribute.buffer].resize( count * attribute.descriptor.size );
	attribute.descriptor.length = buffers[attribute.buffer].size();
	attribute.descriptor.pointer = (void*) (buffers[attribute.buffer].data());
}
void uf::Mesh::_insertI( uf::Mesh::Input& input, const void* data, size_t i ) {			
	auto& attribute = input.attributes[i];
	_reserveIs( input, ++input.count );
	const uint8_t* pointer = (const uint8_t*) data;
#if 1
	buffers[attribute.buffer].insert( buffers[attribute.buffer].end(), pointer, pointer + attribute.descriptor.size );
#else
	if ( isInterleaved(input.interleaved) ) {
		buffers[input.interleaved].insert( buffers[input.interleaved].end(), pointer, pointer + attribute.descriptor.size );
	} else {
		buffers[attribute.buffer].insert( buffers[attribute.buffer].end(), pointer, pointer + attribute.descriptor.size );
	}
#endif
}
void uf::Mesh::_insertIs( uf::Mesh::Input& input, const void* data, size_t size, size_t i ) {
	auto& attribute = input.attributes[i];
	_reserveIs( input, input.count += size );
	const uint8_t* pointer = (const uint8_t*) data;
#if 1
	for ( const uint8_t* p = pointer; p < pointer + size * attribute.descriptor.size; p += attribute.descriptor.size )
		buffers[attribute.buffer].insert( buffers[attribute.buffer].end(), p + attribute.descriptor.offset, p + attribute.descriptor.offset + attribute.descriptor.size );
#else
	if ( isInterleaved(input.interleaved) ) {
		buffers[input.interleaved].insert( buffers[input.interleaved].end(), pointer, pointer + size * attribute.descriptor.size );
	} else for ( const uint8_t* p = pointer; p < pointer + size * attribute.descriptor.size; p += attribute.descriptor.size ) {
		buffers[attribute.buffer].insert( buffers[attribute.buffer].end(), p + attribute.descriptor.offset, p + attribute.descriptor.offset + attribute.descriptor.size );
	}
#endif
}