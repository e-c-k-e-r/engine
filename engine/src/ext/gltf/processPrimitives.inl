for ( auto& p : m.primitives ) {
	vertices.clear();
	indices.clear();
	auto& primitive = primitives.emplace_back();

	struct Attribute {
		uf::stl::string name = "";
		size_t components = 0;
		size_t length = 0;
		size_t stride = 0;
		uint8_t* buffer = NULL;

		uf::stl::vector<float> floats;
		uf::stl::vector<uint8_t> int8s;
		uf::stl::vector<uint16_t> int16s;
		uf::stl::vector<uint32_t> int32s;
	};

	uf::stl::unordered_map<uf::stl::string, Attribute> attributes =  {
		{"POSITION", {}},
		{"TEXCOORD_0", {}},
		{"COLOR_0", {}},
		{"NORMAL", {}},
#if UF_GRAPH_PROCESS_PRIMITIVES_FULL
		{"TANGENT", {}},
		{"JOINTS_0", {}},
		{"WEIGHTS_0", {}},
#endif
	};

	for ( auto& kv : attributes ) {
		auto& attribute = kv.second;
		attribute.name = kv.first;
		auto it = p.attributes.find(attribute.name);
		if ( it == p.attributes.end() ) continue;

		auto& accessor = model.accessors[it->second];
		auto& view = model.bufferViews[accessor.bufferView];
		
		if ( attribute.name == "POSITION" ) {
			vertices.resize(accessor.count);
			primitive.instance.bounds.min = pod::Vector3f{ accessor.minValues[0], accessor.minValues[1], accessor.minValues[2] };
			primitive.instance.bounds.max = pod::Vector3f{ accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2] };

			if ( !(graph.metadata["flags"]["INVERT"].as<bool>()) ){
				primitive.instance.bounds.min.x = -primitive.instance.bounds.min.x;
				primitive.instance.bounds.max.x = -primitive.instance.bounds.max.x;
			}
		}

		switch ( accessor.componentType ) {
			case TINYGLTF_COMPONENT_TYPE_BYTE:
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
				auto* buffer = reinterpret_cast<const uint8_t*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				attribute.components = accessor.ByteStride(view) / sizeof(uint8_t);
				attribute.length = accessor.count * attribute.components;
				attribute.int8s.assign( &buffer[0], &buffer[attribute.length] );
			} break;
			case TINYGLTF_COMPONENT_TYPE_SHORT:
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
				auto* buffer = reinterpret_cast<const uint16_t*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				attribute.components = accessor.ByteStride(view) / sizeof(uint16_t);
				attribute.length = accessor.count * attribute.components;
				attribute.int16s.assign( &buffer[0], &buffer[attribute.length] );
			} break;
			case TINYGLTF_COMPONENT_TYPE_INT:
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
				auto* buffer = reinterpret_cast<const uint32_t*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				attribute.components = accessor.ByteStride(view) / sizeof(uint32_t);
				attribute.length = accessor.count * attribute.components;
				attribute.int32s.assign( &buffer[0], &buffer[attribute.length] );
			} break;
			case TINYGLTF_COMPONENT_TYPE_FLOAT: {
				auto* buffer = reinterpret_cast<const float*>(&(model.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
				attribute.components = accessor.ByteStride(view) / sizeof(float);
				attribute.length = accessor.count * attribute.components;
				attribute.floats.assign( &buffer[0], &buffer[attribute.length] );
			} break;
			default: UF_MSG_ERROR("Unsupported component type");
		}
	}

	for ( size_t i = 0; i < vertices.size(); ++i ) {
	#if 0
		#define ITERATE_ATTRIBUTE( name, member )\
			memcpy( &vertex.member[0], &attributes[name].buffer[i * attributes[name].components], attributes[name].stride );
	#else
		#define ITERATE_ATTRIBUTE( name, member, floatScale )\
			if ( !attributes[name].int8s.empty() ) { \
				for ( size_t j = 0; j < attributes[name].components; ++j )\
					vertex.member[j] = attributes[name].int8s[i * attributes[name].components + j];\
			} else if ( !attributes[name].int16s.empty() ) { \
				for ( size_t j = 0; j < attributes[name].components; ++j )\
					vertex.member[j] = attributes[name].int16s[i * attributes[name].components + j];\
			} else if ( !attributes[name].int32s.empty() ) { \
				for ( size_t j = 0; j < attributes[name].components; ++j )\
					vertex.member[j] = attributes[name].int32s[i * attributes[name].components + j];\
			} else if ( !attributes[name].floats.empty() ) { \
				for ( size_t j = 0; j < attributes[name].components; ++j )\
					vertex.member[j] = attributes[name].floats[i * attributes[name].components + j] * floatScale;\
			}
	#endif

		auto& vertex = vertices[i];
		ITERATE_ATTRIBUTE("POSITION", position, 1);
		ITERATE_ATTRIBUTE("TEXCOORD_0", uv, 1);
		ITERATE_ATTRIBUTE("COLOR_0", color, 255.0f);
		ITERATE_ATTRIBUTE("NORMAL", normal, 1);
	#if UF_GRAPH_PROCESS_PRIMITIVES_FULL
		ITERATE_ATTRIBUTE("TANGENT", tangent, 1);
		ITERATE_ATTRIBUTE("JOINTS_0", joints, 1);
		ITERATE_ATTRIBUTE("WEIGHTS_0", weights, 1);
	#endif

		#undef ITERATE_ATTRIBUTE

		// required due to reverse-Z projection matrix flipping the X axis as well
		// default is to proceed with this
		if ( !(graph.metadata["flags"]["INVERT"].as<bool>()) ){
			vertex.position.x = -vertex.position.x;
			vertex.normal.x = -vertex.normal.x;
		#if UF_GRAPH_PROCESS_PRIMITIVES_FULL
			vertex.tangent.x = -vertex.tangent.x;
		#endif
		}
	}

	if ( p.indices > -1 ) {
		auto& accessor = model.accessors[p.indices];
		auto& view = model.bufferViews[accessor.bufferView];
		auto& buffer = model.buffers[view.buffer];

		indices.reserve( static_cast<uint32_t>(accessor.count) );

	#define COPY_INDICES() for (size_t index = 0; index < accessor.count; index++) indices.emplace_back(buf[index]);

		const void* pointer = &(buffer.data[accessor.byteOffset + view.byteOffset]);
		switch (accessor.componentType) {
			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
				auto* buf = static_cast<const uint32_t*>( pointer );
				COPY_INDICES()
				break;
			}
			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
				auto* buf = static_cast<const uint16_t*>( pointer );
				COPY_INDICES()
				break;
			}
			case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
				auto* buf = static_cast<const uint8_t*>( pointer );
				COPY_INDICES()
				break;
			}
		}
		#undef COPY_INDICES
	}

	primitive.instance.materialID = p.material;
	primitive.instance.primitiveID = primitives.size() - 1;
	primitive.instance.meshID = meshID;
	primitive.instance.objectID = 0;

	primitive.drawCommand.indices = indices.size();
	primitive.drawCommand.instances = 1;
	primitive.drawCommand.indexID = 0;
	primitive.drawCommand.vertexID = 0;
	primitive.drawCommand.instanceID = 0;
	primitive.drawCommand.vertices = vertices.size();

	auto& drawCommand = drawCommands.emplace_back(pod::DrawCommand{
		.indices = indices.size(),
		.instances = 1,
		.indexID = mesh.index.count,
		.vertexID = mesh.vertex.count,
		.instanceID = 0,


		.vertices = vertices.size(),
	});
	
	mesh.insertVertices(vertices);
	mesh.insertIndices(indices);
}