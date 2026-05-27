#include <uf/engine/graph/graph.h>
#include <uf/ext/gltf/gltf.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/math/physics/constraints.h>
#include <uf/utils/math/physics/common.h>
#include <uf/utils/mesh/grid.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/string/base64.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/memory/map.h>
#include <uf/ext/xatlas/xatlas.h>
#include <uf/ext/ffx/fsr.h>

#include <uf/engine/ext.h>

namespace {
	pod::Graph::Storage& getGraphStorage( uf::Object& object ) {
		return uf::graph::globalStorage ? uf::graph::storage : object.getComponent<pod::Graph::Storage>();
	}

	// lazy load animations if requested
	void loadAnimation( const uf::stl::string& name ) {
		auto& scene = uf::scene::getCurrentScene();
		auto& storage = ::getGraphStorage( scene );
		
		auto& animation = storage.animations.map[name];

		//UF_ASSERT( animation.path != "" );
		if ( animation.path == "" ) {
			return;
		}

		uf::Serializer json;
		json.readFromFile( animation.path );

		animation.name = json["name"].as(animation.name);
		animation.start = json["start"].as(animation.start);
		animation.end = json["end"].as(animation.end);

		if ( animation.samplers.empty() ) ext::json::forEach( json["samplers"], [&]( ext::json::Value& value ){
			auto& sampler = animation.samplers.emplace_back();
			sampler.interpolator = value["interpolator"].as(sampler.interpolator);

			sampler.inputs.reserve( value["inputs"].size() );
			ext::json::forEach( value["inputs"], [&]( ext::json::Value& input ){
				sampler.inputs.emplace_back( input.as<float>() );
			});
			
			sampler.outputs.reserve( value["outputs"].size() );
			ext::json::forEach( value["outputs"], [&]( ext::json::Value& output ){
				sampler.outputs.emplace_back( uf::vector::decode( output, pod::Vector4f{} ) );
			});
		});

		if ( animation.channels.empty() ) ext::json::forEach( json["channels"], [&]( ext::json::Value& value ){
			auto& channel = animation.channels.emplace_back();
			channel.path = value["path"].as(channel.path);
			channel.node = value["node"].as(channel.node);
			channel.sampler = value["sampler"].as(channel.sampler);
		});
	}

	void unloadAnimation( const uf::stl::string& name ) {
		auto& scene = uf::scene::getCurrentScene();
		auto& storage = ::getGraphStorage( scene );
		
		auto& animation = storage.animations.map[name];

		animation.samplers.clear();
		animation.channels.clear();
	#if UF_ENV_DREAMCAST
		animation.samplers.shrink_to_fit();
		animation.channels.shrink_to_fit();
	#endif
	}

	pod::Matrix4f localMatrix( pod::Graph& graph, int32_t index ) {
		auto& node = 0 < index && index <= graph.nodes.size() ? graph.nodes[index] : graph.root;
		return
			uf::matrix::translate( uf::matrix::identity(), node.transform.position ) *
			uf::quaternion::matrix(node.transform.orientation) *
			uf::matrix::scale( uf::matrix::identity(), node.transform.scale ) *
			node.transform.model;
	}
	pod::Matrix4f worldMatrix( pod::Graph& graph, int32_t index ) {
		pod::Matrix4f matrix = ::localMatrix( graph, index );
		auto& node = *uf::graph::find( graph, index );
		int32_t parent = node.parent;
		while ( 0 < parent && parent <= graph.nodes.size() ) {
			matrix = ::localMatrix( graph, parent ) * matrix;
			parent = graph.nodes[parent].parent;
		}
		return matrix;
	}
}

pod::Node* uf::graph::find( pod::Graph& graph, int32_t index ) {
	return 0 <= index && index < graph.nodes.size() ? &graph.nodes[index] : NULL;
}
pod::Node* uf::graph::find( pod::Graph& graph, const uf::stl::string& name ) {
	for ( auto& node : graph.nodes ) if ( node.name == name ) return &node;
	return NULL;
}

