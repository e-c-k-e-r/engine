// GI
uint cascadeIndex( vec3 v ) {
	float x = max3( abs( v ) );
	for ( uint cascade = 0; cascade < CASCADES; ++cascade )
		if ( x / cascadePower(cascade) < 1 - voxelInfo.radianceSizeRecip ) return cascade;
	return CASCADES - 1;
}

vec4 voxelTrace( inout Ray ray, float aperture, float maxDistance ) {
	ray.origin += ray.direction * voxelInfo.radianceSizeRecip * 2 * SQRT2;
#if VXGI_NDC
	ray.origin = vec3( ubo.vxgi.matrix * vec4( ray.origin, 1.0 ) );
	ray.direction = vec3( ubo.vxgi.matrix * vec4( ray.direction, 0.0 ) );
	uint cascade = cascadeIndex(ray.origin);
#else
	uint cascade = cascadeIndex( vec3( ubo.vxgi.matrix * vec4( ray.origin, 1.0 ) ) );
#endif
	const float granularityRecip = ubo.vxgi.granularity; //2.0; // 0.25f * (CASCADES - cascade);
	const float granularity = 1.0f / granularityRecip;
	const float occlusionFalloff = ubo.vxgi.occlusionFalloff; //128.0f;
	const vec3 voxelBounds = voxelInfo.max - voxelInfo.min;
	const vec3 voxelBoundsRecip = 1.0f / voxelBounds;
	const float coneCoefficient = 2.0 * tan(aperture * 0.5);
	const uint maxSteps = uint(voxelInfo.radianceSize * cascadePower(CASCADES-1) * granularityRecip);
	// box
	const vec2 rayBoxInfoA = rayBoxDst( voxelInfo.min * cascadePower(cascade), voxelInfo.max * cascadePower(cascade), ray );
	const vec2 rayBoxInfoB = rayBoxDst( voxelInfo.min * cascadePower(CASCADES-1), voxelInfo.max * cascadePower(CASCADES-1), ray );

	const float tStart = rayBoxInfoA.x;
	const float tEnd = maxDistance > 0 ? min(maxDistance, rayBoxInfoB.y) : rayBoxInfoB.y;
	const float tDelta = voxelInfo.radianceSizeRecip * granularityRecip;
	// marcher
	ray.distance = tStart + tDelta * ubo.vxgi.traceStartOffsetFactor;
	ray.position = vec3(0);

	vec4 radiance = vec4(0);
	vec3 uvw = vec3(0);
	float coneDiameter = coneCoefficient * ray.distance;
	float level = aperture > 0 ? log2( coneDiameter ) : 0;
	vec4 color = vec4(0);
	float occlusion = 0.0;
	uint stepCounter = 0;
	while ( color.a < 1.0 && occlusion < 1.0 && ray.distance < tEnd && stepCounter++ < maxSteps ) {
		ray.distance += tDelta * cascadePower(cascade) * max(1, coneDiameter);
		ray.position = ray.origin + ray.direction * ray.distance;

	#if VXGI_NDC
		uvw = ray.position;
	#else
		uvw = vec3( ubo.vxgi.matrix * vec4( ray.position, 1.0 ) );
	#endif
		cascade = cascadeIndex( uvw );
		uvw = (uvw / cascadePower(cascade)) * 0.5 + 0.5;
		if ( cascade >= CASCADES || uvw.x <  0.0 || uvw.y <  0.0 || uvw.z <  0.0 || uvw.x >= 1.0 || uvw.y >= 1.0 || uvw.z >= 1.0 ) break;
		coneDiameter = coneCoefficient * ray.distance;
		level = aperture > 0 ? log2( coneDiameter ) : 0;
		if ( level >= voxelInfo.mipmapLevels ) break;
		radiance = textureLod(voxelRadiance[nonuniformEXT(cascade)], uvw.xzy, level);
		color += (1.0 - color.a) * radiance;
		occlusion += ((1.0f - occlusion) * radiance.a) / (1.0f + occlusionFalloff * coneDiameter);
	}
	return maxDistance > 0 ? color : vec4(color.rgb, occlusion);
//	return vec4(color.rgb, occlusion);
}
vec4 voxelConeTrace( inout Ray ray, float aperture ) {
	return voxelTrace( ray, aperture, 0 );
}
vec4 voxelTrace( inout Ray ray, float maxDistance ) {
	return voxelTrace( ray, 0, maxDistance );
}
uint voxelShadowsCount = 0;
float voxelShadowFactor( const Light light, float def ) {
	if ( ubo.vxgi.shadows < ++voxelShadowsCount ) return 1.0;

	const float SHADOW_APERTURE = 0.2;
	const float DEPTH_BIAS = 0.0;

	Ray ray;
	ray.direction = normalize( light.position - surface.position.world );
	ray.origin = surface.position.world + ray.direction * 0.5;
	float z = distance( surface.position.world, light.position ) - DEPTH_BIAS;
	return 1.0 - voxelTrace( ray, SHADOW_APERTURE, z ).a;
}
void indirectLighting() {
	voxelInfo.radianceSize = textureSize( voxelRadiance[0], 0 ).x;
	voxelInfo.radianceSizeRecip = 1.0 / voxelInfo.radianceSize;
	voxelInfo.mipmapLevels = log2(voxelInfo.radianceSize) + 1;
#if VXGI_NDC
	voxelInfo.min = vec3( -1 );
	voxelInfo.max = vec3(  1 );
#else
	const mat4 inverseOrtho = inverse( ubo.vxgi.matrix );
	voxelInfo.min = vec3( inverseOrtho * vec4( -1, -1, -1, 1 ) );
	voxelInfo.max = vec3( inverseOrtho * vec4(  1,  1,  1, 1 ) );
#endif

	const vec3 P = surface.position.world;
	const vec3 N = surface.normal.world;

#if 1
	const vec3 right = normalize(orthogonal(N));
	const vec3 up = normalize(cross(right, N));

	const uint CONES_COUNT = 6;
	const vec3 CONES[] = {
		N,
		normalize(N + 0.0f * right + 0.866025f * up),
		normalize(N + 0.823639f * right + 0.267617f * up),
		normalize(N + 0.509037f * right + -0.7006629f * up),
		normalize(N + -0.50937f * right + -0.7006629f * up),
		normalize(N + -0.823639f * right + 0.267617f * up),
	};
#else
	const vec3 ortho = normalize(orthogonal(N));
	const vec3 ortho2 = normalize(cross(ortho, N));

	const vec3 corner = 0.5f * (ortho + ortho2);
	const vec3 corner2 = 0.5f * (ortho - ortho2);

	const uint CONES_COUNT = 9;
	const vec3 CONES[] = {
		N,
		normalize(mix(N, ortho, 0.5)),
		normalize(mix(N, -ortho, 0.5)),
		normalize(mix(N, ortho2, 0.5)),
		normalize(mix(N, -ortho2, 0.5)),
		normalize(mix(N, corner, 0.5)),
		normalize(mix(N, -corner, 0.5)),
		normalize(mix(N, corner2, 0.5)),
		normalize(mix(N, -corner2, 0.5)),
	};
#endif

	const float DIFFUSE_CONE_APERTURE = 2.0 * 0.57735f;
	const float DIFFUSE_INDIRECT_FACTOR = 1.0f / float(CONES_COUNT);
	
	const float SPECULAR_CONE_APERTURE = clamp(tan(PI * 0.5f * surface.material.roughness), 0.0174533f, PI); // tan( R * PI * 0.5f * 0.1f );
	const float SPECULAR_INDIRECT_FACTOR = (1.0 - surface.material.metallic) * 0.5; // 1.0f;
	
	vec4 indirectDiffuse = vec4(0);
	vec4 indirectSpecular = vec4(0);

//	outFragColor.rgb = voxelConeTrace( surface.ray, 0 ).rgb; return;
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
	// 	outFragColor.rgb = indirectDiffuse.rgb; return;
	//	outFragColor.rgb = vec3(surface.material.occlusion); return;
	}
	if ( SPECULAR_INDIRECT_FACTOR > 0.0f ) {
		const vec3 R = reflect( normalize(P - surface.ray.origin), N );
		Ray ray;
		ray.direction = R;
		ray.origin = P; // + ray.direction;
		indirectSpecular = voxelConeTrace( ray, SPECULAR_CONE_APERTURE );
	// 	outFragColor.rgb = indirectSpecular.rgb; return;
		if ( length(indirectSpecular) < 0.0125 ) {
		//	indirectSpecular += (1.0 - indirectSpecular.a) * texture( samplerCubemaps[ubo.indexSkybox], R ) * SPECULAR_CONE_APERTURE;
		}
	}

	surface.material.indirect = indirectDiffuse * DIFFUSE_INDIRECT_FACTOR + indirectSpecular * SPECULAR_INDIRECT_FACTOR;
//	outFragColor.rgb = surface.material.indirect.rgb; return;
#if DEFERRED_SAMPLING
	// deferred sampling doesn't have a blended albedo buffer
	// in place we'll just cone trace behind the window
	if ( surface.material.albedo.a < 1.0 ) {
		Ray ray;
		ray.direction = surface.ray.direction;
		ray.origin = surface.position.world + ray.direction;
		vec4 radiance = voxelConeTrace( ray, surface.material.albedo.a * 0.5 );
		surface.fragment.rgb += (1.0 - surface.material.albedo.a) * radiance.rgb;
	}
#endif
}