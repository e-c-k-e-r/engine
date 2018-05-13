#pragma once

#include <uf/config.h>

#if defined(UF_USE_OPENAL)

#include <uf/ext/oal/oal.h>

#include <vector>
#include <map>

#include <uf/utils/math/vector.h>
#include <uf/utils/math/quaternion.h>

namespace uf {
	class UF_API Audio {
	protected:
		std::string m_filename;
		ext::al::Source m_source;
		ext::al::Buffer m_buffer;
	public:
		Audio( const std::string& = "" );

		bool initialized();
		bool playing();

		void load( const std::string& = "" );
		void play();
		void stop();

		void setPosition( const pod::Vector3& );
		void setOrientation( const pod::Quaternion<>& );
	};
	class UF_API SoundEmitter {
	public:
		typedef std::map<std::string, uf::Audio> container_t;
	protected:
		uf::SoundEmitter::container_t m_container;
	public:
		uf::Audio& add( const std::string& );
		uf::Audio& get( const std::string& );
		const uf::Audio& get( const std::string& ) const;

		void cleanup( bool = false );
	};
}

#endif