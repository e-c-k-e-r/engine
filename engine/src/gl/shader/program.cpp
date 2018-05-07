#include <uf/gl/shader/shader.h>
// Base version is OpenGL 2.0
// C-tors
uf::ogl::Shader::Shader() :
	m_index(0)
{

}
uf::ogl::Shader::Shader( const Component::container_t& components ) :
	m_index(0),
	m_components(components)
{

}
// D-tor
uf::ogl::Shader::~Shader() {
	this->flush();
	this->destroy();
}

// Attaches shader units
void uf::ogl::Shader::add( const Shader::Component& component ) {
	this->m_components.push_back(component);
}
void uf::ogl::Shader::add( const Component::container_t& components ) {
	this->m_components.insert(this->m_components.end(), components.begin(), components.end());
}
#include <iostream>
// Compiles all shader units (Calls this->link() if link==true)
void uf::ogl::Shader::compile( bool link ) {
	if ( !this->compiled() ) this->m_index = glCreateProgram();
	for ( auto& component : this->m_components ) {
		if ( !component.compiled() ) component.compile();
		glAttachShader( this->m_index, component.getIndex() );
	}
	if ( link ) this->link();
}
// Checks if compiled
bool uf::ogl::Shader::compiled() const {
	return this->m_index;
}
// Links shader units
void uf::ogl::Shader::link() {
	glLinkProgram(this->m_index);

	int status = 0;
	glGetProgramiv(this->m_index, GL_LINK_STATUS, &status);
	if ( status == GL_FALSE ) {
		/* Error handling */
		std::string log = this->log();
		this->destroy();
		return;
	}
	this->populate();
	this->flush();
}
// Destroys all shader units
void uf::ogl::Shader::flush() {
	this->m_components.clear();
}
// Destroy this shader
void uf::ogl::Shader::destroy() {
	this->flush();
	if ( this->m_index ) glDeleteProgram( this->m_index );
	this->m_index = 0;
}
// Populates uniform list
void uf::ogl::Shader::populate() {
	/* 	OpenGL 4.3
		int len;
		glGetProgramInterfaceiv(this->index, GL_UNIFORM, GL_ACTIVE_RESOURCES, &len);
		std::vector<char> name(256);
		std::vector<GLenum> properties = { GL_NAME_LENGTH, GL_TYPE, GL_ARRAY_SIZE };
		std::vector<int> values(properties.size());
		for ( int i = 0; i < len; i++ ) {
			glGetProgramResourceiv(this->index, GL_UNIFORM, i, properties.size(), &properties[0], values.size(), 0, &values[0] );
			int str_len = values[0];
			name.resize(str_len+1);
			glGetProgramResourceName(this->index, GL_UNIFORM, i, name.size(), 0, &name[0]);
			std::string str(&name[0], name.size()-1);
			this->uniforms[str] = i;
		}
		return;
	*/
	int len = 0;
	int str_len = 0;
	glGetProgramiv(this->m_index, GL_ACTIVE_UNIFORMS, &len);
	glGetProgramiv(this->m_index, GL_ACTIVE_UNIFORM_MAX_LENGTH, &str_len);
	std::vector<char> name(str_len);

	int length;
	int size;
	GLenum type;
	for ( int i = 0; i < len; i++ ) {
		glGetActiveUniform( this->m_index, i, str_len, &length, &size, &type, &name[0] );
		std::string str(&name[0], length);
		this->m_uniforms[str] = i;
	}
}
// Error checking
#include <iostream>
#include <fstream>
std::string uf::ogl::Shader::log() const {
	int len = 0;
	glGetProgramiv( this->m_index, GL_INFO_LOG_LENGTH, &len );
	std::vector<char> buffer(len);
	glGetProgramInfoLog( this->m_index, len, &len, &buffer[0] );

	std::stringstream ss;
	ss << std::string(buffer.begin(), buffer.end());
	std::cout << ss.str() << std::endl;
	return ss.str();
}
// Swap shader
void uf::ogl::Shader::swap( Shader& shader ) {
	std::swap( this->m_index, shader.m_index );
	std::swap( this->m_components, shader.m_components );
	std::swap( this->m_uniforms, shader.m_uniforms );
}
// Bind's shader
void uf::ogl::Shader::bind() const {
/*
	static uint index = -1;
	if ( index != this->m_index )
		glUseProgram(index = this->m_index);
*/
	glUseProgram(this->m_index);
}
// Gets index
uf::ogl::Shader::index_t uf::ogl::Shader::getIndex() const {
	return this->m_index;
}

