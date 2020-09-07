#include "hands.h"

#include <uf/utils/graphic/mesh.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/math/collision.h>
#include <uf/utils/thread/thread.h>

#include <uf/ext/openvr/openvr.h>

#include "../terrain/generator.h"
#include "../world.h"

namespace {
	struct {
		uf::Object left, right;
	} hands, lines;
	struct {
		uf::Object* left;
		uf::Object* right;
	} lights;
}

EXT_OBJECT_REGISTER_CPP(Hands)
void ext::Hands::initialize() {
	uf::Object::initialize();
	
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();

	{
		this->addChild(::hands.left);
		this->addChild(::hands.right);

		::hands.left.addChild(::lines.left);
		::hands.right.addChild(::lines.right);
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
				std::string side = "";
				if ( name == metadata["hands"]["left"]["controller"]["model"].asString() ) {
					side = "left";
				} else if ( name == metadata["hands"]["right"]["controller"]["model"].asString() ) {
					side = "right";
				};
				if ( side == "" ) return "false";

				uf::Object& hand = side == "left" ? hands.left : hands.right;
				uf::Object& line = side == "left" ? lines.left : lines.right;
				{
					uf::Graphic& graphic = (hand.getComponent<uf::Graphic>() = ext::openvr::getRenderModel( name ));
					
					graphic.process = true;

					graphic.descriptor.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
					graphic.material.attachShader("./data/shaders/base.stereo.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
					graphic.material.attachShader("./data/shaders/base.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

					hand.initialize();
				}
				if ( metadata["hands"][side]["pointer"]["length"].asFloat() > 0 ) {
					line.addAlias<uf::LineMesh, uf::Mesh>();

					pod::Transform<>& transform = line.getComponent<pod::Transform<>>();
					transform.orientation = uf::quaternion::axisAngle( 
						{
							metadata["hands"][side]["pointer"]["orientation"]["axis"][0].asFloat(),
							metadata["hands"][side]["pointer"]["orientation"]["axis"][1].asFloat(),
							metadata["hands"][side]["pointer"]["orientation"]["axis"][2].asFloat()
						},
						metadata["hands"][side]["pointer"]["orientation"]["angle"].asFloat() * 3.14159f / 180.0f
					);
					transform.position = {
						metadata["hands"][side]["pointer"]["offset"][0].asFloat(),
						metadata["hands"][side]["pointer"]["offset"][1].asFloat(),
						metadata["hands"][side]["pointer"]["offset"][2].asFloat()
					};

					auto& mesh = line.getComponent<uf::LineMesh>();
					auto& graphic = line.getComponent<uf::Graphic>();

					mesh.vertices = {
						{ {0.0f, 0.0f, 0.0f} },
						{ {0.0f, 0.0f, metadata["hands"][side]["pointer"]["length"].asFloat()} },
					};
					graphic.initialize();
					graphic.initializeGeometry(mesh);
					
					graphic.material.attachShader("./data/shaders/line.stereo.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
					graphic.material.attachShader("./data/shaders/line.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
					graphic.descriptor.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
					graphic.descriptor.fill = VK_POLYGON_MODE_LINE;
					graphic.descriptor.lineWidth = metadata["hands"][side]["pointer"]["width"].asFloat();

					line.initialize();
				}
				if ( metadata["hands"][side]["light"]["should"].asBool() ){
					auto* child = (uf::Object*) hand.findByUid(hand.loadChild("/light.json", false));
					if ( child ) {
						if (side == "left" ) lights.left = child; else lights.right = child;
						auto& json = metadata["hands"][side]["light"];
						auto& light = side == "left" ? *lights.left : *lights.right;

						auto& metadata = light.getComponent<uf::Serializer>();
						if ( !json["color"].isNull() ) metadata["light"]["color"] = json["color"];
						if ( !json["radius"].isNull() ) metadata["light"]["radius"] = json["radius"];
						if ( !json["power"].isNull() ) metadata["light"]["power"] = json["power"];
						if ( !json["shadows"].isNull() ) metadata["light"]["shadows"] = json["shadows"];
						metadata["lights"]["external update"] = true;

						light.initialize();
					}
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
			hand.addHook("world:Collision.%UID%", [&](const std::string& event)->std::string{
				uf::Serializer json = event;
	
				std::string side = &hand == &hands.left ? "left" : "right";

				float mag = json["depth"].asFloat();
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
void ext::Hands::tick() {
	uf::Object::tick();
	auto& scene = uf::scene::getCurrentScene();
	auto& controller = *scene.getController();
	auto& controllerCamera = controller.getComponent<uf::Camera>();
	auto& controllerTransform = controller.getComponent<pod::Transform<>>();
	auto& controllerCameraTransform = controllerCamera.getTransform();
	{
		pod::Transform<>& transform = hands.left.getComponent<pod::Transform<>>();
		transform.position = ext::openvr::controllerPosition( vr::Controller_Hand::Hand_Left );
		transform.orientation = ext::openvr::controllerQuaternion( vr::Controller_Hand::Hand_Left );
		transform.scale = { 1, 1, 1 };
		transform.reference = &controllerTransform;

		auto& collider = hands.left.getComponent<uf::Collider>();
		for ( auto* box : collider.getContainer() ) {
			box->getTransform().position = transform.position + controllerTransform.position + controllerCameraTransform.position;
		}
	}
	{
		pod::Transform<>& transform = hands.right.getComponent<pod::Transform<>>();
		transform.position = ext::openvr::controllerPosition( vr::Controller_Hand::Hand_Right );
		transform.orientation = ext::openvr::controllerQuaternion( vr::Controller_Hand::Hand_Right );
		transform.scale = { 1, 1, 1 };
		transform.reference = &controllerTransform;

		auto& collider = hands.right.getComponent<uf::Collider>();
		for ( auto* box : collider.getContainer() ) {
			box->getTransform().position = transform.position + controllerTransform.position + controllerCameraTransform.position;
		}
	}
	{
		pod::Transform<>& transform = lines.left.getComponent<pod::Transform<>>();
		transform.position = ext::openvr::controllerPosition( vr::Controller_Hand::Hand_Left, true );
		transform.orientation = ext::openvr::controllerQuaternion( vr::Controller_Hand::Hand_Left, true );
		transform.scale = { 1, 1, 1 };
		transform.reference = &controllerTransform;

		if ( lights.left ) {
			auto& light = *lights.left;
			auto& lightTransform = light.getComponent<pod::Transform<>>();
			lightTransform.position = controllerCameraTransform.position + controller.getComponent<pod::Transform<>>().position + transform.position;
		//	lightTransform.orientation = controllerTransform.orientation * transform.orientation;
			auto& lightCamera = light.getComponent<uf::Camera>();
			pod::Matrix4f playerModel = uf::matrix::identity();
			pod::Matrix4f translation = uf::matrix::translate( uf::matrix::identity(), controllerCameraTransform.position + controllerTransform.position );
			pod::Matrix4f rotation = uf::quaternion::matrix( controllerTransform.orientation * pod::Vector4f{1,1,1,-1} );
			playerModel = translation * rotation;

			pod::Matrix4f model = uf::matrix::inverse( playerModel * ext::openvr::controllerModelMatrix( vr::Controller_Hand::Hand_Left, true ) );
			for ( size_t i = 0; i < 2; ++i ) lightCamera.setView( model, i );
		
		//	lightTransform.position = transform.position;
		//	lightTransform.orientation = transform.orientation;
		//	lightTransform.reference = &controllerTransform;
		}
		// transform.reference = hands.left.getComponentPointer<pod::Transform<>>();
	}
	{
		pod::Transform<>& transform = lines.right.getComponent<pod::Transform<>>();
		transform.position = ext::openvr::controllerPosition( vr::Controller_Hand::Hand_Right, true );
		transform.orientation = ext::openvr::controllerQuaternion( vr::Controller_Hand::Hand_Right, true );
		transform.scale = { 1, 1, 1 };
		transform.reference = &controllerTransform;

		if ( lights.right ) {
			auto& light = *lights.right;
			auto& lightTransform = light.getComponent<pod::Transform<>>();
			lightTransform.position = controllerCameraTransform.position + controllerTransform.position + transform.position;
		//	lightTransform.orientation = controllerTransform.orientation * transform.orientation;
			auto& lightCamera = light.getComponent<uf::Camera>();
			pod::Matrix4f playerModel = uf::matrix::identity();
			pod::Matrix4f translation = uf::matrix::translate( uf::matrix::identity(), controllerCameraTransform.position + controllerTransform.position );
			pod::Matrix4f rotation = uf::quaternion::matrix( controllerTransform.orientation * pod::Vector4f{1,1,1,-1} );
			playerModel = translation * rotation;

			pod::Matrix4f model = uf::matrix::inverse( playerModel * ext::openvr::controllerModelMatrix( vr::Controller_Hand::Hand_Right, true ) );
			for ( size_t i = 0; i < 2; ++i ) lightCamera.setView( model, i );
		
		//	lightTransform.position = transform.position;
		//	lightTransform.orientation = transform.orientation;
		//	lightTransform.reference = &controllerTransform;
		}
		// transform.reference = hands.right.getComponentPointer<pod::Transform<>>();
	}

	// raytrace pointer / hand collision
	{
		std::vector<uf::Object*> handPointers = { &::hands.left, &::hands.right };
		for ( auto pointer : handPointers ) { auto& hand = *pointer;
			std::string side = &hand == &hands.left ? "left" : "right";
			if ( !ext::openvr::controllerActive( side == "left" ? vr::Controller_Hand::Hand_Left : vr::Controller_Hand::Hand_Right ) ) continue;
			{
				pod::Transform<>& transform = (side == "left" ? lines.left : lines.right).getComponent<pod::Transform<>>();
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

							metadata["hands"][side]["cursor"] = cMetadata["overlay"]["cursor"];
						}
					}
				}
			}
		#if 0
			/* Collision against world */ {
				bool local = true;
				bool sort = false;
			//	pod::Thread& thread = uf::thread::fetchWorker();
				pod::Thread& thread = uf::thread::has("Physics") ? uf::thread::get("Physics") : uf::thread::create( "Physics", true, false );
				auto function = [&]() -> int {
					if ( !controller.hasParent() ) return 0;
					if ( controller.getParent().getName() != "Region" ) return 0;
					
					pod::Transform<> transform = hand.getComponent<pod::Transform<>>();
					transform.position = uf::quaternion::rotate( controller.getComponent<pod::Transform<>>().orientation, transform.position );
					transform.position += controller.getComponent<pod::Transform<>>().position;
					transform.position += controllerCamera.getTransform().position;

					uf::Entity& parent = controller.getParent();
					ext::TerrainGenerator& generator = parent.getComponent<ext::TerrainGenerator>();
					uf::Serializer& rMetadata = parent.getComponent<uf::Serializer>();
					pod::Transform<>& rTransform = parent.getComponent<pod::Transform<>>();

					pod::Vector3ui size; {
						size.x = rMetadata["region"]["size"][0].asUInt();
						size.y = rMetadata["region"]["size"][1].asUInt();
						size.z = rMetadata["region"]["size"][2].asUInt();
					}
					pod::Vector3f voxelPosition = transform.position - rTransform.position;
					voxelPosition.x += size.x / 2.0f;
					voxelPosition.y += size.y / 2.0f + 1;
					voxelPosition.z += size.z / 2.0f;	

					uf::CollisionBody pCollider;
					std::vector<pod::Vector3ui> positions = {
						{ voxelPosition.x, voxelPosition.y, voxelPosition.z },
						{ voxelPosition.x - 1, voxelPosition.y, voxelPosition.z },
						{ voxelPosition.x + 1, voxelPosition.y, voxelPosition.z },
						{ voxelPosition.x, voxelPosition.y - 1, voxelPosition.z },
						{ voxelPosition.x, voxelPosition.y + 1, voxelPosition.z },
						{ voxelPosition.x, voxelPosition.y, voxelPosition.z - 1 },
						{ voxelPosition.x, voxelPosition.y, voxelPosition.z + 1 },
					};
					
					{
						auto& metadata = this->getComponent<uf::Serializer>();
						// bottom
						uint16_t uid = generator.getVoxel( voxelPosition.x, voxelPosition.y, voxelPosition.z );
						auto light = generator.getLight( voxelPosition.x, voxelPosition.y, voxelPosition.z );
						metadata["hands"][side]["controller"]["color"][0] = ((light >> 12) & 0xF) / (float) (0xF);
						metadata["hands"][side]["controller"]["color"][1] = ((light >>  8) & 0xF) / (float) (0xF);
						metadata["hands"][side]["controller"]["color"][2] = ((light >>  4) & 0xF) / (float) (0xF);
						metadata["hands"][side]["controller"]["color"][3] = ((light      ) & 0xF) / (float) (0xF);
					}
					
					for ( auto& position : positions ) {
						ext::TerrainVoxel voxel = ext::TerrainVoxel::atlas( generator.getVoxel( position.x, position.y, position.z ) );
						pod::Vector3 offset = rTransform.position;
						offset.x += position.x - (size.x / 2.0f);
						offset.y += position.y - (size.y / 2.0f);
						offset.z += position.z - (size.z / 2.0f);

						if ( !voxel.solid() ) continue;

						uf::Collider* box = new uf::BoundingBox( offset, {0.5, 0.5, 0.5} );
						pCollider.add(box);
					}
					
					uf::CollisionBody& collider = hand.getComponent<uf::CollisionBody>(); {
						collider.clear();
						uf::Collider* box = new uf::BoundingBox( transform.position, {0.25, 0.25, 0.25} );
						collider.add(box);
					}
					
					pod::Physics& physics = hand.getComponent<pod::Physics>();
					auto result = pCollider.intersects(collider);
					uf::Collider::Manifold strongest;
					strongest.depth = 0.001;					
					for ( auto manifold : result ) {
						if ( manifold.colliding && manifold.depth > 0 ) {
							if ( strongest.depth < manifold.depth ) strongest = manifold;
						}
					}
					if ( strongest.colliding ) {
						
						pod::Vector3 correction = uf::vector::normalize(strongest.normal) * -(strongest.depth * strongest.depth * 1.001);
						{
							float mag = uf::vector::magnitude( correction );
							uf::Serializer payload;
							payload["delay"] = 0.0f;
							payload["duration"] = uf::physics::time::delta;
							payload["frequency"] = 1.0f;
							payload["amplitude"] = fmin(1.0f, 1000.0f * mag);
							payload["side"] = side;
							uf::hooks.call( "VR:Haptics." + side, payload );
						}
					}
					return 0;
				};
				if ( local ) function(); else uf::thread::add( thread, function, true );
			}
		#endif
		}
	}
}
void ext::Hands::render() {
	uf::Object::render();

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	auto& scene = uf::scene::getCurrentScene();
	auto& controller = *scene.getController();
	auto& camera = controller.getComponent<uf::Camera>();

	pod::Matrix4f playerModel = uf::matrix::identity(); {
		auto& controller = this->getParent();
		auto& camera = controller.getComponent<uf::Camera>();
		pod::Matrix4f translation = uf::matrix::translate( uf::matrix::identity(), camera.getTransform().position + controller.getComponent<pod::Transform<>>().position );
		pod::Matrix4f rotation = uf::quaternion::matrix( controller.getComponent<pod::Transform<>>().orientation * pod::Vector4f{1,1,1,-1} );
		playerModel = translation * rotation;
	}
//	pod::Matrix4f cameraModel = uf::matrix::translate( uf::matrix::identity(), camera.getTransform().position + controller.getComponent<pod::Transform<>>().position ) * uf::quaternion::matrix( controller.getComponent<pod::Transform<>>().orientation * pod::Vector4f{1,1,1,-1} );	
	if ( hands.left.hasComponent<uf::Graphic>() ) {
		auto& graphic = hands.left.getComponent<uf::Graphic>();
		auto& transform = hands.left.getComponent<pod::Transform<>>();
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
			uniforms.color[0] = metadata["hands"]["left"]["controller"]["color"][0].asFloat();
			uniforms.color[1] = metadata["hands"]["left"]["controller"]["color"][1].asFloat();
			uniforms.color[2] = metadata["hands"]["left"]["controller"]["color"][2].asFloat();
			uniforms.color[3] = metadata["hands"]["left"]["controller"]["color"][3].asFloat();
			// graphic.updateBuffer( uniforms, 0, false );
			graphic.material.shaders.front().updateBuffer( uniforms, 0, false );
		}
	}
	if ( hands.right.hasComponent<uf::Graphic>() ) {
		auto& graphic = hands.right.getComponent<uf::Graphic>();
		auto& transform = hands.right.getComponent<pod::Transform<>>();
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
			uniforms.color[0] = metadata["hands"]["right"]["controller"]["color"][0].asFloat();
			uniforms.color[1] = metadata["hands"]["right"]["controller"]["color"][1].asFloat();
			uniforms.color[2] = metadata["hands"]["right"]["controller"]["color"][2].asFloat();
			uniforms.color[3] = metadata["hands"]["right"]["controller"]["color"][3].asFloat();
			// graphic.updateBuffer( uniforms, 0, false );
			graphic.material.shaders.front().updateBuffer( uniforms, 0, false );
		}
	}
	if ( lines.left.hasComponent<uf::Graphic>() ) {
		auto& graphic = lines.left.getComponent<uf::Graphic>();
		auto& transform = lines.left.getComponent<pod::Transform<>>();
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
			uniforms.color[0] = metadata["hands"]["left"]["pointer"]["color"][0].asFloat();
			uniforms.color[1] = metadata["hands"]["left"]["pointer"]["color"][1].asFloat();
			uniforms.color[2] = metadata["hands"]["left"]["pointer"]["color"][2].asFloat();
			uniforms.color[3] = metadata["hands"]["left"]["pointer"]["color"][3].asFloat();
			// graphic.updateBuffer( uniforms, 0, false );
			graphic.material.shaders.front().updateBuffer( uniforms, 0, false );
		}
	}
	if ( lines.right.hasComponent<uf::Graphic>() ) {
		auto& graphic = lines.right.getComponent<uf::Graphic>();
		auto& transform = lines.right.getComponent<pod::Transform<>>();
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
			uniforms.color[0] = metadata["hands"]["right"]["pointer"]["color"][0].asFloat();
			uniforms.color[1] = metadata["hands"]["right"]["pointer"]["color"][1].asFloat();
			uniforms.color[2] = metadata["hands"]["right"]["pointer"]["color"][2].asFloat();
			uniforms.color[3] = metadata["hands"]["right"]["pointer"]["color"][3].asFloat();
			// graphic.updateBuffer( uniforms, 0, false );
			graphic.material.shaders.front().updateBuffer( uniforms, 0, false );
		}
	}
}