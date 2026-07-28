#include <uf/config.h>
#if UF_USE_VORBIS

#if UF_USE_OPENAL
#include <uf/ext/openal/openal.h>
#endif
#if UF_USE_AICA
#include <uf/ext/aica/aica.h>
#endif

#include <uf/ext/audio/vorbis.h>
#include <uf/utils/memory/pool.h>
#include <uf/utils/io/vfs.h>
#include <vector>
#include <cstring>

#if UF_USE_TREMOR
	#define OV_READ( file, buffer, len, endian, _x, _y, bitStream ) ov_read( file, buffer, len, bitStream )
#else
	#define OV_READ( file, buffer, len, endian, _x, _y, bitStream ) ov_read( file, buffer, len, endian, _x, _y, bitStream )
#endif

namespace {
	constexpr int endian = 0; // 0 = little endian

	struct VorbisVfsContext {
		pod::File file;

		~VorbisVfsContext() {
			if ( file ) file.close( file.handle );
		}
	};

	namespace funs {
		size_t read(void* destination, size_t size, size_t nmemb, void* userdata) {
			VorbisVfsContext* ctx = (VorbisVfsContext*) userdata;
			if ( !ctx->file ) return 0;

			size_t bytesRead = ctx->file.read(ctx->file.handle, destination, size * nmemb);
			return bytesRead / size;
		}

		int seek( void* userdata, ogg_int64_t to, int type ) {
			VorbisVfsContext* ctx = (VorbisVfsContext*) userdata;
			if ( !ctx->file ) return -1;

			int whence = SEEK_SET;
			if ( type == SEEK_CUR ) whence = SEEK_CUR;
			else if ( type == SEEK_END ) whence = SEEK_END;

			if ( ctx->file.seek(ctx->file.handle, (long)to, whence) ) return 0;
			return -1;
		}

		int close(void* userdata) {
			VorbisVfsContext* ctx = (VorbisVfsContext*) userdata;
			if (ctx) delete ctx;
			return 0;
		}

		long tell(void* userdata) {
			VorbisVfsContext* ctx = (VorbisVfsContext*) userdata;
			if ( !ctx->file ) return -1;
			return (long)ctx->file.tell(ctx->file.handle);
		}

		bool decode( OggVorbis_File* vorbisFile, pod::PCM& pcm ) {
			vorbis_info* info = ov_info(vorbisFile, -1);
			pcm.channels = info->channels;
			pcm.sampleRate = info->rate;

			ogg_int64_t totalSamples = ov_pcm_total(vorbisFile, -1);
			pcm.samples.resize(totalSamples * info->channels);

			char* bufferPtr = (char*)pcm.samples.data();
			int totalBytesNeeded = totalSamples * info->channels * sizeof(int16_t);
			int totalBytesRead = 0;
			int bitStream = 0;

			while ( totalBytesRead < totalBytesNeeded ) {
				int readCount = OV_READ(vorbisFile, bufferPtr + totalBytesRead, totalBytesNeeded - totalBytesRead, endian, 2, 1, &bitStream);
				if ( readCount <= 0 ) break;
				totalBytesRead += readCount;
			}
			return true;
		}

