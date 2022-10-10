// PBR
void pbr() {
	// per-surface, not per-light, compute once

	// Freslen reflectance for a dieletric 
	const vec3 F0 = mix(vec3(0.04), surface.material.albedo.rgb, surface.material.metallic); 
	// outcoming light from surface to eye
	const vec3 Lo = normalize( -surface.position.eye );
	// angle of outcoming light
	const float cosLo = max(0.0, dot(surface.normal.eye, Lo));

	const float Rs = 4.0;
	
	for ( uint i = 0, shadows = 0; i < MAX_LIGHTS; ++i ) {
	#if BAKING
		// skip if surface is a dynamic light, we aren't baking dynamic lights
		if ( lights[i].type < 0 ) continue;
	#else
		// skip if surface is already baked, and this isn't a dynamic light
		if ( surface.material.lightmapped && lights[i].type >= 0 ) continue;
	#endif
		// skip if light power is too low
		if ( lights[i].power <= LIGHT_POWER_CUTOFF ) continue;
		// incoming light to surface (non-const to normalize it later)
	//	vec3 Li = lights[i].position - surface.position.world;
		vec3 Li = vec3(VIEW_MATRIX * vec4(lights[i].position, 1)) - surface.position.eye;
		// magnitude of incoming light vector (for inverse-square attenuation)
		const float Lmagnitude = dot(Li, Li);
		// distance incoming light travels (reuse from above)
		const float Ldistance = sqrt(Lmagnitude);
		// "free" normalization, since we need to compute the above values anyways
		Li = Li / Ldistance;
		// attenuation factor
	//	const float Lattenuation = 1.0 / (1 + (PI * Lmagnitude));
		const float Lattenuation = 1.0 / (1 + Lmagnitude);
		// skip if attenuation factor is too low
		if ( Lattenuation <= LIGHT_POWER_CUTOFF ) continue;
		// ray cast if our surface is occluded from the light
		const float Lshadow = ( shadows++ < MAX_SHADOWS ) ? shadowFactor( lights[i], 0.0 ) : 1;
		// skip if our shadow factor is too low
		if ( Lshadow <= LIGHT_POWER_CUTOFF ) continue;
		// light radiance
		const vec3 Lr = lights[i].color.rgb * lights[i].power * Lattenuation * Lshadow;
		// skip if our radiance is too low
	//	if ( Lr <= LIGHT_POWER_CUTOFF ) continue;
		// halfway vector
		const vec3 Lh = normalize(Li + Lo);
		// angle of incoming light
		const float cosLi = max(0.0, dot(surface.normal.eye, Li));
		// angle of halfway light vector
		const float cosLh = max(0.0, dot(surface.normal.eye, Lh));
		// Fresnel term for direct lighting
		const vec3 F = fresnelSchlick(F0, max(0.0, dot(Lh, Lo)));
		// Distribution for specular lighting
		const float D = ndfGGX( cosLh, surface.material.roughness * Rs);
		// Geometric attenuation for specular lighting
		const float G = gaSchlickGGX(cosLi, cosLo, surface.material.roughness * Rs);

		// final lighting
		const vec3 diffuse = mix(vec3(1.0) - F, vec3(0), surface.material.metallic) * surface.material.albedo.rgb;
		const vec3 specular = ( shadows < MAX_SHADOWS ) ? ((F * D * G) / max(EPSILON, 4.0 * cosLi * cosLo)) : vec3(0);

		surface.light.rgb += (diffuse + specular) * Lr * cosLi;
		surface.light.a += lights[i].power * Lattenuation * Lshadow;
	}
#if 0
	const float Rs = 4.0; // specular lighting looks gross without this
	uint shadows = 0;	
	for ( uint i = 0; i < ubo.settings.lengths.lights; ++i ) {
		const Light light = lights[i];
		if ( light.power <= LIGHT_POWER_CUTOFF ) continue;
		if ( surface.material.lightmapped && light.type >= 0 ) continue;

		const vec3 Liu = vec3(ubo.eyes[surface.pass].view * vec4(light.position, 1)) - surface.position.eye;
		const float Ld = length(Liu);
		const float La = 1.0 / (1 + (PI * pow(Ld, 2.0)));
		if ( La <= LIGHT_POWER_CUTOFF ) continue;

		const vec3 Li = normalize(Liu);
		const float Ls = ( shadows++ < ubo.settings.lighting.maxShadows ) ? shadowFactor( light, 0.0 ) : 1;
		if ( light.power * La * Ls <= LIGHT_POWER_CUTOFF ) continue;

		const float cosLi = max(0.0, dot(surface.normal.eye, Li));
		const vec3 Lr = light.color.rgb * light.power * La * Ls;
		const vec3 Lh = normalize(Li + Lo);
		const float cosLh = max(0.0, dot(surface.normal.eye, Lh));
		
		const vec3 F = fresnelSchlick( F0, max( 0.0, dot(Lh, Lo) ) );
		const float D = ndfGGX( cosLh, surface.material.roughness * Rs );
		const float G = gaSchlickGGX(cosLi, cosLo, surface.material.roughness);
		const vec3 diffuse = mix( vec3(1.0) - F, vec3(0.0), surface.material.metallic ) * surface.material.albedo.rgb;
		const vec3 specular = (F * D * G) / max(EPSILON, 4.0 * cosLi * cosLo);
	/*
		// lightmapped, compute only specular
		if ( light.type >= 0 && validTextureIndex( surface.instance.lightmapID ) ) surface.light.rgb += (specular) * Lr * cosLi;
		// point light, compute only diffuse
		// else if ( abs(light.type) == 1 ) surface.light.rgb += (diffuse) * Lr * cosLi;
		else surface.light.rgb += (diffuse + specular) * Lr * cosLi;
	*/
		surface.light.rgb += (diffuse + specular) * Lr * cosLi;
		surface.light.a += light.power * La * Ls;
	}
#endif
}