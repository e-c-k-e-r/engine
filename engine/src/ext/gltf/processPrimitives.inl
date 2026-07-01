struct {
	struct {
		bool should = true;
		size_t corrected{};
		size_t total{};
	} windingOrder;
	struct {
		bool should = true;
	} normals;
	struct {
		bool should = true;
	} tangents;
} sanitizer;

if ( graph.metadata["sanitizer"]["winding order"].as<bool>(true) || graph.metadata["renderer"]["invert"].as<bool>(true) ) {
	sanitizer.windingOrder.should = true;
}
if ( graph.metadata["sanitizer"]["normals"].as<bool>(false) ) {
	sanitizer.normals.should = true;
}
if ( graph.metadata["sanitizer"]["tangents"].as<bool>(false) ) {
	sanitizer.tangents.should = true;
}

uf::stl::vector<::Meshlet> meshlets;

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
		{"TANGENT", {}},
		{"JOINTS_0", {}},
		{"WEIGHTS_0", {}},
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
	#define ITERATE_ATTRIBUTE( name, member, floatScale )\
		if ( !attributes[name].int8s.empty() ) { \
			size_t limit = std::min<size_t>(attributes[name].components, vertex.member.size); \
			for ( size_t j = 0; j < limit; ++j )\
				vertex.member[j] = attributes[name].int8s[i * attributes[name].components + j];\
		} else if ( !attributes[name].int16s.empty() ) { \
			size_t limit = std::min<size_t>(attributes[name].components, vertex.member.size); \
			for ( size_t j = 0; j < limit; ++j )\
				vertex.member[j] = attributes[name].int16s[i * attributes[name].components + j];\
		} else if ( !attributes[name].int32s.empty() ) { \
			size_t limit = std::min<size_t>(attributes[name].components, vertex.member.size); \
			for ( size_t j = 0; j < limit; ++j )\
				vertex.member[j] = attributes[name].int32s[i * attributes[name].components + j];\
		} else if ( !attributes[name].floats.empty() ) { \
			size_t limit = std::min<size_t>(attributes[name].components, vertex.member.size); \
			for ( size_t j = 0; j < limit; ++j )\
				vertex.member[j] = attributes[name].floats[i * attributes[name].components + j] * floatScale;\
		} else {\
			/*for some reason setting this breaks VXGI reflections*/\
			/*vertex.member = {};*/\
		}

		auto& vertex = meshlet.vertices[i];
		ITERATE_ATTRIBUTE("POSITION", position, 1);
		ITERATE_ATTRIBUTE("TEXCOORD_0", uv, 1);
		ITERATE_ATTRIBUTE("COLOR_0", color, 255.0f);
		ITERATE_ATTRIBUTE("NORMAL", normal, 1);
		ITERATE_ATTRIBUTE("TANGENT", tangent, 1);
		ITERATE_ATTRIBUTE("JOINTS_0", joints, 1);
		ITERATE_ATTRIBUTE("WEIGHTS_0", weights, 1);

		#undef ITERATE_ATTRIBUTE

		// required due to reverse-Z projection matrix flipping the X axis as well
		// default is to proceed with this
		if ( graph.metadata["renderer"]["invert"].as<bool>(true) ){
			vertex.position.x = -vertex.position.x;
			vertex.normal.x = -vertex.normal.x;
			vertex.tangent.x = -vertex.tangent.x;

			meshlet.primitive.instance.bounds.min = uf::vector::min( meshlet.primitive.instance.bounds.min, vertex.position );
			meshlet.primitive.instance.bounds.max = uf::vector::max( meshlet.primitive.instance.bounds.max, vertex.position );
		}
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

	meshlet.primitive.instance.bounds.center = (meshlet.primitive.instance.bounds.max + meshlet.primitive.instance.bounds.min) * 0.5f;
	meshlet.primitive.instance.bounds.extent = (meshlet.primitive.instance.bounds.max - meshlet.primitive.instance.bounds.min) * 0.5f;

	/* detect winding order */ if ( sanitizer.windingOrder.should ) {
		sanitizer.windingOrder.corrected += uf::mesh::windingOrder( meshlet.vertices, meshlet.indices );
		sanitizer.windingOrder.total += meshlet.indices.empty() ? meshlet.vertices.size() : meshlet.indices.size();
	}
	/* calculate normals */ if ( sanitizer.normals.should ) {
		uf::mesh::normals( meshlet.vertices, meshlet.indices );
	}
	/* calculate tangents */ if ( sanitizer.tangents.should ) {
	#if 1
		uf::mesh::tangents( meshlet.vertices, meshlet.indices );
	#else
		if ( !meshlet.indices.empty() ) {
			for ( size_t i = 0; i < meshlet.indices.size() / 3; ++i ) {
				size_t indices[3] = {
					meshlet.indices[i * 3 + 0],
					meshlet.indices[i * 3 + 1],
					meshlet.indices[i * 3 + 2],
				};
				pod::Vector3f triPosition[3] = {
					meshlet.vertices[indices[0]].position,
					meshlet.vertices[indices[1]].position,
					meshlet.vertices[indices[2]].position,
				};
				pod::Vector2f triUV[3] = {
					meshlet.vertices[indices[0]].uv,
					meshlet.vertices[indices[1]].uv,
					meshlet.vertices[indices[2]].uv,
				};

				pod::Vector3f deltaTriPosition[2] = {
					triPosition[1]  - triPosition[0],
					triPosition[2]  - triPosition[0],
				};
				pod::Vector2f deltaTriUV[2] = {
					triUV[1]  - triUV[0],
					triUV[2]  - triUV[0],
				};

				float r = 1.0f / (deltaTriUV[0].x * deltaTriUV[1].y - deltaTriUV[0].y * deltaTriUV[1].x);
				auto tangent_tri = (deltaTriPosition[0] * deltaTriUV[1].y   - deltaTriPosition[1] * deltaTriUV[0].y) * r;
				auto bitangent_tri = (deltaTriPosition[1] * deltaTriUV[0].x   - deltaTriPosition[0] * deltaTriUV[1].x) * r;

				for ( auto j = 0; j < 3; ++j ) {
					auto& normal = meshlet.vertices[indices[j]].normal;
					auto& tangent = meshlet.vertices[indices[j]].tangent;
					tangent = uf::vector::normalize(tangent_tri - normal * uf::vector::dot(normal, tangent_tri));

					if ( uf::vector::dot(uf::vector::cross(normal, tangent), bitangent_tri) < 0.0f )
						tangent = tangent * -1.0f;
				}
			}
		} else {
			for ( size_t i = 0; i < meshlet.vertices.size() / 3; ++i ) {
				size_t indices[3] = {
					i * 3 + 0,
					i * 3 + 1,
					i * 3 + 2,
				};
				pod::Vector3f triPosition[3] = {
					meshlet.vertices[indices[0]].position,
					meshlet.vertices[indices[1]].position,
					meshlet.vertices[indices[2]].position,
				};
				pod::Vector2f triUV[3] = {
					meshlet.vertices[indices[0]].uv,
					meshlet.vertices[indices[1]].uv,
					meshlet.vertices[indices[2]].uv,
				};

				pod::Vector3f deltaTriPosition[2] = {
					triPosition[1]  - triPosition[0],
					triPosition[2]  - triPosition[0],
				};
				pod::Vector2f deltaTriUV[2] = {
					triUV[1]  - triUV[0],
					triUV[2]  - triUV[0],
				};

				float r = 1.0f / (deltaTriUV[0].x * deltaTriUV[1].y - deltaTriUV[0].y * deltaTriUV[1].x);
				auto tangent_tri = (deltaTriPosition[0] * deltaTriUV[1].y   - deltaTriPosition[1] * deltaTriUV[0].y) * r;
				auto bitangent_tri = (deltaTriPosition[1] * deltaTriUV[0].x   - deltaTriPosition[0] * deltaTriUV[1].x) * r;

				for ( auto j = 0; j < 3; ++j ) {
					auto& normal = meshlet.vertices[indices[j]].normal;
					auto& tangent = meshlet.vertices[indices[j]].tangent;
					tangent = uf::vector::normalize(tangent_tri - normal * uf::vector::dot(normal, tangent_tri));

					if (uf::vector::dot(uf::vector::cross(normal, tangent), bitangent_tri) < 0.0f)
						tangent = tangent * -1.0f;
				}
			}
		}
	#endif
	}
}