		int fill_buffer( pod::AudioSource* source, uint8_t* buffer, int req_bytes ) {
			pod::AudioClip* clip = source->clip;
			VorbisVfsContext* ctx = (VorbisVfsContext*)source->streamState.context;
			OggVorbis_File* vorbisFile = (OggVorbis_File*)source->streamState.handle;

			int totalRead = 0;
			while ( totalRead < req_bytes ) {
				int result = OV_READ(vorbisFile, (char*)buffer + totalRead, req_bytes - totalRead, 0, 2, 1, &source->streamState.bitStream);
				if ( result <= 0 ) {
					if ( result == 0 ) {
						if ( !source->info.pending.empty() ) {
							uf::stl::string nextFile = source->info.pending.front();
							source->info.pending.erase(source->info.pending.begin());

							ov_clear(vorbisFile);

							VorbisVfsContext* nextCtx = new VorbisVfsContext();
							nextCtx->file = uf::vfs::open(nextFile);

							ov_callbacks callbacks = { funs::read, funs::seek, funs::close, funs::tell };

							if ( ov_open_callbacks((void*) nextCtx, vorbisFile, NULL, -1, callbacks) < 0 ) {
								UF_MSG_ERROR("Gapless Vorbis transition failed! Could not open: {}", nextFile);
								delete nextCtx;
								break;
							}

							clip->filename = nextFile;
							nextCtx->file.seek(nextCtx->file.handle, 0, SEEK_END);
							clip->info.size = nextCtx->file.tell(nextCtx->file.handle);
							nextCtx->file.seek(nextCtx->file.handle, 0, SEEK_SET);
							clip->info.duration = ov_time_total(vorbisFile, -1);

							source->info.elapsed = 0.0f;
							source->info.timer.reset();
							source->info.timer.start();

							ctx = nextCtx;
							source->streamState.context = (void*) ctx;
							continue;
						}
						else if ( source->settings.loop ) {
							uint32_t seekTarget = clip->info.loop.has ? clip->info.loop.start : 0;
							ov_pcm_seek(vorbisFile, seekTarget);
							continue;
						}
					}
					break;
				}
				totalRead += result;
			}
			return totalRead;
		}
	}

	inline bool format( pod::AudioClip& clip, int channels, int bitDepth) {
		if (channels == 1 && bitDepth == 8) clip.info.format = AL_FORMAT_MONO8;
		else if (channels == 1 && bitDepth == 16) clip.info.format = AL_FORMAT_MONO16;
		else if (channels == 2 && bitDepth == 8) clip.info.format = AL_FORMAT_STEREO8;
		else if (channels == 2 && bitDepth == 16) clip.info.format = AL_FORMAT_STEREO16;
		else {
			UF_MSG_ERROR("Vorbis: unrecognized OGG format: {} channels, {} bps", channels, bitDepth);
			return false;
		}
		return true;
	}
}

void ext::vorbis::open( pod::AudioSource& source ) {
	pod::AudioClip* clip = source.clip;
	if ( !clip || !clip->streamed ) return;

	source.streamBuffers.initialize( source.settings.buffers );
	source.streamState.consumed = 0;
	source.streamState.bitStream = 0;

	VorbisVfsContext* ctx = new VorbisVfsContext();
	ctx->file = uf::vfs::open(clip->filename);
	if ( !ctx->file ) {
		UF_MSG_ERROR("Vorbis: failed to open file: {}", clip->filename);
		delete ctx;
		return;
	}

	OggVorbis_File* vorbisFile = new OggVorbis_File;
	ov_callbacks callbacks = { funs::read, funs::seek, funs::close, funs::tell };

	if ( ov_open_callbacks((void*) ctx, vorbisFile, NULL, -1, callbacks) < 0 ) {
		UF_MSG_ERROR("Vorbis: failed to open file: {}", clip->filename);
		delete ctx; delete vorbisFile;
		return;
	}

	source.streamState.context = (void*) ctx;
	source.streamState.handle = (void*) vorbisFile;

#if UF_USE_AICA
	source.streamBuffers.set( AL_STREAM_FILL_CALLBACK, (ALint*)(funs::fill_buffer) );
	source.streamBuffers.set( AL_STREAM_USER_DATA, (ALint*)(&source) );
	source.alSource.set( AL_BUFFER, (ALint)(source.streamBuffers.getIndex(0)) );
	source.streamBuffers.buffer( clip->info.format, NULL, 0, clip->info.frequency );
#else
	char buffer[uf::audio::bufferSize];
	uint8_t queuedBuffers = 0;
	for ( ; queuedBuffers < source.settings.buffers; ++queuedBuffers ) {
		int totalRead = 0;
		while (totalRead < uf::audio::bufferSize) {
			int result = OV_READ(vorbisFile, buffer + totalRead, uf::audio::bufferSize - totalRead, endian, 2, 1, &source.streamState.bitStream);
			if ( result <= 0 ) {
				if ( result == 0 && source.settings.loop ) {
					uint32_t seekTarget = clip->info.loop.has ? clip->info.loop.start : 0;
					ov_pcm_seek(vorbisFile, seekTarget);
					continue;
				}
				break;
			}
			totalRead += result;
		}
		if ( totalRead == 0 ) break;
		ext::al::Buffer::buffer( source.streamBuffers.getIndex(queuedBuffers), clip->info.format, buffer, totalRead, clip->info.frequency );
	}
	source.alSource.queue( queuedBuffers, &source.streamBuffers.getIndex(0) );

	if ( queuedBuffers >= source.settings.buffers ) {
		source.settings.loopMode = 1;
		source.alSource.set(AL_LOOPING, AL_FALSE);
	}
#endif
}

