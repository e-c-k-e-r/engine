#include <uf/utils/mesh/mesh.h>
#include <uf/engine/graph/graph.h>

// Used for per-vertex colors
UF_VERTEX_DESCRIPTOR(pod::Vertex_3F2F3F4F,
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F4F, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F4F, R32G32_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F4F, R32G32B32_SFLOAT, normal)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F4F, R32_UINT, color)
)
UF_VERTEX_INTERPOLATE(pod::Vertex_3F2F3F4F, {
	return {
		uf::vector::lerp( p1.position, p2.position, t ),
		uf::vector::lerp( p1.uv, p2.uv, t ),
		uf::vector::normalize( uf::vector::lerp( p1.normal, p2.normal, t ) ),
		uf::vector::lerp( p1.color, p2.color, t ),
	};
})
// Used for terrain
UF_VERTEX_DESCRIPTOR(pod::Vertex_3F2F3F32B,
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F32B, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F32B, R32G32_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F32B, R32G32B32_SFLOAT, normal)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F32B, R32_UINT, color)
)
UF_VERTEX_INTERPOLATE(pod::Vertex_3F2F3F32B, {
	return {
		uf::vector::lerp( p1.position, p2.position, t ),
		uf::vector::lerp( p1.uv, p2.uv, t ),
		uf::vector::normalize( uf::vector::lerp( p1.normal, p2.normal, t ) ),
		t < 0.5 ? p1.color : p2.color,
		//uf::vector::lerp( p1.color, p2.color, t ),
	};
})
// Used for normal meshses
UF_VERTEX_DESCRIPTOR(pod::Vertex_3F2F3F,
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F, R32G32_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F, R32G32B32_SFLOAT, normal)
)
UF_VERTEX_INTERPOLATE(pod::Vertex_3F3F3F, {
	return {
		uf::vector::lerp( p1.position, p2.position, t ),
		uf::vector::lerp( p1.uv, p2.uv, t ),
		uf::vector::normalize( uf::vector::lerp( p1.normal, p2.normal, t ) ),
	};
})
// (Typically) used for displaying textures
UF_VERTEX_DESCRIPTOR(pod::Vertex_3F2F,
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F, R32G32_SFLOAT, uv)
)
UF_VERTEX_INTERPOLATE(pod::Vertex_3F2F3F1UI, {
	return {
		uf::vector::lerp( p1.position, p2.position, t ),
		uf::vector::lerp( p1.uv, p2.uv, t ),
		uf::vector::normalize( uf::vector::lerp( p1.normal, p2.normal, t ) ),
		uf::vector::lerp( p1.id, p2.id, t ),
	};
})
UF_VERTEX_DESCRIPTOR(pod::Vertex_2F2F,
	UF_VERTEX_DESCRIPTION(pod::Vertex_2F2F, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(pod::Vertex_2F2F, R32G32_SFLOAT, uv)
)
UF_VERTEX_INTERPOLATE(pod::Vertex_3F2F3F, {
	return {
		uf::vector::lerp( p1.position, p2.position, t ),
		uf::vector::lerp( p1.uv, p2.uv, t ),
		uf::vector::normalize( uf::vector::lerp( p1.normal, p2.normal, t ) ),
	};
})
// used for texture arrays
UF_VERTEX_DESCRIPTOR(pod::Vertex_3F3F3F,
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F3F3F, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F3F3F, R32G32B32_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F3F3F, R32G32B32_SFLOAT, normal)
)
UF_VERTEX_INTERPOLATE(pod::Vertex_3F2F, {
	return {
		uf::vector::lerp( p1.position, p2.position, t ),
		uf::vector::lerp( p1.uv, p2.uv, t ),
	};
})
UF_VERTEX_DESCRIPTOR(pod::Vertex_3F2F3F1UI,
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F1UI, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F1UI, R32G32_SFLOAT, uv)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F1UI, R32G32B32_SFLOAT, normal)
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F2F3F1UI, R32_UINT, id)
)
UF_VERTEX_INTERPOLATE(pod::Vertex_2F2F, {
	return {
		uf::vector::lerp( p1.position, p2.position, t ),
		uf::vector::lerp( p1.uv, p2.uv, t ),
	};
})
// Basic
UF_VERTEX_DESCRIPTOR(pod::Vertex_3F,
	UF_VERTEX_DESCRIPTION(pod::Vertex_3F, R32G32B32_SFLOAT, position)
)
UF_VERTEX_INTERPOLATE(pod::Vertex_3F, {
	return {
		uf::vector::lerp( p1.position, p2.position, t ),
	};
})

