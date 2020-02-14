#pragma once

#include <uf/ext/vulkan/vk.h>

namespace ext {
	namespace vulkan {
		namespace enums {
			namespace Compare {
				typedef decltype(VK_COMPARE_OP_NEVER) type_t;
				static const decltype(auto) NEVER = VK_COMPARE_OP_NEVER;
				static const decltype(auto) LESS = VK_COMPARE_OP_LESS;
				static const decltype(auto) EQUAL = VK_COMPARE_OP_EQUAL;
				static const decltype(auto) LESS_OR_EQUAL = VK_COMPARE_OP_LESS_OR_EQUAL;
				static const decltype(auto) GREATER = VK_COMPARE_OP_GREATER;
				static const decltype(auto) NOT_EQUAL = VK_COMPARE_OP_NOT_EQUAL;
				static const decltype(auto) GREATER_OR_EQUAL = VK_COMPARE_OP_GREATER_OR_EQUAL;
				static const decltype(auto) ALWAYS = VK_COMPARE_OP_ALWAYS;
			}
			namespace Format {
				typedef decltype(VK_FORMAT_R32G32_SFLOAT) type_t;
				static const decltype(auto) D32_SFLOAT = VK_FORMAT_D32_SFLOAT;
				static const decltype(auto) R16G16B16A16_SFLOAT = VK_FORMAT_R16G16B16A16_SFLOAT;
				static const decltype(auto) R8_UNORM = VK_FORMAT_R8_UNORM;
				static const decltype(auto) R32_UINT = VK_FORMAT_R32_UINT;
				static const decltype(auto) R32G32_SINT = VK_FORMAT_R32G32_SINT;
				static const decltype(auto) R32G32_SFLOAT = VK_FORMAT_R32G32_SFLOAT;
				static const decltype(auto) R32G32B32_SFLOAT = VK_FORMAT_R32G32B32_SFLOAT;
				static const decltype(auto) R32G32B32A32_SFLOAT = VK_FORMAT_R32G32B32A32_SFLOAT;
				static const decltype(auto) R8G8B8A8_UNORM = VK_FORMAT_R8G8B8A8_UNORM;
			}
			namespace Face {
				typedef decltype(VK_FRONT_FACE_CLOCKWISE) type_t;
				static const decltype(auto) CW = VK_FRONT_FACE_CLOCKWISE;
				static const decltype(auto) CCW = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			}
			namespace CullMode {
				typedef decltype(VK_CULL_MODE_NONE) type_t;
				static const decltype(auto) NONE = VK_CULL_MODE_NONE;
				static const decltype(auto) FRONT = VK_CULL_MODE_FRONT_BIT;
				static const decltype(auto) BACK = VK_CULL_MODE_BACK_BIT;
				static const decltype(auto) BOTH = VK_CULL_MODE_FRONT_AND_BACK;
			}
			namespace Shader {
				typedef decltype(VK_SHADER_STAGE_VERTEX_BIT) type_t;
				static const decltype(auto) VERTEX = VK_SHADER_STAGE_VERTEX_BIT;
				static const decltype(auto) TESSELLATION_CONTROL = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
				static const decltype(auto) TESSELLATION_EVALUATION = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
				static const decltype(auto) GEOMETRY = VK_SHADER_STAGE_GEOMETRY_BIT;
				static const decltype(auto) FRAGMENT = VK_SHADER_STAGE_FRAGMENT_BIT;
				static const decltype(auto) COMPUTE = VK_SHADER_STAGE_COMPUTE_BIT;
				static const decltype(auto) ALL_GRAPHICS = VK_SHADER_STAGE_ALL_GRAPHICS;
				static const decltype(auto) ALL = VK_SHADER_STAGE_ALL;
			}
			namespace PrimitiveTopology {
				typedef decltype(VK_PRIMITIVE_TOPOLOGY_POINT_LIST) type_t;
				static const decltype(auto) POINT_LIST = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
				static const decltype(auto) LINE_LIST = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
				static const decltype(auto) LINE_STRIP = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
				static const decltype(auto) TRIANGLE_LIST = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
				static const decltype(auto) TRIANGLE_STRIP = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
				static const decltype(auto) TRIANGLE_FAN = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
				static const decltype(auto) LINE_LIST_WITH_ADJACENCY = VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
				static const decltype(auto) LINE_STRIP_WITH_ADJACENCY = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
				static const decltype(auto) TRIANGLE_LIST_WITH_ADJACENCY = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
				static const decltype(auto) TRIANGLE_STRIP_WITH_ADJACENCY = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;
				static const decltype(auto) PATCH_LIST = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
			}
			namespace PolygonMode {
				typedef decltype(VK_POLYGON_MODE_FILL) type_t;
				static const decltype(auto) FILL = VK_POLYGON_MODE_FILL;
				static const decltype(auto) LINE = VK_POLYGON_MODE_LINE;
				static const decltype(auto) POINT = VK_POLYGON_MODE_POINT;
			}
			namespace AddressMode {
				typedef decltype(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE) type_t;
				static const decltype(auto) CLAMP_TO_EDGE = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
				static const decltype(auto) CLAMP_TO_BORDER = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
				static const decltype(auto) MIRRORED_REPEAT = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
				static const decltype(auto) REPEAT = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			}
			namespace Filter {
				typedef decltype(VK_FILTER_NEAREST) type_t;
				static const decltype(auto) NEAREST = VK_FILTER_NEAREST;
				static const decltype(auto) LINEAR = VK_FILTER_LINEAR;
			}
			namespace Buffer {
				typedef decltype(VK_BUFFER_USAGE_TRANSFER_SRC_BIT) type_t;
				static const decltype(auto) TRANSFER_SRC = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
				static const decltype(auto) TRANSFER_DST = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
				static const decltype(auto) UNIFORM_TEXEL = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
				static const decltype(auto) STORAGE_TEXEL = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
				static const decltype(auto) UNIFORM = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
				static const decltype(auto) STORAGE = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
				static const decltype(auto) INDEX = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
				static const decltype(auto) VERTEX = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
				static const decltype(auto) INDIRECT = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
			}
		}
	}
}