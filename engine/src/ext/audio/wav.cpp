#include <uf/config.h>
#if UF_USE_WAV

#if UF_USE_OPENAL
#include <uf/ext/openal/openal.h>
#endif

#if UF_USE_AICA
#include <uf/ext/aica/aica.h>
#endif

#include <uf/ext/audio/wav.h>
#include <uf/utils/memory/pool.h>
#include <uf/utils/io/vfs.h>
#include <cstdio>
#include <cstring>

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

namespace {
	struct DrWavVfsContext {
		drwav wav;
		pod::File file;

		~DrWavVfsContext() {
			if ( file ) file.close( file.handle );
		}
	};

	namespace funs {
		size_t read( void* pUserData, void* pBufferOut, size_t bytesToRead ) {
			DrWavVfsContext* ctx = (DrWavVfsContext*)pUserData;
			if ( !ctx->file ) return 0;
			return ctx->file.read(ctx->file.handle, pBufferOut, bytesToRead);
		}

		drwav_bool32 seek( void* pUserData, int offset, drwav_seek_origin origin ) {
			DrWavVfsContext* ctx = (DrWavVfsContext*)pUserData;
			if ( !ctx->file ) return 0;

			int whence = SEEK_SET;
			if ( origin == DRWAV_SEEK_CUR ) whence = SEEK_CUR;
			else if ( origin == DRWAV_SEEK_END ) whence = SEEK_END;

			if ( ctx->file.seek(ctx->file.handle, offset, whence) ) return 1;
			return 0;
		}

		drwav_bool32 tell( void* pUserData, long long int* pCursor ) {
			DrWavVfsContext* ctx = (DrWavVfsContext*)pUserData;
			if ( !ctx->file ) return 0;
			if ( pCursor ) *pCursor = (long long int)ctx->file.tell(ctx->file.handle);
			return 1;
		}

		int fill_buffer( void* user_data, uint8_t* buffer, int req_bytes ) {
			pod::AudioSource* source = (pod::AudioSource*)user_data;
			pod::AudioClip* clip = source->clip;
			DrWavVfsContext* ctx = (DrWavVfsContext*)source->streamState.handle;
			drwav* wav = &ctx->wav;

			size_t frameSize = wav->channels * sizeof(int16_t);
			drwav_uint64 framesToRead = req_bytes / frameSize;
			drwav_uint64 totalFramesRead = 0;
			int16_t* bufferPtr = (int16_t*)buffer;

			while ( totalFramesRead < framesToRead ) {
				drwav_uint64 framesRead = drwav_read_pcm_frames_s16(wav, framesToRead - totalFramesRead, bufferPtr);

				totalFramesRead += framesRead;
				bufferPtr += (framesRead * wav->channels);

				if ( framesRead == 0 ) {
					if ( !source->info.pending.empty() ) {
						uf::stl::string nextFile = source->info.pending.front();
						source->info.pending.erase(source->info.pending.begin());

						drwav_uninit(wav);

						if ( ctx->file ) ctx->file.close(ctx->file.handle);
						ctx->file = uf::vfs::open( nextFile );

						if ( !drwav_init(wav, funs::read, funs::seek, funs::tell, ctx, nullptr) ) {
							UF_MSG_ERROR("Transition failed! Could not open: {}", nextFile);
							break;
						}

						clip->filename = nextFile;
						clip->info.size = wav->totalPCMFrameCount * wav->channels * sizeof(int16_t);
						clip->info.duration = (double)wav->totalPCMFrameCount / wav->sampleRate;

						source->info.elapsed = 0.0f;
						source->info.timer.reset();
						source->info.timer.start();
					} else if ( source->settings.loop ) {
						drwav_seek_to_pcm_frame(wav, clip->info.loop.has ? clip->info.loop.start : 0);
					} else {
						break;
					}
				}
			}

			return (int)(totalFramesRead * frameSize);
		}
	}

	// default to 16-bit audio
	inline bool format( pod::AudioClip& clip, int channels, int bitDepth ) {
		if (channels == 1) clip.info.format = AL_FORMAT_MONO16;
		else if (channels == 2) clip.info.format = AL_FORMAT_STEREO16;
		else {
			UF_MSG_ERROR("WAV: unrecognized format: {} channels", channels);
			return false;
		}
		return true;
	}
}

