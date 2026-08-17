bool inRegion( vec3 p, vec3 minBounds, vec3 maxBounds ) {
	return all(greaterThanEqual(p, minBounds)) && all(lessThanEqual(p, maxBounds));
}
uint findRegion(vec3 pos) {
	for ( uint i = 0; i < regions.length(); ++i ) {
		Region r = regions[i];
		if ( inRegion( pos, r.minBounds, r.maxBounds ) ) return i;
	}
	return regions.length();
}

vec4 voxelTrace( inout Ray ray, float aperture, float maxDistance ) {
	uint regionIdx = findRegion(ray.origin);
	if ( regionIdx == regions.length() ) return vec4(0);
	Region region = regions[regionIdx];

	const float granularity = ubo.settings.vxgi.granularity;
	const float occlusionFalloff = ubo.settings.vxgi.occlusionFalloff;
	const float coneCoefficient = 2.0 * tan(aperture * 0.5);

	const uint maxSteps = uint(region.resolution * granularity);
	const float maxRadiance = 0.90;

	const vec2 rayBoxInfo = rayBoxDst( region.minBounds, region.maxBounds, ray );
	const float tStart = max(0.0, rayBoxInfo.x);
	const float tEnd = maxDistance > 0 ? min(maxDistance, rayBoxInfo.y) : rayBoxInfo.y;

	vec3 regionExtent = max(region.maxBounds - region.minBounds, vec3(0.0001));
	vec3 invRegionExtent = vec3(1.0) / regionExtent;

	float voxelWorldSize = regionExtent.x / float(region.resolution);
	float tDelta = max(voxelWorldSize * granularity, 0.0001);

	float startBias = (aperture == 0.0) ? -tDelta : 0.0;
	ray.distance = tStart + startBias;
	ray.distance += tDelta * rand2(gl_GlobalInvocationID.xy) * 0.5;

	vec4 color = vec4(0);
	float occlusion = 0;
	uint stepCounter = 0;

	while ( color.a < maxRadiance && occlusion < 1.0 && ray.distance < tEnd && stepCounter++ < maxSteps ) {
		float coneDiameter = coneCoefficient * ray.distance;

		float diameterInVoxels = coneDiameter / voxelWorldSize;
		float stepScale = max(1.0, diameterInVoxels * 1.5);

		ray.distance += tDelta * stepScale;
		ray.position = ray.origin + ray.direction * ray.distance;

		if ( !inRegion( ray.position, region.minBounds, region.maxBounds ) ) {
			regionIdx = findRegion(ray.position);
			if (regionIdx == regions.length()) break;

			region = regions[regionIdx];

			voxelWorldSize = (region.maxBounds.x - region.minBounds.x) / float(region.resolution);
			tDelta = voxelWorldSize * granularity;
		}

		vec3 uvw = (ray.position - region.minBounds) * invRegionExtent;

		float level = aperture > 0.0 ? log2(max(1.0, diameterInVoxels)) : 0.0;
		vec4 radiance = textureLod(voxelOutput[nonuniformEXT(regionIdx)], uvw, level);

		radiance.a = 1.0 - pow(1.0 - radiance.a, stepScale);
		color.rgb += (1.0 - color.a) * radiance.rgb * radiance.a;
		color.a   += (1.0 - color.a) * radiance.a;

		occlusion += ((1.0f - occlusion) * radiance.a) / (1.0f + occlusionFalloff * coneDiameter);
	}

	return maxDistance > 0 ? color : vec4(color.rgb, occlusion);
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
	const vec3 P = surface.position.world;
	const vec3 N = surface.normal.world;
	const vec3 V = normalize(surface.ray.origin - surface.position.world);

	uint regionIdx = findRegion(P);
	float bias = 0.1;
	if (regionIdx < regions.length()) {
		Region r = regions[regionIdx];
		bias = ((r.maxBounds.x - r.minBounds.x) / float(r.resolution)) * 1.5;
	}
	vec3 biasedP = P + N * bias;

	vec4 indirectDiffuse = vec4(0.0);
	vec4 indirectSpecular = vec4(0.0);

	const vec3 right = normalize(orthogonal(N));
	const vec3 up = normalize(cross(right, N));

	const uint CONES_COUNT = 4;
	const vec3 CONES[] = {
		normalize(N + right + up),
		normalize(N - right + up),
		normalize(N + right - up),
		normalize(N - right - up)
	};

	const float DIFFUSE_CONE_APERTURE = 1.1547;

	for ( uint i = 0; i < CONES_COUNT; ++i ) {
		Ray ray;
		ray.direction = CONES[i];
		ray.origin = biasedP;

		indirectDiffuse += voxelConeTrace( ray, DIFFUSE_CONE_APERTURE ) * 0.25;
	}
	surface.material.occlusion *= (1.0 - clamp(indirectDiffuse.a, 0.0, 1.0));

	float specFactor = (1.0 - surface.material.roughness);

	if ( specFactor > 0.0 ) {
		vec3 R = reflect(-V, N);
		if ( dot( R, N ) < 0.0 ) {
			R = normalize( R - 2.0 * dot( R, N ) * N );
		}

		Ray ray;
		ray.direction = R;
		ray.origin = biasedP;

		float specAperture = clamp(tan(PI * 0.5 * surface.material.roughness), 0.0174533, PI);
		indirectSpecular = voxelConeTrace( ray, specAperture );
	}

	float NdotV = max(dot(N, V), 0.0);
	vec3 F0 = mix(vec3(0.04), surface.material.albedo.rgb, surface.material.metallic);
	vec3 F = fresnelSchlick(F0, NdotV);
	vec3 kD = (vec3(1.0) - F) * (1.0 - surface.material.metallic);

	indirectDiffuse.rgb *= kD * surface.material.albedo.rgb;
	indirectSpecular.rgb *= F * specFactor;

	const float GI_INTENSITY_BOOST = 1.0;
	surface.material.indirect += (indirectDiffuse + indirectSpecular) * GI_INTENSITY_BOOST;

#if !RT
	if ( 0.1 < surface.material.albedo.a && surface.material.albedo.a < 1.0 ) {
		Ray ray;
		ray.direction = surface.ray.direction;
		ray.origin = P + ray.direction * bias;
		vec4 radiance = voxelConeTrace( ray, surface.material.albedo.a * 0.5 );
		surface.fragment.rgb += (1.0 - surface.material.albedo.a) * radiance.rgb;
	}
#endif
}