void uf::Mesh::initialize() {}
void uf::Mesh::destroy() {
	_destroy(vertex);
	_destroy(index);
	_destroy(instance);
	_destroy(indirect);

	buffers.clear();
}
uf::Mesh uf::Mesh::copy() const {
	uf::Mesh res;

	res.bind( *this );
	res.insert(*this);
	res.updateDescriptor();

	return res;
}
uf::Mesh& uf::Mesh::copy( const uf::Mesh& src ) {
	if ( src.buffers.empty() ) return *this;

	bind( src );
	insert( src );
	updateDescriptor();

	return *this;
}
void uf::Mesh::updateDescriptor() {
	_updateDescriptor(vertex);
	_updateDescriptor(index);
	_updateDescriptor(instance);
	_updateDescriptor(indirect);
	_updateViews();
}
void uf::Mesh::bind( const uf::Mesh& mesh ) {
	vertex.attributes = mesh.vertex.attributes;
	index.attributes = mesh.index.attributes;
	instance.attributes = mesh.instance.attributes;
	indirect.attributes = mesh.indirect.attributes;

	_bind();
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
	_bind();
	
	switch ( size ) {
		case 1: { uf::stl::vector<uint8_t>  indices( vertex.count ); std::iota( indices.begin(), indices.end(), 0 ); insertIndices( indices ); } break;
		case 2: { uf::stl::vector<uint16_t> indices( vertex.count ); std::iota( indices.begin(), indices.end(), 0 ); insertIndices( indices ); } break;
		case 4: { uf::stl::vector<uint32_t> indices( vertex.count ); std::iota( indices.begin(), indices.end(), 0 ); insertIndices( indices ); } break;
	}
}
uf::Mesh uf::Mesh::expand( ) {
	 uf::Mesh res = copy();

	res.resizeVertices( index.count );
	res.vertex.count = index.count;

	auto& srcIndex = index.attributes.front();
	auto& dstIndex = res.index.attributes.front();

#define GET_INDEX(T) {\
	index = *(const T*) (static_cast<uint8_t*>(srcIndex.pointer) + idx * srcIndex.stride);\
	*((T*) (static_cast<uint8_t*>(dstIndex.pointer) + idx * dstIndex.stride)) = idx;\
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

			uint8_t* srcAddr = static_cast<uint8_t*>(srcInput.pointer) + index * srcInput.stride;
			uint8_t* dstAddr = static_cast<uint8_t*>(dstInput.pointer) + idx * dstInput.stride;

			memcpy( dstAddr, srcAddr, srcInput.descriptor.size );
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

void uf::Mesh::clearAttribute( uf::Mesh::Input& input, const uf::Mesh::Attribute& attribute ) {
	for ( size_t i = 0; i < input.attributes.size(); ++i ) if ( input.attributes[i].descriptor == attribute.descriptor ) return clearAttribute( input, i );
}
void uf::Mesh::clearAttribute( uf::Mesh::Input& input, size_t i ) {
	auto attribute = input.attributes[i];
	buffers[attribute.buffer].clear();
}
void uf::Mesh::clear() {
	for ( size_t i = 0; i < vertex.attributes.size(); ++i ) clearAttribute( vertex, i );
	for ( size_t i = 0; i < index.attributes.size(); ++i ) clearAttribute( index, i );
	for ( size_t i = 0; i < instance.attributes.size(); ++i ) clearAttribute( instance, i );
	for ( size_t i = 0; i < indirect.attributes.size(); ++i ) clearAttribute( indirect, i );

	vertex.count = 0;
	index.count = 0;
	instance.count = 0;
	indirect.count = 0;
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
			.vertices = vertex.count,
		});
	}

	_destroy( indirect );
	bindIndirect<pod::DrawCommand>();
	_bind();
	insertIndirects( commands );
}
uf::Mesh::buffer_t& uf::Mesh::getBuffer( const uf::Mesh::Input& input, size_t i ) {
	return getBuffer( input, input.attributes[i] );
}
uf::Mesh::buffer_t& uf::Mesh::getBuffer( const uf::Mesh::Input& input, const uf::Mesh::Attribute& attribute ) {
	return buffers[attribute.buffer];
}
const uf::Mesh::buffer_t& uf::Mesh::getBuffer( const uf::Mesh::Input& input, size_t i ) const {
	return getBuffer( input, input.attributes[i] );
}
const uf::Mesh::buffer_t& uf::Mesh::getBuffer( const uf::Mesh::Input& input, const uf::Mesh::Attribute& attribute ) const {
	return buffers[attribute.buffer];
}

