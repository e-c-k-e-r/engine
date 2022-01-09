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
void uf::Mesh::initialize() {}
void uf::Mesh::destroy() {
	_destroy(vertex);
	_destroy(index);
	_destroy(instance);
	_destroy(indirect);

	buffers.clear();
}
// implicitly convert to opposite interleaving
uf::Mesh uf::Mesh::convert() const {
	return copy( !isInterleaved() );
}
uf::Mesh uf::Mesh::copy( bool interleaved ) const {
	uf::Mesh res;

	res.bind( *this, interleaved );
	res.insert(*this);
	res.updateDescriptor();

	return res;
}
void uf::Mesh::updateDescriptor() {
	_updateDescriptor(vertex);
	_updateDescriptor(index);
	_updateDescriptor(instance);
	_updateDescriptor(indirect);
}
void uf::Mesh::bind( const uf::Mesh& mesh ) { return bind( mesh, isInterleaved() ); }
void uf::Mesh::bind( const uf::Mesh& mesh, bool interleaved ) {
	vertex.attributes = mesh.vertex.attributes;
	index.attributes = mesh.index.attributes;
	instance.attributes = mesh.instance.attributes;
	indirect.attributes = mesh.indirect.attributes;

	_bind( interleaved );
}
void uf::Mesh::insert( const uf::Mesh& mesh ) {
	if ( vertex.attributes.empty() && index.attributes.empty() && instance.attributes.empty() && indirect.attributes.empty() ) bind( mesh );
	
	insertVertices(mesh);
	insertIndices(mesh);
	insertInstances(mesh);
	insertIndirects(mesh);

	updateDescriptor();
}
void uf::Mesh::generateIndices() {
	// deduce type
	size_t size = sizeof(uint32_t);
	uf::renderer::enums::Type::type_t type{};
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
uf::Mesh uf::Mesh::expand() { return expand( isInterleaved() ); }
uf::Mesh uf::Mesh::expand( bool interleaved ) {
	uf::Mesh res = copy( interleaved );

	res.resizeVertices( index.count );
	res.vertex.count = index.count;


	auto& srcIndex = index.attributes.front();
	auto& dstIndex = res.index.attributes.front();

#define GET_INDEX(T) {\
	index = *(const T*) (srcIndex.pointer + idx * srcIndex.stride);\
	*((T*) (dstIndex.pointer + idx * dstIndex.stride)) = idx;\
}

	for ( size_t idx = 0; idx < index.count; ++idx ) {
		size_t index = 0;
		switch ( srcIndex.descriptor.size ) {
			case 1: GET_INDEX(uint8_t); break;
			case 2: GET_INDEX(uint16_t); break;
			case 4: GET_INDEX(uint32_t); break;
		}

		for ( size_t _ = 0; _ < vertex.attributes.size(); ++_ ) {
			auto& srcInput = vertex.attributes[_];
			auto& dstInput = res.vertex.attributes[_];

			memcpy( dstInput.pointer, srcInput.pointer + index * srcInput.stride, srcInput.descriptor.size );
			dstInput.pointer += dstInput.stride;
		}
	}

#undef GET_INDEX

	if ( res.indirect.count ) {
		pod::DrawCommand* drawCommands = (pod::DrawCommand*) res.getBuffer(res.indirect).data();
		for ( size_t i = 0, vertexID = 0; i < res.indirect.count; ++i ) {
			auto& drawCommand = drawCommands[i];
			drawCommand.vertexID = vertexID;
			drawCommand.vertices = drawCommand.indices;
			vertexID += drawCommand.indices;
		}
	}

	res.updateDescriptor();

	return res;
}
uf::Mesh::Input uf::Mesh::remapInput( const uf::Mesh::Input& input, size_t i ) const {
	uf::Mesh::Input res = input;
	UF_ASSERT( &input == &vertex || &input == &index );
	UF_ASSERT( i < indirect.count );
	
	const auto& drawCommand = ((const pod::DrawCommand*) getBuffer(indirect).data())[i];
	res.first = &input == &vertex ? drawCommand.vertexID : drawCommand.indexID;
	res.count = &input == &vertex ? drawCommand.vertices : drawCommand.indices;	

	return res;
}
uf::Mesh::Input uf::Mesh::remapVertexInput( size_t i ) const {
	uf::Mesh::Input res = vertex;
	UF_ASSERT( i < indirect.count );

	const auto& drawCommand = ((const pod::DrawCommand*) getBuffer(indirect).data())[i];
	res.first = drawCommand.vertexID;
	res.count = drawCommand.vertices;

	return res;
}
uf::Mesh::Input uf::Mesh::remapIndexInput( size_t i ) const {
	uf::Mesh::Input res = index;
	UF_ASSERT( i < indirect.count );

	const auto& drawCommand = ((const pod::DrawCommand*) getBuffer(indirect).data())[i];
	res.first = drawCommand.indexID;
	res.count = drawCommand.indices;

	return res;
}
void uf::Mesh::generateIndirect() {
	if ( index.count == 0 ) generateIndices();

	uf::stl::vector<pod::DrawCommand> commands;
	for ( auto& attribute : index.attributes ) {
		auto& buffer = getBuffer(index, attribute);
		commands.emplace_back(pod::DrawCommand{
			.indices = buffer.size() / attribute.descriptor.size,
			.instances = instance.count == 0 && instance.attributes.empty() ? 1 : instance.count,
			.indexID = 0,
			.vertexID = 0,
			.instanceID = 0,
		//	.materialID = 0,
		//	.objectID = 0,
		//	.vertices = vertex.count,
		});
	}

	_destroy( indirect );
	bindIndirect<pod::DrawCommand>();
	_bind();
	insertIndirects( commands );
}
bool uf::Mesh::isInterleaved() const { return isInterleaved( vertex.interleaved ); }
bool uf::Mesh::isInterleaved( size_t i ) const { return 0 <= i && i < buffers.size(); }
uf::Mesh::buffer_t& uf::Mesh::getBuffer( const uf::Mesh::Input& input, size_t i ) {
	return getBuffer( input, input.attributes[i] );
}
uf::Mesh::buffer_t& uf::Mesh::getBuffer( const uf::Mesh::Input& input, const uf::Mesh::Attribute& attribute ) {
	return buffers[isInterleaved(input.interleaved) ? input.interleaved : attribute.buffer];
}
const uf::Mesh::buffer_t& uf::Mesh::getBuffer( const uf::Mesh::Input& input, size_t i ) const {
	return getBuffer( input, input.attributes[i] );
}
const uf::Mesh::buffer_t& uf::Mesh::getBuffer( const uf::Mesh::Input& input, const uf::Mesh::Attribute& attribute ) const {
	return buffers[isInterleaved(input.interleaved) ? input.interleaved : attribute.buffer];
}
void uf::Mesh::print( bool full ) const {
	std::cout << "Buffers: " << buffers.size() << "\n" << printVertices(full) << printIndices(full) << printInstances(full) << printIndirects() << std::endl;
}


