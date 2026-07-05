#if UF_USE_ZLIB
#include <uf/ext/zlib/zlib.h>
#include <uf/utils/io/file.h>
#include <uf/utils/io/vfs.h>
#include <uf/utils/userdata/userdata.h>
#include <cstring>
#include <algorithm>
#include <fstream>

#if UF_ENV_DREAMCAST
#include <zlib/zlib.h>
#else
#include <zlib.h>
#endif


size_t ext::zlib::bufferSize = 16384;

bool ext::zlib::decompressFromFile( uf::stl::vector<uint8_t>& buffer, const uf::stl::string& filename ) {
	size_t fileSize = uf::vfs::size(filename);
	if (fileSize == 0) {
		UF_MSG_ERROR("Zlib: file not found or empty: {}", filename);
		return false;
	}

	z_stream strm{};
	if (inflateInit2(&strm, 15 + 32) != Z_OK) return false;

	size_t offset = 0;
	uint8_t outBuffer[ext::zlib::bufferSize];

	while (offset < fileSize) {
		size_t bytesToRead = std::min(ext::zlib::bufferSize, fileSize - offset);
		uf::stl::vector<uint8_t> temp;
		if (!uf::vfs::readRange(filename, offset, bytesToRead, temp)) break;

		strm.avail_in = (uInt)temp.size();
		strm.next_in = temp.data();
		offset += temp.size();

		do {
			strm.avail_out = sizeof(outBuffer);
			strm.next_out = outBuffer;
			int ret = inflate(&strm, Z_NO_FLUSH);
			if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
				UF_MSG_ERROR("Zlib: inflate error on file: {}", filename);
				inflateEnd(&strm);
				return false;
			}
			size_t have = sizeof(outBuffer) - strm.avail_out;
			buffer.insert(buffer.end(), outBuffer, outBuffer + have);
		} while (strm.avail_out == 0);
	}

	inflateEnd(&strm);
	return true;
}

bool ext::zlib::decompressFromFile( uf::stl::vector<uint8_t>& buffer, const uf::stl::string& filename, size_t start, size_t len ) {
	size_t fileSize = uf::vfs::size(filename);
	if (fileSize == 0) return false;

	z_stream strm{};
	if (inflateInit2(&strm, 15 + 32) != Z_OK) return false;

	size_t offset = 0;
	size_t uncompressedOffset = 0;
	uint8_t outBuffer[ext::zlib::bufferSize];

	while (offset < fileSize) {
		size_t bytesToRead = std::min(ext::zlib::bufferSize, fileSize - offset);
		uf::stl::vector<uint8_t> temp;
		if (!uf::vfs::readRange(filename, offset, bytesToRead, temp)) break;

		strm.avail_in = (uInt)temp.size();
		strm.next_in = temp.data();
		offset += temp.size();

		do {
			strm.avail_out = sizeof(outBuffer);
			strm.next_out = outBuffer;
			int ret = inflate(&strm, Z_NO_FLUSH);
			if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
				inflateEnd(&strm);
				return false;
			}
			size_t have = sizeof(outBuffer) - strm.avail_out;

			// Calculate if this chunk overlaps with our requested range
			size_t chunkStart = uncompressedOffset;
			size_t chunkEnd = uncompressedOffset + have;

			if (chunkEnd > start && chunkStart < start + len) {
				size_t copyStart = (chunkStart < start) ? (start - chunkStart) : 0;
				size_t copyLen = std::min(have - copyStart, (start + len) - (chunkStart + copyStart));
				buffer.insert(buffer.end(), outBuffer + copyStart, outBuffer + copyStart + copyLen);
			}

			uncompressedOffset += have;

			// If we've reached the end of the requested length, we can abort early!
			if (uncompressedOffset >= start + len) {
				inflateEnd(&strm);
				return true;
			}
		} while (strm.avail_out == 0);
	}

	inflateEnd(&strm);
	return true;
}

