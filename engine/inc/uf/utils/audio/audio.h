#pragma once

#include <uf/config.h>

#if defined(UF_USE_OPENAL)

#include <uf/ext/oal/oal.h>

#include <vector>
#include <unordered_map>

#include <uf/utils/math/vector.h>
#include <uf/utils/math/quaternion.h>

namespace uf {
	class UF_API Audio {
	protected:
		std::string m_filename;
		ext::al::Source m_source;
		ext::al::Buffer m_buffer;
		float m_duration;
	public:
		static bool mute;
	/*
		Audio( const std::string& = "" );
		Audio( Audio&& );
		Audio( const Audio& );
	*/
		~Audio();

		bool initialized();
		bool playing();
		void destroy();

		void load( const std::string& = "" );
		void play();
		void stop();
		ALfloat getTime();
		void setTime( ALfloat );
		float getDuration() const;

		void setPosition( const pod::Vector3& );
		void setOrientation( const pod::Quaternion<>& );
		void setVolume( float );

		ALfloat getPitch();
		void setPitch( ALfloat );
		ALfloat getGain();
		void setGain( ALfloat );
		ALfloat getRolloffFactor();
		void setRolloffFactor( ALfloat );
		ALfloat getMaxDistance();
		void setMaxDistance( ALfloat );

		float getVolume() const;
		const std::string& getFilename() const;
	};
	class UF_API SoundEmitter {
	public:
		typedef std::unordered_map<std::string, uf::Audio> container_t;
	protected:
		uf::SoundEmitter::container_t m_container;
	public:
		~SoundEmitter();
		uf::Audio& add( const std::string& );
		uf::Audio& add( const uf::Audio& );
		uf::Audio& get( const std::string& );
		const uf::Audio& get( const std::string& ) const;
		container_t& get();
		const container_t& get() const;

		void cleanup( bool = false );
	};
}

#endif