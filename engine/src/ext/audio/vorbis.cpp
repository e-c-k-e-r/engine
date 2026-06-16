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

	inline bool format( pod::AudioClip& clip, int channels, int bitDepth) {
		if (channels == 1 && bitDepth == 8) clip.info.format = AL_FORMAT_MONO8;
		else if (channels == 1 && bitDepth == 16) clip.info.format = AL_FORMAT_MONO16;
		else if (channels == 2 && bitDepth == 8) clip.info.format = AL_FORMAT_STEREO8;
		else if (channels == 2 && bitDepth == 16) clip.info.format = AL_FORMAT_STEREO16;
		else {
			UF_MSG_ERROR("Vorbis: unrecognized OGG format: {} channels, {} bps", channels, bitDepth);
			return false;
		}
		return true;
	}
}

void ext::vorbis::open( pod::AudioSource& source ) {
	pod::AudioClip* clip = source.clip;
	if ( !clip || !clip->streamed ) return;

	source.streamBuffers.initialize( source.settings.buffers );
	source.streamState.consumed = 0;
	source.streamState.bitStream = 0;

	VorbisVfsContext* ctx = new VorbisVfsContext();
	ctx->filename = clip->filename;
	ctx->currentOffset = 0;
	ctx->totalSize = clip->info.size;

	OggVorbis_File* vorbisFile = new OggVorbis_File;
	ov_callbacks callbacks = { funs::read, funs::seek, funs::close, funs::tell };

	if ( ov_open_callbacks((void*) ctx, vorbisFile, NULL, -1, callbacks) < 0 ) {
		delete ctx; delete vorbisFile;
		return;
	}

	source.streamState.context = (void*) ctx;
	source.streamState.handle = (void*) vorbisFile;

	char buffer[uf::audio::bufferSize];
	uint8_t queuedBuffers = 0;
	for ( ; queuedBuffers < source.settings.buffers; ++queuedBuffers ) {
		int totalRead = 0;
		while (totalRead < uf::audio::bufferSize) {
			int result = OV_READ(vorbisFile, buffer + totalRead, uf::audio::bufferSize - totalRead, endian, 2, 1, &source.streamState.bitStream);
			if ( result <= 0 ) {
				if ( result == 0 && source.settings.loop ) {
					uint32_t seekTarget = clip->info.loop.has ? clip->info.loop.start : 0;
					ov_pcm_seek(vorbisFile, seekTarget);
					continue;
				}
				break;
			}
			totalRead += result;
		}
		if ( totalRead == 0 ) break;
		AL_CHECK_RESULT(alBufferData(source.streamBuffers.getIndex(queuedBuffers), clip->info.format, buffer, totalRead, clip->info.frequency));
	}
	AL_CHECK_RESULT(alSourceQueueBuffers(source.alSource.getIndex(), queuedBuffers, &source.streamBuffers.getIndex(0)));

	if ( queuedBuffers >= source.settings.buffers ) {
		source.settings.loopMode = 1;
		source.alSource.set(AL_LOOPING, AL_FALSE);
	}
}

