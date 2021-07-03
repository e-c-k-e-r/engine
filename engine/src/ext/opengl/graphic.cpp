#if UF_USE_OPENGL

#include <uf/ext/opengl/graphic.h>
#include <uf/ext/opengl/initializers.h>
#include <uf/ext/opengl/opengl.h>
#include <uf/ext/opengl/commands.h>
#include <uf/ext/openvr/openvr.h>
#include <uf/ext/gltf/gltf.h>

#define GL_DEBUG_VALIDATION_MESSAGE(x)\
	GL_VALIDATION_MESSAGE(x);

void ext::opengl::Pipeline::initialize( const Graphic& graphic ) {
	return this->initialize( graphic, graphic.descriptor );
}
void ext::opengl::Pipeline::initialize( const Graphic& graphic, const GraphicDescriptor& descriptor ) {
	this->device = graphic.device;
	//this->descriptor = descriptor;
	Device& device = *graphic.device;
#if !UF_USE_OPENGL_FIXED_FUNCTION
	{
		pipeline = glCreateProgram();
		for ( auto& shader : graphic.material.shaders ) {
			GL_ERROR_CHECK(glAttachShader(pipeline, shader.module));
		}
		GL_ERROR_CHECK(glLinkProgram(pipeline));
	}
	{
		size_t len = 1024;
		GLint status;
		GLbyte log[len];
		GL_ERROR_CHECK(glGetProgramiv(pipeline, GL_LINK_STATUS, &status));
		if ( !status ) {
			GL_ERROR_CHECK(glGetProgramInfoLog(pipeline, len, NULL, log));
			GL_VALIDATION_MESSAGE(log);
		}
	}
	{
		GL_ERROR_CHECK(glGenVertexArrays(1, &vertexArray));
		GL_ERROR_CHECK(glBindVertexArray(vertexArray));
		for ( size_t i = 0; i < descriptor.geometry.attributes.descriptor.size(); ++i ) {
			auto& attribute = descriptor.geometry.attributes.descriptor[i];

			GL_ERROR_CHECK(glEnableVertexAttribArray(i));
			GL_ERROR_CHECK(glVertexAttribPointer(0, attribute.components, attribute.type, false, descriptor.geometry.attributes.vertex.size, attribute.offset));
		}
		GL_ERROR_CHECK(glBindVertexArray(0));
	}
	{
		descriptor.pipeline = pipeline;
		descriptor.vertexArray = vertexArray;
	}
#endif
//	graphic.process = true;
}
void ext::opengl::Pipeline::record( const Graphic& graphic, CommandBuffer& commandBuffer, size_t pass, size_t draw ) const {
#if !UF_USE_OPENGL_FIXED_FUNCTION
	CommandBuffer::InfoPipeline pipelineCommandInfo = {};
	pipelineCommandInfo.type = ext::opengl::enums::Command::BIND_BUFFER;
	pipelineCommandInfo.descriptor.pipeline = descriptor.pipeline;
	pipelineCommandInfo.descriptor.vertexArray = descriptor.vertexArray;
	commandBuffer.record(pipelineCommandInfo);
#endif
}
void ext::opengl::Pipeline::update( const Graphic& graphic ) {
	return this->update( graphic, graphic.descriptor );
}
void ext::opengl::Pipeline::update( const Graphic& graphic, const GraphicDescriptor& descriptor ) {
}
void ext::opengl::Pipeline::destroy() {
	if ( aliased ) return;
#if !UF_USE_OPENGL_FIXED_FUNCTION
	if ( pipeline != GL_NULL_HANDLE ) {
		GL_ERROR_CHECK(glDeleteProgram(pipeline));
	}
	if ( vertexArray != GL_NULL_HANDLE ) {
		GL_ERROR_CHECK(glDeleteVertexArrays(1, &vertexArray));
	}
#endif
	descriptor = {};
}

