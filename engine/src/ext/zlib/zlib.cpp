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


size_t ext::zlib::bufferSize = 16384; // 16K

bool ext::zlib::decompressFromFile( uf::stl::vector<uint8_t>& buffer, const uf::stl::string& filename, int flag ) {
	size_t fileSize = uf::vfs::size(filename);
	if (fileSize == 0) {
		UF_MSG_ERROR("Zlib: file not found or empty: {}", filename);
		return false;
	}

	z_stream strm{};
	if (inflateInit2(&strm, flag) != Z_OK) return false;

	uint8_t outBuffer[ext::zlib::bufferSize];

	bool success = uf::vfs::stream(filename, ext::zlib::bufferSize, [&](const uint8_t* data, size_t size) -> bool {
		strm.avail_in = (uInt)size;
		strm.next_in = (Bytef*)data;

		do {
			strm.avail_out = sizeof(outBuffer);
			strm.next_out = outBuffer;
			int ret = inflate(&strm, Z_NO_FLUSH);
			if ( ret < 0 && ret != Z_BUF_ERROR ) return false;

			size_t have = sizeof(outBuffer) - strm.avail_out;
			buffer.insert(buffer.end(), outBuffer, outBuffer + have);

		} while (strm.avail_out == 0);

		return true;
	});

	inflateEnd(&strm);
	return success;
}

bool ext::zlib::decompressFromFile( uf::stl::vector<uint8_t>& buffer, const uf::stl::string& filename, size_t start, size_t len, int flag ) {
	size_t fileSize = uf::vfs::size(filename);
	if (fileSize == 0) return false;

	z_stream strm{};
	if (inflateInit2(&strm, flag) != Z_OK) return false;

	size_t uncompressedOffset = 0;
	uint8_t outBuffer[ext::zlib::bufferSize];

	bool success = uf::vfs::stream(filename, ext::zlib::bufferSize, [&](const uint8_t* data, size_t size) -> bool {
		strm.avail_in = (uInt)size;
		strm.next_in = (Bytef*)data;

		do {
			strm.avail_out = sizeof(outBuffer);
			strm.next_out = outBuffer;
			int ret = inflate(&strm, Z_NO_FLUSH);
			if ( ret < 0 && ret != Z_BUF_ERROR ) return false;

			size_t have = sizeof(outBuffer) - strm.avail_out;
			size_t chunkStart = uncompressedOffset;
			size_t chunkEnd = uncompressedOffset + have;

			if ( chunkEnd > start && chunkStart < start + len ) {
				size_t copyStart = (chunkStart < start) ? (start - chunkStart) : 0;
				size_t copyLen = std::min(have - copyStart, (start + len) - (chunkStart + copyStart));
				buffer.insert(buffer.end(), outBuffer + copyStart, outBuffer + copyStart + copyLen);
			}

			uncompressedOffset += have;

			if ( uncompressedOffset >= start + len ) return false;

		} while (strm.avail_out == 0);

		return true;
	});

	inflateEnd(&strm);

	if ( !success && uncompressedOffset >= start + len ) return true;

	return success;
}

bool ext::zlib::decompressFromFile( uf::stl::vector<uint8_t>& buffer, const uf::stl::string& filename, const uf::stl::vector<pod::Range>& ranges, int flag ) {
	if ( ranges.empty() ) return false;

	uf::stl::vector<pod::Range> sortedRanges = ranges;
	std::sort( sortedRanges.begin(), sortedRanges.end(), [](const pod::Range& a, const pod::Range& b) { return a.start < b.start; } );

	size_t fileSize = uf::vfs::size(filename);
	if (fileSize == 0) return false;

	z_stream strm{};
	if (inflateInit2(&strm, flag) != Z_OK) return false;

	size_t uncompressedOffset = 0;
	size_t currentRangeIdx = 0;
	uint8_t outBuffer[ext::zlib::bufferSize];

	bool success = uf::vfs::stream(filename, ext::zlib::bufferSize, [&](const uint8_t* data, size_t size) -> bool {
		strm.avail_in = (uInt)size;
		strm.next_in = (Bytef*)data;

		do {
			strm.avail_out = sizeof(outBuffer);
			strm.next_out = outBuffer;
			int ret = inflate(&strm, Z_NO_FLUSH);
			if ( ret < 0 && ret != Z_BUF_ERROR ) return false;

			size_t have = sizeof(outBuffer) - strm.avail_out;
			size_t chunkStart = uncompressedOffset;
			size_t chunkEnd = uncompressedOffset + have;

			for (size_t i = currentRangeIdx; i < sortedRanges.size(); ++i) {
				const auto& r = sortedRanges[i];
				if (chunkEnd > r.start && chunkStart < r.start + r.len) {
					size_t copyStart = (chunkStart < r.start) ? (r.start - chunkStart) : 0;
					size_t copyLen = std::min(have - copyStart, (r.start + r.len) - (chunkStart + copyStart));
					buffer.insert(buffer.end(), outBuffer + copyStart, outBuffer + copyStart + copyLen);
				}
				if (chunkEnd >= r.start + r.len) {
					currentRangeIdx = i + 1;
				}
			}
			uncompressedOffset += have;
			if ( currentRangeIdx >= sortedRanges.size() ) return false;

		} while (strm.avail_out == 0);

		return true;
	});

	inflateEnd(&strm);
	return success;
}

