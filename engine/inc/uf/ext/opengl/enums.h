#pragma once

#include <uf/ext/opengl/ogl.h>

namespace ext {
	namespace opengl {
		namespace enums {
			namespace Compare {
				typedef GLenum type_t;
				static const GLenum NEVER = GL_NEVER;
				static const GLenum LESS = GL_LESS;
				static const GLenum EQUAL = GL_EQUAL;
				static const GLenum LESS_OR_EQUAL = GL_LEQUAL;
				static const GLenum GREATER = GL_GREATER;
				static const GLenum NOT_EQUAL = GL_NOTEQUAL;
				static const GLenum GREATER_OR_EQUAL = GL_GEQUAL;
				static const GLenum ALWAYS = GL_ALWAYS;
			}
			namespace Format {
				typedef GLenum type_t;
				static const GLenum D32_SFLOAT = 0x00000001; //GLenumerator(VK_FORMAT_D32_SFLOAT);
				static const GLenum R16G16B16A16_SFLOAT = 0x00000002; //GLenumerator(VK_FORMAT_R16G16B16A16_SFLOAT);
				static const GLenum R8_UNORM = 0x00000004; //GLenumerator(VK_FORMAT_R8_UNORM);
				static const GLenum R32_UINT = 0x00000008; //GLenumerator(VK_FORMAT_R32_UINT);
				static const GLenum R32G32_SINT = 0x00000010; //GLenumerator(VK_FORMAT_R32G32_SINT);
				static const GLenum R32G32_SFLOAT = 0x00000020; //GLenumerator(VK_FORMAT_R32G32_SFLOAT);
				static const GLenum R32G32B32_SFLOAT = 0x00000040; //GLenumerator(VK_FORMAT_R32G32B32_SFLOAT);
				static const GLenum R32G32B32A32_SFLOAT = 0x00000080; //GLenumerator(VK_FORMAT_R32G32B32A32_SFLOAT);
				static const GLenum R8G8B8A8_UNORM = 0x00000100; //GLenumerator(VK_FORMAT_R8G8B8A8_UNORM);
			}
			namespace Face {
				typedef GLenum type_t;
				static const GLenum CW = GL_CW;
				static const GLenum CCW = GL_CCW;
			}
			namespace CullMode {
				typedef GLenum type_t;
				static const GLenum NONE = 0;
				static const GLenum FRONT = GL_FRONT;
				static const GLenum BACK = GL_BACK;
				static const GLenum BOTH = GL_FRONT_AND_BACK;
			}
			namespace Shader {
				typedef GLenum type_t;
			#if UF_ENV_DREAMCAST
				static const GLenum VERTEX = 0x00000001;
				static const GLenum TESSELLATION_CONTROL = 0x00000002;
				static const GLenum TESSELLATION_EVALUATION = 0x00000003;
				static const GLenum GEOMETRY = 0x00000004;
				static const GLenum FRAGMENT = 0x00000005;
				static const GLenum COMPUTE = 0x00000006;
				static const GLenum ALL_GRAPHICS = 0x00000007;
				static const GLenum ALL = 0x00000008;
			#else
				static const GLenum VERTEX = GL_VERTEX_SHADER;
				static const GLenum TESSELLATION_CONTROL = GL_TESS_CONTROL_SHADER;
				static const GLenum TESSELLATION_EVALUATION = GL_TESS_EVALUATION_SHADER;
				static const GLenum GEOMETRY = GL_GEOMETRY_SHADER;
				static const GLenum FRAGMENT = GL_FRAGMENT_SHADER;
				static const GLenum COMPUTE = GL_COMPUTE_SHADER;
				static const GLenum ALL_GRAPHICS = GLenumerator(VK_SHADER_STAGE_ALL_GRAPHICS);
				static const GLenum ALL = GLenumerator(VK_SHADER_STAGE_ALL);
			#endif
			}
			namespace PrimitiveTopology {
				typedef GLenum type_t;
				static const GLenum POINT_LIST = GLenumerator(VK_PRIMITIVE_TOPOLOGY_POINT_LIST);
				static const GLenum LINE_LIST = GLenumerator(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
				static const GLenum LINE_STRIP = GLenumerator(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP);
				static const GLenum TRIANGLE_LIST = GLenumerator(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
				static const GLenum TRIANGLE_STRIP = GLenumerator(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
				static const GLenum TRIANGLE_FAN = GLenumerator(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN);
				static const GLenum LINE_LIST_WITH_ADJACENCY = GLenumerator(VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY);
				static const GLenum LINE_STRIP_WITH_ADJACENCY = GLenumerator(VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY);
				static const GLenum TRIANGLE_LIST_WITH_ADJACENCY = GLenumerator(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY);
				static const GLenum TRIANGLE_STRIP_WITH_ADJACENCY = GLenumerator(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY);
				static const GLenum PATCH_LIST = GLenumerator(VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
			}
			namespace PolygonMode {
				typedef GLenum type_t;
				static const GLenum FILL = GLenumerator(VK_POLYGON_MODE_FILL);
				static const GLenum LINE = GLenumerator(VK_POLYGON_MODE_LINE);
				static const GLenum POINT = GLenumerator(VK_POLYGON_MODE_POINT);
			}
			namespace AddressMode {
				typedef GLenum type_t;
				static const GLenum CLAMP_TO_EDGE = GLenumerator(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
				static const GLenum CLAMP_TO_BORDER = GLenumerator(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER);
				static const GLenum MIRRORED_REPEAT = GLenumerator(VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT);
				static const GLenum REPEAT = GLenumerator(VK_SAMPLER_ADDRESS_MODE_REPEAT);
			}
			namespace Filter {
				typedef GLenum type_t;
				static const GLenum NEAREST = GL_NEAREST;
				static const GLenum LINEAR = GL_LINEAR;
			}
			namespace Buffer {
				typedef GLenum type_t;
				static const GLenum TRANSFER_SRC = 0x00000001;
				static const GLenum TRANSFER_DST = 0x00000002;
				static const GLenum UNIFORM_TEXEL = 0x00000004;
				static const GLenum STORAGE_TEXEL = 0x00000008;
				static const GLenum UNIFORM = 0x00000010;
				static const GLenum STORAGE = 0x00000020;
				static const GLenum INDEX = 0x00000040;
				static const GLenum VERTEX = 0x00000080;
				static const GLenum INDIRECT = 0x00000100;
			}
		}
	}
}