#if UF_USE_OPENGL

#include <uf/ext/opengl/graphic.h>
#include <uf/ext/opengl/initializers.h>
#include <uf/ext/opengl/opengl.h>
#include <uf/ext/opengl/commands.h>
#include <uf/ext/openvr/openvr.h>
#include <uf/engine/graph/graph.h>
#include <uf/utils/camera/camera.h>

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
		for ( size_t i = 0; i < descriptor.inputs.vertex.attributes.size(); ++i ) {
			auto& attribute = descriptor.inputs.vertex.attributes[i];

			GL_ERROR_CHECK(glEnableVertexAttribArray(i));
			GL_ERROR_CHECK(glVertexAttribPointer(0, attribute.descriptor.components, attribute.descriptor.type, false, descriptor.inputs.vertex.size, attribute.descriptor.offset));
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
	return record( graphic, descriptor, commandBuffer, pass, draw );
#endif
}
void ext::opengl::Pipeline::record( const Graphic& graphic, const GraphicDescriptor& descriptor, CommandBuffer& commandBuffer, size_t pass, size_t draw ) const {
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
uf::stl::vector<ext::opengl::Shader*> ext::opengl::Pipeline::getShaders( uf::stl::vector<ext::opengl::Shader>& shaders ) {
	uf::stl::unordered_map<uf::stl::string, ext::opengl::Shader*> map;
	uf::stl::vector<ext::opengl::Shader*> res;
	bool isCompute = false;
	for ( auto& shader : shaders ) {
		if ( shader.metadata.pipeline != "" && shader.metadata.pipeline != metadata.type ) continue;
	//	if ( shader.descriptor.stage == VK_SHADER_STAGE_COMPUTE_BIT ) isCompute = true;
	}
	for ( auto& shader : shaders ) {
		if ( shader.metadata.pipeline != "" && shader.metadata.pipeline != metadata.type ) continue;
	//	if ( isCompute && shader.descriptor.stage != VK_SHADER_STAGE_COMPUTE_BIT ) continue;
		map[shader.metadata.type] = &shader;
	}
	for ( auto pair : map ) res.insert( res.begin(), pair.second);
	return res;
}
uf::stl::vector<const ext::opengl::Shader*> ext::opengl::Pipeline::getShaders( const uf::stl::vector<ext::opengl::Shader>& shaders ) const {
	uf::stl::unordered_map<uf::stl::string, const ext::opengl::Shader*> map;
	uf::stl::vector<const ext::opengl::Shader*> res;
	bool isCompute = false;
	for ( auto& shader : shaders ) {
		if ( shader.metadata.pipeline != "" && shader.metadata.pipeline != metadata.type ) continue;
	//	if ( shader.descriptor.stage == VK_SHADER_STAGE_COMPUTE_BIT ) isCompute = true;
	}
	for ( auto& shader : shaders ) {
		if ( shader.metadata.pipeline != "" && shader.metadata.pipeline != metadata.type ) continue;
	//	if ( isCompute && shader.descriptor.stage != VK_SHADER_STAGE_COMPUTE_BIT ) continue;
		map[shader.metadata.type] = &shader;
	}
	for ( auto pair : map ) res.insert( res.begin(), pair.second);
	return res;
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
	shader.metadata.autoInitializeUniforms = metadata.autoInitializeUniforms;
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
#if 1
	return metadata.shaders.count(pipeline+":"+type) > 0;
#else
#if !UF_USE_OPENGL_FIXED_FUNCTION
//	return !ext::json::isNull( metadata["shaders"][type] );
	return metadata.shaders.count(pipeline+":"+type) > 0;
#else
//	if ( ext::json::isNull( metadata["shaders"][type] ) ) return false;
	if ( metadata.shaders.count(pipeline+":"+type) == 0 ) return false;
	auto& shader = shaders.at(metadata.shaders.at(pipeline+":"+type));
	return (bool) shader.module;
#endif
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
void ext::opengl::Graphic::initializeMesh( uf::Mesh& mesh, bool buffer ) {
	// generate indices if not found
//	if ( mesh.index.count == 0 ) mesh.generateIndices();
	// generate indirect data if not found
//	if ( mesh.indirect.count == 0  ) mesh.generateIndirect();
	// ensure our descriptors are proper
	mesh.updateDescriptor();

	// copy descriptors
	descriptor.inputs.vertex = mesh.vertex;
	descriptor.inputs.index = mesh.index;
	descriptor.inputs.instance = mesh.instance;
	descriptor.inputs.indirect = mesh.indirect;

	bool alias = false;
	buffer = false;
	// create buffer if not set and requested
	if ( buffer ) {
		// ensures each buffer index reflects nicely
		struct Queue {
			void* data;
			size_t size;
			GLenum usage;
		};
		uf::stl::vector<Queue> queue;
		descriptor.inputs.bufferOffset = buffers.empty() ? 0 : buffers.size() - 1;

		#define PARSE_ATTRIBUTE(i, usage) {\
			auto& buffer = mesh.buffers[i];\
			if ( queue.size() <= i ) queue.resize( i );\
			if ( !buffer.empty() ) queue.emplace_back(Queue{ (void*) buffer.data(), buffer.size(), usage });\
		}
		#define PARSE_INPUT(name, usage){\
			if ( mesh.isInterleaved( mesh.name.interleaved ) ) PARSE_ATTRIBUTE(descriptor.inputs.name.interleaved, usage)\
			else for ( auto& attribute : descriptor.inputs.name.attributes ) PARSE_ATTRIBUTE(attribute.buffer, usage)\
		}

		PARSE_INPUT(vertex, uf::renderer::enums::Buffer::VERTEX)
		PARSE_INPUT(index, uf::renderer::enums::Buffer::INDEX)
		PARSE_INPUT(instance, uf::renderer::enums::Buffer::VERTEX)
		PARSE_INPUT(indirect, uf::renderer::enums::Buffer::INDIRECT)

		// allocate buffers
		for ( auto i = 0; i < queue.size(); ++i ) {
			auto& q = queue[i];
			initializeBuffer( q.data, q.size, q.usage, alias );
		}
	}

	if ( mesh.instance.count == 0 && mesh.instance.attributes.empty() ) {
		descriptor.inputs.instance.count = 1;
	}
}
bool ext::opengl::Graphic::updateMesh( uf::Mesh& mesh ) {
	// generate indices if not found
//	if ( mesh.index.count == 0 ) mesh.generateIndices();
	// generate indirect data if not found
//	if ( mesh.indirect.count == 0  ) mesh.generateIndirect();
	// ensure our descriptors are proper
	mesh.updateDescriptor();

	// copy descriptors
	descriptor.inputs.vertex = mesh.vertex;
	descriptor.inputs.index = mesh.index;
	descriptor.inputs.instance = mesh.instance;
	descriptor.inputs.indirect = mesh.indirect;

	// create buffer if not set and requested
	// ensures each buffer index reflects nicely
	struct Queue {
		void* data;
		size_t size;
		uf::renderer::enums::Buffer::type_t usage;
	};
	uf::stl::vector<Queue> queue;

	#define PARSE_ATTRIBUTE(i, usage) {\
		auto& buffer = mesh.buffers[i];\
		if ( queue.size() <= i ) queue.resize( i );\
		if ( !buffer.empty() ) queue.emplace_back(Queue{ (void*) buffer.data(), buffer.size(), usage });\
	}
	#define PARSE_INPUT(name, usage){\
		if ( mesh.isInterleaved( mesh.name.interleaved ) ) PARSE_ATTRIBUTE(descriptor.inputs.name.interleaved, usage)\
		else for ( auto& attribute : descriptor.inputs.name.attributes ) PARSE_ATTRIBUTE(attribute.buffer, usage)\
	}

	PARSE_INPUT(vertex, uf::renderer::enums::Buffer::VERTEX)
	PARSE_INPUT(index, uf::renderer::enums::Buffer::INDEX)
	PARSE_INPUT(instance, uf::renderer::enums::Buffer::VERTEX)
	PARSE_INPUT(indirect, uf::renderer::enums::Buffer::INDIRECT)

	bool rebuild = false;
	// allocate buffers
	for ( auto i = 0; i < queue.size(); ++i ) {
		auto& q = queue[i];
		rebuild = rebuild || updateBuffer( q.data, q.size, descriptor.inputs.bufferOffset + i, true );
	}

	if ( mesh.instance.count == 0 && mesh.instance.attributes.empty() ) {
		descriptor.inputs.instance.count = 1;
	}
	return rebuild;
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
	uf::stl::vector<ext::opengl::Buffer::Descriptor> uniformBuffers;
	uf::stl::vector<ext::opengl::Buffer::Descriptor> storageBuffers;

	auto& pipeline = this->getPipeline( descriptor );
	auto shaders = pipeline.getShaders( material.shaders );

	for ( auto shader : shaders ) {
		for ( auto& buffer : shader->buffers ) {
			if ( buffer.usage & uf::renderer::enums::Buffer::UNIFORM ) { uniformBuffers.emplace_back(buffer.descriptor); }
			if ( buffer.usage & uf::renderer::enums::Buffer::STORAGE ) { storageBuffers.emplace_back(buffer.descriptor); }
		}
	}

	auto uniformBufferIt = uniformBuffers.begin();
	auto uniformBuffer = (*uniformBufferIt).buffer;
	auto uniformBufferSize = (*uniformBufferIt).range;
	pod::Camera::Viewports* viewports = (pod::Camera::Viewports*) device->getBuffer( uniformBuffer );
	pod::Uniform* viewports2 = (pod::Uniform*) device->getBuffer( uniformBuffer );

	CommandBuffer::InfoDraw drawCommandInfoBase = {};
	drawCommandInfoBase.type = ext::opengl::enums::Command::DRAW;
	drawCommandInfoBase.descriptor = descriptor;
	if ( descriptor.inputs.index.count ) {
		drawCommandInfoBase.attributes.index = descriptor.inputs.index.attributes.front();
	}

	for ( uf::Mesh::Attribute attribute : descriptor.inputs.vertex.attributes ) {
		if ( attribute.descriptor.name == "position" ) drawCommandInfoBase.attributes.position = attribute;
		else if ( attribute.descriptor.name == "uv" ) drawCommandInfoBase.attributes.uv = attribute;
		else if ( attribute.descriptor.name == "st" ) drawCommandInfoBase.attributes.st = attribute;
	//	else if ( attribute.descriptor.name == "normal" ) drawCommandInfoBase.attributes.normal = attribute;
		else if ( attribute.descriptor.name == "color" ) drawCommandInfoBase.attributes.color = attribute;
	}

	if ( uniformBufferSize == sizeof(pod::Camera::Viewports) ) {
		drawCommandInfoBase.matrices.view = &viewports->matrices[0].view;
		drawCommandInfoBase.matrices.projection = &viewports->matrices[0].projection;
	} else if ( uniformBufferSize == sizeof(pod::Uniform) ) {
		drawCommandInfoBase.matrices.model = &viewports2->modelView;
		drawCommandInfoBase.matrices.projection = &viewports2->projection;
	}

	if ( descriptor.inputs.indirect.count ) {
		auto& indirectAttribute = descriptor.inputs.indirect.attributes.front();

		auto storageBufferIt = storageBuffers.begin();

		storageBufferIt++;
		storageBufferIt++;
		
		auto instanceBuffer = (*storageBufferIt++).buffer;
		auto materialBuffer = (*storageBufferIt++).buffer;
		auto textureBuffer = (*storageBufferIt++).buffer;

		pod::DrawCommand* drawCommands = (pod::DrawCommand*) indirectAttribute.pointer;
		pod::Instance* instances = (pod::Instance*) device->getBuffer( instanceBuffer );
		pod::Material* materials = (pod::Material*) device->getBuffer( materialBuffer );
		pod::Texture* textures = (pod::Texture*) device->getBuffer( textureBuffer );

	//	const bool optimize = false;
		uf::stl::unordered_map<size_t, uf::stl::vector<CommandBuffer::InfoDraw>> pool;
		for ( auto i = 0; i < descriptor.inputs.indirect.count; ++i ) {
			auto& drawCommand = drawCommands[i];
			auto instanceID = drawCommand.instanceID;
			auto& instance = instances[instanceID];
			auto materialID = instance.materialID;
			auto& material = materials[materialID];
			auto textureID = material.indexAlbedo;
			auto& infos = pool[textureID];
			CommandBuffer::InfoDraw& drawCommandInfo = infos.emplace_back( drawCommandInfoBase );
		//	CommandBuffer::InfoDraw drawCommandInfo = drawCommandInfoBase;

			drawCommandInfo.descriptor.inputs.index.first = drawCommand.indexID;
			drawCommandInfo.descriptor.inputs.index.count = drawCommand.indices;
			
			drawCommandInfo.descriptor.inputs.vertex.first = drawCommand.vertexID;
			drawCommandInfo.descriptor.inputs.vertex.count = drawCommand.vertices;

			drawCommandInfo.attributes.instance.pointer = &instance;
			drawCommandInfo.attributes.instance.length = sizeof(instance);

			drawCommandInfo.attributes.indirect.pointer = &drawCommand;
			drawCommandInfo.attributes.indirect.length = sizeof(drawCommand);

			drawCommandInfo.matrices.model = &instance.model;
			
			if ( 0 <= material.indexAlbedo ) {
				auto texture2DID = textures[material.indexAlbedo].index;
				drawCommandInfo.textures.primary = this->material.textures.at(texture2DID).descriptor;
			}
			if ( 0 <= instance.lightmapID ) {
				auto textureID = instance.lightmapID;
				auto texture2DID = textures[instance.lightmapID].index;
				drawCommandInfo.textures.secondary = this->material.textures.at(texture2DID).descriptor;
			}
			commandBuffer.record(drawCommandInfo);
		}
	} else {		
		CommandBuffer::InfoDraw drawCommandInfo = drawCommandInfoBase;

		drawCommandInfoBase.matrices.model = NULL;
		drawCommandInfoBase.matrices.view = NULL;
		drawCommandInfoBase.matrices.projection = NULL;

		if ( !material.textures.empty() ) {
			drawCommandInfo.textures.primary = material.textures.front().descriptor;
		}

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

#include <uf/utils/string/hash.h>
void ext::opengl::GraphicDescriptor::parse( ext::json::Value& metadata ) {
	if ( metadata["front face"].is<uf::stl::string>() ) {
		if ( metadata["front face"].as<uf::stl::string>() == "ccw" ) frontFace = uf::renderer::enums::Face::CCW;
		else if ( metadata["front face"].as<uf::stl::string>() == "cw" ) frontFace = uf::renderer::enums::Face::CW;
	}
	if ( metadata["cull mode"].is<uf::stl::string>() ) {
		if ( metadata["cull mode"].as<uf::stl::string>() == "back" ) cullMode = uf::renderer::enums::CullMode::BACK;
		else if ( metadata["cull mode"].as<uf::stl::string>() == "front" ) cullMode = uf::renderer::enums::CullMode::FRONT;
		else if ( metadata["cull mode"].as<uf::stl::string>() == "none" ) cullMode = uf::renderer::enums::CullMode::NONE;
		else if ( metadata["cull mode"].as<uf::stl::string>() == "both" ) cullMode = uf::renderer::enums::CullMode::BOTH;
	}

	if ( ext::json::isObject(metadata["depth bias"]) ) {
		depth.bias.enable = true;
		depth.bias.constant = metadata["depth bias"]["constant"].as<float>();
		depth.bias.slope = metadata["depth bias"]["slope"].as<float>();
		depth.bias.clamp = metadata["depth bias"]["clamp"].as<float>();
	}
}
ext::opengl::GraphicDescriptor::hash_t ext::opengl::GraphicDescriptor::hash() const {
	size_t hash{};

	hash += std::hash<decltype(subpass)>{}(subpass);
	if ( settings::invariant::individualPipelines )
		hash += std::hash<decltype(renderMode)>{}(renderMode);
	
	hash += std::hash<decltype(renderTarget)>{}(renderTarget);
	hash += std::hash<decltype(pipeline)>{}(pipeline);

	for ( auto i = 0; i < inputs.vertex.attributes.size(); ++i ) {
		hash += std::hash<decltype(inputs.vertex.attributes[i].descriptor.format)>{}(inputs.vertex.attributes[i].descriptor.format);
		hash += std::hash<decltype(inputs.vertex.attributes[i].descriptor.offset)>{}(inputs.vertex.attributes[i].descriptor.offset);
	}
	for ( auto i = 0; i < inputs.index.attributes.size(); ++i ) {
		hash += std::hash<decltype(inputs.index.attributes[i].descriptor.format)>{}(inputs.index.attributes[i].descriptor.format);
		hash += std::hash<decltype(inputs.index.attributes[i].descriptor.offset)>{}(inputs.index.attributes[i].descriptor.offset);
	}
	for ( auto i = 0; i < inputs.instance.attributes.size(); ++i ) {
		hash += std::hash<decltype(inputs.instance.attributes[i].descriptor.format)>{}(inputs.instance.attributes[i].descriptor.format);
		hash += std::hash<decltype(inputs.instance.attributes[i].descriptor.offset)>{}(inputs.instance.attributes[i].descriptor.offset);
	}
/*
	for ( auto i = 0; i < inputs.indirect.attributes.size(); ++i ) {
		hash += std::hash<decltype(inputs.indirect.attributes[i].descriptor.format)>{}(inputs.indirect.attributes[i].descriptor.format);
		hash += std::hash<decltype(inputs.indirect.attributes[i].descriptor.offset)>{}(inputs.indirect.attributes[i].descriptor.offset);
	}
*/

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
}
#endif