bool ext::zlib::decompressFromMemory( uf::stl::vector<uint8_t>& dst, const void* src, size_t size, size_t usize, int flag ) {
	if (size == 0) return false;

	z_stream strm{};
	if (inflateInit2(&strm, flag) != Z_OK) return false;

	strm.avail_in = (uInt)size;
	strm.next_in = (Bytef*)src;

	dst.resize(usize);
	strm.avail_out = (uInt)usize;
	strm.next_out = dst.data();

	int ret = inflate(&strm, Z_FINISH);
	inflateEnd(&strm);

	if ( ret != Z_STREAM_END && ret != Z_OK ) {
		UF_MSG_ERROR("Decompress encountered error: {}", ret);
	}
	return true;
}

bool ext::zlib::decompressScatter( const uf::stl::string& filename, uf::stl::vector<pod::ScatterRequest>& requests, int flag ) {
	if ( requests.empty() ) return true;

	std::sort(requests.begin(), requests.end(), [](const pod::ScatterRequest& a, const pod::ScatterRequest& b) {
		return a.start < b.start;
	});

	z_stream strm{};
	if ( inflateInit2(&strm, flag) != Z_OK ) return false;

	size_t uncompressedOffset = 0;
	size_t currentReqIdx = 0;
	uint8_t outBuffer[ext::zlib::bufferSize];

	bool success = uf::vfs::stream(filename, ext::zlib::bufferSize, [&](const uint8_t* data, size_t size) -> bool {
		strm.avail_in = (uInt)size;
		strm.next_in = (Bytef*)data;

		do {
			strm.avail_out = sizeof(outBuffer);
			strm.next_out = outBuffer;
			int ret = inflate(&strm, Z_NO_FLUSH);
			if ( ret < 0 && ret != Z_BUF_ERROR ) return false;

			size_t have = sizeof(outBuffer) - strm.avail_out;
			size_t chunkStart = uncompressedOffset;
			size_t chunkEnd = uncompressedOffset + have;

			for ( size_t i = currentReqIdx; i < requests.size(); ++i ) {
				auto& req = requests[i];

				if ( req.start >= chunkEnd ) break;

				if ( chunkEnd > req.start && chunkStart < req.start + req.len ) {
					size_t copyStart = (chunkStart < req.start) ? (req.start - chunkStart) : 0;
					size_t copyLen = std::min(have - copyStart, (req.start + req.len) - (chunkStart + copyStart));
					size_t destOffset = (chunkStart + copyStart) - req.start;

					std::memcpy(req.dest + destOffset, outBuffer + copyStart, copyLen);
				}
			}

			while ( currentReqIdx < requests.size() && chunkEnd >= requests[currentReqIdx].start + requests[currentReqIdx].len ) {
				currentReqIdx++;
			}

			uncompressedOffset += have;

			if ( currentReqIdx >= requests.size() ) return false;

		} while (strm.avail_out == 0);

		return true;
	});

	inflateEnd(&strm);
	return success;
}