void uf::graph::override( pod::Graph& graph ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& storage = ::getGraphStorage( scene );

	graph.settings.animations.override.a = 0;
	graph.settings.animations.override.map.clear();
	bool toNeutralPose = graph.sequence.empty();
	// store every node's current transform
	for ( auto& node : graph.nodes ) {
		graph.settings.animations.override.map[node.index].first = node.transform;
		graph.settings.animations.override.map[node.index].second = node.transform;
		if ( toNeutralPose ) {
			graph.settings.animations.override.map[node.index].second.position = { 0, 0, 0 };
			graph.settings.animations.override.map[node.index].second.orientation = { 0, 0, 0, 1 };
			graph.settings.animations.override.map[node.index].second.scale = { 1, 1, 1 };
		}
	}
	// set our destination transform per node
	if ( !toNeutralPose ) {
		uf::stl::string name = graph.sequence.front();
		pod::Animation& animation = storage.animations.map[name];

		// load animation data
		if ( animation.channels.empty() || animation.samplers.empty() ) ::loadAnimation( name );

		for ( auto& channel : animation.channels ) {
			auto& override = graph.settings.animations.override.map[channel.node];
			auto& sampler = animation.samplers[channel.sampler];
			if ( sampler.interpolator != "LINEAR" ) continue;
			for ( size_t i = 0; i < sampler.inputs.size() - 1; ++i ) {
				if ( !(animation.start >= sampler.inputs[i] && animation.start <= sampler.inputs[i+1]) ) continue;
				if ( channel.path == "translation" ) {
					override.second.position = sampler.outputs[i];
				} else if ( channel.path == "rotation" ) {
					override.second.orientation = uf::quaternion::normalize( sampler.outputs[i] );
				} else if ( channel.path == "scale" ) {
					override.second.scale = sampler.outputs[i];
				}
			}
		}
	}
}

void uf::graph::animate( pod::Graph& graph, const uf::stl::string& _name, float speed, bool immediate ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& storage = ::getGraphStorage( scene );

	if ( !(graph.metadata["renderer"]["skinned"].as<bool>()) ) return;
	const uf::stl::string name = _name;
	if ( storage.animations.map.count( name ) > 0 ) {
		// if already playing, ignore it
		if ( !graph.sequence.empty() && graph.sequence.front() == name ) return;
		if ( immediate ) {
			while ( !graph.sequence.empty() ) {
				// unload
				if ( graph.settings.stream.animations ) ::unloadAnimation( graph.sequence.front() );
				graph.sequence.pop();
			}
		}
		bool empty = graph.sequence.empty();
		graph.sequence.emplace(name);
		if ( empty ) uf::graph::override( graph );
		graph.settings.animations.speed = speed;
	}
	updateAnimation( graph, 0 );
}

