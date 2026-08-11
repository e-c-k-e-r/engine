#include <uf/config.h>
#if UF_USE_PCM

#if UF_USE_OPENAL
#include <uf/ext/openal/openal.h>
#endif
#if UF_USE_AICA
#include <uf/ext/aica/aica.h>
#endif
#include <uf/utils/memory/reader.h>
#include <uf/utils/memory/writer.h>
#include <uf/ext/audio/pcm.h>
#include <uf/utils/memory/pool.h>
#include <cstdio>
#include <cstring>

namespace {
	namespace funs {
		int fill_buffer(void* user_data, uint8_t* buffer, int req_bytes) {
			pod::AudioSource* source = (pod::AudioSource*)user_data;
			pod::AudioClip* clip = source->clip;

			uint8_t* pcm_data = (uint8_t*)clip->stream.buffer;
			size_t frameSize = clip->info.channels * sizeof(int16_t);

			int totalRead = 0;
			while ( totalRead < req_bytes ) {
				size_t bytesLeft = clip->info.size - source->streamState.consumed;
				size_t bytesToCopy = std::min((size_t)(req_bytes - totalRead), bytesLeft);

				if ( bytesToCopy > 0 ) {
					uf::stl::memcpy(buffer + totalRead, pcm_data + source->streamState.consumed, bytesToCopy);
					totalRead += (int)bytesToCopy;
					source->streamState.consumed += bytesToCopy;
				}

				if ( source->streamState.consumed >= clip->info.size ) {
					if ( source->settings.loop ) {
						source->streamState.consumed = clip->info.loop.has ? (clip->info.loop.start * frameSize) : 0;
					} else {
						break;
					}
				}
			}

		#if UF_USE_AICA
			int alignedBytes = (totalRead + 31) & ~31;
			if ( alignedBytes > req_bytes ) alignedBytes = req_bytes;
			if ( alignedBytes > totalRead ) {
				std::memset(buffer + totalRead, 0, alignedBytes - totalRead);
				totalRead = alignedBytes;
			}
		#endif

			return totalRead;
		}
	}
}

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
		uf::stl::memcpy( clip.stream.buffer, pcm.samples.data(), clip.info.size );
	}
}

void ext::pcm::open( pod::AudioSource& source ) {
	pod::AudioClip* clip = source.clip;
	if ( !clip || !clip->stream.buffer ) return;

	source.streamBuffers.initialize( source.settings.buffers );
	source.streamState.consumed = 0;

#if UF_USE_AICA
	source.streamBuffers.set( AL_STREAM_FILL_CALLBACK, (ALint)(funs::fill_buffer) );
	source.streamBuffers.set( AL_STREAM_USER_DATA, (ALint*)(&source) );
	source.streamBuffers.buffer( clip->info.format, NULL, 0, clip->info.frequency );
	source.alSource.set(AL_BUFFER, (ALint)(source.streamBuffers.getIndex(0)));
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

void ext::pcm::update( pod::AudioSource& source ) {
	pod::AudioClip* clip = source.clip;
	if ( !clip || !clip->streamed || !clip->stream.buffer ) return;

#if UF_USE_AICA
	ALint hnd_val = 0;
	source.streamBuffers.get( AL_STREAM_HANDLE, hnd_val );
	snd_stream_hnd_t hnd = (snd_stream_hnd_t)hnd_val;

	if ( hnd == SND_STREAM_INVALID ) return;

	ALint state;
	source.alSource.get(AL_SOURCE_STATE, state);
	if ( state == AL_PLAYING ) {
		int poll_result = snd_stream_poll(hnd);
		if ( poll_result < 0 ) UF_MSG_ERROR("[AICA] snd_stream_poll({}) failed with code: {}", hnd, poll_result);
	}
#else
	ALint state, processed, queued;
	source.alSource.get(AL_SOURCE_STATE, state);
	source.alSource.get(AL_BUFFERS_PROCESSED, processed);
	source.alSource.get(AL_BUFFERS_QUEUED, queued);

	bool hasData = (source.streamState.consumed < clip->info.size) || source.settings.loop;

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

			hasData = (source.streamState.consumed < clip->info.size) || source.settings.loop;
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
		source.alSource.unqueue( 1, &buffer );
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