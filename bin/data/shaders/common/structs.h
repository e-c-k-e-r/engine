struct EyeMatrices {
	mat4 view;
	mat4 projection;
	mat4 iView;
	mat4 iProjection;
	mat4 iProjectionView;
	vec4 eyePos;
};

struct Viewport {
	mat4 view;
	mat4 projection;
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

	vec2 range;
	float densityThreshold;
	float densityMultiplier;
	
	float absorbtion;
	float padding1;
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
	int textureIndex;
	uint textureSlot;
	uint padding;
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

// VXGI stuff
struct Vxgi {
	mat4 matrix;

	float cascadePower;
	float granularity;
	uint shadows;
	uint padding1;
};
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