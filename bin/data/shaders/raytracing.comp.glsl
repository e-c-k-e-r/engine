#version 450

layout (local_size_x = 32, local_size_y = 32) in;

layout (constant_id = 0) const uint LIGHTS = 16;
layout (constant_id = 1) const uint EYES = 2;
layout (binding = 0, rgba8) uniform writeonly image2D resultImage[EYES];

#define EPSILON 0.0001
#define MAXLEN 1000.0

layout( push_constant ) uniform PushBlock {
	uint marchingSteps;
	uint rayBounces;
	float shadowFactor;
	float reflectionStrength;
	float reflectionFalloff;
} PushConstant;

struct Ray {
	vec3 origin;
	vec3 direction;
};
struct Result {
	vec3 color;
	float t;
	int id;
};
struct Light {
	vec3 position;
	float radius;
	vec3 color;
	float power;
	vec2 type;
	vec2 padding;
	mat4 view;
	mat4 projection;
};

struct State {
	vec3 viewPosition;
	Ray ray;
	Result result;
} state;

struct Fog {
	vec2 range;
	vec2 padding;
	vec4 color;
};

layout (binding = 1) uniform UBO {
	mat4 matrices[2];
	vec4 ambient;
	Fog fog;
	Light lights[LIGHTS];
} ubo;

struct Shape {
	vec4 values;
	vec4 albedoSpecular;
	int type;
};
layout (std140, binding = 2) buffer Shapes {
	Shape shapes[];
};

void reflectRay(inout vec3 rayD, in vec3 normal) {
	rayD = rayD + 2.0 * -dot(normal, rayD) * normal;
}
// Lighting =========================================================
float lightDiffuse(vec3 normal, vec3 lightDir) {
	return clamp(dot(normal, lightDir), 0.1, 1.0);
}
float lightSpecular(vec3 normal, vec3 lightDir, float specularFactor) {
	vec3 viewVec = normalize(state.viewPosition);
	vec3 halfVec = normalize(lightDir + viewVec);
	return pow(clamp(dot(normal, halfVec), 0.0, 1.0), specularFactor);
}

// Sphere ===========================================================
float sphereIntersect(in vec3 rayO, in vec3 rayD, in Shape shape ) {
	vec3 oc = rayO - shape.values.xyz;
	float b = 2.0 * dot(oc, rayD);
	float c = dot(oc, oc) - shape.values.w * shape.values.w;
	float h = b*b - 4.0*c;
	if (h < 0.0) return -1.0;
	float t = (-b - sqrt(h)) / 2.0;
	return t;
}
vec3 sphereNormal(in vec3 position, in Shape shape) {
	return (position - shape.values.xyz) / shape.values.w;
}
float sphereSDF( vec3 position, in Shape shape ) {
	return length(position - shape.values.xyz) - shape.values.w;
}
// Plane ===========================================================
float planeIntersect(in vec3 rayO, in vec3 rayD, in Shape shape) {
	float d = dot(rayD, shape.values.xyz);
	if (d == 0.0) return 0.0;
	float t = -(shape.values.w + dot(rayO, shape.values.xyz)) / d;
	return t < 0.0 ? 0.0 : t;
}
vec3 planeNormal(in vec3 position, in Shape shape) {
	return shape.values.xyz;
}
float planeSDF( vec3 position, in Shape shape ) {
	return dot( position, shape.values.xyz ) + shape.values.w;
}
// Generic =========================================================
float shapeIntersect(in vec3 rayO, in vec3 rayD, in Shape shape) {
	if ( shape.type == 1 ) return sphereIntersect( rayO, rayD, shape );
	if ( shape.type == 2 ) return planeIntersect( rayO, rayD, shape );
	return 0.0;
}
vec3 shapeNormal( vec3 position, in Shape shape ) {
	if ( shape.type == 1 ) return sphereNormal(position, shape);
	if ( shape.type == 2 ) return planeNormal(position, shape);
	return vec3(0.0);
}
float shapeSDF( vec3 position, in Shape shape ) {
	if ( shape.type == 1 ) return sphereSDF(position, shape);
	if ( shape.type == 2 ) return planeSDF(position, shape);
	return MAXLEN;
}
// Intersect =======================================================
int intersect(in vec3 rayO, in vec3 rayD, inout float resT) {
	int id = -1;
	for (int i = 0; i < shapes.length(); i++) {
		Shape shape = shapes[i];
		float tShape = shapeIntersect(rayO, rayD, shape);
		if ((tShape > EPSILON) && (tShape < resT)) {
			id = i;
			resT = tShape;
		}	
	}
	return id;
}
float calcShadow(in vec3 rayO, in vec3 rayD, in int objectID, inout float resT) {
	for (int i = 0; i < shapes.length(); i++) {
		if ( i == objectID ) continue;
		Shape shape = shapes[i];
		float tShape = shapeIntersect(rayO, rayD, shape);
		if ((tShape > EPSILON) && (tShape < resT)) {
			resT = tShape;
			return PushConstant.shadowFactor;
		}
	}
	return 1.0;
}
// Marching ========================================
int intersectMarch( in vec3 rayO, in vec3 rayD, inout float resT ) {
	resT = 0;
	for (int i = 0; i < PushConstant.marchingSteps; ++i) {
		vec3 position = resT * rayD + rayO;
		float tNearest = MAXLEN;
		int objectID = -1;
		for ( int j = 0; j < shapes.length(); ++j ) {
			Shape shape = shapes[j];
			float tShape = shapeSDF(position, shape);
		//	if ((tShape > EPSILON) && (tShape < tNearest)) {
			if ( tShape < tNearest ) {
				objectID = j;
				tNearest = tShape;
			}
		}
		if (tNearest < EPSILON) {
			return objectID;
		}
		if (resT > MAXLEN) break;
		resT += tNearest;
	}
	resT = MAXLEN;
	return -1;
}
float calcShadowMarch( in vec3 rayO, in vec3 rayD, in int objectID, inout float resT ) {
	float distance = resT;
	resT = 0;
	for (int i = 0; i < PushConstant.marchingSteps; ++i) {
		vec3 position = resT * rayD + rayO;
		float tNearest = distance;
		int objectID = -1;
		for ( int j = 0; j < shapes.length(); ++j ) {
			if ( j == objectID ) continue;
			Shape shape = shapes[j];
			float tShape = shapeSDF(position, shape);
		//	if ((tShape > EPSILON) && (tShape < tNearest)) {
			if ( tShape < tNearest ) {
				objectID = j;
				tNearest = tShape;
			}
		}
		if (tNearest < EPSILON) {
			return PushConstant.shadowFactor;
		}
		if (resT > distance) break;
		resT += tNearest;
	}
	resT = distance;
	return 1.0;
}

