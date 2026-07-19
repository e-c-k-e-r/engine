#include <uf/utils/audio/audio.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/math/physics.h>

#if UF_USE_OPENAL
	#include <uf/ext/openal/openal.h>
#endif

#include <uf/ext/audio/vorbis.h>
#include <uf/ext/audio/wav.h>
#include <uf/ext/audio/pcm.h>

#if UF_USE_OPENAL
	bool uf::audio::muted = false;
#else
	bool uf::audio::muted = true;
#endif

namespace {
	ext::al::Filter occlusionFilter;
	pod::Transform<> listener;
}

bool uf::audio::asyncUpdate = false;
bool uf::audio::streamsByDefault = true;
uint8_t uf::audio::buffers = 4;
size_t uf::audio::bufferSize = 1024 * 16;

#if UF_AUDIO_MAPPED_VOLUMES
	uf::stl::unordered_map<uf::stl::string, float> uf::audio::volumes;
#else
	float uf::audio::volumes::bgm = 1.0f;
	float uf::audio::volumes::sfx = 1.0f;
	float uf::audio::volumes::voice = 1.0f;
#endif

void uf::audio::initialize( pod::AudioClip& clip, uint8_t buffers ) {
	clip.alBuffer.initialize( buffers );
}
void uf::audio::initialize( pod::AudioSource& source ) {
	source.alSource.initialize();
	source.alSource.set( AL_PITCH, 1.0f );
	source.alSource.set( AL_GAIN, 1.0f );

#if !UF_ENV_DREAMCAST
	source.alFilter.initialize();
#endif
}

bool uf::audio::load( pod::AudioClip& clip, const uf::stl::string& filename, bool streamed ) {
	uf::audio::destroy( clip );
	clip.streamed = streamed;
	clip.filename = uf::io::resolveURI( filename );
	clip.extension = uf::io::extension( clip.filename );

	uf::audio::initialize( clip, 1 );

	if ( clip.extension == "ogg" ) ext::vorbis::load( clip );
	else if ( clip.extension == "wav" ) ext::wav::load( clip );
	// else UF_MSG_ERROR ...

	return true;
}

// to-do: PCM audio load

void uf::audio::bind( pod::AudioSource& source, pod::AudioClip* clip ) {
	source.info.elapsed = 0.0f;
	source.info.timer.stop();
	source.info.timer.reset();

	source.clip = clip;
	if ( !clip ) return;

	if ( !clip->streamed ) {
		source.alSource.set( AL_BUFFER, (ALint)clip->alBuffer.getIndex(0) );
	} else {
		source.streamState.consumed = 0;
		if ( clip->extension == "ogg" ) ext::vorbis::open( source );
		else if ( clip->extension == "wav" ) ext::wav::open( source );
		else if ( clip->extension == "pcm" ) ext::pcm::open( source );
	}
}

void uf::audio::queue( pod::AudioSource& source, const uf::stl::string& filename ) {
	source.info.pending.emplace_back( filename );
}

void uf::audio::play( pod::AudioSource& source ) {
	if ( !source.alSource.playing() ) {
		source.info.elapsed += source.info.timer.elapsed().asDouble();
		source.info.timer.start();
	}
	source.alSource.play();
}

void uf::audio::stop( pod::AudioSource& source ) {
	source.alSource.stop();
	source.info.timer.stop();
	if ( source.clip && source.clip->streamed ) {
		// to-do: reset stream cursor
	}
}

void uf::audio::pause( pod::AudioSource& source ) {
	AL_CHECK_RESULT(alSourcePause( source.alSource.getIndex() ));
	source.info.timer.stop();
}

bool uf::audio::paused( const pod::AudioSource& source ) {
	ALint state;
	AL_CHECK_RESULT(alGetSourcei( source.alSource.getIndex(), AL_SOURCE_STATE, &state ));
	return state == AL_PAUSED;
}

void uf::audio::update( pod::AudioSource& source ) {
	if ( !source.clip || !source.clip->streamed ) return;

	auto update = [&]{
		if ( source.clip->extension == "ogg" ) ext::vorbis::update( source );
		else if ( source.clip->extension == "wav" ) ext::wav::update( source );
		else if ( source.clip->extension == "pcm" ) ext::pcm::update( source );
	};

	if ( uf::audio::asyncUpdate ) uf::thread::queue( uf::thread::fetchWorker(), update );
	else update();
}

void uf::audio::update( pod::AudioSource& source, const pod::Vector3f& position, const pod::Quaternion<>& orientation ) {
	if ( source.settings.spatial ) {
		uf::audio::position( source, position );
		uf::audio::orientation( source, orientation );
	}
	uf::audio::update( source );
}

void uf::audio::destroy( pod::AudioSource& source ) {
	uf::audio::stop( source );
	source.alSource.set( AL_BUFFER, 0 );

	if ( source.clip && source.clip->streamed ) {
		if ( source.clip->extension == "ogg" ) ext::vorbis::close( source );
		else if ( source.clip->extension == "wav" ) ext::wav::close( source );
		else if ( source.clip->extension == "pcm" ) ext::pcm::close( source );
	}

	source.clip = nullptr;
	source.alSource.destroy();
#if !UF_ENV_DREAMCAST
	source.alFilter.destroy();
#endif
}

