#include <uf/utils/io/file.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/string/hash.h>

#include <sys/stat.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <uf/utils/io/file.h>
#include <uf/utils/io/iostream.h>
#include <uf/ext/zlib/zlib.h>

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
std::string UF_API uf::io::basename( const std::string& filename ) {
	std::string str = uf::io::filename( filename );
	std::string extension = uf::io::extension( filename );
	return uf::string::replace( str, "." + extension, "" );
}
std::string UF_API uf::io::extension( const std::string& str ) {
	std::string filename = uf::io::filename(str);
	return filename.substr( filename.find_last_of('.') + 1 );
}
std::string UF_API uf::io::extension( const std::string& str, int32_t count ) {
	std::string filename = uf::io::filename(str);
	std::string extension = "";
	auto split = uf::string::split( filename, "." );
	size_t offset = split.size() - 1;
	if ( count < 0 ) {
		offset = -count;
	} else {
		offset = split.size() - count;
	}
	for ( size_t i = offset; i < split.size(); ++i ) {
		extension += split[i];
		if ( i + 1 < split.size() ) extension += ".";
	}
	return extension;
}
std::string UF_API uf::io::directory( const std::string& str ) {
	return str.substr( 0, str.find_last_of('/') ) + "/";
}
size_t UF_API uf::io::size( const std::string& filename ) {
	std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);
	if ( !is.is_open() ) return 0;
	is.seekg(0, std::ios::end);
	return is.tellg();
}
std::string UF_API uf::io::sanitize( const std::string& str, const std::string& _root ) {
	// resolve %root% to hard root
	std::string path = str;
	std::string root = _root;
	// append root to path
	if ( path.find("%root%") == 0 ) {
		path = uf::string::replace( path, "%root%", "" );
		if ( root == "" ) root = uf::io::root;
	}
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
// would just use readAsBuffer and convert to string, but that's double the memory cost
std::string UF_API uf::io::readAsString( const std::string& _filename, const std::string& hash ) {
	std::string buffer;
	std::string filename = sanitize(_filename);
	std::string extension = uf::io::extension( filename );
	if ( extension == "gz" ) {
		auto decompressed = uf::io::decompress( filename );
		buffer.reserve(decompressed.size());
		buffer.assign(decompressed.begin(), decompressed.end());
	} else {
		std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);
		if ( !is.is_open() ) {
			UF_MSG_ERROR("Error: Could not open file \"" << filename << "\"");
			return buffer;
		}
		is.seekg(0, std::ios::end); buffer.reserve(is.tellg()); is.seekg(0, std::ios::beg);
		buffer.assign((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
	}
	std::string expected = "";
	if ( hash != "" && (expected = uf::string::sha256( buffer )) != hash ) {
		UF_MSG_ERROR("Error: Hash mismatch for file \"" << filename << "\"; expecting " << hash << ", got " << expected);
		return "";
	}
	return buffer;
}
std::vector<uint8_t> UF_API uf::io::readAsBuffer( const std::string& _filename, const std::string& hash ) {
	std::vector<uint8_t> buffer;
	std::string filename = sanitize(_filename);
	std::string extension = uf::io::extension( filename );
	if ( extension == "gz" ) {
		auto decompressed = uf::io::decompress( filename );
		buffer.reserve(decompressed.size());
		buffer.assign(decompressed.begin(), decompressed.end());
	} else {
		std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);
		if ( !is.is_open() ) {
			UF_MSG_ERROR("Error: Could not open file \"" << filename << "\"");
			return buffer;
		}
		is.seekg(0, std::ios::end); buffer.reserve(is.tellg()); is.seekg(0, std::ios::beg);
		buffer.assign((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
	}
	std::string expected = "";
	if ( hash != "" && (expected = uf::string::sha256( buffer )) != hash ) {
		UF_MSG_ERROR("Error: Hash mismatch for file \"" << filename << "\"; expecting " << hash << ", got " << expected);
		return std::vector<uint8_t>();
	}
	return buffer;
}

size_t UF_API uf::io::write( const std::string& filename, const void* buffer, size_t size ) {
	std::string extension = uf::io::extension( filename );
	if ( extension == "gz" ) return uf::io::compress( filename, buffer, size );

	std::ofstream output;
	output.open( uf::io::sanitize( filename ), std::ios::binary);
	output.write( (const char*) buffer, size );
	output.close();
	return size;
}

// indirection for different compression formats, currently only using zlib's gzFile shit
std::vector<uint8_t> UF_API uf::io::decompress( const std::string& filename ) {
	return ext::zlib::decompressFromFile( filename );
}
size_t UF_API uf::io::compress( const std::string& filename, const void* buffer, size_t size ) {
	return ext::zlib::compressToFile( filename, buffer, size );
}

std::string UF_API uf::io::hash( const std::string& filename ) {
	return uf::string::sha256( uf::io::readAsBuffer( filename ) );
}
bool UF_API uf::io::exists( const std::string& _filename ) {
	std::string filename = sanitize(_filename);
	static struct stat buffer;
	return stat(filename.c_str(), &buffer) == 0;
}
size_t UF_API uf::io::mtime( const std::string& _filename ) {
	std::string filename = sanitize(_filename);
	static struct stat buffer;
	if ( stat(filename.c_str(), &buffer) != 0 ) return -1;
	return buffer.st_mtime;
}
bool UF_API uf::io::mkdir( const std::string& _filename ) {
#if UF_ENV_DREAMCAST
	return false;
#else
	std::string filename = sanitize(_filename);
	int status = ::mkdir(filename.c_str());
	return status != -1;
#endif
}
std::string UF_API uf::io::resolveURI( const std::string& filename, const std::string& _root ) {
	std::string root = _root;
	if ( filename.substr(0,8) == "https://" ) return filename;
	const std::string extension = uf::io::extension(filename);
	// just sanitize
	if ( filename.find(uf::io::root) == 0 ) {
		return uf::io::sanitize( uf::io::filename( filename ), uf::io::directory( filename ) );
	}
	if ( filename.find("%root%") == 0 ) {
		const std::string f = uf::string::replace( filename, "%root%", uf::io::root );
		return uf::io::sanitize( uf::io::filename( f ), uf::io::directory( f ) );
	}
	// if the filename contains an absolute path or if no root is provided
	if ( filename[0] == '/' || root == "" ) {
		const std::string basename = uf::io::filename( filename );
		const std::string extensions = uf::io::extension( filename, -1 );
		if ( filename[0] == '/' && filename[1] == '/' ) root = uf::io::root;
	//	else if ( uf::io::extension(filename, -1) == "graph.json" ) root = uf::io::root + "/models/";
	//	else if ( uf::io::extension(filename, -1) == "scene.json" ) root = uf::io::root + "/scenes/";
		else if ( basename == "graph.json" || basename == "graph.json.gz" ) root = uf::io::root + "/models/";
		else if ( basename == "scene.json" || basename == "scene.json.gz" ) root = uf::io::root + "/scenes/";
		else if ( extension == "json" || extensions == "json.gz" ) root = uf::io::root + "/entities/";
		else if ( extension == "png" || extensions == "png.gz" ) root = uf::io::root + "/textures/";
		else if ( extension == "glb" || extensions == "glb.gz" ) root = uf::io::root + "/models/";
		else if ( extension == "gltf" || extensions == "gltf.gz" ) root = uf::io::root + "/models/";
		else if ( extension == "graph" || extensions == "graph.gz" ) root = uf::io::root + "/models/";
		else if ( extension == "ogg" || extensions == "ogg.gz" ) root = uf::io::root + "/audio/";
		else if ( extension == "spv" || extensions == "spv.gz" ) root = uf::io::root + "/shaders/";
		else if ( extension == "lua" || extensions == "lua.gz" ) root = uf::io::root + "/scripts/";
	}
	return uf::io::sanitize(filename, root);
}