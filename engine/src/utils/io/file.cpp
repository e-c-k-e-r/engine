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

const uf::stl::string uf::io::root = UF_IO_ROOT;

uf::stl::string UF_API uf::io::absolute( const uf::stl::string& path ) {
	char buff[FILENAME_MAX];
	GetCurrentDir( buff, FILENAME_MAX );
	return uf::stl::string(buff) + path;
}
uf::stl::string UF_API uf::io::filename( const uf::stl::string& str ) {
	return str.substr( str.find_last_of('/') + 1 );
}
uf::stl::string UF_API uf::io::basename( const uf::stl::string& filename ) {
	uf::stl::string str = uf::io::filename( filename );
	uf::stl::string extension = uf::io::extension( filename );
	return uf::string::replace( str, "." + extension, "" );
}
uf::stl::string UF_API uf::io::extension( const uf::stl::string& str ) {
	uf::stl::string filename = uf::io::filename(str);
	return filename.substr( filename.find_last_of('.') + 1 );
}
uf::stl::string UF_API uf::io::extension( const uf::stl::string& str, int32_t count ) {
	uf::stl::string filename = uf::io::filename(str);
	uf::stl::string extension = "";
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
uf::stl::string UF_API uf::io::directory( const uf::stl::string& str ) {
	return str.substr( 0, str.find_last_of('/') ) + "/";
}
size_t UF_API uf::io::size( const uf::stl::string& filename ) {
	std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);
	if ( !is.is_open() ) return 0;
	is.seekg(0, std::ios::end);
	return is.tellg();
}
uf::stl::string UF_API uf::io::sanitize( const uf::stl::string& str, const uf::stl::string& _root ) {
	// resolve %root% to hard root
	uf::stl::string path = str;
	uf::stl::string root = _root;
	// append root to path
	if ( path.find("%root%") == 0 ) {
		path = uf::string::replace( path, "%root%", "" );
		if ( root == "" ) root = uf::io::root;
	}
	if ( path.find(root) == uf::stl::string::npos ) {
		path = root + "/" + path;
	}
	// flatten all "/./"
	{
		uf::stl::string tmp;
		while ( path != (tmp = uf::string::replace(path, "/\\/\\.\\//", "/")) ) {
			path = tmp;
		}
	}
	// flatten all "//"
	{
		uf::stl::string tmp;
		while ( path != (tmp = uf::string::replace(path, "/\\/\\//", "/")) ) {
			path = tmp;
		}
	}
	return path;
}
// would just use readAsBuffer and convert to string, but that's double the memory cost
uf::stl::string UF_API uf::io::readAsString( const uf::stl::string& _filename, const uf::stl::string& hash ) {
	uf::stl::string buffer;
	uf::stl::string filename = sanitize(_filename);
	uf::stl::string extension = uf::io::extension( filename );
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
	uf::stl::string expected = "";
	if ( hash != "" && (expected = uf::string::sha256( buffer )) != hash ) {
		UF_MSG_ERROR("Error: Hash mismatch for file \"" << filename << "\"; expecting " << hash << ", got " << expected);
		return "";
	}
	return buffer;
}
uf::stl::vector<uint8_t> UF_API uf::io::readAsBuffer( const uf::stl::string& _filename, const uf::stl::string& hash ) {
	uf::stl::vector<uint8_t> buffer;
	uf::stl::string filename = sanitize(_filename);
	uf::stl::string extension = uf::io::extension( filename );
	if ( extension == "gz" ) {
		buffer = uf::io::decompress( filename );
	} else {
		std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);
		if ( !is.is_open() ) {
			UF_MSG_ERROR("Error: Could not open file \"" << filename << "\"");
			return buffer;
		}
		is.seekg(0, std::ios::end); buffer.reserve(is.tellg()); is.seekg(0, std::ios::beg);
		buffer.assign((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
	}
	uf::stl::string expected = "";
	if ( hash != "" && (expected = uf::string::sha256( buffer )) != hash ) {
		UF_MSG_ERROR("Error: Hash mismatch for file \"" << filename << "\"; expecting " << hash << ", got " << expected);
		return uf::stl::vector<uint8_t>();
	}
	return buffer;
}

size_t UF_API uf::io::write( const uf::stl::string& filename, const void* buffer, size_t size ) {
	uf::stl::string extension = uf::io::extension( filename );
	if ( extension == "gz" ) return uf::io::compress( filename, buffer, size );

	std::ofstream output;
	output.open( uf::io::sanitize( filename ), std::ios::binary);
	output.write( (const char*) buffer, size );
	output.close();
	return size;
}

// indirection for different compression formats, currently only using zlib's gzFile shit
uf::stl::vector<uint8_t> UF_API uf::io::decompress( const uf::stl::string& filename ) {
	uf::stl::string extension = uf::io::extension( filename );
	if ( extension == "gz" ) return ext::zlib::decompressFromFile( filename );
	UF_MSG_ERROR("unsupported compression format requested: " << extension);
	return {};
}
size_t UF_API uf::io::compress( const uf::stl::string& filename, const void* buffer, size_t size ) {
	uf::stl::string extension = uf::io::extension( filename );
	if ( extension == "gz" ) return ext::zlib::compressToFile( filename, buffer, size );
	UF_MSG_ERROR("unsupported compression format requested: " << extension);
	return 0;
}

uf::stl::string UF_API uf::io::hash( const uf::stl::string& filename ) {
	return uf::string::sha256( uf::io::readAsBuffer( filename ) );
}
bool UF_API uf::io::exists( const uf::stl::string& _filename ) {
	uf::stl::string filename = sanitize(_filename);
	static struct stat buffer;
	return stat(filename.c_str(), &buffer) == 0;
}
size_t UF_API uf::io::mtime( const uf::stl::string& _filename ) {
	uf::stl::string filename = sanitize(_filename);
	static struct stat buffer;
	if ( stat(filename.c_str(), &buffer) != 0 ) return -1;
	return buffer.st_mtime;
}
bool UF_API uf::io::mkdir( const uf::stl::string& _filename ) {
#if UF_ENV_DREAMCAST
	return false;
#else
	uf::stl::string filename = sanitize(_filename);
	int status = ::mkdir(filename.c_str());
	return status != -1;
#endif
}
uf::stl::string UF_API uf::io::resolveURI( const uf::stl::string& filename, const uf::stl::string& _root ) {
	uf::stl::string root = _root;
	if ( filename.substr(0,8) == "https://" ) return filename;
	const uf::stl::string extension = uf::io::extension(filename);
	// just sanitize
	if ( filename.find(uf::io::root) == 0 ) {
		return uf::io::sanitize( uf::io::filename( filename ), uf::io::directory( filename ) );
	}
	if ( filename.find("%root%") == 0 ) {
		const uf::stl::string f = uf::string::replace( filename, "%root%", uf::io::root );
		return uf::io::sanitize( uf::io::filename( f ), uf::io::directory( f ) );
	}
	// if the filename contains an absolute path or if no root is provided
	if ( filename[0] == '/' || root == "" ) {
		const uf::stl::string basename = uf::io::filename( filename );
		const uf::stl::string extensions = uf::io::extension( filename, -1 );
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