void ext::opengl::Material::initialize( Device& device ) {
	this->device = &device;
}
void ext::opengl::Material::destroy() {
	if ( aliased ) return;

	for ( auto& texture : textures ) texture.destroy();
	for ( auto& shader : shaders ) shader.destroy();
	for ( auto& sampler : samplers ) sampler.destroy();

	shaders.clear();
	textures.clear();
	samplers.clear();
}
void ext::opengl::Material::attachShader( const uf::stl::string& filename, enums::Shader::type_t stage, const uf::stl::string& pipeline ) {
	auto& shader = shaders.emplace_back();
	shader.initialize( *device, filename, stage );
	
	uf::stl::string type = "unknown";
	switch ( stage ) {
		case uf::renderer::enums::Shader::VERTEX: type = "vertex"; break;
		case uf::renderer::enums::Shader::FRAGMENT: type = "fragment"; break;
		case uf::renderer::enums::Shader::COMPUTE: type = "compute"; break;
	}
	shader.metadata.pipeline = pipeline;
	shader.metadata.type = type;

	metadata.json["shaders"][pipeline][type]["index"] = shaders.size() - 1;
	metadata.json["shaders"][pipeline][type]["filename"] = filename;
	metadata.shaders[pipeline+":"+type] = shaders.size() - 1;
}
void ext::opengl::Material::initializeShaders( const uf::stl::vector<std::pair<uf::stl::string, enums::Shader::type_t>>& layout, const uf::stl::string& pipeline ) {
	shaders.clear(); shaders.reserve( layout.size() );
	for ( auto& request : layout ) {
		attachShader( request.first, request.second, pipeline );
	}
}
bool ext::opengl::Material::hasShader( const uf::stl::string& type, const uf::stl::string& pipeline ) const {
#if !UF_USE_OPENGL_FIXED_FUNCTION
//	return !ext::json::isNull( metadata["shaders"][type] );
	return metadata.shaders.count(pipeline+":"+type) > 0;
#else
//	if ( ext::json::isNull( metadata["shaders"][type] ) ) return false;
	if ( metadata.shaders.count(pipeline+":"+type) == 0 ) return false;
	auto& shader = shaders.at(metadata.shaders.at(pipeline+":"+type));
	return (bool) shader.module;
#endif
}
ext::opengl::Shader& ext::opengl::Material::getShader( const uf::stl::string& type, const uf::stl::string& pipeline ) {
	UF_ASSERT( hasShader(type, pipeline) );
	return shaders.at( metadata.shaders[pipeline+":"+type] );
}
const ext::opengl::Shader& ext::opengl::Material::getShader( const uf::stl::string& type, const uf::stl::string& pipeline ) const {
	UF_ASSERT( hasShader(type, pipeline) );
	return shaders.at( metadata.shaders.at(pipeline+":"+type) );
}
bool ext::opengl::Material::validate() {
	bool was = true;
	for ( auto& shader : shaders ) if ( !shader.validate() ) was = false;
	return was;
}
ext::opengl::Graphic::~Graphic() {
	this->destroy();
}
void ext::opengl::Graphic::initialize( const uf::stl::string& renderModeName ) {
	RenderMode& renderMode = ext::opengl::getRenderMode(renderModeName, true);

	this->descriptor.renderMode = renderModeName;
	auto* device = renderMode.device;
	if ( !device ) device = &ext::opengl::device;

	material.initialize( *device );

	ext::opengl::Buffers::initialize( *device );

	// attach our "uniform"
	{
		pod::Uniform uniform;
		initializeBuffer(
			(void*) &uniform,
			sizeof(pod::Uniform),
			uf::renderer::enums::Buffer::UNIFORM
		);
	}
}
void ext::opengl::Graphic::initializePipeline() {
	initializePipeline( this->descriptor, false );
}
ext::opengl::Pipeline& ext::opengl::Graphic::initializePipeline( const GraphicDescriptor& descriptor, bool update ) {
	auto& pipeline = pipelines[descriptor.hash()];

//	auto previous = this->descriptor;
//	this->descriptor = descriptor;
	pipeline.initialize(*this, descriptor);
	pipeline.update(*this, descriptor);
//	if ( !update ) this->descriptor = previous;

	process = true;
	initialized = true;
	material.validate();

	return pipeline;
}
void ext::opengl::Graphic::initializeGeometry( const uf::BaseGeometry& mesh ) {
	// already generated, check if we can just update
	if ( descriptor.indices > 0 ) {
		if ( descriptor.geometry.attributes.vertex.size == mesh.attributes.vertex.size && descriptor.geometry.attributes.index.size == mesh.attributes.index.size && descriptor.indices == mesh.attributes.index.length ) {
			// too lazy to check if this equals, only matters in pipeline creation anyways
			descriptor.geometry = mesh;

			int32_t vertexBuffer = -1;
			int32_t indexBuffer = -1;
			for ( size_t i = 0; i < buffers.size(); ++i ) {
				if ( buffers[i].usage & uf::renderer::enums::Buffer::VERTEX ) vertexBuffer = i;
				if ( buffers[i].usage & uf::renderer::enums::Buffer::INDEX ) indexBuffer = i;
			}

			if ( vertexBuffer > 0 && indexBuffer > 0 ) {
				updateBuffer(
					(void*) mesh.attributes.vertex.pointer,
					mesh.attributes.vertex.size * mesh.attributes.vertex.length,
					vertexBuffer
				);
				updateBuffer(
					(void*) mesh.attributes.index.pointer,
					mesh.attributes.index.size * mesh.attributes.index.length,
					indexBuffer
				);
				return;
			}
		}
	}

	descriptor.geometry = mesh;
	descriptor.indices = mesh.attributes.index.length;

	initializeBuffer(
		(void*) mesh.attributes.vertex.pointer,
		mesh.attributes.vertex.size * mesh.attributes.vertex.length,
		uf::renderer::enums::Buffer::VERTEX
	);
	initializeBuffer(
		(void*) mesh.attributes.index.pointer,
		mesh.attributes.index.size * mesh.attributes.index.length,
		uf::renderer::enums::Buffer::INDEX
	);
}
bool ext::opengl::Graphic::hasPipeline( const GraphicDescriptor& descriptor ) const {
	return pipelines.count( descriptor.hash() ) > 0;
}
ext::opengl::Pipeline& ext::opengl::Graphic::getPipeline() {
	return getPipeline( descriptor );
}
ext::opengl::Pipeline& ext::opengl::Graphic::getPipeline( const GraphicDescriptor& descriptor ) {
	if ( !hasPipeline(descriptor) ) return initializePipeline( descriptor );
	return pipelines[descriptor.hash()];
}
const ext::opengl::Pipeline& ext::opengl::Graphic::getPipeline( const GraphicDescriptor& descriptor ) const {
	UF_ASSERT( hasPipeline(descriptor) ); //return initializePipeline( descriptor );
	return pipelines.at(descriptor.hash());
}
void ext::opengl::Graphic::updatePipelines() {
	for ( auto pair : this->pipelines ) {
		pair.second.update( *this );
	}
}
void ext::opengl::Graphic::record( CommandBuffer& commandBuffer, size_t pass, size_t draw ) const {
	return this->record( commandBuffer, descriptor, pass, draw );
}

