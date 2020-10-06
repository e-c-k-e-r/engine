#include "behavior.h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/graphic/mesh.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/window/window.h>
#include <uf/utils/camera/camera.h>

#include <uf/utils/renderer/renderer.h>

#include <uf/utils/image/image.h>

#include <uf/engine/asset/asset.h>
#include "../../scenes/worldscape//battle.h"
#include "../../scenes/worldscape//.h"

#include <fstream>

#define PRINT_VECTOR2(v)\
	std::cout << #v << ": " << v.x << ", " << v.y << std::endl;

namespace {
	pod::Vector3f AXIS_OF_ROTATION = { 0, 0, 1 };
	struct Bone {
		std::string name = "";
		std::string parent = "";
		float length = 0;
	
		pod::Vector2f position = { 0, 0 };
		float rotation = 0;
	};
	struct AtlasPiece {
		std::string name = "";
		struct {
			pod::Vector2f min = { 0, 0 };
			pod::Vector2f max = { 0, 0 };
		} uv;
		struct {
			pod::Vector2f topLeft = {-0.5f,  0.5f};
			pod::Vector2f topRight = { 0.5f,  0.5f};
			pod::Vector2f bottomLeft = {-0.5f, -0.5f};
			pod::Vector2f bottomRight = { 0.5f, -0.5f};
		} position;
		pod::Vector2f scale = { 1, 1 };
		pod::Vector2f size = { 0, 0 };
		pod::Quaternion<> orientation = { 0, 0, 0, 1 };
		float rotation = 0;
	};
	struct {
		uf::Serializer animation;
		uf::Serializer atlas;
		uf::Serializer overrides;
	} manifests;
	struct {
		std::unordered_map<std::string, Bone> skeleton;
		std::unordered_map<std::string, AtlasPiece> attachments;
		std::unordered_map<std::string, uf::Serializer> skins;
		std::unordered_map<std::string, uf::Serializer> slots;
	} maps;

	float getRotation( const std::string& boneName, bool flatten = false, float start = 0 ) {
		if ( manifests.overrides[boneName]["rotation"].isNumeric() ) {
			return manifests.overrides[boneName]["rotation"].asFloat();
		}
		float rotation = start;
		if ( maps.skeleton.count(boneName) < 0 ) {
			std::cout << "Bone not in skeleton: " << boneName << std::endl;
			return rotation;
		}
		Bone* bone = &maps.skeleton[boneName];
		do {
			rotation += bone->rotation;
			if ( bone->parent == "" ) break;
			bone = &maps.skeleton[bone->parent];
		} while ( flatten );
		return rotation;
	}
	pod::Quaternion<> getOrientation( const std::string& boneName, bool flatten = false, float start = 0 ) {
		float rotation = getRotation( boneName, flatten, start );
		return uf::quaternion::axisAngle( AXIS_OF_ROTATION, rotation * 3.14159f / 180.0f );
	}
	pod::Vector3f getPosition( const std::string& boneName, bool flatten = false ) {
		if ( manifests.overrides[boneName]["position"].isArray() ) {
			return pod::Vector3f{
				manifests.overrides[boneName]["position"][0].asFloat(),
				manifests.overrides[boneName]["position"][1].asFloat(),
				0,
			};
		}
		pod::Vector3f position = { 0, 0, 0 };
		if ( maps.skeleton.count(boneName) < 0 ) {
			std::cout << "Bone not in skeleton: " << boneName << std::endl;
			return position;
		}
		Bone* bone = &maps.skeleton[boneName];
		std::vector<pod::Vector2f> positions;
		do {
			if ( bone->parent == "" ) break;
			float rotation = getRotation( bone->parent, flatten );
			pod::Quaternion<> orientation = uf::quaternion::axisAngle( AXIS_OF_ROTATION, rotation * 3.14159f / 180.0f );
			position += uf::quaternion::rotate( orientation, pod::Vector3f{ bone->position.x, bone->position.y, 0 } );
			bone = &maps.skeleton[bone->parent];
		} while ( flatten );
		return position;
	}
	pod::Matrix4f getMatrix( const std::string& boneName, bool flatten = false, float start = 0 ) {
		pod::Matrix4f matrix = uf::matrix::identity();
		if ( start > 0.0f ) {
			 matrix = matrix * uf::quaternion::matrix( uf::quaternion::axisAngle( AXIS_OF_ROTATION, start ) );
		}
		if ( maps.skeleton.count(boneName) < 0 ) {
			std::cout << "Bone not in skeleton: " << boneName << std::endl;
			return matrix;
		}
		Bone* bone = &maps.skeleton[boneName];
		do {
			pod::Matrix4f translation = uf::matrix::translate( uf::matrix::identity(), {bone->position.x, bone->position.y, 0} );
			pod::Matrix4f rotation = uf::quaternion::matrix( uf::quaternion::axisAngle( AXIS_OF_ROTATION, bone->rotation ) );
			matrix = matrix * translation * rotation;
			if ( bone->parent == "" ) break;
			bone = &maps.skeleton[bone->parent];
		} while ( flatten );
		return matrix;
	}
	uf::Serializer getSkin( const uf::Serializer& slot ) {
		uf::Serializer json;
		std::string name = slot["name"].asString();
		std::string attachment = slot["attachment"].asString();
		if ( !maps.skins["default"][name][attachment].isNull() ) json = maps.skins["default"][name][attachment];
		else if ( !maps.skins["normal"][name][attachment].isNull() ) json = maps.skins["normal"][name][attachment];
		return json;
	}
}

