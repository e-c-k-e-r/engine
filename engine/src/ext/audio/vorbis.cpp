#include <uf/config.h>
#if UF_USE_VORBIS

#if UF_USE_OPENAL
#include <uf/ext/oal/oal.h>
#endif

#include <uf/ext/audio/vorbis.h>
#include <uf/utils/memory/pool.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstdio>
#include <cstring>

namespace {
	constexpr int endian = 0; // 0 = little endian

	namespace funs {
		size_t read(void* destination, size_t size, size_t nmemb, void* userdata) {
			uf::Audio::Metadata& metadata = *((uf::Audio::Metadata*)userdata);
			std::ifstream& file = *metadata.stream.file;
			size_t length = size * nmemb;

			if (metadata.stream.consumed + length > metadata.info.size)
				length = metadata.info.size - metadata.stream.consumed;

			if (!file.is_open()) {
				file.open(metadata.filename, std::ios::binary);
				if (!file.is_open()) {
					UF_MSG_ERROR("Could not open file: {}", metadata.filename);
					return 0;
				}
			}
			file.clear();
			file.seekg(metadata.stream.consumed);
			uf::stl::vector<char> data(length);
			if (!file.read(data.data(), length)) {
				if (file.eof()) file.clear();
				else {
					UF_MSG_ERROR("File stream error: {}", metadata.filename);
					file.clear();
					return 0;
				}
			}
			memcpy(destination, data.data(), length);
			metadata.stream.consumed += length;
			return length;
		}

		int seek(void* userdata, ogg_int64_t to, int type) {
			uf::Audio::Metadata& metadata = *((uf::Audio::Metadata*)userdata);
			switch (type) {
				case SEEK_CUR: metadata.stream.consumed += to; break;
				case SEEK_END: metadata.stream.consumed = metadata.info.size - to; break;
				case SEEK_SET: metadata.stream.consumed = to; break;
				default: return -1;
			}
			if (metadata.stream.consumed < 0) metadata.stream.consumed = 0;
			if (metadata.stream.consumed > metadata.info.size) metadata.stream.consumed = metadata.info.size;
			return 0;
		}

		int close(void* userdata) {
			uf::Audio::Metadata& metadata = *((uf::Audio::Metadata*)userdata);
			if (metadata.stream.file) {
				std::ifstream& file = *metadata.stream.file;
				if (file.is_open()) file.close();
				delete metadata.stream.file;
				metadata.stream.file = NULL;
			}
			return 0;
		}

		long tell(void* userdata) {
			uf::Audio::Metadata& metadata = *((uf::Audio::Metadata*)userdata);
			return metadata.stream.consumed;
		}
	}

	inline bool format(uf::Audio::Metadata& metadata, int channels, int bitDepth) {
		if (channels == 1 && bitDepth == 8) metadata.info.format = AL_FORMAT_MONO8;
		else if (channels == 1 && bitDepth == 16) metadata.info.format = AL_FORMAT_MONO16;
		else if (channels == 2 && bitDepth == 8) metadata.info.format = AL_FORMAT_STEREO8;
		else if (channels == 2 && bitDepth == 16) metadata.info.format = AL_FORMAT_STEREO16;
		else {
			UF_MSG_ERROR("Vorbis: unrecognized OGG format: {} channels, {} bps", channels, bitDepth);
			return false;
		}
		return true;
	}
}

void ext::vorbis::open(uf::Audio::Metadata& metadata) {
	if (metadata.settings.streamed)
		stream(metadata);
	else
		load(metadata);
}

