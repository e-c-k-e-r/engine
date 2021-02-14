#include <uf/utils/string/io.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/string/hash.h>

#include <sys/stat.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <uf/utils/io/iostream.h>

#ifdef WINDOWS
	#include <direct.h>
	#define GetCurrentDir _getcwd
#else
	#include <unistd.h>
	#define GetCurrentDir getcwd
#endif

const std::string uf::io::root = UF_IO_ROOT;

std::string UF_API uf::io::absolute( const std::string& path ) {
	char buff[FILENAME_MAX];
	GetCurrentDir( buff, FILENAME_MAX );
	return std::string(buff) + path;
}
std::string UF_API uf::io::filename( const std::string& str ) {
	return str.substr( str.find_last_of('/') + 1 );
}
std::string UF_API uf::io::extension( const std::string& str ) {
	return str.substr( str.find_last_of('.') + 1 );
}
std::string UF_API uf::io::directory( const std::string& str ) {
	return str.substr( 0, str.find_last_of('/') ) + "/";
}
std::string UF_API uf::io::sanitize( const std::string& str, const std::string& root ) {
	std::string path = str;
	// append root to path
	if ( path.find(root) == std::string::npos ) {
		path = root + "/" + path;
	}
	// flatten all "/./"
	{
		std::string tmp;
		while ( path != (tmp = uf::string::replace(path, "/./", "/")) ) {
			path = tmp;
		}
	}
	// flatten all "//"
	{
		std::string tmp;
		while ( path != (tmp = uf::string::replace(path, "//", "/")) ) {
			path = tmp;
		}
	}
	return path;
}
std::string UF_API uf::io::readAsString( const std::string& filename, const std::string& hash ) {
	std::string buffer;
	std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);
	if ( !is.is_open() ) {
		uf::iostream << "Error: Could not open file \"" << filename << "\"" << "\n";
		return buffer;
	}
	is.seekg(0, std::ios::end); buffer.reserve(is.tellg()); is.seekg(0, std::ios::beg);
	buffer.assign((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
	std::string expected = "";
	if ( hash != "" && (expected = uf::string::sha256( buffer )) != hash ) {
		uf::iostream << "Error: Hash mismatch for file \"" << filename << "\"; expecting " << hash << ", got " << expected << "\n";
		return "";
	}
	return buffer;
}
std::vector<uint8_t> UF_API uf::io::readAsBuffer( const std::string& filename, const std::string& hash ) {
	std::vector<uint8_t> buffer;
	std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);
	if ( !is.is_open() ) {
		uf::iostream << "Error: Could not open file \"" << filename << "\"" << "\n";
		return buffer;
	}
	is.seekg(0, std::ios::end); buffer.reserve(is.tellg()); is.seekg(0, std::ios::beg);
	buffer.assign((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
	std::string expected = "";
	if ( hash != "" && (expected = uf::string::sha256( buffer )) != hash ) {
		uf::iostream << "Error: Hash mismatch for file \"" << filename << "\"; expecting " << hash << ", got " << expected << "\n";
		return std::vector<uint8_t>();
	}
	return buffer;
}
std::string UF_API uf::io::hash( const std::string& filename ) {
	return uf::string::sha256( uf::io::readAsBuffer( filename ) );
}
bool UF_API uf::io::exists( const std::string& filename ) {
	static struct stat buffer;
	return stat(filename.c_str(), &buffer) == 0;
}
size_t UF_API uf::io::mtime( const std::string& filename ) {
	static struct stat buffer;
	if ( stat(filename.c_str(), &buffer) != 0 ) return -1;
	return buffer.st_mtime;
}
std::string UF_API uf::io::resolveURI( const std::string& filename, const std::string& _root ) {
	std::string root = _root;
	if ( filename.substr(0,8) == "https://" ) return filename;
	std::string extension = uf::io::extension(filename);
	// just sanitize
	if ( filename.find(uf::io::root) == 0 )
		return uf::io::sanitize( uf::io::filename( filename ), uf::io::directory( filename ) );
	// if the filename contains an absolute path or if no root is provided
	if ( filename[0] == '/' || root == "" ) {
		if ( filename.substr(0,9) == "/smtsamo/" ) root = uf::io::root;
		else if ( extension == "json" ) root = uf::io::root + "/entities/";
		else if ( extension == "png" ) root = uf::io::root + "/textures/";
		else if ( extension == "glb" ) root = uf::io::root + "/models/";
		else if ( extension == "gltf" ) root = uf::io::root + "/models/";
		else if ( extension == "ogg" ) root = uf::io::root + "/audio/";
		else if ( extension == "spv" ) root = uf::io::root + "/shaders/";
		else if ( extension == "lua" ) root = uf::io::root + "/scripts/";
	}
	return uf::io::sanitize(filename, root);
}