void ext::vorbis::load( pod::AudioClip& clip ) {
	VorbisVfsContext* ctx = new VorbisVfsContext();
	ctx->file = uf::vfs::open(clip.filename);
	if ( !ctx->file ) {
		UF_MSG_ERROR("Vorbis: failed to open file: {}", clip.filename);
		delete ctx;
		return;
	}

	OggVorbis_File* vorbisFile = new OggVorbis_File;
	ov_callbacks callbacks = { funs::read, funs::seek, funs::close, funs::tell };

	if ( ov_open_callbacks((void*) ctx, vorbisFile, NULL, clip.streamed ? -1 : 0, callbacks) < 0 ) {
		UF_MSG_ERROR("Vorbis: failed call to ov_open_callbacks: {}", clip.filename);
		delete ctx; delete vorbisFile;
		return;
	}

	vorbis_info* info = ov_info(vorbisFile, -1);
	clip.info.channels = info->channels;
	clip.info.bitDepth = 16;
	clip.info.frequency = info->rate;
	clip.info.duration = ov_time_total(vorbisFile, -1);

	ctx->file.seek(ctx->file.handle, 0, SEEK_END);
	clip.info.size = ctx->file.tell(ctx->file.handle);
	ctx->file.seek(ctx->file.handle, 0, SEEK_SET);

	clip.info.loop.has = false;
	clip.info.loop.start = 0;
	clip.info.loop.end = (uint32_t)ov_pcm_total(vorbisFile, -1);

	vorbis_comment* vc = ov_comment(vorbisFile, -1);
	if ( vc != nullptr ) {
		for ( auto i = 0; i < vc->comments; ++i ) {
			uf::stl::string comment(vc->user_comments[i], vc->comment_lengths[i]);
			uf::stl::string upperComment = uf::string::uppercase( comment );
			if ( upperComment.starts_with("LOOPSTART=" )) {
				clip.info.loop.start = std::stoul(comment.substr(10));
				clip.info.loop.has = true;
			} else if ( upperComment.starts_with("LOOPLENGTH=" )) {
				clip.info.loop.end = clip.info.loop.start + std::stoul(comment.substr(11));
			} else if ( upperComment.starts_with("LOOPEND=") ) {
				clip.info.loop.end = std::stoul(comment.substr(8));
			}
		}
	}

	if ( !format(clip, info->channels, 16) ) {
		ov_clear(vorbisFile); delete vorbisFile;
		return;
	}

	if ( !clip.streamed ) {
		pod::PCM pcm;
		if ( funs::decode( vorbisFile, pcm ) ) {
			clip.alBuffer.buffer(clip.info.format, pcm.samples.data(), (ALsizei)(pcm.samples.size() * sizeof(int16_t)), clip.info.frequency);
			if ( clip.info.loop.has ) {
				ALint loopPoints[2] = { (ALint) clip.info.loop.start, (ALint) clip.info.loop.end };
				clip.alBuffer.set( 0x2015 /* AL_LOOP_POINTS_SOFT */, loopPoints );
			}
		}
	}

	ov_clear(vorbisFile);
	delete vorbisFile;
}

