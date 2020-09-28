#include <uf/engine/object/object.h>
#include <uf/engine/asset/asset.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/time/time.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/ext/gltf/gltf.h>

UF_BEHAVIOR_ENTITY_CPP_BEGIN(Object)
#define this (&self)
void uf::ObjectBehavior::initialize( uf::Object& self ) {	
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	if ( metadata["system"]["type"].isNull() || metadata["system"]["defaults"]["asset load"].asBool()  ) {
		// Default load: GLTF model
		this->addHook( "asset:Load.%UID%", [&](const std::string& event)->std::string{	
			uf::Serializer json = event;
			std::string filename = json["filename"].asString();

			if ( uf::string::extension(filename) != "glb" ) return "false";
			int8_t LOAD_FLAGS = 0;
			if ( metadata["model"]["flags"]["GENERATE_NORMALS"].asBool() )
				LOAD_FLAGS |= ext::gltf::LoadMode::GENERATE_NORMALS; 	// 0x1 << 0;
			if ( metadata["model"]["flags"]["APPLY_TRANSFORMS"].asBool() )
				LOAD_FLAGS |= ext::gltf::LoadMode::APPLY_TRANSFORMS; 	// 0x1 << 1;
			if ( metadata["model"]["flags"]["SEPARATE_MESHES"].asBool() )
				LOAD_FLAGS |= ext::gltf::LoadMode::SEPARATE_MESHES; 	// 0x1 << 2;
			if ( metadata["model"]["flags"]["RENDER"].asBool() )
				LOAD_FLAGS |= ext::gltf::LoadMode::RENDER; 				// 0x1 << 3;
			if ( metadata["model"]["flags"]["COLLISION"].asBool() )
				LOAD_FLAGS |= ext::gltf::LoadMode::COLLISION; 			// 0x1 << 4;
			if ( metadata["model"]["flags"]["AABB"].asBool() )
				LOAD_FLAGS |= ext::gltf::LoadMode::AABB; 				// 0x1 << 5;

			ext::gltf::load( *this, filename, LOAD_FLAGS );
			return "true";
		});
	}
}
void uf::ObjectBehavior::destroy( uf::Object& self ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	for( Json::Value::iterator it = metadata["system"]["hooks"]["alloc"].begin() ; it != metadata["system"]["hooks"]["alloc"].end() ; ++it ) {
	 	std::string name = it.key().asString();
		for ( size_t i = 0; i < metadata["system"]["hooks"]["alloc"][name].size(); ++i ) {
			size_t id = metadata["system"]["hooks"]["alloc"][name][(int) i].asUInt();
			uf::hooks.removeHook(name, id);
		}
	}
}
void uf::ObjectBehavior::tick( uf::Object& self ) {
	// listen for metadata file changes
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	if ( metadata["system"]["hot reload"]["enabled"].asBool() ) {
		size_t mtime = uf::string::mtime( metadata["system"]["source"].asString() );
		if ( metadata["system"]["hot reload"]["mtime"].asUInt64() < mtime ) {
			std::cout << metadata["system"]["hot reload"]["mtime"] << ": " << mtime << std::endl;
			metadata["system"]["hot reload"]["mtime"] = mtime;
			this->reload();
			//this->queueHook("metadata:Reload.%UID%");
		}
	}

	// Call queued hooks
	{
		if ( !uf::Object::timer.running() ) uf::Object::timer.start();
		float curTime = uf::Object::timer.elapsed().asDouble();
		uf::Serializer newQueue = Json::Value(Json::arrayValue);
		for ( auto& member : metadata["system"]["hooks"]["queue"] ) {
			uf::Serializer payload = member["payload"];
			std::string name = member["name"].asString();
			float timeout = member["timeout"].asFloat();
			if ( timeout < curTime ) {
				this->callHook( name, payload );
			} else {
				newQueue.append(member);
			}
		}
		if ( metadata.isObject() ) metadata["system"]["hooks"]["queue"] = newQueue;
	}
}
void uf::ObjectBehavior::render( uf::Object& self ) {
	auto& metadata = this->getComponent<uf::Serializer>();
	if ( metadata["system"]["type"].isNull() || metadata["system"]["defaults"]["render"].asBool() ) {
		/* Update uniforms */ if ( this->hasComponent<uf::Graphic>() ) {
			auto& scene = uf::scene::getCurrentScene();
			auto& graphic = this->getComponent<uf::Graphic>();
			auto& transform = this->getComponent<pod::Transform<>>();
			auto& controller = scene.getController();
			auto& camera = controller.getComponent<uf::Camera>();		
			
			if ( !graphic.initialized ) return;

			auto& uniforms = graphic.material.shaders.front().uniforms.front().get<uf::StereoMeshDescriptor>();
			uniforms.matrices.model = uf::transform::model( transform );
			for ( std::size_t i = 0; i < 2; ++i ) {
				uniforms.matrices.view[i] = camera.getView( i );
				uniforms.matrices.projection[i] = camera.getProjection( i );
			}

			uniforms.color[0] = 1;
			uniforms.color[1] = 1;
			uniforms.color[2] = 1;
			uniforms.color[3] = 1;

			graphic.material.shaders.front().updateBuffer( uniforms, 0, false );
		};
	}
}
#undef this
UF_BEHAVIOR_ENTITY_CPP_END(Entity)