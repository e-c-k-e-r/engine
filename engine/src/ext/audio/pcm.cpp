#include <uf/config.h>
#if UF_USE_WAV

#if UF_USE_OPENAL
#include <uf/ext/oal/oal.h>
#endif

#include <uf/ext/audio/pcm.h>
#include <uf/utils/memory/pool.h>
#include <iostream>
#include <cstdio>

void ext::pcm::open( uf::Audio::Metadata& metadata, const pod::PCM& pcm ) {
	metadata.info.channels = pcm.channels;
	metadata.info.bitDepth = 16;
	metadata.info.frequency = pcm.sampleRate;
	metadata.info.duration = double(pcm.waveform.size()) / pcm.channels / pcm.sampleRate;
	metadata.info.size = pcm.waveform.size() * sizeof(int16_t);


	// Determine OpenAL format
	if (pcm.channels == 1)
		metadata.info.format = AL_FORMAT_MONO16;
	else if (pcm.channels == 2)
		metadata.info.format = AL_FORMAT_STEREO16;
	else {
		UF_MSG_ERROR("PCM: Only mono or stereo supported ({} channels)", pcm.channels);
		return;
	}

	metadata.stream.handle = malloc( metadata.info.size );
	metadata.stream.consumed = 0;

	// Convert float waveform to int16_t PCM
	int16_t* pcm16 = (int16_t*) metadata.stream.handle;
	for (size_t i = 0; i < pcm.waveform.size(); ++i) {
		float sample = std::clamp(pcm.waveform[i], -1.0f, 1.0f);
		pcm16[i] = static_cast<int16_t>(sample * 32767.0f);
	}

	// choose load or stream
	return metadata.settings.streamed ? ext::pcm::stream(metadata) : ext::pcm::load(metadata);
}

void ext::pcm::load( uf::Audio::Metadata& metadata ) {
	if ( metadata.settings.streamed ) return ext::pcm::stream( metadata );

	// Upload to OpenAL buffer
	metadata.al.buffer.buffer(metadata.info.format, metadata.stream.handle, (ALsizei) metadata.info.size, metadata.info.frequency);
	metadata.al.source.set(AL_BUFFER, (ALint) metadata.al.buffer.getIndex());
}

void ext::pcm::stream(uf::Audio::Metadata& metadata) {
	if ( !metadata.settings.streamed ) return ext::pcm::load( metadata );

	int16_t* pcm16 = (int16_t*) metadata.stream.handle;
	size_t frameSize = metadata.info.channels * sizeof(int16_t);
	size_t totalFrames = metadata.info.size / frameSize;
	size_t bufferFrames = uf::audio::bufferSize / frameSize;

	uint8_t queuedBuffers = 0;
	size_t offset = 0;
	for (; queuedBuffers < metadata.settings.buffers; ++queuedBuffers) {
		size_t framesToRead = std::min(bufferFrames, totalFrames - offset);
		size_t bytesToRead = framesToRead * frameSize;

		if (framesToRead == 0) {
			if (metadata.settings.loop) {
				offset = 0;
				framesToRead = std::min(bufferFrames, totalFrames);
				bytesToRead = framesToRead * frameSize;
			} else {
				break;
			}
		}

		AL_CHECK_RESULT(alBufferData(metadata.al.buffer.getIndex(queuedBuffers), metadata.info.format,
									pcm16 + offset * metadata.info.channels, (ALsizei)bytesToRead, metadata.info.frequency));
		offset += framesToRead;
	}
	metadata.stream.consumed = offset * frameSize;
	AL_CHECK_RESULT(alSourceQueueBuffers(metadata.al.source.getIndex(), queuedBuffers, &metadata.al.buffer.getIndex()));

	// Switch to soft looping if needed
	if (queuedBuffers >= metadata.settings.buffers) {
		metadata.settings.loopMode = 1;
		metadata.al.source.set(AL_LOOPING, AL_FALSE);
	}
}

void ext::pcm::update(uf::Audio::Metadata& metadata) {
	if (!metadata.settings.streamed) return;
	if (metadata.settings.loopMode == 1) metadata.al.source.set(AL_LOOPING, AL_FALSE);

	ALint state;
	metadata.al.source.get(AL_SOURCE_STATE, state);
	if (state != AL_PLAYING) {
		if (!metadata.settings.loop && metadata.stream.consumed >= metadata.info.size) {
			// stream finished
			return;
		}
		metadata.al.source.play();
	}

	ALint processed = 0;
	metadata.al.source.get(AL_BUFFERS_PROCESSED, processed);
	if (processed <= 0) return;

	int16_t* pcm16 = (int16_t*) metadata.stream.handle;
	size_t frameSize = metadata.info.channels * sizeof(int16_t);
	size_t totalFrames = metadata.info.size / frameSize;
	size_t bufferFrames = uf::audio::bufferSize / frameSize;

	ALuint index;
	while (processed--) {
		AL_CHECK_RESULT(alSourceUnqueueBuffers(metadata.al.source.getIndex(), 1, &index));

		size_t offset = metadata.stream.consumed / frameSize;
		size_t framesToRead = std::min(bufferFrames, totalFrames - offset);
		size_t bytesToRead = framesToRead * frameSize;

		if (framesToRead == 0) {
			if (!metadata.settings.loop) break;
			offset = 0;
			framesToRead = std::min(bufferFrames, totalFrames);
			bytesToRead = framesToRead * frameSize;
		}

		if (framesToRead > 0) {
			AL_CHECK_RESULT(alBufferData(index, metadata.info.format,
										 pcm16 + offset * metadata.info.channels, (ALsizei)bytesToRead, metadata.info.frequency));
			AL_CHECK_RESULT(alSourceQueueBuffers(metadata.al.source.getIndex(), 1, &index));
			metadata.stream.consumed = (offset + framesToRead) * frameSize;
		}
		if (metadata.settings.loop && bytesToRead < uf::audio::bufferSize) {
			UF_MSG_ERROR("PCM: missing data: {}", metadata.filename);
		}
	}
	if (metadata.settings.loopMode == 1) metadata.al.source.set(AL_LOOPING, AL_TRUE);
}

void ext::pcm::close(uf::Audio::Metadata& metadata) {
	if ( metadata.stream.handle ) {
		free( metadata.stream.handle );
		metadata.stream.handle = NULL;
	}
}

#endif