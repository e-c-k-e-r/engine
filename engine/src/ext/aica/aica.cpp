#include <uf/config.h>

#if UF_ENV_DREAMCAST && UF_USE_AICA
#include <uf/ext/aica/aica.h>
#include <kos.h>
#include <dc/sound/sound.h>
#include <dc/sound/sfxmgr.h>
#include <uf/utils/memory/pool.h>
#include <uf/utils/string/io.h>
#include <uf/utils/audio/audio.h>
#include <math.h>
#include <kos/mutex.h>
#include <dc/cache.h>
#include <dc/sound/aica_comm.h>

typedef struct snd_effect {
	uint32_t  locl, locr;
	uint32_t  len;
	uint32_t  rate;
	uint32_t  used;
	uint32_t  fmt;
	uint16_t  stereo;

	LIST_ENTRY(snd_effect)  list;
} snd_effect_t;

#define UF_AICA_THREADED 0

namespace impl {
	typedef int (*fill_callback_t)(void* user_data, uint8_t* buffer, int req_bytes);

	constexpr int STREAM_BUFFER_SIZE = 65536;
	constexpr int DRIVER_BUFFER_SIZE = 65536;
	constexpr size_t MAX_VIRTUAL_SOURCES = 256;
	constexpr size_t MAX_VIRTUAL_BUFFERS = 1024;
	constexpr size_t MAX_QUEUE_SIZE = 16;

	struct Source {
		bool active = false;

		pod::Vector3f position = {0, 0, 0};
		float gain = 1.0f;
		float pitch = 1.0f;
		float max_distance = 100.0f;
		bool looping = false;
		int state = AL_STOPPED;
		ALuint buffer_id = 0;

		int hw_channel = -1;

		ALuint queue[MAX_QUEUE_SIZE];
		size_t queue_head = 0;
		size_t queue_tail = 0;
		size_t queue_count = 0;
	};

	struct Buffer {
		bool active = false;
		uint32_t aica_addr = 0;
		snd_stream_hnd_t stream_hnd = SND_STREAM_INVALID;

		snd_effect_t* sfx_effect = nullptr;

		void* user_data = nullptr;
		uint8_t* stream_buffer = nullptr;
		impl::fill_callback_t fill_callback = nullptr;

		ALenum format = 0;
		ALsizei size = 0;
		ALsizei frequency = 0;
	};

	struct Listener {
		pod::Vector3f position = {0, 0, 0};
		pod::Vector3f right = {1, 0, 0};
	} listener;

	struct AicaFormatInfo {
		uint16_t channels = 1;
		uint32_t aica_fmt = 0;
		uint32_t sample_len = 0;
	};

	struct SpatialParams {
		int vol = 0;
		int left_pan = 127;
		int right_pan = 127;
	};

	impl::Source sources[impl::MAX_VIRTUAL_SOURCES];
	impl::Buffer buffers[impl::MAX_VIRTUAL_BUFFERS];
	std::unordered_map<snd_stream_hnd_t, impl::Buffer*> g_activeStreams;
	mutex_t g_streamMutex = MUTEX_INITIALIZER;

	void* kos_stream_callback( snd_stream_hnd_t hnd, int req_bytes, int* bytes_returned ) {
		auto it = g_activeStreams.find(hnd);
		if ( it == g_activeStreams.end() ) {
			*bytes_returned = 0;
			UF_MSG_DEBUG("[AICA] No active stream found: {}", hnd);
			return nullptr;
		}

		impl::Buffer* buf = it->second;
		if ( req_bytes > impl::STREAM_BUFFER_SIZE ) {
			req_bytes = impl::STREAM_BUFFER_SIZE;
		}

		if ( buf->fill_callback && buf->user_data ) {
			*bytes_returned = buf->fill_callback(buf->user_data, buf->stream_buffer, req_bytes);
		} else {
			*bytes_returned = 0;
		}

		if ( *bytes_returned == 0 ) return nullptr;

		return buf->stream_buffer;
	}

	uint32_t getSourceIndex() {
		for ( size_t i = 1; i < impl::MAX_VIRTUAL_SOURCES; ++i ) {
			if ( !impl::sources[i].active ) {
				impl::sources[i] = {
					.active = true,
				};
				UF_MSG_DEBUG("Assigning Source index: {}", i);
				return i;
			}
		}
		return 0;
	}