void ext::vorbis::update( pod::AudioSource& source ) {
	pod::AudioClip* clip = source.clip;
	if ( !clip || !clip->streamed || !source.streamState.handle ) return;

#if UF_USE_AICA
	source.streamBuffers.poll();
#else
	OggVorbis_File* vorbisFile = (OggVorbis_File*) source.streamState.handle;
	VorbisVfsContext* ctx = (VorbisVfsContext*) source.streamState.context;

	ALint state, processed, queued;
	source.alSource.get(AL_SOURCE_STATE, state);
	source.alSource.get(AL_BUFFERS_PROCESSED, processed);
	source.alSource.get(AL_BUFFERS_QUEUED, queued);

	bool hasData = (ctx->file.tell(ctx->file.handle) < clip->info.size) || !source.info.pending.empty() || source.settings.loop;

	auto fillAndQueueBuffer = [&](ALuint index) -> bool {
		uint8_t buffer[uf::audio::bufferSize];
		int totalRead = funs::fill_buffer(&source, buffer, uf::audio::bufferSize);
		if ( totalRead > 0 ) {
			ext::al::Buffer::buffer(index, clip->info.format, buffer, totalRead, clip->info.frequency);
			source.alSource.queue( 1, &index );
			return true;
		}
		return false;
	};

	if ( queued == 0 ) {
		if ( hasData ) {
			for ( int i = 0; i < source.settings.buffers; ++i ) {
				if ( !fillAndQueueBuffer(source.streamBuffers.getIndex(i)) ) break;
			}
		}
	} else {
		while ( processed > 0 ) {
			ALuint index;
			source.alSource.unqueue( 1, &index );
			processed--;

			bool hasData = (ctx->file.tell(ctx->file.handle) < clip->info.size) || !source.info.pending.empty() || source.settings.loop;

			if ( hasData ) {
				fillAndQueueBuffer(index);
			}
		}
	}

	source.alSource.get(AL_SOURCE_STATE, state);
	source.alSource.get(AL_BUFFERS_QUEUED, queued);

	if ( state != AL_PLAYING && queued > 0 ) {
		source.alSource.play();
	}
#endif
}

void ext::vorbis::close( pod::AudioClip& clip ) {
	// ...
}

void ext::vorbis::close( pod::AudioSource& source ) {
	if ( !source.clip || !source.clip->streamed ) return;

#if !UF_USE_AICA
	ALint queued;
	source.alSource.get(AL_BUFFERS_QUEUED, queued);
	while ( queued-- ) {
		ALuint buffer;
		source.alSource.unqueue( 1, &buffer );
	}
#endif

	source.streamBuffers.destroy();

	if ( source.streamState.handle ) {
		OggVorbis_File* file = (OggVorbis_File*) source.streamState.handle;
		ov_clear(file);
		delete file;
		source.streamState.handle = NULL;
		source.streamState.context = NULL;
	}
}

bool ext::vorbis::decode( const uf::stl::string& filename, pod::PCM& outPcm ) {
	VorbisVfsContext* ctx = new VorbisVfsContext();
	ctx->file = uf::vfs::open(filename);
	if ( !ctx->file ) {
		delete ctx; return false;
	}

	OggVorbis_File* vorbisFile = new OggVorbis_File;
	ov_callbacks callbacks = { funs::read, funs::seek, funs::close, funs::tell };

	if ( ov_open_callbacks((void*) ctx, vorbisFile, NULL, 0, callbacks) < 0 ) {
		delete ctx; delete vorbisFile; return false;
	}

	bool result = funs::decode( vorbisFile, outPcm );

	ov_clear(vorbisFile);
	delete vorbisFile;
	return result;
}

