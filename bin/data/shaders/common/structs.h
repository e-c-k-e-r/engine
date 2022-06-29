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
	mat4 view;
	mat4 projection;
	
	vec3 position;
	float radius;
	
	vec3 color;
	float power;
	
	int type;
	int typeMap;
	int indexMap;
	float depthBias;
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
	int padding1;
	int padding2;
	int modeAlpha;
};

struct Texture {
	int index;
	int samp;
	int remap;
	float blend;

	vec4 lerp;
};

struct DrawCommand {
	uint indices; // triangle count
	uint instances; // instance count
	uint indexID; // starting triangle position
	 int vertexID; // starting vertex position

	uint instanceID; // starting instance position
	uint auxID; //
	uint materialID; // material to use for this draw call
	uint vertices; // number of vertices used
};

struct Bounds {
	vec3 min;
	float padding1;
	vec3 max;
	float padding2;
};

struct Instance {
	mat4 model;
	vec4 color;

	uint materialID;
	uint primitiveID;
	uint meshID;
	uint objectID;

	 int jointID;
	 int lightmapID;
	uint imageID;
	uint auxID;

	Bounds bounds;
//	InstanceAddresses addresses;
};

#if UINT64_ENABLED
struct InstanceAddresses {
	uint64_t vertex;
	uint64_t index;
	
	uint64_t indirect;
	uint drawID;
	uint padding0;

	uint64_t position;
	uint64_t uv;

	uint64_t color;
	uint64_t st;

	uint64_t normal;
	uint64_t tangent;

	uint64_t joints;
	uint64_t weights;

	uint64_t id;
	uint64_t padding1;
};
#else
struct InstanceAddresses {
	uvec2 vertex;
	uvec2 index;
	
	uvec2 indirect;
	uint drawID;
	uint padding0;

	uvec2 position;
	uvec2 uv;

	uvec2 color;
	uvec2 st;

	uvec2 normal;
	uvec2 tangent;

	uvec2 joints;
	uvec2 weights;

	uvec2 id;
	uvec2 padding1;
};
#endif

struct SurfaceMaterial {
	vec4 albedo;
	vec4 indirect;

	float metallic;
	float roughness;
	float occlusion;
	bool lightmapped;
};

struct Surface {
	uint pass;
	vec3 uv;
	vec3 st;
	Space position;
	Space normal;
	mat3 tbn;
	
	Ray ray;
	
	SurfaceMaterial material;
	Instance instance;

	vec4 light;
	vec4 fragment;
} surface;

// MSAA info
#if MULTISAMPLING
struct MSAA {
	int currentID;
	uvec2 IDs[MAX_MSAA_SAMPLES];
	vec4 fragment;
	vec4 fragments[MAX_MSAA_SAMPLES];
} msaa;
#endif
// VXGI stuff
struct Vxgi {
	mat4 matrix;

	float cascadePower;
	float granularity;
	float voxelizeScale;
	float occlusionFalloff;

	float traceStartOffsetFactor;
	uint shadows;
	uint padding2;
	uint padding3;
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

// Raytrace stuff

struct Vertex {
	vec3 position;
	vec2 uv;
	uint color;
	vec2 st;
	vec3 normal;
	vec3 tangent;
	uvec2 joints;
	vec4 weights;
	uint id;
};

struct Triangle {
	Vertex point;
	
	vec3 geomNormal;
	
	uint instanceID;
};

struct RayTracePayload {
	bool hit;
	Triangle triangle;
};