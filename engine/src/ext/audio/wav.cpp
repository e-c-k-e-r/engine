#include <uf/config.h>
#if UF_USE_WAV

#if UF_USE_OPENAL
#include <uf/ext/oal/oal.h>
#endif

#include <uf/ext/audio/wav.h>
#include <uf/utils/memory/pool.h>
#include <iostream>
#include <cstdio>

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

namespace {
	namespace funs {
		size_t read(void* destination, size_t size, size_t nmemb, void* userdata) {
			uf::Audio::Metadata& metadata = *((uf::Audio::Metadata*)userdata);
			drwav* wav = (drwav*) metadata.stream.handle;
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
			drwav* wav = (drwav*) metadata.stream.handle;

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
			drwav* wav = (drwav*) metadata.stream.handle;
			if (wav) {
				drwav_uninit(wav);
				delete wav;
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
	// open file
	drwav* wav = new drwav;
	if (!drwav_init_file(wav, metadata.filename.c_str(), nullptr)) {
		UF_MSG_ERROR("Could not open WAV file: {}", metadata.filename);
		delete wav;
		return;
	}

	// fill out metadata
	metadata.stream.handle = wav;
	metadata.info.size = wav->totalPCMFrameCount * wav->channels * (wav->bitsPerSample / 8);
	metadata.stream.consumed = 0;
	metadata.info.channels = wav->channels;
	metadata.info.bitDepth = wav->bitsPerSample;
	metadata.info.frequency = wav->sampleRate;
	metadata.info.duration = (double) wav->totalPCMFrameCount / wav->sampleRate;

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
	return metadata.settings.streamed ? ext::wav::stream(metadata) : ext::wav::load(metadata);
}

void ext::wav::load(uf::Audio::Metadata& metadata) {
	// if streaming is requested, use streaming function
	if (metadata.settings.streamed) return ext::wav::stream(metadata);

	drwav* wav = (drwav*) metadata.stream.handle;

	// read all PCM data
	size_t totalBytes = (size_t) metadata.info.size;
	std::vector<uint8_t> bytes(totalBytes);

	// Use funs::read instead of drwav_read_pcm_frames
	size_t bytesRead = funs::read(bytes.data(), 1, totalBytes, &metadata);
	if (bytesRead < totalBytes) {
		bytes.resize(bytesRead); // in case file is truncated
	}

	// upload to OpenAL buffer
	metadata.al.buffer.buffer(metadata.info.format, bytes.data(), (ALsizei) bytes.size(), metadata.info.frequency);
	metadata.al.source.set(AL_BUFFER, (ALint) metadata.al.buffer.getIndex());

	funs::close(&metadata);
}

void ext::wav::stream(uf::Audio::Metadata& metadata) {
	if (!metadata.settings.streamed) return ext::wav::load(metadata);

	// Ensure we're at the start
	funs::seek(&metadata, 0, SEEK_SET);

	drwav* wav = (drwav*) metadata.stream.handle;
	char buffer[uf::audio::bufferSize];
	uint8_t queuedBuffers = 0;
	for (; queuedBuffers < metadata.settings.buffers; ++queuedBuffers) {
		size_t bytesRead = funs::read(buffer, 1, uf::audio::bufferSize, &metadata);

		if (bytesRead == 0) {
			if (metadata.settings.loop) {
				metadata.stream.consumed = 0;
				if (funs::seek(&metadata, 0, SEEK_SET) != 0) {
					UF_MSG_ERROR("WAV: failed to loop (seek to start): {}", metadata.filename);
					break;
				}
				bytesRead = funs::read(buffer, 1, uf::audio::bufferSize, &metadata);
			}
		}

		if (bytesRead == 0) {
			UF_MSG_WARNING("WAV: consumed file stream before buffers are filled: {} {}", (int)queuedBuffers, metadata.filename);
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
			// stream finished
			return;
		}
		// stream stalled, restart it
		metadata.al.source.play();
	}

	ALint processed = 0;
	metadata.al.source.get(AL_BUFFERS_PROCESSED, processed);
	if (processed <= 0) return;

	drwav* wav = (drwav*) metadata.stream.handle;

	ALuint index;
	char buffer[uf::audio::bufferSize];
	while (processed--) {
		memset(buffer, 0, uf::audio::bufferSize);
		AL_CHECK_RESULT(alSourceUnqueueBuffers(metadata.al.source.getIndex(), 1, &index));

		// Use funs::read instead of drwav_read_pcm_frames
		size_t bytesRead = funs::read(buffer, 1, uf::audio::bufferSize, &metadata);

		if (bytesRead == 0) {
			// no more data left to read, reset file stream if we're looping
			if (!metadata.settings.loop) break;
			if (funs::seek(&metadata, 0, SEEK_SET) != 0) {
				UF_MSG_ERROR("WAV: failed to loop (seek to start): {}", metadata.filename);
				break;
			}
			bytesRead = funs::read(buffer, 1, uf::audio::bufferSize, &metadata);
			if (bytesRead == 0) {
				UF_MSG_ERROR("WAV: failed to read after looping: {}", metadata.filename);
				break;
			}
		}

		if (bytesRead > 0) {
			AL_CHECK_RESULT(alBufferData(index, metadata.info.format, buffer, bytesRead, metadata.info.frequency));
			AL_CHECK_RESULT(alSourceQueueBuffers(metadata.al.source.getIndex(), 1, &index));
		}
		if (metadata.settings.loop && bytesRead < uf::audio::bufferSize) {
			// should never actually reach here
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