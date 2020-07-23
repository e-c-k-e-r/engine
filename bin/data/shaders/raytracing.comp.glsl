#version 450

layout (local_size_x = 32, local_size_y = 32) in;
layout (binding = 0, rgba8) uniform writeonly image2D resultImage;

#define EPSILON 0.00001
#define MAXLEN 1000.0
#define SHADOW 0.5
#define RAYBOUNCES 4
#define REFLECTIONS true
#define REFLECTIONSTRENGTH 0.4
#define REFLECTIONFALLOFF 0.5

#define UINT32_MAX 0xFFFFFFFF

#define TREE_SIZE 8
#define TREE_STACK 32
#define PRIMITIVE_TYPE_EMPTY UINT32_MAX
#define PRIMITIVE_TYPE_CUBE 1
#define PRIMITIVE_TYPE_LEAF 2
#define PRIMITIVE_TYPE_TREE 3
#define PRIMITIVE_TYPE_ROOT 4

struct Camera {
	vec3 position;
	mat4 view;
	float aspectRatio; 
};

struct StartEnd {
	uint start;
	uint end;
};

struct Cube {
	vec4 position;
	uint type;
//	uint _padding[3];
};

struct Tree {
	vec4 position;
	uint type;
	uint children[TREE_SIZE];
//	uint _padding[3];
};

struct Light {
	vec3 position;
	vec3 color;
};

layout (binding = 1) uniform UBO  {
	Camera camera;
	StartEnd cubes;
	StartEnd lights;
	uint root;
} ubo;
layout (std140, binding = 2) buffer Cubes {
	Cube cubes[ ];
};
layout (std430, binding = 3) buffer Trees {
	Tree trees[ ];
};
layout (std140, binding = 4) buffer Lights {
	Light lights[ ];
};
layout (binding = 5) uniform sampler2D samplerTexture;

// Lighting =========================================================
void reflectRay(inout vec3 rayD, in vec3 mormal) {
	rayD = rayD + 2.0 * -dot(mormal, rayD) * mormal;
}
float lightDiffuse(vec3 normal, vec3 lightDir) {
	return clamp(dot(normal, lightDir), 0.1, 1.0);
}
float lightSpecular(vec3 normal, vec3 lightDir, float specularFactor) {
	vec3 viewVec = normalize(ubo.camera.position);
	vec3 halfVec = normalize(lightDir + viewVec);
	return pow(clamp(dot(normal, halfVec), 0.0, 1.0), specularFactor);
}
// Cube ===========================================================
float cubeIntersect(vec3 rayO, vec3 rayD, vec3 rayDRecip, Cube cube) {
	float t[10];
	t[1] = ( cube.position.x - cube.position.w - rayO.x) * rayDRecip.x;
	t[2] = ( cube.position.x + cube.position.w - rayO.x) * rayDRecip.x;
	t[3] = ( cube.position.y - cube.position.w - rayO.y) * rayDRecip.y;
	t[4] = ( cube.position.y + cube.position.w - rayO.y) * rayDRecip.y;
	t[5] = ( cube.position.z - cube.position.w - rayO.z) * rayDRecip.z;
	t[6] = ( cube.position.z + cube.position.w - rayO.z) * rayDRecip.z;
	t[7] = max(max(min(t[1], t[2]), min(t[3], t[4])), min(t[5], t[6]));
	t[8] = min(min(max(t[1], t[2]), max(t[3], t[4])), max(t[5], t[6]));
	t[9] = (t[8] < 0 || t[7] > t[8]) ? 0.0 : t[7];
	return t[9];
}
uint intersect(in vec3 rayO, in vec3 rayD, inout float resT, uint start, uint end) {
	uint id = UINT32_MAX;
	vec3 rayDRecip = 1.0f / rayD;
	for (uint i = start; i < end && i < ubo.cubes.end && i < cubes.length(); i++) {
		Cube cube = cubes[i];
		if ( cube.type == PRIMITIVE_TYPE_EMPTY ) continue;
		float tcube = cubeIntersect(rayO, rayD, rayDRecip, cube);
		if ((tcube > EPSILON) && (tcube < resT)) {
			id = i;
			resT = tcube;
		}	
	}

	return id;
}
// Tree ===========================================================
float treeIntersect( in vec3 rayO, in vec3 rayD, in vec3 rayDRecip, Tree tree ) {
	Cube treecube;
	treecube.type = tree.type;
	treecube.position = tree.position;
	float t = cubeIntersect( rayO, rayD, rayDRecip, treecube );
	return t;
}
struct StackIterator {
	uint tree;
	uint child;
};
struct Stack {
	int pointer;
	StackIterator container[TREE_STACK];
};
Stack stack;

