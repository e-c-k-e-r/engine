// Helper Functions
float random(vec3 seed, int i){ return fract(sin(dot(vec4(seed,i), vec4(12.9898,78.233,45.164,94.673))) * 43758.5453); }
float rand2(vec2 co){ return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 143758.5453); }
float rand3(vec3 co){ return fract(sin(dot(co.xyz ,vec3(12.9898,78.233, 37.719))) * 143758.5453); }
//
uint tea(uint val0, uint val1) {
	uint v0 = val0;
	uint v1 = val1;
	uint s0 = 0;

	#pragma unroll 16
	for(uint n = 0; n < 16; n++) {
		s0 += 0x9e3779b9;
		v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
		v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
	}
	return v0;
}
uint lcg(inout uint prev) {
	uint LCG_A = 1664525u;
	uint LCG_C = 1013904223u;
	prev       = (LCG_A * prev + LCG_C);
	return prev & 0x00FFFFFF;
}
float rnd(inout uint prev) { return (float(lcg(prev)) / float(0x01000000)); }

uint prngSeed;
float rnd() { return rnd(prngSeed); }
//
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
//
void tangentBitangent(in vec3 N, out vec3 Nt, out vec3 Nb) {
	if(abs(N.x) > abs(N.y)) Nt = vec3(N.z, 0, -N.x) / sqrt(N.x * N.x + N.z * N.z);
	else Nt = vec3(0, -N.z, N.y) / sqrt(N.y * N.y + N.z * N.z);
	Nb = cross(N, Nt);
}
vec3 samplingHemisphere(inout uint seed, in vec3 x, in vec3 y, in vec3 z) {
	float r1 = rnd(seed);
	float r2 = rnd(seed);
	float sq = sqrt(1.0 - r2);
	vec3 direction = vec3(cos(2 * PI * r1) * sq, sin(2 * PI * r1) * sq, sqrt(r2));
	direction      = direction.x * x + direction.y * y + direction.z * z;
	return direction;
}
vec3 samplingHemisphere(inout uint seed, in vec3 z) {
	vec3 x;
	vec3 y;
	tangentBitangent( z, x, y );

	float r1 = rnd(seed);
	float r2 = rnd(seed);
	float sq = sqrt(1.0 - r2);
	vec3 direction = vec3(cos(2 * PI * r1) * sq, sin(2 * PI * r1) * sq, sqrt(r2));
	direction      = direction.x * x + direction.y * y + direction.z * z;
	return direction;
}
//
float max3( vec3 v ) { return max(max(v.x, v.y), v.z); }
float min3( vec3 v ) { return min(min(v.x, v.y), v.z); }
uint biasedRound( float x, float bias ) { return uint( ( x < bias ) ? floor(x) : ceil(x)); }
float wrap( float i ) { return fract(i); }
vec2 wrap( vec2 uv ) { return vec2( wrap( uv.x ), wrap( uv.y ) ); }
vec3 orthogonal(vec3 u){
	u = normalize(u);
	const vec3 v = vec3(0.99146, 0.11664, 0.05832); // Pick any normalized vector.
	return abs(dot(u, v)) > 0.99999f ? cross(u, vec3(0, 1, 0)) : cross(u, v);
}
vec4 blend( vec4 source, vec4 dest, float a ) {
	return source * a + dest * (1.0 - a);
}
float gauss( float x, float sigma )  {
	return (1.0 / (2.0 * 3.14157 * sigma) * exp(-(x*x) / (2.0 * sigma)));
}
bool enabled( uint flag, uint bit ) {
	return (flag & (1 << bit)) != 0;
}
vec3 decodeNormals( vec2 enc ) {
	const vec2 ang = enc*2-1;
	const vec2 scth = vec2( sin(ang.x * PI), cos(ang.x * PI) );
	const vec2 scphi = vec2(sqrt(1.0 - ang.y*ang.y), ang.y);
	return normalize( vec3(scth.y*scphi.x, scth.x*scphi.x, scphi.y) );
}
vec2 encodeNormals( vec3 n ) {
//	float p = sqrt(n.z*8+8);
//	return n.xy/p + 0.5;
	return (vec2(atan(n.y,n.x)/PI, n.z)+1.0)*0.5;
}
vec3 encodeSrgb(vec3 rgb) {
	const vec3 a = 12.92 * rgb;
	const vec3 b = 1.055 * pow(rgb, vec3(1.0 / 2.4)) - 0.055;
	const vec3 c = step(vec3(0.0031308), rgb);
	return mix(a, b, c);
}

