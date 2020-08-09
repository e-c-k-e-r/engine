#include "hands.h"

#include <uf/utils/mesh/mesh.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/ext/openvr/openvr.h>

namespace {
	struct {
		uf::Object left, right;
	} hands;


	struct {
		uf::Object left, right;
	} lines;
}

EXT_OBJECT_REGISTER_CPP(Hands)
void ext::Hands::initialize() {
	uf::Object::initialize();
	
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();

	{
		this->addChild(::hands.left);
		this->addChild(::hands.right);

		this->addChild(::lines.left);
		this->addChild(::lines.right);
	}
	{
		bool loaded = true;
		for ( auto it = metadata["hands"].begin(); it != metadata["hands"].end(); ++it ) {
			std::string key = it.key().asString();
			if ( !ext::openvr::requestRenderModel(metadata["hands"][key]["controller"]["model"].asString()) ) loaded = false;
		}
		if ( !loaded ) {
			this->addHook( "VR:Model.Loaded", [&](const std::string& event)->std::string{
				uf::Serializer json = event;

				std::string name = json["name"].asString();
				uf::Object* pointer = NULL;
				if ( name == metadata["hands"]["left"]["controller"]["model"].asString() ) pointer = &hands.left;
				else if ( name == metadata["hands"]["right"]["controller"]["model"].asString() ) pointer = &hands.right;
				else return "false";

				{
					uf::Object& hand = *pointer;
					uf::Mesh& mesh = (hand.getComponent<uf::Mesh>() = ext::openvr::getRenderModel( name ));
					mesh.graphic.initializeShaders({
						{"./data/shaders/base.stereo.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
						{"./data/shaders/base.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT}
					});
					mesh.graphic.description.rasterMode.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
					mesh.generate();
					mesh.graphic.bindUniform<uf::StereoMeshDescriptor>();
					mesh.graphic.initialize();
					mesh.graphic.autoAssign();

					hand.initialize();
				}
				{
					std::string hand = pointer == &hands.left ? "left" : "right";
					auto& line = pointer == &hands.left ? lines.left : lines.right;
					line.addAlias<uf::LineMesh, uf::Mesh>();

					pod::Transform<>& transform = line.getComponent<pod::Transform<>>();
					transform.orientation = uf::quaternion::axisAngle( 
						{
							metadata["hands"][hand]["pointer"]["orientation"]["axis"][0].asFloat(),
							metadata["hands"][hand]["pointer"]["orientation"]["axis"][1].asFloat(),
							metadata["hands"][hand]["pointer"]["orientation"]["axis"][2].asFloat()
						},
						metadata["hands"][hand]["pointer"]["orientation"]["angle"].asFloat() * 3.14159f / 180.0f
					);
					transform.position = {
						metadata["hands"][hand]["pointer"]["offset"][0].asFloat(),
						metadata["hands"][hand]["pointer"]["offset"][1].asFloat(),
						metadata["hands"][hand]["pointer"]["offset"][2].asFloat()
					};

					auto& mesh = line.getComponent<uf::LineMesh>();
					mesh.vertices = {
						{ {0.0f, 0.0f,    0.0f} },
						{ {0.0f, 0.0f, metadata["hands"][hand]["pointer"]["length"].asFloat()} },
					};
					mesh.initialize(true);
					mesh.graphic.initializeShaders({
						{"./data/shaders/line.stereo.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
						{"./data/shaders/line.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT}
					});
					mesh.generate();
					mesh.graphic.bindUniform<uf::StereoMeshDescriptor>();
					mesh.graphic.description.rasterMode.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
					mesh.graphic.description.rasterMode.fill = VK_POLYGON_MODE_LINE;
					mesh.graphic.description.rasterMode.lineWidth = metadata["hands"][hand]["pointer"]["width"].asFloat();
					mesh.graphic.initialize();
					mesh.graphic.autoAssign();

					line.initialize();
				}

				return "true";
			});
		}

		std::vector<uf::Object*> vHands = { &::hands.left, &::hands.right };
		for ( auto pointer : vHands ) {
			auto& hand = *pointer;
			hand.addHook("VR:Input.Digital", [&](const std::string& event)->std::string{
				uf::Serializer json = event;
				
				std::string side = &hand == &hands.left ? "left" : "right";
				if ( json["hand"].asString() != side ) return "false";

				// fire mouse click
				if ( json["name"].asString() == "click" ) {

					uf::Serializer payload;
					payload["type"] = "window:Mouse.Click";
					payload["invoker"] = "vr";
					payload["mouse"]["position"]["x"]	= metadata["hands"][side]["cursor"]["position"][0].asFloat();
					payload["mouse"]["position"]["y"]	= metadata["hands"][side]["cursor"]["position"][1].asFloat();
					payload["mouse"]["delta"]["x"] 		= 0;
					payload["mouse"]["delta"]["y"] 		= 0;
					payload["mouse"]["button"] 			= side == "left" ? "Right" : "Left";
					payload["mouse"]["state"] = json["state"].asBool() ? "Down": "Up";


					uf::hooks.call( payload["type"].asString(), payload );
				}

				return "true";
			});
		}
	}
}
void ext::Hands::tick() {
	uf::Object::tick();
	auto& scene = uf::scene::getCurrentScene();
	auto& controller = *scene.getController();
	auto& camera = controller.getComponent<uf::Camera>();
	{
		pod::Transform<>& transform = hands.left.getComponent<pod::Transform<>>();
		transform.position = ext::openvr::controllerPosition( vr::Controller_Hand::Hand_Left );
		transform.orientation = ext::openvr::controllerQuaternion( vr::Controller_Hand::Hand_Left );
		transform.scale = { 1, 1, 1 };
		// transform.reference = &camera.getTransform();
	}
	{
		pod::Transform<>& transform = hands.right.getComponent<pod::Transform<>>();
		transform.position = ext::openvr::controllerPosition( vr::Controller_Hand::Hand_Right );
		transform.orientation = ext::openvr::controllerQuaternion( vr::Controller_Hand::Hand_Right );
		transform.scale = { 1, 1, 1 };
		// transform.reference = &camera.getTransform();
	}
	{
		pod::Transform<>& transform = lines.left.getComponent<pod::Transform<>>();
		transform.position = ext::openvr::controllerPosition( vr::Controller_Hand::Hand_Left, true );
		transform.orientation = ext::openvr::controllerQuaternion( vr::Controller_Hand::Hand_Left, true );
		transform.scale = { 1, 1, 1 };
		// transform.reference = hands.left.getComponentPointer<pod::Transform<>>();
	}
	{
		pod::Transform<>& transform = lines.right.getComponent<pod::Transform<>>();
		transform.position = ext::openvr::controllerPosition( vr::Controller_Hand::Hand_Right, true );
		transform.orientation = ext::openvr::controllerQuaternion( vr::Controller_Hand::Hand_Right, true );
		transform.scale = { 1, 1, 1 };
		// transform.reference = hands.right.getComponentPointer<pod::Transform<>>();

		// test
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
		if ( cMetadata["overlay"]["position"].isArray() ) 
			gtransform.position = {
				cMetadata["overlay"]["position"][0].asFloat(),
				cMetadata["overlay"]["position"][1].asFloat(),
				cMetadata["overlay"]["position"][2].asFloat(),
			};
		if ( cMetadata["overlay"]["scale"].isArray() ) 
			gtransform.scale = {
				cMetadata["overlay"]["scale"][0].asFloat(),
				cMetadata["overlay"]["scale"][1].asFloat(),
				cMetadata["overlay"]["scale"][2].asFloat(),
			};
		if ( cMetadata["overlay"]["orientation"].isArray() ) 
			gtransform.orientation = {
				cMetadata["overlay"]["orientation"][0].asFloat(),
				cMetadata["overlay"]["orientation"][1].asFloat(),
				cMetadata["overlay"]["orientation"][2].asFloat(),
				cMetadata["overlay"]["orientation"][3].asFloat(),
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

					metadata["hands"]["right"]["cursor"] = cMetadata["overlay"]["cursor"];
				}
			}
		}
	}
}
void ext::Hands::render() {
	uf::Object::render();

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	auto& scene = uf::scene::getCurrentScene();
	auto& controller = *scene.getController();
	auto& camera = controller.getComponent<uf::Camera>();

	pod::Matrix4f cameraModel = uf::matrix::translate( uf::matrix::identity(), camera.getTransform().position + controller.getComponent<pod::Transform<>>().position ) * uf::quaternion::matrix( controller.getComponent<pod::Transform<>>().orientation * pod::Vector4f{1,1,1,-1} );
	if ( hands.left.hasComponent<uf::Mesh>() ) {
		auto& mesh = hands.left.getComponent<uf::Mesh>();
		auto& transform = hands.left.getComponent<pod::Transform<>>();
		mesh.graphic.process = ext::openvr::controllerActive( vr::Controller_Hand::Hand_Left );
		pod::Matrix4f model = cameraModel * ext::openvr::controllerModelMatrix( vr::Controller_Hand::Hand_Left, false );
		if ( mesh.generated ) {	
			auto& uniforms = mesh.graphic.uniforms<uf::StereoMeshDescriptor>();
			uniforms.matrices.model = model;
			for ( std::size_t i = 0; i < 2; ++i ) {
				uniforms.matrices.view[i] = camera.getView( i );
				uniforms.matrices.projection[i] = camera.getProjection( i );
			}
			uniforms.color[0] = metadata["hands"]["left"]["controller"]["color"][0].asFloat();
			uniforms.color[1] = metadata["hands"]["left"]["controller"]["color"][1].asFloat();
			uniforms.color[2] = metadata["hands"]["left"]["controller"]["color"][2].asFloat();
			uniforms.color[3] = metadata["hands"]["left"]["controller"]["color"][3].asFloat();
			mesh.graphic.updateBuffer( uniforms, 0, false );
		}
	}
	if ( hands.right.hasComponent<uf::Mesh>() ) {
		auto& mesh = hands.right.getComponent<uf::Mesh>();
		auto& transform = hands.right.getComponent<pod::Transform<>>();
		mesh.graphic.process = ext::openvr::controllerActive( vr::Controller_Hand::Hand_Right );
		pod::Matrix4f model = cameraModel * ext::openvr::controllerModelMatrix( vr::Controller_Hand::Hand_Right, false );
		if ( mesh.generated ) {	
			auto& uniforms = mesh.graphic.uniforms<uf::StereoMeshDescriptor>();
			uniforms.matrices.model = model;
			for ( std::size_t i = 0; i < 2; ++i ) {
				uniforms.matrices.view[i] = camera.getView( i );
				uniforms.matrices.projection[i] = camera.getProjection( i );
			}
			uniforms.color[0] = metadata["hands"]["right"]["controller"]["color"][0].asFloat();
			uniforms.color[1] = metadata["hands"]["right"]["controller"]["color"][1].asFloat();
			uniforms.color[2] = metadata["hands"]["right"]["controller"]["color"][2].asFloat();
			uniforms.color[3] = metadata["hands"]["right"]["controller"]["color"][3].asFloat();
			mesh.graphic.updateBuffer( uniforms, 0, false );
		}
	}
	if ( lines.left.hasComponent<uf::Mesh>() ) {
		auto& mesh = lines.left.getComponent<uf::Mesh>();
		auto& transform = lines.left.getComponent<pod::Transform<>>();
		mesh.graphic.process = ext::openvr::controllerActive( vr::Controller_Hand::Hand_Left );
		pod::Matrix4f model = cameraModel * ext::openvr::controllerModelMatrix( vr::Controller_Hand::Hand_Left, true );
		if ( mesh.generated ) {	
			auto& uniforms = mesh.graphic.uniforms<uf::StereoMeshDescriptor>();
			uniforms.matrices.model = model;
			for ( std::size_t i = 0; i < 2; ++i ) {
				uniforms.matrices.view[i] = camera.getView( i );
				uniforms.matrices.projection[i] = camera.getProjection( i );
			}
			uniforms.color[0] = metadata["hands"]["left"]["pointer"]["color"][0].asFloat();
			uniforms.color[1] = metadata["hands"]["left"]["pointer"]["color"][1].asFloat();
			uniforms.color[2] = metadata["hands"]["left"]["pointer"]["color"][2].asFloat();
			uniforms.color[3] = metadata["hands"]["left"]["pointer"]["color"][3].asFloat();
			mesh.graphic.updateBuffer( uniforms, 0, false );
		}
	}
	if ( lines.right.hasComponent<uf::Mesh>() ) {
		auto& mesh = lines.right.getComponent<uf::Mesh>();
		auto& transform = lines.right.getComponent<pod::Transform<>>();
		mesh.graphic.process = ext::openvr::controllerActive( vr::Controller_Hand::Hand_Right );
		pod::Matrix4f model = cameraModel * ext::openvr::controllerModelMatrix( vr::Controller_Hand::Hand_Right, true );
		if ( mesh.generated ) {	
			auto& uniforms = mesh.graphic.uniforms<uf::StereoMeshDescriptor>();
			uniforms.matrices.model = model;
			for ( std::size_t i = 0; i < 2; ++i ) {
				uniforms.matrices.view[i] = camera.getView( i );
				uniforms.matrices.projection[i] = camera.getProjection( i );
			}
			uniforms.color[0] = metadata["hands"]["right"]["pointer"]["color"][0].asFloat();
			uniforms.color[1] = metadata["hands"]["right"]["pointer"]["color"][1].asFloat();
			uniforms.color[2] = metadata["hands"]["right"]["pointer"]["color"][2].asFloat();
			uniforms.color[3] = metadata["hands"]["right"]["pointer"]["color"][3].asFloat();
			mesh.graphic.updateBuffer( uniforms, 0, false );
		}
	}
/*
	if ( lines.right.hasComponent<uf::Mesh>() ) {
		auto& mesh = lines.right.getComponent<uf::Mesh>();
		// mesh.graphic.process = ext::openvr::controllerActive( vr::Controller_Hand::Hand_Right );
		auto& transform = lines.right.getComponent<pod::Transform<>>();
		if ( mesh.generated ) {				
			auto& uniforms = mesh.graphic.uniforms<uf::StereoMeshDescriptor>();
			uniforms.matrices.model = cameraModel * ext::openvr::controllerMatrix( vr::Controller_Hand::Hand_Right, true );//uf::transform::model( transform );
			for ( std::size_t i = 0; i < 2; ++i ) {
				uniforms.matrices.view[i] = camera.getView( i );
				uniforms.matrices.projection[i] = camera.getProjection( i );
			}
			uniforms.color[0] = metadata["hands"]["right"]["pointer"]["color"][0].asFloat();
			uniforms.color[1] = metadata["hands"]["right"]["pointer"]["color"][1].asFloat();
			uniforms.color[2] = metadata["hands"]["right"]["pointer"]["color"][2].asFloat();
			uniforms.color[3] = metadata["hands"]["right"]["pointer"]["color"][3].asFloat();
			mesh.graphic.updateBuffer( uniforms, 0, false );
		}
	}
*/
}