#define PRINT_HEADER(input) "Count: " << input.count << " | First: " << input.first << " | Size: " << input.size << " | Offset: " << input.offset << " | " << (isInterleaved(input.interleaved) ? "interleaved" : "deinterleaved") << "\n"

std::string uf::Mesh::printVertices( bool full ) const {
	std::stringstream str;
	str << "Vertices: " << PRINT_HEADER( vertex );
	if ( full ) for ( auto i = 0; i < vertex.count; ++i ) {
		for ( auto& attribute : vertex.attributes ) {
			str << "[" << i << "][" << attribute.descriptor.name << "]: ( ";
			uint8_t* e = (uint8_t*) attribute.pointer + i * attribute.stride;
			switch ( attribute.descriptor.type ) {
				case uf::renderer::enums::Type::UINT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << (int) ((uint32_t*) e)[j] << " "; break;
				case uf::renderer::enums::Type::INT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << (int) ((int32_t*) e)[j] << " "; break;
				case uf::renderer::enums::Type::USHORT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << (int) ((uint16_t*) e)[j] << " "; break;
				case uf::renderer::enums::Type::SHORT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << (int) ((int16_t*) e)[j] << " "; break;
				case uf::renderer::enums::Type::UBYTE: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << (int) ((uint8_t*) e)[j] << " "; break;
				case uf::renderer::enums::Type::BYTE: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << (int) ((int8_t*) e)[j] << " "; break;
				case uf::renderer::enums::Type::FLOAT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((float*) e)[j] << " "; break;
				default: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((float*) e)[j] << " "; break;
			}
			str << ")\n";
		}
	}
	return str.str();
}
std::string uf::Mesh::printIndices( bool full ) const {
	std::stringstream str;
	str << "Indices: " << PRINT_HEADER( index );
	if ( full ) for ( auto i = 0; i < index.count; ++i ) {
		auto& buffer = getBuffer( index );
		switch ( index.size ) {
			case 1: str << "[" << i << "]: " << *(( uint8_t*) &buffer[i * index.size]) << "\n"; break;
			case 2: str << "[" << i << "]: " << *((uint16_t*) &buffer[i * index.size]) << "\n"; break;
			case 4: str << "[" << i << "]: " << *((uint32_t*) &buffer[i * index.size]) << "\n"; break;
		}
	}
	return str.str();
}
std::string uf::Mesh::printInstances( bool full ) const {
	std::stringstream str;
	str << "Instances: " << PRINT_HEADER( instance );
	if ( full ) for ( auto i = 0; i < instance.count; ++i ) {
		for ( auto& attribute : vertex.attributes ) {
			str << "[" << i << "][" << attribute.descriptor.name << "]: ( ";
			uint8_t* e = (uint8_t*) attribute.pointer + i * attribute.stride;
			switch ( attribute.descriptor.type ) {
				case uf::renderer::enums::Type::UINT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << (int) ((uint32_t*) e)[j] << " "; break;
				case uf::renderer::enums::Type::INT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << (int) ((int32_t*) e)[j] << " "; break;
				case uf::renderer::enums::Type::USHORT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << (int) ((uint16_t*) e)[j] << " "; break;
				case uf::renderer::enums::Type::SHORT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << (int) ((int16_t*) e)[j] << " "; break;
				case uf::renderer::enums::Type::UBYTE: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << (int) ((uint8_t*) e)[j] << " "; break;
				case uf::renderer::enums::Type::BYTE: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << (int) ((int8_t*) e)[j] << " "; break;
				case uf::renderer::enums::Type::FLOAT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((float*) e)[j] << " "; break;
				default: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((float*) e)[j] << " "; break;
			}
			str << ")\n";
		}
	}
	return str.str();
}
std::string uf::Mesh::printIndirects( bool full ) const {
	std::stringstream str;
	str << "Indirect: " << PRINT_HEADER( indirect ) << "{ indices, instances, indexID, vertexID, instanceID, padding1, padding2, vertices }\n";
	if ( full ) for ( auto i = 0; i < indirect.count; ++i ) {
		auto& buffer = getBuffer( indirect );
		auto& drawCommand = *(const pod::DrawCommand*) (&buffer[i * indirect.size]);
		str << "[" << i << "]: {" << drawCommand.indices << ", " << drawCommand.instances << ", " << drawCommand.indexID << ", " << drawCommand.vertexID << ", " << drawCommand.instanceID << ", " << drawCommand.padding1 << ", " << drawCommand.padding2 << ", " << drawCommand.vertices << "}\n";
	}
	return str.str();
}
//
void uf::Mesh::_destroy( uf::Mesh::Input& input ) {
	for ( auto& attribute : input.attributes ) {
		attribute.length = 0;
		attribute.pointer = NULL;
		attribute.buffer = 0;
	}
	input.attributes.clear();
}
void uf::Mesh::_bind( bool interleave ) {
	int32_t buffer = 0;
#define PARSE_INPUT(INPUT, INTERLEAVED){\
	INPUT.interleaved = (INTERLEAVED ? buffer : -1);\
	for ( auto i = 0; i < INPUT.attributes.size(); ++i ) {\
		INPUT.attributes[i].buffer = !INTERLEAVED ? buffer++ : buffer;\
		INPUT.attributes[i].pointer = NULL;\
	}\
	if ( !INPUT.attributes.empty() && INTERLEAVED ) ++buffer;\
}
	
	PARSE_INPUT(vertex, interleave)
	PARSE_INPUT(index, false)
	PARSE_INPUT(instance, interleave)
	PARSE_INPUT(indirect, false)

	buffers.resize( buffer );
	updateDescriptor();