vec3 decodeSrgb(vec3 rgb) {
	const vec3 a = rgb / 12.92;
	const vec3 b = pow((rgb + 0.055) / 1.055, vec3(2.4));
	const vec3 c = step(vec3(0.04045), rgb);
	return mix(a, b, c);
}
bool validTextureIndex( int textureIndex ) {
	return 0 <= textureIndex && textureIndex < MAX_TEXTURES;
}
#if MAX_CUBEMAPS
bool validCubemapIndex( int textureIndex ) {
	return 0 <= textureIndex && textureIndex < MAX_CUBEMAPS;
}
#endif
#if !BLOOM && (DEFERRED || FRAGMENT || COMPUTE)
bool validTextureIndex( uint id ) {
	return 0 <= id && id < MAX_TEXTURES;
}
bool validTextureIndex( uint start, int offset ) {
	return 0 <= offset && start + offset < MAX_TEXTURES;
}
uint textureIndex( uint start, int offset ) {
	return start + offset;
}
vec4 sampleTexture( uint id, vec2 uv ) {
	const Texture t = textures[id];
	return texture( samplerTextures[nonuniformEXT(t.index)], mix( t.lerp.xy, t.lerp.zw, uv ) );
}
vec4 sampleTexture( uint id, vec2 uv, float mip ) {
#if QUERY_MIPMAP
	return sampleTexture( id, uv );
#else
	const Texture t = textures[id];
	return textureLod( samplerTextures[nonuniformEXT(t.index)], mix( t.lerp.xy, t.lerp.zw, uv ), mip );
#endif
}
vec4 sampleTexture( uint id, vec3 uvm ) { return sampleTexture( id, uvm.xy, uvm.z ); }
vec4 sampleTexture( uint id ) { return sampleTexture( id, surface.uv.xy, surface.uv.z ); }
vec4 sampleTexture( uint id, float mip ) { return sampleTexture( id, surface.uv.xy, mip ); }
#endif
vec2 rayBoxDst( vec3 boundsMin, vec3 boundsMax, in Ray ray ) {
	const vec3 t0 = (boundsMin - ray.origin) / ray.direction;
	const vec3 t1 = (boundsMax - ray.origin) / ray.direction;
	const vec3 tmin = min(t0, t1);
	const vec3 tmax = max(t0, t1);
	const float tStart = max(0, max( max(tmin.x, tmin.y), tmin.z ));
	const float tEnd = max(0, min( tmax.x, min(tmax.y, tmax.z) ) - tStart);
	return vec2(tStart, tEnd);
}
#if VXGI
float cascadePower( uint x ) {
	return pow(1 + x, ubo.settings.vxgi.cascadePower);
//	return max( 1, x * ubo.settings.vxgi.cascadePower );
}
#endif
#if !COMPUTE
void whitenoise(inout vec3 color, const vec4 parameters) {
	const float flicker = parameters.x;
	const float pieces = parameters.y;
	const float blend = parameters.z;
	const float time = parameters.w;
	if ( blend < 0.0001 ) return;
	const float freq = sin(pow(mod(time, flicker) + flicker, 1.9));
	const float whiteNoise = rand2( floor(gl_FragCoord.xy / pieces) + mod(time, freq) );
	color = mix( color, vec3(whiteNoise), blend );
}
float mipLevel( in vec2 dx_vtc, in vec2 dy_vtc ) {
	const float delta_max_sqr = max(dot(dx_vtc, dx_vtc), dot(dy_vtc, dy_vtc));
	return 0.5 * log2(delta_max_sqr);
//	return max(0.0, 0.5 * log2(delta_max_sqr) - 1.0);

//	return 0.5 * log2(max(dot(dx_vtc, dx_vtc), dot(dy_vtc, dy_vtc)));
}
vec4 resolve( subpassInputMS t, const uint samples ) {
	vec4 resolved = vec4(0);
	for ( int i = 0; i < samples; ++i ) resolved += subpassLoad(t, i);
	resolved /= vec4(samples);
	return resolved;
}
uvec4 resolve( usubpassInputMS t, const uint samples ) {
	uvec4 resolved = uvec4(0);
	for ( int i = 0; i < samples; ++i ) resolved += subpassLoad(t, i);
	resolved /= uvec4(samples);
	return resolved;
}
#endif
vec4 resolve( sampler2DMS t, ivec2 uv ) {
	vec4 resolved = vec4(0);
	int samples = textureSamples(t);
	for ( int i = 0; i < samples; ++i ) {
		resolved += texelFetch(t, uv, i);
	}
	resolved /= float(samples);
	return resolved;
}
//

