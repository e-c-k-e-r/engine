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

#define UF_AICA_THREADED 0

namespace impl {
	typedef int (*fill_callback_t)(void* user_data, uint8_t* buffer, int req_bytes);

	constexpr int STREAM_BUFFER_SIZE = 16384;
	constexpr size_t MAX_VIRTUAL_SOURCES = 256;
	constexpr size_t MAX_VIRTUAL_BUFFERS = 1024;

	struct Source {
		bool active = false;

		pod::Vector3f position = {0, 0, 0};
		float gain = 1;
		float pitch = 1;
		float max_distance = 100;
		bool looping = false;
		int state = AL_STOPPED;
		ALuint buffer_id = 0;
	};

	struct Buffer {
		bool active = false;
		uint32_t aica_addr = 0;
		snd_stream_hnd_t stream_hnd = SND_STREAM_INVALID;

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

	impl::Source sources[impl::MAX_VIRTUAL_SOURCES];
	impl::Buffer buffers[impl::MAX_VIRTUAL_BUFFERS];
	std::unordered_map<snd_stream_hnd_t, impl::Buffer*> g_activeStreams;
	mutex_t g_streamMutex = MUTEX_INITIALIZER;

	void* kos_stream_callback( snd_stream_hnd_t hnd, int req_bytes, int* bytes_returned ) {
		UF_MSG_DEBUG("[AICA] Requesting {} bytes", req_bytes);
		auto it = g_activeStreams.find(hnd);
		if ( it == g_activeStreams.end() ) {
			*bytes_returned = 0;
			UF_MSG_DEBUG("[AICA] No active stream found: {}", hnd);
			return nullptr;
		}

		impl::Buffer* buf = it->second;
		if ( req_bytes > impl::STREAM_BUFFER_SIZE ) {
			UF_MSG_WARNING("[AICA] Requesting more than buffer: {} > {}", req_bytes, impl::STREAM_BUFFER_SIZE);
			req_bytes = impl::STREAM_BUFFER_SIZE;
		}

		// generate a sin wave for testing
		// washdc plays this, albeit slowly because emulation speed is hindered
		// flycast doesn't play audio, this callback gets called twice then never again
	#if 1
		float frequency = 440.0f;
		float sample_rate = buf->frequency > 0 ? buf->frequency : 44100.0f;
		int channels = (buf->format == AL_FORMAT_STEREO16 || buf->format == AL_FORMAT_STEREO8) ? 2 : 1;

		int16_t* samples = (int16_t*)buf->stream_buffer;
		int num_samples = req_bytes / sizeof(int16_t);
		int half_period = (int)(sample_rate / (frequency * 2.0f));
		static int sample_count = 0;

		for( int i = 0; i < num_samples; i += channels ) {
			int16_t sample_val = ((sample_count / half_period) % 2 == 0) ? 16000 : -16000;

			samples[i] = sample_val;
			if ( channels == 2 ) samples[i + 1] = sample_val;
			sample_count++;
		}

		*bytes_returned = req_bytes;
	#else
		if ( buf->fill_callback && buf->user_data ) {
			*bytes_returned = buf->fill_callback(buf->user_data, buf->stream_buffer, req_bytes);
		} else {
			*bytes_returned = 0;
		}
	#endif

		if ( *bytes_returned == 0 ) return nullptr;

		return buf->stream_buffer;
	}

	uint32_t getSourceIndex() {
		for ( size_t i = 1; i < impl::MAX_VIRTUAL_SOURCES; ++i ) {
			if ( !impl::sources[i].active ) {
				impl::sources[i].active = true;
				impl::sources[i].state = AL_STOPPED;
				UF_MSG_DEBUG("Assigning index: {}", i);
				return i;
			}
		}
		return 0;
	}
	uint32_t getBufferIndex() {
		for ( size_t i = 1; i < impl::MAX_VIRTUAL_BUFFERS; ++i ) {
			if ( !impl::buffers[i].active ) {
				impl::buffers[i].active = true;
				UF_MSG_DEBUG("Assigning index: {}", i);
				return i;
			}
		}
		return 0;
	}

#if UF_AICA_THREADED
	kthread_t* thread_handle = nullptr;
	