	uint32_t getBufferIndex() {
		for ( size_t i = 1; i < impl::MAX_VIRTUAL_BUFFERS; ++i ) {
			if ( !impl::buffers[i].active ) {
				impl::buffers[i] = {
					.active = true,
				};
				UF_MSG_DEBUG("Assigning Buffer index: {}", i);
				return i;
			}
		}
		return 0;
	}

	pod::Vector3f spatial( const impl::Source& source, impl::Listener& listener ) {
		if ( !uf::vector::isValid( source.position ) || !uf::vector::isValid( listener.position ) ) {
			return { 0, 0, 0 };
		}
		pod::Vector3f d = source.position - listener.position;
		float dist = uf::vector::norm( d );
		if ( dist > EPS ) d /= dist;

		float pan_factor = 0.0f;
		if ( dist > 0.001f ) {
			pan_factor = uf::vector::dot( listener.right, d );
		}

		int pan = (int)((pan_factor + 1.0f) * 127.5f);
		if ( pan < 0 ) pan = 0;
		if ( pan > 255 ) pan = 255;

		return pod::Vector3f{ dist, (float)pan, 0.0f };
	}

	SpatialParams calculateSpatial(const impl::Source& src) {
		SpatialParams params;
		pod::Vector3f spatial_data = impl::spatial(src, impl::listener);

		float dist = spatial_data[0];
		params.left_pan = (int)spatial_data[1];
		params.right_pan = (int)spatial_data[2];

		float gain_factor = 1.0f;
		if ( dist > 0.0f && src.max_distance > 0.0f ) {
			gain_factor = 1.0f - (dist / src.max_distance);
			if ( gain_factor < 0.0f ) gain_factor = 0.0f;
		}

		params.vol = (int)(255.0f * src.gain * gain_factor);
		params.vol = std::max(0, std::min(255, params.vol));
		return params;
	}

	bool isPlaying(const impl::Source& src, const impl::Buffer& buffer) {
		if ( buffer.stream_hnd != SND_STREAM_INVALID ) {
			return snd_stream_poll(buffer.stream_hnd) >= 0;
		}
		if ( src.hw_channel != -1 ) {
			return snd_is_playing(src.hw_channel);
		}
		return false;
	}

	void freeSource(impl::Source& src, impl::Buffer& buffer) {
		src.state = AL_STOPPED;
		if ( buffer.stream_hnd != SND_STREAM_INVALID ) {
			snd_stream_stop(buffer.stream_hnd);
		} else if ( src.hw_channel != -1 ) {
			snd_sfx_chn_free(src.hw_channel);
			if ( buffer.sfx_effect && buffer.sfx_effect->stereo ) {
				snd_sfx_chn_free(src.hw_channel + 1);
			}
			src.hw_channel = -1;
		}
	}

	void sendAicaCommand(int channel, int vol, int pan) {
		AICA_CMDSTR_CHANNEL(tmp, cmd, chan);
		cmd->cmd = AICA_CMD_CHAN;
		cmd->timestamp = 0;
		cmd->size = AICA_CMDSTR_CHANNEL_SIZE;
		cmd->cmd_id = channel;

		chan->cmd = AICA_CH_CMD_UPDATE | AICA_CH_UPDATE_SET_VOL | AICA_CH_UPDATE_SET_PAN;
		chan->vol = vol;
		chan->pan = pan;

		snd_sh4_to_aica(tmp, cmd->size);
	}

