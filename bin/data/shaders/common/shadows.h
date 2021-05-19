const vec2 poissonDisk[16] = vec2[]( 
   vec2( -0.94201624, -0.39906216 ), 
   vec2( 0.94558609, -0.76890725 ), 
   vec2( -0.094184101, -0.92938870 ), 
   vec2( 0.34495938, 0.29387760 ), 
   vec2( -0.91588581, 0.45771432 ), 
   vec2( -0.81544232, -0.87912464 ), 
   vec2( -0.38277543, 0.27676845 ), 
   vec2( 0.97484398, 0.75648379 ), 
   vec2( 0.44323325, -0.97511554 ), 
   vec2( 0.53742981, -0.47373420 ), 
   vec2( -0.26496911, -0.41893023 ), 
   vec2( 0.79197514, 0.19090188 ), 
   vec2( -0.24188840, 0.99706507 ), 
   vec2( -0.81409955, 0.91437590 ), 
   vec2( 0.19984126, 0.78641367 ), 
   vec2( 0.14383161, -0.14100790 ) 
);

#ifndef SHADOW_SAMPLES
	#define SHADOW_SAMPLES ubo.shadowSamples
#endif
#if VXGI
	float voxelShadowFactor( const Light, float def );
#endif

#if CUBEMAPS
float omniShadowMap( const Light light, float def ) {
	return 1.0;
}
#else
float omniShadowMap( const Light light, float def ) {
	float factor = 1.0;

	const mat4 views[6] = {
		mat4( 0, 0, 1, 0, 0, 1, 0, 0,-1, 0, 0, 0, 0, 0, 0, 1 ),
		mat4( 0, 0,-1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1 ),
		mat4( 1, 0, 0, 0, 0, 0, 1, 0, 0,-1, 0, 0, 0, 0, 0, 1 ),
		mat4( 1, 0, 0, 0, 0, 0,-1, 0, 0, 1, 0, 0, 0, 0, 0, 1 ),
		mat4( 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 ),
		mat4(-1, 0, 0, 0, 0, 1, 0, 0, 0, 0,-1, 0, 0, 0, 0, 1 ),
	};
	
	const vec3 D = normalize(surface.position.world - light.position);
	const vec3 N = abs(D);
	uint A = N.y > N.x ? 1 : 0;
	A = N.z > N[A] ? 2 : A;
	uint index = A * 2;
	if ( D[A] < 0.0 ) ++index;
	
	vec4 positionClip = light.projection * views[index] * vec4(surface.position.world - light.position, 1.0);
	positionClip.xy /= positionClip.w;

	if ( positionClip.x < -1 || positionClip.x >= 1 ) return 0.0;
	if ( positionClip.y < -1 || positionClip.y >= 1 ) return 0.0;
	if ( positionClip.z < -1 || positionClip.z >= 1 ) return 0.0;

	const float eyeDepthScale = 1.0;
	const float sampledDepthScale = light.view[1][1]; // light view matricies will incorporate scaling factors for some retarded reason, so we need to rescale it by grabbing from here, hopefully it remains coherent between all light matrices to ever exist in engine

	const float bias = light.depthBias;
	const float eyeDepth = abs(positionClip.z / positionClip.w) * eyeDepthScale;

	const vec3 sampleOffsetDirections[20] = {
		vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
		vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
		vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
		vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
		vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
	};
	
	float sampled = 0;
	const int samples = int(SHADOW_SAMPLES);
	if ( light.typeMap == 1 ) {
		if ( samples < 1 ) {
			sampled = texture(samplerCubemaps[nonuniformEXT(light.indexMap)], D).r * sampledDepthScale;
		} else {
			for ( int i = 0; i < samples; ++i ) {
				const int idx = int( float(samples) * random(floor(surface.position.world.xyz * 1000.0), i)) % samples;
				vec2 poisson = poissonDisk[idx] / 700.0;
				vec3 P = vec3( poisson.xy, (poisson.x + poisson.y) * 0.5 );
				sampled = texture(samplerCubemaps[nonuniformEXT(light.indexMap)], D + P ).r * sampledDepthScale;
				if ( eyeDepth < sampled - bias ) factor -= 1.0 / samples;
			}
			return factor;
		}
	} else if ( light.typeMap == 2 ) {
		const vec2 uv = positionClip.xy * 0.5 + 0.5;
		if ( samples < 1 ) {
			sampled = texture(samplerTextures[nonuniformEXT(light.indexMap + index)], uv).r * sampledDepthScale;
		} else {
			for ( int i = 0; i < samples; ++i ) {
				const int idx = int( float(samples) * random(floor(surface.position.world.xyz * 1000.0), i)) % samples;
				sampled = texture(samplerTextures[nonuniformEXT(light.indexMap + index)], uv + poissonDisk[idx] / 700.0 ).r * sampledDepthScale;
				if ( eyeDepth < sampled - bias ) factor -= 1.0 / samples;
			}
			return factor;
		}
	}
	return eyeDepth < sampled - bias ? 0.0 : factor;
}
#endif
float shadowFactor( const Light light, float def ) {
	if ( light.typeMap != 0 ) return omniShadowMap( light, def );
	if ( !validTextureIndex(light.indexMap) )
	#if VXGI
		return voxelShadowFactor( light, def );
	#else
		return 1.0;
	#endif

	vec4 positionClip = light.projection * light.view * vec4(surface.position.world, 1.0);
	positionClip.xyz /= positionClip.w;

	if ( positionClip.x < -1 || positionClip.x >= 1 ) return def; //0.0;
	if ( positionClip.y < -1 || positionClip.y >= 1 ) return def; //0.0;
	if ( positionClip.z <= 0 || positionClip.z >= 1 ) return def; //0.0;

	float factor = 1.0;

	// spot light
	if ( abs(light.type) == 2 || abs(light.type) == 3 ) {
		const float dist = length( positionClip.xy );
		if ( dist > 0.5 ) return def; //0.0;
		
		// spot light with attenuation
		if ( abs(light.type) == 3 ) {
			factor = 1.0 - (pow(dist * 2,2.0));
		}
	}
	
	const vec2 uv = positionClip.xy * 0.5 + 0.5;
	const float bias = light.depthBias;
	const float eyeDepth = positionClip.z;
	const int samples = int(SHADOW_SAMPLES);
	if ( samples < 1 ) return eyeDepth < texture(samplerTextures[nonuniformEXT(light.indexMap)], uv).r - bias ? 0.0 : factor;
	for ( int i = 0; i < samples; ++i ) {
		const int index = int( float(samples) * random(floor(surface.position.world.xyz * 1000.0), i)) % samples;
		const float lightDepth = texture(samplerTextures[nonuniformEXT(light.indexMap)], uv + poissonDisk[index] / 700.0 ).r;
		if ( eyeDepth < lightDepth - bias ) factor -= 1.0 / samples;
	}
	return factor;
}