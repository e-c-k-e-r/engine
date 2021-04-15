#pragma once

#include <uf/utils/image/atlas.h>
#include <uf/engine/object/object.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/utils/math/collision.h>

#include "mesh.h"
#include "pod.h"

#include <queue>

namespace pod {
	struct UF_API Texture {
		struct Storage {
			alignas(4) int32_t index = -1;
			alignas(4) int32_t sampler = -1;
			alignas(4) int32_t remap = -1;
			alignas(4) float blend = 0;

			alignas(16) pod::Vector4f lerp = {0, 0, 1, 1};
		} storage;
		std::string name = "";
		uf::renderer::Texture2D texture;
		bool bind = false;
	};
	struct UF_API Node {
		std::string name = "";
		int32_t index = -1;
		int32_t skin = -1;
		int32_t parent = -1;
		int32_t mesh = -1;
		std::vector<int32_t> children;
		
		pod::Transform<> transform;

		uf::Object* entity = NULL;
		uf::Collider collider;
		int32_t jointBufferIndex = -1;
		int32_t materialBufferIndex = -1;
		int32_t textureBufferIndex = -1;
	};
	struct UF_API Graph {
		typedef ext::gltf::skinned_mesh_t Mesh;

	//	Node* node = NULL;
		pod::Node root;
		std::vector<pod::Node> nodes;

		uf::Object* entity = NULL;
		size_t instanceBufferIndex = -1;

		std::string name = "";
		ext::gltf::load_mode_t mode;
		uf::Serializer metadata;

		uf::Atlas atlas;
		std::vector<uf::Image> images;
		std::vector<uf::renderer::Sampler> samplers;
		std::vector<pod::Texture> textures;
		std::vector<pod::Material> materials;
		std::vector<pod::Light> lights;
		std::vector<pod::DrawCall> drawCalls;

		std::vector<Skin> skins;
		std::vector<Mesh> meshes;
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
		pod::Node* UF_API find( pod::Graph& graph, int32_t index );
		pod::Node* UF_API find( pod::Graph& graph, const std::string& name );

		pod::Matrix4f UF_API local( pod::Graph&, int32_t );
		pod::Matrix4f UF_API matrix( pod::Graph&, int32_t );

		void UF_API process( uf::Object& entity );
		void UF_API process( pod::Graph& graph );
		void UF_API process( pod::Graph& graph, int32_t, uf::Object& parent );
		void UF_API cleanup( pod::Graph& graph );
		void UF_API initialize( pod::Graph& graph );

		void UF_API override( pod::Graph& );
		void UF_API animate( pod::Graph&, const std::string&, float = 1, bool = true );
		void UF_API update( pod::Graph& );
		void UF_API update( pod::Graph&, float );
		void UF_API update( pod::Graph&, pod::Node& );
		
		void UF_API destroy( pod::Graph& );

		pod::Graph UF_API load( const std::string&, ext::gltf::load_mode_t = 0, const uf::Serializer& = ext::json::null() );
		void UF_API save( const pod::Graph&, const std::string& );

		std::string UF_API print( const pod::Graph& graph );
		uf::Serializer UF_API stats( const pod::Graph& graph );
	}
}