	snd_stream_hnd_t allocateStreamSlot() {
		snd_stream_hnd_t hnd = snd_stream_alloc( impl::kos_stream_callback, impl::DRIVER_BUFFER_SIZE );
		if ( hnd != SND_STREAM_INVALID ) return hnd;

		impl::Source* victim_src = nullptr;
		impl::Buffer* victim_buf = nullptr;

		for ( size_t i = 1; i < impl::MAX_VIRTUAL_SOURCES; ++i ) {
			impl::Source& src = impl::sources[i];
			if ( src.active && src.state == AL_PLAYING && src.buffer_id > 0 ) {
				impl::Buffer& buf = impl::buffers[src.buffer_id];
				if ( buf.stream_hnd != SND_STREAM_INVALID ) {
					victim_src = &src;
					victim_buf = &buf;
					break;
				}
			}
		}

		if ( victim_buf && victim_src ) {
			UF_MSG_DEBUG("[AICA] Evicting active stream handle {} to release hardware slot", victim_buf->stream_hnd);
			snd_stream_stop( victim_buf->stream_hnd );

			mutex_lock(&impl::g_streamMutex);
			impl::g_activeStreams.erase(victim_buf->stream_hnd);
			mutex_unlock(&impl::g_streamMutex);

			snd_stream_destroy( victim_buf->stream_hnd );
			victim_buf->stream_hnd = SND_STREAM_INVALID;
			victim_src->state = AL_STOPPED;

			hnd = snd_stream_alloc( impl::kos_stream_callback, impl::DRIVER_BUFFER_SIZE );
		}

		return hnd;
	}

	AicaFormatInfo getAicaFormatInfo(ALenum format, ALsizei size) {
		AicaFormatInfo info;
		info.channels = (format == AL_FORMAT_STEREO8 || format == AL_FORMAT_STEREO16 || format == AL_FORMAT_STEREO_ADPCM_SEGA) ? 2 : 1;

		if (format == AL_FORMAT_MONO_ADPCM_SEGA || format == AL_FORMAT_STEREO_ADPCM_SEGA) {
			info.aica_fmt = AICA_SM_ADPCM;
			info.sample_len = (size * 2) / info.channels;
		} else if (format == AL_FORMAT_MONO8 || format == AL_FORMAT_STEREO8) {
			info.aica_fmt = AICA_SM_8BIT;
			info.sample_len = size / info.channels;
		} else {
			info.aica_fmt = AICA_SM_16BIT;
			info.sample_len = (size / 2) / info.channels;
		}
		return info;
	}

	template<typename T>
	void deinterleaveLoad(const void* data, size_t sample_len, uint32_t locl, uint32_t locr, uint32_t left_size) {
		T* left_temp = (T*)memalign(32, left_size);
		T* right_temp = (T*)memalign(32, left_size);

		if ( left_temp && right_temp ) {
			std::memset(left_temp, 0, left_size);
			std::memset(right_temp, 0, left_size);

			const T* src = (const T*)data;
			for ( size_t s = 0; s < sample_len; ++s ) {
				left_temp[s] = src[s * 2];
				right_temp[s] = src[s * 2 + 1];
			}

			spu_memload(locl, left_temp, left_size);
			spu_memload(locr, right_temp, left_size);
		}

		if ( left_temp ) free(left_temp);
		if ( right_temp ) free(right_temp);
	}

	void stereoToSpu(ALenum format, const void* data, ALsizei size, size_t sample_len, uint32_t locl, uint32_t locr, uint32_t left_size) {
		if ( format == AL_FORMAT_STEREO_ADPCM_SEGA ) {
			uint32_t* left_temp = (uint32_t*)memalign(32, left_size);
			uint32_t* right_temp = (uint32_t*)memalign(32, left_size);

			if ( left_temp && right_temp ) {
				std::memset(left_temp, 0, left_size);
				std::memset(right_temp, 0, left_size);
				snd_adpcm_split((uint32_t*)data, left_temp, right_temp, size);
				spu_memload(locl, left_temp, left_size);
				spu_memload(locr, right_temp, left_size);
			}
			if ( left_temp ) free(left_temp);
			if ( right_temp ) free(right_temp);
		} else if ( format == AL_FORMAT_STEREO16 ) {
			deinterleaveLoad<int16_t>(data, sample_len, locl, locr, left_size);
		} else if ( format == AL_FORMAT_STEREO8 ) {
			deinterleaveLoad<uint8_t>(data, sample_len, locl, locr, left_size);
		}
	}

	void destroyBuffer(impl::Buffer& buffer) {
		if ( buffer.sfx_effect ) {
			free(buffer.sfx_effect);
			buffer.sfx_effect = nullptr;
		}
		if ( buffer.aica_addr ) {
			snd_mem_free(buffer.aica_addr);
			buffer.aica_addr = 0;
		}
		if ( buffer.stream_hnd != SND_STREAM_INVALID ) {
			snd_stream_destroy(buffer.stream_hnd);
			buffer.stream_hnd = SND_STREAM_INVALID;
		}
	}