void fog(inout vec3 i, in float t ) {
/*
	vec3 fogColor = ubo.ambient.rgb;
	i = mix(i, fogColor.rgb, clamp(sqrt(t*t)/20.0, 0.0, 1.0));
*/
	if ( ubo.fog.range.x == 0 || ubo.fog.range.y == 0 ) return;

	vec3 color = ubo.fog.color.rgb;
	float inner = ubo.fog.range.x, outer = ubo.fog.range.y;
	float factor = (t - inner) / (outer - inner);
	factor = clamp( factor, 0.0, 1.0 );

	i = mix(i.rgb, color, factor);
}

Result renderScene(inout vec3 rayO, inout vec3 rayD ) {
	Result result;
	result.color = vec3(0.0);
	result.t = MAXLEN;
	result.id = PushConstant.marchingSteps > 0 ? intersectMarch(rayO, rayD, result.t) : intersect(rayO, rayD, result.t);
	if (result.id == -1) return result;
	
	Shape shape = shapes[result.id];
	vec3 position = rayO + result.t * rayD;
	vec3 normal = shapeNormal( position, shape );

	for ( uint i = 0; i < LIGHTS; ++i ) {
		Light light = ubo.lights[i];
		if ( light.radius <= EPSILON ) continue;
		if ( light.power <= EPSILON ) continue;

		vec3 L = light.position - position;
		float dist = length(L);
		vec3 D = normalize(L);
		float attenuation = light.radius / (pow(dist, 2.0) + 1.0);;
		attenuation = 1;
	//	if ( dist > light.radius ) continue;
		
		vec4 albedoSpecular = shape.albedoSpecular;

		float d_dot = lightDiffuse(normal, D);
		float s_factor = lightSpecular(normal, D, albedoSpecular.a);
		vec3 color = (light.color * albedoSpecular.rgb) * d_dot + s_factor;

		// Shadows
		float tShadow = dist;
		float shadowed = 1;
		if ( PushConstant.shadowFactor < 1.0 )
			shadowed = PushConstant.marchingSteps > 0 ? calcShadowMarch(position, D, result.id, tShadow) : calcShadow(position, D, result.id, tShadow);
		result.color += color * light.power * attenuation * shadowed;
	}

	// Fog
	// fog(t, result.color);

	// Reflect ray for next render pass
	reflectRay(rayD, normal);
	rayO = position;	
	
	return result;
}

void main() {
	for ( int pass = 0; pass < EYES; ++pass ) {
		{
			vec2 uv = vec2(gl_GlobalInvocationID.xy) / imageSize(resultImage[pass]);
			vec4 near4 = ubo.matrices[pass] * (vec4(2.0 * uv - 1.0, -1.0, 1.0));
			vec4 far4 = ubo.matrices[pass] * (vec4(2.0 * uv - 1.0, 1.0, 1.0));
			vec3 near3 = near4.xyz / near4.w;
			vec3 far3 = far4.xyz / far4.w;

			state.viewPosition = near3;
			state.ray.origin = near3;
			state.ray.direction = normalize( far3 - near3 );
		}

		// Basic color path
		state.result = renderScene(state.ray.origin, state.ray.direction);
		vec3 finalColor = state.result.color;
		
		// Reflection
		float reflectionStrength = PushConstant.reflectionStrength;
		for (int i = 0; i < PushConstant.rayBounces; i++) {
			Result result = renderScene(state.ray.origin, state.ray.direction);
			vec3 reflectionColor = result.color;

			finalColor = (1.0 - reflectionStrength) * finalColor + reflectionStrength * mix(reflectionColor, finalColor, 1.0 - reflectionStrength);
			reflectionStrength *= PushConstant.reflectionFalloff;
		}
				
		imageStore(resultImage[pass], ivec2(gl_GlobalInvocationID.xy), vec4(finalColor, 1.0));
	}
}