void ext::opengl::Graphic::record( CommandBuffer& commandBuffer, const GraphicDescriptor& descriptor, size_t pass, size_t draw ) const {
	if ( !process ) return;

	Buffer::Descriptor vertexBuffer = {};
	Buffer::Descriptor indexBuffer = {};
	Buffer::Descriptor uniformBuffer = {};
	uf::stl::vector<Buffer::Descriptor> storageBuffers;

	for ( auto& buffer : buffers ) {
		if ( buffer.usage & uf::renderer::enums::Buffer::VERTEX ) { vertexBuffer = buffer.descriptor; }
		if ( buffer.usage & uf::renderer::enums::Buffer::INDEX ) { indexBuffer = buffer.descriptor; }
		if ( buffer.usage & uf::renderer::enums::Buffer::UNIFORM ) { uniformBuffer = buffer.descriptor; }
		if ( buffer.usage & uf::renderer::enums::Buffer::STORAGE ) { storageBuffers.emplace_back(buffer.descriptor); }
	}
	if ( !vertexBuffer.buffer || !indexBuffer.buffer ) return;

	uf::renderer::VertexDescriptor 	vertexAttributePosition, 
									vertexAttributeNormal,
									vertexAttributeColor,
									vertexAttributeUv,
									vertexAttributeSt,
									vertexAttributeId;

	for ( auto& attribute : descriptor.geometry.attributes.descriptor ) {
		if ( attribute.name == "position" ) vertexAttributePosition = attribute;
//		else if ( attribute.name == "normal" ) vertexAttributeNormal = attribute;
//		else if ( attribute.name == "color" ) vertexAttributeColor = attribute;
		else if ( attribute.name == "uv" ) vertexAttributeUv = attribute;
		else if ( attribute.name == "st" ) vertexAttributeSt = attribute;
		else if ( attribute.name == "id" ) vertexAttributeId = attribute;
	}

	if ( vertexAttributePosition.name == "" ) return;

	size_t indices = descriptor.indices;
	size_t indicesStride = descriptor.geometry.attributes.index.size;
	size_t vertexStride = descriptor.geometry.attributes.vertex.size;
	size_t vertices = vertexBuffer.range / vertexStride;
	void* vertexPointer = (void*) ( device->getBuffer( vertexBuffer.buffer ) + vertexBuffer.offset );
	void* indicesPointer = (void*) ( device->getBuffer( indexBuffer.buffer ) + indexBuffer.offset );

	CommandBuffer::InfoDraw drawCommandInfo = {};
	drawCommandInfo.type = ext::opengl::enums::Command::DRAW;
	drawCommandInfo.descriptor = descriptor;
	drawCommandInfo.vertexBuffer = vertexBuffer;
	drawCommandInfo.indexBuffer = indexBuffer;
	drawCommandInfo.uniformBuffer = uniformBuffer;

	if ( vertexAttributePosition.name != "" ) drawCommandInfo.attributes.position = vertexAttributePosition.offset;
	if ( vertexAttributeUv.name != "" ) drawCommandInfo.attributes.uv = vertexAttributeUv.offset;
	if ( vertexAttributeSt.name != "" ) drawCommandInfo.attributes.st = vertexAttributeSt.offset;
	if ( vertexAttributeNormal.name != "" ) drawCommandInfo.attributes.normal = vertexAttributeNormal.offset;
	if ( vertexAttributeColor.name != "" ) drawCommandInfo.attributes.color = vertexAttributeColor.offset;


	// split up our mesh by textures, if the right attribute exists
	if ( vertexAttributeId.name != "" ) {
		struct TextureMapping {
			size_t texture = 0;
			size_t offset = 0;
			size_t range = 0;
		};
		uf::stl::vector<TextureMapping> mappings;
		TextureMapping currentMapping;
	
		pod::Texture::Storage* storageBufferTextures = NULL;
		pod::Material::Storage* storageBufferMaterials = NULL;
		size_t storageBufferTexturesSize = 0;
		size_t storageBufferMaterialsSize = 0;

		if ( storageBuffers.size() >= 2 ) {
			Buffer::Descriptor storageBufferTexturesDescriptor = storageBuffers[storageBuffers.size() - 1];
			Buffer::Descriptor storageBufferMaterialsDescriptor = storageBuffers[storageBuffers.size() - 2];

			storageBufferTextures = (pod::Texture::Storage*) ( device->getBuffer( storageBufferTexturesDescriptor.buffer ) + storageBufferTexturesDescriptor.offset );
			storageBufferMaterials = (pod::Material::Storage*) ( device->getBuffer( storageBufferMaterialsDescriptor.buffer ) + storageBufferMaterialsDescriptor.offset );

			storageBufferTexturesSize = storageBufferTexturesDescriptor.range / sizeof(pod::Texture::Storage);
			storageBufferMaterialsSize = storageBufferMaterialsDescriptor.range / sizeof(pod::Material::Storage);
		}

		bool useLightmap = false;
		for ( size_t currentIndex = 0; currentIndex < indices; ++currentIndex ) {
		//	auto index = indicesPointer[currentIndex];
			uint32_t index = 0;
			void* indexSrc = indicesPointer + (currentIndex * indicesStride);
			switch ( indicesStride ) {
				case sizeof( uint8_t): index = *(( uint8_t*) indexSrc); break;
				case sizeof(uint16_t): index = *((uint16_t*) indexSrc); break;
				case sizeof(uint32_t): index = *((uint32_t*) indexSrc); break;
			}
			void* vertices = vertexPointer + (index * vertexStride);
			const pod::Vector2ui& id = *((pod::Vector2ui*) (vertices + vertexAttributeId.offset));
			// check if we're using a lightmap, having a lightmap means we have provided ST attributes
			if ( !useLightmap && vertexAttributeSt.name != "" ) {
				const pod::Vector2f& st = *((pod::Vector2f*) (vertices + vertexAttributeSt.offset));
				if ( st > pod::Vector2f{0,0} ) useLightmap = true;
			}
			size_t materialId = id.y;
			size_t textureId = id.y;
			if ( storageBufferMaterials && 0 <= materialId && materialId < storageBufferMaterialsSize ) {
				auto& material = storageBufferMaterials[materialId];
				size_t textureIndex = material.indexAlbedo;
				if ( 0 <= textureIndex && textureIndex < storageBufferTexturesSize ) {
					auto& texture = storageBufferTextures[textureIndex];
					if ( 0 <= texture.index && texture.index < this->material.textures.size() ) {
						textureId = texture.index;
					}
				}
			}
			if ( currentMapping.texture != textureId ) {
				if ( currentMapping.range > 0 ) mappings.emplace_back(currentMapping);
				currentMapping.offset += currentMapping.range;
				currentMapping.range = 0;
			}
			currentMapping.texture = textureId;
			++currentMapping.range;
		}
		if ( currentMapping.range > 0 ) mappings.emplace_back(currentMapping);
		// if we have ST values and an extra, unused texture, bind it
		if ( useLightmap && currentMapping.texture < material.textures.size() - 1 ) {
			drawCommandInfo.auxTexture = material.textures.back().descriptor;
		}
		for ( auto& mapping : mappings ) {
			if ( mapping.range == 0 || !( 0 <= mapping.texture && mapping.texture < material.textures.size()) ) continue;
			drawCommandInfo.texture = material.textures[mapping.texture].descriptor;
			drawCommandInfo.descriptor.indices = mapping.range;
			drawCommandInfo.indexBuffer.offset = indexBuffer.offset + sizeof(uf::renderer::index_t) * mapping.offset;
			drawCommandInfo.indexBuffer.range = indexBuffer.range + sizeof(uf::renderer::index_t) * mapping.range;
			commandBuffer.record(drawCommandInfo);
		}
	} else {
		drawCommandInfo.texture = material.textures.front().descriptor;
		commandBuffer.record(drawCommandInfo);
	}
}
void ext::opengl::Graphic::destroy() {
	for ( auto& pair : pipelines ) pair.second.destroy();
	pipelines.clear();

	material.destroy();

	ext::opengl::Buffers::destroy();

	ext::opengl::states::rebuild = true;
}

