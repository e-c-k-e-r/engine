uint findRegion(vec3 pos) {
	for ( uint i = 0; i < regions.length(); ++i ) {
		Region r = regions[i];
		if ( all(greaterThanEqual(pos, r.minBounds)) && all(lessThanEqual(pos, r.maxBounds)) ) return i;
	}
	return regions.length();
}

vec4 voxelTrace( inout Ray ray, float aperture, float maxDistance ) {
	ray.direction.x = abs(ray.direction.x) < 0.00001 ? 0.00001 : ray.direction.x;
	ray.direction.y = abs(ray.direction.y) < 0.00001 ? 0.00001 : ray.direction.y;
	ray.direction.z = abs(ray.direction.z) < 0.00001 ? 0.00001 : ray.direction.z;

	ray.origin += ray.direction * voxelInfo.radianceSizeRecip * 1.5;

	uint regionIdx = findRegion(ray.origin);
	if (regionIdx == regions.length()) return vec4(0);

	Region region = regions[regionIdx];

	const float granularity = ubo.settings.vxgi.granularity;
	const float occlusionFalloff = ubo.settings.vxgi.occlusionFalloff;
	const float coneCoefficient = 2.0 * tan(aperture * 0.5);

	const uint maxSteps = uint(region.size * granularity);
	const float maxRadiance = 0.90;

	const vec2 rayBoxInfo = rayBoxDst( region.minBounds, region.maxBounds, ray );
	const float tStart = rayBoxInfo.x;
	const float tEnd = maxDistance > 0 ? min(maxDistance, rayBoxInfo.y) : rayBoxInfo.y;

	float voxelWorldSize = (region.maxBounds.x - region.minBounds.x) / float(region.size);
	float tDelta = voxelWorldSize * granularity;

	ray.distance = tStart + tDelta * ubo.settings.vxgi.traceStartOffsetFactor;
	ray.distance += tDelta * rand2(gl_GlobalInvocationID.xy);

	vec4 color = vec4(0);
	float occlusion = 0;
	uint stepCounter = 0;

	while ( color.a < maxRadiance && occlusion < 1.0 && ray.distance < tEnd && stepCounter++ < maxSteps ) {
		float coneDiameter = coneCoefficient * ray.distance;
		float stepScale = max(1.0, (coneDiameter * float(region.size)) * 1.5);

		ray.distance += tDelta * stepScale;
		ray.position = ray.origin + ray.direction * ray.distance;

		if (any(lessThan(ray.position, region.minBounds)) || any(greaterThan(ray.position, region.maxBounds))) {
			regionIdx = findRegion(ray.position);
			if (regionIdx == regions.length()) break;

			region = regions[regionIdx];

			voxelWorldSize = (region.maxBounds.x - region.minBounds.x) / float(region.size);
			tDelta = voxelWorldSize * granularity;
		}

		vec3 uvw = (ray.position - region.minBounds) / (region.maxBounds - region.minBounds);

		float level = aperture > 0 ? log2( coneDiameter * float(region.size) ) : 0;
		vec4 radiance = textureLod(voxelOutput[nonuniformEXT(regionIdx)], uvw, level);

		color.rgb += (1.0 - color.a) * radiance.rgb * radiance.a;
		color.a   += (1.0 - color.a) * radiance.a;

		occlusion += ((1.0f - occlusion) * radiance.a) / (1.0f + occlusionFalloff * coneDiameter);
	}

	vec4 finalColor = maxDistance > 0 ? color : vec4(color.rgb, occlusion);

//	if (any(isnan(finalColor)) || any(isinf(finalColor))) return vec4(0.0);

	return finalColor;
}

vec4 voxelConeTrace( inout Ray ray, float aperture ) {
	return voxelTrace( ray, aperture, 4096.0 );
}

vec4 voxelTrace( inout Ray ray, float maxDistance ) {
	return voxelTrace( ray, 0.0, maxDistance );
}