void ext::vorbis::load( pod::AudioClip& clip ) {
	VorbisVfsContext* ctx = new VorbisVfsContext();
	ctx->filename = clip.filename;
	ctx->currentOffset = 0;
	ctx->totalSize = uf::vfs::size(clip.filename);

	if ( ctx->totalSize == 0 ) {
		UF_MSG_ERROR("Vorbis: failed to open file: {}", clip.filename);
		delete ctx;
		return;
	}

	OggVorbis_File* vorbisFile = new OggVorbis_File;
	ov_callbacks callbacks = { funs::read, funs::seek, funs::close, funs::tell };

	if ( ov_open_callbacks((void*) ctx, vorbisFile, NULL, clip.streamed ? -1 : 0, callbacks) < 0 ) {
		UF_MSG_ERROR("Vorbis: failed call to ov_open_callbacks: {}", clip.filename);
		delete ctx; delete vorbisFile;
		return;
	}

	vorbis_info* info = ov_info(vorbisFile, -1);
	clip.info.channels = info->channels;
	clip.info.bitDepth = 16;
	clip.info.frequency = info->rate;
	clip.info.duration = ov_time_total(vorbisFile, -1);
	clip.info.size = ctx->totalSize;

	clip.info.loop.has = false;
	clip.info.loop.start = 0;
	clip.info.loop.end = (uint32_t)ov_pcm_total(vorbisFile, -1);

	vorbis_comment* vc = ov_comment(vorbisFile, -1);
	if ( vc != nullptr ) {
		for ( auto i = 0; i < vc->comments; ++i ) {
			uf::stl::string comment(vc->user_comments[i], vc->comment_lengths[i]);
			uf::stl::string upperComment = uf::string::uppercase( comment );
			if ( upperComment.starts_with("LOOPSTART=" )) {
				clip.info.loop.start = std::stoul(comment.substr(10));
				clip.info.loop.has = true;
			} else if ( upperComment.starts_with("LOOPLENGTH=" )) {
				clip.info.loop.end = clip.info.loop.start + std::stoul(comment.substr(11));
			} else if ( upperComment.starts_with("LOOPEND=") ) {
				clip.info.loop.end = std::stoul(comment.substr(8));
			}
		}
	}

	if ( !format(clip, info->channels, 16) ) {
		ov_clear(vorbisFile); delete vorbisFile;
		return;
	}

	if ( !clip.streamed ) {
		uf::stl::vector<uint8_t> bytes;
		char buffer[uf::audio::bufferSize];
		int bitStream = 0;
		int readCount = 0;
		do {
			readCount = OV_READ(vorbisFile, buffer, uf::audio::bufferSize, endian, 2, 1, &bitStream);
			if ( readCount > 0 ) bytes.insert(bytes.end(), buffer, buffer + readCount);
		} while ( readCount > 0 );

		clip.alBuffer.buffer(clip.info.format, bytes.data(), (ALsizei) bytes.size(), clip.info.frequency);
		if ( clip.info.loop.has ) {
			ALint loopPoints[2] = { (ALint) clip.info.loop.start, (ALint) clip.info.loop.end };
			alBufferiv(clip.alBuffer.getIndex(0), 0x2015 /* AL_LOOP_POINTS_SOFT */, loopPoints);
		}
	}

	ov_clear(vorbisFile);
	delete vorbisFile;
}

void ext::vorbis::update( pod::AudioSource& source ) {
	pod::AudioClip* clip = source.clip;
	if ( !clip || !clip->streamed || !source.streamState.handle ) return;

	if ( source.settings.loopMode == 1 ) source.alSource.set(AL_LOOPING, AL_FALSE);

	OggVorbis_File* vorbisFile = (OggVorbis_File*) source.streamState.handle;
	VorbisVfsContext* ctx = (VorbisVfsContext*) source.streamState.context;

	ALint state;
	source.alSource.get(AL_SOURCE_STATE, state);
	if ( state != AL_PLAYING ) {
		if ( !source.settings.loop && ctx && ctx->currentOffset >= ctx->totalSize ) return;
		source.alSource.play();
	}

	ALint processed = 0;
	source.alSource.get(AL_BUFFERS_PROCESSED, processed);
	if ( processed <= 0 ) return;

	ALuint index;
	char buffer[uf::audio::bufferSize];

	while ( processed-- ) {
		memset(buffer, 0, uf::audio::bufferSize);
		AL_CHECK_RESULT(alSourceUnqueueBuffers(source.alSource.getIndex(), 1, &index));

		int totalRead = 0;
		while (totalRead < uf::audio::bufferSize) {
			int result = OV_READ(vorbisFile, buffer + totalRead, uf::audio::bufferSize - totalRead, endian, 2, 1, &source.streamState.bitStream);
			if (result <= 0) {
				if (result == 0 && source.settings.loop) {
					uint32_t seekTarget = clip->info.loop.has ? clip->info.loop.start : 0;
					ov_pcm_seek(vorbisFile, seekTarget);
					continue;
				}
				break;
			}
			totalRead += result;
		}

		if (totalRead > 0) {
			AL_CHECK_RESULT(alBufferData(index, clip->info.format, buffer, totalRead, clip->info.frequency));
			AL_CHECK_RESULT(alSourceQueueBuffers(source.alSource.getIndex(), 1, &index));
		}
	}

	if ( source.settings.loopMode == 1 ) source.alSource.set(AL_LOOPING, AL_TRUE);
}

void ext::vorbis::close( pod::AudioClip& clip ) {
	// ...
}
void ext::vorbis::close( pod::AudioSource& source ) {
	if ( !source.clip || !source.clip->streamed ) return;

	ALint queued;
	source.alSource.get(AL_BUFFERS_QUEUED, queued);
	while ( queued-- ) {
		ALuint buffer;
		alSourceUnqueueBuffers(source.alSource.getIndex(), 1, &buffer);
	}
	source.streamBuffers.destroy();

	if ( source.streamState.handle ) {
		OggVorbis_File* file = (OggVorbis_File*) source.streamState.handle;
		ov_clear(file);
		delete file;
		source.streamState.handle = NULL;
		source.streamState.context = NULL;
	}
}

#endif