	void setupStreamBuffer(impl::Buffer& buffer) {
		if ( !buffer.stream_buffer ) {
			buffer.stream_buffer = (uint8_t*)(memalign(32, impl::STREAM_BUFFER_SIZE));
		}
		if ( !buffer.stream_buffer ) {
			UF_MSG_ERROR("[AICA] Failed to allocate {} bytes", impl::STREAM_BUFFER_SIZE);
			return;
		}
		buffer.stream_hnd = impl::allocateStreamSlot();
		if ( buffer.stream_hnd == SND_STREAM_INVALID ) {
			UF_MSG_ERROR("[AICA] Failed to allocate streaming slot context");
			return;
		}
		mutex_lock(&impl::g_streamMutex);
		impl::g_activeStreams[buffer.stream_hnd] = &buffer;
		mutex_unlock(&impl::g_streamMutex);
	}

	void loadStaticBuffer(impl::Buffer& buffer, ALuint index, ALenum format, const void* data, ALsizei size, ALsizei frequency) {
		auto info = getAicaFormatInfo(format, size);

		buffer.sfx_effect = (snd_effect_t*)malloc(sizeof(snd_effect_t));
		if ( !buffer.sfx_effect ) {
			UF_MSG_ERROR("[AICA] Out of host memory for static sound descriptor");
			return;
		}
		std::memset(buffer.sfx_effect, 0, sizeof(snd_effect_t));
		buffer.sfx_effect->rate = frequency;
		buffer.sfx_effect->stereo = (info.channels > 1);
		buffer.sfx_effect->fmt = info.aica_fmt;
		buffer.sfx_effect->len = info.sample_len;

		uint32_t alloc_size = size;
		uint32_t left_size = size;
		uint32_t right_offset = 0;

		if ( info.channels > 1 ) {
			uint32_t half_size = size / 2;
			left_size = (half_size + 31) & ~31;
			alloc_size = left_size * 2;
			right_offset = left_size;
		}

		uint32_t aica_addr = snd_mem_malloc(alloc_size);
		if ( !aica_addr ) {
			free(buffer.sfx_effect);
			buffer.sfx_effect = nullptr;
			UF_MSG_ERROR("[AICA] Failed to allocate SPU RAM ({} bytes)", alloc_size);
			return;
		}

		buffer.aica_addr = aica_addr;
		buffer.sfx_effect->locl = aica_addr;
		if ( info.channels > 1 ) {
			buffer.sfx_effect->locr = aica_addr + right_offset;
			stereoToSpu(format, data, size, info.sample_len, buffer.sfx_effect->locl, buffer.sfx_effect->locr, left_size);
		} else {
			spu_memload(aica_addr, (void*)data, size);
		}

		UF_MSG_DEBUG("[AICA] Static Buffer ID {} allocated {} bytes at SPU addr 0x{:X}", index, alloc_size, aica_addr);
	}

	void update( impl::Source& src, impl::Buffer& buffer ) {
		if ( !isPlaying(src, buffer) ) {
			freeSource(src, buffer);
			return;
		}

		auto params = calculateSpatial(src);
		if ( buffer.stream_hnd != SND_STREAM_INVALID ) {
			snd_stream_volume(buffer.stream_hnd, params.vol);
			snd_stream_pan(buffer.stream_hnd, params.left_pan, params.right_pan);
		} else if ( src.hw_channel != -1 ) {
			bool is_stereo = (buffer.sfx_effect && buffer.sfx_effect->stereo);
			int left_pan = is_stereo ? 0 : params.left_pan;

			sendAicaCommand(src.hw_channel, params.vol, left_pan);

			if ( is_stereo ) {
				sendAicaCommand(src.hw_channel + 1, params.vol, 255);
			}
		}
	}

	void update( impl::Source& src ) {
		if ( !src.active || src.state != AL_PLAYING ) return;
		if ( 0 < src.buffer_id && src.buffer_id < impl::MAX_VIRTUAL_BUFFERS ) {
			update( src, impl::buffers[src.buffer_id] );
		}
	}

