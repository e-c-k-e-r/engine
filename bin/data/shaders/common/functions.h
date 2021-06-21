// Helper Functions
float random(vec3 seed, int i){ return fract(sin(dot(vec4(seed,i), vec4(12.9898,78.233,45.164,94.673))) * 43758.5453); }
float rand2(vec2 co){ return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 143758.5453); }
float rand3(vec3 co){ return fract(sin(dot(co.xyz ,vec3(12.9898,78.233, 37.719))) * 143758.5453); }
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
bool validTextureIndex( int textureIndex ) {
	return 0 <= textureIndex && textureIndex < MAX_TEXTURES;
}
bool validCubemapIndex( int textureIndex ) {
	return 0 <= textureIndex && textureIndex < CUBEMAPS;
}
#if DEFERRED || COMPUTE
bool validTextureIndex( uint start, int offset ) {
	return 0 <= offset && start + offset < MAX_TEXTURES;
}
uint textureIndex( uint start, int offset ) {
	return start + offset;
}
vec4 sampleTexture( uint start, uint slot, int offset, int atlas ) {
	Texture a;
	const bool useAtlas = 0 <= atlas && validTextureIndex( start, atlas );
	if ( useAtlas ) a = textures[textureIndex( start, atlas )];
	const Texture t = textures[textureIndex( start, offset )];
	const vec2 uv = ( useAtlas ) ? mix( t.lerp.xy, t.lerp.zw, surface.uv ) : surface.uv;
	const uint i = slot + ( useAtlas ? a.index : t.index );
	return texture( samplerTextures[nonuniformEXT(i)], uv );
}
vec4 sampleTexture( uint start, uint slot, int offset, int atlas, float mip ) {
	Texture a;
	const bool useAtlas = 0 <= atlas && validTextureIndex( start, atlas );
	if ( useAtlas ) a = textures[textureIndex( start, atlas )];
	const Texture t = textures[textureIndex( start, offset )];
	const vec2 uv = ( useAtlas ) ? mix( t.lerp.xy, t.lerp.zw, surface.uv ) : surface.uv;
	const uint i = slot + ( useAtlas ? a.index : t.index );
	return textureLod( samplerTextures[nonuniformEXT(i)], uv, mip );
}
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
	return pow(1 + x, ubo.vxgi.cascadePower);
//	return max( 1, x * ubo.vxgi.cascadePower );
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
float mipLevel( in vec2 uv ) {
	const vec2 dx_vtc = dFdx(uv);
	const vec2 dy_vtc = dFdy(uv);
	return 0.5 * log2(max(dot(dx_vtc, dx_vtc), dot(dy_vtc, dy_vtc)));
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