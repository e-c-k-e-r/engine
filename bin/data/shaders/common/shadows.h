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

float shadowFactor( const Light light, float def ) {
	if ( !validTextureIndex(light.mapIndex) ) {
	#if VXGI
		return voxelShadowFactor( light, def );
	#else
		return 1.0;
	#endif
	}

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
	if ( samples < 1 ) return eyeDepth < texture(samplerTextures[nonuniformEXT(light.mapIndex)], uv).r - bias ? 0.0 : factor;
	for ( int i = 0; i < samples; ++i ) {
		const int index = int( float(samples) * random(floor(surface.position.world.xyz * 1000.0), i)) % samples;
		const float lightDepth = texture(samplerTextures[nonuniformEXT(light.mapIndex)], uv + poissonDisk[index] / 700.0 ).r;
		if ( eyeDepth < lightDepth - bias ) factor -= 1.0 / samples;
	}
	return factor;
}