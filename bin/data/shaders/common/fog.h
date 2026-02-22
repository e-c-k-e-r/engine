// Perlin Fog
void fog( in Ray ray, inout vec3 i, float scale ) {
	if ( ubo.settings.fog.stepScale <= 0 || ubo.settings.fog.range.x == 0 || ubo.settings.fog.range.y == 0 ) return;
#if FOG_RAY_MARCH
	if ( ubo.settings.fog.stepScale <= 0 || ubo.settings.fog.range.y <= 0 ) return;

	const float range = ubo.settings.fog.range.y;
	const vec3 boundsMin = ray.origin - vec3(range);
	const vec3 boundsMax = ray.origin + vec3(range);

	const vec2 rayBoxInfo = rayBoxDst( boundsMin, boundsMax, ray );
	const float dstToBox = rayBoxInfo.x;
	const float dstInsideBox = rayBoxInfo.y;
	const float depth = surface.position.eye.z;

	if ( dstInsideBox < 0 || dstToBox > depth ) return;

	const float marchLimit = min( depth - dstToBox, dstInsideBox );
	const int MAX_STEPS = 32;

	const float stepSize = marchLimit / float(MAX_STEPS);
	const vec3 stepVec = ray.direction * stepSize;
	const float jitter = rand2(gl_GlobalInvocationID.xy);

	const float densityScale = ubo.settings.fog.densityScale * 0.001;
	const vec3 densityOffset = ubo.settings.fog.offset * 0.01;
	const float densityThreshold = ubo.settings.fog.densityThreshold;
	const float densityMult = ubo.settings.fog.densityMultiplier;
	const float absorption = ubo.settings.fog.absorbtion;
	
	vec3 currentPos = ray.origin + (ray.direction * (dstToBox + stepSize * jitter));
	float transmittance = 1.0;

	for ( uint j = 0; j < MAX_STEPS; ++j ) {
		vec3 uvw = currentPos * densityScale + densityOffset;
		float noiseVal = textureLod(samplerNoise, uvw, 0.0).r;
		float density = max(0.0, noiseVal - densityThreshold) * densityMult;
		if ( density > 0.0 ) {
			float beer = exp(-density * stepSize * absorption);
			transmittance *= beer;
			if ( transmittance < 0.01 ) break;
		}
		currentPos += stepVec;
	}

	i.rgb = mix(ubo.settings.fog.color.rgb, i.rgb, transmittance );
#endif
#if FOG_BASIC
	const vec3 color = ubo.settings.fog.color.rgb;
	const float inner = ubo.settings.fog.range.x;
	const float outer = ubo.settings.fog.range.y * scale;
	const float distance = length(-surface.position.eye);
	const float factor = clamp( (distance - inner) / (outer - inner), 0.0, 1.0 );

	i.rgb = mix(i.rgb, color, factor);
#endif
}