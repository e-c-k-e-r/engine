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
			/*alignas(4)*/ int32_t index = -1;
			/*alignas(4)*/ int32_t sampler = -1;
			/*alignas(4)*/ int32_t remap = -1;
			/*alignas(4)*/ float blend = 0;

			/*alignas(16)*/ pod::Vector4f lerp = {0, 0, 1, 1};
		} storage;
		uf::stl::string name = "";
		uf::renderer::Texture2D texture;
		bool bind = false;
	};
	struct UF_API Node {
		uf::stl::string name = "";

		int32_t index = -1;
		int32_t skin = -1;
		int32_t parent = -1;
		int32_t mesh = -1;

		pod::Transform<> transform;
		uf::stl::vector<int32_t> children;

		uf::Object* entity = NULL;
		struct {
			int32_t material = -1;
			int32_t texture = -1;
			int32_t joint = -1;
		} buffers;
	};
	struct UF_API Graph {
		typedef uf::graph::skinned_mesh_t Mesh;

	//	Node* node = NULL;
		pod::Node root;
		uf::stl::vector<pod::Node> nodes;

		uf::Object* entity = NULL;
		
		struct {
			int32_t instance = -1;
		} buffers;

		uf::stl::string name = "";
		uf::graph::load_mode_t mode;
		uf::Serializer metadata;

		uf::Atlas atlas;
		uf::stl::vector<uf::Image> images;
		uf::stl::vector<uf::renderer::Sampler> samplers;
		uf::stl::vector<pod::Texture> textures;
		uf::stl::vector<pod::Material> materials;
		uf::stl::vector<pod::Light> lights;
		uf::stl::vector<pod::DrawCall> drawCalls;

		uf::stl::vector<Skin> skins;
		uf::stl::vector<Mesh> meshes;
		uf::stl::unordered_map<uf::stl::string, Animation> animations;

		std::queue<uf::stl::string> sequence;
		struct {
			struct {
				bool loop = true;
				float speed = 1;
				struct {
					float a = -std::numeric_limits<float>::max();
					float speed = 1 / 0.125f;
					uf::stl::unordered_map<int32_t, std::pair<pod::Transform<>, pod::Transform<>>> map;
				} override;
			} animations;
		} settings;
	};
}

namespace uf {
	namespace graph {
	/*
		namespace {
			extern UF_API uf::stl::vector<pod::Material::Storage> storage;
			extern UF_API uf::stl::vector<uf::stl::string> indices;
		} materials;
		namespace {
			extern UF_API uf::stl::vector<pod::Texture::Storage> storage;
			extern UF_API uf::stl::vector<uf::stl::string> indices;
		} textures;
	*/

		pod::Node* UF_API find( pod::Graph& graph, int32_t index );
		pod::Node* UF_API find( pod::Graph& graph, const uf::stl::string& name );

		pod::Matrix4f UF_API local( pod::Graph&, int32_t );
		pod::Matrix4f UF_API matrix( pod::Graph&, int32_t );

		void UF_API process( uf::Object& entity );
		void UF_API process( pod::Graph& graph );
		void UF_API process( pod::Graph& graph, int32_t, uf::Object& parent );
		void UF_API cleanup( pod::Graph& graph );
		void UF_API initialize( pod::Graph& graph );

		void UF_API override( pod::Graph& );
		void UF_API animate( pod::Graph&, const uf::stl::string&, float = 1, bool = true );
		void UF_API update( pod::Graph& );
		void UF_API update( pod::Graph&, float );
		void UF_API update( pod::Graph&, pod::Node& );
		
		void UF_API destroy( pod::Graph& );

		pod::Graph UF_API load( const uf::stl::string&, uf::graph::load_mode_t = 0, const uf::Serializer& = ext::json::null() );
		void UF_API save( const pod::Graph&, const uf::stl::string& );

		uf::stl::string UF_API print( const pod::Graph& graph );
		uf::Serializer UF_API stats( const pod::Graph& graph );
	}
}