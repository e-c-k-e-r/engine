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
uf::stl::vector<uint8_t> UF_API ext::zlib::compress( const void* data, size_t size ) {
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
uf::stl::vector<uint8_t> UF_API ext::zlib::decompress( const void* data, size_t size ) {
	uf::stl::vector<uint8_t> buffer;

	return buffer;
}
*/
uf::stl::vector<uint8_t> UF_API ext::zlib::decompressFromFile( const uf::stl::string& filename ) {
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
size_t UF_API ext::zlib::compressToFile( const uf::stl::string& filename, const void* data, size_t size ) {
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
#endif