	void update( impl::Buffer& buffer, ALuint buffer_id ) {
		for ( size_t i = 1; i < impl::MAX_VIRTUAL_SOURCES; ++i ) {
			impl::Source& src = impl::sources[i];
			if ( src.active && src.buffer_id == buffer_id ) {
				update( src, buffer );
			}
		}
	}

	void update() {
		for ( size_t i = 1; i < impl::MAX_VIRTUAL_SOURCES; ++i ) {
			update( impl::sources[i] );
		}
	}

#if UF_AICA_THREADED
	kthread_t* thread_handle = nullptr;

	void* audio_thread( void* param ) {
		while ( true ) {
			mutex_lock( &impl::g_streamMutex );
			impl::update();
			mutex_unlock( &impl::g_streamMutex );
			thd_sleep(20);
		}
		return NULL;
	}
#endif
}

void ext::al::initialize() {
	snd_init();
	snd_stream_init();
#if UF_AICA_THREADED
	impl::thread_handle = thd_create(0, impl::audio_thread, NULL);
#endif

	UF_MSG_DEBUG("Initialized AICA.");
}

void ext::al::destroy() {
	snd_shutdown();
}

uf::stl::string ext::al::getError( ALenum error ) {
	return "AL_NO_ERROR";
}

void ext::al::Listener::set( ALenum name, ALfloat x, ALfloat y, ALfloat z ) {
	if ( name == AL_POSITION ) {
		impl::listener.position = { x, y, z };
	}
}

void ext::al::Listener::set( ALenum name, const ALfloat* values ) {
	if ( name == AL_ORIENTATION ) {
		pod::Vector3f forward{values[0], values[1], values[2]};
		pod::Vector3f up{values[3], values[4], values[5]};

		impl::listener.right = uf::vector::normalize(uf::vector::cross(forward, up));
	}
}

pod::Vector3f ext::al::Listener::getPosition() {
	return impl::listener.position;
}
pod::Vector3f ext::al::Listener::getRight() {
	return impl::listener.right;
}

void ext::al::Source::initialize() {
	if ( this->m_index ) this->destroy();
	this->m_index = impl::getSourceIndex();
}

void ext::al::Source::destroy() {
	if ( 0 < this->m_index && this->m_index < impl::MAX_VIRTUAL_SOURCES ) {
		impl::Source& src = impl::sources[this->m_index];
		src.active = false;
	}
	this->m_index = 0;
}

ALuint& ext::al::Source::getIndex() { return this->m_index; }
ALuint ext::al::Source::getIndex() const { return this->m_index; }

void ext::al::Source::set( ALenum name, ALfloat x, ALfloat y, ALfloat z ) {
	if ( !this->m_index ) return;
	impl::Source& src = impl::sources[this->m_index];
	if ( name == AL_POSITION ) {
		src.position = { x, y, z };
	}
}

void ext::al::Source::set( ALenum name, ALfloat x ) {
	if ( !this->m_index ) return;
	impl::Source& src = impl::sources[this->m_index];
	if ( name == AL_GAIN ) src.gain = x;
	else if ( name == AL_MAX_DISTANCE ) src.max_distance = x;
	else if ( name == AL_PITCH ) src.pitch = x;
}

void ext::al::Source::set( ALenum name, ALint x ) {
	if ( !this->m_index ) return;
	impl::Source& src = impl::sources[this->m_index];
	if ( name == AL_SOURCE_STATE ) src.state = (int)(x);
	else if ( name == AL_LOOPING ) src.looping = (x != 0);
	else if ( name == AL_BUFFER ) src.buffer_id = x;
}

void ext::al::Source::get( ALenum name, ALfloat& x ) const {
	x = 0.0f;
	if ( !this->m_index ) return;
	if ( name == AL_GAIN ) x = impl::sources[this->m_index].gain;
	else if ( name == AL_PITCH ) x = impl::sources[this->m_index].pitch;
	else if ( name == AL_MAX_DISTANCE ) x = impl::sources[this->m_index].max_distance;
}

void ext::al::Source::get( ALenum name, ALint& x ) const {
	x = 0;
	if ( !this->m_index ) return;
#if !UF_AICA_THREADED
	impl::update( impl::sources[this->m_index] );
#endif
	if ( name == AL_SOURCE_STATE ) x = impl::sources[this->m_index].state;
	else if ( name == AL_LOOPING ) x = impl::sources[this->m_index].looping ? 1 : 0;
	else if ( name == AL_BUFFER ) x = impl::sources[this->m_index].buffer_id;
}

