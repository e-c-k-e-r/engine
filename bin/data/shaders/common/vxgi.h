// GI
float cascadeScales[CASCADES];
float cascadeScalesInv[CASCADES];
void precomputeCascades() {
	for ( int i = 0; i < CASCADES; ++i ) {
		cascadeScales[i] = cascadePower(i);
		cascadeScalesInv[i] = 1.0 / cascadeScales[i];
	}
}
vec4 voxelTrace( inout Ray ray, float aperture, float maxDistance ) {
	ray.origin += ray.direction * voxelInfo.radianceSizeRecip * 1.5;

#if VXGI_NDC
	ray.origin = vec3( ubo.settings.vxgi.matrix * vec4( ray.origin, 1.0 ) );
	ray.direction = vec3( ubo.settings.vxgi.matrix * vec4( ray.direction, 0.0 ) );
	float absMax = max3(abs(ray.origin));
#else
	const float voxelOrigin = vec3( ubo.settings.vxgi.matrix * vec4( ray.origin, 1.0 ) );
	float absMax = max3(abs(voxelOrigin));
#endif

	uint cascade = 0;
	for ( uint c = 0; c < CASCADES - 1; ++c ) {
		if ( absMax * cascadeScalesInv[c] < (1.0 - voxelInfo.radianceSizeRecip)) {
			cascade = c;
			break;
		}
		cascade = CASCADES - 1;
	}

	const float maxCascadeScale = cascadeScales[CASCADES-1];
	float currentCascadeScale = cascadeScales[cascade];
	float currentCascadeScaleInv = cascadeScalesInv[cascade];

	const float granularity = ubo.settings.vxgi.granularity;
	const float occlusionFalloff = ubo.settings.vxgi.occlusionFalloff;
	const float coneCoefficient = 2.0 * tan(aperture * 0.5);
	
	const uint maxSteps = uint(voxelInfo.radianceSize * maxCascadeScale * granularity);
	const float maxRadiance = 0.90;
	// box
	const vec2 rayBoxInfoA = rayBoxDst( voxelInfo.min * currentCascadeScale, voxelInfo.max * currentCascadeScale, ray );
	const vec2 rayBoxInfoB = rayBoxDst( voxelInfo.min * maxCascadeScale, voxelInfo.max * maxCascadeScale, ray );

	const float tStart = rayBoxInfoA.x;
	const float tEnd = maxDistance > 0 ? min(maxDistance, rayBoxInfoB.y) : rayBoxInfoB.y;
	const float tDelta = voxelInfo.radianceSizeRecip * granularity;

	// marcher
	ray.distance = tStart + tDelta * ubo.settings.vxgi.traceStartOffsetFactor;
	ray.position = vec3(0);

	vec4 radiance = vec4(0);
	vec3 uvw = vec3(0);
	float coneDiameter = coneCoefficient * ray.distance;
	float level = aperture > 0 ? log2( coneDiameter ) : 0;
	vec4 color = vec4(0);
	float occlusion = 0;
	uint stepCounter = 0;
	
	ray.distance += tDelta * rand2(gl_GlobalInvocationID.xy);

	while ( color.a < maxRadiance && occlusion < 1.0 && ray.distance < tEnd && stepCounter++ < maxSteps ) {
		float stepScale = max(1.0, (coneDiameter * currentCascadeScale) * 1.5);
		ray.distance += tDelta * stepScale;
		ray.position = ray.origin + ray.direction * ray.distance;

		absMax = max3(abs(ray.position));
		if ( absMax * currentCascadeScaleInv > 0.99 ) {
			if ( ++cascade >= CASCADES ) break;

			currentCascadeScale = cascadeScales[cascade];
			currentCascadeScaleInv = cascadeScalesInv[cascade];

			ray.distance += tDelta * currentCascadeScale;
			continue;
		}

		uvw = ray.position * currentCascadeScaleInv * 0.5 + 0.5;
		coneDiameter = coneCoefficient * ray.distance;
		level = aperture > 0 ? log2( coneDiameter ) : 0;

		radiance = textureLod(voxelOutput[nonuniformEXT(cascade)], uvw.xzy, level);
		
		color.rgb += (1.0 - color.a) * radiance.rgb * radiance.a;
		color.a   += (1.0 - color.a) * radiance.a;

		occlusion += ((1.0f - occlusion) * radiance.a) / (1.0f + occlusionFalloff * coneDiameter);
	}
	return maxDistance > 0 ? color : vec4(color.rgb, occlusion);
}
vec4 voxelConeTrace( inout Ray ray, float aperture ) {
	return voxelTrace( ray, aperture, 0 );
}
vec4 voxelTrace( inout Ray ray, float maxDistance ) {
	return voxelTrace( ray, 0, maxDistance );
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
	precomputeCascades();

	voxelInfo.radianceSize = textureSize( voxelOutput[0], 0 ).x;
	voxelInfo.radianceSizeRecip = 1.0 / voxelInfo.radianceSize;
	voxelInfo.mipmapLevels = log2(voxelInfo.radianceSize) + 1;

#if VXGI_NDC
	voxelInfo.min = vec3( -1 );
	voxelInfo.max = vec3(  1 );
#else
	const mat4 inverseOrtho = inverse( ubo.settings.vxgi.matrix );
	voxelInfo.min = vec3( inverseOrtho * vec4( -1, -1, -1, 1 ) );
	voxelInfo.max = vec3( inverseOrtho * vec4(  1,  1,  1, 1 ) );
#endif

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
			ray.origin = P; // + ray.direction;
			indirectDiffuse += voxelConeTrace( ray, DIFFUSE_CONE_APERTURE ) * weight;
			weight = PI * 0.15f;
		}
		surface.material.occlusion += 1.0 - clamp(indirectDiffuse.a, 0.0, 1.0);
	}
	indirectDiffuse *= DIFFUSE_INDIRECT_FACTOR;
#endif

	const float SPECULAR_CONE_APERTURE = clamp(tan(PI * 0.5f * surface.material.roughness), 0.0174533f, PI); // tan( R * PI * 0.5f * 0.1f );
	const float SPECULAR_INDIRECT_FACTOR = (1.0f - surface.material.metallic) * (1.0f - surface.material.roughness); // * 0.25; // 1.0f;
	if ( SPECULAR_INDIRECT_FACTOR > 0.0f ) {
		const vec3 R = reflect( normalize(P - surface.ray.origin), N );
		Ray ray;
		ray.direction = R;
		ray.origin = P; // + ray.direction;
		indirectSpecular = voxelConeTrace( ray, SPECULAR_CONE_APERTURE );
	}
	indirectSpecular *= SPECULAR_INDIRECT_FACTOR;

	// Calculate Fresnel
	{
		const vec3 V = normalize(surface.ray.origin - surface.position.world);
		const vec3 N = surface.normal.world;
		const float NdotV = max(dot(N, V), 0.0);

		const vec3 F0 = mix(vec3(0.04), surface.material.albedo.rgb, surface.material.metallic);
		const vec3 F = fresnelSchlick(F0, NdotV);

		indirectDiffuse.rgb *= (1.0 - F) * (1.0 - surface.material.metallic) * surface.material.occlusion;
	}

	surface.material.indirect += indirectDiffuse + indirectSpecular;
	
	// deferred sampling doesn't have a blended albedo buffer
	// in place we'll just cone trace behind the window
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