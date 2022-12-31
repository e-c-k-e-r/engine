// RT
RayTracePayload payload;

void populateSurface( RayTracePayload payload );
void directLighting();

void trace( Ray ray, float tMin, float tMax ) {
	uint rayFlags = gl_RayFlagsOpaqueEXT;
	uint cullMask = 0xFF;
	
	payload.hit = false;
	surface.position.eye.z = tMax;
//	traceRayEXT(tlas, rayFlags, cullMask, 0, 0, 0, ray.origin, tMin, ray.direction, tMax, 0);
	rayQueryEXT rayQuery;
	rayQueryInitializeEXT(rayQuery, tlas, rayFlags, cullMask, ray.origin, tMin, ray.direction, tMax);

	while(rayQueryProceedEXT(rayQuery));

	payload.hit = rayQueryGetIntersectionTypeEXT(rayQuery, true) != gl_RayQueryCommittedIntersectionNoneEXT;
	payload.instanceID = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, true); //rayQueryGetIntersectionInstanceIdEXT(rayQuery, true);
	payload.primitiveID = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, true);
	payload.attributes = rayQueryGetIntersectionBarycentricsEXT(rayQuery, true);
}
void trace( Ray ray, float tMin ) {
	uint rayFlags = gl_RayFlagsOpaqueEXT;
	uint cullMask = 0xFF;

	float tMax = ubo.settings.rt.defaultRayBounds.y;
	
	payload.hit = false;
	surface.position.eye.z = tMax;
//	traceRayEXT(tlas, rayFlags, cullMask, 0, 0, 0, ray.origin, tMin, ray.direction, tMax, 0);
	rayQueryEXT rayQuery;
	rayQueryInitializeEXT(rayQuery, tlas, rayFlags, cullMask, ray.origin, tMin, ray.direction, tMax);

	while(rayQueryProceedEXT(rayQuery));

	payload.hit = rayQueryGetIntersectionTypeEXT(rayQuery, true) != gl_RayQueryCommittedIntersectionNoneEXT;
	payload.instanceID = rayQueryGetIntersectionInstanceCustomIndexEXT(rayQuery, true); //rayQueryGetIntersectionInstanceIdEXT(rayQuery, true);
	payload.primitiveID = rayQueryGetIntersectionPrimitiveIndexEXT(rayQuery, true);
	payload.attributes = rayQueryGetIntersectionBarycentricsEXT(rayQuery, true);
}

void trace( Ray ray ) {
	trace( ray, ubo.settings.rt.defaultRayBounds.x, ubo.settings.rt.defaultRayBounds.y );
}

vec4 traceStep( Ray ray ) {
	Surface previousSurface = surface;
	float eyeDepth = 0;
	vec4 outFrag = vec4(0);

	// initial condition
	{
		trace( ray );

		if ( payload.hit ) {
			populateSurface( payload );
			directLighting();
		} else if (  0 <= ubo.settings.lighting.indexSkybox && ubo.settings.lighting.indexSkybox < CUBEMAPS ) {
			surface.fragment = texture( samplerCubemaps[ubo.settings.lighting.indexSkybox], ray.direction );
			surface.fragment.a = 4096;
			surface.position.eye.z /= 8;
		} else {
			surface.fragment = vec4(ubo.settings.lighting.ambient.rgb, 0.5);
		}
	#if FOG
		fog( ray, surface.fragment.rgb, surface.fragment.a );
	#endif
		outFrag = surface.fragment;
		eyeDepth = surface.position.eye.z;
	}

#if 0
	// "transparency"
	if ( payload.hit && surface.material.albedo.a < 0.999 ) {
		const vec4 TRANSPARENCY_COLOR = vec4(1.0 - surface.material.albedo.a);
		
		if ( surface.material.albedo.a < 0.001 ) outFrag = vec4(0);

		RayTracePayload surfacePayload = payload;
		Ray transparency;
		transparency.direction = ray.direction;
		transparency.origin = surface.position.world;
		fogRay = transparency;

		trace( transparency, ubo.settings.rt.alphaTestOffset );
		if ( payload.hit ) {
			populateSurface( payload );
			directLighting();
		} else if (  0 <= ubo.settings.lighting.indexSkybox && ubo.settings.lighting.indexSkybox < CUBEMAPS ) {
			surface.fragment = texture( samplerCubemaps[ubo.settings.lighting.indexSkybox], ray.direction );
			surface.fragment.a = 4096;
			surface.position.eye.z /= 8;
		}
	#if FOG
		fog( transparency, surface.fragment.rgb, surface.fragment.a );
	#endif
		outFrag += TRANSPARENCY_COLOR * surface.fragment;
		eyeDepth = surface.position.eye.z;

		payload = surfacePayload;
		populateSurface( payload );
	}
#if FOG
	{
	//	surface.position.eye.z = eyeDepth;
	//	fog( fogRay, outFrag.rgb, outFrag.a );
	//	fog( ray, surface.fragment.rgb, surface.fragment.a );
	}
#endif

	// reflection
	if ( payload.hit ) {
		const float REFLECTIVITY = 1.0 - surface.material.roughness;
		const vec4 REFLECTED_ALBEDO = surface.material.albedo * REFLECTIVITY;

		if ( REFLECTIVITY > 0.001 ) {
			RayTracePayload surfacePayload = payload;

			Ray reflection;
			reflection.origin = surface.position.world;
			reflection.direction = reflect( ray.direction, surface.normal.world );

			trace( reflection );

			if ( payload.hit ) {
				populateSurface( payload );
				directLighting();
			} else if (  0 <= ubo.settings.lighting.indexSkybox && ubo.settings.lighting.indexSkybox < CUBEMAPS ) {
				surface.fragment = texture( samplerCubemaps[ubo.settings.lighting.indexSkybox], reflection.direction );
				surface.fragment.a = 4096;
			}
		#if FOG
			fog( reflection, surface.fragment.rgb, surface.fragment.a );
		#endif
			outFrag += REFLECTED_ALBEDO * surface.fragment;

			payload = surfacePayload;
			populateSurface( payload );
		}
	}
#endif
	surface = previousSurface;
	return outFrag;
}

