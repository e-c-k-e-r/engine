#pragma once

#include <uf/config.h>
#include <string>
#include <uf/utils/math/vector.h>
#include <uf/utils/math/transform.h>

namespace pod {
	struct UF_API SceneTextures {
		uf::renderer::Texture3D noise;
		uf::renderer::TextureCube skybox;
		struct {
			std::vector<uf::renderer::Texture3D> id;
			std::vector<uf::renderer::Texture3D> uv;
			std::vector<uf::renderer::Texture3D> normal;
			std::vector<uf::renderer::Texture3D> radiance;
			
			pod::Matrix4f matrix;
			float cascadePower = 2.0;
		} voxels;
	};

	struct UF_API DrawCall {
		struct Storage {
			alignas(4) int32_t materialIndex = -1;
			alignas(4) uint32_t materials = 0;
			alignas(4) int32_t textureIndex = -1;
			alignas(4) uint32_t textures = 0;
		} storage;
		std::string name = "";
		size_t verticesIndex = 0;
		size_t vertices = 0;
		size_t indicesIndex = 0;
		size_t indices = 0;
	};
	struct UF_API Light {
		struct UF_API Storage {
			alignas(16) pod::Vector4f position;
			alignas(16) pod::Vector4f color;

			alignas(4) int32_t type = 0;
			alignas(4) int32_t mapIndex = -1;
			alignas(4) float depthBias = 0;
			alignas(4) float padding = 0;
			
			alignas(16) pod::Matrix4f view;
			alignas(16) pod::Matrix4f projection;
		};
		std::string name = "";
		pod::Vector3f color = { 1, 1, 1 };
		float intensity = 1.0f;
		float range = 0.0f;
	};
	struct UF_API Material {
		struct Storage {
			alignas(16) pod::Vector4f colorBase = { 0, 0, 0, 0 };
			alignas(16) pod::Vector4f colorEmissive = { 0, 0, 0, 0 };
			
			alignas(4) float factorMetallic = 0.0f;
			alignas(4) float factorRoughness = 0.0f;
			alignas(4) float factorOcclusion = 0.0f;
			alignas(4) float factorAlphaCutoff = 1.0f;

			alignas(4) int32_t indexAlbedo = -1;
			alignas(4) int32_t indexNormal = -1;
			alignas(4) int32_t indexEmissive = -1;
			alignas(4) int32_t indexOcclusion = -1;

			alignas(4) int32_t indexMetallicRoughness = -1;
			alignas(4) int32_t indexAtlas = -1;
			alignas(4) int32_t indexLightmap = -1;
			alignas(4) int32_t modeAlpha = -1;
		} storage;
		std::string name = "";
		std::string alphaMode = "";
	};
	struct UF_API Skin {
		std::string name = "";
		std::vector<int32_t> joints;
		std::vector<pod::Matrix4f> inverseBindMatrices;
	};
	struct UF_API Animation {
		struct Sampler {
			std::string interpolator;
			std::vector<float> inputs;
			std::vector<pod::Vector4f> outputs;
		};
		struct Channel {
			std::string path;
			int32_t node;
			uint32_t sampler;
		};

		std::string name = "";
		std::vector<Sampler> samplers;
		std::vector<Channel> channels;
		float start = std::numeric_limits<float>::max();
		float end = std::numeric_limits<float>::min();
		float cur = 0;
	};
}