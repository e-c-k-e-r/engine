#include <uf/ext/lz4/lz4.h>
#include <uf/utils/io/file.h>
#include <uf/utils/io/vfs.h>
#include <lz4frame.h>
#include <algorithm>
#include <cstring>

size_t ext::lz4::bufferSize = 16384;

bool ext::lz4::decompressFromFile( uf::stl::vector<uint8_t>& buffer, const uf::stl::string& filename ) {
	LZ4F_dctx* dctx;
	if ( LZ4F_isError( LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION)) ) return false;

	uint8_t outBuffer[ext::lz4::bufferSize];
	bool success = uf::vfs::stream(filename, ext::lz4::bufferSize, [&](const uint8_t* data, size_t size) -> bool {
		size_t srcSize = size;
		const uint8_t* srcPtr = data;

		while (srcSize > 0) {
			size_t dstSize = sizeof(outBuffer);
			size_t consumed = srcSize;
			size_t ret = LZ4F_decompress(dctx, outBuffer, &dstSize, srcPtr, &consumed, nullptr);

			if (LZ4F_isError(ret)) return false;

			buffer.insert(buffer.end(), outBuffer, outBuffer + dstSize);
			srcPtr += consumed;
			srcSize -= consumed;
		}
		return true;
	});

	LZ4F_freeDecompressionContext(dctx);
	return success;
}

bool ext::lz4::decompressFromFile( uf::stl::vector<uint8_t>& buffer, const uf::stl::string& filename, size_t start, size_t len ) {
	size_t fileSize = uf::vfs::size(filename);
	if ( fileSize == 0 ) return false;

	LZ4F_dctx* dctx;
	if ( LZ4F_isError( LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION)) ) return false;

	size_t uncompressedOffset = 0;
	uint8_t outBuffer[ext::lz4::bufferSize];

	bool success = uf::vfs::stream(filename, ext::lz4::bufferSize, [&](const uint8_t* data, size_t size) -> bool {
		size_t srcSize = size;
		const uint8_t* srcPtr = data;

		while ( srcSize > 0 ) {
			size_t dstSize = sizeof(outBuffer);
			size_t consumed = srcSize;
			size_t ret = LZ4F_decompress(dctx, outBuffer, &dstSize, srcPtr, &consumed, nullptr);

			if ( LZ4F_isError(ret) ) return false;

			if ( dstSize > 0 ) {
				size_t chunkStart = uncompressedOffset;
				size_t chunkEnd = uncompressedOffset + dstSize;

				if ( chunkEnd > start && chunkStart < start + len ) {
					size_t copyStart = (chunkStart < start) ? (start - chunkStart) : 0;
					size_t copyLen = std::min(dstSize - copyStart, (start + len) - (chunkStart + copyStart));
					buffer.insert(buffer.end(), outBuffer + copyStart, outBuffer + copyStart + copyLen);
				}

				uncompressedOffset += dstSize;

				if ( uncompressedOffset >= start + len ) return false;
			}

			srcPtr += consumed;
			srcSize -= consumed;
		}
		return true;
	});

	LZ4F_freeDecompressionContext(dctx);

	if ( !success && uncompressedOffset >= start + len ) return true;

	return success;
}

bool ext::lz4::decompressFromFile( uf::stl::vector<uint8_t>& buffer, const uf::stl::string& filename, const uf::stl::vector<pod::Range>& ranges ) {
	if ( ranges.empty() ) return false;

	uf::stl::vector<pod::Range> sortedRanges = ranges;
	std::sort( sortedRanges.begin(), sortedRanges.end(), [](const pod::Range& a, const pod::Range& b) { return a.start < b.start; } );

	size_t fileSize = uf::vfs::size(filename);
	if ( fileSize == 0 ) return false;

	LZ4F_dctx* dctx;
	if ( LZ4F_isError( LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION)) ) return false;

	size_t uncompressedOffset = 0;
	size_t currentRangeIdx = 0;
	uint8_t outBuffer[ext::lz4::bufferSize];

	bool success = uf::vfs::stream(filename, ext::lz4::bufferSize, [&](const uint8_t* data, size_t size) -> bool {
		size_t srcSize = size;
		const uint8_t* srcPtr = data;

		while ( srcSize > 0 ) {
			size_t dstSize = sizeof(outBuffer);
			size_t consumed = srcSize;
			size_t ret = LZ4F_decompress(dctx, outBuffer, &dstSize, srcPtr, &consumed, nullptr);

			if ( LZ4F_isError(ret) ) return false;

			if ( dstSize > 0 ) {
				size_t chunkStart = uncompressedOffset;
				size_t chunkEnd = uncompressedOffset + dstSize;

				for ( size_t i = currentRangeIdx; i < sortedRanges.size(); ++i ) {
					const auto& r = sortedRanges[i];
					if ( chunkEnd > r.start && chunkStart < r.start + r.len ) {
						size_t copyStart = (chunkStart < r.start) ? (r.start - chunkStart) : 0;
						size_t copyLen = std::min(dstSize - copyStart, (r.start + r.len) - (chunkStart + copyStart));
						buffer.insert(buffer.end(), outBuffer + copyStart, outBuffer + copyStart + copyLen);
					}
					if ( chunkEnd >= r.start + r.len ) {
						currentRangeIdx = i + 1;
					}
				}

				uncompressedOffset += dstSize;

				if ( currentRangeIdx >= sortedRanges.size() ) return false;
			}

			srcPtr += consumed;
			srcSize -= consumed;
		}
		return true;
	});

	LZ4F_freeDecompressionContext(dctx);

	if ( !success && currentRangeIdx >= sortedRanges.size() ) return true;

	return success;
}

