#include <uf/config.h>
#if UF_USE_VORBIS

#if UF_USE_OPENAL
#include <uf/ext/openal/openal.h>
#endif

#include <uf/ext/audio/vorbis.h>
#include <uf/utils/memory/pool.h>
#include <uf/utils/io/vfs.h>
#include <iostream>
#include <vector>
#include <cstring>

#if UF_USE_TREMOR
	#define OV_READ( file, buffer, len, endian, _x, _y, bitStream ) ov_read( file, buffer, len, bitStream )
#else
	#define OV_READ( file, buffer, len, endian, _x, _y, bitStream ) ov_read( file, buffer, len, endian, _x, _y, bitStream )
#endif

namespace {
	constexpr int endian = 0; // 0 = little endian

	struct VorbisVfsContext {
		uf::stl::string filename;
		size_t currentOffset;
		size_t totalSize;
	};

	namespace funs {
		size_t read(void* destination, size_t size, size_t nmemb, void* userdata) {
			VorbisVfsContext* ctx = (VorbisVfsContext*) userdata;
			size_t bytesToRead = size * nmemb;
			size_t bytesLeft = ctx->totalSize - ctx->currentOffset;
			if (bytesToRead > bytesLeft) bytesToRead = bytesLeft;

			if (bytesToRead > 0) {
				uf::stl::vector<uint8_t> tempBuffer;
				if (uf::vfs::readRange(ctx->filename, ctx->currentOffset, bytesToRead, tempBuffer)) {
					std::memcpy(destination, tempBuffer.data(), tempBuffer.size());
					ctx->currentOffset += tempBuffer.size();
					return tempBuffer.size() / size;
				}
			}
			return 0;
		}

		int seek( void* userdata, ogg_int64_t to, int type ) {
			VorbisVfsContext* ctx = (VorbisVfsContext*) userdata;
			int64_t targetOffset = 0;

			switch ( type ) {
				case SEEK_CUR: targetOffset = (int64_t)ctx->currentOffset + to; break;
				case SEEK_END: targetOffset = (int64_t)ctx->totalSize + to; break;
				case SEEK_SET: targetOffset = to; break;
				default: return -1;
			}

			if (targetOffset < 0) targetOffset = 0;
			if ((size_t)targetOffset > ctx->totalSize) targetOffset = ctx->totalSize;

			ctx->currentOffset = (size_t)targetOffset;
			return 0;
		}

		int close(void* userdata) {
			VorbisVfsContext* ctx = (VorbisVfsContext*) userdata;
			if (ctx) {
				delete ctx;
			}
			return 0;
		}

		long tell(void* userdata) {
			VorbisVfsContext* ctx = (VorbisVfsContext*) userdata;
			return (long)ctx->currentOffset;
		}
	}

	inline bool format(uf::Audio::Metadata& metadata, int channels, int bitDepth) {
		if (channels == 1 && bitDepth == 8) metadata.info.format = AL_FORMAT_MONO8;
		else if (channels == 1 && bitDepth == 16) metadata.info.format = AL_FORMAT_MONO16;
		else if (channels == 2 && bitDepth == 8) metadata.info.format = AL_FORMAT_STEREO8;
		else if (channels == 2 && bitDepth == 16) metadata.info.format = AL_FORMAT_STEREO16;
		else {
			UF_MSG_ERROR("Vorbis: unrecognized OGG format: {} channels, {} bps", channels, bitDepth);
			return false;
		}
		return true;
	}
}

