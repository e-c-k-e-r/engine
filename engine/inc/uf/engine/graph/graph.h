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
		uf::stl::vector<uf::stl::string> primitives; //
		uf::stl::vector<uf::stl::string> meshes; //

		uf::stl::vector<uf::stl::string> images; //
		uf::stl::vector<uf::stl::string> materials; //
		uf::stl::vector<uf::stl::string> textures; //
		uf::stl::vector<uf::stl::string> samplers; //

		// Lighting information
		uf::stl::unordered_map<uf::stl::string, pod::Light> lights;

		// Animations
		uf::stl::vector<uf::stl::string> skins;
		uf::stl::vector<uf::stl::string> animations;
		// Animation queue
		uf::stl::queue<uf::stl::string> sequence;

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

				int32_t world = -1;
				int32_t player = -1;
				
				size_t hash = 0;
				float lastUpdate = 0;
			} stream;
		} settings;

		struct StreamRegistry {
			uf::stl::unordered_map<uf::stl::string, pod::AnimationStream> animations;
			uf::stl::unordered_map<uf::stl::string, pod::SkinStream> skins;
			uf::stl::unordered_map<uf::stl::string, pod::MeshStream> meshes;
			uf::stl::unordered_map<uf::stl::string, pod::ImageStream> images;
			uf::stl::unordered_map<uf::stl::string, pod::BvhStream> bvhs;
		} streams;

		// Local storage, used for save/load
		struct Storage {
			enum StorageType : uint32_t {
				OBJECT,
				GRAPH,
				SCENE,
				GLOBAL,
			};

			uf::stl::KeyMap<uf::stl::vector<pod::Primitive>> primitives;
			uf::stl::KeyMap<uf::stl::vector<pod::Instance>> instances;
			uf::stl::KeyMap<uf::Mesh> meshes;

			uf::stl::KeyMap<pod::ImageTexture> images;
			uf::stl::KeyMap<pod::Material> materials;
			uf::stl::KeyMap<pod::Texture> textures;
			uf::stl::KeyMap<uf::renderer::Sampler> samplers;
			uf::stl::vector<pod::Light> lights;
			uf::stl::KeyMap<pod::Skin> skins;
			uf::stl::KeyMap<pod::Animation> animations;

			// maps without direct analogues
			uf::stl::KeyMap<pod::BVH> bvhs;
			uf::stl::KeyMap<uf::Atlas> atlases;
			uf::stl::KeyMap<pod::Instance::Object> objects;
			uf::stl::KeyMap<uf::stl::vector<pod::Matrix4f>> joints;
			uf::stl::KeyMap<uf::Entity*> entities;
			
			uf::stl::vector<uf::renderer::Texture2D> shadow2Ds;
			uf::stl::vector<uf::renderer::TextureCube> shadowCubes;

			// flattened variants
			uf::stl::vector<pod::Primitive> flattenedPrimitives;

			struct Buffer {
				uf::renderer::Buffer camera;
				uf::renderer::Buffer drawCommands;
				uf::renderer::Buffer instance;
				uf::renderer::Buffer addresses;
				uf::renderer::Buffer lodMetadata;
				uf::renderer::Buffer joint;
				uf::renderer::Buffer object;
				uf::renderer::Buffer material;
				uf::renderer::Buffer texture;
				uf::renderer::Buffer light;

				uf::renderer::Texture2D depthPyramid;
			} buffers;

			bool stale = false;
			bool shouldRebind = false; 

			std::shared_ptr<std::mutex> mutex = std::make_shared<std::mutex>();
		}/* storage*/;

		pod::Graph::Storage* storage = NULL;
	};
}

namespace uf {
	namespace graph {
		extern UF_API size_t initialBufferElements;
		extern UF_API uint32_t storageMode;
		extern UF_API pod::Graph::Storage globalStorage;
	}
}

namespace uf {
	namespace graph {
		pod::Node* UF_API find( pod::Graph& graph, int32_t index );
		pod::Node* UF_API find( pod::Graph& graph, const uf::stl::string& name );