StackIterator popStack( ) {
	StackIterator top = stack.container[stack.pointer];
	stack.container[stack.pointer--] = StackIterator( UINT32_MAX, UINT32_MAX );
	return top;
}
void pushStack( StackIterator item ) {
	stack.container[++stack.pointer] = item;
}
uint intersectTreecursive( in vec3 rayO, in vec3 rayD, in vec3 rayDRecip, inout float resT, uint root ) {
	uint id = UINT32_MAX;
	if ( root == UINT32_MAX ) return id;
	// set up stack
	stack.pointer = -1;
	for ( uint i = 0; i < TREE_STACK; ++i ) stack.container[i] = StackIterator( UINT32_MAX, UINT32_MAX );
	pushStack(StackIterator( root, UINT32_MAX ));

	while ( true ) {
		StackIterator it = popStack();
		// end of stack
		if ( it.tree == UINT32_MAX ) break;
		Tree tree = trees[it.tree];
		// invalid tree
		if ( tree.type == PRIMITIVE_TYPE_EMPTY ) break;
		// new tree, parse collision
		if ( it.child == UINT32_MAX ) {
			float t = treeIntersect( rayO, rayD, rayDRecip, tree );
			// bad intersection with this tree, continue with next iteration
			if ( t <= EPSILON || t >= resT ) continue;
			// push back with new stack
			it.child = 0;
			pushStack( it );
			// continue with next iteration
			continue;
		} else if ( it.child >= TREE_SIZE ) {
			// no new children, continue with next iteration
			continue;
		} else {
			// is leaf
			if ( tree.type == PRIMITIVE_TYPE_LEAF ) {
				// check children for a match
				for ( uint i = 0; i < TREE_SIZE; ++i ) {
					uint branchId = tree.children[i];
					// unallocated, skip
					if ( branchId == UINT32_MAX ) continue;
					Cube primitive = cubes[branchId];
					if ( primitive.type == PRIMITIVE_TYPE_EMPTY ) continue;
					float t = cubeIntersect( rayO, rayD, rayDRecip, primitive );
					// branch intersects with ray, set as new parent
					if ( (t <= EPSILON) || (t >= resT) ) continue;
					id = branchId;
					resT = t;
				}
				// continue with next iteration
				continue;
			}
			// parse children
			uint branchId = tree.children[it.child++];
			// add new iterator to the stack
			pushStack( it );
			// unused child, continue with next iteration
			if ( branchId == UINT32_MAX ) continue;
			// tree branch, push to stack
			// the first if block will check its collision
			it.tree = branchId;
			it.child = UINT32_MAX;
			pushStack( it );
			continue;
		}
	}
	return id;
}

uint intersectTree( in vec3 rayO, in vec3 rayD, in vec3 rayDRecip, inout float resT ) {
	uint id = UINT32_MAX;
	// traverse the tree branches to the leaf
	uint index = 0;
	while ( true ) {
		Tree tree = trees[index++];
		if ( tree.type == PRIMITIVE_TYPE_EMPTY ) break;
		if ( tree.type != PRIMITIVE_TYPE_LEAF ) continue;
		float tcube = treeIntersect( rayO, rayD, rayDRecip, tree );
		// ray fails collision with parent tree
		if ( (tcube <= EPSILON) || (tcube >= resT) ) continue;
		// ray intersects with parent tree, check children branches
		for ( uint i = 0; i < TREE_SIZE; ++i ) {
			uint branchId = tree.children[i];
			// unallocated, skip
			if ( branchId == UINT32_MAX ) continue;
			Cube primitive = cubes[branchId];
			if ( primitive.type == PRIMITIVE_TYPE_EMPTY ) continue;
			tcube = cubeIntersect( rayO, rayD, rayDRecip, primitive );
			// branch intersects with ray, set as new parent
			if ( (tcube <= EPSILON) || (tcube >= resT) ) continue;
			id = branchId;
			resT = tcube;
		}
	}
	return id;
}