bool ext::opengl::Graphic::hasStorage( const uf::stl::string& name ) const {
	for ( auto& shader : material.shaders ) if ( shader.hasStorage(name) ) return true;
	return false;
}
ext::opengl::Buffer* ext::opengl::Graphic::getStorageBuffer( const uf::stl::string& name ) {
	size_t storageIndex = -1;
	for ( auto& shader : material.shaders ) {
		if ( !shader.hasStorage(name) ) continue;
	//	storageIndex = shader.metadata.json["definitions"]["storage"][name]["index"].as<size_t>();
		storageIndex = shader.metadata.definitions.storage[name].index;
		break;
	}
	for ( size_t bufferIndex = 0, storageCounter = 0; bufferIndex < buffers.size(); ++bufferIndex ) {
		if ( !(buffers[bufferIndex].usage & uf::renderer::enums::Buffer::STORAGE) ) continue;
		if ( storageCounter++ != storageIndex ) continue;
		return &buffers[bufferIndex];
	}
	return NULL;
}
uf::Serializer ext::opengl::Graphic::getStorageJson( const uf::stl::string& name, bool cache ) {
	for ( auto& shader : material.shaders ) {
		if ( !shader.hasStorage(name) ) continue;
		return shader.getStorageJson(name, cache);
	}
	return ext::json::null();
}
ext::opengl::userdata_t ext::opengl::Graphic::getStorageUserdata( const uf::stl::string& name, const ext::json::Value& payload ) {
	for ( auto& shader : material.shaders ) {
		if ( !shader.hasStorage(name) ) continue;
		return shader.getStorageUserdata(name, payload);
	}
	return ext::opengl::userdata_t();
}