uint voxelShadowsCount = 0;
float shadowFactorVXGI( const Light light, float def ) {
	if ( ubo.settings.vxgi.shadows < ++voxelShadowsCount ) return 1.0;

	const float SHADOW_APERTURE = 0.2;
	const float DEPTH_BIAS = 0.0;

	Ray ray;
	ray.direction = normalize( light.position - surface.position.world );
	ray.origin = surface.position.world + ray.direction * 0.5;
	float z = distance( surface.position.world, light.position ) - DEPTH_BIAS;
	return 1.0 - voxelTrace( ray, SHADOW_APERTURE, z ).a;
}
void indirectLightingVXGI() {
	voxelInfo.radianceSize = textureSize( voxelOutput[0], 0 ).x;
	voxelInfo.radianceSizeRecip = 1.0 / voxelInfo.radianceSize;
	voxelInfo.mipmapLevels = log2(voxelInfo.radianceSize) + 1;

	voxelInfo.min = vec3( -1.0 );
	voxelInfo.max = vec3(  1.0 );

	vec4 indirectDiffuse = vec4(0);
	vec4 indirectSpecular = vec4(0);

	const vec3 P = surface.position.world;
	const vec3 N = surface.normal.world;

	const vec3 right = normalize(orthogonal(N));
	const vec3 up = normalize(cross(right, N));

#if 0
	{
		Ray ray;
		ray.direction = N;
		ray.origin = P + N * (voxelInfo.radianceSizeRecip * 4.0);

		indirectDiffuse = voxelConeTrace(ray, 1.0f);
		surface.material.occlusion += 1.0 - clamp(indirectDiffuse.a, 0.0, 1.0);
	}
#else
	const uint CONES_COUNT = 4;
	const vec3 CONES[] = {
		normalize(N + right + up),
		normalize(N - right + up),
		normalize(N + right - up),
		normalize(N - right - up)
	};

	const float DIFFUSE_CONE_APERTURE = 1.1547;
	const float DIFFUSE_INDIRECT_FACTOR = 1.0f / float(CONES_COUNT) * 0.125f;

	if ( DIFFUSE_INDIRECT_FACTOR > 0.0f ) {
		float weight = PI * 0.25f;
		for ( uint i = 0; i < CONES_COUNT; ++i ) {
			Ray ray;
			ray.direction = CONES[i].xyz;
			ray.origin = P;
			indirectDiffuse += voxelConeTrace( ray, DIFFUSE_CONE_APERTURE ) * weight;
			weight = PI * 0.15f;
		}
		surface.material.occlusion += 1.0 - clamp(indirectDiffuse.a, 0.0, 1.0);
	}
	indirectDiffuse *= DIFFUSE_INDIRECT_FACTOR;
#endif

	const float SPECULAR_CONE_APERTURE = clamp(tan(PI * 0.5f * surface.material.roughness), 0.0174533f, PI);
	const float SPECULAR_INDIRECT_FACTOR = (1.0f - surface.material.metallic) * (1.0f - surface.material.roughness);
	if ( SPECULAR_INDIRECT_FACTOR > 0.0f ) {
		const vec3 R = reflect( normalize(P - surface.ray.origin), N );
		Ray ray;
		ray.direction = R;
		ray.origin = P;
		indirectSpecular = voxelConeTrace( ray, SPECULAR_CONE_APERTURE );
	}
	indirectSpecular *= SPECULAR_INDIRECT_FACTOR;

	{
		const vec3 V = normalize(surface.ray.origin - surface.position.world);
		const vec3 N_dir = surface.normal.world;
		const float NdotV = max(dot(N_dir, V), 0.0);

		const vec3 F0 = mix(vec3(0.04), surface.material.albedo.rgb, surface.material.metallic);
		const vec3 F = fresnelSchlick(F0, NdotV);

		indirectDiffuse.rgb *= (1.0 - F) * (1.0 - surface.material.metallic) * surface.material.occlusion;
	}

	surface.material.indirect += indirectDiffuse + indirectSpecular;

#if !RT
	if ( 0.1 < surface.material.albedo.a && surface.material.albedo.a < 1.0 ) {
		Ray ray;
		ray.direction = surface.ray.direction;
		ray.origin = surface.position.world + ray.direction;
		vec4 radiance = voxelConeTrace( ray, surface.material.albedo.a * 0.5 );
		surface.fragment.rgb += (1.0 - surface.material.albedo.a) * radiance.rgb;
	}
#endif
}