bool ext::lz4::decompressScatter( const uf::stl::string& filename, uf::stl::vector<pod::ScatterRequest>& requests ) {
	if (requests.empty()) return true;

	std::sort(requests.begin(), requests.end(), [](const pod::ScatterRequest& a, const pod::ScatterRequest& b) {
		return a.start < b.start;
	});

	LZ4F_dctx* dctx;
	if ( LZ4F_isError( LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION)) ) return false;

	size_t uncompressedOffset = 0;
	size_t currentReqIdx = 0;
	uint8_t outBuffer[ext::lz4::bufferSize];

	bool success = uf::vfs::stream(filename, ext::lz4::bufferSize, [&](const uint8_t* data, size_t size) -> bool {
		size_t srcSize = size;
		const uint8_t* srcPtr = data;

		while ( srcSize > 0 ) {
			size_t dstSize = sizeof(outBuffer);
			size_t consumed = srcSize;
			size_t ret = LZ4F_decompress(dctx, outBuffer, &dstSize, srcPtr, &consumed, nullptr);

			if ( LZ4F_isError(ret) ) return false;

			if ( dstSize > 0 ) {
				size_t chunkStart = uncompressedOffset;
				size_t chunkEnd = uncompressedOffset + dstSize;

				for ( size_t i = currentReqIdx; i < requests.size(); ++i ) {
					auto& req = requests[i];
					if ( chunkEnd > req.start && chunkStart < req.start + req.len ) {
						size_t copyStart = (chunkStart < req.start) ? (req.start - chunkStart) : 0;
						size_t copyLen = std::min(dstSize - copyStart, (req.start + req.len)  - (chunkStart + copyStart));

						size_t destOffset = (chunkStart + copyStart) - req.start;
						std::memcpy(req.dest + destOffset, outBuffer + copyStart, copyLen);
					}
					if ( chunkEnd >= req.start + req.len && i == currentReqIdx ) {
						currentReqIdx++;
					}
				}
				uncompressedOffset += dstSize;
				if ( currentReqIdx >= requests.size() ) return false;
			}

			srcPtr += consumed;
			srcSize -= consumed;
		}
		return true;
	});

	LZ4F_freeDecompressionContext(dctx);
	return success;
}

bool ext::lz4::decompressFromMemory( uf::stl::vector<uint8_t>& dst, const void* src, size_t compressedSize, size_t uncompressedSize ) {
	LZ4F_dctx* dctx;
	if ( LZ4F_isError( LZ4F_createDecompressionContext(&dctx, LZ4F_VERSION)) ) return false;

	dst.resize(uncompressedSize);

	size_t dstSize = uncompressedSize;
	size_t srcSize = compressedSize;
	size_t ret = LZ4F_decompress(dctx, dst.data(), &dstSize, src, &srcSize, nullptr);

	LZ4F_freeDecompressionContext(dctx);

	if ( LZ4F_isError(ret) || dstSize != uncompressedSize ) {
		dst.clear();
		return false;
	}
	return true;
}

size_t ext::lz4::compressToFile( const uf::stl::string& filename, const void* data, size_t size ) {
	LZ4F_preferences_t prefs = {};
	size_t bound = LZ4F_compressFrameBound(size, &prefs);

	uf::stl::vector<uint8_t> compressedData(bound);
	size_t compressedSize = LZ4F_compressFrame(compressedData.data(), bound, data, size, &prefs);

	if ( LZ4F_isError(compressedSize) ) return 0;

	return uf::vfs::write( filename, compressedData.data(), compressedSize );
}