ext::opengl::Buffer::Descriptor ext::opengl::Graphic::getUniform( size_t target ) const {
	size_t offset = 0;
	for ( auto& buffer : buffers ) {
		if ( !(buffer.usage & uf::renderer::enums::Buffer::UNIFORM) ) continue;
		if ( offset++ != target ) continue;
	//	return device->getBuffer( buffer.buffer );
		return buffer.descriptor;
	}
	return {};
}
void ext::opengl::Graphic::updateUniform( const void* data, size_t size, size_t target ) const {
	size_t offset = 0;
	for ( auto& buffer : buffers ) {
		if ( !(buffer.usage & uf::renderer::enums::Buffer::UNIFORM) ) continue;
		if ( offset++ != target ) continue;
		updateBuffer( data, size, buffer );
	}
}

ext::opengl::Buffer::Descriptor ext::opengl::Graphic::getStorage( size_t target ) const {
	size_t offset = 0;
	for ( auto& buffer : buffers ) {
		if ( !(buffer.usage & uf::renderer::enums::Buffer::STORAGE) ) continue;
		if ( offset++ != target ) continue;
	//	return device->getBuffer( buffer.buffer );
		return buffer.descriptor;
	}
	return {};
}
void ext::opengl::Graphic::updateStorage( const void* data, size_t size, size_t target ) const {
	size_t offset = 0;
	for ( auto& buffer : buffers ) {
		if ( !(buffer.usage & uf::renderer::enums::Buffer::STORAGE) ) continue;
		if ( offset++ != target ) continue;
		updateBuffer( data, size, buffer );
	}
}

