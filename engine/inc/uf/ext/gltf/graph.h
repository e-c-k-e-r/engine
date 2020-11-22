#pragma once

#include <uf/utils/image/atlas.h>
#include <uf/engine/object/object.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/utils/math/collision.h>

#include "mesh.h"

namespace pod {
	struct UF_API Node {
		typedef ext::gltf::skinned_mesh_t Mesh;

		int32_t index = -1;

		Node* parent = NULL;
		std::vector<Node*> children;
		uf::Object* entity = NULL;
		uf::renderer::Buffer* buffer = NULL;
		size_t bufferIndex = -1;

		int32_t skin = -1;
		Mesh mesh;
		uf::Collider collider;

		pod::Transform<> transform;
	};

	struct UF_API Skin {
		std::string name;
		std::vector<Node*> joints;
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
			Node* node;
			uint32_t sampler;
		};

		std::string name;
		std::vector<Sampler> samplers;
		std::vector<Channel> channels;
		float start = std::numeric_limits<float>::max();
		float end = std::numeric_limits<float>::min();
		float cur = 0;
	};
	struct UF_API Graph {
		Node* node = NULL;
		uf::Object* entity = NULL;

		ext::gltf::load_mode_t mode;
	//	uint32_t animation = 0;
		std::string animation = "";

		std::vector<Skin> skins;
	//	std::vector<Animation> animations;
		std::unordered_map<std::string, Animation> animations;
		uf::Atlas* atlas = NULL;
		std::vector<ext::vulkan::Sampler> samplers;
	};
}

namespace uf {
	namespace graph {
		pod::Node& UF_API node();
		pod::Node* UF_API find( const pod::Node& node, int32_t index );
		pod::Node* UF_API find( pod::Node* node, int32_t index );
		pod::Node* UF_API find( const pod::Graph& graph, int32_t index );
		pod::Matrix4f UF_API local( const pod::Node& node );
		pod::Matrix4f UF_API matrix( const pod::Node& node );

		void UF_API process( uf::Object& entity );
		void UF_API process( pod::Graph& graph );
		void UF_API process( pod::Graph& graph, pod::Node& node, uf::Object& parent );

		void UF_API animate( pod::Graph&, const std::string& );
		void UF_API update( pod::Graph& );
		void UF_API update( pod::Graph&, float );
		void UF_API update( pod::Graph&, pod::Node& );
		
		void UF_API destroy( pod::Graph& );
	}
}