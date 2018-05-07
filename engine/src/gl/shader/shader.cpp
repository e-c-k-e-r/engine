#include <uf/gl/shader/shader.h>

// C-tors
uf::ogl::Shader::Component::Component( Shader::type_t type ) :
	m_type(type),
	m_index(0)
{

}
uf::ogl::Shader::Component::Component( const Component::source_t& source, Shader::type_t type, bool isFilename ) :
	m_type(type),
	m_index(0),
	m_source(source)
{
	if ( isFilename ) this->read(source);
}
// D-tor
uf::ogl::Shader::Component::~Component() {
	this->destroy();
}

// Read from file
#include <fstream>
#include <sys/stat.h>
#include <iostream>
bool uf::ogl::Shader::Component::read( const std::string& filename ) {
	/* Check if file exists */ {
		static struct stat buffer;
		bool exists = stat(filename.c_str(), &buffer) == 0;
		if ( !exists ) {
			std::cout << "ERROR: " << filename << std::endl;
			return false;
		}
	}

	std::string& buffer = this->m_source = "";
	std::ifstream input;
	input.open( filename, std::ios::binary );
		
	input.seekg(0, std::ios::end);
	buffer.reserve(input.tellg());
	input.seekg(0, std::ios::beg);

	buffer.assign((std::istreambuf_iterator<char>(input)),std::istreambuf_iterator<char>());

	input.close();

	return true;
}
// Import from source
void uf::ogl::Shader::Component::import( const std::string& source ) {
	this->m_source = source;
}
// Compile shader unit
bool uf::ogl::Shader::Component::compile() {
	if ( this->compiled() ) glDeleteShader( this->m_index );
	this->m_index = glCreateShader( this->m_type );
	const char* c_str = this->m_source.c_str();
	glShaderSource( this->m_index, 1, &c_str, NULL );
	glCompileShader( this->m_index );

	int status = 0;
	glGetShaderiv( this->m_index, GL_COMPILE_STATUS, &status );
	if ( status == GL_FALSE ) {
		std::string log = this->log();
		/* 	Error handle */
		//	std::cout << log << std::endl;
		return false;
	}
	return true;
}
// Checks if valid
bool uf::ogl::Shader::Component::compiled() const {
	return this->m_index != 0;
}
// Releases resources
void uf::ogl::Shader::Component::destroy() {
	if ( this->compiled() ) glDeleteShader(this->m_index);
	this->m_index = 0;
	this->m_source = "";
}
// Error checking
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
std::string uf::ogl::Shader::Component::log() const {
	int len = 0;
	glGetShaderiv( this->m_index, GL_INFO_LOG_LENGTH, &len );
	std::vector<char> buffer(len);
	glGetShaderInfoLog( this->m_index, len, &len, &buffer[0] );

	std::stringstream ss;
	ss << std::string(buffer.begin(), buffer.end());
	std::cout << ss.str() << std::endl;
	return ss.str();
}
// Gets index
uf::ogl::Shader::index_t uf::ogl::Shader::Component::getIndex() const {
	return this->m_index;
}