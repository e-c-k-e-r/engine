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

	size_t drwav_vfs_read(void* pUserData, void* pBufferOut, size_t bytesToRead) {
		DrWavVfsContext* ctx = (DrWavVfsContext*)pUserData;
		size_t bytesLeft = ctx->totalSize - ctx->currentOffset;
		if (bytesToRead > bytesLeft) bytesToRead = bytesLeft;

		if (bytesToRead > 0) {
			uf::stl::vector<uint8_t> tempBuffer;
			if (uf::vfs::readRange(ctx->filename, ctx->currentOffset, bytesToRead, tempBuffer)) {
				std::memcpy(pBufferOut, tempBuffer.data(), tempBuffer.size());
				ctx->currentOffset += tempBuffer.size();
				return tempBuffer.size();
			}
		}
		return 0;
	}

	drwav_bool32 drwav_vfs_seek(void* pUserData, int offset, drwav_seek_origin origin) {
		DrWavVfsContext* ctx = (DrWavVfsContext*)pUserData;
		long long targetOffset = 0;

		if ( (int)origin == 0 ) {
			targetOffset = offset;
		} else if ( (int)origin == 1 ) {
			targetOffset = (long long)ctx->currentOffset + offset;
		}

		if (targetOffset < 0) targetOffset = 0;
		if ((size_t)targetOffset > ctx->totalSize) targetOffset = ctx->totalSize;

		ctx->currentOffset = (size_t)targetOffset;
		return 1;
	}

	drwav_bool32 drwav_vfs_tell(void* pUserData, long long int* pCursor) {
		DrWavVfsContext* ctx = (DrWavVfsContext*)pUserData;
		if (pCursor) {
			*pCursor = (long long int)ctx->currentOffset;
		}
		return 1;
	}

	namespace funs {
		size_t read(void* destination, size_t size, size_t nmemb, void* userdata) {
			uf::Audio::Metadata& metadata = *((uf::Audio::Metadata*)userdata);
			DrWavVfsContext* ctx = (DrWavVfsContext*) metadata.stream.handle;
			drwav* wav = &ctx->wav;

			size_t bytesRequested = size * nmemb;
			size_t frameSize = wav->channels * (wav->bitsPerSample / 8);
			size_t framesToRead = bytesRequested / frameSize;

			drwav_uint64 framesRead = drwav_read_pcm_frames(wav, framesToRead, destination);
			size_t bytesRead = framesRead * frameSize;

			metadata.stream.consumed += bytesRead;
			return bytesRead;
		}
		int seek(void* userdata, int64_t to, int type) {
			uf::Audio::Metadata& metadata = *((uf::Audio::Metadata*)userdata);
			DrWavVfsContext* ctx = (DrWavVfsContext*) metadata.stream.handle;
			drwav* wav = &ctx->wav;

			drwav_uint64 targetFrame = 0;
			switch (type) {
				case SEEK_CUR: targetFrame = metadata.stream.consumed / (wav->channels * (wav->bitsPerSample / 8)) + to; break;
				case SEEK_END: targetFrame = wav->totalPCMFrameCount - to; break;
				case SEEK_SET: targetFrame = to; break;
				default: return -1;
			}
			if (!drwav_seek_to_pcm_frame(wav, targetFrame)) return -1;
			metadata.stream.consumed = (size_t)(targetFrame * wav->channels * (wav->bitsPerSample / 8));
			return 0;
		}
		int close(void* userdata) {
			uf::Audio::Metadata& metadata = *((uf::Audio::Metadata*)userdata);
			DrWavVfsContext* ctx = (DrWavVfsContext*) metadata.stream.handle;
			if (ctx) {
				drwav_uninit(&ctx->wav);
				delete ctx;
				metadata.stream.handle = nullptr;
			}
			return 0;
		}
		long tell(void* userdata) {
			uf::Audio::Metadata& metadata = *((uf::Audio::Metadata*)userdata);
			return metadata.stream.consumed;
		}
	}
}

