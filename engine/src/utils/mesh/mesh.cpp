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
uf::Mesh uf::Mesh::convert( bool interleave ) const {
	uf::Mesh res;

	// overkill but I don't want to rewrite a bind<>();
	res.vertex.attributes = vertex.attributes;
	res.vertex.count = 0;
	res.vertex.stride = vertex.stride;
	res.vertex.offset = vertex.offset;

	res.index.attributes = index.attributes;
	res.index.count = 0;
	res.index.interleaved = -1;
	res.index.stride = index.stride;
	res.index.offset = index.offset;

	res.instance.attributes = instance.attributes;
	res.instance.count = 0;
	res.index.interleaved = -1;
	res.instance.stride = instance.stride;
	res.instance.offset = instance.offset;
	res.indirect.attributes = indirect.attributes;

	res.indirect.count = 0;
	res.indirect.interleaved = -1;
	res.indirect.stride = indirect.stride;
	res.indirect.offset = indirect.offset;
	res._bind( interleave );

	res.insertVertices(*this);
	res.insertIndices(*this);
	res.insertInstances(*this);
	res.insertIndirects(*this);

	res.updateDescriptor();

	return res;
}
void uf::Mesh::updateDescriptor() {
	_updateDescriptor(vertex);
	_updateDescriptor(index);
	_updateDescriptor(instance);
	_updateDescriptor(indirect);
}
void uf::Mesh::bind( const uf::Mesh& mesh ) {
	vertex.attributes = mesh.vertex.attributes;
	vertex.interleaved = mesh.vertex.interleaved;
	index.attributes = mesh.index.attributes;
	index.interleaved = mesh.index.interleaved;
	instance.attributes = mesh.instance.attributes;
	instance.interleaved = mesh.instance.interleaved;
	indirect.attributes = mesh.indirect.attributes;
	indirect.interleaved = mesh.indirect.interleaved;

	_bind();
}
void uf::Mesh::insert( const uf::Mesh& mesh ) {
	if ( vertex.attributes.empty() && index.attributes.empty() && instance.attributes.empty() && indirect.attributes.empty() ) bind( mesh );
	
	insertVertices(mesh);
	insertIndices(mesh);
	insertInstances(mesh);
	insertIndirects(mesh);
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
void uf::Mesh::generateIndirect() {
	if ( index.count == 0 ) generateIndices();

	uf::stl::vector<pod::DrawCommand> commands;
	for ( auto& attribute : index.attributes ) {
		auto& buffer = buffers[isInterleaved(index.interleaved) ? index.interleaved : attribute.buffer];
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
					case uf::renderer::enums::Type::UINT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << (int) ((uint32_t*) (e + attribute.descriptor.offset))[j] << " "; break;
					case uf::renderer::enums::Type::INT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << (int) ((int32_t*) (e + attribute.descriptor.offset))[j] << " "; break;
					case uf::renderer::enums::Type::USHORT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << (int) ((uint16_t*) (e + attribute.descriptor.offset))[j] << " "; break;
					case uf::renderer::enums::Type::SHORT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << (int) ((int16_t*) (e + attribute.descriptor.offset))[j] << " "; break;
					case uf::renderer::enums::Type::UBYTE: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << (int) ((uint8_t*) (e + attribute.descriptor.offset))[j] << " "; break;
					case uf::renderer::enums::Type::BYTE: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << (int) ((int8_t*) (e + attribute.descriptor.offset))[j] << " "; break;
				}
				str << ")\n";
			}
		} else for ( auto& attribute : vertex.attributes ) {
			auto& buffer = buffers[attribute.buffer];
			str << "[" << i << "][" << attribute.descriptor.name << "]: ( ";
			switch ( attribute.descriptor.type ) {
				case uf::renderer::enums::Type::FLOAT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << ((float*) &buffer[i * attribute.descriptor.size])[j] << " "; break;
				case uf::renderer::enums::Type::UINT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << (int) ((uint32_t*) &buffer[i * attribute.descriptor.size])[j] << " "; break;
				case uf::renderer::enums::Type::INT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << (int) ((int32_t*) &buffer[i * attribute.descriptor.size])[j] << " "; break;
				case uf::renderer::enums::Type::USHORT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << (int) ((uint16_t*) &buffer[i * attribute.descriptor.size])[j] << " "; break;
				case uf::renderer::enums::Type::SHORT: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << (int) ((int16_t*) &buffer[i * attribute.descriptor.size])[j] << " "; break;
				case uf::renderer::enums::Type::UBYTE: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << (int) ((uint8_t*) &buffer[i * attribute.descriptor.size])[j] << " "; break;
				case uf::renderer::enums::Type::BYTE: for ( auto j = 0; j < attribute.descriptor.components; ++j ) str << (int) ((int8_t*) &buffer[i * attribute.descriptor.size])[j] << " "; break;
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
		attribute.length = 0;
		attribute.pointer = NULL;
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
//	PARSE_INPUT(instance, interleave)
	PARSE_INPUT(instance, false)
	PARSE_INPUT(indirect, false)

	buffers.resize( buffer );
	updateDescriptor();

//	for ( auto& attribute : vertex.attributes ) UF_MSG_DEBUG( attribute.descriptor.name << "[" << attribute.descriptor.offset << "]: " << attribute.descriptor.size );
#undef PARSE_INPUT
}
void uf::Mesh::_updateDescriptor( uf::Mesh::Input& input ) {
	input.stride = 0;
	for ( auto& attribute : input.attributes ) {
		const bool interleaved = isInterleaved(input.interleaved);
		auto& buffer = buffers[interleaved ? input.interleaved : attribute.buffer];
		attribute.length = buffer.size();
		attribute.pointer = (void*) (buffer.data() + attribute.offset);
		if ( !interleaved ) attribute.stride = attribute.descriptor.size;
		else attribute.pointer += attribute.descriptor.offset;
		input.stride += attribute.descriptor.size;
	}
	for ( auto& attribute : input.attributes ) {
		const bool interleaved = isInterleaved(input.interleaved);
		if ( interleaved ) attribute.stride = input.stride;
	}
}
uf::Mesh::Attribute uf::Mesh::_remapAttribute( const uf::Mesh::Input& input, const uf::Mesh::Attribute& attribute, size_t i ) const {
	uf::Mesh::Attribute res = attribute;
	if ( i < indirect.count ) {
		auto& drawCommand = ((const pod::DrawCommand*) buffers[isInterleaved(indirect.interleaved) ? indirect.interleaved : indirect.attributes.front().buffer].data())[i];
		if ( &input == &vertex ) {
			res.pointer += drawCommand.vertexID * res.stride;
			res.length = drawCommand.vertices * res.stride;
		} else if ( &input == &index ) {
			res.pointer += drawCommand.indexID * res.stride;
			res.length = drawCommand.indices * res.stride;
		}
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
		uf::Mesh::Input _srcInput = _srcInput;
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
		auto& src = mesh.buffers[srcInput.interleaved];
		auto& dst = buffers[dstInput.interleaved];
		dst.insert( dst.end(), src.begin(), src.end() );
	// both meshes are de-interleaved, just copy directly
	} else if ( !isInterleaved(dstInput.interleaved) && !isInterleaved(srcInput.interleaved) ) {
		for ( auto i = 0; i < dstInput.attributes.size(); ++i ) {
			auto& srcAttribute = srcInput.attributes[i];
			auto& dstAttribute = dstInput.attributes[i];
			auto& src = mesh.buffers[srcAttribute.buffer];
			auto& dst = buffers[dstAttribute.buffer];
			dst.insert( dst.end(), src.begin(), src.end() );
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
		uf::Mesh::Input _srcInput = _srcInput;
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
		buffers[input.interleaved].resize( count * input.stride );
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