vec2 encodeBarycentrics( vec3 barycentric ) {
	return barycentric.yz;
}
vec3 decodeBarycentrics( vec2 attributes ) {
	return vec3(
		1.0 - attributes.x - attributes.y,
		attributes.x,
		attributes.y
	);
}
#if DEFERRED_SAMPLING
void populateSurfaceMaterial() {
	const Material material = materials[surface.instance.materialID];
	surface.material.albedo = material.colorBase;
	surface.material.metallic = material.factorMetallic;
	surface.material.roughness = material.factorRoughness;
	surface.material.occlusion = material.factorOcclusion;
	surface.light = material.colorEmissive;

	if ( validTextureIndex( material.indexAlbedo ) ) {
		surface.material.albedo *= sampleTexture( material.indexAlbedo );
	}
	// OPAQUE
	if ( material.modeAlpha == 0 ) {
		surface.material.albedo.a = 1;
	// BLEND
	} else if ( material.modeAlpha == 1 ) {

	// MASK
	} else if ( material.modeAlpha == 2 ) {

	}
	// Lightmap
	if ( (surface.subID++ > 0 || bool(ubo.settings.lighting.useLightmaps)) && validTextureIndex( surface.instance.lightmapID ) ) {
		vec4 light = sampleTexture( surface.instance.lightmapID, surface.st );
		surface.material.lightmapped = light.a > 0.001;
		if ( surface.material.lightmapped )	surface.light += surface.material.albedo * light;
	} else {
		surface.material.lightmapped = false;
	}
	// Emissive textures
	if ( validTextureIndex( material.indexEmissive ) ) {
		surface.light += sampleTexture( material.indexEmissive );
	}
	// Occlusion map
	if ( validTextureIndex( material.indexOcclusion ) ) {
	 	surface.material.occlusion = sampleTexture( material.indexOcclusion ).r;
	}
	// Metallic/Roughness map
	if ( validTextureIndex( material.indexMetallicRoughness ) ) {
	 	vec4 samp = sampleTexture( material.indexMetallicRoughness );
	 	surface.material.metallic = samp.r;
		surface.material.roughness = samp.g;
	}
	// Normals
	if ( validTextureIndex( material.indexNormal ) && surface.tangent.world != vec3(0) ) {
		surface.normal.world = surface.tbn * normalize( sampleTexture( material.indexNormal ).xyz * 2.0 - vec3(1.0));
	}
	{
		surface.normal.eye = normalize(vec3( ubo.eyes[surface.pass].view * vec4(surface.normal.world, 0.0) ));
	}
}

bool isValidAddress( uint64_t address ) {
#if UINT64_ENABLED
	return bool(address);
#else
	return bool(address.x) && bool(address.y);
#endif
}