void uf::graph::updateAnimation( pod::Graph& graph, float delta ) {
	// update our instances

	// no skins
	if ( !(graph.metadata["renderer"]["skinned"].as<bool>()) ) {
		return;
	}

	if ( graph.sequence.empty() ) goto UPDATE;
	if ( graph.settings.animations.override.a >= 0 ) goto OVERRIDE;
	{
		uf::stl::string name = graph.sequence.front();
		pod::Animation* animation = &storage.animations.map[name]; // &graph.animations[name];
		animation->cur += delta * graph.settings.animations.speed; // * graph.settings.animations.override.speed;
		if ( animation->end < animation->cur ) {
			animation->cur = graph.settings.animations.loop ? animation->cur - animation->end : 0;
			// go-to next animation
			if ( !graph.settings.animations.loop ) {
				// unload
				if ( graph.settings.stream.animations ) ::unloadAnimation( graph.sequence.front() );
				graph.sequence.pop();

				// out of animations, set to neutral pose
				if ( graph.sequence.empty() ) {
					uf::graph::override( graph );
					goto OVERRIDE;
				}
				name = graph.sequence.front();
				animation = &storage.animations.map[name]; // &graph.animations[name];
			}
		}

		// load animation data
		if ( animation->channels.empty() || animation->samplers.empty() ) ::loadAnimation( name );

		for ( auto& channel : animation->channels ) {
			auto& sampler = animation->samplers[channel.sampler];
			if ( sampler.interpolator != "LINEAR" ) continue;
			for ( size_t i = 0; i < sampler.inputs.size() - 1; ++i ) {
				if ( !(animation->cur >= sampler.inputs[i] && animation->cur <= sampler.inputs[i+1]) ) continue;
				float a = (animation->cur - sampler.inputs[i]) / (sampler.inputs[i+1] - sampler.inputs[i]);
				auto& transform = graph.nodes[channel.node].transform;
				if ( channel.path == "translation" ) {
					transform.position = uf::vector::mix( sampler.outputs[i], sampler.outputs[i+1], a );
				} else if ( channel.path == "rotation" ) {
					transform.orientation = uf::quaternion::normalize( uf::quaternion::slerp(sampler.outputs[i], sampler.outputs[i+1], a) );
				} else if ( channel.path == "scale" ) {
					transform.scale = uf::vector::mix( sampler.outputs[i], sampler.outputs[i+1], a );
				}
			}
		}
		goto UPDATE;
	}
OVERRIDE:
	for ( auto pair : graph.settings.animations.override.map ) {
		graph.nodes[pair.first].transform.position = uf::vector::mix( pair.second.first.position, pair.second.second.position, graph.settings.animations.override.a );
		graph.nodes[pair.first].transform.orientation = uf::quaternion::normalize( uf::quaternion::slerp(pair.second.first.orientation, pair.second.second.orientation, graph.settings.animations.override.a) );
		graph.nodes[pair.first].transform.scale = uf::vector::mix( pair.second.first.scale, pair.second.second.scale, graph.settings.animations.override.a );
	}
	// finished our overrided interpolation, clear it
	if ( (graph.settings.animations.override.a += delta * graph.settings.animations.override.speed) >= 1 ) {
		graph.settings.animations.override.a = -std::numeric_limits<float>::max();
		graph.settings.animations.override.map.clear();
	}
UPDATE:
	for ( auto& node : graph.nodes ) uf::graph::updateAnimation( graph, node );
}
void uf::graph::updateAnimation( pod::Graph& graph, pod::Node& node ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& storage = ::getGraphStorage( scene );

	if ( 0 <= node.skin && node.skin < graph.skins.size() ) {
		pod::Matrix4f nodeMatrix = ::worldMatrix( graph, node.index );
		pod::Matrix4f inverseTransform = uf::matrix::inverse( nodeMatrix );

		auto& name = graph.skins[node.skin];
		auto& skin = storage.skins[name];
		auto& joints = storage.joints[name];
		joints.resize( skin.joints.size() );
		for ( size_t i = 0; i < skin.joints.size(); ++i ) joints[i] = uf::matrix::identity();

		if ( graph.settings.animations.override.a >= 0 || !graph.sequence.empty() ) {
			for ( size_t i = 0; i < skin.joints.size(); ++i ) {
				joints[i] = inverseTransform * (::worldMatrix(graph, skin.joints[i]) * skin.inverseBindMatrices[i]);
			}
		}
	}
}

// separate function in the event something later might need it
uf::stl::vector<pod::OBB> uf::graph::obbFromSkin( const pod::Graph& graph, const pod::Node& node ) {
	const float wThresold = 0.15f;

	auto& storage = ::getGraphStorage( uf::scene::getCurrentScene() );
	auto& meshName = graph.meshes[node.mesh];
	auto& skinName = graph.skins[node.skin];
	
	auto& skin = storage.skins[skinName];
	auto& mesh = storage.meshes[meshName];

	// store as min/max AABB
	uf::stl::vector<pod::OBB> bounds(skin.joints.size(), {
		pod::Vector3f{ FLT_MAX, FLT_MAX, FLT_MAX },
		pod::Vector3f{-FLT_MAX,-FLT_MAX,-FLT_MAX }
	});

	// iterate through mesh to fetch attributes
	for ( const auto& view : mesh.buffer_views ) {
		auto posView	= view["position"];
		auto jointsView = view["joints"];
		auto weightView = view["weights"];

		for ( auto i = 0; i < view.vertex.count; ++i ) {
			auto pos = uf::mesh::fetchVertex( view, posView, i );
			auto joints = uf::mesh::fetchVertexAttribute<pod::Vector4us>( view, jointsView, i );
			auto weights = uf::mesh::fetchVertexAttribute<pod::Vector4f>( view, weightView, i );

			for ( auto w = 0; w < 4; ++w ) {
				if ( weights[w] <= wThresold ) continue;
				uint16_t boneID = joints[w];
				pod::Vector3f localPos = uf::matrix::multiply( skin.inverseBindMatrices[boneID], pos );

				bounds[boneID].center = uf::vector::min( bounds[boneID].center, localPos );
				bounds[boneID].extent = uf::vector::max( bounds[boneID].extent, localPos );
			}
		}
	}

	// convert from min-max to center-extent
	for ( auto& box : bounds ) {
		auto extent = (box.extent - box.center) * 0.5f;
		auto center = (box.extent + box.center) * 0.5f;
		box = pod::OBB{ center, extent };
	}

	return bounds;
}

void uf::graph::rigRagdoll( pod::Graph& graph, pod::Node& node ) {
	// scrapped for now
}