namespace pod {
	template<typename T, typename U = uint32_t>
	struct UF_API Meshlet_T {
		uf::stl::vector<T> vertices;
		uf::stl::vector<U> indices;

		pod::Primitive primitive;
	};

	struct UF_API Material {
		pod::Vector4f colorBase = { 0, 0, 0, 0 };
		pod::Vector4f colorEmissive = { 0, 0, 0, 0 };
		
		float factorMetallic = 0.0f;
		float factorRoughness = 0.0f;
		float factorOcclusion = 0.0f;
		float factorAlphaCutoff = 1.0f;

		int32_t indexAlbedo = -1;
		int32_t indexNormal = -1;
		int32_t indexEmissive = -1;
		int32_t indexOcclusion = -1;

		int32_t indexMetallicRoughness = -1;
		int32_t padding1 = -1;
		int32_t padding2 = -1;
		int32_t modeAlpha = -1;
	};
	
	struct UF_API Texture {
		int32_t index = -1;
		int32_t sampler = -1;
		int32_t remap = -1;
		float blend = 0;

		pod::Vector4f lerp = {0, 0, 1, 1};
	};

	struct UF_API Light {
		pod::Matrix4f view;
		pod::Matrix4f projection;

		pod::Vector3f position;
		alignas(4) float range = 0.0f;
		pod::Vector3f color;
		alignas(4) float intensity = 0.0f;

		int32_t type = 0;
		int32_t typeMap = 0;
		int32_t indexMap = -1;
		float depthBias = 0;
	};
	//
	struct UF_API Skin {
		uf::stl::string name = "";

		uf::stl::vector<int32_t> joints;
		uf::stl::vector<pod::Matrix4f> inverseBindMatrices;
	};

	struct UF_API Animation {
		struct Sampler {
			uf::stl::string interpolator;
			uf::stl::vector<float> inputs;
			uf::stl::vector<pod::Vector4f> outputs;
		};

		struct Channel {
			uf::stl::string path;
			int32_t node;
			uint32_t sampler;
		};

		uf::stl::string name = "";
		uf::stl::string path = "";

		uf::stl::vector<Sampler> samplers;
		uf::stl::vector<Channel> channels;
		float start = std::numeric_limits<float>::max();
		float end = std::numeric_limits<float>::min();
		float cur = 0;
	};
	//
	struct UF_API Node {
		uf::stl::string name = "";

		int32_t index = -1;
		int32_t parent = -1;
		int32_t mesh = -1;
		int32_t skin = -1;
		uf::stl::vector<int32_t> children;

		uf::Object* entity = NULL;
		pod::Transform<> transform;
	};
}