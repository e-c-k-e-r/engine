#include <uf/ext/vall_e/vall_e.h>
#include <uf/utils/time/time.h>
#include <uf/ext/audio/pcm.h>

#if UF_USE_VALL_E
namespace {
	vall_e_context_t* ctx = NULL;
}

void ext::vall_e::initialize( const std::string& model_path, const std::string& encodec_path ) {
	vall_e_context_params_t params;
	params.model_path = model_path == "" ? "./data/llm/vall_e.gguf" : model_path;
	params.encodec_path = encodec_path == "" ? "./data/llm/encodec.bin" : encodec_path;
	params.gpu_layers = N_GPU_LAYERS;
	params.n_threads = N_THREADS;
	params.ctx_size = CTX_SIZE;
	params.verbose = false;

	::ctx = vall_e_load( params );
	if ( !::ctx || !::ctx->llama.model || !::ctx->llama.ctx || !::ctx->encodec.ctx  ) {
		UF_MSG_ERROR("failed to initialize vall_e.cpp");
		return;
	}
}
pod::PCM ext::vall_e::generate( const std::string& text, const std::string& prom, const std::string& lang ) {	
	pod::PCM pcm;

	if ( !::ctx ) return pcm;

	vall_e_args_t args;
	args.text = text;
	args.prompt_path = prom;
	args.language = lang == "" ? "en" : lang;
	args.task = "tts";
	args.modality = MODALITY_NAR_LEN;
	args.max_steps = 15;
	args.max_duration = MAX_DURATION;

	auto inputs = vall_e_prepare_inputs( ::ctx, args.text, args.prompt_path, args.language );
	auto output_audio_codes = vall_e_generate( ::ctx, inputs, args.max_steps, args.max_duration, args.modality );
	auto waveform = decode_audio( ::ctx->encodec.ctx, output_audio_codes );
	pcm.samples = ext::pcm::convertTo16bit( waveform.data(), waveform.size() ); // need to cringily pass it this way because theyre different vector classes
	pcm.sampleRate = 24000; // should deduce from the backend in the event I ever get around to porting the other models
	pcm.channels = 1;

	return pcm;
}
void ext::vall_e::terminate() {	
	if ( !::ctx ) return;

	vall_e_free( ::ctx );
}
#endif