// Helper Functions
float random(vec3 seed, int i){ return fract(sin(dot(vec4(seed,i), vec4(12.9898,78.233,45.164,94.673))) * 43758.5453); }
float rand2(vec2 co){ return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 143758.5453); }
float rand3(vec3 co){ return fract(sin(dot(co.xyz ,vec3(12.9898,78.233, 37.719))) * 143758.5453); }
float interleavedGradientNoise(vec2 co) {
	vec3 seed = vec3(0.06711056, 0.00583715, 52.9829189);
	return fract(seed.z * fract(dot(co, seed.xy)));
}
//
float mipLevel( in vec2 dx_vtc, in vec2 dy_vtc ) {
	const float delta_max_sqr = max(dot(dx_vtc, dx_vtc), dot(dy_vtc, dy_vtc));
	return 0.5 * log2(delta_max_sqr);
//	return max(0.0, 0.5 * log2(delta_max_sqr) - 1.0);

//	return 0.5 * log2(max(dot(dx_vtc, dx_vtc), dot(dy_vtc, dy_vtc)));
}
float mipLevels( vec2 size ) {
	return floor(log2(max(size.x, size.y)));
}
float mipLevels( ivec2 size ) {
	return floor(log2(max(size.x, size.y)));
}
//
vec4 rgbaUVec4toVec4( uvec4 rgba ) {
/*
	return vec4(float((val & 0x000000FF)), 
	float((val & 0x0000FF00) >> 8U), 
	float((val & 0x00FF0000) >> 16U), 
	float((val & 0xFF000000) >> 24U));
*/
	return rgba / vec4(256.0);
}
uvec4 rgbaVec4toUvec4( vec4 rgba ) {
/*
	 return (uint(val.w) & 0x000000FF) << 24U | 
	(uint(val.z) & 0x000000FF) << 16U | 
	(uint(val.y) & 0x000000FF) << 8U | 
	(uint(val.x) & 0x000000FF);
*/
	return uvec4(rgba * uvec4(256));
}
//
void toneMap( inout vec3 color, float exposure ) {
	color.rgb = vec3(1.0) - exp(-color.rgb * exposure);
}
void gammaCorrect( inout vec3 color, float gamma ) {
	color.rgb = pow(color.rgb, vec3(1.0 / gamma));
}
void toneMap( inout vec4 color, float exposure ) { toneMap(color.rgb, exposure); }
void gammaCorrect( inout vec4 color, float gamma ) { gammaCorrect(color.rgb, gamma); }
float luma( vec3 color ) { return dot(color, vec3(0.2126, 0.7152, 0.0722)); } // REF 709
float luminance( vec3 color ) { return dot(color, vec3(0.299, 0.587, 0.114)); } // REF 601

vec4 encodeRGBE( vec3 color ) {
	float maxColor = max(color.r, max(color.g, color.b));
	if ( maxColor < 1e-6 ) return vec4(0.0); // bail early

	int exponent;
	float mantissa = frexp( maxColor, exponent );
	vec3 rgb = color * exp2(-float(exponent));
	float alpha = (float(exponent) + 128.0) / 255.0;
	return vec4(rgb, alpha);
}
vec4 decodeRGBE(vec4 rgbe) {
	if ( rgbe.a == 0.0 ) return vec4(0);
	float exponent = rgbe.a * 255.0 - 128.0;
	return vec4(rgbe.rgb * exp2(exponent), 1.0);
}
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
	prev	   = (LCG_A * prev + LCG_C);
	return prev & 0x00FFFFFF;
}
float rnd(inout uint prev) { return (float(lcg(prev)) / float(0x01000000)); }

