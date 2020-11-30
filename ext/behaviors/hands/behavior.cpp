#include "behavior.h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/window/window.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/audio/audio.h>
#include <uf/ext/openvr/openvr.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/math/collision.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/renderer/renderer.h>

#include <sstream>

namespace {
	struct {
		uf::Object* left;
		uf::Object* right;
	} hands, lines, lights;
}

UF_BEHAVIOR_REGISTER_CPP(ext::PlayerHandBehavior)
#define this (&self)
void ext::PlayerHandBehavior::initialize( uf::Object& self ) {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();

	{
		::hands.left = (uf::Object*) &uf::instantiator::instantiate("Object");
		::hands.right = (uf::Object*) &uf::instantiator::instantiate("Object");
		
		::lines.left = (uf::Object*) &uf::instantiator::instantiate("Object");
		::lines.right = (uf::Object*) &uf::instantiator::instantiate("Object");

		this->addChild(::hands.left);
		this->addChild(::hands.right);

		::hands.left->addChild(::lines.left);
		::hands.right->addChild(::lines.right);
	}
	{
		bool loaded = true;
		for ( auto it = metadata["hands"].begin(); it != metadata["hands"].end(); ++it ) {
			std::string key = it.key();
			if ( !ext::openvr::requestRenderModel(metadata["hands"][key]["controller"]["model"].as<std::string>()) ) loaded = false;
		}
		if ( !loaded ) {
			this->addHook( "VR:Model.Loaded", [&](const std::string& event)->std::string{
				uf::Serializer json = event;

				std::string name = json["name"].as<std::string>();
				std::string side = "";
				if ( name == metadata["hands"]["left"]["controller"]["model"].as<std::string>() ) {
					side = "left";
				} else if ( name == metadata["hands"]["right"]["controller"]["model"].as<std::string>() ) {
					side = "right";
				};
				if ( side == "" ) return "false";

				uf::Object& hand = *(side == "left" ? ::hands.left : ::hands.right);
				uf::Object& line = *(side == "left" ? ::lines.left : ::lines.right);
				{
					uf::Graphic& graphic = (hand.getComponent<uf::Graphic>() = ext::openvr::getRenderModel( name ));
					
					graphic.process = true;

					graphic.descriptor.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
					graphic.material.attachShader("./data/shaders/base.stereo.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
					graphic.material.attachShader("./data/shaders/base.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

					uf::instantiator::bind( "EntityBehavior", hand );
					uf::instantiator::bind( "ObjectBehavior", hand );
					hand.initialize();
				}
				if ( metadata["hands"][side]["pointer"]["length"].as<float>() > 0 ) {
					line.addAlias<uf::LineMesh, uf::Mesh>();

					pod::Transform<>& transform = line.getComponent<pod::Transform<>>();
					transform.orientation = uf::quaternion::axisAngle( 
						{
							metadata["hands"][side]["pointer"]["orientation"]["axis"][0].as<float>(),
							metadata["hands"][side]["pointer"]["orientation"]["axis"][1].as<float>(),
							metadata["hands"][side]["pointer"]["orientation"]["axis"][2].as<float>()
						},
						metadata["hands"][side]["pointer"]["orientation"]["angle"].as<float>() * 3.14159f / 180.0f
					);
					transform.position = {
						metadata["hands"][side]["pointer"]["offset"][0].as<float>(),
						metadata["hands"][side]["pointer"]["offset"][1].as<float>(),
						metadata["hands"][side]["pointer"]["offset"][2].as<float>()
					};

					auto& mesh = line.getComponent<uf::LineMesh>();
					auto& graphic = line.getComponent<uf::Graphic>();

					mesh.vertices = {
						{ {0.0f, 0.0f, 0.0f} },
						{ {0.0f, 0.0f, metadata["hands"][side]["pointer"]["length"].as<float>()} },
					};
					graphic.initialize();
					graphic.initializeGeometry(mesh);
					
					graphic.material.attachShader("./data/shaders/line.stereo.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
					graphic.material.attachShader("./data/shaders/line.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
					graphic.descriptor.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
					graphic.descriptor.fill = VK_POLYGON_MODE_LINE;
					graphic.descriptor.lineWidth = metadata["hands"][side]["pointer"]["width"].as<float>();

					line.initialize();
				}
				if ( metadata["hands"][side]["light"]["should"].as<bool>() ){
					auto& child = hand.loadChild("/light.json", false);
					if (side == "left" )
						lights.left = &child;
					else
						lights.right = &child;

					auto& json = metadata["hands"][side]["light"];
					auto& light = side == "left" ? *lights.left : *lights.right;

					auto& metadata = light.getComponent<uf::Serializer>();
					if ( !ext::json::isNull( json["color"] ) ) metadata["light"]["color"] = json["color"];
					if ( !ext::json::isNull( json["radius"] ) ) metadata["light"]["radius"] = json["radius"];
					if ( !ext::json::isNull( json["power"] ) ) metadata["light"]["power"] = json["power"];
					if ( !ext::json::isNull( json["type"] ) ) metadata["light"]["type"] = json["type"];
					if ( !ext::json::isNull( json["shadows"] ) ) metadata["light"]["shadows"] = json["shadows"];
					metadata["lights"]["external update"] = true;

					// light.initialize();
				/*
					auto* child = (uf::Object*) hand.findByUid(hand.loadChild("/light.json", false));
					if ( child ) {
						if (side == "left" ) lights.left = child; else lights.right = child;
						auto& json = metadata["hands"][side]["light"];
						auto& light = side == "left" ? *lights.left : *lights.right;

						auto& metadata = light.getComponent<uf::Serializer>();
						if ( !ext::json::isNull( json["color"] ) ) metadata["light"]["color"] = json["color"];
						if ( !ext::json::isNull( json["radius"] ) ) metadata["light"]["radius"] = json["radius"];
						if ( !ext::json::isNull( json["power"] ) ) metadata["light"]["power"] = json["power"];
						if ( !ext::json::isNull( json["shadows"] ) ) metadata["light"]["shadows"] = json["shadows"];
						metadata["lights"]["external update"] = true;

						light.initialize();
					}
				*/
				}

				return "true";
			});
		}
		std::vector<uf::Object*> vHands = { ::hands.left, ::hands.right };
		for ( auto pointer : vHands ) {
			auto& hand = *pointer;
			hand.addHook("VR:Input.Digital", [&](const std::string& event)->std::string{
				uf::Serializer json = event;
				
				std::string side = &hand == hands.left ? "left" : "right";
				if ( json["hand"].as<std::string>() != side ) return "false";

				// fire mouse click
				if ( json["name"].as<std::string>() == "click" ) {

					uf::Serializer payload;
					payload["type"] = "window:Mouse.Click";
					payload["invoker"] = "vr";
					payload["mouse"]["position"]["x"]	= metadata["hands"][side]["cursor"]["position"][0].as<float>();
					payload["mouse"]["position"]["y"]	= metadata["hands"][side]["cursor"]["position"][1].as<float>();
					payload["mouse"]["delta"]["x"] 		= 0;
					payload["mouse"]["delta"]["y"] 		= 0;
					payload["mouse"]["button"] 			= side == "left" ? "Right" : "Left";
					payload["mouse"]["state"] = json["state"].as<bool>() ? "Down": "Up";

					uf::hooks.call( payload["type"].as<std::string>(), payload );
				}

				return "true";
			});
			hand.addHook("world:Collision.%UID%", [&](const std::string& event)->std::string{
				uf::Serializer json = event;
	
				std::string side = &hand == hands.left ? "left" : "right";

				float mag = json["depth"].as<float>();
				uf::Serializer payload;
				payload["delay"] = 0.0f;
				payload["duration"] = uf::physics::time::delta;
				payload["frequency"] = 1.0f;
				payload["amplitude"] = fmin(1.0f, 1000.0f * mag);
				payload["side"] = side;
				uf::hooks.call( "VR:Haptics." + side, payload );

				return "true";
			});

			auto& transform = hand.getComponent<pod::Transform<>>();
			auto& collider = hand.getComponent<uf::Collider>();

		//	auto* box = new uf::BoundingBox( transform.position, {0.25, 0.25, 0.25} );
		//	box->getTransform().reference = &transform;
		//	collider.add(box);
		}
	}
}
void ext::PlayerHandBehavior::tick( uf::Object& self ) {
	auto& scene = uf::scene::getCurrentScene();
	auto& controller = scene.getController();
	auto& controllerCamera = controller.getComponent<uf::Camera>();
	auto& controllerTransform = controller.getComponent<pod::Transform<>>();
	auto& controllerCameraTransform = controllerCamera.getTransform();
	{
		pod::Transform<>& transform = ::hands.left->getComponent<pod::Transform<>>();
		transform.position = ext::openvr::controllerPosition( vr::Controller_Hand::Hand_Left );
		transform.orientation = ext::openvr::controllerQuaternion( vr::Controller_Hand::Hand_Left );
		transform.scale = { 1, 1, 1 };
		transform.reference = &controllerTransform;

		auto& collider = ::hands.left->getComponent<uf::Collider>();
		for ( auto* box : collider.getContainer() ) {
			box->getTransform().position = transform.position + controllerTransform.position + controllerCameraTransform.position;
		}
	}
	{
		pod::Transform<>& transform = ::hands.right->getComponent<pod::Transform<>>();
		transform.position = ext::openvr::controllerPosition( vr::Controller_Hand::Hand_Right );
		transform.orientation = ext::openvr::controllerQuaternion( vr::Controller_Hand::Hand_Right );
		transform.scale = { 1, 1, 1 };
		transform.reference = &controllerTransform;

		auto& collider = ::hands.right->getComponent<uf::Collider>();
		for ( auto* box : collider.getContainer() ) {
			box->getTransform().position = transform.position + controllerTransform.position + controllerCameraTransform.position;
		}
	}
	{
		pod::Transform<>& transform = ::lines.left->getComponent<pod::Transform<>>();
		transform.position = ext::openvr::controllerPosition( vr::Controller_Hand::Hand_Left, true );
		transform.orientation = ext::openvr::controllerQuaternion( vr::Controller_Hand::Hand_Left, true );
		transform.scale = { 1, 1, 1 };
		transform.reference = &controllerTransform;

		if ( lights.left ) {
			if ( lights.left->getUid() == 0 ) {
				lights.left->initialize();
			}
			auto& light = *lights.left;
			auto& lightTransform = light.getComponent<pod::Transform<>>();
			lightTransform.position = controllerCameraTransform.position + controller.getComponent<pod::Transform<>>().position + transform.position;

			auto& lightCamera = light.getComponent<uf::Camera>();
			pod::Matrix4f playerModel = uf::matrix::identity();
			pod::Matrix4f translation = uf::matrix::translate( uf::matrix::identity(), controllerCameraTransform.position + controllerTransform.position );
			pod::Matrix4f rotation = uf::quaternion::matrix( controllerTransform.orientation );
			playerModel = translation * rotation;

			pod::Matrix4f model = uf::matrix::inverse( playerModel * ext::openvr::controllerModelMatrix( vr::Controller_Hand::Hand_Left, true ) );
			for ( size_t i = 0; i < 2; ++i ) lightCamera.setView( model, i );
		}
	}
	{
		pod::Transform<>& transform = ::lines.right->getComponent<pod::Transform<>>();
		transform.position = ext::openvr::controllerPosition( vr::Controller_Hand::Hand_Right, true );
		transform.orientation = ext::openvr::controllerQuaternion( vr::Controller_Hand::Hand_Right, true );
		transform.scale = { 1, 1, 1 };
		transform.reference = &controllerTransform;

		if ( lights.right ) {
			if ( lights.right->getUid() == 0 ) {
				lights.right->initialize();
			}
			auto& light = *lights.right;
			auto& lightTransform = light.getComponent<pod::Transform<>>();
			lightTransform.position = controllerCameraTransform.position + controllerTransform.position + transform.position;

			auto& lightCamera = light.getComponent<uf::Camera>();
			pod::Matrix4f playerModel = uf::matrix::identity();
			pod::Matrix4f translation = uf::matrix::translate( uf::matrix::identity(), controllerCameraTransform.position + controllerTransform.position );
			pod::Matrix4f rotation = uf::quaternion::matrix( controllerTransform.orientation );
			playerModel = translation * rotation;

			pod::Matrix4f model = uf::matrix::inverse( playerModel * ext::openvr::controllerModelMatrix( vr::Controller_Hand::Hand_Right, true ) );
			for ( size_t i = 0; i < 2; ++i ) lightCamera.setView( model, i );
		}
	}

	// raytrace pointer / hand collision
	{
		std::vector<uf::Object*> handPointers = { ::hands.left, ::hands.right };
		for ( auto pointer : handPointers ) { auto& hand = *pointer;
			std::string side = &hand == ::hands.left ? "left" : "right";
			if ( !ext::openvr::controllerActive( side == "left" ? vr::Controller_Hand::Hand_Left : vr::Controller_Hand::Hand_Right ) ) continue;
			{
				pod::Transform<>& transform = (side == "left" ? ::lines.left : ::lines.right)->getComponent<pod::Transform<>>();
				struct {
					pod::Vector3f origin;
					pod::Vector3f direction;
				} ray;
				struct {
					pod::Vector3f center;
					pod::Vector3f normal;
				} plane;

				transform = uf::transform::reorient( transform );
				ray.origin = transform.position;
				ray.direction = transform.forward;

				pod::Transform<> gtransform;
				pod::Matrix4f mvp;
				uf::Serializer& cMetadata = controller.getComponent<uf::Serializer>();
				if ( ext::json::isArray( cMetadata["overlay"]["position"] ) ) 
					gtransform.position = {
						cMetadata["overlay"]["position"][0].as<float>(),
						cMetadata["overlay"]["position"][1].as<float>(),
						cMetadata["overlay"]["position"][2].as<float>(),
					};
				if ( ext::json::isArray( cMetadata["overlay"]["scale"] ) ) 
					gtransform.scale = {
						cMetadata["overlay"]["scale"][0].as<float>(),
						cMetadata["overlay"]["scale"][1].as<float>(),
						cMetadata["overlay"]["scale"][2].as<float>(),
					};
				if ( ext::json::isArray( cMetadata["overlay"]["orientation"] ) ) 
					gtransform.orientation = {
						cMetadata["overlay"]["orientation"][0].as<float>(),
						cMetadata["overlay"]["orientation"][1].as<float>(),
						cMetadata["overlay"]["orientation"][2].as<float>(),
						cMetadata["overlay"]["orientation"][3].as<float>(),
					};

				plane.center = gtransform.position;
				{
					auto rotated = uf::quaternion::multiply( gtransform.orientation, pod::Vector4f{ 0, 0, 1, 1 } );
					plane.normal.x = rotated.x;
					plane.normal.y = rotated.y;
					plane.normal.z = rotated.z;
					plane.normal = uf::vector::normalize( plane.normal );
				}

				float denom = uf::vector::dot(plane.normal, ray.direction);
				if (abs(denom) > 0.0001f) {
					float t = uf::vector::dot( uf::vector::subtract(plane.center, ray.origin), plane.normal ) / denom;
					if ( t >= 0 ) {
						pod::Vector3f hit = ray.origin + (ray.direction * t);
						pod::Vector3f translated = uf::matrix::multiply<float>( uf::matrix::inverse( uf::matrix::scale( uf::matrix::identity(), gtransform.scale ) ), uf::vector::subtract( plane.center, hit ) );
						{
							auto& metadata = this->getComponent<uf::Serializer>();
							cMetadata["overlay"]["cursor"]["type"] = "vr";
							cMetadata["overlay"]["cursor"]["position"][0] =  translated.x;
							cMetadata["overlay"]["cursor"]["position"][1] =  translated.y;
							cMetadata["overlay"]["cursor"]["position"][2] =  translated.z;

							metadata["hands"][side]["cursor"] = cMetadata["overlay"]["cursor"];
						}
					}
				}
			}
		}
	}
}

void ext::PlayerHandBehavior::render( uf::Object& self ){
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	auto& scene = uf::scene::getCurrentScene();
	auto& controller = scene.getController();
	auto& camera = controller.getComponent<uf::Camera>();

	pod::Matrix4f playerModel = uf::matrix::identity(); {
		auto& controller = this->getParent();
		auto& camera = controller.getComponent<uf::Camera>();
		pod::Matrix4f translation = uf::matrix::translate( uf::matrix::identity(), camera.getTransform().position + controller.getComponent<pod::Transform<>>().position );
		pod::Matrix4f rotation = uf::quaternion::matrix( controller.getComponent<pod::Transform<>>().orientation );
		playerModel = translation * rotation;
	}
//	pod::Matrix4f cameraModel = uf::matrix::translate( uf::matrix::identity(), camera.getTransform().position + controller.getComponent<pod::Transform<>>().position ) * uf::quaternion::matrix( controller.getComponent<pod::Transform<>>().orientation );	
	if ( ::hands.left && ::hands.left->hasComponent<uf::Graphic>() ) {
		auto& graphic = ::hands.left->getComponent<uf::Graphic>();
		auto& transform = ::hands.left->getComponent<pod::Transform<>>();
		graphic.process = ext::openvr::controllerActive( vr::Controller_Hand::Hand_Left );
		pod::Matrix4f model = playerModel * ext::openvr::controllerModelMatrix( vr::Controller_Hand::Hand_Left, false );
		if ( graphic.initialized ) {	
			// auto& uniforms = graphic.uniforms<uf::StereoMeshDescriptor>();
			auto& uniforms = graphic.material.shaders.front().uniforms.front().get<uf::StereoMeshDescriptor>();
			uniforms.matrices.model = model;
			for ( std::size_t i = 0; i < 2; ++i ) {
				uniforms.matrices.view[i] = camera.getView( i );
				uniforms.matrices.projection[i] = camera.getProjection( i );
			}
			uniforms.color[0] = metadata["hands"]["left"]["controller"]["color"][0].as<float>();
			uniforms.color[1] = metadata["hands"]["left"]["controller"]["color"][1].as<float>();
			uniforms.color[2] = metadata["hands"]["left"]["controller"]["color"][2].as<float>();
			uniforms.color[3] = metadata["hands"]["left"]["controller"]["color"][3].as<float>();
			// graphic.updateBuffer( uniforms, 0, false );

			if ( uf::renderer::currentRenderMode ) {
				auto& renderMode = *uf::renderer::currentRenderMode;
				if ( renderMode.name == "" )
					uniforms.matrices.model = pod::Matrix4f{};
			} else {
				uniforms.matrices.model = pod::Matrix4f{};
			}

			graphic.material.shaders.front().updateBuffer( uniforms, 0, false );
		}
	}
	if ( ::hands.right && ::hands.right->hasComponent<uf::Graphic>() ) {
		auto& graphic = ::hands.right->getComponent<uf::Graphic>();
		auto& transform = ::hands.right->getComponent<pod::Transform<>>();
		graphic.process = ext::openvr::controllerActive( vr::Controller_Hand::Hand_Right );
		pod::Matrix4f model = playerModel * ext::openvr::controllerModelMatrix( vr::Controller_Hand::Hand_Right, false );
		if ( graphic.initialized ) {	
			// auto& uniforms = graphic.uniforms<uf::StereoMeshDescriptor>();
			auto& uniforms = graphic.material.shaders.front().uniforms.front().get<uf::StereoMeshDescriptor>();
			uniforms.matrices.model = model;
			for ( std::size_t i = 0; i < 2; ++i ) {
				uniforms.matrices.view[i] = camera.getView( i );
				uniforms.matrices.projection[i] = camera.getProjection( i );
			}
			uniforms.color[0] = metadata["hands"]["right"]["controller"]["color"][0].as<float>();
			uniforms.color[1] = metadata["hands"]["right"]["controller"]["color"][1].as<float>();
			uniforms.color[2] = metadata["hands"]["right"]["controller"]["color"][2].as<float>();
			uniforms.color[3] = metadata["hands"]["right"]["controller"]["color"][3].as<float>();
			// graphic.updateBuffer( uniforms, 0, false );

			if ( uf::renderer::currentRenderMode ) {
				auto& renderMode = *uf::renderer::currentRenderMode;
				if ( renderMode.name == "" )
					uniforms.matrices.model = pod::Matrix4f{};
			} else {
				uniforms.matrices.model = pod::Matrix4f{};
			}

			graphic.material.shaders.front().updateBuffer( uniforms, 0, false );
		}
	}
	if ( ::lines.left && ::lines.left->hasComponent<uf::Graphic>() ) {
		auto& graphic = ::lines.left->getComponent<uf::Graphic>();
		auto& transform = ::lines.left->getComponent<pod::Transform<>>();
		graphic.process = ext::openvr::controllerActive( vr::Controller_Hand::Hand_Left );
		pod::Matrix4f model = playerModel * ext::openvr::controllerModelMatrix( vr::Controller_Hand::Hand_Left, true );
		if ( graphic.initialized ) {	
			// auto& uniforms = graphic.uniforms<uf::StereoMeshDescriptor>();
			auto& uniforms = graphic.material.shaders.front().uniforms.front().get<uf::StereoMeshDescriptor>();
			uniforms.matrices.model = model;
			for ( std::size_t i = 0; i < 2; ++i ) {
				uniforms.matrices.view[i] = camera.getView( i );
				uniforms.matrices.projection[i] = camera.getProjection( i );
			}
			uniforms.color[0] = metadata["hands"]["left"]["pointer"]["color"][0].as<float>();
			uniforms.color[1] = metadata["hands"]["left"]["pointer"]["color"][1].as<float>();
			uniforms.color[2] = metadata["hands"]["left"]["pointer"]["color"][2].as<float>();
			uniforms.color[3] = metadata["hands"]["left"]["pointer"]["color"][3].as<float>();
			// graphic.updateBuffer( uniforms, 0, false );
			graphic.material.shaders.front().updateBuffer( uniforms, 0, false );
		}
	}
	if ( ::lines.right && ::lines.right->hasComponent<uf::Graphic>() ) {
		auto& graphic = ::lines.right->getComponent<uf::Graphic>();
		auto& transform = ::lines.right->getComponent<pod::Transform<>>();
		graphic.process = ext::openvr::controllerActive( vr::Controller_Hand::Hand_Right );
		pod::Matrix4f model = playerModel * ext::openvr::controllerModelMatrix( vr::Controller_Hand::Hand_Right, true );
		if ( graphic.initialized ) {	
			// auto& uniforms = graphic.uniforms<uf::StereoMeshDescriptor>();
			auto& uniforms = graphic.material.shaders.front().uniforms.front().get<uf::StereoMeshDescriptor>();
			uniforms.matrices.model = model;
			for ( std::size_t i = 0; i < 2; ++i ) {
				uniforms.matrices.view[i] = camera.getView( i );
				uniforms.matrices.projection[i] = camera.getProjection( i );
			}
			uniforms.color[0] = metadata["hands"]["right"]["pointer"]["color"][0].as<float>();
			uniforms.color[1] = metadata["hands"]["right"]["pointer"]["color"][1].as<float>();
			uniforms.color[2] = metadata["hands"]["right"]["pointer"]["color"][2].as<float>();
			uniforms.color[3] = metadata["hands"]["right"]["pointer"]["color"][3].as<float>();
			// graphic.updateBuffer( uniforms, 0, false );
			graphic.material.shaders.front().updateBuffer( uniforms, 0, false );
		}
	}
}
void ext::PlayerHandBehavior::destroy( uf::Object& self ){

}
#undef this