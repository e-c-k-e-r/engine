#include <uf/config.h>
#if UF_USE_VORBIS

#if UF_USE_OPENAL
#include <uf/ext/oal/oal.h>
#endif

#include <uf/ext/vorbis/vorbis.h>
#include <uf/utils/memory/pool.h>
#include <iostream>
#include <cstdio>

namespace {
	int endian = 0;
	namespace funs {
		size_t read( void* destination, size_t size, size_t nmemb, void* userdata ) {
			//UF_MSG_DEBUG( size * nmemb );
			uf::Audio::Metadata& metadata = *((uf::Audio::Metadata*) userdata);
			std::ifstream& file = *metadata.stream.file;

			ALsizei length = size * nmemb; // chunk size * chunk count
			if ( metadata.stream.consumed + length > metadata.info.size ) {
				length = metadata.info.size - metadata.stream.consumed;
			}
			// reopen file handle if necessary
			if ( !file.is_open() ) {
				file.open(metadata.filename, std::ios::binary);
				if ( !file.is_open() ) {
					UF_MSG_ERROR("Could not open file: {}", metadata.filename);
					return 0;
				}
			}
			// (re)seek to current position
			file.clear();
			file.seekg(metadata.stream.consumed);
			// setup read buffer
			char data[length];
			if ( !file.read(&data[0], length) ) {
				if ( file.eof() ) file.clear();
				else if ( file.fail() ) {
					UF_MSG_ERROR("File stream has fail bit set: {}", metadata.filename );
					file.clear();
					return 0;
				}
				else if ( file.bad() ) {
					UF_MSG_ERROR("File stream has bad bit set: {}", metadata.filename );
					file.clear();
					return 0;
				}
			} else file.clear();
			// copy from temp buffer
			metadata.stream.consumed += length;
			memcpy( destination, &data[0], length );

			return length;
		}
		int seek( void* userdata, ogg_int64_t to, int type ) {
			//UF_MSG_DEBUG(type << " " << to);
			uf::Audio::Metadata& metadata = *((uf::Audio::Metadata*) userdata);
			switch ( type ) {
				case SEEK_CUR: metadata.stream.consumed += to; break; // increment
				case SEEK_END: metadata.stream.consumed = metadata.info.size - to; break; // from the end
				case SEEK_SET: metadata.stream.consumed = to; break; // from the start
				default: return -1;
			}
			// clamp
			if ( metadata.stream.consumed < 0 ) {
				metadata.stream.consumed = 0;
				return -1;
			}
			if ( metadata.stream.consumed > metadata.info.size ) {
				metadata.stream.consumed = metadata.info.size;
				return -1;
			}
			return 0;
		}
		int close( void* userdata ) {
			//UF_MSG_DEBUG( userdata );
			uf::Audio::Metadata& metadata = *((uf::Audio::Metadata*) userdata);
			if ( !metadata.stream.file ) return 0;

			std::ifstream& file = *metadata.stream.file;
			if ( file.is_open() ) file.close();
			delete metadata.stream.file;
			metadata.stream.file = NULL;
			return 0;
		}
		long tell( void* userdata ) {
			//UF_MSG_DEBUG( userdata );
			uf::Audio::Metadata& metadata = *((uf::Audio::Metadata*) userdata);
			return metadata.stream.consumed;
		}
	}
}

