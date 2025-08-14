#include <uf/ext/zlib/zlib.h>
#if UF_USE_ZLIB
#include <uf/utils/io/file.h>
#include <cstring>
#if UF_ENV_DREAMCAST
#include <zlib/zlib.h>
#else
#include <zlib.h>
#endif

size_t ext::zlib::bufferSize = 2048;
/*
uf::stl::vector<uint8_t> ext::zlib::compress( const void* data, size_t size ) {
	uf::stl::vector<uint8_t> buffer;

	// zlib struct
	z_stream defstream;
	defstream.zalloc = Z_NULL;
	defstream.zfree = Z_NULL;
	defstream.opaque = Z_NULL;
	// setup "a" as the input and "b" as the compressed output
	defstream.avail_in = (uInt) strlen(a)+1; // size of input, string + terminator
	defstream.next_in = (Bytef*) a; // input char array
	defstream.avail_out = (uInt) sizeof(b); // size of output
	defstream.next_out = (Bytef*) b; // output char array
	
	// the actual compression work.
	deflateInit(&defstream, Z_BEST_COMPRESSION);
	deflate(&defstream, Z_FINISH);
	deflateEnd(&defstream);

	return buffer;
}
uf::stl::vector<uint8_t> ext::zlib::decompress( const void* data, size_t size ) {
	uf::stl::vector<uint8_t> buffer;

	return buffer;
}
*/
uf::stl::vector<uint8_t> ext::zlib::decompressFromFile( const uf::stl::string& filename ) {
	uf::stl::vector<uint8_t> buffer;
	
	gzFile in = gzopen( filename.c_str(), "rb" );
	if ( !in ) {
		UF_MSG_ERROR("Zlib: failed to open file for read: {}", filename);
		return buffer;
	}

	size_t read{};
	uint8_t gzBuffer[ext::zlib::bufferSize];
	while ( (read = gzread( in, gzBuffer, ext::zlib::bufferSize ) ) > 0 ) {
	//	buffer.reserve(buffer.size() + read);
	//	buffer.insert( buffer.end(), &gzBuffer[0], &gzBuffer[ext::zlib::bufferSize] );
		size_t s = buffer.size();
		buffer.resize(s + read);
		memcpy( &buffer[s], &gzBuffer[0], read );
	}
	gzclose( in );
	return buffer;
}
size_t ext::zlib::compressToFile( const uf::stl::string& filename, const void* data, size_t size ) {
	gzFile out = gzopen( filename.c_str(), "wb" );
	if ( !out ) {
		UF_MSG_ERROR("Zlib: failed to open file for write: {}", filename);
		return 0;
	}
#if 1
	gzwrite( out, data, size );
#else
	size_t wrote{};
	while ( wrote < size ) {
		size_t write = std::min( ext::zlib::bufferSize, size - wrote );
		gzwrite( out, data + wrote, write );
		wrote += write;
	}
#endif
	gzclose( out );
	return uf::io::size( filename );
}

uf::stl::vector<uint8_t> ext::zlib::decompressFromFile( const uf::stl::string& filename, size_t start, size_t len ) {
	uf::stl::vector<uint8_t> buffer;

	gzFile in = gzopen(filename.c_str(), "rb");
	if ( !in ) {
		UF_MSG_ERROR("Zlib: failed to open file for read: {}", filename);
		return buffer;
	}

	// Seek to the requested uncompressed offset
	if (gzseek(in, static_cast<z_off_t>(start), SEEK_SET) == -1) {
		UF_MSG_ERROR("Zlib: failed to seek to position {} in file {}", start, filename);
		gzclose(in);
		return buffer;
	}

	buffer.resize(len);
	int bytesRead = gzread(in, buffer.data(), static_cast<unsigned int>(len));
	if (bytesRead < 0) {
		int errnum;
		const char* errMsg = gzerror(in, &errnum);
		UF_MSG_ERROR("Zlib read error: {}", errMsg ? errMsg : "unknown error");
		buffer.clear();
	} else {
		buffer.resize(static_cast<size_t>(bytesRead)); // Adjust size in case EOF occurs early
	}

	gzclose(in);
	return buffer;
}

uf::stl::vector<uint8_t> ext::zlib::decompressFromFile( const uf::stl::string& filename, const uf::stl::vector<pod::Range>& ranges ) {
	uf::stl::vector<uint8_t> result;

	if ( ranges.empty() ) {
		return result;
	}

	// ensure they're ordered
	uf::stl::vector<pod::Range> sortedRanges = ranges;
	std::sort( sortedRanges.begin(), sortedRanges.end(), [](const pod::Range& a, const pod::Range& b) { return a.start < b.start; } );

	gzFile in = gzopen(filename.c_str(), "rb");
	if ( !in ) {
		UF_MSG_ERROR("Zlib: failed to open file for read: {}", filename);
		return result;
	}

	for ( const auto& r : sortedRanges ) {
		if ( gzseek(in, static_cast<z_off_t>(r.start), SEEK_SET) == -1 ) {
			UF_MSG_ERROR("Zlib: failed to seek to position {} in file {}", r.start, filename);
			gzclose(in);
			return uf::stl::vector<uint8_t>(); // Return empty on failure
		}

		size_t oldSize = result.size();
		result.resize(oldSize + r.len);

		int bytesRead = gzread(in, result.data() + oldSize, static_cast<unsigned int>(r.len));
		if ( bytesRead < 0 ) {
			int errnum;
			const char* errMsg = gzerror(in, &errnum);
			UF_MSG_ERROR("Zlib read error: {}", errMsg ? errMsg : "unknown error");
			gzclose(in);
			return uf::stl::vector<uint8_t>(); // Return empty on error
		}

		// In case EOF ended early
		result.resize(oldSize + static_cast<size_t>(bytesRead));
	}

	gzclose(in);
	return result;
}
#endif