if ( sanitizer.windingOrder.should && !graph.metadata["renderer"]["invert"].as<bool>(true) ) {
	UF_MSG_DEBUG("Winding order correction: {:.3f}%", ( (float) sanitizer.windingOrder.corrected / (float) sanitizer.windingOrder.total ) * 100.0f );
	if ( sanitizer.windingOrder.corrected * 2 > sanitizer.windingOrder.total ) {
		UF_MSG_DEBUG("Consider enablind renderer.invert for mesh: {}", m.name);
	}
}


if ( meshgrid.grid.divisions.x > 1 || meshgrid.grid.divisions.y > 1 || meshgrid.grid.divisions.z > 1 ) {
	auto partitioned = uf::meshgrid::partition( meshgrid.grid, meshlets, meshgrid.eps, meshgrid.clip, meshgrid.cleanup );
	if ( meshgrid.print ) UF_MSG_DEBUG( "Draw commands: {}: {} -> {} | Partitions: {} -> {}", m.name, meshlets.size(), partitioned.size(), 
		(meshgrid.grid.divisions.x * meshgrid.grid.divisions.y * meshgrid.grid.divisions.z), meshgrid.grid.nodes.size()
	);
	for ( auto& meshlet : partitioned ) {
		uf::mesh::tangents( meshlet.vertices, meshlet.indices );
	}
	mesh.compile( partitioned, primitives );
} else {
	mesh.compile( meshlets, primitives );
}