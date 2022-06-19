#version 460
#extension GL_EXT_ray_tracing : enable
#pragma shader_stage(raygen)
layout (constant_id = 0) const uint PASSES = 2;

#include "../common/macros.h"
#include "../common/structs.h"

layout (binding = 0) uniform UBO {
	EyeMatrices eyes[2];
} ubo;
layout (binding = 1) uniform accelerationStructureEXT inTlas;
layout (binding = 2, rgba8) uniform volatile coherent image2D outImage;

layout (location = 0) rayPayloadEXT vec4 hitValue;

layout( push_constant ) uniform PushBlock {
  uint pass;
  uint draw;
} PushConstant;

void main()  {
	{
		surface.pass = PushConstant.pass;

		const vec2 inUv = (vec2(gl_LaunchIDEXT.xy) + vec2(0.5)) / vec2(gl_LaunchSizeEXT.xy);

		const mat4 iProjectionView = inverse( ubo.eyes[surface.pass].projection * mat4(mat3(ubo.eyes[surface.pass].view)) );
		const vec4 near4 = iProjectionView * (vec4(2.0 * inUv - 1.0, -1.0, 1.0));
		const vec4 far4 = iProjectionView * (vec4(2.0 * inUv - 1.0, 1.0, 1.0));
		const vec3 near3 = near4.xyz / near4.w;
		const vec3 far3 = far4.xyz / far4.w;

		surface.ray.direction = normalize( far3 - near3 );
		surface.ray.origin = ubo.eyes[surface.pass].eyePos.xyz;
	}

	uint rayFlags = gl_RayFlagsOpaqueEXT;
	uint cullMask = 0xFF;
	float tMin = 0.001;
	float tMax = 10000.0;

    hitValue = vec4(1, 0, 1, 1);
    traceRayEXT(inTlas, rayFlags, cullMask, 0, 0, 0, surface.ray.origin.xyz, tMin, surface.ray.direction.xyz, tMax, 0);

	imageStore(outImage, ivec2(gl_LaunchIDEXT.xy), vec4(hitValue));
}