uf::Mesh::View uf::Mesh::makeView( const uf::stl::vector<uf::stl::string>& wanted, size_t lod ) const {
	uf::Mesh::View view;
    view.vertex = vertex;
    view.index  = index;

    if ( wanted.size() ) {
        for ( auto& attr : vertex.attributes ) {
            if ( std::find(wanted.begin(), wanted.end(), attr.descriptor.name ) == wanted.end() ) continue;
            view.attributes[uf::string::fnv1a(attr.descriptor.name)] = { attr };
        }
    } else {
        for ( auto& attr : vertex.attributes ) {
            view.attributes[uf::string::fnv1a(attr.descriptor.name)] = { attr };
        }
    }

    if ( !index.attributes.empty() ) {
        view.attributes["index"_hash] = { index.attributes[lod] };
    }

    return view;
}
uf::Mesh::View uf::Mesh::makeView( size_t i, const uf::stl::vector<uf::stl::string>& wanted, size_t lod ) const {
	uf::Mesh::View view;
	view.vertex = remapVertexInput(i, lod);
	view.index  = remapIndexInput(i, lod);
	view.indirectIndex = i;

	if ( wanted.size() ) {
		for (auto& attr : vertex.attributes) {
			if ( std::find(wanted.begin(), wanted.end(), attr.descriptor.name ) == wanted.end() ) continue;
			view.attributes[uf::string::fnv1a(attr.descriptor.name)] = { attr };
		}
	} else {
		for ( auto& attr : vertex.attributes ) view.attributes[uf::string::fnv1a(attr.descriptor.name)] = { attr };
	}

	if ( !index.attributes.empty() ) {
		view.attributes["index"_hash] = { index.attributes[lod] };
	}

	return view;
}
uf::stl::vector<uf::Mesh::View> uf::Mesh::makeViews( const uf::stl::vector<uf::stl::string>& wanted, size_t lod ) const {
	uf::stl::vector<uf::Mesh::View> views;
	if ( indirect.count > 0 ) {
		for ( auto i = 0; i < indirect.count; i++ ) {
			auto view = makeView( i, wanted, lod );
			if ( view.index.count == 0 && view.vertex.count == 0 ) continue;
			views.emplace_back( view );
		}
	} else {
		views.emplace_back( makeView(wanted, lod) );
	}
	return views;
}