void ext::vorbis::load(uf::Audio::Metadata& metadata) {
	if (metadata.settings.streamed) return stream(metadata);

	FILE* file = fopen(metadata.filename.c_str(), "rb");
	if (!file) {
		UF_MSG_ERROR("Vorbis: failed to open {}. File error.", metadata.filename);
		return;
	}

	fseek(file, 0, SEEK_END);
	metadata.info.size = ftell(file);
	fseek(file, 0, SEEK_SET);
	metadata.stream.consumed = 0;

	OggVorbis_File vorbisFile;
	if (ov_open_callbacks(file, &vorbisFile, NULL, 0, OV_CALLBACKS_DEFAULT) < 0) {
		UF_MSG_ERROR("Vorbis: failed to open {}. Not Ogg.", metadata.filename);
		fclose(file);
		return;
	}

	vorbis_info* info = ov_info(&vorbisFile, -1);
	metadata.info.channels = info->channels;
	metadata.info.bitDepth = 16;
	metadata.info.frequency = info->rate;
	metadata.info.duration = ov_time_total(&vorbisFile, -1);

	if (!format(metadata, info->channels, 16)) {
		ov_clear(&vorbisFile);
		return;
	}

	uf::stl::vector<char> bytes;
	char buffer[uf::audio::bufferSize];
	int bitStream = 0;
	int read = 0;
	do {
		read = ov_read(&vorbisFile, buffer, uf::audio::bufferSize, endian, 2, 1, &bitStream);
		if (read > 0)
			bytes.insert(bytes.end(), buffer, buffer + read);
	} while (read > 0);

	metadata.al.buffer.buffer(metadata.info.format, bytes.data(), (ALsizei)bytes.size(), metadata.info.frequency);
	metadata.al.source.set(AL_BUFFER, (ALint)metadata.al.buffer.getIndex());

	ov_clear(&vorbisFile);
}

void ext::vorbis::stream(uf::Audio::Metadata& metadata) {
	if (!metadata.settings.streamed) return load(metadata);

	if (!metadata.stream.file) metadata.stream.file = new std::ifstream;
	if (!metadata.stream.handle) metadata.stream.handle = new OggVorbis_File;

	std::ifstream& file = *metadata.stream.file;
	OggVorbis_File& vorbisFile = *(OggVorbis_File*)metadata.stream.handle;

	file.open(metadata.filename, std::ios::binary);
	if (!file.is_open()) {
		UF_MSG_ERROR("Vorbis: failed to open file stream: {}", metadata.filename);
		return;
	}

	file.seekg(0, std::ios::end);
	metadata.info.size = file.tellg();
	file.seekg(0, std::ios::beg);
	metadata.stream.consumed = 0;

	ov_callbacks callbacks;
	callbacks.read_func = funs::read;
	callbacks.seek_func = funs::seek;
	callbacks.close_func = funs::close;
	callbacks.tell_func = funs::tell;

	if (ov_open_callbacks((void*)&metadata, &vorbisFile, NULL, -1, callbacks) < 0) {
		UF_MSG_ERROR("Vorbis: failed call to ov_open_callbacks: {}", metadata.filename);
		return;
	}

	vorbis_info* info = ov_info(&vorbisFile, -1);
	metadata.info.channels = info->channels;
	metadata.info.bitDepth = 16;
	metadata.info.frequency = info->rate;
	metadata.info.duration = ov_time_total(&vorbisFile, -1);

	if (!format(metadata, info->channels, 16)) {
		ov_clear(&vorbisFile);
		return;
	}

	// Fill and queue initial buffers
	char buffer[uf::audio::bufferSize];
	uint8_t queuedBuffers = 0;
	int bitStream = 0;
	for (; queuedBuffers < metadata.settings.buffers; ++queuedBuffers) {
		int totalRead = 0;
		while (totalRead < uf::audio::bufferSize) {
			int result = ov_read(&vorbisFile, buffer + totalRead, uf::audio::bufferSize - totalRead, endian, 2, 1, &bitStream);
			if (result <= 0) {
				if (result == 0 && metadata.settings.loop) {
					if (ov_raw_seek(&vorbisFile, 0) != 0) {
						UF_MSG_ERROR("Vorbis: failed to loop (seek to start): {}", metadata.filename);
						break;
					}
					continue;
				}
				if (result == OV_HOLE) UF_MSG_ERROR("Vorbis: OV_HOLE in buffer read: {}", metadata.filename);
				if (result == OV_EBADLINK) UF_MSG_ERROR("Vorbis: OV_EBADLINK in buffer read: {}", metadata.filename);
				if (result == OV_EINVAL) UF_MSG_ERROR("Vorbis: OV_EINVAL in buffer read: {}", metadata.filename);
				break;
			}
			totalRead += result;
		}
		if (totalRead == 0) {
			UF_MSG_WARNING("Vorbis: consumed file stream before buffers are filled: {} {}", (int)queuedBuffers, metadata.filename);
			break;
		}
		AL_CHECK_RESULT(alBufferData(metadata.al.buffer.getIndex(queuedBuffers), metadata.info.format, buffer, totalRead, metadata.info.frequency));
	}
	AL_CHECK_RESULT(alSourceQueueBuffers(metadata.al.source.getIndex(), queuedBuffers, &metadata.al.buffer.getIndex()));

	if (queuedBuffers >= metadata.settings.buffers) {
		metadata.settings.loopMode = 1;
		metadata.al.source.set(AL_LOOPING, AL_FALSE);
	}
}