float calcShadow(in vec3 rayO, in vec3 rayD, in uint objectId, inout float t, uint start, uint end) {
	vec3 rayDRecip = 1.0f / rayD;
	for (uint i = ubo.cubes.start; i < ubo.cubes.end && i < cubes.length(); i++) {
		if (i == objectId) continue;
		float tCube = cubeIntersect(rayO, rayD, rayDRecip, cubes[i]);
		if ((tCube > EPSILON) && (tCube < t)) {
			t = tCube;
			return SHADOW;
		}
	}		
	return 1.0;
}
float calcShadowTree(in vec3 rayO, in vec3 rayD, in vec3 rayDRecip, in uint objectId, inout float t){
	// traverse the tree branches to the leaf
	uint index = 0;
	while ( true ) {
		Tree tree = trees[index++];
		if ( tree.type == PRIMITIVE_TYPE_EMPTY ) break;
		if ( tree.type != PRIMITIVE_TYPE_LEAF ) continue;
		float tcube = treeIntersect( rayO, rayD, rayDRecip, tree );
		// ray fails collision with parent tree
		if ( (tcube <= EPSILON) || (tcube >= t) ) continue;
		// ray intersects with parent tree, check children branches
		for ( uint i = 0; i < TREE_SIZE; ++i ) {
			uint branchId = tree.children[i];
			// unallocated, skip
			if ( branchId == UINT32_MAX ) continue;
			Cube primitive = cubes[branchId];
			if ( primitive.type == PRIMITIVE_TYPE_EMPTY ) continue;
			if ( branchId == objectId ) continue;
			tcube = cubeIntersect( rayO, rayD, rayDRecip, primitive );
			// branch intersects with ray, set as new parent
			if ( (tcube <= EPSILON) || (tcube >= t) ) continue;
			t = tcube;
			return SHADOW;
		}
	}
	return 1.0;
}

vec3 fog(in float t, in vec3 color) {
	return mix(color, vec3(0, 0, 0), clamp(sqrt(t*t)/20.0, 0.0, 1.0));
}