void uf::audio::destroy( pod::AudioClip& clip ) {
	clip.alBuffer.destroy();
	if ( clip.extension == "ogg" ) ext::vorbis::close( clip );
	else if ( clip.extension == "wav" ) ext::wav::close( clip );
	else if ( clip.extension == "pcm" ) ext::pcm::close( clip );
}

void uf::audio::listener( const pod::Transform<>& transform ) {
	if ( uf::audio::muted ) return;
	::listener = transform;
	auto axes = uf::transform::axes( transform );
	axes.forward *= -1;
	float o[6] = { axes.forward.x, axes.forward.y, axes.forward.z, axes.up.x, axes.up.y, axes.up.z };
	AL_CHECK_RESULT(alListener3f( AL_POSITION, transform.position.x, transform.position.y, transform.position.z ));
	AL_CHECK_RESULT(alListener3f( AL_VELOCITY, 0, 0, 0 ));
	AL_CHECK_RESULT(alListenerfv( AL_ORIENTATION, &o[0] ));
}

void uf::audio::loop( pod::AudioSource& source, bool state ) {
	source.settings.loop = state;
	if ( source.clip && !source.clip->streamed ) {
		source.alSource.set( AL_LOOPING, state ? AL_TRUE : AL_FALSE );
	}
}

void uf::audio::position( pod::AudioSource& source, const pod::Vector3f& v ) {
	source.transform.position = v;
	source.alSource.set( AL_POSITION, v[0], v[1], v[2] );
}

void uf::audio::orientation( pod::AudioSource& source, const pod::Quaternion<>& q ) {
	source.transform.orientation = q;
}

float uf::audio::time( pod::AudioSource& source ) {
	return source.info.elapsed + source.info.timer.elapsed().asDouble();
}
float uf::audio::time( const pod::AudioSource& source ) {
	return source.info.elapsed + source.info.timer.elapsed().asDouble();
}
void uf::audio::time( pod::AudioSource& source, float v ) {
	source.alSource.set( AL_SEC_OFFSET, v ); 
}
float uf::audio::pitch( const pod::AudioSource& source ) {
	float v;
	source.alSource.get( AL_PITCH, v );
	return v;
}
void uf::audio::pitch( pod::AudioSource& source, float v ) {
	source.alSource.set( AL_PITCH, v );
}
float uf::audio::gain( const pod::AudioSource& source ) {
	float v;
	source.alSource.get( AL_GAIN, v );
	return v;
}
void uf::audio::gain( pod::AudioSource& source, float v ) {
	source.alSource.set( AL_GAIN, v );
}
float uf::audio::rolloff( const pod::AudioSource& source ) {
	float v;
	source.alSource.get( AL_ROLLOFF_FACTOR, v );
	return v;
}
void uf::audio::rolloff( pod::AudioSource& source, float v ) {
	source.alSource.set( AL_ROLLOFF_FACTOR, v );
}
float uf::audio::maxDistance( const pod::AudioSource& source ) {
	float v;
	source.alSource.get( AL_MAX_DISTANCE, v );
	return v;
}
void uf::audio::maxDistance( pod::AudioSource& source, float v ) {
	source.alSource.set( AL_MAX_DISTANCE, v );
}

//
float uf::audio::distance( const pod::Vector3f& position ) {
	return uf::vector::distance( ::listener.position, position );
}

float uf::audio::occlusion( const pod::Vector3f& position ) {
	return uf::physics::occlusion( ::listener.position, position );
}

void uf::audio::occlude( pod::AudioSource& source, float factor ) {
#if UF_ENV_DREAMCAST
	uf::audio::gain( source, factor );
#else
	if ( factor >= 0.99f ) {
		source.alSource.set( AL_DIRECT_FILTER, AL_FILTER_NULL );
	} else {
		ALuint filterId = source.alFilter.getIndex();

		AL_CHECK_RESULT(alFilterf( filterId, AL_LOWPASS_GAINHF, factor ));
		AL_CHECK_RESULT(alFilterf( filterId, AL_LOWPASS_GAIN, 0.8f ));

		source.alSource.set( AL_DIRECT_FILTER, (ALint)filterId );
	}
#endif
}

void uf::audio::acoustics( const pod::Vector3f& position, const pod::Quaternion<>& orientation, float& energy, float& distance, int& bounces ) {
#if !UF_ENV_DREAMCAST
	static uf::stl::vector<pod::Vector3f> sphere = uf::math::fibonacciSphere( 32 );
	for ( const auto& dir : sphere ) {
		auto bounce = uf::physics::acousticReflection( position, dir, ::listener.position, 50.0f );
		if ( bounce.valid ) {
			energy += bounce.retainedEnergy;
			distance += bounce.totalDistance;
			bounces++;
		}
	}
#endif
}