#if UF_USE_OPENGL

#include <uf/ext/opengl/shader.h>
#include <uf/ext/opengl/initializers.h>
#include <uf/ext/opengl/opengl.h>
#include <uf/ext/opengl/commands.h>
#include <uf/ext/openvr/openvr.h>
#include <uf/ext/gltf/gltf.h>

#include <fstream>
#include <regex>

namespace {
	uint32_t VERTEX_BUFFER_BIND_ID = 0;
#if UF_USE_OPENGL_FIXED_FUNCTION
	uf::stl::unordered_map<uf::stl::string, ext::opengl::Shader::module_t> modules;
#endif
}

#if UF_USE_OPENGL_FIXED_FUNCTION
void ext::opengl::Shader::bind( const uf::stl::string& name, const module_t& module ) {
	::modules[name] = module;
}
void ext::opengl::Shader::execute( const ext::opengl::Graphic& graphic, void* userdata ) const {
	if ( module ) module( *this, graphic, userdata );
}
#endif

/*
ext::opengl::Shader::~Shader() {
	if ( !aliased ) destroy();
}
*/
void ext::opengl::Shader::initialize( ext::opengl::Device& device, const uf::stl::string& filename, enums::Shader::type_t stage ) {
	this->device = &device;
	descriptor.stage = stage;

	ext::opengl::Buffers::initialize( device );
	aliased = false;

	// set up metadata
	{
		metadata.json["filename"] = filename;
		metadata.type = "";
	}
// GPU-based shader execution
#if !UF_USE_OPENGL_FIXED_FUNCTION
	uf::stl::string glsl;
	{
		std::ifstream is(this->filename = filename, std::ios::binary | std::ios::in | std::ios::ate);
		if ( !is.is_open() ) {
			GL_VALIDATION_MESSAGE("Error: Could not open shader file \"" << filename << "\"");
			return;
		}
		is.seekg(0, std::ios::end); spirv.reserve(is.tellg()); is.seekg(0, std::ios::beg);
		spirv.assign((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
	
		assert(spirv.size() > 0);
	}
	{
		device.activateContext();
		module = glCreateShader(stage);
		GL_ERROR_CHECK(glShaderSource(module, 1, &glsl.c_str(), NULL));
		GL_ERROR_CHECK(glCompileShader(module));
	}
	{
		size_t len = 1024;
		GLint status;
		GLbyte log[len];
		GL_ERROR_CHECK(glGetShaderiv(module, GL_LINK_STATUS, &status));
		if ( !status ) {
			GL_ERROR_CHECK(glGetShaderInfoLog(module, len, NULL, log));
			GL_VALIDATION_MESSAGE(log);
		}
	}
// CPU-based shader execution
#else
	{
	//	GL_VALIDATION_MESSAGE(filename);
		if ( ::modules.count(filename) > 0 ) {
			module = ::modules[filename];
		}
	}
	if ( metadata.autoInitializeUniformBuffers ) {
		initializeBuffer( nullptr, sizeof(pod::Uniform), uf::renderer::enums::Buffer::UNIFORM );
	}
#endif
}

void ext::opengl::Shader::destroy() {
	if ( aliased ) return;
	if ( !device ) return;

	ext::opengl::Buffers::destroy();
#if !UF_USE_OPENGL_FIXED_FUNCTION
	if ( module != GL_NULL_HANDLE ) {
		GL_ERROR_CHECK(glDeleteShader( module ));
		module = GL_NULL_HANDLE;
		descriptor = {};
	}
#endif

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
bool ext::opengl::Shader::hasUniform( const uf::stl::string& name ) const {
//	return metadata.definitions.uniforms.count(name) > 0;
	return true;
}
ext::opengl::Buffer& ext::opengl::Shader::getUniformBuffer( const uf::stl::string& name ) {
	UF_ASSERT( hasUniform(name) );
	size_t uniformIndex = 0; // metadata.definitions.uniforms[name].index;
	for ( size_t bufferIndex = 0, uniformCounter = 0; bufferIndex < buffers.size(); ++bufferIndex ) {
		if ( !(buffers[bufferIndex].usage & uf::renderer::enums::Buffer::UNIFORM) ) continue;
		if ( uniformCounter++ != uniformIndex ) continue;
		return buffers[bufferIndex];
	}
	UF_EXCEPTION("buffer not found: " << name);
}
const ext::opengl::Buffer& ext::opengl::Shader::getUniformBuffer( const uf::stl::string& name ) const {
	UF_ASSERT( hasUniform(name) );
	size_t uniformIndex = 0; // metadata.definitions.uniforms.at(name).index;
	for ( size_t bufferIndex = 0, uniformCounter = 0; bufferIndex < buffers.size(); ++bufferIndex ) {
		if ( !(buffers.at(bufferIndex).usage & uf::renderer::enums::Buffer::UNIFORM) ) continue;
		if ( uniformCounter++ != uniformIndex ) continue;
		return buffers.at(bufferIndex);
	}
	UF_EXCEPTION("buffer not found: " << name);
}
ext::opengl::userdata_t& ext::opengl::Shader::getUniform( const uf::stl::string& name ) {
	UF_ASSERT( hasUniform(name) );
	return uniforms[0/*metadata.definitions.uniforms[name].index*/];
}
const ext::opengl::userdata_t& ext::opengl::Shader::getUniform( const uf::stl::string& name ) const {
	UF_ASSERT( hasUniform(name) );
	return uniforms.at(0/*metadata.definitions.uniforms.at(name).index*/);
}
bool ext::opengl::Shader::updateUniform( const uf::stl::string& name, const void* data, size_t size ) const {
	if ( !hasUniform(name) ) return false;
	auto& bufferObject = getUniformBuffer(name);
//	size = MAX(metadata.definitions.uniforms.at(name).size, bufferObject.allocationInfo.size);
	updateBuffer( data, size, bufferObject );
	return true;
}

bool ext::opengl::Shader::hasStorage( const uf::stl::string& name ) const {
//	return metadata.definitions.storage.count(name) > 0;
	return true;
}

ext::opengl::Buffer& ext::opengl::Shader::getStorageBuffer( const uf::stl::string& name ) {
	UF_ASSERT( hasStorage(name) );
	size_t storageIndex = 0; // metadata.definitions.storage[name].index;
	for ( size_t bufferIndex = 0, storageCounter = 0; bufferIndex < buffers.size(); ++bufferIndex ) {
		if ( !(buffers[bufferIndex].usage & uf::renderer::enums::Buffer::STORAGE) ) continue;
		if ( storageCounter++ != storageIndex ) continue;
		return buffers[bufferIndex];
	}
	UF_EXCEPTION("buffer not found: " << name);
}
const ext::opengl::Buffer& ext::opengl::Shader::getStorageBuffer( const uf::stl::string& name ) const {
	UF_ASSERT( hasStorage(name) );
	size_t storageIndex = 0; // metadata.definitions.storage.at(name).index;
	for ( size_t bufferIndex = 0, storageCounter = 0; bufferIndex < buffers.size(); ++bufferIndex ) {
		if ( !(buffers.at(bufferIndex).usage & uf::renderer::enums::Buffer::STORAGE) ) continue;
		if ( storageCounter++ != storageIndex ) continue;
		return buffers.at(bufferIndex);
	}
	UF_EXCEPTION("buffer not found: " << name);
}
// JSON shit
#if 0
ext::opengl::userdata_t ext::opengl::Shader::getUniformUserdata( const uf::stl::string& name, const ext::json::Value& payload ) {
	if ( !hasUniform(name) ) return false;
	return jsonToUserdata(payload, metadata.json["definitions"]["uniforms"][name]);
}
uf::Serializer ext::opengl::Shader::getUniformJson( const uf::stl::string& name, bool cache ) {
	if ( !hasUniform(name) ) return ext::json::null();
	if ( cache && !ext::json::isNull(metadata.json["uniforms"][name]) ) return metadata.json["uniforms"][name];
	auto& definition = metadata.json["definitions"]["uniforms"][name];
	if ( cache ) return metadata.json["uniforms"][name] = definitionToJson(definition);
	return definitionToJson(definition);
}
uf::Serializer ext::opengl::Shader::getStorageJson( const uf::stl::string& name, bool cache ) {
	if ( !hasStorage(name) ) return ext::json::null();
	if ( cache && !ext::json::isNull(metadata.json["storage"][name]) ) return metadata.json["storage"][name];
	auto& definition = metadata.json["definitions"]["storage"][name];
	if ( cache ) return metadata.json["storage"][name] = definitionToJson(definition);
	return definitionToJson(definition);
}
ext::opengl::userdata_t ext::opengl::Shader::getStorageUserdata( const uf::stl::string& name, const ext::json::Value& payload ) {
	if ( !hasStorage(name) ) return false;
	return jsonToUserdata(payload, metadata.json["definitions"]["storage"][name]);
}
#endif
#endif