#include <uf/utils/string/hash.h>
void ext::opengl::GraphicDescriptor::parse( ext::json::Value& metadata ) {
	if ( metadata["front face"].is<uf::stl::string>() ) {
		if ( metadata["front face"].as<uf::stl::string>() == "ccw" ) {
			frontFace = uf::renderer::enums::Face::CCW;
		} else if ( metadata["front face"].as<uf::stl::string>() == "cw" ) {
			frontFace = uf::renderer::enums::Face::CW;
		}
	}
	if ( metadata["cull mode"].is<uf::stl::string>() ) {
		if ( metadata["cull mode"].as<uf::stl::string>() == "back" ) {
			cullMode = uf::renderer::enums::CullMode::BACK;
		} else if ( metadata["cull mode"].as<uf::stl::string>() == "front" ) {
			cullMode = uf::renderer::enums::CullMode::FRONT;
		} else if ( metadata["cull mode"].as<uf::stl::string>() == "none" ) {
			cullMode = uf::renderer::enums::CullMode::NONE;
		} else if ( metadata["cull mode"].as<uf::stl::string>() == "both" ) {
			cullMode = uf::renderer::enums::CullMode::BOTH;
		}
	}

	if ( ext::json::isObject(metadata["depth test"]) ) {
		if ( metadata["depth test"]["test"].is<bool>() ) depth.test = metadata["depth test"]["test"].as<bool>();
		if ( metadata["depth test"]["write"].is<bool>() ) depth.write = metadata["depth test"]["write"].as<bool>();
	}

	if ( metadata["indices"].is<size_t>() ) {
		indices = metadata["indices"].as<size_t>();
	}
	if ( ext::json::isObject( metadata["offsets"] ) ) {
		offsets.vertex = metadata["offsets"]["vertex"].as<size_t>();
		offsets.index = metadata["offsets"]["index"].as<size_t>();
	}
	if ( ext::json::isObject(metadata["depth bias"]) ) {
		depth.bias.enable = true;
		depth.bias.constant = metadata["depth bias"]["constant"].as<float>();
		depth.bias.slope = metadata["depth bias"]["slope"].as<float>();
		depth.bias.clamp = metadata["depth bias"]["clamp"].as<float>();
	}
}
ext::opengl::GraphicDescriptor::hash_t ext::opengl::GraphicDescriptor::hash() const {
#if UF_GRAPHIC_DESCRIPTOR_USE_STRING
	uf::Serializer serializer;

	serializer["subpass"] = subpass;
	if ( settings::experimental::individualPipelines )
		serializer["renderMode"] = renderMode;

	serializer["renderTarget"] = renderTarget;
	serializer["geometry"]["sizes"]["vertex"] = geometry.attributes.vertex.size;
	serializer["geometry"]["sizes"]["indices"] = geometry.attributes.index.size;
	serializer["indices"] = indices;
	serializer["offsets"]["vertex"] = offsets.vertex;
	serializer["offsets"]["index"] = offsets.index;

	for ( uint8_t i = 0; i < geometry.attributes.descriptor.size(); ++i ) {
		serializer["geometry"]["attributes"][i]["format"] = geometry.attributes.descriptor[i].format;
		serializer["geometry"]["attributes"][i]["offset"] = geometry.attributes.descriptor[i].offset;
	}

	serializer["topology"] = topology;
	serializer["cullMode"] = cullMode;
	serializer["fill"] = fill;
	serializer["lineWidth"] = lineWidth;
	serializer["frontFace"] = frontFace;
	serializer["depth"]["test"] = depth.test;
	serializer["depth"]["write"] = depth.write;
	serializer["depth"]["operation"] = depth.operation;
	serializer["depth"]["bias"]["enable"] = depth.bias.enable;
	serializer["depth"]["bias"]["constant"] = depth.bias.constant;
	serializer["depth"]["bias"]["slope"] = depth.bias.slope;
	serializer["depth"]["bias"]["clamp"] = depth.bias.clamp;

	return uf::string::sha256( serializer.serialize() );
//	return serializer.dump();
#else
	size_t hash{};

	hash += std::hash<decltype(subpass)>{}(subpass);
	if ( settings::experimental::individualPipelines )
		hash += std::hash<decltype(renderMode)>{}(renderMode);
	
	hash += std::hash<decltype(renderTarget)>{}(renderTarget);
	hash += std::hash<decltype(geometry.attributes.vertex.size)>{}(geometry.attributes.vertex.size);
	hash += std::hash<decltype(geometry.attributes.index.size)>{}(geometry.attributes.index.size);
	hash += std::hash<decltype(indices)>{}(indices);
	hash += std::hash<decltype(offsets.vertex)>{}(offsets.vertex);
	hash += std::hash<decltype(offsets.index)>{}(offsets.index);

	for ( uint8_t i = 0; i < geometry.attributes.descriptor.size(); ++i ) {
		hash += std::hash<decltype(geometry.attributes.descriptor[i].format)>{}(geometry.attributes.descriptor[i].format);
		hash += std::hash<decltype(geometry.attributes.descriptor[i].offset)>{}(geometry.attributes.descriptor[i].offset);
	}

	hash += std::hash<decltype(topology)>{}(topology);
	hash += std::hash<decltype(cullMode)>{}(cullMode);
	hash += std::hash<decltype(fill)>{}(fill);
	hash += std::hash<decltype(lineWidth)>{}(lineWidth);
	hash += std::hash<decltype(frontFace)>{}(frontFace);
	hash += std::hash<decltype(depth.test)>{}(depth.test);
	hash += std::hash<decltype(depth.write)>{}(depth.write);
	hash += std::hash<decltype(depth.operation)>{}(depth.operation);
	hash += std::hash<decltype(depth.bias.enable)>{}(depth.bias.enable);
	hash += std::hash<decltype(depth.bias.constant)>{}(depth.bias.constant);
	hash += std::hash<decltype(depth.bias.slope)>{}(depth.bias.slope);
	hash += std::hash<decltype(depth.bias.clamp)>{}(depth.bias.clamp);

	return hash;
#endif
}

