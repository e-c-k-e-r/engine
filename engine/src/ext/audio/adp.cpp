#if UF_USE_ADP
#include <uf/config.h>
#include <uf/utils/memory/reader.h>
#include <uf/utils/memory/writer.h>
#include <uf/ext/audio/adp.h>
#include <uf/utils/io/vfs.h>
#if UF_USE_OPENAL
#include <uf/ext/openal/openal.h>
#endif
#if UF_USE_AICA
#include <uf/ext/aica/aica.h>
#endif

namespace {
	struct AdpHeader {
		char magic[4]; // "ADPF"
		uint32_t channels;
		uint32_t sampleRate;
		uint32_t totalSamples;
		uint32_t dataSize;
	};

	struct AdpState {
		int32_t predictor = 0;
		int32_t step = 127;
	};

	struct AdpVfsContext {
		AdpHeader header;
		pod::File file;
		size_t dataOffset = 0;
		AdpState state[2];

		~AdpVfsContext() {
			if ( file ) file.close( file.handle );
		}
	};

	static const int32_t YAMAHA_SCALE[8] = { 230, 230, 230, 230, 307, 409, 512, 614 };

	inline int16_t decode_sample(uint8_t nibble, AdpState& state) {
		int32_t d = nibble & 7;

		int32_t diff = state.step >> 3;
		if ( d & 1 ) diff += state.step >> 2;
		if ( d & 2 ) diff += state.step >> 1;
		if ( d & 4 ) diff += state.step;

		if ( nibble & 8 ) {
			state.predictor -= diff;
		} else {
			state.predictor += diff;
		}

		if ( state.predictor > 32767 ) state.predictor = 32767;
		else if ( state.predictor < -32768 ) state.predictor = -32768;

		state.step = (state.step * YAMAHA_SCALE[d]) >> 8;
		if ( state.step < 127 ) state.step = 127;
		if ( state.step > 24576 ) state.step = 24576;

		return (int16_t)state.predictor;
	}

	int fill_buffer( void* user_data, uint8_t* buffer, int req_bytes ) {
		pod::AudioSource* source = (pod::AudioSource*)user_data;
		pod::AudioClip* clip = source->clip;
		AdpVfsContext* ctx = (AdpVfsContext*)source->streamState.handle;

	#if UF_USE_AICA
		int bytesRead = 0;
		while ( bytesRead < req_bytes ) {
			size_t read = ctx->file.read(ctx->file.handle, buffer + bytesRead, req_bytes - bytesRead);
			if ( read == 0 ) {
				if ( source->settings.loop ) {
					ctx->file.seek(ctx->file.handle, ctx->dataOffset, SEEK_SET);
					continue;
				}
				break;
			}
			bytesRead += read;
		}

		int alignedBytes = (bytesRead + 31) & ~31;
		if ( alignedBytes > req_bytes ) alignedBytes = req_bytes;
		if ( alignedBytes > bytesRead ) {
			std::memset(buffer + bytesRead, 0, alignedBytes - bytesRead);
			bytesRead = alignedBytes;
		}

		return bytesRead;
	#else
		int16_t* pcmOut = (int16_t*)buffer;
		int totalPcmSamplesNeeded = req_bytes / sizeof(int16_t);
		int totalSamplesRead = 0;

		int channels = ctx->header.channels;
		uint8_t adpByte;

		while ( totalSamplesRead < totalPcmSamplesNeeded ) {
			if ( ctx->file.read(ctx->file.handle, &adpByte, 1) != 1 ) {
				if ( source->settings.loop ) {
					ctx->file.seek(ctx->file.handle, ctx->dataOffset, SEEK_SET);
					ctx->state[0] = {0, 127}; ctx->state[1] = {0, 127};
					continue;
				}
				break;
			}

			if ( channels == 1 ) {
				pcmOut[totalSamplesRead++] = decode_sample(adpByte & 0x0F, ctx->state[0]);
				if ( totalSamplesRead < totalPcmSamplesNeeded ) {
					pcmOut[totalSamplesRead++] = decode_sample((adpByte >> 4) & 0x0F, ctx->state[0]);
				}
			} else if ( channels == 2 ) {
				pcmOut[totalSamplesRead++] = decode_sample(adpByte & 0x0F, ctx->state[0]); // Left
				pcmOut[totalSamplesRead++] = decode_sample((adpByte >> 4) & 0x0F, ctx->state[1]); // Right
			}
		}
		return totalSamplesRead * sizeof(int16_t);
	#endif
	}