void ext::vorbis::update(uf::Audio::Metadata& metadata) {
	if (!metadata.settings.streamed) return;
	if (metadata.settings.loopMode == 1)
		metadata.al.source.set(AL_LOOPING, AL_FALSE);

	ALint state;
	metadata.al.source.get(AL_SOURCE_STATE, state);
	if (state != AL_PLAYING) {
		if (!metadata.settings.loop && metadata.stream.consumed >= metadata.info.size) {
			// Stream finished
			return;
		}
		metadata.al.source.play();
	}

	ALint processed = 0;
	metadata.al.source.get(AL_BUFFERS_PROCESSED, processed);
	if (processed <= 0) return;

	OggVorbis_File& vorbisFile = *(OggVorbis_File*)metadata.stream.handle;
	int bitStream = metadata.stream.bitStream;
	ALuint index;
	char buffer[uf::audio::bufferSize];

	while (processed--) {
		memset(buffer, 0, uf::audio::bufferSize);
		AL_CHECK_RESULT(alSourceUnqueueBuffers(metadata.al.source.getIndex(), 1, &index));

		int totalRead = 0;
		while (totalRead < uf::audio::bufferSize) {
			int result = ov_read(&vorbisFile, buffer + totalRead, uf::audio::bufferSize - totalRead, endian, 2, 1, &bitStream);
			if (result <= 0) {
				if (result == 0 && metadata.settings.loop) {
					if (ov_raw_seek(&vorbisFile, 0) != 0) {
						UF_MSG_ERROR("Vorbis: failed to loop (seek to start): {}", metadata.filename);
						break;
					}
					continue;
				}
				if (result == OV_HOLE) UF_MSG_ERROR("Vorbis: OV_HOLE in buffer read: {}", metadata.filename);
				if (result == OV_EBADLINK) UF_MSG_ERROR("Vorbis: OV_EBADLINK in buffer read: {}", metadata.filename);
				if (result == OV_EINVAL) UF_MSG_ERROR("Vorbis: OV_EINVAL in buffer read: {}", metadata.filename);
				break;
			}
			totalRead += result;
		}

		if (totalRead > 0) {
			AL_CHECK_RESULT(alBufferData(index, metadata.info.format, buffer, totalRead, metadata.info.frequency));
			AL_CHECK_RESULT(alSourceQueueBuffers(metadata.al.source.getIndex(), 1, &index));
		}
		if (metadata.settings.loop && totalRead < uf::audio::bufferSize) {
			UF_MSG_ERROR("Vorbis: missing data: {}", metadata.filename);
		}
	}

	if (metadata.settings.loopMode == 1)
		metadata.al.source.set(AL_LOOPING, AL_TRUE);
}

void ext::vorbis::close(uf::Audio::Metadata& metadata) {
	if (metadata.stream.handle) {
		OggVorbis_File* file = (OggVorbis_File*) metadata.stream.handle;
		ov_clear(file);
		delete file;
		metadata.stream.handle = NULL;
	}
	if (metadata.stream.file) {
		if (metadata.stream.file->is_open()) metadata.stream.file->close();
		delete metadata.stream.file;
		metadata.stream.file = NULL;
	}
}

#endif