#if UF_USE_VORBIS_ENCODER
#include <vorbis/vorbisenc.h>

uf::stl::vector<uint8_t> ext::vorbis::encode( const pod::PCM& pcm ) {
	uf::stl::vector<uint8_t> output;

	if ( pcm.samples.empty() ) return output;

	vorbis_info vi;
	vorbis_info_init(&vi);

	int ret = vorbis_encode_init_vbr(&vi, pcm.channels, pcm.sampleRate, 0.4f);
	if ( ret < 0 ) {
		UF_MSG_ERROR("Vorbis Encoder: Initialization failed.");
		vorbis_info_clear(&vi);
		return output;
	}

	vorbis_dsp_state vd;
	vorbis_block vb;
	vorbis_analysis_init(&vd, &vi);
	vorbis_block_init(&vd, &vb);

	ogg_stream_state os;
	ogg_stream_init(&os, std::rand());

	vorbis_comment vc;
	vorbis_comment_init(&vc);
	vorbis_comment_add_tag(&vc, "ENCODER", "uf::audio pipeline");

	ogg_packet header_packet;
	ogg_packet header_comm;
	ogg_packet header_code;
	vorbis_analysis_headerout(&vd, &vc, &header_packet, &header_comm, &header_code);

	ogg_stream_packetin(&os, &header_packet);
	ogg_stream_packetin(&os, &header_comm);
	ogg_stream_packetin(&os, &header_code);

	auto flush_ogg_pages = [&](bool force) {
		ogg_page og;
		if ( force ) {
			while ( ogg_stream_flush(&os, &og) != 0 ) {
				output.insert(output.end(), og.header, og.header + og.header_len);
				output.insert(output.end(), og.body, og.body + og.body_len);
			}
		} else {
			while ( ogg_stream_pageout(&os, &og) != 0 ) {
				output.insert(output.end(), og.header, og.header + og.header_len);
				output.insert(output.end(), og.body, og.body + og.body_len);
			}
		}
	};

	flush_ogg_pages(true);

	size_t samplesProcessed = 0;
	size_t totalSamples = pcm.samples.size() / pcm.channels;
	constexpr int block_size = 1024;

	while ( samplesProcessed < totalSamples ) {
		int toWrite = (totalSamples - samplesProcessed > block_size) ? block_size : (totalSamples - samplesProcessed);

		float** buffer = vorbis_analysis_buffer(&vd, toWrite);

		for ( int i = 0; i < toWrite; ++i ) {
			for ( int c = 0; c < pcm.channels; ++c ) {
				size_t srcIdx = (samplesProcessed + i) * pcm.channels + c;
				buffer[c][i] = pcm.samples[srcIdx] / 32768.f;
			}
		}

		vorbis_analysis_wrote(&vd, toWrite);

		while ( vorbis_analysis_blockout(&vd, &vb) == 1 ) {
			vorbis_analysis(&vb, nullptr);
			vorbis_bitrate_addblock(&vb);

			ogg_packet op;
			while ( vorbis_bitrate_flushpacket(&vd, &op) == 1 ) {
				ogg_stream_packetin(&os, &op);
				flush_ogg_pages(false);
			}
		}

		samplesProcessed += toWrite;
	}

	vorbis_analysis_wrote(&vd, 0);

	while ( vorbis_analysis_blockout(&vd, &vb) == 1 ) {
		vorbis_analysis(&vb, nullptr);
		vorbis_bitrate_addblock(&vb);

		ogg_packet op;
		while ( vorbis_bitrate_flushpacket(&vd, &op) == 1 ) {
			ogg_stream_packetin(&os, &op);
			flush_ogg_pages(false);
		}
	}

	flush_ogg_pages(true);

	ogg_stream_clear(&os);
	vorbis_block_clear(&vb);
	vorbis_dsp_clear(&vd);
	vorbis_comment_clear(&vc);
	vorbis_info_clear(&vi);

	return output;
}
#endif

#endif