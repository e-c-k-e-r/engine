#include <uf/config.h>
#if UF_USE_WAV

#if UF_USE_OPENAL
#include <uf/ext/openal/openal.h>
#endif

#include <uf/ext/audio/wav.h>
#include <uf/utils/memory/pool.h>
#include <uf/utils/io/vfs.h>
#include <iostream>
#include <cstdio>
#include <cstring>

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

namespace {
	struct DrWavVfsContext {
		drwav wav;
		uf::stl::string filename;
		size_t currentOffset;
		size_t totalSize;
	};

	size_t drwav_vfs_read( void* pUserData, void* pBufferOut, size_t bytesToRead ) {
		DrWavVfsContext* ctx = (DrWavVfsContext*)pUserData;
		size_t bytesLeft = ctx->totalSize - ctx->currentOffset;
		if ( bytesToRead > bytesLeft ) bytesToRead = bytesLeft;

		if ( bytesToRead > 0 ) {
			uf::stl::vector<uint8_t> tempBuffer;
			if ( uf::vfs::readRange(ctx->filename, ctx->currentOffset, bytesToRead, tempBuffer) ) {
				std::memcpy(pBufferOut, tempBuffer.data(), tempBuffer.size());
				ctx->currentOffset += tempBuffer.size();
				return tempBuffer.size();
			}
		}
		return 0;
	}

	drwav_bool32 drwav_vfs_seek( void* pUserData, int offset, drwav_seek_origin origin ) {
		DrWavVfsContext* ctx = (DrWavVfsContext*)pUserData;
		long long targetOffset = 0;
		if ( (int)origin == 0 ) targetOffset = offset;
		else if ( (int)origin == 1 ) targetOffset = (long long)ctx->currentOffset + offset;
		else if ( (int)origin == 2 ) targetOffset = (long long)ctx->totalSize + offset;

		if (targetOffset < 0) targetOffset = 0;
		if ((size_t)targetOffset > ctx->totalSize) targetOffset = ctx->totalSize;

		ctx->currentOffset = (size_t)targetOffset;
		return 1;
	}

	drwav_bool32 drwav_vfs_tell( void* pUserData, long long int* pCursor ) {
		DrWavVfsContext* ctx = (DrWavVfsContext*)pUserData;
		if (pCursor) *pCursor = (long long int)ctx->currentOffset;
		return 1;
	}

	inline bool format( pod::AudioClip& clip, int channels, int bitDepth ) {
		if (channels == 1 && bitDepth == 8) clip.info.format = AL_FORMAT_MONO8;
		else if (channels == 1 && bitDepth == 16) clip.info.format = AL_FORMAT_MONO16;
		else if (channels == 2 && bitDepth == 8) clip.info.format = AL_FORMAT_STEREO8;
		else if (channels == 2 && bitDepth == 16) clip.info.format = AL_FORMAT_STEREO16;
		else {
			UF_MSG_ERROR("WAV: unrecognized format: {} channels, {} bps", channels, bitDepth);
			return false;
		}
		return true;
	}
}

void ext::wav::load( pod::AudioClip& clip ) {
	DrWavVfsContext* ctx = new DrWavVfsContext();
	ctx->filename = clip.filename;
	ctx->currentOffset = 0;
	ctx->totalSize = uf::vfs::size(clip.filename);

	if ( !drwav_init(&ctx->wav, drwav_vfs_read, drwav_vfs_seek, drwav_vfs_tell, ctx, nullptr) ) {
		UF_MSG_ERROR("Could not open WAV file: {}", clip.filename);
		delete ctx;
		return;
	}

	drwav* wav = &ctx->wav;
	clip.info.size = wav->totalPCMFrameCount * wav->channels * (wav->bitsPerSample / 8);
	clip.info.channels = wav->channels;
	clip.info.bitDepth = wav->bitsPerSample;
	clip.info.frequency = wav->sampleRate;
	clip.info.duration = (double) wav->totalPCMFrameCount / wav->sampleRate;
	clip.info.loop.has = false;
	clip.info.loop.start = 0;
	clip.info.loop.end = (uint32_t)wav->totalPCMFrameCount;

	for ( drwav_uint32 i = 0; i < wav->metadataCount; ++i ) {
		if ( wav->pMetadata[i].type == drwav_metadata_type_smpl ) {
			const drwav_smpl& smpl = wav->pMetadata[i].data.smpl;
			if ( smpl.sampleLoopCount > 0 && smpl.pLoops != nullptr ) {
				clip.info.loop.has = true;
				clip.info.loop.start = smpl.pLoops[0].firstSampleOffset;
				clip.info.loop.end = smpl.pLoops[0].lastSampleOffset;
				if ( clip.info.loop.end == 0 ) clip.info.loop.end = (uint32_t)wav->totalPCMFrameCount;
			}
			break;
		}
	}

	if ( !format(clip, wav->channels, wav->bitsPerSample) ) {
		drwav_uninit(wav); delete ctx;
		return;
	}

	if (!clip.streamed) {
		uf::stl::vector<uint8_t> bytes(clip.info.size);
		drwav_read_pcm_frames(wav, wav->totalPCMFrameCount, bytes.data());

		clip.alBuffer.buffer(clip.info.format, bytes.data(), (ALsizei)bytes.size(), clip.info.frequency);
		if ( clip.info.loop.has ) {
			ALint loopPoints[2] = { (ALint) clip.info.loop.start, (ALint) clip.info.loop.end };
			alBufferiv(clip.alBuffer.getIndex(0), 0x2015 /* AL_LOOP_POINTS_SOFT */, loopPoints);
		}
	}

	drwav_uninit(wav);
	delete ctx;
}

