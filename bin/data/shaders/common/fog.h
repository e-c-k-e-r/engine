// Perlin Fog
void fog( in Ray ray, inout vec3 i, float scale ) {
	if ( ubo.fog.stepScale <= 0 || ubo.fog.range.x == 0 || ubo.fog.range.y == 0 ) return;
#if FOG_RAY_MARCH
	const float range = ubo.fog.range.y;
	const vec3 boundsMin = vec3(-range,-range,-range) + ray.origin;
	const vec3 boundsMax = vec3(range,range,range) + ray.origin;
	const int numSteps = int(length(boundsMax - boundsMin) * ubo.fog.stepScale );

	const vec2 rayBoxInfo = rayBoxDst( boundsMin, boundsMax, ray );
	const float dstToBox = rayBoxInfo.x;
	const float dstInsideBox = rayBoxInfo.y;
	const float depth = surface.position.eye.z;

	const float aperture = PI * 0.5;
	const float coneCoefficient = 2.0 * tan(aperture * 0.5);

	// march
	if ( 0 <= dstInsideBox && dstToBox <= depth ) {
		float stepSize = dstInsideBox / numSteps;
		float dstLimit = min( depth - dstToBox, dstInsideBox );
		float totalDensity = 0;
		float transmittance = 1;
		float lightFactor = scale;
		float coneDiameter = coneCoefficient * ray.distance;
		float level = aperture > 0 ? log2( coneDiameter ) : 0;
		float density = 0;
		vec3 uvw;
		ray.distance = dstToBox;
		while ( ray.distance < dstLimit ) {
			ray.distance += stepSize;
			ray.position = ray.origin + ray.direction * ray.distance;
			coneDiameter = coneCoefficient * ray.distance;
			level = aperture > 0 ? log2( coneDiameter ) : 0;
			uvw = ray.position * ubo.fog.densityScale * 0.001 + ubo.fog.offset * 0.01;
			density = max(0, textureLod(samplerNoise, uvw, level).r - ubo.fog.densityThreshold) * ubo.fog.densityMultiplier;
			if ( density > 0 ) {
				density = exp(-density * stepSize * ubo.fog.absorbtion);
				transmittance *= density;
				lightFactor *= density;
				if ( transmittance < 0.1 ) break;
			}
		}
		i.rgb = mix(ubo.fog.color.rgb, i.rgb, transmittance );
	}
#endif
#if FOG_BASIC
	const vec3 color = ubo.fog.color.rgb;
	const float inner = ubo.fog.range.x;
	const float outer = ubo.fog.range.y * scale;
	const float distance = length(-surface.position.eye);
	const float factor = clamp( (distance - inner) / (outer - inner), 0.0, 1.0 );

	i.rgb = mix(i.rgb, color, factor);
#endif
}