void ext::al::Source::get( ALenum name, ALint* f ) const {
	if ( f ) this->get( name, f[0] );
}

void ext::al::Source::set( ALenum name, ALint x, ALint y, ALint z ) {}

void ext::al::Source::set( const uf::stl::string& string, ALfloat x, ALfloat y, ALfloat z ) {
	if ( string == "POSITION" ) this->set( AL_POSITION, x, y, z );
}
void ext::al::Source::set( const uf::stl::string& string, ALfloat x ) {
	if ( string == "GAIN" ) this->set( AL_GAIN, x );
	if ( string == "MAX_DISTANCE" ) this->set( AL_MAX_DISTANCE, x );
	if ( string == "PITCH" ) this->set( AL_PITCH, x );
}
void ext::al::Source::set( const uf::stl::string& string, ALint x ) {
	if ( string == "SOURCE_STATE" && this->m_index ) this->set( AL_SOURCE_STATE, x );
	else if ( string == "LOOPING" ) this->set( AL_LOOPING, x );
	else if ( string == "BUFFER" ) this->set( AL_BUFFER, x );
}

void ext::al::Source::play() {
	if ( !this->m_index ) return;
	impl::Source& src = impl::sources[this->m_index];
	src.state = AL_PLAYING;

	if ( 0 < src.buffer_id && src.buffer_id < impl::MAX_VIRTUAL_BUFFERS ) {
		impl::Buffer& buffer = impl::buffers[src.buffer_id];

		pod::Vector3f spatial_data = impl::spatial(src, impl::listener);
		float dist = spatial_data[0];
		int left_pan = (int)spatial_data[1];

		float gain_factor = 1.0f;
		if ( dist > 0.0f && src.max_distance > 0.0f ) {
			gain_factor = 1.0f - (dist / src.max_distance);
			if ( gain_factor < 0.0f ) gain_factor = 0.0f;
		}
		int vol = (int)(255.0f * src.gain * gain_factor);
		if ( vol < 0 ) vol = 0;
		if ( vol > 255 ) vol = 255;

		if ( buffer.aica_addr != 0 && buffer.sfx_effect ) {
			sfx_play_data_t play_data = {0};
			play_data.chn = -1;
			play_data.idx = (sfxhnd_t)buffer.sfx_effect;
			play_data.vol = vol;
			play_data.pan = buffer.sfx_effect->stereo ? 0 : left_pan;
			play_data.loop = src.looping ? 1 : 0;
			play_data.loopstart = 0;
			play_data.loopend = buffer.sfx_effect->len;
			play_data.freq = buffer.sfx_effect->rate;

			src.hw_channel = snd_sfx_play_ex(&play_data);
		} else if ( buffer.stream_hnd != SND_STREAM_INVALID ) {
			bool is_stereo = (buffer.format == AL_FORMAT_STEREO16 || buffer.format == AL_FORMAT_STEREO8 || buffer.format == AL_FORMAT_STEREO_ADPCM_SEGA);
			int st = is_stereo ? 1 : 0;

			mutex_lock(&impl::g_streamMutex);
			snd_stream_reinit(buffer.stream_hnd, impl::kos_stream_callback);

			if ( buffer.format == AL_FORMAT_MONO_ADPCM_SEGA || buffer.format == AL_FORMAT_STEREO_ADPCM_SEGA ) {
				snd_stream_start_adpcm(buffer.stream_hnd, buffer.frequency, st);
			} else if ( buffer.format == AL_FORMAT_MONO8 || buffer.format == AL_FORMAT_STEREO8 ) {
				snd_stream_start_pcm8(buffer.stream_hnd, buffer.frequency, st);
			} else {
				snd_stream_start(buffer.stream_hnd, buffer.frequency, st);
			}

			snd_stream_volume(buffer.stream_hnd, vol);
			mutex_unlock(&impl::g_streamMutex);
		}
	}
}

