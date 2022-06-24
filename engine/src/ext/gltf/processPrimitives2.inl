uf::stl::vector<uf::Meshlet_T<UF_GRAPH_MESH_FORMAT>> meshlets;

for ( auto& p : m.primitives ) {
	size_t primitiveID = meshlets.size();
	auto& meshlet = meshlets.emplace_back();

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
			meshlet.vertices.resize(accessor.count);
			meshlet.primitive.instance.bounds.min = pod::Vector3f{ accessor.minValues[0], accessor.minValues[1], accessor.minValues[2] };
			meshlet.primitive.instance.bounds.max = pod::Vector3f{ accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2] };

			if ( !(graph.metadata["flags"]["INVERT"].as<bool>()) ){
				meshlet.primitive.instance.bounds.min.x = -meshlet.primitive.instance.bounds.min.x;
				meshlet.primitive.instance.bounds.max.x = -meshlet.primitive.instance.bounds.max.x;
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

	for ( size_t i = 0; i < meshlet.vertices.size(); ++i ) {
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

		auto& vertex = meshlet.vertices[i];
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

		vertex.id.x = primitiveID;
		vertex.id.y = meshID;
	}

	if ( p.indices > -1 ) {
		auto& accessor = model.accessors[p.indices];
		auto& view = model.bufferViews[accessor.bufferView];
		auto& buffer = model.buffers[view.buffer];

		meshlet.indices.reserve( static_cast<uint32_t>(accessor.count) );

	#define COPY_INDICES() for (size_t index = 0; index < accessor.count; index++) meshlet.indices.emplace_back(buf[index]);

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

	meshlet.primitive.instance.materialID = p.material;
	meshlet.primitive.instance.primitiveID = primitiveID;
	meshlet.primitive.instance.meshID = meshID;
	meshlet.primitive.instance.objectID = 0;

	meshlet.primitive.drawCommand.indices = meshlet.indices.size();
	meshlet.primitive.drawCommand.instances = 1;
	meshlet.primitive.drawCommand.indexID = 0;
	meshlet.primitive.drawCommand.vertexID = 0;
	meshlet.primitive.drawCommand.instanceID = 0;
	meshlet.primitive.drawCommand.vertices = meshlet.vertices.size();
}


if ( meshgrid.grid.divisions.x > 1 || meshgrid.grid.divisions.y > 1 || meshgrid.grid.divisions.z > 1 ) {
	auto partitioned = uf::meshgrid::partition( meshgrid.grid, meshlets, meshgrid.eps );
	if ( meshgrid.print ) UF_MSG_DEBUG( "Draw commands: " << m.name << ": " << meshlets.size() << " -> " << partitioned.size() << " | Partitions: " << 
		(meshgrid.grid.divisions.x * meshgrid.grid.divisions.y * meshgrid.grid.divisions.z) << " -> " << meshgrid.grid.nodes.size()
	 );
	meshlets = std::move( partitioned );
}

{
	size_t indexID = 0;
	size_t vertexID = 0;

	mesh.bindIndirect<pod::DrawCommand>();
	mesh.bind<UF_GRAPH_MESH_FORMAT>(false); // default to de-interleaved regardless of requirement (makes things easier)
	
	for ( auto& meshlet : meshlets ) {
		auto& drawCommand = drawCommands.emplace_back(pod::DrawCommand{
			.indices = meshlet.indices.size(),
			.instances = 1,
			.indexID = indexID,
			.vertexID = vertexID,
			.instanceID = 0,
			.auxID = meshlet.primitive.drawCommand.auxID,
			.materialID = meshlet.primitive.drawCommand.materialID,
			.vertices = meshlet.vertices.size(),
		});
		
		primitives.emplace_back( meshlet.primitive );

		indexID += meshlet.indices.size();
		vertexID += meshlet.vertices.size();

		mesh.insertVertices(meshlet.vertices);
		mesh.insertIndices(meshlet.indices);
	}

	mesh.insertIndirects(drawCommands);
	mesh.updateDescriptor();
}