float shadowFactor( const Light light, float def );
void lambert() {
	// outcoming light from surface to eye
	const vec3 Lo = normalize( -surface.position.eye );
	// angle of outcoming light
	const float cosLo = max(0.0, dot(surface.normal.eye, Lo));

	for ( uint i = 0, shadows = 0; i < MAX_LIGHTS; ++i ) {
	#if BAKING
		// skip if surface is a dynamic light, we aren't baking dynamic lights
		if ( lights[i].type < 0 ) continue;
	#else
		// skip if surface is already baked, and this isn't a dynamic light
		if ( surface.material.lightmapped && lights[i].type >= 0 ) continue;
	#endif
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
	//	if ( Lattenuation <= LIGHT_POWER_CUTOFF ) continue;
		// ray cast if our surface is occluded from the light
		const float Lshadow = ( shadows++ < MAX_SHADOWS ) ? shadowFactor( lights[i], 0.0 ) : 1;
		// skip if our shadow factor is too low
//		if ( Lshadow <= LIGHT_POWER_CUTOFF ) continue; // in case of any divergence
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
/*	
		const vec3 Liu = vec3(VIEW_MATRIX * vec4(lights[i].position, 1)) - surface.position.eye;
		const vec3 Li = normalize(Liu);
	//	const float Lattenuation = 1.0 / (PI * pow(length(Liu), 2.0));
	//	const float Lattenuation = 1.0 / (1 + (PI * pow(length(Liu), 2.0)));
		const float Lattenuation = 1.0 / (1 + pow(length(Liu), 2.0));
		const float Lshadow = ( shadows++ < MAX_SHADOWS ) ? shadowFactor( lights[i], 0.0 ) : 1;
		if ( lights[i].power * Lattenuation * Lshadow <= LIGHT_POWER_CUTOFF ) continue;

		const float cosLi = max(0.0, dot(surface.normal.eye, Li));
		const vec3 Lr = lights[i].color.rgb * lights[i].power * Lattenuation * Lshadow;
	//	const vec3 Lh = normalize(Li + Lo);
	//	const float cosLh = max(0.0, dot(surface.normal.eye, Lh));	
*/	

		const vec3 diffuse = surface.material.albedo.rgb;
		const vec3 specular = vec3(0);

		surface.light.rgb += (diffuse + specular) * Lr * cosLi;
		surface.light.a += lights[i].power * Lattenuation * Lshadow;
	}
}