EXT_BEHAVIOR_REGISTER_CPP(LahSpineBehavior)
#define this (&self)
void ext::LahSpineBehavior::initialize( uf::Object& self ) {
	uf::Scene& scene = uf::scene::getCurrentScene();
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();
	uf::Asset& assetLoader = scene.getComponent<uf::Asset>();

	manifests.atlas.readFromFile(metadata["system"]["root"].asString() + "atlas.json");
	manifests.animation.readFromFile(metadata["system"]["root"].asString() + "animation.json");

	this->addHook( "graphics:Assign.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		std::string filename = json["filename"].asString();
		metadata["system"]["control"] = false;

		if ( uf::io::extension(filename) != "png" ) return "false";

		uf::Scene& scene = this->getRootParent<uf::Scene>();
		uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
		const uf::Image* imagePointer = NULL;
		try { imagePointer = &assetLoader.get<uf::Image>(filename); } catch ( ... ) {}
		if ( !imagePointer ) return "false";

		uf::Image image = *imagePointer;
		// test overrides
		{
			manifests.overrides = metadata["atlas"]["overrides"];
		}
		// assemble skeleton
		{
			auto& manifest = manifests.animation;
			for ( int i = 0; i < manifest["bones"].size(); ++i ) {
				auto& entry = manifest["bones"][i];
				std::string name = entry["name"].asString();

				auto& bone = maps.skeleton[name];
				bone.name = name;
				if ( entry["parent"].isString() ) bone.parent = entry["parent"].asString();
				if ( entry["x"].isNumeric() ) bone.position.x = entry["x"].asFloat();
				if ( entry["y"].isNumeric() ) bone.position.y = entry["y"].asFloat();
				if ( entry["length"].isNumeric() ) bone.length = entry["length"].asFloat();
				if ( entry["rotation"].isNumeric() ) bone.rotation = entry["rotation"].asFloat(); // * 3.14159f / 180.0f;
			}
		}
		// create sprite attachments
		{
			auto& manifest = manifests.animation;
			for ( int i = 0; i < manifest["skins"].size(); ++i ) {
				std::string skinName = manifest["skins"][i]["name"].asString();
		 		if ( skinName == "damage" ) continue;
		 		maps.skins[skinName] = manifest["skins"][i]["attachments"];
			}
		}
		// register slots
		{
			auto& manifest = manifests.animation;
			for ( int i = 0; i < manifest["slots"].size(); ++i ) {
				std::string slotName = manifest["slots"][i]["name"].asString();
		 		maps.slots[slotName] = manifest["slots"][i];
			}
		}
		// parse atlas
		{
			auto& manifest = manifests.atlas;
			for ( auto it = manifest["pieces"].begin() ; it != manifest["pieces"].end() ; ++it ) {
				std::string name = it.key().asString();
				pod::Vector2f sheetSize = { manifest["size"][0].asFloat(), manifest["size"][1].asFloat() };
				pod::Vector2f xy = { manifest["pieces"][name]["xy"][0].asFloat(), manifest["pieces"][name]["xy"][1].asFloat() };
				pod::Vector2f size = { manifest["pieces"][name]["size"][0].asFloat(), manifest["pieces"][name]["size"][1].asFloat() };
				float rotation = 0.0f;
				auto& piece = maps.attachments[name];			
				piece.size = size;
				if ( manifest["pieces"][name]["rotate"].isNumeric() ) {
					auto& rotation = piece.rotation;
					auto& orientation = piece.orientation;

					rotation = manifest["pieces"][name]["rotate"].asFloat(); // * 3.14159f / 180.0f;
					orientation = uf::quaternion::axisAngle( AXIS_OF_ROTATION, rotation * 3.14159f / 180.0f );

					pod::Vector3f rotated;
					{
						rotated = uf::quaternion::rotate( orientation, pod::Vector3f{ size.x, size.y, 0 } );
						size.x = fabs(rotated.x);
						size.y = fabs(rotated.y);
					}
					orientation = uf::quaternion::axisAngle( AXIS_OF_ROTATION, (rotation + 90) * 3.14159f / 180.0f );
					{
						rotated = uf::quaternion::rotate( orientation, pod::Vector3f{ size.x, size.y, 0 } );
						piece.size.x = fabs(rotated.x);
						piece.size.y = fabs(rotated.y);
					}
					{
						rotated = uf::quaternion::rotate( orientation, pod::Vector3f{ piece.position.topLeft.x, piece.position.topLeft.y, 0 } );
						piece.position.topLeft.x = rotated.x;
						piece.position.topLeft.y = rotated.y;
					}
					{
						rotated = uf::quaternion::rotate( orientation, pod::Vector3f{ piece.position.topRight.x, piece.position.topRight.y, 0 } );
						piece.position.topRight.x = rotated.x;
						piece.position.topRight.y = rotated.y;
					}
					{
						rotated = uf::quaternion::rotate( orientation, pod::Vector3f{ piece.position.bottomLeft.x, piece.position.bottomLeft.y, 0 } );
						piece.position.bottomLeft.x = rotated.x;
						piece.position.bottomLeft.y = rotated.y;
					}
					{
						rotated = uf::quaternion::rotate( orientation, pod::Vector3f{ piece.position.bottomRight.x, piece.position.bottomRight.y, 0 } );
						piece.position.bottomRight.x = rotated.x;
						piece.position.bottomRight.y = rotated.y;
					}
				}

				piece.name = name;
				piece.uv.min = ( xy        ) / sheetSize;
				piece.uv.max = ( xy + size ) / sheetSize;
				piece.scale = size / sheetSize;
			}
		}
		{
			std::vector<std::string> whitelist = {};
			for ( int i = 0; i < metadata["atlas"]["whitelist"].size(); ++i ) {
				whitelist.emplace_back(metadata["atlas"]["whitelist"][i].asString());
			}
			auto& manifest = manifests.atlas;
			for ( auto it = manifest["pieces"].begin() ; it != manifest["pieces"].end() ; ++it ) {
		 		std::string name = it.key().asString();
				if ( !whitelist.empty() && std::find( whitelist.begin(), whitelist.end(), name ) == whitelist.end() ) continue;

				auto& child = uf::instantiator::instantiate<uf::Object>();
				uf::instantiator::bind( "LahSpinePieceBehavior", child );
				this->addChild(child);			
				child.initialize();
				
				uf::Serializer payload;
				payload["attachment"]["name"] = name;
				payload["attachment"]["useMatrix"] = metadata["atlas"]["useMatrix"];
				payload["filename"] = json["filename"];
				child.queueHook("graphics:Assign.%UID%", payload);
				
				metadata["attachments"][name] = child.getUid();
			}
		}

		return "true";
	});
	this->addHook( "asset:Load.%UID%", [&](const std::string& event)->std::string{
		this->queueHook("graphics:Assign.%UID%", event, 0.0f);
		return "true";
	});
}
void ext::LahSpineBehavior::tick( uf::Object& self ) {}
void ext::LahSpineBehavior::render( uf::Object& self ){
}
void ext::LahSpineBehavior::destroy( uf::Object& self ){}
#undef this