// Shader input/ouptuts
uf::ogl::Shader::Uniform::index_t uf::ogl::Shader::getUniform( const Uniform::name_t& name ) const {
	this->bind();
	return this->m_uniforms.count(name) > 0 ? this->m_uniforms.at(name) : glGetUniformLocation( this->m_index, name.c_str());
}
void uf::ogl::Shader::bindAttribute( Attribute::index_t index, const Attribute::name_t& name  ) const {
	this->bind();
	glBindAttribLocation(this->m_index, index, name.c_str());
}
void uf::ogl::Shader::bindFragmentData( Attribute::index_t index, const Attribute::name_t& name  ) const {
	this->bind();
	glBindFragDataLocation(this->m_index, index, name.c_str());
}
// Uniforms
// 	Legacy glUniform calls
void uf::ogl::Shader::push( Attribute::index_t index, int x ) const {
	// OpenGL 4.1: glProgramUniform1i( this->m_index, index, x );
	this->bind();
	glUniform1i( index, x );
}
void uf::ogl::Shader::push( Attribute::index_t index, uint x ) const {
	// OpenGL 4.1: glProgramUniform1ui( this->m_index, index, x );
	this->bind();
	glUniform1ui( index, x );
}
void uf::ogl::Shader::push( Attribute::index_t index, float x ) const {
	// OpenGL 4.1: glProgramUniform1f( this->m_index, index, x );
	this->bind();
	glUniform1f( index, x );
}
void uf::ogl::Shader::push( Attribute::index_t index, float x, float y ) const {
	// OpenGL 4.1: glProgramUniform2f( this->m_index, index, x, y );
	this->bind();
	glUniform2f( index, x, y );
}
void uf::ogl::Shader::push( Attribute::index_t index, float x, float y, float z ) const {
	// OpenGL 4.1: glProgramUniform1f( this->m_index, index, x, y, z );
	this->bind();
	glUniform3f( index, x, y, z );
}
void uf::ogl::Shader::push( Attribute::index_t index, float x, float y, float z, float w ) const {
	// OpenGL 4.1: glProgramUniform4f( this->m_index, index, x, y, z, w );
	this->bind();
	glUniform4f( index, x, y, z, w );
}
void uf::ogl::Shader::push( Attribute::index_t index, bool transpose, float* matrix ) const {
	// OpenGL 4.1: glProgramUniformMatrix4fv( this->m_index, index, 1, transpose, matrix );
	this->bind();
	glUniformMatrix4fv( index, 1, transpose, matrix );
}

void uf::ogl::Shader::push( Attribute::index_t index, const uf::Vector2& vec2 ) const {
	this->push( index, vec2.x, vec2.y );
}
void uf::ogl::Shader::push( Attribute::index_t index, const uf::Vector3& vec3 ) const {
	this->push( index, vec3.x, vec3.y, vec3.z );
}
void uf::ogl::Shader::push( Attribute::index_t index, const uf::Vector4& vec4 ) const {
	this->push( index, vec4.x, vec4.y, vec4.z, vec4.w );
}
void uf::ogl::Shader::push( Attribute::index_t index, bool transpose, const uf::Matrix4& mat ) const {
	// this->push( index, transpose, (uf::Matrix4::type_t*) &mat[0] );
	pod::Matrix4t<float> converted = mat.convert<float>();
	this->push( index, transpose, (float*) &converted[0] );
}
// 	"optimized" glUniform calls
void uf::ogl::Shader::push( Attribute::name_t name, int x ) const {
	return this->push( this->getUniform(name), x );
}
void uf::ogl::Shader::push( Attribute::name_t name, uint x ) const {
	return this->push( this->getUniform(name), x );
}
void uf::ogl::Shader::push( Attribute::name_t name, float x ) const {
	return this->push( this->getUniform(name), x );
}
void uf::ogl::Shader::push( Attribute::name_t name, float x, float y ) const {
	return this->push( this->getUniform(name), x, y );
}
void uf::ogl::Shader::push( Attribute::name_t name, float x, float y, float z ) const {
	return this->push( this->getUniform(name), x, y, z );
}
void uf::ogl::Shader::push( Attribute::name_t name, float x, float y, float z, float w ) const {
	return this->push( this->getUniform(name), x, y, z, w );
}
void uf::ogl::Shader::push( Attribute::name_t name, float* mat, bool transpose ) const {
	return this->push( this->getUniform(name), transpose, mat );
}

void uf::ogl::Shader::push( Attribute::name_t name, const uf::Vector2& vec2 ) const {
	return this->push( this->getUniform(name), vec2 );
}
void uf::ogl::Shader::push( Attribute::name_t name, const uf::Vector3& vec3 ) const {
	return this->push( this->getUniform(name), vec3 );
}
void uf::ogl::Shader::push( Attribute::name_t name, const uf::Vector4& vec4 ) const {
	return this->push( this->getUniform(name), vec4 );
}
void uf::ogl::Shader::push( Attribute::name_t name, const uf::Matrix4& mat, bool transpose ) const {
	return this->push( this->getUniform(name), transpose, mat );
}