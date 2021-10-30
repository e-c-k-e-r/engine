// PBR
float shadowFactor( const Light light, float def );
float ndfGGX(float cosLh, float roughness) {
	const float alpha   = roughness * roughness;
	const float alphaSq = alpha * alpha;
	const float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
}
float gaSchlickG1(float cosTheta, float k) { return cosTheta / (cosTheta * (1.0 - k) + k); }
float gaSchlickGGX(float cosLi, float cosLo, float roughness) {
	const float r = roughness + 1.0;
	const float k = (r * r) / 8.0;
	return gaSchlickG1(cosLi, k) * gaSchlickG1(cosLo, k);
}
vec3 fresnelSchlick(vec3 F0, float cosTheta) { return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0); }
#if !BAKING
void pbr() {
	if ( validTextureIndex( surface.instance.lightmapID ) ) return;

	const float Rs = 4.0; // specular lighting looks gross without this
	const vec3 F0 = mix(vec3(0.04), surface.material.albedo.rgb, surface.material.metallic); 
	const vec3 Lo = normalize( -surface.position.eye );
	const float cosLo = max(0.0, dot(surface.normal.eye, Lo));
	
	for ( uint i = 0; i < ubo.lights; ++i ) {
		const Light light = lights[i];
		if ( light.power <= LIGHT_POWER_CUTOFF ) continue;
		const vec3 Liu = vec3(ubo.eyes[surface.pass].view * vec4(light.position, 1)) - surface.position.eye;
		const vec3 Li = normalize(Liu);
		const bool reverseZ = light.projection[2][2] < 0.00001;
		const float Ls = shadowFactor( light, 0.0 );
		const float La = 1.0 / (PI * pow(length(Liu), 2.0));
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
		// lightmapped, compute only specular
		if ( light.type >= 0 && validTextureIndex( surface.instance.lightmapID ) ) surface.fragment.rgb += (specular) * Lr * cosLi;
		// point light, compute only diffuse
		// else if ( abs(light.type) == 1 ) surface.fragment.rgb += (diffuse) * Lr * cosLi;
		else surface.fragment.rgb += (diffuse + specular) * Lr * cosLi;
		surface.fragment.rgb += (diffuse + specular) * Lr * cosLi;
		surface.fragment.a += light.power * La * Ls;
	}
}
#endif