	bool decode( const AdpHeader& header, const uf::stl::vector<uint8_t>& rawAdp, pod::PCM& pcm ) {
		pcm.channels = header.channels;
		pcm.sampleRate = header.sampleRate;
		pcm.samples.resize(header.totalSamples * header.channels);

		AdpState decState[2] = { {0, 127}, {0, 127} };
		size_t sampleIdx = 0;

		uf::stl::reader reader(rawAdp, 0, rawAdp.size());
		const uint8_t* adpData = reader.read<uint8_t>(header.dataSize);
		if ( !adpData ) return false;

		if ( header.channels == 1 ) {
			for ( size_t i = 0; i < header.dataSize; ++i ) {
				pcm.samples[sampleIdx++] = decode_sample(adpData[i] & 0x0F, decState[0]);
				if ( sampleIdx < pcm.samples.size() ) {
					pcm.samples[sampleIdx++] = decode_sample((adpData[i] >> 4) & 0x0F, decState[0]);
				}
			}
		} else if ( header.channels == 2 ) {
			for ( size_t i = 0; i < header.dataSize; ++i ) {
				pcm.samples[sampleIdx++] = decode_sample(adpData[i] & 0x0F, decState[0]); // Left
				pcm.samples[sampleIdx++] = decode_sample((adpData[i] >> 4) & 0x0F, decState[1]); // Right
			}
		}
		return true;
	}
}

void ext::adp::load( pod::AudioClip& clip ) {
	AdpVfsContext* ctx = new AdpVfsContext();
	ctx->file = uf::vfs::open(clip.filename);
	if ( !ctx->file ) {
		UF_MSG_ERROR("ADP: Failed to open file: {}", clip.filename);
		delete ctx; return;
	}

	ctx->file.read(ctx->file.handle, &ctx->header, sizeof(AdpHeader));
	clip.info.channels = ctx->header.channels;
	clip.info.bitDepth = 4;
	clip.info.frequency = ctx->header.sampleRate;
	clip.info.size = ctx->header.dataSize;
	clip.info.duration = (double)ctx->header.totalSamples / ctx->header.sampleRate;


#if UF_USE_AICA
	clip.info.format = (ctx->header.channels == 2) ? AL_FORMAT_STEREO_ADPCM_SEGA : AL_FORMAT_MONO_ADPCM_SEGA;
#else
	clip.info.format = (ctx->header.channels == 2) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;
#endif

	UF_MSG_DEBUG("filename={}, streamed={}", clip.filename, clip.streamed);
	if ( !clip.streamed ) {
		uf::stl::vector<uint8_t> rawAdp(ctx->header.dataSize);
		ctx->file.read(ctx->file.handle, rawAdp.data(), ctx->header.dataSize);

	#if UF_USE_AICA
		clip.alBuffer.buffer(clip.info.format, rawAdp.data(), (ALsizei)rawAdp.size(), clip.info.frequency);
	#else
		pod::PCM pcm;
		if ( ::decode(ctx->header, rawAdp, pcm) ) {
			clip.alBuffer.buffer(clip.info.format, pcm.samples.data(), (ALsizei)(pcm.samples.size() * sizeof(int16_t)), clip.info.frequency);
		}
	#endif
	}

	delete ctx;
}

void ext::adp::open( pod::AudioSource& source ) {
	pod::AudioClip* clip = source.clip;
	if ( !clip || !clip->streamed ) return;

	source.streamBuffers.initialize(source.settings.buffers);

	AdpVfsContext* ctx = new AdpVfsContext();
	ctx->file = uf::vfs::open(clip->filename);
	if ( !ctx->file ) {
		UF_MSG_ERROR("ADP: Failed to open streaming file: {}", clip->filename);
		delete ctx; return;
	}

	ctx->file.read(ctx->file.handle, &ctx->header, sizeof(AdpHeader));
	ctx->dataOffset = sizeof(AdpHeader);
	ctx->state[0] = {0, 127}; ctx->state[1] = {0, 127};

	source.streamState.handle = (void*)ctx;

#if UF_USE_AICA
	source.streamBuffers.set( AL_STREAM_FILL_CALLBACK, (ALint*)(::fill_buffer) );
	source.streamBuffers.set( AL_STREAM_USER_DATA, (ALint*)(&source) );
	source.alSource.set(AL_BUFFER, (ALint)(source.streamBuffers.getIndex(0)));
	source.streamBuffers.buffer( clip->info.format, NULL, 0, clip->info.frequency );
#else
	uint8_t buffer[uf::audio::bufferSize];
	uint8_t queuedBuffers = 0;

	for ( ; queuedBuffers < source.settings.buffers; ++queuedBuffers ) {
		int totalRead = ::fill_buffer(&source, buffer, uf::audio::bufferSize);
		if ( totalRead == 0 ) break;

		ext::al::Buffer::buffer( source.streamBuffers.getIndex(queuedBuffers), clip->info.format, buffer, totalRead, clip->info.frequency);
	}
	source.alSource.queue( queuedBuffers, &source.streamBuffers.getIndex(0) );
#endif
}

void ext::adp::update( pod::AudioSource& source ) {
	pod::AudioClip* clip = source.clip;
	if ( !clip || !clip->streamed || !source.streamState.handle ) return;

#if UF_USE_AICA
	source.streamBuffers.poll();
#else
	AdpVfsContext* ctx = (AdpVfsContext*)source.streamState.handle;
	ALint state, processed, queued;

	source.alSource.get(AL_SOURCE_STATE, state);
	source.alSource.get(AL_BUFFERS_PROCESSED, processed);
	source.alSource.get(AL_BUFFERS_QUEUED, queued);

	bool hasData = (ctx->file.tell(ctx->file.handle) < (long)clip->info.size) || !source.info.pending.empty() || source.settings.loop;

	auto fillAndQueueBuffer = [&](ALuint index) -> bool {
		uint8_t buffer[uf::audio::bufferSize];
		int totalRead = ::fill_buffer(&source, buffer, uf::audio::bufferSize);

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
			if ( hasData ) fillAndQueueBuffer(index);
		}
	}

	if ( state != AL_PLAYING && queued > 0 ) {
		source.alSource.play();
	}