bool ext::zlib::decompressFromFile( uf::stl::vector<uint8_t>& buffer, const uf::stl::string& filename, const uf::stl::vector<pod::Range>& ranges ) {
	if ( ranges.empty() ) return false;

	uf::stl::vector<pod::Range> sortedRanges = ranges;
	std::sort( sortedRanges.begin(), sortedRanges.end(), [](const pod::Range& a, const pod::Range& b) { return a.start < b.start; } );

	size_t fileSize = uf::vfs::size(filename);
	if (fileSize == 0) return false;

	z_stream strm{};
	if (inflateInit2(&strm, 15 + 32) != Z_OK) return false;

	size_t offset = 0;
	size_t uncompressedOffset = 0;
	uint8_t outBuffer[ext::zlib::bufferSize];
	size_t currentRangeIdx = 0;

	while (offset < fileSize && currentRangeIdx < sortedRanges.size()) {
		size_t bytesToRead = std::min(ext::zlib::bufferSize, fileSize - offset);
		uf::stl::vector<uint8_t> temp;
		if (!uf::vfs::readRange(filename, offset, bytesToRead, temp)) break;

		strm.avail_in = (uInt)temp.size();
		strm.next_in = temp.data();
		offset += temp.size();

		do {
			strm.avail_out = sizeof(outBuffer);
			strm.next_out = outBuffer;
			int ret = inflate(&strm, Z_NO_FLUSH);
			if (ret < 0 && ret != Z_BUF_ERROR) {
				inflateEnd(&strm);
				return false;
			}
			size_t have = sizeof(outBuffer) - strm.avail_out;

			size_t chunkStart = uncompressedOffset;
			size_t chunkEnd = uncompressedOffset + have;

			// Check all remaining ranges against this chunk
			for (size_t i = currentRangeIdx; i < sortedRanges.size(); ++i) {
				const auto& r = sortedRanges[i];
				if (chunkEnd > r.start && chunkStart < r.start + r.len) {
					size_t copyStart = (chunkStart < r.start) ? (r.start - chunkStart) : 0;
					size_t copyLen = std::min(have - copyStart, (r.start + r.len) - (chunkStart + copyStart));
					buffer.insert(buffer.end(), outBuffer + copyStart, outBuffer + copyStart + copyLen);
				}
				if (chunkEnd >= r.start + r.len) {
					currentRangeIdx = i + 1; // Move past completed ranges
				}
			}
			uncompressedOffset += have;
		} while (strm.avail_out == 0);
	}

	inflateEnd(&strm);
	return true;
}

bool ext::zlib::decompressFromMemory( uf::stl::vector<uint8_t>& dst, const void* src, size_t size, size_t usize ) {
	if (size == 0) return false;

	z_stream strm{};
	if (inflateInit2(&strm, 15 + 32) != Z_OK) return false;

	strm.avail_in = (uInt)size;
	strm.next_in = (Bytef*)src;

	dst.resize(usize);
	strm.avail_out = (uInt)usize;
	strm.next_out = dst.data();

	int ret = inflate(&strm, Z_FINISH);
	inflateEnd(&strm);

	return (ret == Z_STREAM_END || ret == Z_OK);
}

size_t ext::zlib::compressToFile( const uf::stl::string& filename, const void* data, size_t size ) {
	z_stream strm{};
	// 31 means gzip format
	if (deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY) != Z_OK) return 0;

	strm.avail_in = (uInt)size;
	strm.next_in = (Bytef*)data;

	uf::stl::vector<uint8_t> compressedData;
	uint8_t outBuffer[ext::zlib::bufferSize];
	int ret;
	do {
		strm.avail_out = sizeof(outBuffer);
		strm.next_out = outBuffer;
		ret = deflate(&strm, Z_FINISH);
		size_t have = sizeof(outBuffer) - strm.avail_out;

		compressedData.insert(compressedData.end(), outBuffer, outBuffer + have);
	} while (strm.avail_out == 0);

	deflateEnd(&strm);

	return uf::vfs::write( filename, compressedData.data(), compressedData.size() );
}

