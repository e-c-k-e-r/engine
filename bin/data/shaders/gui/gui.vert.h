layout (constant_id = 0) const uint PASSES = 6;
layout (location = 0) in vec2 inPos;
layout (location = 1) in vec2 inUv;

#include "./gui.h"

layout( push_constant ) uniform PushBlock {
  uint pass;
  uint draw;
} PushConstant;

struct Matrices {
	mat4 model[PASSES];
};
layout (binding = 0) uniform UBO {
	Matrices matrices;
	Gui gui;
#if GLYPH
	Glyph glyph;
#endif
} ubo;

layout (location = 0) out vec2 outUv;
layout (location = 1) out flat Gui outGui;
#if GLYPH
layout (location = 7) out flat Glyph outGlyph;
#endif
#if 0
out gl_PerVertex {
    vec4 gl_Position;
};
#endif
void main() {
	outUv = inUv;
	outGui = ubo.gui;
#if GLYPH
	outGlyph = ubo.glyph;
#endif
	gl_Position = ubo.matrices.model[PushConstant.pass] * vec4(inPos.xy, ubo.gui.depth, 1.0);
}