void ext::wav::load( pod::AudioClip& clip ) {
	DrWavVfsContext* ctx = new DrWavVfsContext();
	ctx->file = uf::vfs::open(clip.filename);
	if ( !ctx->file ) {
		UF_MSG_ERROR("WAV: failed to open file: {}", clip.filename);
		delete ctx;
		return;
	}

	if ( !drwav_init(&ctx->wav, funs::read, funs::seek, funs::tell, ctx, nullptr) ) {
		UF_MSG_ERROR("Could not open WAV file: {}", clip.filename);
		delete ctx;
		return;
	}

	drwav* wav = &ctx->wav;
	clip.info.channels = wav->channels;
	clip.info.bitDepth = 16; // wav->bitsPerSample;
	clip.info.size = wav->totalPCMFrameCount * wav->channels * sizeof(uint16_t); // (wav->bitsPerSample / 8);
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

	if ( !format( clip, wav->channels, wav->bitsPerSample ) ) {
		drwav_uninit(wav); delete ctx;
		return;
	}

	if ( !clip.streamed ) {
		uf::stl::vector<int16_t> pcm(wav->totalPCMFrameCount * wav->channels);
		drwav_read_pcm_frames_s16(wav, wav->totalPCMFrameCount, pcm.data());
		clip.alBuffer.buffer(clip.info.format, pcm.data(), (ALsizei)(pcm.size() * sizeof(int16_t)), clip.info.frequency);
		if ( clip.info.loop.has ) {
			ALint loopPoints[2] = { (ALint) clip.info.loop.start, (ALint) clip.info.loop.end };
			clip.alBuffer.set( 0x2015 /* AL_LOOP_POINTS_SOFT */, loopPoints );
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
	ctx->file = uf::vfs::open(clip->filename);
	if ( !ctx->file ) {
		UF_MSG_ERROR("WAV: failed to open file: {}", clip->filename);
		delete ctx;
		return;
	}

	if ( !drwav_init(&ctx->wav, funs::read, funs::seek, funs::tell, ctx, nullptr) ) {
		UF_MSG_ERROR("WAV: failed to open file: {}", clip->filename);
		delete ctx; return;
	}

	source.streamState.handle = (void*) ctx;

#if UF_USE_AICA
	source.streamBuffers.set( AL_STREAM_FILL_CALLBACK, (ALint*)(funs::fill_buffer) );
	source.streamBuffers.set( AL_STREAM_USER_DATA, (ALint*)(&source) );
	source.alSource.set(AL_BUFFER, (ALint)(source.streamBuffers.getIndex(0)));
	source.streamBuffers.buffer( clip->info.format, NULL, 0, clip->info.frequency );
#else
	uint8_t buffer[uf::audio::bufferSize];
	uint8_t queuedBuffers = 0;

	for ( ; queuedBuffers < source.settings.buffers; ++queuedBuffers ) {
		int totalRead = funs::fill_buffer(&source, buffer, uf::audio::bufferSize);
		if ( totalRead == 0 ) break;

		ext::al::Buffer::buffer( source.streamBuffers.getIndex(queuedBuffers), clip->info.format, buffer, totalRead, clip->info.frequency);
	}
	source.alSource.queue( queuedBuffers, &source.streamBuffers.getIndex(0) );

	if ( queuedBuffers >= source.settings.buffers ) {
		source.settings.loopMode = 1;
		source.alSource.set(AL_LOOPING, AL_FALSE);
	}
#endif
}

void ext::wav::update( pod::AudioSource& source ) {
	pod::AudioClip* clip = source.clip;
	if ( !clip || !clip->streamed || !source.streamState.handle ) return;

#if UF_USE_AICA
	ALint state;
	source.alSource.get(AL_SOURCE_STATE, state);
	if ( state == AL_PLAYING ) {
		source.streamBuffers.poll();
	}
#else
	DrWavVfsContext* ctx = (DrWavVfsContext*) source.streamState.handle;
	ALint state, processed, queued;

	source.alSource.get(AL_SOURCE_STATE, state);
	source.alSource.get(AL_BUFFERS_PROCESSED, processed);
	source.alSource.get(AL_BUFFERS_QUEUED, queued);

	bool hasData = (ctx->file.tell(ctx->file.handle) < clip->info.size) || !source.info.pending.empty() || source.settings.loop;
#if !NO_FUN
	if ( source.settings.loopMode == 1 ) {
		source.alSource.set(AL_LOOPING, AL_FALSE);
	}
#endif

	auto fillAndQueueBuffer = [&](ALuint index) -> bool {
		uint8_t buffer[uf::audio::bufferSize];
		int totalRead = funs::fill_buffer(&source, buffer, uf::audio::bufferSize);

		if ( totalRead > 0 ) {
			ext::al::Buffer::buffer(index, clip->info.format, buffer, totalRead, clip->info.frequency);
			source.alSource.queue( 1, &index );
			return true;
		}
		return false;
	};

	if ( queued == 0 ) {
		if ( hasData ) {
			for ( int i = 0; i < source.settings.buffers; ++i ) {
				if ( !fillAndQueueBuffer(source.streamBuffers.getIndex(i)) ) break;
			}
		}
	} else {
		while ( processed > 0 ) {
			ALuint index;
			source.alSource.unqueue( 1, &index );
			processed--;

			bool hasData = (ctx->file.tell(ctx->file.handle) < clip->info.size) || !source.info.pending.empty() || source.settings.loop;

			if ( hasData ) fillAndQueueBuffer(index);
		}
	}

	source.alSource.get(AL_SOURCE_STATE, state);
	source.alSource.get(AL_BUFFERS_QUEUED, queued);

	if ( state != AL_PLAYING && queued > 0 ) {
		source.alSource.play();
	}
#if !NO_FUN
	if ( source.settings.loopMode == 1 ) source.alSource.set(AL_LOOPING, AL_FALSE);
#endif

#endif
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
		source.alSource.unqueue( 1, &buffer );
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
