#pragma once

#include <uf/ext/opengl/ogl.h>

namespace ext {
	namespace opengl {
		namespace enums {
			namespace Compare {
				typedef GLenum type_t;
				static const GLenum NEVER = GLenumerator(VK_COMPARE_OP_NEVER);
				static const GLenum LESS = GLenumerator(VK_COMPARE_OP_LESS);
				static const GLenum EQUAL = GLenumerator(VK_COMPARE_OP_EQUAL);
				static const GLenum LESS_OR_EQUAL = GLenumerator(VK_COMPARE_OP_LESS_OR_EQUAL);
				static const GLenum GREATER = GLenumerator(VK_COMPARE_OP_GREATER);
				static const GLenum NOT_EQUAL = GLenumerator(VK_COMPARE_OP_NOT_EQUAL);
				static const GLenum GREATER_OR_EQUAL = GLenumerator(VK_COMPARE_OP_GREATER_OR_EQUAL);
				static const GLenum ALWAYS = GLenumerator(VK_COMPARE_OP_ALWAYS);
			}
			namespace Format {
				typedef GLenum type_t;
				static const GLenum D32_SFLOAT = GLenumerator(VK_FORMAT_D32_SFLOAT);
				static const GLenum R16G16B16A16_SFLOAT = GLenumerator(VK_FORMAT_R16G16B16A16_SFLOAT);
				static const GLenum R8_UNORM = GLenumerator(VK_FORMAT_R8_UNORM);
				static const GLenum R32_UINT = GLenumerator(VK_FORMAT_R32_UINT);
				static const GLenum R32G32_SINT = GLenumerator(VK_FORMAT_R32G32_SINT);
				static const GLenum R32G32_SFLOAT = GLenumerator(VK_FORMAT_R32G32_SFLOAT);
				static const GLenum R32G32B32_SFLOAT = GLenumerator(VK_FORMAT_R32G32B32_SFLOAT);
				static const GLenum R32G32B32A32_SFLOAT = GLenumerator(VK_FORMAT_R32G32B32A32_SFLOAT);
				static const GLenum R8G8B8A8_UNORM = GLenumerator(VK_FORMAT_R8G8B8A8_UNORM);
			}
			namespace Face {
				typedef GLenum type_t;
				static const GLenum CW = GL_CW;
				static const GLenum CCW = GL_CCW;
			}
			namespace CullMode {
				typedef GLenum type_t;
				static const GLenum NONE = GL_NONE;
				static const GLenum FRONT = GL_FRONT;
				static const GLenum BACK = GL_BACK;
				static const GLenum BOTH = GL_FRONT_AND_BACK;
			}
			namespace Shader {
				typedef GLenum type_t;
				static const GLenum VERTEX = GLenumerator(VK_SHADER_STAGE_VERTEX_BIT);
				static const GLenum TESSELLATION_CONTROL = GLenumerator(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
				static const GLenum TESSELLATION_EVALUATION = GLenumerator(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
				static const GLenum GEOMETRY = GLenumerator(VK_SHADER_STAGE_GEOMETRY_BIT);
				static const GLenum FRAGMENT = GLenumerator(VK_SHADER_STAGE_FRAGMENT_BIT);
				static const GLenum COMPUTE = GLenumerator(VK_SHADER_STAGE_COMPUTE_BIT);
				static const GLenum ALL_GRAPHICS = GLenumerator(VK_SHADER_STAGE_ALL_GRAPHICS);
				static const GLenum ALL = GLenumerator(VK_SHADER_STAGE_ALL);
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
				static const GLenum TRANSFER_SRC = GLenumerator(VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
				static const GLenum TRANSFER_DST = GLenumerator(VK_BUFFER_USAGE_TRANSFER_DST_BIT);
				static const GLenum UNIFORM_TEXEL = GLenumerator(VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT);
				static const GLenum STORAGE_TEXEL = GLenumerator(VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT);
				static const GLenum UNIFORM = GLenumerator(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
				static const GLenum STORAGE = GLenumerator(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
				static const GLenum INDEX = GLenumerator(VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
				static const GLenum VERTEX = GLenumerator(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
				static const GLenum INDIRECT = GLenumerator(VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT);
			}
		}
	}
}