void ext::al::Source::pause() {
	if ( !this->m_index ) return;
	impl::Source& src = impl::sources[this->m_index];
	src.state = AL_PAUSED;

	if ( 0 < src.buffer_id && src.buffer_id < impl::MAX_VIRTUAL_BUFFERS ) {
		impl::Buffer& buffer = impl::buffers[src.buffer_id];

		if ( buffer.aica_addr != 0 && src.hw_channel != -1 ) {
			snd_sfx_stop(src.hw_channel);
			if ( buffer.sfx_effect && buffer.sfx_effect->stereo ) {
				snd_sfx_stop(src.hw_channel + 1);
			}
		} else if ( buffer.stream_hnd != SND_STREAM_INVALID ) {
			snd_stream_stop(buffer.stream_hnd);
		}
	}
}

void ext::al::Source::stop() {
	if (!this->m_index) return;
	impl::Source& src = impl::sources[this->m_index];
	src.state = AL_STOPPED;

	if ( 0 < src.buffer_id && src.buffer_id < impl::MAX_VIRTUAL_BUFFERS ) {
		impl::Buffer& buffer = impl::buffers[src.buffer_id];

		if ( buffer.aica_addr != 0 && src.hw_channel != -1 ) {
			snd_sfx_stop(src.hw_channel);
			snd_sfx_chn_free(src.hw_channel);
			if ( buffer.sfx_effect && buffer.sfx_effect->stereo ) {
				snd_sfx_stop(src.hw_channel + 1);
				snd_sfx_chn_free(src.hw_channel + 1);
			}
			src.hw_channel = -1;
		} else if ( buffer.stream_hnd != SND_STREAM_INVALID ) {
			snd_stream_stop(buffer.stream_hnd);
		}
	}
}

bool ext::al::Source::playing() const {
	if ( !this->m_index ) return false;
#if !UF_AICA_THREADED
	impl::update( impl::sources[this->m_index] );
#endif
	return impl::sources[this->m_index].state == AL_PLAYING;
}

void ext::al::Source::queue( ALsizei n, ALuint* indices ) {
	if ( !this->m_index ) return;
	impl::Source& src = impl::sources[this->m_index];
	for ( ALsizei i = 0; i < n; ++i ) {
		if ( src.queue_count < impl::MAX_QUEUE_SIZE ) {
			src.queue[src.queue_tail] = indices[i];
			src.queue_tail = (src.queue_tail + 1) % impl::MAX_QUEUE_SIZE;
			src.queue_count++;
		}
	}
	if ( src.state == AL_STOPPED && src.queue_count > 0 ) {
		src.buffer_id = src.queue[src.queue_head];
	}
}

void ext::al::Source::unqueue( ALsizei n, ALuint* indices ) {
	if ( !this->m_index ) return;
	impl::Source& src = impl::sources[this->m_index];
	for ( ALsizei i = 0; i < n; ++i ) {
		if ( src.queue_count > 0 ) {
			indices[i] = src.queue[src.queue_head];
			src.queue_head = (src.queue_head + 1) % impl::MAX_QUEUE_SIZE;
			src.queue_count--;
		}
	}
}

bool ext::al::Buffer::initialized() const {
	return !this->m_indices.empty();
}

void ext::al::Buffer::initialize( size_t size ) {
	if ( this->initialized() ) this->destroy();
	this->m_indices.resize(size, 0);
	for ( size_t i = 0; i < size; ++i ) {
		this->m_indices[i] = impl::getBufferIndex();
	}
}

void ext::al::Buffer::destroy() {
	if ( this->initialized() ) {
		for ( size_t i = 0; i < this->m_indices.size(); i++ ) {
			ALuint index = this->m_indices[i];
			if ( 0 < index && index < impl::MAX_VIRTUAL_BUFFERS ) {
				impl::Buffer& buffer = impl::buffers[index];

				if ( buffer.sfx_effect ) {
					free(buffer.sfx_effect);
					buffer.sfx_effect = nullptr;
				}

				if ( buffer.aica_addr ) {
					snd_mem_free( buffer.aica_addr );
				} else if ( buffer.stream_hnd != SND_STREAM_INVALID ) {
					mutex_lock(&impl::g_streamMutex);
					impl::g_activeStreams.erase(buffer.stream_hnd);
					mutex_unlock(&impl::g_streamMutex);

					snd_stream_destroy( buffer.stream_hnd );
				}

				if ( buffer.stream_buffer ) {
					free(buffer.stream_buffer);
					buffer.stream_buffer = nullptr;
				}

				buffer.active = false;
				buffer.aica_addr = 0;
				buffer.stream_hnd = SND_STREAM_INVALID;
				buffer.fill_callback = nullptr;
				buffer.user_data = nullptr;
			}
		}
	}
	this->m_indices.clear();
}

