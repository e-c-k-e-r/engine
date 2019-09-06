#include <uf/config.h>
#if defined(UF_USE_VORBIS)

#include <uf/ext/oal/oal.h>
#include <uf/ext/vorbis/vorbis.h>
#include <iostream>
#include <cstdio>

int ext::Vorbis::BUFFER = 32768;

void UF_API_CALL ext::Vorbis::load( const std::string& filename ) {
	int endian = 0;
	int bitStream;
	long bytes;
	char buffer[ext::Vorbis::BUFFER];
	FILE* fp = fopen( filename.c_str(), "rb" );

	if ( !fp ) { 
		std::cerr << "Vorbis: failed to open `" << filename << "`. File error." << std::endl;
		return;
	}

	OggVorbis_File file;
//	ov_open( fp, &file, NULL, 0 );
	if( ov_open_callbacks(fp, &file, NULL, 0, OV_CALLBACKS_DEFAULT) < 0 ) {
		std::cerr << "Vorbis: failed to open `" << filename << "`. Not Ogg." << std::endl;
		return;
	}

	vorbis_info* info = ov_info(&file, -1);
	this->m_format = info->channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
	this->m_frequency = info->rate;

	do {
		bytes = ov_read( &file, buffer, ext::Vorbis::BUFFER, endian, 2, 1, &bitStream );
		this->m_buffer.insert( this->m_buffer.end(), buffer, buffer + bytes );
	} while ( bytes > 0 );
	
	this->m_duration = ov_time_total(&file, -1);

	ov_clear(&file);
}

std::vector<char>& UF_API_CALL ext::Vorbis::getBuffer() {
	return this->m_buffer;
}
const std::vector<char>& UF_API_CALL ext::Vorbis::getBuffer() const {
	return this->m_buffer;
}
int UF_API_CALL ext::Vorbis::getFormat() const {
	return this->m_format;
}
int UF_API_CALL ext::Vorbis::getFrequency() const {
	return this->m_frequency;
}
float UF_API_CALL ext::Vorbis::getDuration() const {
	return this->m_duration;
}

#endif