#pragma once

#include <uf/config.h>
#if UF_USE_VORBIS

#include <vorbis/vorbisfile.h>

#include <vector>
#include <string>

namespace ext {
	class UF_API Vorbis {
	public:
		static int BUFFER;
	protected:
		std::vector<char> m_buffer;
		int m_format;
		int m_frequency;
		float m_duration;
	public:
		void load( const std::string& );
		
		std::vector<char>& getBuffer();
		const std::vector<char>& getBuffer() const;
		int getFormat() const;
		int getFrequency() const;
		float getDuration() const;
	};
}

#endif