void ext::wav::open( pod::AudioSource& source ) {
	pod::AudioClip* clip = source.clip;
	if ( !clip || !clip->streamed ) return;

	source.streamBuffers.initialize(source.settings.buffers);

	DrWavVfsContext* ctx = new DrWavVfsContext();
	ctx->filename = clip->filename;
	ctx->currentOffset = 0;
	ctx->totalSize = uf::vfs::size(clip->filename);

	if ( !drwav_init(&ctx->wav, drwav_vfs_read, drwav_vfs_seek, drwav_vfs_tell, ctx, nullptr) ) {
		delete ctx; return;
	}

	source.streamState.handle = (void*) ctx;
	drwav* wav = &ctx->wav;

	size_t frameSize = wav->channels * (wav->bitsPerSample / 8);
	size_t bufferFrames = uf::audio::bufferSize / frameSize;

	char buffer[uf::audio::bufferSize];
	uint8_t queuedBuffers = 0;

	for ( ; queuedBuffers < source.settings.buffers; ++queuedBuffers ) {
		drwav_uint64 framesRead = drwav_read_pcm_frames(wav, bufferFrames, buffer);

		if ( framesRead == 0 ) {
			if ( source.settings.loop ) {
				drwav_seek_to_pcm_frame(wav, clip->info.loop.has ? clip->info.loop.start : 0);
				framesRead = drwav_read_pcm_frames(wav, bufferFrames, buffer);
			}
		}
		if ( framesRead == 0 ) break;

		AL_CHECK_RESULT(alBufferData(source.streamBuffers.getIndex(queuedBuffers), clip->info.format, buffer, (ALsizei)(framesRead * frameSize), clip->info.frequency));
	}
	AL_CHECK_RESULT(alSourceQueueBuffers(source.alSource.getIndex(), queuedBuffers, &source.streamBuffers.getIndex(0)));

	if ( queuedBuffers >= source.settings.buffers ) {
		source.settings.loopMode = 1;
		source.alSource.set(AL_LOOPING, AL_FALSE);
	}
}

void ext::wav::update( pod::AudioSource& source ) {
	pod::AudioClip* clip = source.clip;
	if ( !clip || !clip->streamed || !source.streamState.handle ) return;

	if ( source.settings.loopMode == 1 ) source.alSource.set(AL_LOOPING, AL_FALSE);

	DrWavVfsContext* ctx = (DrWavVfsContext*) source.streamState.handle;
	drwav* wav = &ctx->wav;

	ALint state;
	source.alSource.get(AL_SOURCE_STATE, state);
	if ( state != AL_PLAYING ) {
		if ( !source.settings.loop && ctx->currentOffset >= ctx->totalSize ) return;
		source.alSource.play();
	}

	ALint processed = 0;
	source.alSource.get(AL_BUFFERS_PROCESSED, processed);
	if ( processed <= 0 ) return;

	size_t frameSize = wav->channels * (wav->bitsPerSample / 8);
	size_t bufferFrames = uf::audio::bufferSize / frameSize;

	ALuint index;
	char buffer[uf::audio::bufferSize];

	while ( processed-- ) {
		AL_CHECK_RESULT(alSourceUnqueueBuffers(source.alSource.getIndex(), 1, &index));

		drwav_uint64 framesRead = drwav_read_pcm_frames(wav, bufferFrames, buffer);

		if ( framesRead == 0 ) {
			if ( !source.settings.loop ) break;
			drwav_seek_to_pcm_frame(wav, clip->info.loop.has ? clip->info.loop.start : 0);
			framesRead = drwav_read_pcm_frames(wav, bufferFrames, buffer);
		}

		if ( framesRead > 0 ) {
			AL_CHECK_RESULT(alBufferData(index, clip->info.format, buffer, (ALsizei)(framesRead * frameSize), clip->info.frequency));
			AL_CHECK_RESULT(alSourceQueueBuffers(source.alSource.getIndex(), 1, &index));
		}
	}
	if ( source.settings.loopMode == 1 ) source.alSource.set(AL_LOOPING, AL_TRUE);
}

void ext::wav::close( pod::AudioClip& clip ) {
	// ...
}
void ext::wav::close(pod::AudioSource& source) {
	if ( !source.clip || !source.clip->streamed ) return;

	ALint queued;
	source.alSource.get(AL_BUFFERS_QUEUED, queued);
	while ( queued-- ) {
		ALuint buffer;
		alSourceUnqueueBuffers(source.alSource.getIndex(), 1, &buffer);
	}
	source.streamBuffers.destroy();

	if ( source.streamState.handle ) {
		DrWavVfsContext* ctx = (DrWavVfsContext*)source.streamState.handle;
		drwav_uninit(&ctx->wav);
		delete ctx;
		source.streamState.handle = nullptr;
	}
}

#endif