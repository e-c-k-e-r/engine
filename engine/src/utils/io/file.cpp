#include <uf/utils/io/file.h>
#include <uf/utils/io/vfs.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/string/hash.h>
#include <uf/utils/memory/memcpy.h>

#include <sys/stat.h>
#include <algorithm>
#include <fstream>
#include <uf/utils/io/file.h>
#include <uf/utils/io/iostream.h>
#include <uf/ext/zlib/zlib.h>
#include <uf/ext/lz4/lz4.h>

#ifdef WINDOWS
	#include <direct.h>
	#define GetCurrentDir _getcwd
#else
	#include <unistd.h>
	#define GetCurrentDir getcwd
#endif

#define CACHE_QUERIES 1

// cache queries by key and value in the event redundant I/O or string processing calls are made (namely for exists)
#if CACHE_QUERIES
	#define QUERY_CACHE( query, type )\
		static thread_local std::pair<uf::stl::string, type> qkv;\
		if ( !qkv.first.empty() && qkv.first == query ) return qkv.second;\
		qkv.first = query;

	#define CACHE_QUERY( value ) qkv.second = value;
#else
	#define QUERY_CACHE( query, type ) {}
	#define CACHE_QUERY( value ) value;
#endif

namespace {
	uf::stl::string resolveURI( const uf::stl::string& _filename, const uf::stl::string& _root = "" ) {
		QUERY_CACHE( _root + _filename, uf::stl::string );
		if ( _filename.length() >= 8 && _filename.substr(0,8) == "https://" ) return CACHE_QUERY(_filename);

		uf::stl::string f = uf::io::normalize( _filename );
		uf::stl::string root = uf::io::normalize( _root );
		bool schemeResolved = f.find("://") != uf::stl::string::npos;
		bool fAlreadyHasScheme = schemeResolved;

		// process macros (only %root%)
		if ( !schemeResolved && f.starts_with("%root%") ) {
			f = f.substr(6);
			root = "data://";
			schemeResolved = true;
		}

		// explicit relative path
		if ( !schemeResolved && f.length() >= 2 && f.substr(0, 2) == "./" ) {
			f = f.substr(2);
			if ( root.empty() ) schemeResolved = true;
		}

		// explicit absolute
		if ( !fAlreadyHasScheme && f.length() > 0 && f[0] == '/' ) {
			root = "";
		}

		// deduce scheme and apply
		if ( !schemeResolved && f.substr(0, 3) != "../" ) {
			if ( root.empty() ) {
				uf::stl::string deducedScheme = uf::io::assetScheme( f );

				if ( deducedScheme != "data://" ) {
					root = deducedScheme;
					schemeResolved = true;
				} else {
					root = deducedScheme;
				}
			}
		}

		// absolute system paths
		bool isWindowsAbsolute = !fAlreadyHasScheme && f.length() >= 2 && std::isalpha(f[0]) && f[1] == ':';
		bool isUnixAbsolute = !fAlreadyHasScheme && !schemeResolved && f.length() > 0 && f[0] == '/';

		if ( isWindowsAbsolute || isUnixAbsolute ) {
			root = "sys://";
			if ( isUnixAbsolute ) f = f.substr(1); // might not be necessary because of the final normalization?
		}

		// apply root
		if ( !fAlreadyHasScheme && !root.empty() ) {
			if ( f.length() > 0 && f[0] == '/' ) f = f.substr(1);

			// remove cringe like `tex://textures/`
			for ( const auto& mount : uf::vfs::mounts ) {
				if ( root != mount.prefix ) continue;

				uf::stl::string target = mount.path;
				if (target.back() == '/') target.pop_back();
				target = target.substr(target.find_last_of('/') + 1) + "/";

				if ( f.starts_with(target) ) f = f.substr(target.length());
				break;
			}

			if ( root.back() != '/') root += "/";
			f = root + f;
		}

		return CACHE_QUERY(uf::io::normalize( f ));
	}
}

const uf::stl::string uf::io::root = UF_IO_ROOT;

