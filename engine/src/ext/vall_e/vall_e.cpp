#include <uf/ext/vall_e/vall_e.h>
#include <uf/utils/time/time.h>

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
std::string ext::vall_e::generate( const std::string& text, const std::string& prom, const std::string& lang ) {	
	if ( !::ctx ) return "";

	std::string path = "./data/tmp/" + std::to_string(uf::time::time()) + ".wav";

	vall_e_args_t args;
	args.text = text;
	args.prompt_path = prom;
	args.output_path = path;
	args.language = lang == "" ? "en" : lang;
	args.task = "tts";
	args.modality = MODALITY_NAR_LEN;
	args.max_steps = 30;
	args.max_duration = MAX_DURATION;

	auto inputs = vall_e_prepare_inputs( ::ctx, args.text, args.prompt_path, args.language );
	auto output_audio_codes = vall_e_generate( ::ctx, inputs, args.max_steps, args.max_duration, args.modality );
	auto waveform = decode_audio( ::ctx->encodec.ctx, output_audio_codes );
	write_audio_to_disk( waveform, args.output_path );
	//UF_MSG_DEBUG("Generated to {}", path);

	return path;
}
void ext::vall_e::terminate() {	
	if ( !::ctx ) return;

	vall_e_free( ::ctx );
}
#endif