struct Matrices {
	mat4 view[2];
	mat4 projection[2];
	mat4 iView[2];
	mat4 iProjection[2];
	mat4 iProjectionView[2];
	vec4 eyePos[2];
	mat4 vxgi;
};

struct Cursor {
	vec2 position;
	vec2 radius;
	vec4 color;
};

struct Ray {
	vec3 origin;
	vec3 direction;

	vec3 position;
	float distance;
};

struct Space {
	vec3 eye;
	vec3 world;
};

struct Fog {
	vec3 color;
	float stepScale;

	vec3 offset;
	float densityScale;

	float densityThreshold;
	float densityMultiplier;
	float absorbtion;
	float padding1;

	vec2 range;
	float padding2;
	float padding3;
};

struct Mode {
	uint type;
	uint scalar;
	vec2 padding;
	vec4 parameters;
};

struct Light {
	vec3 position;
	float radius;
	
	vec3 color;
	float power;
	
	int type;
	int typeMap;
	int indexMap;
	float depthBias;

	mat4 view;
	mat4 projection;
};

struct Material {
	vec4 colorBase;
	vec4 colorEmissive;

	float factorMetallic;
	float factorRoughness;
	float factorOcclusion;
	float factorAlphaCutoff;

	int indexAlbedo;
	int indexNormal;
	int indexEmissive;
	int indexOcclusion;
	
	int indexMetallicRoughness;
	int indexAtlas;
	int indexLightmap;
	int modeAlpha;
};

struct Texture {
	int index;
	int samp;
	int remap;
	float blend;

	vec4 lerp;
};

struct DrawCall {
	int materialIndex;
	uint materials;
	int textureIndex;
	uint textures;
};

struct SurfaceMaterial {
	uint id;

	vec4 albedo;
	vec4 indirect;

	float metallic;
	float roughness;
	float occlusion;
	int indexLightmap;

};

struct Surface {
	uint pass;
	vec2 uv;
	Space position;
	Space normal;
	
	Ray ray;
	
	SurfaceMaterial material;

	vec4 fragment;
} surface;

struct Voxel {
	uvec2 id;
	vec3 position;
	vec3 normal;
	vec2 uv;
	vec4 color;
};

struct VoxelInfo {
	vec3 min;
	vec3 max;

	float mipmapLevels;
	float radianceSize;
	float radianceSizeRecip;
} voxelInfo;