float shadowFactorRT( const Light light, float def ) {
	Ray ray;
	ray.origin = surface.position.world;
	ray.direction = light.position - ray.origin;

	float tMin = ubo.settings.rt.defaultRayBounds.x;
	float tMax = length(ray.direction) - ubo.settings.rt.defaultRayBounds.x;
	
	ray.direction = normalize(ray.direction);

	uint rayFlags = gl_RayFlagsOpaqueEXT | gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT;
	uint cullMask = 0xFF;

	rayQueryEXT rayQuery;
	rayQueryInitializeEXT(rayQuery, tlas, rayFlags, cullMask, ray.origin, tMin, ray.direction, tMax);

	while(rayQueryProceedEXT(rayQuery));

	return rayQueryGetIntersectionTypeEXT(rayQuery, true) == gl_RayQueryCommittedIntersectionNoneEXT ? 1.0 : 0.0;
}

void indirectLightingRT() {	
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
	const float DIFFUSE_INDIRECT_FACTOR = 0; // 1.0f / float(CONES_COUNT) * 0.125f;
	
	const float SPECULAR_CONE_APERTURE = clamp(tan(PI * 0.5f * surface.material.roughness), 0.0174533f, PI); // tan( R * PI * 0.5f * 0.1f );
	const float SPECULAR_INDIRECT_FACTOR = (1.0 - surface.material.metallic) * (1.0 - surface.material.roughness); // * 0.25; // 1.0f;
	
	vec4 indirectDiffuse = vec4(0);
	vec4 indirectSpecular = vec4(0);

//	outFragColor.rgb = voxelConeTrace( surface.ray, 0 ).rgb; return;
#if !VXGI
	if ( DIFFUSE_INDIRECT_FACTOR > 0.0f ) {
		float weight = PI * 0.25f;
		for ( uint i = 0; i < CONES_COUNT; ++i ) {
			Ray ray;
			ray.direction = CONES[i].xyz;
			ray.origin = P; // + ray.direction;
			indirectDiffuse += traceStep( ray/*, DIFFUSE_CONE_APERTURE*/ ) * weight;
			weight = PI * 0.15f;
		}
		indirectDiffuse.rgb *= surface.material.albedo.rgb;
		surface.material.occlusion += 1.0 - clamp(indirectDiffuse.a, 0.0, 1.0);
	// 	outFragColor.rgb = indirectDiffuse.rgb; return;
	//	outFragColor.rgb = vec3(surface.material.occlusion); return;
	}
#endif
	if ( SPECULAR_INDIRECT_FACTOR > 0.0f ) {
		const vec3 R = reflect( normalize(P - surface.ray.origin), N );
		Ray ray;
		ray.direction = R;
		ray.origin = P; // + ray.direction;
		indirectSpecular = traceStep( ray/*, SPECULAR_CONE_APERTURE*/ );
		indirectSpecular.rgb *= surface.material.albedo.rgb;
	// 	outFragColor.rgb = indirectSpecular.rgb; return;
	}

	indirectDiffuse *= DIFFUSE_INDIRECT_FACTOR;
	indirectSpecular *= SPECULAR_INDIRECT_FACTOR;


	surface.material.indirect += indirectDiffuse + indirectSpecular;
//	outFragColor.rgb = surface.material.indirect.rgb; return;
	
#if 1 || !VXGI
	// deferred sampling doesn't have a blended albedo buffer
	// in place we'll just cone trace behind the window
	if ( surface.material.albedo.a < 1.0 ) {
		Ray ray;
		ray.direction = surface.ray.direction;
		ray.origin = surface.position.world + ray.direction;
		vec4 radiance = traceStep( ray/*, surface.material.albedo.a * 0.5*/ );
		surface.fragment.rgb += (1.0 - surface.material.albedo.a) * radiance.rgb;
	}
#endif
}