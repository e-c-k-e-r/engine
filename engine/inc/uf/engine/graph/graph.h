#pragma once

#include <uf/utils/image/atlas.h>
#include <uf/engine/object/object.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/utils/memory/fifo_map.h>

#include <queue>

#define UF_GRAPH_VARYING_MESH 1
#define UF_GRAPH_INDIRECT_DRAW 1

#include "pod.inl"
#include "mesh.inl"
#include "scene.inl"


namespace pod {
	struct UF_API Graph {
		uf::stl::string name = "";
		uf::Serializer metadata;

		pod::Node root;
		uf::stl::vector<pod::Node> nodes; // node's position corresponds to its drawCommand and instance

		// Render information
		uf::stl::vector<uf::stl::vector<pod::DrawCommand>> drawCommands; // draws to dispatch, one per primitive, gets copied per rendermode

		uf::stl::vector<pod::Instance> instances; // instance data to use, gets copied per rendermode

		uf::stl::vector<uf::stl::string> meshes; // collection of primitives (stored as meshes, by material)

		uf::stl::vector<uf::stl::string> images; // references global pool of images
		uf::stl::vector<uf::stl::string> materials; // references global pool of materials
		uf::stl::vector<uf::stl::string> textures; // references global pool of textures

		uf::stl::vector<uf::stl::string> texture2Ds; // references global pool of texture2Ds
		uf::stl::vector<uf::stl::string> samplers; // references global pool of samplers

		// Lighting information
		uf::stl::unordered_map<uf::stl::string, pod::Light> lights;

		// Animations
		uf::stl::vector<Skin> skins;
		uf::stl::unordered_map<uf::stl::string, Animation> animations;
		// Animation queue
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

		// Local storage, used for save/load
		struct Storage {
			uf::stl::fifo_map<uf::stl::string, uf::Atlas> atlases;
			uf::stl::fifo_map<uf::stl::string, uf::Image> images;
			uf::stl::fifo_map<uf::stl::string, pod::Mesh> meshes;
			uf::stl::fifo_map<uf::stl::string, pod::Material> materials;
			uf::stl::fifo_map<uf::stl::string, pod::Texture> textures;

			uf::stl::fifo_map<uf::stl::string, uf::renderer::Texture2D> texture2Ds;
			uf::stl::fifo_map<uf::stl::string, uf::renderer::Sampler> samplers;
		};
	};
}

namespace uf {
	namespace graph {
		extern UF_API pod::Graph::Storage storage;
	}
}

namespace uf {
	namespace graph {
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

		pod::Graph UF_API load( const uf::stl::string&, const uf::Serializer& = ext::json::null() );
		void UF_API save( const pod::Graph&, const uf::stl::string& );

		uf::stl::string UF_API print( const pod::Graph& graph );
		uf::Serializer UF_API stats( const pod::Graph& graph );
	}
}