uf::stl::string uf::io::absolute( const uf::stl::string& path ) {
	QUERY_CACHE( path, uf::stl::string );
	char buff[FILENAME_MAX];
	GetCurrentDir( buff, FILENAME_MAX );
	return CACHE_QUERY(uf::stl::string(buff) + path);
}
uf::stl::string uf::io::filename( const uf::stl::string& str ) {
	QUERY_CACHE( str, uf::stl::string );
	return CACHE_QUERY(str.substr( str.find_last_of('/') + 1 ));
}
uf::stl::string uf::io::basename( const uf::stl::string& filename ) {
	QUERY_CACHE( filename, uf::stl::string );
	uf::stl::string str = uf::io::filename( filename );
	uf::stl::string extension = uf::io::extension( filename );
	return CACHE_QUERY(uf::string::replace( str, "." + extension, "" ));
}
uf::stl::string uf::io::extension( const uf::stl::string& str ) {
	QUERY_CACHE( str, uf::stl::string );
	uf::stl::string filename = uf::io::filename(str);
	return CACHE_QUERY(filename.substr( filename.find_last_of('.') + 1 ));
}
uf::stl::string uf::io::extension( const uf::stl::string& str, int32_t count ) {
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
uf::stl::string uf::io::remove_extension( const uf::stl::string& str ) {
	QUERY_CACHE( str, uf::stl::string );
	size_t last_dot = str.find_last_of('.');
	size_t last_slash = str.find_last_of('/');
	if ( last_dot != uf::stl::string::npos && (last_slash == uf::stl::string::npos || last_dot > last_slash) ) {
		return CACHE_QUERY(str.substr( 0, last_dot ));
	}
	return CACHE_QUERY(str);
}
uf::stl::string uf::io::replace_extension( const uf::stl::string& str, const uf::stl::string& new_ext ) {
	QUERY_CACHE( str + new_ext, uf::stl::string );
	uf::stl::string base = uf::io::remove_extension( str );
	if ( new_ext.empty() ) return CACHE_QUERY(base);
	if ( new_ext[0] == '.' ) return CACHE_QUERY(base + new_ext);
	return CACHE_QUERY(base + "." + new_ext);
}
uf::stl::string uf::io::directory( const uf::stl::string& str ) {
	QUERY_CACHE( str, uf::stl::string );
	return CACHE_QUERY(str.substr( 0, str.find_last_of('/') ) + "/");
}
uf::stl::vector<uf::stl::string> uf::io::list( const uf::stl::string& dir, const uf::stl::string& extension ) {
	QUERY_CACHE( dir + extension, uf::stl::vector<uf::stl::string> );
	return CACHE_QUERY(uf::vfs::list( uf::io::resolveURI(dir), extension ));
}
size_t uf::io::size( const uf::stl::string& filename ) {
	QUERY_CACHE( filename, size_t );
	return CACHE_QUERY(uf::vfs::size( uf::io::resolveURI(filename) ));
}
uf::stl::string uf::io::normalize( const uf::stl::string& path ) {
	QUERY_CACHE( path, uf::stl::string );
	uf::stl::string clean = path;

	std::replace(clean.begin(), clean.end(), '\\', '/');

	clean = uf::string::replace(clean, "/./", "/", false); // explicitly set as non-regex

	size_t schemePos = clean.find("://");
	uf::stl::string scheme = "";
	if ( schemePos != uf::stl::string::npos ) {
		scheme = clean.substr(0, schemePos + 3);
		clean = clean.substr(schemePos + 3);
	}

	clean = uf::string::replace(clean, "//", "/");

	return CACHE_QUERY(scheme + clean);
}
// would just use readAsBuffer and convert to string, but that's double the memory cost
bool uf::io::readAsString( uf::stl::string& buffer, const uf::stl::string& _filename, const uf::stl::string& hash ) {
	buffer.clear();
	uf::stl::string filename = uf::io::normalize( _filename );
	uf::stl::string extension = uf::io::extension( filename );

	if ( extension == "gz" || extension == "lz4" ) {
		auto decompressed = uf::io::decompress( filename );
		buffer.resize(decompressed.size());
		buffer.assign(decompressed.begin(), decompressed.end());
	} else {
		uf::stl::vector<uint8_t> tempBuffer;
		if ( !uf::vfs::read( filename, tempBuffer ) ) {
			UF_MSG_ERROR("Error: Could not open file: {}", filename);
			return false;
		}
		buffer.assign(tempBuffer.begin(), tempBuffer.end());
	}

	uf::stl::string expected = "";
	if ( hash != "" && (expected = uf::string::sha256( buffer )) != hash ) {
		UF_MSG_ERROR("Error: Hash mismatch for file {}; expecting {}, got {}", filename, hash, expected);
	}
	return true;
}
bool uf::io::readAsBuffer( uf::stl::vector<uint8_t>& buffer, const uf::stl::string& _filename, const uf::stl::string& hash ) {
	buffer.clear();
	uf::stl::string filename = uf::io::normalize( _filename );
	uf::stl::string extension = uf::io::extension( filename );

	if ( extension == "gz" || extension == "lz4" ) {
		uf::io::decompress( buffer, filename );
	} else {
		if ( !uf::vfs::read( filename, buffer ) ) {
			UF_MSG_ERROR("Error: Could not open file: {}", filename);
			return false;
		}
	}

	uf::stl::string expected = "";
	if ( !hash.empty() && (expected = uf::string::sha256( buffer )) != hash ) {
		UF_MSG_ERROR("Error: Hash mismatch for file {}; expecting {}, got {}", filename, hash, expected);
	}
	return true;
}

bool uf::io::readAsBuffer( uf::stl::vector<uint8_t>& buffer, const uf::stl::string& _filename, size_t start, size_t len, const uf::stl::string& hash ) {
	buffer.clear();
	uf::stl::string filename = uf::io::normalize(_filename);
	uf::stl::string extension = uf::io::extension(filename);

	if ( extension == "gz" || extension == "lz4" ) {
		uf::io::decompress( buffer, filename, start, len );
	} else {
		if (!uf::vfs::readRange(filename, start, len, buffer)) {
			UF_MSG_ERROR("Error: Could not open file range: {}", filename);
			return false;
		}
	}

	uf::stl::string expected;
	if ( !hash.empty() && (expected = uf::string::sha256( buffer )) != hash ) {
		UF_MSG_ERROR("Error: Hash mismatch for file {}; expecting {}, got {}", filename, hash, expected);
		// should probably clear
	}
	return true;
}

bool uf::io::readAsBuffer( uf::stl::vector<uint8_t>& buffer,  const uf::stl::string& _filename, const uf::stl::vector<pod::Range>& ranges, const uf::stl::string& hash ) {
	buffer.clear();
	uf::stl::string filename = uf::io::normalize(_filename);
	uf::stl::string extension = uf::io::extension(filename);

	if ( extension == "gz" || extension == "lz4" ) {
		uf::io::decompress( buffer, filename, ranges );
	} else {
		if (!uf::vfs::readRanges(filename, ranges, buffer)) {
			UF_MSG_ERROR("Error: Could not open file ranges: {}", filename);
			return false;
		}
	}

	uf::stl::string expected;
	if ( !hash.empty() && (expected = uf::string::sha256( buffer )) != hash ) {
		UF_MSG_ERROR("Error: Hash mismatch for file {}; expecting {}, got {}", filename, hash, expected);
		// should probably clear
	}
	return true;
}

bool uf::io::readScatter( const uf::stl::string& filename, uf::stl::vector<pod::ScatterRequest>& requests ) {
#if UF_ENV_DREAMCAST
	const size_t THRESHOLD = 16 * 1024; // 16K
	const size_t CHUNK_SIZE = 8 * 1024;
#else
	const size_t THRESHOLD = 16 * 1024 * 1024; // 16M
	const size_t CHUNK_SIZE = 0;
#endif

	uf::stl::string extension = uf::io::extension(filename);
	size_t fileSize = uf::io::size(filename);

	if ( 0 < fileSize && fileSize <= THRESHOLD ) {
		uf::stl::vector<uint8_t> fullBuffer;
		if ( extension == "gz" || extension == "lz4" ) uf::io::decompress(fullBuffer, filename);
		else uf::vfs::read(filename, fullBuffer);

		for ( auto& req : requests ) {
			if ( req.start < fullBuffer.size() ) {
				size_t copyLen = std::min(req.len, fullBuffer.size() - req.start);
				uf::stl::memcpy(req.dest, fullBuffer.data() + req.start, copyLen);
			}
		}
		return true;
	}

	if ( extension == "gz" ) return ext::zlib::decompressScatter(filename, requests);
	if ( extension == "lz4" ) return ext::lz4::decompressScatter(filename, requests);

	pod::File file = uf::vfs::open(filename);
	if ( !file ) return false;

	for ( auto& req : requests ) {
		if ( req.len == 0 ) continue;

		if ( !file.seek(file.handle, req.start, SEEK_SET) ) continue;
		file.stream(file.handle, req.dest, req.len, CHUNK_SIZE);
	}

	file.close(file.handle);
	return true;
}

size_t uf::io::write( const uf::stl::string& filename, const void* buffer, size_t size ) {
	uf::stl::string extension = uf::io::extension( filename );
	if ( extension == "gz" || extension == "lz4" ) return uf::io::compress( filename, buffer, size );

	return uf::vfs::write( uf::io::resolveURI( filename ), buffer, size );
}

// indirection for different compression formats, currently only using zlib's gzFile shit
bool uf::io::decompress( uf::stl::vector<uint8_t>& buffer, const uf::stl::string& filename ) {
	uf::stl::string extension = uf::io::extension( filename );
	if ( extension == "gz" ) return ext::zlib::decompressFromFile( buffer, filename );
	if ( extension == "lz4" ) return ext::lz4::decompressFromFile( buffer, filename );
	UF_MSG_ERROR("unsupported compression format requested: {}", extension);
	return false;
}
bool uf::io::decompress( uf::stl::vector<uint8_t>& buffer, const uf::stl::string& filename, size_t start, size_t len ) {
	uf::stl::string extension = uf::io::extension( filename );
	if ( extension == "gz" ) return ext::zlib::decompressFromFile( buffer, filename, start, len );
	if ( extension == "lz4" ) return ext::lz4::decompressFromFile( buffer, filename, start, len );
	UF_MSG_ERROR("unsupported compression format requested: {}", extension);
	return false;
}
bool uf::io::decompress( uf::stl::vector<uint8_t>& buffer, const uf::stl::string& filename, const uf::stl::vector<pod::Range>& ranges ) {
	uf::stl::string extension = uf::io::extension( filename );
	if ( extension == "gz" ) return ext::zlib::decompressFromFile( buffer, filename, ranges );
	if ( extension == "lz4" ) return ext::lz4::decompressFromFile( buffer, filename, ranges );
	UF_MSG_ERROR("unsupported compression format requested: {}", extension);
	return false;
}
size_t uf::io::compress( const uf::stl::string& filename, const void* buffer, size_t size ) {
	uf::stl::string extension = uf::io::extension( filename );
#if !UF_ENV_DREAMCAST
	if ( extension == "gz" ) return ext::zlib::compressToFile( filename, buffer, size );
	if ( extension == "lz4" ) return ext::lz4::compressToFile( filename, buffer, size );
#endif
	UF_MSG_ERROR("unsupported compression format requested: {}", extension);
	return 0;
}

uf::stl::string uf::io::hash( const uf::stl::string& filename ) {
	QUERY_CACHE( filename, uf::stl::string );
	return CACHE_QUERY(uf::string::sha256( uf::io::readAsBuffer( filename ) ));
}
bool uf::io::exists( const uf::stl::string& filename ) {
	QUERY_CACHE( filename, bool );
	return CACHE_QUERY(uf::vfs::exists( uf::io::resolveURI(filename) ));
}
size_t uf::io::mtime( const uf::stl::string& filename ) {
	QUERY_CACHE( filename, size_t );
	return CACHE_QUERY(uf::vfs::mtime( uf::io::resolveURI(filename) ));
}
bool uf::io::mkdir( const uf::stl::string& filename ) {
	return uf::vfs::mkdir( uf::io::resolveURI(filename) );
}

uf::stl::string uf::io::assetType( const uf::stl::string& _filename ) {
	QUERY_CACHE( _filename, uf::stl::string );
	// remove .gz
	uf::stl::string filename = uf::string::replace( _filename, ".gz", "" );

	// grab filename and extension
	uf::stl::string basename = uf::io::filename( filename );
	uf::stl::string extension = uf::io::extension( filename );

	// a map does allocations, an if ladder is easy
	if ( basename == "graph.json" ) return CACHE_QUERY("model");
	if ( basename == "scene.json" ) return CACHE_QUERY("scene");
	if ( extension == "json" ) return CACHE_QUERY("entity");
	if ( extension == "png" ) return CACHE_QUERY("texture");
	if ( extension == "glb" ) return CACHE_QUERY("model");
	if ( extension == "gltf" ) return CACHE_QUERY("model");
	if ( extension == "graph" ) return CACHE_QUERY("model");
	if ( extension == "ogg" ) return CACHE_QUERY("audio");
	if ( extension == "wav" ) return CACHE_QUERY("audio");
	if ( extension == "spv" ) return CACHE_QUERY("shader");
	if ( extension == "lua" ) return CACHE_QUERY("script");
	return CACHE_QUERY("");
}

// to-do: map to above
uf::stl::string uf::io::assetScheme( const uf::stl::string& _filename ) {
	QUERY_CACHE( _filename, uf::stl::string );
	uf::stl::string filename = uf::string::replace( _filename, ".gz", "" );
	uf::stl::string basename = uf::io::filename( filename );
	uf::stl::string extension = uf::io::extension( filename );

	if ( basename == "graph.json" ) return CACHE_QUERY("mdl://");
	if ( basename == "scene.json" ) return CACHE_QUERY("scene://");
	if ( extension == "json" ) return CACHE_QUERY("ent://");
	if ( extension == "png" || extension == "dtex" ) return CACHE_QUERY("tex://");
	if ( extension == "glb" || extension == "gltf" || extension == "graph" ) return CACHE_QUERY("mdl://");
	if ( extension == "ogg" || extension == "wav" || extension == "adp" ) return CACHE_QUERY("snd://");
	if ( extension == "spv" ) return CACHE_QUERY("spv://");
	if ( extension == "lua" ) return CACHE_QUERY("lua://");

	return CACHE_QUERY("data://");
}

uf::stl::string uf::io::resolveURI( const uf::stl::string& _filename, const uf::stl::string& _root ) {
	QUERY_CACHE( _root + _filename, uf::stl::string );
	return CACHE_QUERY(uf::io::preferred( ::resolveURI( _filename, _root ) ));
}

// attempts to coerce files into a preferred one if it exists
uf::stl::string uf::io::preferred( const uf::stl::string& filename ) {
	QUERY_CACHE( filename, uf::stl::string );
	uf::stl::string compressionSuffix = "";
	if ( filename.ends_with(".gz") ) {
		compressionSuffix = ".gz";
	} else if ( filename.ends_with(".lz4") ) {
		compressionSuffix = ".lz4";
	}

	uf::stl::string cleanFilename = filename;
	if ( !compressionSuffix.empty() ) {
		cleanFilename = cleanFilename.substr(0, cleanFilename.length() - compressionSuffix.length());
	}

	auto extension = "." + uf::io::extension( cleanFilename );
	auto assetType = uf::io::assetType( extension );

#if UF_ENV_DREAMCAST
	static const uf::stl::unordered_map<uf::stl::string, uf::stl::vector<uf::stl::string>> PREFERENCES = {
		{ "audio",   { ".adp", ".wav", ".ogg" } },
		{ "texture", { ".dtex", ".png", ".jpg" } }
	};
#else
	static const uf::stl::unordered_map<uf::stl::string, uf::stl::vector<uf::stl::string>> PREFERENCES = {
		{ "audio",   { ".ogg", ".wav", ".adp" } },
		{ "texture", { ".png", ".jpg", ".dtex" } }
	};
#endif

	auto it = PREFERENCES.find( assetType );
	if ( it == PREFERENCES.end() ) {
		return CACHE_QUERY(filename);
	}

	const auto& extensionList = it->second;
	uf::stl::string basePath = cleanFilename.substr(0, cleanFilename.length() - extension.length());

	for ( const auto& prefExt : extensionList ) {
		uf::stl::string candidatePath = ::resolveURI( basePath + prefExt + compressionSuffix );

		if ( uf::vfs::exists( candidatePath ) ) {
			return CACHE_QUERY(candidatePath);
		}
		uf::stl::string cachePath = uf::io::normalize( "cache://" + candidatePath );
		if ( uf::vfs::exists( cachePath ) ) {
			return CACHE_QUERY(cachePath);
		}
	}

	return CACHE_QUERY(filename);
}