vec3 renderScene(inout vec3 rayO, inout vec3 rayD, inout uint id) {
	float t = MAXLEN;
	vec3 rayDRecip = 1.0f / rayD;

	// Get intersected object ID
	uint objectID = intersect(rayO, rayD, t, ubo.cubes.start, ubo.cubes.end );
//	uint objectID = intersectTree( rayO, rayD, rayDRecip, t );
//	uint objectID = intersectTreecursive( rayO, rayD, rayDRecip, t, ubo.root );

	vec3 color = vec3(0.0);
	if ( objectID == UINT32_MAX ) return color;
	
	vec3 pos = rayO + t * rayD;
	vec3 normal;

	// Cubes
	if ( id == UINT32_MAX ) return color;
	id = objectID;
	// Hit Data
	{
		Cube cube = cubes[objectID];
		vec2 mappedUv = vec2(0, 0); {
			float min_distance = MAXLEN;
			vec3 point = cube.position.xyz - pos;
			float distance = abs(cube.position.w - abs(point.x));
			if (distance < min_distance) {
				min_distance = distance;
				normal = vec3(-1, 0, 0);
				if ( cube.position.w + point.x <= EPSILON ) normal *= -1;

				mappedUv.x = point.y - cube.position.y;
				mappedUv.y = point.z - cube.position.z;
			}
			distance = abs(cube.position.w - abs(point.y));
			if (distance < min_distance) {
				min_distance = distance;
				normal = vec3(0, -1, 0);
				if ( cube.position.w + point.y <= EPSILON ) normal *= -1;

				mappedUv.x = point.x - cube.position.x;
				mappedUv.y = point.z - cube.position.z;
			}
			distance = abs(cube.position.w - abs(point.z));
			if (distance < min_distance) {
				min_distance = distance;
				normal = vec3(0, 0, -1);
				if ( cube.position.w + point.z <= EPSILON ) normal *= -1;

				mappedUv.x = point.x - cube.position.x;
				mappedUv.y = point.y - cube.position.y;
			}
			mappedUv -= 2.0f / 4.0f;
			mappedUv *= 1.0f / 4.0f;
			mappedUv.x = mod(mappedUv.x, 1.0f / 4.0f);
			mappedUv.y = mod(mappedUv.y, 1.0f / 4.0f);
		}
		vec3 textureMapped = texture(samplerTexture, mappedUv).rgb;
		color = textureMapped;
	}
	// Lighting
	{
		int di = 0;
		vec4 diffuses[256];
		for ( int j = 0; j < lights.length(); ++j ) {	
			Light light = lights[j];
			if ( light.color.r <= EPSILON && light.color.g <= EPSILON && light.color.b <= EPSILON ) continue;
			vec3 lightVec = normalize(light.position - pos);
			float diffuse = lightDiffuse(normal, lightVec);
			float specular = lightSpecular(normal, lightVec, 2000.0f);
			diffuses[di].x = length(light.position - pos);
			diffuses[di].gba = diffuse * light.color + specular;
			++di;
		}
		vec4 a = vec4(MAXLEN, vec3(1.0));
		vec4 b = vec4(MAXLEN, vec3(1.0));
		vec4 c = vec4(MAXLEN, vec3(1.0));
		for ( int j = 0; j < lights.length(); ++j ) {
			vec4 current = diffuses[j];
			if ( current.y <= EPSILON ) continue;
			// use slot a
			if ( current.x < a.x ) {
				c = b;
				b = a;
				a = current;
			} else if ( current.x < b.x ) {
				c = b;
				b = current;
			} else if ( current.x < c.x ) {
				c = current;
			}
		}
		vec3 avg = (a.gba * 0.5 + b.gba * 0.3 + c.gba * 0.2);
		color *= avg;
	}
	// Shadows
	{
		vec3 lightVec = vec3(0, 4, 0) - pos;
		t = length(lightVec);
		color *= calcShadow(pos, lightVec, id, t, ubo.cubes.start, ubo.cubes.end);
	//	color *= calcShadowTree(pos, lightVec, rayDRecip, id, t);
	}

	// Fog
	// color = fog(t, color);	
	
	// Reflect ray for next render pass
	reflectRay(rayD, normal);
	rayO = pos;
	
	return color;
}

void main() {
	ivec2 dim = imageSize(resultImage);
	vec2 uv = vec2(gl_GlobalInvocationID.xy) / dim;

	vec3 rayO = ubo.camera.position; // * vec3(1,1,-1);
	vec3 rayD = normalize(vec3((-1.0 + 2.0 * uv) * vec2(ubo.camera.aspectRatio, 1.0), -1.0));
	rayD = (ubo.camera.view * vec4( rayD, 0.0 )).xyz;
		
	// Basic color path
	uint id = 0;
	vec3 finalColor = renderScene(rayO, rayD, id);
	
	// Reflection
	if ( REFLECTIONS ) {
		float reflectionStrength = REFLECTIONSTRENGTH;
		for (int i = 0; i < RAYBOUNCES; i++) {
			vec3 reflectionColor = renderScene(rayO, rayD, id);
			finalColor = (1.0 - reflectionStrength) * finalColor + reflectionStrength * mix(reflectionColor, finalColor, 1.0 - reflectionStrength);			
			reflectionStrength *= REFLECTIONFALLOFF;
		}
	}
			
	imageStore(resultImage, ivec2(gl_GlobalInvocationID.xy), vec4(finalColor, 0.0));
}