void ext::wav::open(uf::Audio::Metadata& metadata) {
	DrWavVfsContext* ctx = new DrWavVfsContext();
	ctx->filename = metadata.filename;
	ctx->currentOffset = 0;
	ctx->totalSize = uf::vfs::size(metadata.filename);

	if (!drwav_init(&ctx->wav, drwav_vfs_read, drwav_vfs_seek, drwav_vfs_tell, ctx, nullptr)) {
		UF_MSG_ERROR("Could not open WAV file: {}", metadata.filename);
		delete ctx;
		return;
	}

	drwav* wav = &ctx->wav;

	// fill out metadata
	metadata.stream.handle = ctx;
	metadata.info.size = wav->totalPCMFrameCount * wav->channels * (wav->bitsPerSample / 8);
	metadata.stream.consumed = 0;
	metadata.info.channels = wav->channels;
	metadata.info.bitDepth = wav->bitsPerSample;
	metadata.info.frequency = wav->sampleRate;
	metadata.info.duration = (double) wav->totalPCMFrameCount / wav->sampleRate;

	for ( drwav_uint32 i = 0; i < wav->metadataCount; ++i ) {
		if ( wav->pMetadata[i].type == drwav_metadata_type_smpl ) {
			const drwav_smpl& smpl = wav->pMetadata[i].data.smpl;

			if ( smpl.sampleLoopCount > 0 && smpl.pLoops != nullptr ) {
				metadata.info.loop.has = true;
				metadata.info.loop.start = smpl.pLoops[0].firstSampleOffset;
				metadata.info.loop.end = smpl.pLoops[0].lastSampleOffset;

				if ( metadata.info.loop.end == 0 ) {
					metadata.info.loop.end = (uint32_t)wav->totalPCMFrameCount;
				}
			}
			break;
		}
	}

	// determine OpenAL format
	if (wav->channels == 1 && wav->bitsPerSample == 8)
		metadata.info.format = AL_FORMAT_MONO8;
	else if (wav->channels == 1 && wav->bitsPerSample == 16)
		metadata.info.format = AL_FORMAT_MONO16;
	else if (wav->channels == 2 && wav->bitsPerSample == 8)
		metadata.info.format = AL_FORMAT_STEREO8;
	else if (wav->channels == 2 && wav->bitsPerSample == 16)
		metadata.info.format = AL_FORMAT_STEREO16;
	else {
		UF_MSG_ERROR("WAV: unrecognized format: {} channels, {} bps", wav->channels, wav->bitsPerSample);
		funs::close(&metadata);
		return;
	}

	// choose load or stream
	if (metadata.settings.streamed) ext::wav::stream(metadata); else ext::wav::load(metadata);
}

void ext::wav::load(uf::Audio::Metadata& metadata) {
	// if streaming is requested, use streaming function
	if (metadata.settings.streamed) return ext::wav::stream(metadata);
	// read all PCM data
	size_t totalBytes = (size_t) metadata.info.size;
	std::vector<uint8_t> bytes(totalBytes);

	size_t bytesRead = funs::read(bytes.data(), 1, totalBytes, &metadata);
	if (bytesRead < totalBytes) {
		bytes.resize(bytesRead);
	}

	metadata.al.buffer.buffer(metadata.info.format, bytes.data(), (ALsizei) bytes.size(), metadata.info.frequency);
	if ( metadata.info.loop.has ) {
		ALint loopPoints[2] = { (ALint) metadata.info.loop.start, (ALint) metadata.info.loop.end };
		alBufferiv(metadata.al.buffer.getIndex(), 0x2015 /* AL_LOOP_POINTS_SOFT */, loopPoints);
	}
	metadata.al.source.set(AL_BUFFER, (ALint) metadata.al.buffer.getIndex());

	funs::close(&metadata);
}