#if !UF_ENV_DREAMCAST
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
#endif

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

	struct ZipFileStream {
		const uint8_t* data = nullptr;
		size_t size = 0;
		size_t cursor = 0;
		uf::stl::vector<uint8_t> decompressedBuffer;
	};

	size_t zip_file_read(void* handle, void* buffer, size_t bytes) {
		ZipFileStream* stream = (ZipFileStream*)handle;
		size_t bytesLeft = stream->size - stream->cursor;
		size_t toRead = (bytes > bytesLeft) ? bytesLeft : bytes;

		if (toRead > 0) {
			std::memcpy(buffer, stream->data + stream->cursor, toRead);
			stream->cursor += toRead;
		}
		return toRead;
	}

	bool zip_file_seek(void* handle, long offset, int origin) {
		ZipFileStream* stream = (ZipFileStream*)handle;
		long target = 0;

		if (origin == SEEK_SET) target = offset;
		else if (origin == SEEK_CUR) target = (long)stream->cursor + offset;
		else if (origin == SEEK_END) target = (long)stream->size + offset;

		if (target < 0) target = 0;
		if ((size_t)target > stream->size) target = stream->size;

		stream->cursor = (size_t)target;
		return true;
	}

	size_t zip_file_tell(void* handle) {
		return ((ZipFileStream*)handle)->cursor;
	}

	void zip_file_close(void* handle) {
		ZipFileStream* stream = (ZipFileStream*)handle;
		delete stream;
	}

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
			return ext::zlib::decompressFromMemory(buffer, fileData, entry.compressedSize, entry.uncompressedSize, -15);
		}
		// ID for lz4
		return false;
	};
	uf::stl::vector<uf::stl::string> vfs_list( pod::Mount& mount, const uf::stl::string& dir, const uf::stl::string& extension = "", bool recursive = false ) {
		uf::stl::vector<uf::stl::string> files;
		auto& state = uf::pointeredUserdata::get<ZipMountState>( mount.userdata );

		for ( const auto& [filepath, entry] : state.entries ) {
			if ( !dir.empty() && !filepath.starts_with(dir) ) continue;

			if ( !recursive ) {
				uf::stl::string remainder = filepath.substr(dir.length());
				if ( remainder.find('/') != uf::stl::string::npos ) continue;
			}

			if ( !extension.empty() && !filepath.ends_with(extension) ) continue;

			files.emplace_back(filepath);
		}

		return files;
	}

	pod::File vfs_open(pod::Mount& mount, const uf::stl::string& file) {
		auto& state = uf::pointeredUserdata::get<ZipMountState>(mount.userdata);
		auto it = state.entries.find(file);
		if (it == state.entries.end()) return pod::File{};

		const auto& entry = it->second;
		const uint8_t* fileData = state.buffer.data() + entry.offset;

		ZipFileStream* stream = new ZipFileStream();
		stream->size = entry.uncompressedSize;
		stream->cursor = 0;

		if (entry.compressionMethod == 0) {
			stream->data = fileData;
		}
		else if (entry.compressionMethod == 8) {
			if (!ext::zlib::decompressFromMemory(stream->decompressedBuffer, fileData, entry.compressedSize, entry.uncompressedSize, -15)) {
				delete stream;
				return pod::File{};
			}
			stream->data = stream->decompressedBuffer.data();
		}
		else {
			delete stream;
			return pod::File{};
		}

		return pod::File{
			.handle = stream,
			.read = zip_file_read,
			.seek = zip_file_seek,
			.tell = zip_file_tell,
			.close = zip_file_close
		};
	}
}

pod::Mount ext::zlib::createZipMount( const uf::stl::string& uri, uf::stl::vector<uint8_t>& buffer, int priority ) {
	return ext::zlib::createZipMount(uri, std::move(buffer), priority);
}

pod::Mount ext::zlib::createZipMount( const uf::stl::string& uri, uf::stl::vector<uint8_t>&& buffer, int priority ) {
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
	mount.list = ::vfs_list;
	mount.open = ::vfs_open;
	
	auto& state = uf::pointeredUserdata::get<ZipMountState>( mount.userdata );
	state.buffer = buffer;
	ext::zlib::directory( state.buffer, state.entries );

	if ( mount.path.empty() ) mount.path = FMT_FORMAT( "{}/{}", uri, (void*) state.buffer.data() );
//	for ( auto& [ k, v ] : state.entries ) UF_MSG_DEBUG("{} => {}/{}", mount.path, uri, k);

	return mount;
}

pod::Mount ext::zlib::createZipMount( const uf::stl::string& uri, const uf::stl::string& filename, int priority ) {
	uf::stl::vector<uint8_t> buffer;
	if ( !uf::io::exists( filename ) ) {
		UF_MSG_ERROR("Does not exist: {}", filename);
	}
	if ( !uf::io::readAsBuffer(buffer, filename) ) {
		UF_MSG_ERROR("Failed to load ZIP mount from disk: {}", filename);
		return pod::Mount{};
	}

	return ext::zlib::createZipMount( FMT_FORMAT("{}/{}", uri, filename), std::move(buffer), priority );
}

#endif