		pod::Matrix4f UF_API local( pod::Graph&, int32_t );
		pod::Matrix4f UF_API matrix( pod::Graph&, int32_t );

		pod::Graph::Storage& UF_API getStorage( pod::Graph& );
		pod::Graph::Storage& UF_API getStorage( uf::Object& );
		
		const pod::Graph::Storage& UF_API getStorage( const pod::Graph& );
		const pod::Graph::Storage& UF_API getStorage( const uf::Object& );

	//	void UF_API process( uf::Object& entity );
		void UF_API initializeGraphics( pod::Graph& graph, uf::Object& entity, uf::Mesh& mesh, uf::stl::vector<pod::Primitive>& );
		void UF_API process( pod::Graph& graph );
		void UF_API process( pod::Graph& graph, int32_t, uf::Object& parent );
		void UF_API reload( pod::Graph& );
		void UF_API initialize( pod::Graph& graph );

		void UF_API update( pod::Graph& );
		void UF_API update( pod::Graph&, float );

		void UF_API updateAnimation( pod::Graph&, float );
		void UF_API updateAnimation( pod::Graph&, pod::Node& );
		void UF_API override( pod::Graph& );
		void UF_API animate( pod::Graph&, const uf::stl::string&, float = 1, bool = true );

		uf::stl::vector<pod::Bone> collectBones( const pod::Graph& graph, const pod::Node& node );
		uf::stl::vector<pod::OBB> obbFromSkin( const pod::Graph& graph, const pod::Node& node );
		void rigRagdoll( pod::Graph& graph, pod::Node& node );
		
		void UF_API destroy( pod::Graph& );
		
		void UF_API initialize();
		void UF_API tick();
		void UF_API render();
		void UF_API destroy( bool soft = false );

		void UF_API initialize( uf::Object&, size_t = uf::graph::initialBufferElements );
		void UF_API initialize( pod::Graph::Storage&, size_t = uf::graph::initialBufferElements );
		void UF_API tick( uf::Object& );
		bool UF_API tick( pod::Graph::Storage& );
		void UF_API render( uf::Object& );
		void UF_API render( pod::Graph::Storage& );
		void UF_API destroy( uf::Object&, bool soft = false );
		void UF_API destroy( pod::Graph::Storage&, bool soft = false );

		void UF_API aggregate();
		void UF_API aggregate( uf::Object&, pod::Graph::Storage& );

		void UF_API load( pod::Graph&, const uf::stl::string&, const uf::Serializer& = ext::json::null() );
		inline pod::Graph load( const uf::stl::string& filename, const uf::Serializer& metadata = ext::json::null() ) {
			// do some deprecation warning or something because this actually is bad for doing a copy + dealloc
			pod::Graph graph;
			load( graph, filename, metadata );
			return graph;
		}

		pod::Graph& UF_API convert( uf::Object&, bool = false ); // converts an object into a graph
		void UF_API preprocess( pod::Graph&, const uf::Serializer& = ext::json::null(), const uf::stl::string& = "" ); // applies pre-processing for format importing
		void UF_API postprocess( pod::Graph& ); // applies post-processing for format importing
		void UF_API import( pod::Graph::Storage& to, pod::Graph::Storage& from, bool move = true  ); // moves storage from one to the other
		uf::stl::string UF_API save( const pod::Graph&, const uf::stl::string& ); // saves a graph to disk

		uf::stl::string UF_API print( const pod::Graph& graph );
		uf::Serializer UF_API stats( const pod::Graph& graph );
		void UF_API reload( pod::Graph& graph );
		void UF_API reload( pod::Graph& graph, pod::Node& node );
		void UF_API reload();

		// access helpers
		// to-do: the other storage values (although I don't foresee ever needing more)
		uf::stl::string UF_API getMaterialName( pod::Graph& graph, size_t id );
		pod::Material UF_API getMaterial( pod::Graph& graph, size_t id );
		pod::Primitive UF_API getPrimitive( pod::Graph& graph, size_t id );
		pod::Instance UF_API getInstance( pod::Graph& graph, size_t id );
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