void ext::vorbis::open( uf::Audio::Metadata& metadata ) {
	// UF_MSG_INFO( "Vorbis " << (metadata.settings.streamed ? "stream" : "load") << " opened: " << metadata.filename );
	return metadata.settings.streamed ? ext::vorbis::stream( metadata ) : ext::vorbis::load( metadata );
}
void ext::vorbis::load( uf::Audio::Metadata& metadata ) {
	// use correct function
	if ( metadata.settings.streamed ) return ext::vorbis::stream( metadata );
	// create file handles
	FILE* file = fopen( metadata.filename.c_str(), "rb" );
	OggVorbis_File vorbisFile;

	if ( !file ) {
		UF_MSG_ERROR("Vorbis: failed to open {}. File error.", metadata.filename);
		return;
	}
	// get total file size
	fseek(file, 0L, SEEK_END);
	metadata.info.size = ftell(file);
	metadata.stream.consumed = 0;
	fseek(file, 0L, SEEK_SET);
	// create oggvorbis handle
	if( ov_open_callbacks(file, &vorbisFile, NULL, 0, OV_CALLBACKS_DEFAULT) < 0 ) {
		UF_MSG_ERROR("Vorbis: failed to open {}. Not Ogg.", metadata.filename);
		return;
	}
	// grab metadata
	vorbis_info* info = ov_info(&vorbisFile, -1);
	metadata.info.channels = info->channels;
	metadata.info.bitDepth = 16;
	metadata.info.frequency = info->rate;
	metadata.info.duration = ov_time_total(&vorbisFile, -1);
	if ( metadata.info.channels == 1 && metadata.info.bitDepth == 8 ) metadata.info.format = AL_FORMAT_MONO8;
	else if ( metadata.info.channels == 1 && metadata.info.bitDepth == 16 ) metadata.info.format = AL_FORMAT_MONO16;
	else if ( metadata.info.channels == 2 && metadata.info.bitDepth == 8 ) metadata.info.format = AL_FORMAT_STEREO8;
	else if ( metadata.info.channels == 2 && metadata.info.bitDepth == 16 ) metadata.info.format = AL_FORMAT_STEREO16;
	else {
		UF_MSG_ERROR("Vorbis: unrecognized OGG format: {} channels, {} bps", metadata.info.channels, metadata.info.bitDepth);
		return;
	}

	int32_t read;
	char buffer[uf::audio::bufferSize];
	uf::stl::vector<char> bytes;
	do {
		read = ov_read( &vorbisFile, buffer, uf::audio::bufferSize, endian, 2, 1, &metadata.stream.bitStream );
		bytes.insert( bytes.end(), buffer, buffer + read );
	} while ( read > 0 );
	
	metadata.al.buffer.buffer( metadata.info.format, (char*) &bytes[0], bytes.size(), metadata.info.frequency );
	metadata.al.source.set( AL_BUFFER, (ALint) metadata.al.buffer.getIndex() );

	ov_clear(&vorbisFile);
}
void ext::vorbis::stream( uf::Audio::Metadata& metadata ) {
	// use correct function
	if ( !metadata.settings.streamed ) return ext::vorbis::load( metadata );
	if ( !metadata.stream.file ) metadata.stream.file = new std::ifstream;
	if ( !metadata.stream.handle ) metadata.stream.handle = (void*) new OggVorbis_File;
	// create file handles
	std::ifstream& file = *metadata.stream.file;
	OggVorbis_File& vorbisFile = *((OggVorbis_File*) metadata.stream.handle);
	file.open( metadata.filename, std::ios::binary );
	if ( !file.is_open() ) {
		UF_MSG_ERROR("Vorbis: failed to open file stream: {}", metadata.filename);
		return;
	}
	if ( file.eof() ) {
		UF_MSG_ERROR("Vorbis: file stream EOF bit set: {}", metadata.filename);
		return;
	}
	if ( file.fail() ) {
		UF_MSG_ERROR("Vorbis: file stream fail bit set: {}", metadata.filename);
		return;
	}
	if ( !file ) {
		UF_MSG_ERROR("Vorbis: file is false: {}", metadata.filename);
		return;
	}
	// get total file size
	file.seekg(0, std::ios::end);
	metadata.info.size = file.tellg();
	metadata.stream.consumed = 0;
	file.seekg(0, std::ios::beg);
	// set our custom callbacks
	ov_callbacks callbacks;
	callbacks.read_func = ::funs::read;
	callbacks.seek_func = ::funs::seek;
	callbacks.close_func = ::funs::close;
	callbacks.tell_func = ::funs::tell;
	// create oggvorbis handle
	if ( ov_open_callbacks((void*) &metadata, &vorbisFile, NULL, -1, callbacks) < 0 ) {
		UF_MSG_ERROR("Vorbis: failed call to ov_open_callbacks: {}", metadata.filename);
		return;
	}
	// grab metadata
	vorbis_info* info = ov_info(&vorbisFile, -1);
	metadata.info.channels = info->channels;
	metadata.info.bitDepth = 16;
	metadata.info.frequency = info->rate;
	metadata.info.duration = ov_time_total(&vorbisFile, -1);
	if ( metadata.info.channels == 1 && metadata.info.bitDepth == 8 ) metadata.info.format = AL_FORMAT_MONO8;
	else if ( metadata.info.channels == 1 && metadata.info.bitDepth == 16 ) metadata.info.format = AL_FORMAT_MONO16;
	else if ( metadata.info.channels == 2 && metadata.info.bitDepth == 8 ) metadata.info.format = AL_FORMAT_STEREO8;
	else if ( metadata.info.channels == 2 && metadata.info.bitDepth == 16 ) metadata.info.format = AL_FORMAT_STEREO16;
	else {
		UF_MSG_ERROR("Vorbis: unrecognized OGG format: {} channels, {} bps", (int) metadata.info.channels, (int) metadata.info.bitDepth);
		return;
	}

//	UF_MSG_DEBUG( "filename\tchannels\tbitDepth\tfrequency\tduration\tchannels" );
//	UF_MSG_DEBUG( metadata.filename << "\t" <<  (int)metadata.info.channels << "\t" << (int)metadata.info.bitDepth << "\t" << (int)metadata.info.frequency << "\t" << metadata.info.duration << "\t" << (int) metadata.info.channels );

	// fill and queue initial buffers
	char buffer[uf::audio::bufferSize];
	uint8_t queuedBuffers = 0;
	for ( ; queuedBuffers < metadata.settings.buffers; ++queuedBuffers ) {
		int32_t read = 0;
		while ( read < uf::audio::bufferSize ) {
			int32_t result = ov_read( &vorbisFile, &buffer[read], uf::audio::bufferSize - read, endian, 2, 1, &metadata.stream.bitStream );
		
			if ( result == OV_HOLE ) {
				UF_MSG_ERROR("Vorbis: OV_HOLE found in buffer read: {} {}", (int) queuedBuffers, metadata.filename);
				break;
			} else if ( result == OV_EBADLINK ) {
				UF_MSG_ERROR("Vorbis: OV_EBADLINK found in buffer read: {} {}", (int) queuedBuffers, metadata.filename);
				break;
			} else if ( result == OV_EINVAL ) {
				UF_MSG_ERROR("Vorbis: OV_EINVAL found in buffer read: {} {}", (int) queuedBuffers, metadata.filename);
				break;
			} else if ( result == 0 ) {
				if ( !metadata.settings.loop ) break;
				std::int32_t seek = ov_raw_seek( &vorbisFile, 0 );
				// UF_MSG_ERROR("Vorbis: EOF found in buffer read: " << (int) queuedBuffers << " " << metadata.filename); break;
				switch ( seek ) {
					case OV_ENOSEEK: UF_MSG_ERROR("Vorbis: OV_ENOSEEK found in buffer loop: {}", metadata.filename); break;
					case OV_EINVAL: UF_MSG_ERROR("Vorbis: OV_EINVAL found in buffer loop: {}", metadata.filename); break;
					case OV_EREAD: UF_MSG_ERROR("Vorbis: OV_EREAD found in buffer loop: {}", metadata.filename); break;
					case OV_EFAULT: UF_MSG_ERROR("Vorbis: OV_EFAULT found in buffer loop: {}", metadata.filename); break;
					case OV_EOF: UF_MSG_ERROR("Vorbis: OV_EOF found in buffer loop: {}", metadata.filename); break;
					case OV_EBADLINK: UF_MSG_ERROR("Vorbis: OV_EBADLINK found in buffer loop: {}", metadata.filename); break;
				}
			}
			read += result;
		}
	
		if ( read == 0 ) {
		//	UF_MSG_WARNING("Vorbis: consumed file stream before buffers are filled: " << (int) queuedBuffers << " " << metadata.filename);
		//	if ( metadata.settings.loopMode == 0 ) metadata.settings.loopMode = 1;
		//	if ( metadata.settings.loop ) metadata.al.source.set( AL_LOOPING, AL_TRUE );
			break;
		}
		AL_CHECK_RESULT(alBufferData(metadata.al.buffer.getIndex(queuedBuffers), metadata.info.format, buffer, read, metadata.info.frequency ));
	}
	AL_CHECK_RESULT(alSourceQueueBuffers(metadata.al.source.getIndex(), queuedBuffers, &metadata.al.buffer.getIndex()));
	// switch to soft looping
	if ( queuedBuffers >= metadata.settings.buffers ) {
	//	UF_MSG_WARNING("Vorbis: file not completely consumed from initial buffer filled, yet looping is enabled. Soft looping...: " << metadata.filename );
		metadata.settings.loopMode = 1;
		metadata.al.source.set( AL_LOOPING, AL_FALSE );
	}
}
void ext::vorbis::update( uf::Audio::Metadata& metadata ) {
	if ( !metadata.settings.streamed ) return;
	// disable hard looping temporarily
	if ( metadata.settings.loopMode == 1 ) metadata.al.source.set( AL_LOOPING, AL_FALSE );

	ALint state;
//	metadata.al.source.get( AL_SOURCE_STATE, &state );
	metadata.al.source.get( AL_SOURCE_STATE, state );
	if ( state != AL_PLAYING ) {
		if ( !metadata.settings.loop && metadata.stream.consumed >= metadata.info.size ) {
		//	UF_MSG_INFO("Vorbis stream finished: " << metadata.filename);
			return;
		}
		// stream stalled, restart it
	//	UF_MSG_INFO("Vorbis stream stalled: " << metadata.filename);
		metadata.al.source.play();
	}

	ALint processed = 0;
//	metadata.al.source.get(AL_BUFFERS_PROCESSED, &processed);
	metadata.al.source.get(AL_BUFFERS_PROCESSED, processed);
	// no work need to be done
	if ( processed <= 0 ) return;

	OggVorbis_File& vorbisFile = *((OggVorbis_File*) metadata.stream.handle);
	
	ALuint index;
	char buffer[uf::audio::bufferSize];
	while ( processed-- ) {
		// clear read buffer
		memset( buffer, 0, uf::audio::bufferSize );
		// grab an available, unqueued buffer
		AL_CHECK_RESULT(alSourceUnqueueBuffers(metadata.al.source.getIndex(), 1, &index));
		// decode in chunks
		int32_t read = 0;
		while ( read < uf::audio::bufferSize ) {
			int32_t result = ov_read( &vorbisFile, &buffer[read], uf::audio::bufferSize - read, endian, 2, 1, &metadata.stream.bitStream );
			if ( result == OV_HOLE ) {
				UF_MSG_ERROR("Vorbis: OV_HOLE found in buffer read: {}", metadata.filename);
				break;
			} else if ( result == OV_EBADLINK ) {
				UF_MSG_ERROR("Vorbis: OV_EBADLINK found in buffer read: {}", metadata.filename);
				break;
			} else if ( result == OV_EINVAL ) {
				UF_MSG_ERROR("Vorbis: OV_EINVAL found in buffer read: {}", metadata.filename);
				break;
			} else if ( result == 0 ) {
				// no more data left to read, reset file stream if we're looping
				if ( !metadata.settings.loop ) break;
				std::int32_t seek = ov_raw_seek( &vorbisFile, 0 );
				switch ( seek ) {
					case OV_ENOSEEK: UF_MSG_ERROR("Vorbis: OV_ENOSEEK found in buffer loop: {}", metadata.filename); break;
					case OV_EINVAL: UF_MSG_ERROR("Vorbis: OV_EINVAL found in buffer loop: {}", metadata.filename); break;
					case OV_EREAD: UF_MSG_ERROR("Vorbis: OV_EREAD found in buffer loop: {}", metadata.filename); break;
					case OV_EFAULT: UF_MSG_ERROR("Vorbis: OV_EFAULT found in buffer loop: {}", metadata.filename); break;
					case OV_EOF: UF_MSG_ERROR("Vorbis: OV_EOF found in buffer loop: {}", metadata.filename); break;
					case OV_EBADLINK: UF_MSG_ERROR("Vorbis: OV_EBADLINK found in buffer loop: {}", metadata.filename); break;
				}
				if( seek != 0 ) {
                    UF_MSG_ERROR("Vorbis: Unknown error in ov_raw_seek: {}", metadata.filename);
                    return;
                }
			}
			read += result;
		}
		// buffer more data
		if ( read > 0 ) {
			AL_CHECK_RESULT(alBufferData(index, metadata.info.format, &buffer[0], read, metadata.info.frequency));
			AL_CHECK_RESULT(alSourceQueueBuffers(metadata.al.source.getIndex(), 1, &index));
		}
		if ( metadata.settings.loop && read < uf::audio::bufferSize ) {
			// should never actually reach here
			UF_MSG_ERROR("Vorbis: missing data: {}", metadata.filename);
		}
	}
	// enable hard looping for if we aren't able to call an update in a timely manner
	if ( metadata.settings.loopMode == 1 ) metadata.al.source.set( AL_LOOPING, AL_TRUE );
}
void ext::vorbis::close( uf::Audio::Metadata& metadata ) {
//	UF_MSG_INFO("Vorbis " << ( metadata.settings.streamed ? "stream" : "load" ) << " closed: " << metadata.filename);
	if ( metadata.stream.handle ) {
		ov_clear((OggVorbis_File*) metadata.stream.handle);
		delete (OggVorbis_File*) metadata.stream.handle;
	}
	if ( metadata.stream.file ) {
		if ( metadata.stream.file->is_open() ) metadata.stream.file->close();
		delete metadata.stream.file;
	}
	metadata.stream.file = NULL;
	metadata.stream.handle = NULL;
}
#endif