void ext::wav::stream(uf::Audio::Metadata& metadata) {
	if (!metadata.settings.streamed) return ext::wav::load(metadata);

	funs::seek(&metadata, 0, SEEK_SET);

	char buffer[uf::audio::bufferSize];
	uint8_t queuedBuffers = 0;
	for (; queuedBuffers < metadata.settings.buffers; ++queuedBuffers) {
		size_t bytesRead = funs::read(buffer, 1, uf::audio::bufferSize, &metadata);

		if (bytesRead == 0) {
			if (metadata.settings.loop) {
				metadata.stream.consumed = 0;
				if ( funs::seek(&metadata, metadata.info.loop.start, SEEK_SET) != 0 ) {
					UF_MSG_ERROR("WAV: failed to loop (seek to start): {}", metadata.filename);
					break;
				}
				bytesRead = funs::read(buffer, 1, uf::audio::bufferSize, &metadata);
			}
		}

		if (bytesRead == 0) {
			if ( queuedBuffers == 0 ) UF_MSG_WARNING("WAV: consumed file stream before {} buffers were filled: {}", (int) queuedBuffers, metadata.filename);
			break;
		}

		AL_CHECK_RESULT(alBufferData(metadata.al.buffer.getIndex(queuedBuffers), metadata.info.format, buffer, bytesRead, metadata.info.frequency));
	}
	AL_CHECK_RESULT(alSourceQueueBuffers(metadata.al.source.getIndex(), queuedBuffers, &metadata.al.buffer.getIndex()));

	if (queuedBuffers >= metadata.settings.buffers) {
		metadata.settings.loopMode = 1;
		metadata.al.source.set(AL_LOOPING, AL_FALSE);
	}
}

void ext::wav::update(uf::Audio::Metadata& metadata) {
	if (!metadata.settings.streamed) return;
	// disable hard looping temporarily
	if (metadata.settings.loopMode == 1) metadata.al.source.set(AL_LOOPING, AL_FALSE);

	ALint state;
	metadata.al.source.get(AL_SOURCE_STATE, state);
	if (state != AL_PLAYING) {
		if (!metadata.settings.loop && metadata.stream.consumed >= metadata.info.size) {
			return; // stream finished
		}
		// stream stalled, restart it
		metadata.al.source.play();
	}
	ALint processed = 0;
	metadata.al.source.get(AL_BUFFERS_PROCESSED, processed);
	if (processed <= 0) return;

	ALuint index;
	char buffer[uf::audio::bufferSize];
	while (processed--) {
		memset(buffer, 0, uf::audio::bufferSize);
		AL_CHECK_RESULT(alSourceUnqueueBuffers(metadata.al.source.getIndex(), 1, &index));

		size_t bytesRead = funs::read(buffer, 1, uf::audio::bufferSize, &metadata);

		if (bytesRead == 0) {
			// no more data left to read, reset file stream if we're looping
			if (!metadata.settings.loop) break;
			if ( funs::seek(&metadata, metadata.info.loop.start, SEEK_SET) != 0 ) {
				UF_MSG_ERROR("WAV: failed to loop (seek to start): {}", metadata.filename);
				break;
			}
			bytesRead = funs::read(buffer, 1, uf::audio::bufferSize, &metadata);
			if (bytesRead == 0) {
				// should never actually reach here
				UF_MSG_ERROR("WAV: failed to read after looping: {}", metadata.filename);
				break;
			}
		}

		if (bytesRead > 0) {
			AL_CHECK_RESULT(alBufferData(index, metadata.info.format, buffer, bytesRead, metadata.info.frequency));
			AL_CHECK_RESULT(alSourceQueueBuffers(metadata.al.source.getIndex(), 1, &index));
		}
		if (metadata.settings.loop && bytesRead < uf::audio::bufferSize) {
			UF_MSG_ERROR("WAV: missing data: {}", metadata.filename);
		}
	}
	// enable hard looping for if we aren't able to call an update in a timely manner
	if (metadata.settings.loopMode == 1) metadata.al.source.set(AL_LOOPING, AL_TRUE);
}

void ext::wav::close(uf::Audio::Metadata& metadata) {
	if (metadata.stream.handle) {
		funs::close(&metadata);
	}
	metadata.stream.handle = nullptr;
}

#endif