uf::Mesh::Input uf::Mesh::remapInput( const uf::Mesh::Input& input, size_t i, size_t lod ) const {
	uf::Mesh::Input res = input;
	UF_ASSERT( &input == &vertex || &input == &index );
	UF_ASSERT( i < indirect.count );
	
	const auto& drawCommand = ((const pod::DrawCommand*) getBuffer(indirect, lod).data())[i];
	res.first = &input == &vertex ? drawCommand.vertexID : drawCommand.indexID;
	res.count = &input == &vertex ? drawCommand.vertices : drawCommand.indices;	

	return res;
}
uf::Mesh::Input uf::Mesh::remapVertexInput( size_t i, size_t lod ) const {
	uf::Mesh::Input res = vertex;
	UF_ASSERT( i < indirect.count );

	const auto& drawCommand = ((const pod::DrawCommand*) getBuffer(indirect, lod).data())[i];
	res.first = drawCommand.vertexID;
	res.count = drawCommand.vertices;

	return res;
}
uf::Mesh::Input uf::Mesh::remapIndexInput( size_t i, size_t lod ) const {
	uf::Mesh::Input res = index;
	UF_ASSERT( i < indirect.count );

	const auto& drawCommand = ((const pod::DrawCommand*) getBuffer(indirect, lod).data())[i];
	res.first = drawCommand.indexID;
	res.count = drawCommand.indices;

	return res;
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
void uf::Mesh::_bind() {
	int32_t buffer = 0;

	auto parse_input = [&](uf::Mesh::Input& input) {
		for ( auto& attribute : input.attributes ) {
			attribute.buffer = buffer++;
			attribute.pointer = NULL;
		}
	};

	parse_input(vertex);
	parse_input(index);
	parse_input(instance);
	parse_input(indirect);

	buffers.resize( buffer );
	updateDescriptor();
}
void uf::Mesh::_updateDescriptor( uf::Mesh::Input& input ) {
	input.size = 0;
	for ( auto& attribute : input.attributes ) {
		auto& buffer = buffers[attribute.buffer];
		attribute.length = buffer.size();
		attribute.pointer = buffer.data() + attribute.offset;

		if ( &input == &index || &input == &indirect ) input.size = attribute.descriptor.size;
		else input.size += attribute.descriptor.size;

		attribute.stride = attribute.descriptor.size;
	}
}
void uf::Mesh::_updateViews() {
	buffer_views = makeViews();
}
uf::Mesh::Attribute uf::Mesh::_remapAttribute( const uf::Mesh::Input& input, const uf::Mesh::Attribute& attribute, size_t i ) const {
	UF_ASSERT( i < indirect.count );
	UF_ASSERT( &input == &vertex || &input == &index );
	UF_MSG_WARNING( "deprecated, please use remapInput" );

	uf::Mesh::Attribute res = attribute;

	auto& drawCommand = ((const pod::DrawCommand*) getBuffer(indirect).data())[i];
	if ( &input == &vertex ) {
		res.pointer = static_cast<uint8_t*>(res.pointer) + drawCommand.vertexID * res.stride;
		res.length = drawCommand.vertices * res.stride;
	} else if ( &input == &index ) {
		res.pointer = static_cast<uint8_t*>(res.pointer) + drawCommand.indexID * res.stride;
		res.length = drawCommand.indices * res.stride;
	}
	return res;
}
void uf::Mesh::_insertVs( uf::Mesh::Input& dstInput, const uf::Mesh& mesh, const uf::Mesh::Input& srcInput ) {
	_reserveVs( dstInput, dstInput.count += srcInput.count );

	if ( _hasV( dstInput, srcInput ) ) {
		for ( auto i = 0; i < dstInput.attributes.size(); ++i ) {
			auto& src = mesh.buffers[srcInput.attributes[i].buffer];
			auto& dst = buffers[dstInput.attributes[i].buffer];
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

	_updateDescriptor( dstInput );
}
void uf::Mesh::_insertIs( uf::Mesh::Input& dstInput, const uf::Mesh& mesh, const uf::Mesh::Input& srcInput ) {
	_reserveIs( dstInput, dstInput.count += srcInput.count );

	for ( auto i = 0; i < dstInput.attributes.size(); ++i ) {
		auto& src = mesh.getBuffer( srcInput, i );
		auto& dst = getBuffer( dstInput, i );

		dst.insert( dst.end(), src.begin(), src.end() );
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
	for ( auto& attribute : input.attributes ) {
		buffers[attribute.buffer].reserve( count * attribute.descriptor.size );
		attribute.length = buffers[attribute.buffer].size();
		attribute.pointer = (uint8_t*) (buffers[attribute.buffer].data());
	}
}
void uf::Mesh::_resizeVs( uf::Mesh::Input& input, size_t count ) {
	for ( auto& attribute : input.attributes ) {
		buffers[attribute.buffer].resize( count * attribute.descriptor.size );
		attribute.length = buffers[attribute.buffer].size();
		attribute.pointer = (uint8_t*) (buffers[attribute.buffer].data());
	}
}
void uf::Mesh::_insertV( uf::Mesh::Input& input, const void* data ) {
	_reserveVs( input, ++input.count );
	const uint8_t* pointer = (const uint8_t*) data;

	for ( auto& attribute : input.attributes ) {
		buffers[attribute.buffer].insert(
			buffers[attribute.buffer].end(),
			pointer + attribute.descriptor.offset,
			pointer + attribute.descriptor.offset + attribute.descriptor.size
		);
	}
}
void uf::Mesh::_insertVs( uf::Mesh::Input& input, const void* data, size_t size ) {
	size_t count = input.count;
    input.count += size;

    _resizeVs( input, input.count );

    const uint8_t* pointer = static_cast<const uint8_t*>(data);
    for ( auto& attribute : input.attributes ) {
        uint8_t* dstBase = buffers[attribute.buffer].data() + (count * attribute.descriptor.size);

        size_t srcOffset = attribute.descriptor.offset;
        size_t attrSize = attribute.descriptor.size;

        for ( size_t i = 0; i < size; ++i ) {
            const uint8_t* srcAddr = pointer + (i * input.size) + srcOffset;
            uint8_t* dstAddr = dstBase + (i * attrSize);

            memcpy( dstAddr, srcAddr, attrSize );
        }
    }
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
	attribute.pointer = (uint8_t*) (buffers[attribute.buffer].data());
}
void uf::Mesh::_resizeIs( uf::Mesh::Input& input, size_t count, size_t i ) {
	auto& attribute = input.attributes[i];
	buffers[attribute.buffer].resize( count * attribute.descriptor.size );
	attribute.length = buffers[attribute.buffer].size();
	attribute.pointer = (uint8_t*) (buffers[attribute.buffer].data());
}
void uf::Mesh::_insertI( uf::Mesh::Input& input, const void* data, size_t i ) {			
	auto& attribute = input.attributes[i];
	_reserveIs( input, ++input.count );
	const uint8_t* pointer = (const uint8_t*) data;
	
	buffers[attribute.buffer].insert( buffers[attribute.buffer].end(), pointer, pointer + attribute.descriptor.size );
}
void uf::Mesh::_insertIs( uf::Mesh::Input& input, const void* data, size_t size, size_t i ) {
	auto& attribute = input.attributes[i];
	_reserveIs( input, input.count += size );
	const uint8_t* pointer = (const uint8_t*) data;
	for ( const uint8_t* p = pointer; p < pointer + size * attribute.descriptor.size; p += attribute.descriptor.size )
		buffers[attribute.buffer].insert( buffers[attribute.buffer].end(), p + attribute.descriptor.offset, p + attribute.descriptor.offset + attribute.descriptor.size );
}

////
void uf::mesh::setIndex( void* pointer, size_t stride, size_t index, size_t value ) {
	switch ( stride ) {
		case 1: ((uint8_t*)  pointer)[index] = static_cast<uint8_t>(value);  break;
		case 2: ((uint16_t*) pointer)[index] = static_cast<uint16_t>(value); break;
		case 4: ((uint32_t*) pointer)[index] = static_cast<uint32_t>(value); break;
		default: {
			UF_EXCEPTION("invalid stride type: {}", stride);
		} break;
	}
}
size_t uf::mesh::fetchIndex( const void* pointer, size_t stride, size_t index ) { 
	#define CAST_INDEX(T) case sizeof(T): return ((T*) pointer)[index];
	switch ( stride ) {
		CAST_INDEX(uint8_t);
		CAST_INDEX(uint16_t);
		CAST_INDEX(uint32_t);
		default: {
			UF_EXCEPTION("invalid stride type: {}", stride);
		} break;
	}
}
pod::Vector3f uf::mesh::fetchVertex( const uf::Mesh::View& view, const uf::Mesh::AttributeView& positions, size_t index ) {
	return uf::mesh::fetchVertexAttribute<pod::Vector3f>( view, positions, index );
}

pod::Triangle uf::mesh::fetchTriangle( const uf::Mesh::View& view, const uf::Mesh::AttributeView& indices, const uf::Mesh::AttributeView& positions, size_t triID ) {
	auto index = triID * 3;
	pod::Triangle tri;
	FOR_EACH(3, {
		tri.points[i] = uf::mesh::fetchVertex( view, positions, uf::mesh::fetchIndex( view, indices, index + i ) );
	});
	return tri;
}

pod::TriangleWithNormal uf::mesh::fetchTriangle( const uf::Mesh& mesh, size_t triID ) {
	const auto& views = mesh.buffer_views;
	UF_ASSERT(!views.empty());

	// find which view contains this triangle index.
	size_t triBase = 0;
	const uf::Mesh::View* view = nullptr;
	for ( auto& v : views ) {
		auto trisInView = v.index.count / 3;
		if (triID < triBase + trisInView) {
			view = &v;
			triID -= triBase; // local triangle index inside this view
			break;
		}
		triBase += trisInView;
	}
	UF_ASSERT( view );
	
	pod::TriangleWithNormal tri = { uf::mesh::fetchTriangle( *view, triID ) };
	tri.normal = uf::vector::normalize(uf::vector::cross(tri.points[1] - tri.points[0], tri.points[2] - tri.points[0]));

	return tri;
}