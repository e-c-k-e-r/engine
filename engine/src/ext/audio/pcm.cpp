#include <uf/config.h>
#if UF_USE_WAV

#if UF_USE_OPENAL
#include <uf/ext/openal/openal.h>
#endif

#include <uf/ext/audio/pcm.h>
#include <uf/utils/memory/pool.h>
#include <cstdio>

void ext::pcm::load( pod::AudioClip& clip, const pod::PCM& pcm ) {
	clip.info.channels = pcm.channels;
	clip.info.bitDepth = 16;
	clip.info.frequency = pcm.sampleRate;
	clip.info.duration = double(pcm.samples.size()) / pcm.channels / pcm.sampleRate;
	clip.info.size = pcm.samples.size() * sizeof(int16_t);

	if ( pcm.channels == 1 ) clip.info.format = AL_FORMAT_MONO16;
	else if ( pcm.channels == 2 ) clip.info.format = AL_FORMAT_STEREO16;
	else {
		UF_MSG_ERROR("PCM: Only mono or stereo supported ({} channels)", pcm.channels);
		return;
	}

	if ( !clip.streamed ) {
		clip.alBuffer.buffer(clip.info.format, pcm.samples.data(), (ALsizei) clip.info.size, clip.info.frequency);
	} else {
		clip.stream.buffer = malloc( clip.info.size ); // to-do: memory pool
		memcpy( clip.stream.buffer, pcm.samples.data(), clip.info.size );
	}
}

void ext::pcm::open( pod::AudioSource& source ) {
	pod::AudioClip* clip = source.clip;
	if ( !clip || !clip->stream.buffer ) return;

	source.streamBuffers.initialize( source.settings.buffers );
	source.streamState.consumed = 0;

	int16_t* pcm16 = (int16_t*) clip->stream.buffer;
	size_t frameSize = clip->info.channels * sizeof(int16_t);
	size_t totalFrames = clip->info.size / frameSize;
	size_t bufferFrames = uf::audio::bufferSize / frameSize;

	uint8_t queuedBuffers = 0;
	size_t offset = 0;

	for ( ; queuedBuffers < source.settings.buffers; ++queuedBuffers ) {
		size_t framesToRead = std::min(bufferFrames, totalFrames - offset);
		size_t bytesToRead = framesToRead * frameSize;

		if ( framesToRead == 0 ) {
			if (source.settings.loop) {
				offset = 0;
				framesToRead = std::min(bufferFrames, totalFrames);
				bytesToRead = framesToRead * frameSize;
			} else {
				break;
			}
		}

		AL_CHECK_RESULT(alBufferData(source.streamBuffers.getIndex(queuedBuffers), clip->info.format, pcm16 + offset * clip->info.channels, (ALsizei)bytesToRead, clip->info.frequency));
		offset += framesToRead;
	}
	source.streamState.consumed = offset * frameSize;
	AL_CHECK_RESULT(alSourceQueueBuffers(source.alSource.getIndex(), queuedBuffers, &source.streamBuffers.getIndex(0)));

	if ( queuedBuffers >= source.settings.buffers ) {
		source.settings.loopMode = 1;
		source.alSource.set(AL_LOOPING, AL_FALSE);
	}
}

void ext::pcm::update( pod::AudioSource& source ) {
	pod::AudioClip* clip = source.clip;
	if ( !clip ) return;

	if ( source.settings.loopMode == 1 ) source.alSource.set(AL_LOOPING, AL_FALSE);

	ALint state;
	source.alSource.get(AL_SOURCE_STATE, state);
	if ( state != AL_PLAYING ) {
		if ( !source.settings.loop && source.streamState.consumed >= clip->info.size ) {
			return;
		}
		source.alSource.play();
	}

	ALint processed = 0;
	source.alSource.get(AL_BUFFERS_PROCESSED, processed);
	if ( processed <= 0 ) return;

	int16_t* pcm16 = (int16_t*) clip->stream.buffer;
	size_t frameSize = clip->info.channels * sizeof(int16_t);
	size_t totalFrames = clip->info.size / frameSize;
	size_t bufferFrames = uf::audio::bufferSize / frameSize;

	ALuint index;
	while ( processed-- ) {
		AL_CHECK_RESULT(alSourceUnqueueBuffers(source.alSource.getIndex(), 1, &index));

		size_t offset = source.streamState.consumed / frameSize;
		size_t framesToRead = std::min(bufferFrames, totalFrames - offset);
		size_t bytesToRead = framesToRead * frameSize;

		if ( framesToRead == 0 ) {
			if ( !source.settings.loop ) break;
			offset = 0;
			framesToRead = std::min(bufferFrames, totalFrames);
			bytesToRead = framesToRead * frameSize;
		}

		if ( framesToRead > 0 ) {
			AL_CHECK_RESULT(alBufferData(index, clip->info.format, pcm16 + offset * clip->info.channels, (ALsizei)bytesToRead, clip->info.frequency));
			AL_CHECK_RESULT(alSourceQueueBuffers(source.alSource.getIndex(), 1, &index));
			source.streamState.consumed = (offset + framesToRead) * frameSize;
		}
	}
	if ( source.settings.loopMode == 1 ) source.alSource.set(AL_LOOPING, AL_TRUE);
}

void ext::pcm::close( pod::AudioClip& clip ) {
	if ( !clip.stream.buffer ) return;
	free( clip.stream.buffer );
	clip.stream.buffer = NULL;
}

void ext::pcm::close( pod::AudioSource& source ) {
	pod::AudioClip* clip = source.clip;
	if ( !clip || !clip->streamed ) return;

	ALint queued;
	source.alSource.get(AL_BUFFERS_QUEUED, queued);
	while ( queued-- ) {
		ALuint buffer;
		AL_CHECK_RESULT(alSourceUnqueueBuffers(source.alSource.getIndex(), 1, &buffer));
	}
	source.streamBuffers.destroy();
	source.streamState.consumed = 0;
}

uf::stl::vector<int16_t> ext::pcm::convertTo16bit( const uf::stl::vector<float>& waveform ) {
	return ext::pcm::convertTo16bit( waveform.data(), waveform.size() );
}
uf::stl::vector<int16_t> ext::pcm::convertTo16bit( const float* data, size_t len ) {
	uf::stl::vector<int16_t> samples( len );
	for (size_t i = 0; i < len; ++i) {
		float sample = std::clamp(data[i], -1.0f, 1.0f);
		samples[i] = static_cast<int16_t>(sample * 32767.0f);
	}
	return samples;
}
#endif