EXT_BEHAVIOR_REGISTER_CPP(LahSpinePieceBehavior)
#define this (&self)
void ext::LahSpinePieceBehavior::initialize( uf::Object& self ) {
	uf::Scene& scene = uf::scene::getCurrentScene();
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();
	uf::Asset& assetLoader = scene.getComponent<uf::Asset>();

	this->addHook( "graphics:Assign.%UID%", [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		std::string filename = json["filename"].asString();
		metadata["system"]["control"] = false;

		if ( uf::io::extension(filename) != "png" ) return "false";

		uf::Scene& scene = this->getRootParent<uf::Scene>();
		uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
		const uf::Image* imagePointer = NULL;
		try { imagePointer = &assetLoader.get<uf::Image>(filename); } catch ( ... ) {}
		if ( !imagePointer ) return "false";

		uf::Image image = *imagePointer;
		uf::Mesh& mesh = this->getComponent<uf::Mesh>();
		auto& graphic = this->getComponent<uf::Graphic>();
		auto& transform = this->getComponent<pod::Transform<>>();

		metadata["attachment"] = json["attachment"];

		metadata["color"][0] = 1;
		metadata["color"][1] = 1;
		metadata["color"][2] = 1;
		metadata["color"][3] = 1;

		std::string name = metadata["attachment"]["name"].asString();
		auto& attachment = maps.attachments[name];
		auto& slot = maps.slots[name];
	//	transform.scale = { attachment.scale.x, attachment.scale.y, 1 };
		transform.scale = { 1.0f / 512.0f, 1.0f / 512.0f, 1 };

		std::string boneName = slot["bone"].asString();
		transform.position = getPosition( boneName, true );
		transform.orientation = getOrientation( boneName, true );
		std::cout << "\"" << name << "\": {\"position\": [ " << transform.position.x << ", " << transform.position.y << " ], \"rotation\": " << getRotation( name, true ) << " }," << std::endl; 
		transform.position *= transform.scale;
		struct {
			pod::Vector2f position;
			pod::Vector2f uv;
		} topLeft, topRight, bottomLeft, bottomRight;

		{
			topLeft.position = attachment.size * attachment.position.topLeft;
			topRight.position = attachment.size * attachment.position.topRight;
			bottomLeft.position = attachment.size * attachment.position.bottomLeft;
			bottomRight.position = attachment.size * attachment.position.bottomRight;
		}
		{
			topLeft.uv = { attachment.uv.min.x, 1.0f - attachment.uv.min.y };
			topRight.uv = { attachment.uv.max.x, 1.0f - attachment.uv.min.y };
			bottomLeft.uv = { attachment.uv.min.x, 1.0f - attachment.uv.max.y };
			bottomRight.uv = { attachment.uv.max.x, 1.0f - attachment.uv.max.y };
		}

		mesh.vertices = {
			{ { topLeft.position.x, topLeft.position.y, 0.0f }, topLeft.uv, { 0.0f, 0.0f, -1.0f } },
			{ { topRight.position.x, topRight.position.y, 0.0f }, topRight.uv,{ 0.0f, 0.0f, -1.0f } },
			{ { bottomRight.position.x, bottomRight.position.y, 0.0f }, bottomRight.uv, { 0.0f, 0.0f, -1.0f } },
		
			{ { bottomRight.position.x, bottomRight.position.y, 0.0f }, bottomRight.uv, { 0.0f, 0.0f, -1.0f } },
			{ { bottomLeft.position.x, bottomLeft.position.y, 0.0f }, bottomLeft.uv, { 0.0f, 0.0f, -1.0f } },
			{ { topLeft.position.x, topLeft.position.y, 0.0f }, topLeft.uv, { 0.0f, 0.0f, -1.0f } },
		};
		{
			std::cout << name << ". Slot: " << slot << std::endl;
			auto skin = getSkin( slot );
			struct WeightedVertex {
				int boneId;
				float weight;
				pod::Vector2f position;
			};
			struct SkinnedVertex {
				std::vector<WeightedVertex> weights;
				pod::Vector2f position;
				pod::Vector2f uv;
			};
			std::vector<SkinnedVertex> skinnedVertices;

			std::cout << name << ", Vertices?:\n";
			for ( int i = 0; i < skin["vertices"].size(); ) {
				int read = skin["vertices"][i++].asInt();
				auto& skinVertex = skinnedVertices.emplace_back();
				for ( int j = 0; j < read; ++j ) {
					auto& weightedVertex = skinVertex.weights.emplace_back();
					weightedVertex.boneId = skin["vertices"][i++].asInt();
					weightedVertex.position.x = skin["vertices"][i++].asFloat();
					weightedVertex.position.y = skin["vertices"][i++].asFloat();
					weightedVertex.weight = skin["vertices"][i++].asFloat();
				}
			}
			for ( int i = 0; i < skinnedVertices.size(); ++i ) {
				auto& skinVertex = skinnedVertices.at(i);
				skinVertex.uv.x = skin["uvs"][(i*2+0)].asFloat();
				skinVertex.uv.y = skin["uvs"][(i*2+1)].asFloat();

				// average
				for ( auto& weightedVertex : skinVertex.weights ) {
					std::string boneName = manifests.animation["bones"][weightedVertex.boneId]["name"].asString();
					auto bonePosition = getPosition( boneName, true );
					auto boneOrientation = getOrientation( boneName, true );

					bonePosition = uf::quaternion::rotate( boneOrientation, bonePosition );
					auto rotatedVertex = uf::quaternion::rotate( boneOrientation, pod::Vector3f{ weightedVertex.position.x, weightedVertex.position.y, 0 } );
					skinVertex.position.x += (rotatedVertex.x + bonePosition.x) * weightedVertex.weight;
					skinVertex.position.y += (rotatedVertex.y + bonePosition.y) * weightedVertex.weight;
				// 	wx += (vx * bone.a + vy * bone.b + bone.worldX) * weight;
				}
			}
			for ( int i = 0; i < skinnedVertices.size(); ++i ) {
				auto& skinVertex = skinnedVertices.at(i);
				std::cout << "Index " << i << ":\n";
				for ( auto& weightedVertex : skinVertex.weights ) {
					std::string boneName = manifests.animation["bones"][weightedVertex.boneId]["name"].asString();
					std::cout << "\tPosition: " << weightedVertex.position.x << ", " << weightedVertex.position.y << " | Bone ID: " << weightedVertex.boneId << " | Bone Name: " << boneName << " | Weight: " << weightedVertex.weight << "\n";
				}
				std::cout << "\tUV: " << skinVertex.uv.x << ", " << skinVertex.uv.y << "\n";
				std::cout << "\tWeighted Position: " << skinVertex.position.x << ", " << skinVertex.position.y << "\n";
			}
		
			std::cout << std::endl;
			std::cout << name << ", Triangles:\n";

			mesh.vertices.clear();

			for ( auto i = 0; i < skin["triangles"].size(); ) {
				std::cout << "Triangle:\n";
				for ( int j = 0; j < 3; ++j ) {
					int index = skin["triangles"][i++].asInt();
					auto& skinVertex = skinnedVertices.at(index);
					struct {
						pod::Vector2f interp;
					} position, uv;
					
					uv.interp.x = std::lerp( attachment.uv.min.x, attachment.uv.max.x, skinVertex.uv.x );
					uv.interp.y = std::lerp( attachment.uv.min.y, attachment.uv.max.y, skinVertex.uv.y );

				//	uv.interp = skinVertex.uv;
					position.interp.x = (uv.interp.x - 0.5f) * attachment.size.x;
					position.interp.y = (uv.interp.y - 0.5f) * attachment.size.y;

					std::cout << "\tIndex: " << index << "\tUV: " << uv.interp.x << ", " << uv.interp.y << "\tPosition: " << position.interp.x << ", " << position.interp.y << "\n";

					auto& vertex = mesh.vertices.emplace_back();
				//	vertex.position.x = position.interp.x;
				//	vertex.position.y = position.interp.y;

					vertex.position.x = skinVertex.position.x;
					vertex.position.y = skinVertex.position.y;
					
					vertex.position.z = 0;
					vertex.uv.x = uv.interp.x;
					vertex.uv.y = 1.0f - uv.interp.y;
					vertex.normal = { 0, 0, -1 };
				}
				std::cout << std::endl;
			}
		/*
			std::cout << name << ", Triangles:\n";
			for ( auto i = 0; i < skin["triangles"].size(); i += 3 ) {
			//	std::cout << "\t" << skin["triangles"][i+0].asInt() << " " << skin["triangles"][i+1].asInt() << " " << skin["triangles"][i+2].asInt() << std::endl;
				std::cout << "\t" << skin["uvs"][skin["triangles"][i+0].asInt()].asFloat() << " " << skin["uvs"][skin["triangles"][i+1].asInt()].asFloat() << " " << skin["uvs"][skin["triangles"][i+2].asInt()].asFloat()
			}
		*/
			std::cout << name << ", "
			 	<< "UVs: " << skin["uvs"].size() << ", "
			 	<< "Positions: " << skin["vertices"].size() << ", "
			 	<< "Triangles: " << skin["triangles"].size() << ", "
			 	<< "Edges: " << skin["edges"].size() << ", "
			 	<< "Hull: " << skin["hull"]
			 << std::endl;
		}

		graphic.initialize( "Gui" );
		graphic.initializeGeometry( mesh );

		auto& texture = graphic.material.textures.emplace_back();
		texture.loadFromImage( image );

		graphic.material.attachShader("./data/shaders/base.stereo.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		graphic.material.attachShader("./data/shaders/base.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);

		metadata["system"]["control"] = true;
		metadata["system"]["loaded"] = true;

		return "true";
	});
}
void ext::LahSpinePieceBehavior::tick( uf::Object& self ) {}
void ext::LahSpinePieceBehavior::render( uf::Object& self ){
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	if ( !metadata["system"]["loaded"].asBool() ) return;

	/* Update uniforms */ if ( this->hasComponent<uf::Graphic>() ) {
		auto& mesh = this->getComponent<uf::Mesh>();
		auto& scene = uf::scene::getCurrentScene();
		auto& graphic = this->getComponent<uf::Graphic>();
		auto& transform = this->getComponent<pod::Transform<>>();
		auto& camera = scene.getController().getComponent<uf::Camera>();		
		if ( !graphic.initialized ) return;
		auto& uniforms = graphic.material.shaders.front().uniforms.front().get<uf::StereoMeshDescriptor>();
		auto translation = uf::matrix::translate( uf::matrix::identity(), transform.position );
		auto rotation = uf::quaternion::matrix( transform.orientation );
		auto scale = uf::matrix::scale( uf::matrix::identity(), transform.scale );
		auto model = translation * rotation * scale;

		std::string name = metadata["attachment"]["name"].asString();
		auto& attachment = maps.attachments[name];
		auto bone = getMatrix( name, true );

		uniforms.matrices.model = metadata["attachment"]["useMatrix"].asBool() ? bone : model;

		for ( std::size_t i = 0; i < 2; ++i ) {
			uniforms.matrices.view[i] = camera.getView( i );
			uniforms.matrices.projection[i] = camera.getProjection( i );
		}
	
		uniforms.color[0] = metadata["color"][0].asFloat();
		uniforms.color[1] = metadata["color"][1].asFloat();
		uniforms.color[2] = metadata["color"][2].asFloat();
		uniforms.color[3] = metadata["color"][3].asFloat();
		graphic.material.shaders.front().updateBuffer( uniforms, 0, false );
	};
}
void ext::LahSpinePieceBehavior::destroy( uf::Object& self ){}
#undef this