void populateSurface( InstanceAddresses instanceAddresses, uvec3 indices ) {
	Triangle triangle;
	Vertex points[3];

	if ( isValidAddress(instanceAddresses.vertex) ) {
		Vertices vertices = Vertices(nonuniformEXT(instanceAddresses.vertex));

		#pragma unroll 3
		for ( uint _ = 0; _ < 3; ++_ ) /*triangle.*/points[_] = vertices.v[/*triangle.*/indices[_]];
	} else {
		if ( isValidAddress(instanceAddresses.position) ) {
			VPos buf = VPos(nonuniformEXT(instanceAddresses.position));
			#pragma unroll 3
			for ( uint _ = 0; _ < 3; ++_ ) /*triangle.*/points[_].position = buf.v[/*triangle.*/indices[_]];
		}
		if ( isValidAddress(instanceAddresses.uv) ) {
			VUv buf = VUv(nonuniformEXT(instanceAddresses.uv));
			#pragma unroll 3
			for ( uint _ = 0; _ < 3; ++_ ) /*triangle.*/points[_].uv = buf.v[/*triangle.*/indices[_]];
		}
		if ( isValidAddress(instanceAddresses.st) ) {
			VSt buf = VSt(nonuniformEXT(instanceAddresses.st));
			#pragma unroll 3
			for ( uint _ = 0; _ < 3; ++_ ) /*triangle.*/points[_].st = buf.v[/*triangle.*/indices[_]];
		}
		if ( isValidAddress(instanceAddresses.normal) ) {
			VNormal buf = VNormal(nonuniformEXT(instanceAddresses.normal));
			#pragma unroll 3
			for ( uint _ = 0; _ < 3; ++_ ) /*triangle.*/points[_].normal = buf.v[/*triangle.*/indices[_]];
		}
		if ( isValidAddress(instanceAddresses.tangent) ) {
			VTangent buf = VTangent(nonuniformEXT(instanceAddresses.tangent));
			#pragma unroll 3
			for ( uint _ = 0; _ < 3; ++_ ) /*triangle.*/points[_].tangent = buf.v[/*triangle.*/indices[_]];
		}
	}

#if BARYCENTRIC_CALCULATE
	{
		const vec3 p = vec3(inverse( surface.instance.model ) * vec4(surface.position.world, 1));
	
		const vec3 a = points[0].position;
		const vec3 b = points[1].position;
		const vec3 c = points[2].position;

		const vec3 v0 = b - a;
		const vec3 v1 = c - a;
		const vec3 v2 = p - a;
		const float d00 = dot(v0, v0);
		const float d01 = dot(v0, v1);
		const float d11 = dot(v1, v1);
		const float d20 = dot(v2, v0);
		const float d21 = dot(v2, v1);
		const float denom = d00 * d11 - d01 * d01;
		
		const float v = (d11 * d20 - d01 * d21) / denom;
		const float w = (d00 * d21 - d01 * d20) / denom;
		const float u = 1.0f - v - w;

		surface.barycentric = vec3( u, v, w );
	}
#endif
	
	triangle.geomNormal = normalize(cross(points[1].position - points[0].position, points[2].position - points[0].position));
	triangle.point.normal = /*triangle.*/points[0].normal * surface.barycentric[0] + /*triangle.*/points[1].normal * surface.barycentric[1] + /*triangle.*/points[2].normal * surface.barycentric[2];

	triangle.point.position = /*triangle.*/points[0].position * surface.barycentric[0] + /*triangle.*/points[1].position * surface.barycentric[1] + /*triangle.*/points[2].position * surface.barycentric[2];
	triangle.point.uv = /*triangle.*/points[0].uv * surface.barycentric[0] + /*triangle.*/points[1].uv * surface.barycentric[1] + /*triangle.*/points[2].uv * surface.barycentric[2];
	triangle.point.st = /*triangle.*/points[0].st * surface.barycentric[0] + /*triangle.*/points[1].st * surface.barycentric[1] + /*triangle.*/points[2].st * surface.barycentric[2];
	triangle.point.tangent = /*triangle.*/points[0].tangent * surface.barycentric[0] + /*triangle.*/points[1].tangent * surface.barycentric[1] + /*triangle.*/points[2].tangent * surface.barycentric[2];

	
	if ( triangle.point.tangent != vec3(0) ) {
		surface.tangent.world = normalize(vec3( surface.instance.model * vec4(triangle.point.tangent, 0.0) ));
		vec3 bitangent = normalize(vec3( surface.instance.model * vec4(cross( triangle.point.normal, triangle.point.tangent ), 0.0) ));
		surface.tbn = mat3(surface.tangent.world, bitangent, triangle.point.normal);
	}

	// bind position
#if 1 || BARYCENTRIC_CALCULATE
	{
		surface.position.world = vec3( surface.instance.model * vec4(triangle.point.position, 1.0 ) );
		surface.position.eye = vec3( ubo.eyes[surface.pass].view * vec4(surface.position.world, 1.0) );
	}
#endif
	// bind normals
	{
		surface.normal.world = normalize(vec3( surface.instance.model * vec4(triangle.point.normal, 0.0 ) ));
	//	surface.normal.eye = vec3( ubo.eyes[surface.pass].view * vec4(surface.normal.world, 0.0) );
	}
	// bind UVs
	{
		surface.uv.xy = triangle.point.uv;
		surface.uv.z = 0;
		surface.st.xy = triangle.point.st;
		surface.st.z = 0;
	}
	
	populateSurfaceMaterial();
}
void populateSurface( uint instanceID, uint primitiveID ) {
	surface.fragment = vec4(0);
	surface.light = vec4(0);
	surface.instance = instances[instanceID];

	const InstanceAddresses instanceAddresses = instanceAddresses[instanceID];
	if ( !isValidAddress(instanceAddresses.index) ) return;
	const DrawCommand drawCommand = Indirects(nonuniformEXT(instanceAddresses.indirect)).dc[instanceAddresses.drawID];
	const uint triangleID = primitiveID + (drawCommand.indexID / 3);
	uvec3 indices = Indices(nonuniformEXT(instanceAddresses.index)).i[triangleID];
	#pragma unroll 3
	for ( uint _ = 0; _ < 3; ++_ ) /*triangle.*/indices[_] += drawCommand.vertexID;

	populateSurface( instanceAddresses, indices );
}
void populateSurface( RayTracePayload payload ) {
	surface.fragment = vec4(0);
	surface.light = vec4(0);
	surface.instance = instances[payload.instanceID];

	if ( !payload.hit ) return;
	surface.barycentric = decodeBarycentrics(payload.attributes);
	populateSurface( payload.instanceID, payload.primitiveID );
}
#endif