uint prngSeed;
float rnd() { return rnd(prngSeed); }
//
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
	direction	  = direction.x * x + direction.y * y + direction.z * z;
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
	direction	  = direction.x * x + direction.y * y + direction.z * z;
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
#if !SPD && (DEFERRED || FRAGMENT || COMPUTE || RT || FORWARD)
bool validTextureIndex( int textureIndex ) {
	return 0 <= textureIndex && textureIndex < MAX_TEXTURES;
}
#if MAX_CUBEMAPS
bool validCubemapIndex( int textureIndex ) {
	return 0 <= textureIndex && textureIndex < MAX_CUBEMAPS;
}
#endif
bool validTextureIndex( uint id ) {
	return 0 <= id && id < MAX_TEXTURES;
}
bool validTextureIndex( uint start, int offset ) {
	return 0 <= offset && start + offset < MAX_TEXTURES;
}
uint textureIndex( uint start, int offset ) {
	return start + offset;
}
vec4 sampleTexture( uint id, vec2 uv, vec2 ddx, vec2 ddy ) {
	const Texture t = textures[id];
	vec2 scale = t.lerp.zw - t.lerp.xy;
	vec2 final_uv = mix( t.lerp.xy, t.lerp.zw, uv );

	return textureGrad( samplerTextures[nonuniformEXT(t.index)], final_uv, ddx * scale, ddy * scale );
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
vec4 sampleTexture( uint id ) {
#if QUERY_MIPMAP
	return sampleTexture( id, uv );
#else
	return sampleTexture( id, surface.uv.xy, surface.dUvDx, surface.dUvDy );
#endif
}
// vec4 sampleTexture( uint id ) { return sampleTexture( id, surface.uv.xy, surface.uv.z ); }
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
#if FRAGMENT
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
	const Material material = materials[surface.instance.materialID >= materials.length() ? 0 : surface.instance.materialID];
	surface.material.albedo = material.colorBase;
	surface.material.metallic = material.factorMetallic;
	surface.material.roughness = material.factorRoughness;
	surface.material.occlusion = material.factorOcclusion;
	surface.material.lightmapped = false;
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

	// EMISSIVE
	} else if ( material.modeAlpha == 3 ) {
		surface.light.rgb *= surface.material.albedo.rgb * surface.material.albedo.a;
		surface.material.albedo.a = 1.0;
	}

	// Emissive mapping
	if ( validTextureIndex( material.indexEmissive ) ) {
		surface.light *= sampleTexture( material.indexEmissive );
	}
	// Cubemap
	int cubemapIndex = -1;
	if ( 0 <= surface.instance.cubemapID ) cubemapIndex = surface.instance.cubemapID; // instance takes priority over material
	else if ( 0 <= material.indexCubemap ) cubemapIndex = material.indexCubemap;

	if ( 0 <= cubemapIndex && surface.material.roughness < 1.0 ) {
		const Texture texture = textures[cubemapIndex];

		vec3 V = normalize(surface.position.eye);
		vec3 N = surface.normal.eye;
		vec3 R = reflect(V, N);

		mat3 invView = mat3(ubo.eyes[surface.pass].iView);
		vec3 worldR = invView * R;

		float mipLevel = surface.material.roughness * mipLevels(textureSize(samplerCubemaps[nonuniformEXT(texture.index)], 0).xy);
		vec3 reflection = textureLod(samplerCubemaps[nonuniformEXT(texture.index)], worldR, mipLevel).rgb;

		vec3 F0 = mix(vec3(0.04), surface.material.albedo.rgb, surface.material.metallic);
		float cosTheta = max(dot(N, -V), 0.0);
		vec3 F = F0 + (max(vec3(1.0 - surface.material.roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);

		surface.light.rgb += reflection * F * surface.material.occlusion;
	}
	// (Occlusion/)Metallic/Roughness map
	if ( validTextureIndex( material.indexMetallicRoughness ) ) {
		vec4 samp = sampleTexture( material.indexMetallicRoughness );

		surface.material.metallic *= samp.b;
		surface.material.roughness *= samp.g;

		if ( material.indexOcclusion == material.indexMetallicRoughness ) {
			surface.material.occlusion = mix(1.0, samp.r, material.factorOcclusion);
		}
	}
	// Occlusion mapping
	if ( validTextureIndex( material.indexOcclusion ) && material.indexOcclusion != material.indexMetallicRoughness ) {
		float occ = sampleTexture( material.indexOcclusion ).r;
		surface.material.occlusion = mix(1.0, occ, material.factorOcclusion);
	}
	// Normal mapping
	if ( validTextureIndex( material.indexNormal ) && surface.tangent.world != vec3(0) ) {
		surface.normal.world = surface.tbn * normalize( sampleTexture( material.indexNormal ).xyz * 2.0 - vec3(1.0));
	}
	{
		surface.normal.eye = normalize(vec3( VIEW_MATRIX * vec4(surface.normal.world, 0.0) ));
	}

	// Light mapping
	if ( ( bool(ubo.settings.lighting.useLightmaps)) && validTextureIndex( surface.instance.lightmapID ) ) {
		surface.material.lightmapped = true; // light.a > 0.001;
		vec4 light = decodeRGBE( sampleTexture( surface.instance.lightmapID, surface.st.xy, 0.0 ) );

		const vec3 F0 = mix(vec3(0.04), surface.material.albedo.rgb, surface.material.metallic);
		const vec3 Lo = normalize(-surface.position.eye);
		const float cosLo = max(0.0, dot(surface.normal.eye, Lo));
		const vec3 F = F0 + (max(vec3(1.0 - surface.material.roughness), F0) - F0) * pow(clamp(1.0 - cosLo, 0.0, 1.0), 5.0);
		const vec3 kD = (vec3(1.0) - F) * (1.0 - surface.material.metallic);

		surface.light.rgb += (kD * surface.material.albedo.rgb * light.rgb) * surface.material.occlusion;
	}
}
#endif
#if DEFERRED_SAMPLING
bool isValidAddress( uint64_t address ) {
#if UINT64_ENABLED
	return bool(address);
#else
	return bool(address.x) && bool(address.y);
#endif
}

uvec4 uvec2_16x4( uvec2 i ) {
	uvec4 converted;

	converted.x = (i.x >> 16) & 0xFFFF;
	converted.y = (i.x >>  0) & 0xFFFF;
	converted.z = (i.y >> 16) & 0xFFFF;
	converted.w = (i.y >>  0) & 0xFFFF;

	return converted;
}

#if BUFFER_REFERENCE

void populateSurface( InstanceAddresses addresses, uvec3 indices ) {
	Triangle triangle;
	Vertex points[3];
	if ( isValidAddress(addresses.position) ) {
		VPos buf = VPos(nonuniformEXT(addresses.position));
		#pragma unroll 3
		for ( uint _ = 0; _ < 3; ++_ ) points[_].position = vec3( buf.v[indices[_]*3+0], buf.v[indices[_]*3+1], buf.v[indices[_]*3+2] );
	}
	if ( isValidAddress(addresses.uv) ) {
		VUv buf = VUv(nonuniformEXT(addresses.uv));
		#pragma unroll 3
		for ( uint _ = 0; _ < 3; ++_ ) points[_].uv = buf.v[indices[_]];
	}
	if ( isValidAddress(addresses.st) ) {
		VSt buf = VSt(nonuniformEXT(addresses.st));
		#pragma unroll 3
		for ( uint _ = 0; _ < 3; ++_ ) points[_].st = buf.v[indices[_]];
	}
	
	vec3 e0 = points[1].position - points[0].position;
	vec3 e1 = points[2].position - points[0].position;
	vec2 dUv1 = points[1].uv - points[0].uv;
	vec2 dUv2 = points[2].uv - points[0].uv;
	float det = (dUv1.x * dUv2.y - dUv1.y * dUv2.x);
	float handedness = (det < 0.0) ? -1.0 : 1.0;

	if ( isValidAddress(addresses.normal) ) {
		VNormal buf = VNormal(nonuniformEXT(addresses.normal));
		#pragma unroll 3
		for ( uint _ = 0; _ < 3; ++_ ) points[_].normal = vec3( buf.v[indices[_]*3+0], buf.v[indices[_]*3+1], buf.v[indices[_]*3+2] );
	} else {
		vec3 normal = normalize(cross(e0, e1));
		#pragma unroll 3
		for ( uint _ = 0; _ < 3; ++_ ) points[_].normal = normal;
	}
	if ( isValidAddress(addresses.tangent) ) {
		VTangent buf = VTangent(nonuniformEXT(addresses.tangent));
		#pragma unroll 3
		for ( uint _ = 0; _ < 3; ++_ ) points[_].tangent = vec4( buf.v[indices[_]*4+0], buf.v[indices[_]*4+1], buf.v[indices[_]*4+2], buf.v[indices[_]*4+3] );
	} else {
		vec3 tangent = (e0 * dUv2.y - e1 * dUv1.y);
		if ( abs(det) < 0.000001 ) {
			tangent = abs(points[0].normal.y) < 0.999 ? vec3(0, 1, 0) : vec3(1, 0, 0);
		} else {
			tangent /= det;
		}

		#pragma unroll 3
		for ( uint _ = 0; _ < 3; ++_ ) points[_].tangent = vec4(tangent, handedness);
	}
#if BARYCENTRIC_CALCULATE
	{
		const vec3 p = vec3(inverse( surface.object.model ) * vec4(surface.position.world, 1));
	
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
	
	triangle.point.position = points[0].position * surface.barycentric[0] + points[1].position * surface.barycentric[1] + points[2].position * surface.barycentric[2];
	triangle.point.normal = points[0].normal * surface.barycentric[0] + points[1].normal * surface.barycentric[1] + points[2].normal * surface.barycentric[2];
	triangle.point.uv = points[0].uv * surface.barycentric[0] + points[1].uv * surface.barycentric[1] + points[2].uv * surface.barycentric[2];
	triangle.point.st = points[0].st * surface.barycentric[0] + points[1].st * surface.barycentric[1] + points[2].st * surface.barycentric[2];
	triangle.point.tangent = points[0].tangent * surface.barycentric[0] + points[1].tangent * surface.barycentric[1] + points[2].tangent * surface.barycentric[2];


	// bind position (seems to muck with the skybox + fog)
#if 0 && BARYCENTRIC_CALCULATE
	{
		surface.position.world = vec3( surface.object.model * vec4(triangle.point.position, 1.0 ) );
		surface.position.eye = vec3( VIEW_MATRIX * vec4(surface.position.world, 1.0) );
	}
#endif
	// bind normals
	{
		surface.normal.world = normalize(vec3( surface.object.model * vec4(triangle.point.normal, 0.0 ) ));
	//	surface.normal.eye = vec3( VIEW_MATRIX * vec4(surface.normal.world, 0.0) );
	}
	// bind tangent
	{
		surface.tangent.world = vec3( surface.object.model * vec4(triangle.point.tangent.xyz, 0.0) );
		surface.tangent.world = normalize(surface.tangent.world - dot(surface.tangent.world, surface.normal.world) * surface.normal.world);
		vec3 bitangent = normalize(cross(surface.normal.world, surface.tangent.world)) * sign(triangle.point.tangent.w);

		surface.tbn = mat3(surface.tangent.world, bitangent, surface.normal.world);
	}
	// bind UVs
	{
		surface.uv.xy = triangle.point.uv;
		surface.uv.z = 0;
		surface.st.xy = triangle.point.st;
		surface.st.z = 0;
	}

	// bind UV derivatives
	{
		surface.dUvDx = vec2(0.0);
		surface.dUvDy = vec2(0.0);

	#if BARYCENTRIC && MULTISAMPLING
		ivec2 size = textureSize(samplerId).xy;
		int sampleIdx = msaa.currentID;
	#elif BARYCENTRIC
		ivec2 size = textureSize(samplerId, 0).xy;
		int sampleIdx = 0;
	#elif DEFERRED && MULTISAMPLING
		ivec2 size = textureSize(samplerUv).xy;
		int sampleIdx = msaa.currentID;
	#elif DEFERRED
		ivec2 size = textureSize(samplerUv, 0).xy;
		int sampleIdx = 0;
	#else
		ivec2 size = imageSize(outImage).xy;
		int sampleIdx = 0;
	#endif

#if BARYCENTRIC
		ivec2 coord = ivec2(gl_GlobalInvocationID.xy);
		int layer = int(gl_GlobalInvocationID.z);

		uvec2 centerID = uvec2(texelFetch(samplerId, ivec3(coord, layer), sampleIdx).xy);

	#if BARYCENTRIC_CALCULATE
		#if USE_CAMERA_VIEWPORT
			mat4 iProj = inverse( camera.viewport[surface.pass].projection );
			mat4 iView = inverse( camera.viewport[surface.pass].view );
		#else
			mat4 iProj = ubo.eyes[surface.pass].iProjection;
			mat4 iView = ubo.eyes[surface.pass].iView;
		#endif
		mat4 invModel = inverse(surface.object.model);

		vec3 pA = points[0].position;
		vec3 v0 = points[1].position - pA;
		vec3 v1 = points[2].position - pA;
		float d00 = dot(v0, v0);
		float d01 = dot(v0, v1);
		float d11 = dot(v1, v1);
		float denom = d00 * d11 - d01 * d01;
	#endif

		ivec2 offsetX[2] = ivec2[]( ivec2(1, 0), ivec2(-1, 0) );
		ivec2 offsetY[2] = ivec2[]( ivec2(0, 1), ivec2(0, -1) );

	#if !BARYCENTRIC_CALCULATE
	#define FETCH_NEIGHBOR_UV(OFFSETS, OUT_GRAD) \
		for (int i = 0; i < 2; ++i) { \
			ivec2 off = OFFSETS[i]; \
			ivec2 nCoord = coord + off; \
			if ( nCoord.x >= 0 && nCoord.y >= 0 && nCoord.x < size.x && nCoord.y < size.y ) { \
				if ( uvec2(texelFetch(samplerId, ivec3(nCoord, layer), sampleIdx).xy) == centerID ) { \
					vec3 bN = decodeBarycentrics(texelFetch(samplerBary, ivec3(nCoord, layer), sampleIdx).xy); \
					vec2 uvN = points[0].uv * bN.x + points[1].uv * bN.y + points[2].uv * bN.z; \
					OUT_GRAD = (uvN - surface.uv.xy) * float(off.x + off.y); \
					break; \
				} \
			} \
		}
	#else
	#define FETCH_NEIGHBOR_UV( OFFSETS, OUT_GRAD ) \
		for (int i = 0; i < 2; ++i) { \
			ivec2 off = OFFSETS[i]; \
			ivec2 nCoord = coord + off; \
			if ( nCoord.x >= 0 && nCoord.y >= 0 && nCoord.x < size.x && nCoord.y < size.y ) { \
				if ( uvec2(texelFetch(samplerId, ivec3(nCoord, layer), sampleIdx).xy) == centerID ) { \
					float dN = texelFetch(samplerDepth, ivec3(nCoord, layer), sampleIdx).r; \
					vec2 inUvN = (vec2(nCoord) / vec2(size)) * 2.0f - 1.0f; \
					vec4 eyeN = iProj * vec4(inUvN, dN, 1.0); \
					vec3 pN = vec3(invModel * vec4(vec3(iView * (eyeN / eyeN.w)), 1.0)); \
					vec3 v2N = pN - pA; \
					float vN = (d11 * dot(v2N, v0) - d01 * dot(v2N, v1)) / denom; \
					float wN = (d00 * dot(v2N, v1) - d01 * dot(v2N, v0)) / denom; \
					vec2 uvN = points[0].uv * (1.0f - vN - wN) + points[1].uv * vN + points[2].uv * wN; \
					OUT_GRAD = (uvN - surface.uv.xy) * float(off.x + off.y); \
					break; \
				} \
			} \
		}
	#endif

		FETCH_NEIGHBOR_UV( offsetX, surface.dUvDx );
		FETCH_NEIGHBOR_UV( offsetY, surface.dUvDy );

		#undef FETCH_NEIGHBOR_UV
#endif
		if ( surface.dUvDx == vec2(0.0) && surface.dUvDy == vec2(0.0) ) {
		#if USE_CAMERA_VIEWPORT
			mat4 proj = camera.viewport[surface.pass].projection;
		#else
			mat4 proj = ubo.eyes[surface.pass].projection;
		#endif

			float pixelSize = abs(surface.position.eye.z) * 2.0 / (proj[1][1] * float(size.y));
			float geomArea = length(cross(e0, e1));
			float uvArea = abs(dUv1.x * dUv2.y - dUv2.x * dUv1.y);
			float uvPerMeter = sqrt(uvArea / max(geomArea, 0.00001));
			float fallback = pixelSize * uvPerMeter;

			surface.dUvDx = vec2(fallback, 0.0);
			surface.dUvDy = vec2(0.0, fallback);
		}
	}
	
	populateSurfaceMaterial();
}
void populateSurface( uint instanceID, uint primitiveID ) {
	surface.fragment = vec4(0);
	surface.light = vec4(0);
	surface.instance = instances[instanceID];
	surface.object = objects[surface.instance.objectID];

	const InstanceAddresses addresses = addresses[instanceID];
	if ( !isValidAddress(addresses.index) ) return;
	const DrawCommand drawCommand = Indirects(nonuniformEXT(addresses.indirect)).dc[addresses.drawID];
	const uint triangleID = primitiveID + (drawCommand.indexID / 3);
	//uvec3 indices = Indices(nonuniformEXT(addresses.index)).i[triangleID];
	
	uvec3 indices = uvec3(
		Indices(nonuniformEXT(addresses.index)).i[triangleID*3+0],
		Indices(nonuniformEXT(addresses.index)).i[triangleID*3+1],
		Indices(nonuniformEXT(addresses.index)).i[triangleID*3+2]
	);
	

	#pragma unroll 3
	for ( uint _ = 0; _ < 3; ++_ ) indices[_] += drawCommand.vertexID;

	populateSurface( addresses, indices );
}
void populateSurface( RayTracePayload payload ) {
	surface.fragment = vec4(0);
	surface.light = vec4(0);
	surface.instance = instances[payload.instanceID];
	surface.object = objects[surface.instance.objectID];

	if ( !payload.hit ) return;
	surface.barycentric = decodeBarycentrics(payload.attributes);
	populateSurface( payload.instanceID, payload.primitiveID );
}
#endif
#endif