float shadowFactor( const Light light, float def );
void lambert() {
	const vec3 F0 = mix(vec3(0.04), surface.material.albedo.rgb, surface.material.metallic); 
	const vec3 Lo = normalize( -surface.position.world );
	const float cosLo = max(0.0, dot(surface.normal.world, Lo));
	for ( uint i = 0; i < lights.length(); ++i ) {
		const Light light = lights[i];
		if ( light.power <= LIGHT_POWER_CUTOFF ) continue;
		const vec3 Lp = light.position;
	//	const vec3 Liu = light.position - surface.position.world;
		const vec3 Liu = vec3(ubo.eyes[surface.pass].view * vec4(light.position, 1)) - surface.position.eye;
		const float La = 1.0 / (PI * pow(length(Liu), 2.0));
		const float Ls = shadowFactor( light, 0.0 );
		if ( light.power * La * Ls <= LIGHT_POWER_CUTOFF ) continue;

		const vec3 Li = normalize(Liu);
		const vec3 Lr = light.color.rgb * light.power * La * Ls;
	//	const float cosLi = abs(dot(surface.normal.world, Li));
		const float cosLi = max(0.0, dot(surface.normal.eye, Li));

		const vec3 diffuse = surface.material.albedo.rgb;
		const vec3 specular = vec3(0);

		surface.fragment.rgb += (diffuse + specular) * Lr * cosLi;
		surface.fragment.a += light.power * La * Ls;
	}
}