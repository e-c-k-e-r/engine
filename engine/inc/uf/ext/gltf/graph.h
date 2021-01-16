#pragma once

#include <uf/utils/image/atlas.h>
#include <uf/engine/object/object.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/utils/math/collision.h>

#include "mesh.h"

#include <queue>

namespace pod {
	struct UF_API Texture {
		std::string name = "";
		int32_t index = -1;
		int32_t sampler = -1;
	};
	struct UF_API Light {
		std::string name = "";
		pod::Transform<> transform = {};
		pod::Vector3f color = { 1, 1, 1 };
		float intensity = 1.0f;
		float range = 0.0f;
	};
/*
	struct UF_API Instance {
		alignas(16) pod::Matrix4f model = uf::matrix::identity();
		alignas(4) int32_t materialId = -1;
	};
*/
	struct UF_API Material {
		std::string name = "";
		// 
		struct Storage {
			alignas(16) pod::Vector4f colorBase = { 1, 0, 1, 1 };

			alignas(16) pod::Vector4f colorEmissive = { 0, 0, 0, 0 };
			
			alignas(4) float factorMetallic = 0.0f;
			alignas(4) float factorRoughness = 0.0f;
			alignas(4) float factorOcclusion = 0.0f;
			alignas(4) float factorMappedBlend = 0.0f;

			alignas(4) float factorAlphaCutoff = 1.0f;
			alignas(4) float factorPadding = 0.0f;
			alignas(4) int indexAlbedo = -1;
			alignas(4) int indexNormal = -1;

			alignas(4) int indexEmissive = -1;
			alignas(4) int indexOcclusion = -1;
			alignas(4) int indexMetallicRoughness = -1;
			alignas(4) int indexMappedTarget = -1;
		} storage;
	};
	struct UF_API Node {
		typedef ext::gltf::skinned_mesh_t Mesh;

		std::string name = "";
		int32_t index = -1;

	//	Node* parent = NULL;
	//	std::vector<Node*> children;
		int32_t parent = -1;
		std::vector<int32_t> children;

		uf::Object* entity = NULL;
		size_t jointBufferIndex = -1;
		size_t materialBufferIndex = -1;

		int32_t skin = -1;
		Mesh mesh;
		uf::Collider collider;

		pod::Transform<> transform;
	};

	struct UF_API Skin {
		std::string name = "";
	//	std::vector<Node*> joints;
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
		//	Node* node;
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
	struct UF_API Graph {
	//	Node* node = NULL;
		pod::Node root;
		std::vector<pod::Node> nodes;

		uf::Object* entity = NULL;
		size_t instanceBufferIndex = -1;

		std::string name = "";
		ext::gltf::load_mode_t mode;
		uf::Serializer metadata;

		uf::Atlas* atlas = NULL;
		std::vector<uf::Image> images;
		std::vector<ext::vulkan::Sampler> samplers;
		std::vector<pod::Texture> textures;
		std::vector<pod::Material> materials;
		std::vector<pod::Light> lights;

		std::vector<Skin> skins;
		std::unordered_map<std::string, Animation> animations;
		std::queue<std::string> sequence;
		struct {
			struct {
				bool loop = true;
				float speed = 1;
				struct {
					float a = -std::numeric_limits<float>::max();
					float speed = 1 / 0.125f;
					std::unordered_map<int32_t, std::pair<pod::Transform<>, pod::Transform<>>> map;
				} override;
			} animations;
		} settings;
	};
}

namespace uf {
	namespace graph {
	/*
		pod::Node& UF_API node();
		pod::Node* UF_API find( const pod::Node& node, int32_t index );
		pod::Node* UF_API find( pod::Node* node, int32_t index );
		pod::Node* UF_API find( const pod::Graph& graph, int32_t index );

		pod::Node* UF_API find( const pod::Node& node, const std::string& name );
		pod::Node* UF_API find( pod::Node* node, const std::string& name );
		pod::Node* UF_API find( const pod::Graph& graph, const std::string& name );

		pod::Matrix4f UF_API local( const pod::Node& node );
		pod::Matrix4f UF_API matrix( const pod::Node& node );
	*/
		pod::Node* UF_API find( pod::Graph& graph, int32_t index );
		pod::Node* UF_API find( pod::Graph& graph, const std::string& name );

	//	pod::Matrix4f UF_API local( const pod::Node& node );
	//	pod::Matrix4f UF_API matrix( pod::Graph&, const pod::Node& node );
		
		pod::Matrix4f UF_API local( pod::Graph&, int32_t );
		pod::Matrix4f UF_API matrix( pod::Graph&, int32_t );

		void UF_API process( uf::Object& entity );
		void UF_API process( pod::Graph& graph );
		void UF_API process( pod::Graph& graph, int32_t, uf::Object& parent );

		void UF_API override( pod::Graph& );
		void UF_API animate( pod::Graph&, const std::string&, float = 1, bool = true );
		void UF_API update( pod::Graph& );
		void UF_API update( pod::Graph&, float );
		void UF_API update( pod::Graph&, pod::Node& );
		
		void UF_API destroy( pod::Graph& );
	}
}