ext::json::Value ext::opengl::definitionToJson(/*const*/ ext::json::Value& definition ) {
	ext::json::Value member;
	// is object
	if ( !ext::json::isNull(definition["members"]) ) {
		ext::json::forEach(definition["members"], [&](/*const*/ ext::json::Value& value){
			uf::stl::string key = uf::string::split(value["name"].as<uf::stl::string>(), " ").back();
			member[key] = ext::opengl::definitionToJson(value);
		});
	// is primitive
	} else if ( !ext::json::isNull(definition["value"]) ) {
		// is array of structs
		if ( definition["struct"].as<bool>() ) {
			ext::json::forEach(definition["value"], [&](/*const*/ ext::json::Value& value){
				ext::json::Value parsed;
				parsed["name"] = definition["name"];
				parsed["size"] = definition["size"];
				parsed["struct"] = definition["struct"];
				parsed["members"] = value;
				parsed = ext::opengl::definitionToJson(parsed);
				member.emplace_back(parsed);
			});
		} else {
			member = definition["value"];
		}
	}
	return member;
}
ext::opengl::userdata_t ext::opengl::jsonToUserdata( const ext::json::Value& payload, const ext::json::Value& definition ) {
	size_t bufferLen = definition["size"].as<size_t>();
	
	ext::opengl::userdata_t userdata;
	userdata.create(bufferLen);

	uint8_t* byteBuffer = (uint8_t*) (void*) userdata;
	uint8_t* byteBufferStart = byteBuffer;
	uint8_t* byteBufferEnd = byteBuffer + bufferLen;

#if UF_JSON_NLOHMANN_ORDERED
	// JSON is ordered, we can just push directly
#define UF_SHADER_TRACK_NAMES 0
#if UF_SHADER_TRACK_NAMES
	uf::stl::vector<uf::stl::string> variableName;	
#endif
	std::function<void(const ext::json::Value&)> parse = [&]( const ext::json::Value& value ){
		// is array or object
	#if UF_SHADER_TRACK_NAMES
		if ( ext::json::isObject(value) ) {
			ext::json::forEach(value, [&]( const uf::stl::string& name, const ext::json::Value& member ){
			#if UF_SHADER_TRACK_NAMES
				variableName.emplace_back(name);
			#endif
				parse(member);
			});
			#if UF_SHADER_TRACK_NAMES
				if ( !variableName.empty() ) variableName.pop_back();
			#endif
			return;
		}
		if ( ext::json::isArray(value) ) {
			ext::json::forEach(value, [&]( size_t i, const ext::json::Value& element ){
				variableName.emplace_back("["+std::to_string(i)+"]");
				parse(element);
			});
			#if UF_SHADER_TRACK_NAMES
				if ( !variableName.empty() ) variableName.pop_back();
			#endif
			return;
		}
	#else
		if ( ext::json::isArray(value) || ext::json::isObject(value) ) {
			ext::json::forEach(value, parse);
			return;
		}
	#endif
	#if UF_SHADER_TRACK_NAMES
		uf::stl::string path = uf::string::join(variableName, ".");
		path = uf::string::replace( path, ".[", "[" );
		GL_VALIDATION_MESSAGE("[" << (byteBuffer - byteBufferStart) << " / "<< (byteBufferEnd - byteBuffer) <<"]\tInserting: " << path << " = " << value.dump());
	#endif
		// is strictly an int
		if ( value.is<int>(true) ) {
			size_t size = sizeof(int32_t);
			auto get = value.as<int32_t>();
			memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		// is strictly an unsigned int
		} else if ( value.is<size_t>(true) ) {
			size_t size = sizeof(uint32_t);
			auto get = value.as<uint32_t>();
			memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		// is strictly a float
		} else if ( value.is<float>(true) ) {
			size_t size = sizeof(float);
			auto get = value.as<float>();
			memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		}
	#if UF_SHADER_TRACK_NAMES
		if ( !variableName.empty() ) variableName.pop_back();
	#endif
	};
#if UF_SHADER_TRACK_NAMES
	GL_VALIDATION_MESSAGE("Updating " << name << " in " << filename);
	GL_VALIDATION_MESSAGE("Iterator: " << (void*) byteBuffer << "\t" << (void*) byteBufferEnd << "\t" << (byteBufferEnd - byteBuffer));
#endif
	parse(payload);
#if UF_SHADER_TRACK_NAMES
	GL_VALIDATION_MESSAGE("Iterator: " << (void*) byteBuffer << "\t" << (void*) byteBufferEnd << "\t" << (byteBufferEnd - byteBuffer));
#endif

#else
	auto pushValue = [&]( const uf::stl::string& primitive, const ext::json::Value& input ){
		if ( primitive == "bool" ) {
			size_t size = sizeof(bool); // v["size"].as<size_t>();
			if ( byteBufferEnd < byteBuffer + size ) return false; // overflow
			auto get = input.as<bool>();
			memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		} else if ( primitive == "int8_t" ) {
			size_t size = sizeof(int8_t); // v["size"].as<size_t>();
			if ( byteBufferEnd < byteBuffer + size ) return false; // overflow
	
		//	auto get = input.as<int8_t>();
		//	memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		} else if ( primitive == "uint8_t" ) {
			size_t size = sizeof(uint8_t); // v["size"].as<size_t>();
			if ( byteBufferEnd < byteBuffer + size ) return false; // overflow
		//	auto get = input.as<uint8_t>();
		//	memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		} else if ( primitive == "int16_t" ) {
			size_t size = sizeof(int16_t); // v["size"].as<size_t>();
			if ( byteBufferEnd < byteBuffer + size ) return false; // overflow
		//	auto get = input.as<int16_t>();
		//	memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		} else if ( primitive == "uint16_t" ) {
			size_t size = sizeof(uint16_t); // v["size"].as<size_t>();
			if ( byteBufferEnd < byteBuffer + size ) return false; // overflow
		//	auto get = input.as<uint16_t>();
		//	memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		} else if ( primitive == "int32_t" ) {
			size_t size = sizeof(int32_t); // v["size"].as<size_t>();
			if ( byteBufferEnd < byteBuffer + size ) return false; // overflow
			auto get = input.as<int32_t>();
			memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		} else if ( primitive == "uint32_t" ) {
			size_t size = sizeof(uint32_t); // v["size"].as<size_t>();
			if ( byteBufferEnd < byteBuffer + size ) return false; // overflow
			auto get = input.as<int32_t>();
			memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		} else if ( primitive == "int64_t" ) {
			size_t size = sizeof(int64_t); // v["size"].as<size_t>();
			if ( byteBufferEnd < byteBuffer + size ) return false; // overflow
			auto get = input.as<uint64_t>();
			memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		} else if ( primitive == "uint64_t" ) {
			size_t size = sizeof(uint64_t); // v["size"].as<size_t>();
			if ( byteBufferEnd < byteBuffer + size ) return false; // overflow
			auto get = input.as<uint64_t>();
			memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		} else if ( primitive == "half" ) {
			size_t size = sizeof(float); // v["size"].as<size_t>();
			if ( byteBufferEnd < byteBuffer + size ) return false; // overflow
		//	auto get = input.as<float>();
		//	memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		} else if ( primitive == "float" ) {
			size_t size = sizeof(float); // v["size"].as<size_t>();
			if ( byteBufferEnd < byteBuffer + size ) return false; // overflow
			auto get = input.as<float>();
			memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		} else if ( primitive == "double" ) {
			auto get = input.as<double>();
			size_t size = sizeof(double); // v["size"].as<size_t>();
			if ( byteBufferEnd < byteBuffer + size ) return false; // overflow
			memcpy( byteBuffer, &get, size );
			byteBuffer += size;
		}
		return true;
	};
	#define UF_SHADER_TRACK_NAMES 0
	#if UF_SHADER_TRACK_NAMES
		bool SKIP_ADD = false;
		uf::stl::vector<uf::stl::string> variableName;
	#endif
	std::function<void(const ext::json::Value&, const ext::json::Value&)> parseDefinition = [&](const ext::json::Value& input, const ext::json::Value& definition ){
	#if UF_SHADER_TRACK_NAMES
		if ( SKIP_ADD ) {
			SKIP_ADD = false;
		} else {
			auto split = uf::string::split(definition["name"].as<uf::stl::string>(), " ");
			uf::stl::string type = split.front();
			uf::stl::string name = split.back();
			variableName.emplace_back(name);
		}
	#endif
		// is object
		if ( !ext::json::isNull(definition["members"]) ) {
			ext::json::forEach(definition["members"], [&](const ext::json::Value& member){
				uf::stl::string key = uf::string::split(member["name"].as<uf::stl::string>(), " ").back();
				parseDefinition(input[key], member);
			});
		// is array or primitive
		} else if ( !ext::json::isNull(definition["value"]) ) {
			// is object
			auto split = uf::string::split(definition["name"].as<uf::stl::string>(), " ");
			uf::stl::string type = split.front();
			uf::stl::string name = split.back();
			std::regex regex("^(?:(.+?)\\<)?(.+?)(?:\\>)?(?:\\[(\\d+)\\])?$");
			std::smatch match;
			if ( !std::regex_search( type, match, regex ) ) {
				std::cout << "Ill formatted typename: " << definition["name"].as<uf::stl::string>() << std::endl;
				return;
			}
			uf::stl::string vectorMatrix = match[1].str();
			uf::stl::string primitive = match[2].str();
			uf::stl::string arraySize = match[3].str();	
			if ( ext::json::isObject(input) ) {
				ext::json::Value cloned;
				cloned["name"] = definition["name"];
				cloned["size"] = definition["size"].as<size_t>() / definition["value"].size();
				cloned["members"] = definition["value"][0];
				parseDefinition( input, cloned );
			}
			// is array
			else if ( ext::json::isArray(input) ) {
				ext::json::forEach( input, [&]( size_t i, const ext::json::Value& value){
				#if UF_SHADER_TRACK_NAMES
					variableName.emplace_back("["+std::to_string(i)+"]");
					SKIP_ADD = true;
				#endif
					parseDefinition(input[i], definition);
				});
			}
			// is primitive
			else {
			#if UF_SHADER_TRACK_NAMES
				uf::stl::string path = uf::string::join(variableName, ".");
				path = uf::string::replace( path, ".[", "[" );
				GL_VALIDATION_MESSAGE("[" << (byteBuffer - byteBufferStart) << " / "<< (byteBufferEnd - byteBuffer) <<"]\tInserting: " << path << " = (" << primitive << ") " << input.dump());
			#endif
				pushValue( primitive, input );
			}
		}
	#if UF_SHADER_TRACK_NAMES
		if ( !variableName.empty() ) variableName.pop_back();
	#endif
	};
	//auto& definitions = metadata["definitions"]["uniforms"][name];
#if UF_SHADER_TRACK_NAMES
	GL_VALIDATION_MESSAGE("Updating " << name << " in " << filename);
	GL_VALIDATION_MESSAGE("Iterator: " << (void*) byteBuffer << "\t" << (void*) byteBufferEnd << "\t" << (byteBufferEnd - byteBuffer));
#endif
	parseDefinition(payload, definition);
#if UF_SHADER_TRACK_NAMES
	GL_VALIDATION_MESSAGE("Iterator: " << (void*) byteBuffer << "\t" << (void*) byteBufferEnd << "\t" << (byteBufferEnd - byteBuffer));
#endif
#endif
	return userdata;
}

#endif