void ext::vorbis::open(uf::Audio::Metadata& metadata) {
	if ( !metadata.stream.context ) {
		VorbisVfsContext* ctx = new VorbisVfsContext();
		ctx->filename = metadata.filename;
		ctx->currentOffset = 0;
		ctx->totalSize = uf::vfs::size(metadata.filename);

		if ( ctx->totalSize == 0 ) {
			UF_MSG_ERROR("Vorbis: failed to open file: {}", metadata.filename);
			delete ctx;
			return;
		}
		metadata.stream.context = (void*) ctx;
	}
	if ( !metadata.stream.handle ) metadata.stream.handle = (void*) new OggVorbis_File;

	VorbisVfsContext* ctx = (VorbisVfsContext*) metadata.stream.context;
	OggVorbis_File* vorbisFile = (OggVorbis_File*) metadata.stream.handle;

	metadata.info.size = ctx->totalSize;
	metadata.stream.consumed = 0;

	ov_callbacks callbacks;
	callbacks.read_func = funs::read;
	callbacks.seek_func = funs::seek;
	callbacks.close_func = funs::close;
	callbacks.tell_func = funs::tell;

	int error = ov_open_callbacks((void*) ctx, vorbisFile, NULL,  metadata.settings.streamed ? -1 : 0, callbacks);
	if (error < 0) {
		UF_MSG_ERROR("Vorbis: failed call to ov_open_callbacks: {}", metadata.filename);
		delete ctx;
		metadata.stream.context = nullptr;
		return;
	}

	vorbis_info* info = ov_info(vorbisFile, -1);
	metadata.info.channels = info->channels;
	metadata.info.bitDepth = 16;
	metadata.info.frequency = info->rate;
	metadata.info.duration = ov_time_total(vorbisFile, -1);

	metadata.info.loop.has = false;
	metadata.info.loop.start = 0;
	metadata.info.loop.end = (uint32_t)ov_pcm_total(vorbisFile, -1); // Default to full file

	vorbis_comment* vc = ov_comment(vorbisFile, -1);
	if ( vc != nullptr ) {
		for ( auto i = 0; i < vc->comments; ++i ) {
			uf::stl::string comment(vc->user_comments[i], vc->comment_lengths[i]);
			uf::stl::string upperComment = uf::string::uppercase( comment );

			// to-do: handle exceptions without exceptions
			if ( upperComment.starts_with("LOOPSTART=" )) {
				metadata.info.loop.start = std::stoul(comment.substr(10));
				metadata.info.loop.has = true;
			} else if ( upperComment.starts_with("LOOPLENGTH=" )) {
				uint32_t length = std::stoul(comment.substr(11));
				metadata.info.loop.end = metadata.info.loop.start + length;
			} else if ( upperComment.starts_with("LOOPEND=") ) {
				metadata.info.loop.end = std::stoul(comment.substr(8));
			}
		}
	}

	if ( !format(metadata, info->channels, 16) ) {
		ov_clear(vorbisFile);
		metadata.stream.context = nullptr;
		return;
	}

	if ( metadata.settings.streamed ) ext::vorbis::stream(metadata); else ext::vorbis::load(metadata);
}

void ext::vorbis::load( uf::Audio::Metadata& metadata ) {
	if ( metadata.settings.streamed ) return ext::vorbis::stream(metadata);

	OggVorbis_File* vorbisFile = (OggVorbis_File*) metadata.stream.handle;

	uf::stl::vector<uint8_t> bytes;
	char buffer[uf::audio::bufferSize];
	int bitStream = 0;
	int read = 0;
	do {
		read = OV_READ(vorbisFile, buffer, uf::audio::bufferSize, endian, 2, 1, &bitStream);
		if ( read > 0 ) bytes.insert(bytes.end(), buffer, buffer + read);
	} while ( read > 0 );

	metadata.al.buffer.buffer(metadata.info.format, bytes.data(), (ALsizei) bytes.size(), metadata.info.frequency);
	if ( metadata.info.loop.has ) {
		ALint loopPoints[2] = { (ALint) metadata.info.loop.start, (ALint) metadata.info.loop.end };
		alBufferiv(metadata.al.buffer.getIndex(), 0x2015 /* AL_LOOP_POINTS_SOFT */, loopPoints);
	}
	metadata.al.source.set(AL_BUFFER, (ALint) metadata.al.buffer.getIndex());

	ov_clear(vorbisFile);
	metadata.stream.context = nullptr;
}