#undef PARSE_INPUT
}
void uf::Mesh::_updateDescriptor( uf::Mesh::Input& input ) {
	input.size = 0;
	for ( auto& attribute : input.attributes ) {
		const bool interleaved = isInterleaved(input.interleaved);
		auto& buffer = buffers[interleaved ? input.interleaved : attribute.buffer];
		attribute.length = buffer.size();
		attribute.pointer = buffer.data() + attribute.offset;
		input.size += attribute.descriptor.size;
		if ( interleaved ) attribute.pointer += attribute.descriptor.offset;
	}
	for ( auto& attribute : input.attributes ) {
		attribute.stride = isInterleaved(input.interleaved) ? input.size : attribute.descriptor.size;
	}
}
uf::Mesh::Attribute uf::Mesh::_remapAttribute( const uf::Mesh::Input& input, const uf::Mesh::Attribute& attribute, size_t i ) const {
	uf::Mesh::Attribute res = attribute;
	UF_ASSERT( i < indirect.count );
	UF_ASSERT( &input == &vertex || &input == &index );

	auto& drawCommand = ((const pod::DrawCommand*) getBuffer(indirect).data())[i];
	if ( &input == &vertex ) {
		res.pointer += drawCommand.vertexID * res.stride;
		res.length = drawCommand.vertices * res.stride;
	} else if ( &input == &index ) {
		res.pointer += drawCommand.indexID * res.stride;
		res.length = drawCommand.indices * res.stride;
	}
	return res;
}
void uf::Mesh::_insertVs( uf::Mesh::Input& dstInput, const uf::Mesh& mesh, const uf::Mesh::Input& srcInput ) {
	_reserveVs( dstInput, dstInput.count += srcInput.count );

	// both meshes are interleaved, just copy directly
	if ( isInterleaved(dstInput.interleaved) && isInterleaved(srcInput.interleaved) ) {
		if ( !_hasV( dstInput, srcInput ) ) return;
		auto& src = mesh.buffers[srcInput.interleaved];
		auto& dst = buffers[dstInput.interleaved];
		dst.insert( dst.end(), src.begin(), src.end() );
	// both meshes are de-interleaved, just copy directly
	} else if ( !isInterleaved(dstInput.interleaved) && !isInterleaved(srcInput.interleaved) ) {
		if ( _hasV( dstInput, srcInput ) ) {	
			for ( auto i = 0; i < dstInput.attributes.size(); ++i ) {
				auto& srcAttribute = srcInput.attributes[i];
				auto& dstAttribute = dstInput.attributes[i];
				auto& src = mesh.buffers[srcAttribute.buffer];
				auto& dst = buffers[dstAttribute.buffer];
				dst.insert( dst.end(), src.begin(), src.end() );
			}
		} else {
			for ( auto& dstAttribute : dstInput.attributes ) {
				for ( auto& srcAttribute : srcInput.attributes ) {
					if ( srcAttribute.descriptor != dstAttribute.descriptor ) continue;

					auto& src = mesh.buffers[srcAttribute.buffer];
					auto& dst = buffers[dstAttribute.buffer];
					dst.insert( dst.end(), src.begin(), src.end() );

					break;
				}
			}
		}
	// not easy to convert, will implement later
	} else if ( isInterleaved(dstInput.interleaved) && !isInterleaved(srcInput.interleaved) ) {
	//	UF_EXCEPTION("to be implemented: deinterleaved -> interleaved");
		uf::Mesh::Input _srcInput = srcInput;
		auto& dst = buffers.at(dstInput.interleaved);
		size_t _ = 0;
		while ( _++ < _srcInput.count ) {
			for ( auto& srcAttribute : _srcInput.attributes ) {
				dst.insert( dst.end(), (uint8_t*) srcAttribute.pointer, (uint8_t*) srcAttribute.pointer + srcAttribute.descriptor.size );
				srcAttribute.pointer += srcAttribute.descriptor.size;
			}
		}
	} else if ( !isInterleaved(dstInput.interleaved) && isInterleaved(srcInput.interleaved) ) {
	//	UF_EXCEPTION("to be implemented: interleaved -> deinterleaved");
		uf::Mesh::Input _srcInput = srcInput;
		const uint8_t* src = (const uint8_t*) mesh.buffers.at(srcInput.interleaved).data();
		size_t _ = 0;
		while ( _++ < _srcInput.count ) {
			for ( size_t i = 0; i < dstInput.attributes.size(); ++i ) {
				auto& srcAttribute = _srcInput.attributes.at(i);
				auto& dstAttribute = dstInput.attributes.at(i);
				
				auto& dst = buffers.at(dstAttribute.buffer);
				dst.insert( dst.end(), src, src + srcAttribute.descriptor.size );
				src += srcAttribute.descriptor.size;
			}
		}
	} else {
		UF_EXCEPTION("to be implemented: ??");
	}
	_updateDescriptor( dstInput );
}
void uf::Mesh::_insertIs( uf::Mesh::Input& dstInput, const uf::Mesh& mesh, const uf::Mesh::Input& srcInput ) {
//	if ( !_hasI( source ) ) return;
	_reserveIs( dstInput, dstInput.count += srcInput.count );

	// both meshes are interleaved, just copy directly
	if ( isInterleaved(dstInput.interleaved) && isInterleaved(srcInput.interleaved) ) {
		auto& src = mesh.getBuffer( srcInput );
		auto& dst = getBuffer( dstInput );

		dst.insert( dst.end(), src.begin(), src.end() );
	// both meshes are de-interleaved, just copy directly
	} else if ( !isInterleaved(dstInput.interleaved) && !isInterleaved(srcInput.interleaved) ) {
		for ( auto i = 0; i < dstInput.attributes.size(); ++i ) {
			auto& src = mesh.getBuffer( srcInput );
			auto& dst = getBuffer( dstInput );

			dst.insert( dst.end(), src.begin(), src.end() );
		}
	// not easy to convert, will implement later
	} else if ( isInterleaved(dstInput.interleaved) && !isInterleaved(srcInput.interleaved) ) {
	//	UF_EXCEPTION("to be implemented: deinterleaved -> interleaved");
		uf::Mesh::Input _srcInput = srcInput;
		auto& dst = getBuffer( dstInput );
		size_t _ = 0;
		while ( _++ < _srcInput.count ) {
			for ( auto& srcAttribute : _srcInput.attributes ) {
				dst.insert( dst.end(), (uint8_t*) srcAttribute.pointer, (uint8_t*) srcAttribute.pointer + srcAttribute.descriptor.size );
				srcAttribute.pointer += srcAttribute.descriptor.size;
			}
		}
	} else if ( !isInterleaved(dstInput.interleaved) && isInterleaved(srcInput.interleaved) ) {
	//	UF_EXCEPTION("to be implemented: interleaved -> deinterleaved");
		uf::Mesh::Input _srcInput = srcInput;
		const uint8_t* src = (const uint8_t*) mesh.getBuffer( srcInput ).data();
		size_t _ = 0;
		while ( _++ < _srcInput.count ) {
			for ( size_t i = 0; i < dstInput.attributes.size(); ++i ) {
				auto& srcAttribute = _srcInput.attributes[i];
				auto& dstAttribute = dstInput.attributes[i];
				
				auto& dst = buffers.at(dstAttribute.buffer);
				dst.insert( dst.end(), src, src + srcAttribute.descriptor.size );
				src += srcAttribute.descriptor.size;
			}
		}
	} else {
		UF_EXCEPTION("to be implemented: ??");
	}
	_updateDescriptor( dstInput );
}
// Vertices
bool uf::Mesh::_hasV( const uf::Mesh::Input& input, const uf::stl::vector<ext::RENDERER::AttributeDescriptor>& descriptors ) const {
	if ( input.attributes.size() != descriptors.size() ) return false;
	for ( auto i = 0; i < input.attributes.size(); ++i ) if ( input.attributes[i].descriptor != descriptors[i] ) return false;
	return true;
}
bool uf::Mesh::_hasV( const uf::Mesh::Input& input, const uf::Mesh::Input& srcInput ) const {
	if ( input.attributes.size() != srcInput.attributes.size() || input.size != srcInput.size ) return false;
	for ( auto i = 0; i < input.attributes.size(); ++i ) if ( input.attributes[i].descriptor != srcInput.attributes[i].descriptor ) return false;
	return true;
}
void uf::Mesh::_bindV( uf::Mesh::Input& input, const uf::stl::vector<uf::renderer::AttributeDescriptor>& descriptors ) {
	input.attributes.resize( descriptors.size() );
	for ( auto i = 0; i < descriptors.size(); ++i ) {
		auto& attribute = input.attributes[i];
		attribute.descriptor = descriptors[i];
		input.size += attribute.descriptor.size;
	}
}
void uf::Mesh::_reserveVs( uf::Mesh::Input& input, size_t count ) {
	if ( isInterleaved(input.interleaved) ) {
		buffers[input.interleaved].reserve( count * input.size );
		for ( auto& attribute : input.attributes ) {
			attribute.length = buffers[input.interleaved].size();
			attribute.pointer = (void*) (buffers[input.interleaved].data());
		}
	} else for ( auto& attribute : input.attributes ) {
		buffers[attribute.buffer].reserve( count * attribute.descriptor.size );
		attribute.length = buffers[attribute.buffer].size();
		attribute.pointer = (void*) (buffers[attribute.buffer].data());
	}
}
void uf::Mesh::_resizeVs( uf::Mesh::Input& input, size_t count ) {
	if ( isInterleaved(input.interleaved) ) {
		buffers[input.interleaved].resize( count * input.size );
		for ( auto& attribute : input.attributes ) {
			attribute.length = buffers[input.interleaved].size();
			attribute.pointer = (void*) (buffers[input.interleaved].data());
		}
	} else for ( auto& attribute : input.attributes ) {
		buffers[attribute.buffer].resize( count * attribute.descriptor.size );
		attribute.length = buffers[attribute.buffer].size();
		attribute.pointer = (void*) (buffers[attribute.buffer].data());
	}
}
void uf::Mesh::_insertV( uf::Mesh::Input& input, const void* data ) {
	_reserveVs( input, ++input.count );
	const uint8_t* pointer = (const uint8_t*) data;
	if ( isInterleaved(input.interleaved) ) {
		buffers[input.interleaved].insert( buffers[input.interleaved].end(), pointer, pointer + input.size );
	} else for ( auto& attribute : input.attributes ) {
		buffers[attribute.buffer].insert( buffers[attribute.buffer].end(), pointer + attribute.descriptor.offset, pointer + attribute.descriptor.offset + attribute.descriptor.size );
	}
}
void uf::Mesh::_insertVs( uf::Mesh::Input& input, const void* data, size_t size ) {
#if 0
	const uint8_t* pointer = (const uint8_t*) data;
	for ( auto i = 0; i < size; ++i ) insertV( pointer + i * input.size  );
#else
	_reserveVs( input, input.count += size );
	const uint8_t* pointer = (const uint8_t*) data;
	if ( isInterleaved(input.interleaved) ) {
		buffers[input.interleaved].insert( buffers[input.interleaved].end(), pointer, pointer + size * input.size );
	} else for ( const uint8_t* p = pointer; p < pointer + size * input.size; p += input.size ) {
		for ( auto& attribute : input.attributes )
			buffers[attribute.buffer].insert( buffers[attribute.buffer].end(), p + attribute.descriptor.offset, p + attribute.descriptor.offset + attribute.descriptor.size );
	}
#endif
}
// Indices
void uf::Mesh::_bindI( uf::Mesh::Input& input, size_t size, ext::RENDERER::enums::Type::type_t type, size_t count ) {
	input.attributes.resize( count );
	input.size = size;
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
	attribute.length = buffers[attribute.buffer].size();
	attribute.pointer = (void*) (buffers[attribute.buffer].data());
}
void uf::Mesh::_resizeIs( uf::Mesh::Input& input, size_t count, size_t i ) {
	auto& attribute = input.attributes[i];
	buffers[attribute.buffer].resize( count * attribute.descriptor.size );
	attribute.length = buffers[attribute.buffer].size();
	attribute.pointer = (void*) (buffers[attribute.buffer].data());
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