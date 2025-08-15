#pragma once

#include <uf/utils/image/atlas.h>
#include <uf/engine/object/object.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/utils/memory/unordered_map.h>
#include <uf/utils/memory/key_map.h>
#include <uf/utils/memory/queue.h>

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
		uf::stl::vector<pod::Node> nodes; //

		// Render information
		uf::stl::vector<uf::stl::string> instances; //
		uf::stl::vector<uf::stl::string> primitives; //
		uf::stl::vector<uf::stl::string> drawCommands; //
		uf::stl::vector<uf::stl::string> meshes; //

		uf::stl::vector<uf::stl::string> images; //
		uf::stl::vector<uf::stl::string> materials; //
		uf::stl::vector<uf::stl::string> textures; //

		uf::stl::vector<uf::stl::string> texture2Ds; //
		uf::stl::vector<uf::stl::string> samplers; //

		// Lighting information
		uf::stl::unordered_map<uf::stl::string, pod::Light> lights;

		// Animations
		uf::stl::vector<uf::stl::string> skins;
		uf::stl::vector<uf::stl::string> animations;
		// Animation queue
		uf::stl::queue<uf::stl::string> sequence;

		// Streaming stuff
		uf::stl::unordered_map<uf::stl::string, uf::stl::string> buffer_paths; // probably will go unused since cramming it all in here is pain

		struct {
			struct {
				bool loop = true;
				float speed = 1;
				struct {
					float a = -std::numeric_limits<float>::max();
					float speed = 1 / 0.125f;
					uf::stl::unordered_map<int32_t, std::pair<pod::Transform<>, pod::Transform<>>> map;
				} override;

				uf::stl::string target = "";
			} animations;

			struct {
				bool enabled = false;

				float radius = 64.0f;
				float every = 4.0f;

				bool textures = true;
				bool animations = true;

				uf::stl::string tag = "worldspawn";
				uf::stl::string player = "info_player_spawn";
				
				size_t hash = 0;
				float lastUpdate = 0;
			} stream;
		} settings;

		// Local storage, used for save/load
		struct Storage {
			uf::stl::KeyMap<pod::Instance> instances;
			uf::stl::KeyMap<pod::Instance::Addresses> instanceAddresses;
			uf::stl::KeyMap<uf::stl::vector<pod::Primitive>> primitives;
			uf::stl::KeyMap<uf::stl::vector<pod::DrawCommand>> drawCommands;
			uf::stl::KeyMap<uf::Mesh> meshes;

			uf::stl::KeyMap<uf::Image> images;
			uf::stl::KeyMap<pod::Material> materials;
			uf::stl::KeyMap<pod::Texture> textures;
			uf::stl::KeyMap<uf::renderer::Sampler> samplers;
			uf::stl::vector<pod::Light> lights;
			uf::stl::KeyMap<pod::Skin> skins;
			uf::stl::KeyMap<pod::Animation> animations;

			// maps without direct analogues
			uf::stl::KeyMap<uf::Atlas> atlases;
			uf::stl::KeyMap<uf::stl::vector<pod::Matrix4f>> joints;
			uf::stl::KeyMap<uf::renderer::Texture2D> texture2Ds;
			uf::stl::KeyMap<uf::Entity*> entities;
			
			uf::stl::vector<uf::renderer::Texture2D> shadow2Ds;
			uf::stl::vector<uf::renderer::TextureCube> shadowCubes;

			struct Buffer {
				uf::renderer::Buffer camera;
				uf::renderer::Buffer drawCommands;
				uf::renderer::Buffer instance;
				uf::renderer::Buffer instanceAddresses;
				uf::renderer::Buffer joint;
				uf::renderer::Buffer material;
				uf::renderer::Buffer texture;
				uf::renderer::Buffer light;
			} buffers;
		}/* storage*/;
	};
}

namespace uf {
	namespace graph {
		extern UF_API size_t initialBufferElements;
		extern UF_API bool globalStorage;
		extern UF_API pod::Graph::Storage storage;
	}
}

namespace uf {
	namespace graph {
		pod::Node* UF_API find( pod::Graph& graph, int32_t index );
		pod::Node* UF_API find( pod::Graph& graph, const uf::stl::string& name );

		pod::Matrix4f UF_API local( pod::Graph&, int32_t );
		pod::Matrix4f UF_API matrix( pod::Graph&, int32_t );

	//	void UF_API process( uf::Object& entity );
		void UF_API initializeGraphics( pod::Graph& graph, uf::Object& entity, uf::Mesh& mesh );
		void UF_API process( pod::Graph& graph );
		void UF_API process( pod::Graph& graph, int32_t, uf::Object& parent );
		void UF_API cleanup( pod::Graph& graph );
		void UF_API reload( pod::Graph& );
		void UF_API initialize( pod::Graph& graph );

		void UF_API override( pod::Graph& );
		void UF_API animate( pod::Graph&, const uf::stl::string&, float = 1, bool = true );
		void UF_API update( pod::Graph& );
		void UF_API update( pod::Graph&, float );
		void UF_API update( pod::Graph&, pod::Node& );
		
		void UF_API destroy( pod::Graph& );
		
		void UF_API initialize();
		void UF_API tick();
		void UF_API render();
		void UF_API destroy( bool soft = false );

		void UF_API initialize( uf::Object& );
		void UF_API initialize( pod::Graph::Storage& );
		void UF_API tick( uf::Object& );
		void UF_API tick( pod::Graph::Storage& );
		void UF_API render( uf::Object& );
		void UF_API render( pod::Graph::Storage& );
		void UF_API destroy( uf::Object&, bool soft = false );
		void UF_API destroy( pod::Graph::Storage&, bool soft = false );

		void UF_API load( pod::Graph&, const uf::stl::string&, const uf::Serializer& = ext::json::null() );
		inline pod::Graph load( const uf::stl::string& filename, const uf::Serializer& metadata = ext::json::null() ) {
			// do some deprecation warning or something because this actually is bad for doing a copy + dealloc
			pod::Graph graph;
			load( graph, filename, metadata );
			return graph;
		}

		pod::Graph& UF_API convert( uf::Object&, bool = false );
		uf::stl::string UF_API save( const pod::Graph&, const uf::stl::string& );

		uf::stl::string UF_API print( const pod::Graph& graph );
		uf::Serializer UF_API stats( const pod::Graph& graph );
		void UF_API reload( pod::Graph& graph );
		void UF_API reload( pod::Graph& graph, pod::Node& node );
		void UF_API reload();
	}
}

namespace pod {
	namespace payloads {
		struct QueueAnimationPayload {
			bool loop = true;
			float speed = 1.0f;
			uf::stl::string name = "";
		};
	}
}