void ext::vorbis::stream(uf::Audio::Metadata& metadata) {
	if ( !metadata.settings.streamed ) return ext::vorbis::load(metadata);

	OggVorbis_File* vorbisFile = (OggVorbis_File*) metadata.stream.handle;

	// fill and queue initial buffers
	char buffer[uf::audio::bufferSize];
	uint8_t queuedBuffers = 0;
	int bitStream = 0;
	for (; queuedBuffers < metadata.settings.buffers; ++queuedBuffers) {
		int totalRead = 0;
		while (totalRead < uf::audio::bufferSize) {
			int result = OV_READ(vorbisFile, buffer + totalRead, uf::audio::bufferSize - totalRead, endian, 2, 1, &bitStream);
			if (result <= 0) {
				if (result == 0 && metadata.settings.loop) {
					uint32_t seekTarget = metadata.info.loop.has ? metadata.info.loop.start : 0;
					if ( ov_pcm_seek(vorbisFile, seekTarget) != 0 ) {
						UF_MSG_ERROR("Vorbis: failed to loop (seek to start): {}", metadata.filename);
						break;
					}
					continue;
				}
				if (result == OV_HOLE) UF_MSG_ERROR("Vorbis: OV_HOLE in buffer read: {}", metadata.filename);
				if (result == OV_EBADLINK) UF_MSG_ERROR("Vorbis: OV_EBADLINK in buffer read: {}", metadata.filename);
				if (result == OV_EINVAL) UF_MSG_ERROR("Vorbis: OV_EINVAL in buffer read: {}", metadata.filename);
				break;
			}
			totalRead += result;
		}
		if (totalRead == 0) {
			UF_MSG_WARNING("Vorbis: consumed file stream before buffers are filled: {} {}", (int)queuedBuffers, metadata.filename);
			break;
		}
		AL_CHECK_RESULT(alBufferData(metadata.al.buffer.getIndex(queuedBuffers), metadata.info.format, buffer, totalRead, metadata.info.frequency));
	}
	AL_CHECK_RESULT(alSourceQueueBuffers(metadata.al.source.getIndex(), queuedBuffers, &metadata.al.buffer.getIndex()));

	if (queuedBuffers >= metadata.settings.buffers) {
		metadata.settings.loopMode = 1;
		metadata.al.source.set(AL_LOOPING, AL_FALSE);
	}
}

void ext::vorbis::update(uf::Audio::Metadata& metadata) {
	if (!metadata.settings.streamed) return;
	if (metadata.settings.loopMode == 1)
		metadata.al.source.set(AL_LOOPING, AL_FALSE);

	ALint state;
	metadata.al.source.get(AL_SOURCE_STATE, state);
	if (state != AL_PLAYING) {
		VorbisVfsContext* ctx = (VorbisVfsContext*) metadata.stream.context;
		if (!metadata.settings.loop && ctx && ctx->currentOffset >= ctx->totalSize) {
			return;
		}
		metadata.al.source.play();
	}

	ALint processed = 0;
	metadata.al.source.get(AL_BUFFERS_PROCESSED, processed);
	if (processed <= 0) return;

	OggVorbis_File* vorbisFile = (OggVorbis_File*) metadata.stream.handle;
	int bitStream = metadata.stream.bitStream;
	ALuint index;
	char buffer[uf::audio::bufferSize];

	while (processed--) {
		memset(buffer, 0, uf::audio::bufferSize);
		AL_CHECK_RESULT(alSourceUnqueueBuffers(metadata.al.source.getIndex(), 1, &index));

		int totalRead = 0;
		while (totalRead < uf::audio::bufferSize) {
			int result = OV_READ(vorbisFile, buffer + totalRead, uf::audio::bufferSize - totalRead, endian, 2, 1, &bitStream);
			if (result <= 0) {
				if (result == 0 && metadata.settings.loop) {
					uint32_t seekTarget = metadata.info.loop.has ? metadata.info.loop.start : 0;

					if ( ov_pcm_seek(vorbisFile, seekTarget) != 0 ) {
						UF_MSG_ERROR("Vorbis: failed to loop (seek to start): {}", metadata.filename);
						break;
					}
					continue;
				}
				if (result == OV_HOLE) UF_MSG_ERROR("Vorbis: OV_HOLE in buffer read: {}", metadata.filename);
				if (result == OV_EBADLINK) UF_MSG_ERROR("Vorbis: OV_EBADLINK in buffer read: {}", metadata.filename);
				if (result == OV_EINVAL) UF_MSG_ERROR("Vorbis: OV_EINVAL in buffer read: {}", metadata.filename);
				break;
			}
			totalRead += result;
		}

		if (totalRead > 0) {
			AL_CHECK_RESULT(alBufferData(index, metadata.info.format, buffer, totalRead, metadata.info.frequency));
			AL_CHECK_RESULT(alSourceQueueBuffers(metadata.al.source.getIndex(), 1, &index));
		}
		if (metadata.settings.loop && totalRead < uf::audio::bufferSize) {
			UF_MSG_ERROR("Vorbis: missing data: {}", metadata.filename);
		}
	}

	if (metadata.settings.loopMode == 1)
		metadata.al.source.set(AL_LOOPING, AL_TRUE);
}

void ext::vorbis::close(uf::Audio::Metadata& metadata) {
	if (metadata.stream.handle) {
		OggVorbis_File* file = (OggVorbis_File*) metadata.stream.handle;
		ov_clear(file);
		delete file;
		metadata.stream.handle = NULL;
		metadata.stream.context = NULL;
	}
}

#endif