#include <uf/utils/math/vector.h>
#if UF_USE_SIMD

#if SSE_INSTR_SET > 7                  // AVX2 and later
	#pragma message "Using AVX2"
#elif SSE_INSTR_SET == 7
	#pragma message "Using AVX"
#elif SSE_INSTR_SET == 6
	#pragma message "Using SSE4.2"
#elif SSE_INSTR_SET == 5
	#pragma message "Using SSE4.1"
#elif SSE_INSTR_SET == 4
	#pragma message "Using SSSE3"
#elif SSE_INSTR_SET == 3
	#pragma message "Using SSE3"
#elif SSE_INSTR_SET == 2
	#pragma message "Using SSE2"
#elif SSE_INSTR_SET == 1
	#pragma message "Using SSE"
#endif

#endif