bool ext::zlib::directory( const uf::stl::vector<uint8_t>& buffer, uf::stl::unordered_map<uf::stl::string, pod::ZipEntry>& entries ) {
	if ( buffer.size() < 22 ) return false;

	int eocdOffset = -1;
	for ( int i = (int)buffer.size() - 22; i >= 0; --i ) {
		if (buffer[i] == 0x50 && buffer[i+1] == 0x4b && buffer[i+2] == 0x05 && buffer[i+3] == 0x06) {
			eocdOffset = i;
			break;
		}
	}

	if ( eocdOffset == -1 ) {
		UF_MSG_ERROR("ZIP Parse: Could not find End of Central Directory!");
		return false;
	}

	const uint8_t* eocd = buffer.data() + eocdOffset;
	uint16_t numEntries = *(const uint16_t*)(eocd + 10);
	uint32_t cdOffset   = *(const uint32_t*)(eocd + 16);

	if ( cdOffset >= buffer.size() ) return false;

	size_t currentOffset = cdOffset;
	for (uint16_t i = 0; i < numEntries; ++i) {
		if (currentOffset + 46 > buffer.size()) break;

		const uint8_t* cdHeader = buffer.data() + currentOffset;

		if (*(const uint32_t*)cdHeader != 0x02014b50) break;

		uint16_t compressionMethod = *(const uint16_t*)(cdHeader + 10);
		uint32_t compressedSize	= *(const uint32_t*)(cdHeader + 20);
		uint32_t uncompressedSize  = *(const uint32_t*)(cdHeader + 24);
		uint16_t nameLen		   = *(const uint16_t*)(cdHeader + 28);
		uint16_t extraLen		  = *(const uint16_t*)(cdHeader + 30);
		uint16_t commentLen		= *(const uint16_t*)(cdHeader + 32);
		uint32_t localHeaderOffset = *(const uint32_t*)(cdHeader + 42);

		uf::stl::string filename((const char*)(cdHeader + 46), nameLen);

		std::replace(filename.begin(), filename.end(), '\\', '/');
		std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);

		if ( localHeaderOffset + 30 <= buffer.size() ) {
			const uint8_t* localHeader = buffer.data() + localHeaderOffset;

			if (*(const uint32_t*)localHeader == 0x04034b50) {
				uint16_t localNameLen  = *(const uint16_t*)(localHeader + 26);
				uint16_t localExtraLen = *(const uint16_t*)(localHeader + 28);

				size_t dataOffset = localHeaderOffset + 30 + localNameLen + localExtraLen;

				entries[filename] = {
					dataOffset,
					compressedSize,
					uncompressedSize,
					compressionMethod
				};
			}
		}

		currentOffset += 46 + nameLen + extraLen + commentLen;
	}

	return true;
}

//
namespace {
	struct ZipMountState {
		uf::stl::vector<uint8_t> buffer;
		uf::stl::unordered_map<uf::stl::string, pod::ZipEntry> entries;
	};

	bool vfs_exists( pod::Mount& mount, const uf::stl::string& file ){
		auto& state = uf::pointeredUserdata::get<ZipMountState>( mount.userdata );
		return state.entries.find(file) != state.entries.end();
	};
	size_t vfs_size( pod::Mount& mount, const uf::stl::string& file ){
		auto& state = uf::pointeredUserdata::get<ZipMountState>( mount.userdata );
		auto it = state.entries.find(file);
		return (it != state.entries.end()) ? it->second.uncompressedSize : 0;
	};
	bool vfs_read( pod::Mount& mount, const uf::stl::string& file, uf::stl::vector<uint8_t>& buffer ){
		auto& state = uf::pointeredUserdata::get<ZipMountState>( mount.userdata );
		auto it = state.entries.find( file );
		if ( it == state.entries.end() ) return false;

		const auto& entry = it->second;
		const uint8_t* fileData = state.buffer.data() + entry.offset;

		if ( entry.compressionMethod == 0 ) {
			buffer.assign(fileData, fileData + entry.uncompressedSize);
			return true;
		}
		if ( entry.compressionMethod == 8 ) {
			return ext::zlib::decompressFromMemory(buffer, fileData, entry.compressedSize, entry.uncompressedSize);
		}
		return false;
	};
}

pod::Mount ext::zlib::createZipMount( const uf::stl::string& uri, uf::stl::vector<uint8_t>& buffer, int priority ) {
	uf::stl::string prefix;
	uf::stl::string path;
	uf::io::splitUri( uri, prefix, path );

	pod::Mount mount;
	mount.prefix = prefix;
	mount.path = path;
	mount.priority = priority;
	mount.userdata = uf::pointeredUserdata::create<ZipMountState>();
	mount.exists = ::vfs_exists;
	mount.size = ::vfs_size;
	mount.read = ::vfs_read;
	
	auto& state = uf::pointeredUserdata::get<ZipMountState>( mount.userdata );
	state.buffer = buffer; // should be a move?
	ext::zlib::directory( state.buffer, state.entries );

	return mount;
}

#endif