	// to-do: pipe into uf::thread instead?
	void* audio_thread( void* param ) {
		while ( true ) {
			mutex_lock( &impl::g_streamMutex );
			for ( auto& pair : impl::g_activeStreams ) {
				// to-do: check if playing
				int status = snd_stream_poll(pair.first);
				if ( status < 0 ) {
					// to-do: set source to stopped
				}
			}
			mutex_unlock( &impl::g_streamMutex );
			thd_sleep(50);
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
	if ( name == AL_SOURCE_STATE ) x = impl::sources[this->m_index].state;
	else if ( name == AL_LOOPING ) x = impl::sources[this->m_index].looping ? 1 : 0;
	else if ( name == AL_BUFFER ) x = impl::sources[this->m_index].buffer_id;
}

void ext::al::Source::get( ALenum name, ALint* f ) const {
	if ( f ) this->get( name, f[0] );
}

void ext::al::Source::set( ALenum name, ALint x, ALint y, ALint z ) {
	// ...
}

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

		int vol = 255; // (int)(255.0f * src.gain);
		int pan = 128;
		
		if ( buffer.aica_addr != 0 ) {
			snd_sfx_play(buffer.aica_addr, vol, pan);
		} else if ( buffer.stream_hnd != SND_STREAM_INVALID ) {
			int channels = (buffer.format == AL_FORMAT_STEREO16 || buffer.format == AL_FORMAT_STEREO8) ? 1 : 0;

			mutex_lock(&impl::g_streamMutex);
		
			snd_stream_reinit(buffer.stream_hnd, impl::kos_stream_callback);
			snd_stream_start(buffer.stream_hnd, buffer.frequency, channels);
			snd_stream_volume(buffer.stream_hnd, vol);
		/*
			snd_stream_reinit(buffer.stream_hnd, impl::kos_stream_callback);
			snd_stream_queue_enable(buffer.stream_hnd);
			snd_stream_start(buffer.stream_hnd, buffer.frequency, channels);
			snd_stream_queue_go(buffer.stream_hnd);
			snd_stream_volume(buffer.stream_hnd, vol);
		*/

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

		if ( buffer.aica_addr != 0 ) {
			// ...
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

		if ( buffer.aica_addr != 0 ) {
			// ...
		} else if ( buffer.stream_hnd != SND_STREAM_INVALID ) {
			snd_stream_stop(buffer.stream_hnd);
		}
	}
}

bool ext::al::Source::playing() const {
	if ( !this->m_index ) return false;
	return impl::sources[this->m_index].state == AL_PLAYING;
}

// to-do: implement queueing
void ext::al::Source::queue( ALsizei n, ALuint* indices ) {
	//AL_CHECK_RESULT(alSourceQueueBuffers(this->m_index, n, indices));
}
void ext::al::Source::unqueue( ALsizei n, ALuint* indices ) {
	//AL_CHECK_RESULT(alSourceUnqueueBuffers(this->m_index, n, indices));
}

bool ext::al::Buffer::initialized() const {
	return !this->m_indices.empty();
}

void ext::al::Buffer::initialize( size_t size ) {
	if ( this->initialized() ) this->destroy();
	this->m_indices.resize(size, 0);
	for ( auto i = 0; i < size; ++i ) {
		this->m_indices[i] = impl::getBufferIndex();
	}
}

void ext::al::Buffer::destroy() {
	if ( this->initialized() ) {
		for ( size_t i = 0; i < this->m_indices.size(); i++ ) {
			ALuint index = this->m_indices[i];
			if ( 0 < index && index < impl::MAX_VIRTUAL_BUFFERS ) {
				impl::Buffer& buffer = impl::buffers[index];

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
	if ( 0 < index && index < impl::MAX_VIRTUAL_BUFFERS ) {
		impl::Buffer& buffer = impl::buffers[index];
		if ( buffer.stream_hnd == SND_STREAM_INVALID ) return;

		// to-do: adjust volume based on distance / pan based on orientation

		int poll_result = snd_stream_poll( buffer.stream_hnd );
		if ( poll_result < 0 ) UF_MSG_ERROR("[AICA] snd_stream_poll({}) failed with code: {}", buffer.stream_hnd, poll_result);
	}
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
	//AL_CHECK_RESULT(alBufferiv( this->m_indices[i], name, iv ));
	
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
// to-do: pick between snd_sfx_* backend (non-streamed) or snd_stream_* (streamed)
void ext::al::Buffer::buffer( ALuint index, ALenum format, const ALvoid* data, ALsizei size, ALsizei frequency ) {
	impl::Buffer& buffer = impl::buffers[index];

	if ( buffer.aica_addr ) {
		snd_mem_free(buffer.aica_addr);
		buffer.aica_addr = 0;
	}
	if ( buffer.stream_hnd != SND_STREAM_INVALID ) {
		snd_stream_destroy(buffer.stream_hnd);
	}

	buffer.format = format;
	buffer.frequency = frequency;
	buffer.size = size;

	if ( buffer.fill_callback ) {
		if ( !buffer.stream_buffer ) buffer.stream_buffer = (uint8_t*)(memalign(32, impl::STREAM_BUFFER_SIZE));
		if ( !buffer.stream_buffer ) {
			UF_MSG_ERROR("[AICA] Failed to allocate {} bytes", impl::STREAM_BUFFER_SIZE);
			return;
		}
		buffer.stream_hnd = snd_stream_alloc( impl::kos_stream_callback, impl::STREAM_BUFFER_SIZE );
		if ( buffer.stream_hnd == SND_STREAM_INVALID ) {
			UF_MSG_ERROR("[AICA] Failed to allocate {} bytes", impl::STREAM_BUFFER_SIZE);
			return;
		}
		mutex_lock(&impl::g_streamMutex);
		impl::g_activeStreams[buffer.stream_hnd] = &buffer;
		mutex_unlock(&impl::g_streamMutex);
	} else if ( data && size ) {
		uint32_t aica_addr = snd_mem_malloc(size);
		if ( !aica_addr ) {
			UF_MSG_ERROR("[AICA] Failed to allocate {} bytes", size);
			return;
		}
		spu_memload(aica_addr, (void*)(data), size);
		buffer.aica_addr = aica_addr;
		UF_MSG_DEBUG("[AICA] Buffer ID {} allocated {} bytes at SPU addr 0x{:X}", index, size, aica_addr);
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