#endif
}

void ext::adp::close( pod::AudioClip& clip ) {}

void ext::adp::close( pod::AudioSource& source ) {
	if ( !source.clip || !source.clip->streamed ) return;

#if !UF_USE_AICA
	ALint queued;
	source.alSource.get(AL_BUFFERS_QUEUED, queued);
	while ( queued-- ) {
		ALuint buffer;
		source.alSource.unqueue( 1, &buffer );
	}
#endif

	source.streamBuffers.destroy();

	if ( source.streamState.handle ) {
		AdpVfsContext* ctx = (AdpVfsContext*)source.streamState.handle;
		delete ctx;
		source.streamState.handle = nullptr;
	}
}

bool ext::adp::decode( const uf::stl::string& filename, pod::PCM& pcm ) {
	pod::File file = uf::vfs::open(filename);
	if ( !file ) {
		UF_MSG_ERROR("ADP Decoder: Failed to open file: {}", filename);
		return false;
	}

	AdpHeader header;
	file.read(file.handle, &header, sizeof(AdpHeader));

	if ( std::strncmp(header.magic, "ADPF", 4) != 0 ) {
		UF_MSG_ERROR("ADP Decoder: Invalid file format magic in: {}", filename);
		file.close(file.handle);
		return false;
	}

	uf::stl::vector<uint8_t> rawAdp(header.dataSize);
	file.read(file.handle, rawAdp.data(), header.dataSize);
	file.close(file.handle);

	return ::decode(header, rawAdp, pcm);
}

#if UF_USE_ADP_ENCODER
namespace {
	uint8_t adp_encode_sample(int16_t sample, AdpState& state) {
		int32_t diff = sample - state.predictor;
		uint8_t nibble = 0;

		if (diff < 0) {
			nibble = 8;
			diff = -diff;
		}

		int32_t d = 0;
		int32_t temp_step = state.step;
		if (diff >= temp_step) {
			d |= 4;
			diff -= temp_step;
		}
		temp_step >>= 1;
		if (diff >= temp_step) {
			d |= 2;
			diff -= temp_step;
		}
		temp_step >>= 1;
		if (diff >= temp_step) {
			d |= 1;
		}

		nibble |= d;

		int32_t reconstructed_diff = state.step >> 3;
		if ( d & 1 ) reconstructed_diff += state.step >> 2;
		if ( d & 2 ) reconstructed_diff += state.step >> 1;
		if ( d & 4 ) reconstructed_diff += state.step;

		if (nibble & 8) {
			state.predictor -= reconstructed_diff;
		} else {
			state.predictor += reconstructed_diff;
		}

		if (state.predictor > 32767) state.predictor = 32767;
		else if (state.predictor < -32768) state.predictor = -32768;

		state.step = (state.step * YAMAHA_SCALE[d]) >> 8;
		if (state.step < 127) state.step = 127;
		if (state.step > 24576) state.step = 24576;

		return nibble;
	}
}

uf::stl::vector<uint8_t> ext::adp::encode( const pod::PCM& pcm ) {
	uf::stl::vector<uint8_t> output;
	if ( pcm.samples.empty() ) return output;

	AdpHeader header;
	uf::stl::memcpy(header.magic, "ADPF", 4);
	header.channels = pcm.channels;
	header.sampleRate = pcm.sampleRate;

	uint32_t totalFrames = (uint32_t)(pcm.samples.size() / pcm.channels);
	header.totalSamples = totalFrames;
	header.dataSize = (totalFrames * pcm.channels + 1) / 2;

	uf::stl::writer writer(output);
	writer.write(header);

	uint8_t* adpData = writer.reserve<uint8_t>(header.dataSize);

	AdpState state[2] = { {0, 127}, {0, 127} };

	if ( pcm.channels == 1 ) {
		for ( size_t i = 0; i < pcm.samples.size(); i += 2 ) {
			uint8_t n0 = ::adp_encode_sample(pcm.samples[i], state[0]);
			uint8_t n1 = (i + 1 < pcm.samples.size()) ? ::adp_encode_sample(pcm.samples[i + 1], state[0]) : 0;
			adpData[i / 2] = n0 | (n1 << 4);
		}
	} else if ( pcm.channels == 2 ) {
		for ( size_t i = 0; i < pcm.samples.size(); i += 2 ) {
			uint8_t left = ::adp_encode_sample(pcm.samples[i], state[0]);
			uint8_t right = ::adp_encode_sample(pcm.samples[i + 1], state[1]);
			adpData[i / 2] = left | (right << 4);
		}
	}

	return output;
}
#endif
#endif