#pragma once

#include <uf/config.h>

#if defined(UF_USE_OPENAL)

#include <uf/ext/oal/oal.h>

#include <vector>
#include <unordered_map>

#include <uf/utils/math/vector.h>
#include <uf/utils/math/quaternion.h>
#include <uf/utils/serialize/serializer.h>

namespace uf {
	class UF_API Audio {
	protected:
		std::string m_filename;
		ext::al::Source m_source;
		ext::al::Buffer m_buffer;
		float m_duration;
	public:
		static bool mute;
		static uf::Audio null;
		Audio( const std::string& = "" );
		Audio( Audio&& );
		Audio( const Audio& );
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

		Audio& operator=( const uf::Audio& );
	};
	class UF_API SoundEmitter {
	public:
		typedef std::vector<uf::Audio*> container_t;
	protected:
		uf::SoundEmitter::container_t m_container;
	public:
		~SoundEmitter();
		
		bool has( const std::string& ) const;

		uf::Audio& add( const std::string&, bool unique = false );
		uf::Audio& add( const uf::Audio&, bool unique = false );

		uf::Audio& get( const std::string& );
		const uf::Audio& get( const std::string& ) const;
		
		container_t& get();
		const container_t& get() const;
		
		void cleanup( bool = false );
	};
	class UF_API MappedSoundEmitter {
	public:
		typedef std::unordered_map<std::string, uf::Audio*> container_t;
	protected:
		uf::MappedSoundEmitter::container_t m_container;
	public:
		~MappedSoundEmitter();
		
		bool has( const std::string& ) const;

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