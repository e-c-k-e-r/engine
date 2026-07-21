void pbr() {
#if LIGHTING_IN_WORLD_SPACE
	const vec3 POSITION = surface.position.world;
	const vec3 NORMAL = surface.normal.world;
#else
	const vec3 POSITION = surface.position.eye;
	const vec3 NORMAL = surface.normal.eye;
#endif
	// per-surface, not per-light, compute once

	// Fresnel reflectance for a dieletric 
	const vec3 F0 = mix(vec3(0.04), surface.material.albedo.rgb, surface.material.metallic); 
	// outcoming light from surface to eye
	const vec3 Lo = normalize( -POSITION );
	// angle of outcoming light
	const float cosLo = DOT(NORMAL, Lo);

	const float Rs = 4.0;
	for ( uint i = 0, shadows = 0; i < MAX_LIGHTS; ++i ) {
	#if BAKING
		// skip if surface is a dynamic light, we aren't baking dynamic lights
		if ( lights[i].type < 0 ) continue;
		// shouldn't ever need this, but in the event we hit the end of the buffer and everything after is corrupted
		if ( lights[i].type <= 0 ) break;
	#else
		// skip if surface is already baked, and this isn't a dynamic light
		// to-do: still calculate specular
		if ( surface.material.lightmapped && lights[i].type >= 0 ) continue;
	#endif
		// incoming light to surface (non-const to normalize it later)
	#if LIGHTING_IN_WORLD_SPACE
		vec3 Li = lights[i].position - POSITION;
	#else
		vec3 Li = vec3(VIEW_MATRIX * vec4(lights[i].position, 1)) - POSITION;
	#endif
		// magnitude of incoming light vector (for inverse-square attenuation)
		const float Lmagnitude = dot(Li, Li);
		// distance incoming light travels (reuse from above)
		const float Ldistance = sqrt(Lmagnitude);
		// "free" normalization, since we need to compute the above values anyways
		Li = Li / Ldistance;
		// calculate the smooth windowing
		const float radius2 = lights[i].radius * lights[i].radius;
		float window = 1.0f;
		if ( radius2 > 0.0f ) {
			float distOverRadius = Lmagnitude / radius2;
			window = max(0.0, 1.0 - (distOverRadius * distOverRadius));
			window = window * window; // smoother curve
		}
		// attenuation factor
		const float Lattenuation = window / (1.0 + Lmagnitude);
		// skip if attenuation factor is too low
	//	if ( Lattenuation <= LIGHT_POWER_CUTOFF ) continue;
		// ray cast if our surface is occluded from the light
		const float Lshadow = shadowFactor( lights[i], 0.0 );
		// skip if our shadow factor is too low
//		if ( Lshadow <= LIGHT_POWER_CUTOFF ) continue; // in case of any divergence
		// light radiance
		const vec3 Lr = lights[i].color.rgb * lights[i].power * Lattenuation * Lshadow;
		// skip if our radiance is too low
	//	if ( Lr <= LIGHT_POWER_CUTOFF ) continue;
		// halfway vector
		const vec3 Lh = normalize(Li + Lo);
		// angle of incoming light
		const float cosLi = DOT(NORMAL, Li);
		// angle of halfway light vector
		const float cosLh = DOT(NORMAL, Lh);
	
		// Fresnel term for direct lighting
		const vec3 F = fresnelSchlick(F0, DOT(Lh, Lo));
		// Distribution for specular lighting
		const float D = ndfGGX( cosLh, surface.material.roughness * Rs);
		// Geometric attenuation for specular lighting
		const float G = gaSchlickGGX(cosLi, cosLo, surface.material.roughness * Rs);

		// final lighting
		const vec3 diffuse = mix(vec3(1.0) - F, vec3(0), surface.material.metallic) * surface.material.albedo.rgb;
	#if BAKING
		const vec3 specular = (F * D * G) / max(EPSILON, 4.0 * cosLi * cosLo);
	#else
		const vec3 specular = vec3(0);
	#endif

		surface.light.rgb += (diffuse + specular) * Lr * cosLi;
		surface.light.a += lights[i].power * Lattenuation * Lshadow;
	}
}