ALuint& ext::al::Buffer::getIndex( size_t i ) { return this->m_indices[i]; }
ALuint ext::al::Buffer::getIndex( size_t i ) const { return this->m_indices[i]; }

void ext::al::Buffer::poll( size_t i ) {
#if !UF_AICA_THREADED
	if ( !this->initialized() ) return;
	ALuint index = this->m_indices[i];
	if ( !(0 < index && index < impl::MAX_VIRTUAL_BUFFERS) ) return;
	mutex_lock( &impl::g_streamMutex );
	impl::update( impl::buffers[index], index );
	mutex_unlock( &impl::g_streamMutex );
#endif
}

void ext::al::Buffer::set( ALenum name, ALint value, size_t i ) {
	if ( !this->initialized() ) return;
	ALuint index = this->m_indices[i];
	if ( 0 < index && index < impl::MAX_VIRTUAL_BUFFERS ) {
		impl::Buffer& buffer = impl::buffers[index];

		if ( name == AL_STREAM_HANDLE ) buffer.stream_hnd = (snd_stream_hnd_t)(value);
	}
}

void ext::al::Buffer::get( ALenum name, ALint& value, size_t i ) const {
	if ( !this->initialized() ) return;
	ALuint index = this->m_indices[i];

	if ( 0 < index && index < impl::MAX_VIRTUAL_BUFFERS ) {
		const impl::Buffer& buffer = impl::buffers[index];

		if ( name == AL_STREAM_HANDLE ) value = (ALint)(buffer.stream_hnd);
	}
}

void ext::al::Buffer::set( ALenum name, ALint* iv, size_t i ) {
	if ( !this->initialized() ) this->initialize(1);
	ALuint index = this->m_indices[i];

	if ( 0 < index && index < impl::MAX_VIRTUAL_BUFFERS ) {
		impl::Buffer& buffer = impl::buffers[index];
		if ( name == AL_STREAM_FILL_CALLBACK ) buffer.fill_callback = (impl::fill_callback_t)(iv);
		else if ( name == AL_STREAM_USER_DATA ) buffer.user_data = (void*)(iv);
	}
}

void ext::al::Buffer::buffer( ALenum format, const ALvoid* data, ALsizei size, ALsizei frequency, size_t i ) {
	if ( !this->initialized() ) this->initialize();
	ext::al::Buffer::buffer( this->m_indices[i], format, data, size, frequency );
}
void ext::al::Buffer::buffer( ALuint index, ALenum format, const ALvoid* data, ALsizei size, ALsizei frequency ) {
	impl::Buffer& buffer = impl::buffers[index];

	impl::destroyBuffer(buffer);

	buffer.format = format;
	buffer.frequency = frequency;
	buffer.size = size;

	if ( buffer.fill_callback ) {
		impl::setupStreamBuffer(buffer);
		return;
	}

	if ( data && size ) {
		impl::loadStaticBuffer(buffer, index, format, data, size, frequency);
	} else {
		UF_EXCEPTION("[AICA] invalid invocation");
	}
}

// stubbed
void ext::al::Filter::initialize() {}
void ext::al::Filter::destroy() {}
ALuint ext::al::Filter::getIndex() const { return 0; }
void ext::al::Filter::set( ALenum name, ALfloat x ) {}
void ext::al::Filter::set( ALenum name, ALint x ) {}

void ext::al::Effect::initialize() {}
void ext::al::Effect::destroy() {}
ALuint ext::al::Effect::getIndex() const { return 0; }
void ext::al::Effect::set( ALenum name, ALfloat x ) {}
void ext::al::Effect::set( ALenum name, ALint x ) {}

void ext::al::EffectSlot::initialize() {}
void ext::al::EffectSlot::destroy() {}
ALuint ext::al::EffectSlot::getIndex() const { return 0; }
void ext::al::EffectSlot::set( ALenum name, ALfloat x ) {}
void ext::al::EffectSlot::set( ALenum name, ALint x ) {}

#endif
