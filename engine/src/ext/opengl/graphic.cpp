#if UF_USE_OPENGL

#include <uf/ext/opengl/graphic.h>
#include <uf/ext/opengl/initializers.h>
#include <uf/ext/opengl/opengl.h>
#include <uf/ext/openvr/openvr.h>

#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include <fstream>
#include <regex>

#define VK_DEBUG_VALIDATION_MESSAGE(x)\
	//VK_VALIDATION_MESSAGE(x);

namespace {
	uint32_t VERTEX_BUFFER_BIND_ID = 0;
}
ext::json::Value ext::opengl::definitionToJson(/*const*/ ext::json::Value& definition ) {
	ext::json::Value member;
	// is object
	if ( !ext::json::isNull(definition["members"]) ) {
		ext::json::forEach(definition["members"], [&](/*const*/ ext::json::Value& value){
			std::string key = uf::string::split(value["name"].as<std::string>(), " ").back();
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
	std::vector<std::string> variableName;	
#endif
	std::function<void(const ext::json::Value&)> parse = [&]( const ext::json::Value& value ){
		// is array or object
	#if UF_SHADER_TRACK_NAMES
		if ( ext::json::isObject(value) ) {
			ext::json::forEach(value, [&]( const std::string& name, const ext::json::Value& member ){
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
		std::string path = uf::string::join(variableName, ".");
		path = uf::string::replace( path, ".[", "[" );
		VK_VALIDATION_MESSAGE("[" << (byteBuffer - byteBufferStart) << " / "<< (byteBufferEnd - byteBuffer) <<"]\tInserting: " << path << " = " << value.dump());
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
	VK_VALIDATION_MESSAGE("Updating " << name << " in " << filename);
	VK_VALIDATION_MESSAGE("Iterator: " << (void*) byteBuffer << "\t" << (void*) byteBufferEnd << "\t" << (byteBufferEnd - byteBuffer));
#endif
	parse(payload);
#if UF_SHADER_TRACK_NAMES
	VK_VALIDATION_MESSAGE("Iterator: " << (void*) byteBuffer << "\t" << (void*) byteBufferEnd << "\t" << (byteBufferEnd - byteBuffer));
#endif
#else
	auto pushValue = [&]( const std::string& primitive, const ext::json::Value& input ){
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
		}
 else if ( primitive == "uint32_t" ) {
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
		std::vector<std::string> variableName;
	#endif
	std::function<void(const ext::json::Value&, const ext::json::Value&)> parseDefinition = [&](const ext::json::Value& input, const ext::json::Value& definition ){
	#if UF_SHADER_TRACK_NAMES
		if ( SKIP_ADD ) {
			SKIP_ADD = false;
		} else {
			auto split = uf::string::split(definition["name"].as<std::string>(), " ");
			std::string type = split.front();
			std::string name = split.back();
			variableName.emplace_back(name);
		}
	#endif
		// is object
		if ( !ext::json::isNull(definition["members"]) ) {
			ext::json::forEach(definition["members"], [&](const ext::json::Value& member){
				std::string key = uf::string::split(member["name"].as<std::string>(), " ").back();
				parseDefinition(input[key], member);
			});
		// is array or primitive
		} else if ( !ext::json::isNull(definition["value"]) ) {
			// is object
			auto split = uf::string::split(definition["name"].as<std::string>(), " ");
			std::string type = split.front();
			std::string name = split.back();
			std::regex regex("^(?:(.+?)\\<)?(.+?)(?:\\>)?(?:\\[(\\d+)\\])?$");
			std::smatch match;
			if ( !std::regex_search( type, match, regex ) ) {
				std::cout << "Ill formatted typename: " << definition["name"].as<std::string>() << std::endl;
				return;
			}
			std::string vectorMatrix = match[1].str();
			std::string primitive = match[2].str();
			std::string arraySize = match[3].str();	
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
				std::string path = uf::string::join(variableName, ".");
				path = uf::string::replace( path, ".[", "[" );
				VK_VALIDATION_MESSAGE("[" << (byteBuffer - byteBufferStart) << " / "<< (byteBufferEnd - byteBuffer) <<"]\tInserting: " << path << " = (" << primitive << ") " << input.dump());
			#endif
				pushValue( primitive, input );
			}
		}
	#if UF_SHADER_TRACK_NAMES
		if ( !variableName.empty() ) variableName.pop_back();
	#endif
	};
	auto& definitions = metadata["definitions"]["uniforms"][name];
#if UF_SHADER_TRACK_NAMES
	VK_VALIDATION_MESSAGE("Updating " << name << " in " << filename);
	VK_VALIDATION_MESSAGE("Iterator: " << (void*) byteBuffer << "\t" << (void*) byteBufferEnd << "\t" << (byteBufferEnd - byteBuffer));
#endif
	parseDefinition(payload, definitions);
#if UF_SHADER_TRACK_NAMES
	VK_VALIDATION_MESSAGE("Iterator: " << (void*) byteBuffer << "\t" << (void*) byteBufferEnd << "\t" << (byteBufferEnd - byteBuffer));
#endif
#endif
	return userdata;
}
/*
ext::opengl::Shader::~Shader() {
	if ( !aliased ) destroy();
}
*/
void ext::opengl::Shader::initialize( ext::opengl::Device& device, const std::string& filename, GLhandle(VkShaderStageFlagBits) stage ) {
	this->device = &device;
	ext::opengl::Buffers::initialize( device );
	aliased = false;
}

void ext::opengl::Shader::destroy() {
	if ( aliased ) return;
	if ( !device ) return;

	ext::opengl::Buffers::destroy();

	for ( auto& userdata : uniforms ) userdata.destroy();
	for ( auto& userdata : pushConstants ) userdata.destroy();
	uniforms.clear();
	pushConstants.clear();
}

bool ext::opengl::Shader::validate() {
	// check if uniforms match buffer size
	bool valid = true;
	return valid;
}
bool ext::opengl::Shader::hasUniform( const std::string& name ) {
	return !ext::json::isNull(metadata["definitions"]["uniforms"][name]);
}
ext::opengl::Buffer* ext::opengl::Shader::getUniformBuffer( const std::string& name ) {
	return NULL;
}
ext::opengl::userdata_t& ext::opengl::Shader::getUniform( const std::string& name ) {
	static ext::opengl::userdata_t null;
	return null;
}
bool ext::opengl::Shader::updateUniform( const std::string& name ) {
	return false;
}
bool ext::opengl::Shader::updateUniform( const std::string& name, const ext::opengl::userdata_t& userdata ) {
	return false;
}

uf::Serializer ext::opengl::Shader::getUniformJson( const std::string& name, bool cache ) {
	return ext::json::null();
}
ext::opengl::userdata_t ext::opengl::Shader::getUniformUserdata( const std::string& name, const ext::json::Value& payload ) {
	return false;
}
bool ext::opengl::Shader::updateUniform( const std::string& name, const ext::json::Value& payload ) {
	return false;
}

bool ext::opengl::Shader::hasStorage( const std::string& name ) {
	return !ext::json::isNull(metadata["definitions"]["storage"][name]);
}

ext::opengl::Buffer* ext::opengl::Shader::getStorageBuffer( const std::string& name ) {
	return NULL;
}
uf::Serializer ext::opengl::Shader::getStorageJson( const std::string& name, bool cache ) {
	return ext::json::null();
}
ext::opengl::userdata_t ext::opengl::Shader::getStorageUserdata( const std::string& name, const ext::json::Value& payload ) {
	return false;
}

void ext::opengl::Pipeline::initialize( Graphic& graphic ) {
	return this->initialize( graphic, graphic.descriptor );
}
void ext::opengl::Pipeline::initialize( Graphic& graphic, GraphicDescriptor& descriptor ) {
	this->device = graphic.device;
	//this->descriptor = descriptor;
	Device& device = *graphic.device;
}
void ext::opengl::Pipeline::record( Graphic& graphic, GLhandle(VkCommandBuffer) commandBuffer, size_t pass, size_t draw ) {

}
void ext::opengl::Pipeline::update( Graphic& graphic ) {
	return this->update( graphic, graphic.descriptor );
}
void ext::opengl::Pipeline::update( Graphic& graphic, GraphicDescriptor& descriptor ) {
}
void ext::opengl::Pipeline::destroy() {
	if ( aliased ) return;
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
void ext::opengl::Material::attachShader( const std::string& filename, GLhandle(VkShaderStageFlagBits) stage ) {

}
void ext::opengl::Material::initializeShaders( const std::vector<std::pair<std::string, GLhandle(VkShaderStageFlagBits)>>& layout ) {
	shaders.clear(); shaders.reserve( layout.size() );
	for ( auto& request : layout ) {
		attachShader( request.first, request.second );
	}
}
bool ext::opengl::Material::hasShader( const std::string& type ) {
	return !ext::json::isNull( metadata["shaders"][type] );
}
ext::opengl::Shader& ext::opengl::Material::getShader( const std::string& type ) {
	static ext::opengl::Shader null;
	return null;
}
bool ext::opengl::Material::validate() {
	bool was = true;
	return was;
}
ext::opengl::Graphic::~Graphic() {
	this->destroy();
}
void ext::opengl::Graphic::initialize( const std::string& renderModeName ) {
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
ext::opengl::Pipeline& ext::opengl::Graphic::initializePipeline( GraphicDescriptor& descriptor, bool update ) {
	auto& pipeline = pipelines[descriptor.hash()];

//	auto previous = this->descriptor;
//	this->descriptor = descriptor;
	pipeline.initialize(*this, descriptor);
	pipeline.update(*this, descriptor);
//	if ( !update ) this->descriptor = previous;

	initialized = true;
	material.validate();

	return pipeline;
}
bool ext::opengl::Graphic::hasPipeline( GraphicDescriptor& descriptor ) {
	return pipelines.count( descriptor.hash() ) > 0;
}
ext::opengl::Pipeline& ext::opengl::Graphic::getPipeline() {
	return getPipeline( descriptor );
}
ext::opengl::Pipeline& ext::opengl::Graphic::getPipeline( GraphicDescriptor& descriptor ) {
	if ( !hasPipeline(descriptor) ) return initializePipeline( descriptor );
	return pipelines[descriptor.hash()];
}
void ext::opengl::Graphic::updatePipelines() {
	for ( auto pair : this->pipelines ) {
		pair.second.update( *this );
	}
}
void ext::opengl::Graphic::record( GLhandle(VkCommandBuffer) commandBuffer, size_t pass, size_t draw ) {
	return this->record( commandBuffer, descriptor, pass, draw );
}
void ext::opengl::Graphic::record( GLhandle(VkCommandBuffer) commandBuffer, GraphicDescriptor& descriptor, size_t pass, size_t draw ) {
}
void ext::opengl::Graphic::destroy() {
	for ( auto& pair : pipelines ) pair.second.destroy();
	pipelines.clear();

	material.destroy();

	ext::opengl::Buffers::destroy();

	ext::opengl::states::rebuild = true;
}

bool ext::opengl::Graphic::hasStorage( const std::string& name ) {
	return false;
}
ext::opengl::Buffer* ext::opengl::Graphic::getStorageBuffer( const std::string& name ) {
	return NULL;
}
uf::Serializer ext::opengl::Graphic::getStorageJson( const std::string& name, bool cache ) {
	return ext::json::null();
}
ext::opengl::userdata_t ext::opengl::Graphic::getStorageUserdata( const std::string& name, const ext::json::Value& payload ) {
	return ext::opengl::userdata_t();
}

#include <uf/utils/string/hash.h>
void ext::opengl::GraphicDescriptor::parse( ext::json::Value& metadata ) {
	if ( metadata["front face"].is<std::string>() ) {
		if ( metadata["front face"].as<std::string>() == "ccw" ) {
			frontFace = uf::renderer::enums::Face::CCW;
		} else if ( metadata["front face"].as<std::string>() == "cw" ) {
			frontFace = uf::renderer::enums::Face::CW;
		}
	}
	if ( metadata["cull mode"].is<std::string>() ) {
		if ( metadata["cull mode"].as<std::string>() == "back" ) {
			cullMode = uf::renderer::enums::CullMode::BACK;
		} else if ( metadata["cull mode"].as<std::string>() == "front" ) {
			cullMode = uf::renderer::enums::CullMode::FRONT;
		} else if ( metadata["cull mode"].as<std::string>() == "none" ) {
			cullMode = uf::renderer::enums::CullMode::NONE;
		} else if ( metadata["cull mode"].as<std::string>() == "both" ) {
			cullMode = uf::renderer::enums::CullMode::BOTH;
		}
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
std::string ext::opengl::GraphicDescriptor::hash() const {
	uf::Serializer serializer;

	serializer["subpass"] = subpass;
	if ( settings::experimental::individualPipelines ) serializer["renderMode"] = renderMode;

	serializer["renderTarget"] = renderTarget;
	serializer["geometry"]["sizes"]["vertex"] = geometry.sizes.vertex;
	serializer["geometry"]["sizes"]["indices"] = geometry.sizes.indices;
	serializer["indices"] = indices;
	serializer["offsets"]["vertex"] = offsets.vertex;
	serializer["offsets"]["index"] = offsets.index;

	{
		int i = 0;
		for ( auto& attribute : geometry.attributes ) {
			serializer["geometry"]["attributes"][i]["format"] = attribute.format;
			serializer["geometry"]["attributes"][i]["offset"] = attribute.offset;
			++i;
		}
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

//	if ( renderMode != "Gui" ) uf::iostream << this << ": " << indices << ": " << renderMode << ": " << subpass << ": " << serializer << "\n";
//	return uf::string::sha256( serializer.serialize() );
	return serializer.dump();
}

#endif