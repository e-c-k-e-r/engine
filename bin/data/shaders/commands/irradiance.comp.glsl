#version 450
#pragma shader_stage(compute)
layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(binding = 0) uniform samplerCube environmentMap;
layout(binding = 1, rgba16f) uniform writeonly image2DArray irradianceMap;

const float PI = 3.14159265359;

vec3 getDirection(uint face, vec2 uv) {
	vec3 dir;
	uv = uv * 2.0 - 1.0;
	switch(face) {
		case 0: dir = vec3( 1.0, -uv.y, -uv.x); break; // POSITIVE_X
		case 1: dir = vec3(-1.0, -uv.y,  uv.x); break; // NEGATIVE_X
		case 2: dir = vec3( uv.x,  1.0,  uv.y); break; // POSITIVE_Y
		case 3: dir = vec3( uv.x, -1.0, -uv.y); break; // NEGATIVE_Y
		case 4: dir = vec3( uv.x, -uv.y,  1.0); break; // POSITIVE_Z
		case 5: dir = vec3(-uv.x, -uv.y, -1.0); break; // NEGATIVE_Z
	}
	return normalize(dir);
}

void main() {
	ivec3 storePos = ivec3(gl_GlobalInvocationID);
	ivec2 size = imageSize(irradianceMap).xy;
	if (storePos.x >= size.x || storePos.y >= size.y) return;

	vec2 uv = (vec2(storePos.xy) + 0.5) / vec2(size);
	vec3 normal = getDirection(storePos.z, uv);

	vec3 irradiance = vec3(0.0);
	vec3 up	= vec3(0.0, 1.0, 0.0);
	vec3 right = normalize(cross(up, normal));
	up		 = normalize(cross(normal, right));

	float sampleDelta = 0.025;
	float nrSamples = 0.0;

	for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta) {
		for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta) {
			vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
			vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;

			irradiance += texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta);
			nrSamples++;
		}
	}
	irradiance = PI * irradiance * (1.0 / float(nrSamples));

	imageStore(irradianceMap, storePos, vec4(irradiance, 1.0));
}