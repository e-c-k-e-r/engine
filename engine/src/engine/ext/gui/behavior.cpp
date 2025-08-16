#include "behavior.h"
#include "./glyph/behavior.h"
#include "./manager/behavior.h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/image/atlas.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/window/window.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/mesh/mesh.h>
#include <uf/utils/graphic/graphic.h>
#include <uf/utils/string/ext.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/text/glyph.h>
#include <uf/engine/asset/asset.h>
#include <uf/engine/scene/scene.h>
#include <uf/utils/memory/unordered_map.h>
#include <uf/utils/renderer/renderer.h>
#include <uf/ext/openvr/openvr.h>
#include <uf/utils/http/http.h>
#include <uf/utils/audio/audio.h>
#include <uf/utils/window/payloads.h>

#include "./payload.h"

#define EXT_COLOR_FLOATS 1

namespace {
	struct {
		bool initialized = false;

		uf::Mesh mesh;
		uf::Image image;
		uf::Serializer settings;
	} defaults;

	struct Mesh {
		pod::Vector3f position;
		pod::Vector2f uv;

	#if EXT_COLOR_FLOATS
		pod::Vector4f color;
	#else
		pod::ColorRgba color;
	#endif

		static uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
	};

	template<size_t N = uf::renderer::settings::maxViews>
	struct UniformDescriptor {
		struct Matrices {
			/*alignas(16)*/ pod::Matrix4f model;
		} matrices[N];
		struct Gui {
			/*alignas(16)*/ pod::Vector4f offset;
			/*alignas(16)*/ pod::Vector4f color;

			/*alignas(4)*/ int32_t mode = 0;
			/*alignas(4)*/ float depth = 0.0f;
			/*alignas(8)*/ float padding1;
			/*alignas(8)*/ float padding2;
		} gui;
	};
}

UF_VERTEX_DESCRIPTOR(Mesh,
	UF_VERTEX_DESCRIPTION(Mesh, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(Mesh, R32G32_SFLOAT, uv)
#if EXT_COLOR_FLOATS
	UF_VERTEX_DESCRIPTION(Mesh, R32G32B32A32_SFLOAT, color)
#else
	UF_VERTEX_DESCRIPTION(Mesh, R8G8B8A8_UNORM, color)
#endif
)

namespace {
	uf::Image& generateImage( uf::Image& image ) {
		image.open(uf::io::root+"/textures/missing.png");
		return image;
	}
	uf::Mesh& generateMesh( uf::Mesh& mesh, const pod::Vector4f& color = {1, 1, 1, 1} ) {
		mesh.bind<::Mesh, uint16_t>();
		mesh.insertVertices<::Mesh>({
			{ pod::Vector3f{-1.0f,  1.0f, 0.0f}, pod::Vector2f{0.0f, 0.0f}, color },
			{ pod::Vector3f{-1.0f, -1.0f, 0.0f}, pod::Vector2f{0.0f, 1.0f}, color },
			{ pod::Vector3f{ 1.0f, -1.0f, 0.0f}, pod::Vector2f{1.0f, 1.0f}, color },
		
			{ pod::Vector3f{ 1.0f, -1.0f, 0.0f}, pod::Vector2f{1.0f, 1.0f}, color },
			{ pod::Vector3f{ 1.0f,  1.0f, 0.0f}, pod::Vector2f{1.0f, 0.0f}, color },
			{ pod::Vector3f{-1.0f,  1.0f, 0.0f}, pod::Vector2f{0.0f, 0.0f}, color },
		});
		mesh.insertIndices<uint16_t>({
			0, 1, 2, 3, 4, 5
		});

		return mesh;
	}
}

// YUCK
UF_OBJECT_REGISTER_BEGIN(ext::Gui)
//	UF_OBJECT_REGISTER_BEHAVIOR(uf::EntityBehavior)
	UF_OBJECT_REGISTER_BEHAVIOR(uf::ObjectBehavior)
	UF_OBJECT_REGISTER_BEHAVIOR(ext::GuiBehavior)
	UF_OBJECT_REGISTER_BEHAVIOR(ext::GuiGlyphBehavior)
UF_OBJECT_REGISTER_END()

UF_BEHAVIOR_REGISTER_CPP(ext::GuiBehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::GuiBehavior, ticks = true, renders = false, multithread = false)
#define this (&self)
void ext::GuiBehavior::initialize( uf::Object& self ) {
	auto& metadata = this->getComponent<ext::GuiBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();

	auto& transform = this->getComponent<pod::Transform<>>();
	auto& mesh = this->getComponent<uf::Mesh>();

	auto& scene = uf::scene::getCurrentScene();
	
	if ( !::defaults.initialized ) {
		::defaults.initialized = true;

		::generateImage( ::defaults.image );
		::generateMesh( ::defaults.mesh );
	}
/*
	if ( ext::json::isNull( ::defaults.settings["metadata"] ) ) ::defaults.settings.readFromFile(uf::io::root+"/entities/gui.json");
	// set defaults
	if ( metadataJson["string"].is<uf::stl::string>() ) {
		auto copyMetadataJson = metadataJson;
		ext::json::forEach(::defaults.settings["metadata"], [&]( const uf::stl::string& key, ext::json::Value& value ){
			if ( ext::json::isNull( copyMetadataJson[key] ) ) {
				metadataJson[key] = value;
			}
		});
	}
*/

	UF_BEHAVIOR_METADATA_BIND_SERIALIZER_HOOKS(metadata, metadataJson);

	this->addHook( "gui:Update.%UID%", [&](ext::payloads::GuiInitializationPayload& payload){
		auto& graphic = this->getComponent<uf::Graphic>();

	/*
		if ( metadataJson["mode"].as<uf::stl::string>() == "flat" ) {
			if ( ext::json::isNull(metadataJson["projection"]) ) metadataJson["projection"] = false;
			if ( ext::json::isNull(metadataJson["flip uv"]) ) metadataJson["flip uv"] = true;
			if ( ext::json::isNull(metadataJson["front face"]) ) metadataJson["front face"] = "ccw";
		} else {
			if ( ext::json::isNull(metadataJson["projection"]) ) metadataJson["projection"] = true;
			if ( ext::json::isNull(metadataJson["flip uv"]) ) metadataJson["flip uv"] = false;
			if ( ext::json::isNull(metadataJson["front face"]) ) metadataJson["front face"] = "cw";
		}

		if ( metadataJson["world"].as<bool>() ) {
		//	metadataJson["gui layer"] = false;
		} else {
		#if UF_USE_OPENGL
			if ( ext::json::isNull(metadataJson["cull mode"]) ) metadataJson["cull mode"] = "front";
			if ( uf::matrix::reverseInfiniteProjection ) metadata.depth = 1 - metadata.depth;
		//	transform.position.z = metadata.depth;
		#else
		//	if ( metadataJson["flip uv"].as<bool>() ) for ( auto& v : vertices ) v.uv.y = 1 - v.uv.y;
		#endif
		//	for ( auto& v : vertices ) v.position.z = metadata.depth;
		}
	*/
		if ( ext::json::isNull(metadataJson["cull mode"]) ) metadataJson["cull mode"] = "back";

		// 
		graphic.descriptor.parse( metadataJson );

		// bind texture data
		if ( payload.image ) {
			if ( metadata.initialized ) {
				for ( auto& texture : graphic.material.textures ) texture.destroy();
				graphic.material.textures.clear();
			}

			auto& image = *payload.image;
	
			metadata.size = image.getDimensions();

			auto& texture = graphic.material.textures.emplace_back();
			texture.loadFromImage( image );
			
			// update transform
			{
			
			}

			if ( payload.free ) {
				delete payload.image;
				payload.image = {};
			}
		}

		// bind mesh data
		if ( payload.mesh ) {
			auto& mesh = *payload.mesh;

			uf::Mesh::Attribute positionAttribute;
			uf::Mesh::Attribute uvAttribute;
			for ( auto& attribute : mesh.vertex.attributes ) {
				if ( attribute.descriptor.name == "position" ) positionAttribute = attribute;
				if ( attribute.descriptor.name == "uv" ) uvAttribute = attribute;
			}

		#if UF_USE_OPENGL
			if ( uf::matrix::reverseInfiniteProjection ) metadata.depth = 1 - metadata.depth;
			
			// set depth 
			for ( auto i = 0; i < mesh.vertex.count; ++i ) {
				float* position = (float*) (static_cast<uint8_t*>(positionAttribute.pointer) + i * positionAttribute.stride );
				position[2] = metadata.depth;
			}
		#endif


			if ( !metadata.initialized ) {
				graphic.initialize( metadata.renderMode );
				graphic.initializeMesh( mesh );

				struct {
					uf::stl::string vertex = uf::io::root+"/shaders/gui/base/vert.spv";
					uf::stl::string fragment = uf::io::root+"/shaders/gui/base/frag.spv";
				} filenames;
				uf::stl::string suffix = ""; {
					uf::stl::string _ = scene.getComponent<uf::Serializer>()["shaders"]["gui"]["suffix"].as<uf::stl::string>();
					if ( _ != "" ) suffix = _ + ".";
				}
				if ( metadataJson["shaders"]["vertex"].is<uf::stl::string>() ) filenames.vertex = metadataJson["shaders"]["vertex"].as<uf::stl::string>();
				if ( metadataJson["shaders"]["fragment"].is<uf::stl::string>() ) filenames.fragment = metadataJson["shaders"]["fragment"].as<uf::stl::string>();
				else if ( suffix != "" ) filenames.fragment = uf::io::root+"/shaders/gui/"+suffix+"base/frag.spv";

				graphic.material.initializeShaders({
					{filenames.vertex, uf::renderer::enums::Shader::VERTEX},
					{filenames.fragment, uf::renderer::enums::Shader::FRAGMENT},
				});


			#if UF_USE_VULKAN
				{
					auto& shader = graphic.material.getShader("vertex");
					struct SpecializationConstant {
						uint32_t passes = 6;
					};
					auto& specializationConstants = shader.specializationConstants.get<SpecializationConstant>();
					specializationConstants.passes = uf::renderer::settings::maxViews;
				}
			#endif
			} else {
				graphic.initializeMesh( mesh );
				graphic.getPipeline().update( graphic );
			}
			
			if ( payload.free ) {
				delete payload.mesh;
				payload.mesh = {};
			}
		}


		metadata.initialized = true;
	});

	this->addHook( "asset:Load.%UID%", [&](pod::payloads::assetLoad& payload){
		if ( !uf::asset::isExpected( payload, uf::asset::Type::IMAGE ) ) return;
		if ( !uf::asset::has( payload ) ) uf::asset::load( payload );

		auto& image = uf::asset::get<uf::Image>( payload );

		// generate default mesh
		::generateMesh( mesh, metadata.color );

		{
			ext::payloads::GuiInitializationPayload payload;
			payload.image = &image;
			payload.mesh = &mesh;
			this->callHook("gui:Update.%UID%", payload);
		}
	});


	if ( metadata.ui.click.able ) {
		uf::Timer<long long> clickTimer(false);
		clickTimer.start( uf::Time<>(-1000000) );
		if ( !clickTimer.running() ) clickTimer.start();

		this->addHook( "window:Mouse.Click", [&](pod::payloads::windowMouseClick& payload){
			if ( metadata.world ) return;
			//if ( !metadata.box.min && !metadata.box.max ) return;
			if ((metadata.box.min.x > metadata.box.max.x)||(metadata.box.min.y > metadata.box.max.y)) return;

		//	uf::Object* manager = (uf::Object*) this->globalFindByName("Gui Manager");
		//	pod::Vector2ui guiSize = manager ? manager->getComponent<ext::GuiManagerBehavior::Metadata>().size : pod::Vector2ui{ uf::renderer::settings::width, uf::renderer::settings::height };
			pod::Vector2ui guiSize = pod::Vector2ui{ uf::renderer::settings::width, uf::renderer::settings::height };
			
			bool clicked = false;
			if ( payload.mouse.state == -1 ) {
				pod::Vector2f click;
				click.x = (float) payload.mouse.position.x / (float) guiSize.x;
				click.y = (float) payload.mouse.position.y / (float) guiSize.y;

				click.x = (click.x * 2.0f) - 1.0f;
				click.y = (click.y * 2.0f) - 1.0f;

				float x = click.x;
				float y = click.y;


				if (payload.invoker == "vr" ) {
					x = payload.mouse.position.x;
					y = payload.mouse.position.y;
				}
				clicked = ( metadata.box.min.x <= x && metadata.box.min.y <= y && metadata.box.max.x >= x && metadata.box.max.y >= y );
			
				int minX = (metadata.box.min.x * 0.5f + 0.5f) * guiSize.x;
				int minY = (metadata.box.min.y * 0.5f + 0.5f) * guiSize.y;

				int maxX = (metadata.box.max.x * 0.5f + 0.5f) * guiSize.x;
				int maxY = (metadata.box.max.y * 0.5f + 0.5f) * guiSize.y;

				int mouseX = payload.mouse.position.x;
				int mouseY = payload.mouse.position.y;
			}

			if ( !metadata.ui.click.ed && clicked ) {
				this->callHook("gui:ClickStart.%UID%", payload);
			} else if ( metadata.ui.click.ed && !clicked ) {
				this->callHook("gui:ClickEnd.%UID%", payload);
			}
			
			metadata.ui.click.ed = clicked;

			if ( clicked ) {
				this->callHook("gui:Clicked.%UID%", payload);
			}

			this->callHook("gui:Mouse.Clicked.%UID%", payload);
		} );


		this->addHook( "gui:ClickStart.%UID%", [&]( pod::payloads::windowMouseClick& payload ){
			uf::Serializer jsonPayload;
			this->callHook("gui:ClickStart.%UID%", jsonPayload);
		});
		this->addHook( "gui:ClickEnd.%UID%", [&]( pod::payloads::windowMouseClick& payload ){
			uf::Serializer jsonPayload;
			this->callHook("gui:ClickEnd.%UID%", jsonPayload);
		});
		this->addHook( "gui:Clicked.%UID%", [&](ext::json::Value& json){
			if ( ext::json::isNull( json ) ) return;

			pod::payloads::windowMouseClick payload;
			this->callHook("gui:Clicked.%UID%", payload);
		});
		this->addHook( "gui:Clicked.%UID%", [&](pod::payloads::windowMouseClick& payload){
			uf::Serializer jsonPayload;
			this->callHook("gui:Clicked.%UID%", jsonPayload);

			if ( ext::json::isObject( metadataJson["events"]["click"] ) ) {
				ext::json::Value event = metadataJson["events"]["click"];
				metadataJson["events"]["click"] = ext::json::array();
				metadataJson["events"]["click"][0] = event;
			} else if ( !ext::json::isArray( metadataJson["events"]["click"] ) ) {
				this->getParent().as<uf::Object>().callHook("gui:Clicked.%UID%", payload);
				return;
			}
			for ( int i = 0; i < metadataJson["events"]["click"].size(); ++i ) {
				ext::json::Value event = metadataJson["events"]["click"][i];
				ext::json::Value payload = event["payload"];
				if ( event["delay"].is<float>() ) {
					this->queueHook(event["name"].as<uf::stl::string>(), payload, event["delay"].as<float>());
				} else {
					this->callHook(event["name"].as<uf::stl::string>(), payload );
				}
			}
		});
	}
	if ( metadata.ui.hover.able ) {
		uf::Timer<long long> hoverTimer(false);
		hoverTimer.start( uf::Time<>(-1000000) );

		this->addHook( "window:Mouse.Moved", [&](pod::payloads::windowMouseMoved& payload){
			if ( metadata.world ) return;
			//if ( !metadata.box.min && !metadata.box.max ) return;
			if ((metadata.box.min.x > metadata.box.max.x)||(metadata.box.min.y > metadata.box.max.y)) return;

		//	uf::Object* manager = (uf::Object*) this->globalFindByName("Gui Manager");
		//	pod::Vector2ui guiSize = manager ? manager->getComponent<ext::GuiManagerBehavior::Metadata>().size : pod::Vector2ui{ uf::renderer::settings::width, uf::renderer::settings::height };
			pod::Vector2ui guiSize = pod::Vector2ui{ uf::renderer::settings::width, uf::renderer::settings::height };

			bool hovered = false;
			pod::Vector2f click;
			click.x = (float) payload.mouse.position.x / (float) guiSize.x;
			click.y = (float) payload.mouse.position.y / (float) guiSize.y;

			click.x = (click.x * 2.0f) - 1.0f;
			click.y = (click.y * 2.0f) - 1.0f;

			float x = click.x;
			float y = click.y;

			hovered = ( metadata.box.min.x <= x && metadata.box.min.y <= y && metadata.box.max.x >= x && metadata.box.max.y >= y );

			if ( hovered ) {
				int minX = (metadata.box.min.x * 0.5f + 0.5f) * guiSize.x;
				int minY = (metadata.box.min.y * 0.5f + 0.5f) * guiSize.y;

				int maxX = (metadata.box.max.x * 0.5f + 0.5f) * guiSize.x;
				int maxY = (metadata.box.max.y * 0.5f + 0.5f) * guiSize.y;

				int mouseX = payload.mouse.position.x;
				int mouseY = payload.mouse.position.y;
			}

			uf::Serializer jsonPayload = ext::json::null();

			// to-do: do something about trying to trigger json-bound hooks
			if ( !metadata.ui.hover.ed && hovered ) {
				this->callHook("gui:HoverStart.%UID%", payload);
			} else if ( metadata.ui.hover.ed && !hovered ) {
				this->callHook("gui:HoverEnd.%UID%", payload);
			} else if ( hovered ) { /*&& hoverTimer.elapsed().asDouble() >= 1 ) {
				hoverTimer.reset();*/
				this->callHook("gui:Hovered.%UID%", payload);
			}

			metadata.ui.hover.ed = hovered;
			this->callHook("gui:Mouse.Moved.%UID%", payload);
		});

		this->addHook( "gui:HoverStart.%UID%", [&]( pod::payloads::windowMouseMoved& payload ){
			uf::Serializer jsonPayload;
			this->callHook("gui:HoverStart.%UID%", jsonPayload);
		});
		this->addHook( "gui:HoverEnd.%UID%", [&]( pod::payloads::windowMouseMoved& payload ){
			uf::Serializer jsonPayload;
			this->callHook("gui:HoverEnd.%UID%", jsonPayload);
		});
		this->addHook( "gui:Hovered.%UID%", [&](ext::json::Value& json){
			if ( ext::json::isNull( json ) ) return;

			pod::payloads::windowMouseMoved payload;
			this->callHook("gui:Hovered.%UID%", payload);
		});
		this->addHook( "gui:Hovered.%UID%", [&](pod::payloads::windowMouseMoved& payload){
			uf::Serializer jsonPayload;
			this->callHook("gui:Hovered.%UID%", jsonPayload);

			if ( ext::json::isObject( metadataJson["events"]["hover"] ) ) {
				ext::json::Value event = metadataJson["events"]["hover"];
				metadataJson["events"]["hover"] = ext::json::array();
				metadataJson["events"]["hover"][0] = event;
			} else if ( !ext::json::isArray( metadataJson["events"]["hover"] ) ) {
				this->getParent().as<uf::Object>().callHook("gui:Hovered.%UID%", payload);
				return;
			}
			for ( int i = 0; i < metadataJson["events"]["hover"].size(); ++i ) {
				ext::json::Value event = metadataJson["events"]["hover"][i];
				ext::json::Value payload = event["payload"];
				if ( event["delay"].is<float>() ) {
					this->queueHook(event["name"].as<uf::stl::string>(), payload, event["delay"].as<float>());
				} else {
					this->callHook(event["name"].as<uf::stl::string>(), payload );
				}
			}
		});
	}
}
void ext::GuiBehavior::tick( uf::Object& self ) {
	if ( !this->hasComponent<uf::Graphic>() ) return;

	auto& transform = this->getComponent<pod::Transform<>>();
	auto& metadata = this->getComponent<ext::GuiBehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();
	auto& mesh = this->getComponent<uf::Mesh>();
	auto& graphic = this->getComponent<uf::Graphic>();
	
	auto model = uf::matrix::identity();
	auto flatten = uf::transform::flatten( transform );
	
	auto& scene = uf::scene::getCurrentScene();
	auto& controller = scene.getController();
	auto& camera = controller.getComponent<uf::Camera>();

//	uf::Object* manager = (uf::Object*) this->globalFindByName("Gui Manager");
//	pod::Vector2ui guiSize = manager ? manager->getComponent<ext::GuiManagerBehavior::Metadata>().size : pod::Vector2ui{ uf::renderer::settings::width, uf::renderer::settings::height };
	pod::Vector2ui guiSize = pod::Vector2ui{ uf::renderer::settings::width, uf::renderer::settings::height };

	if ( metadata.scaleMode == "fixed" || metadata.scaleMode == "fixed-x" ) {
		flatten.scale.x *= (float) metadata.size.x / (float) guiSize.x;
	}
	if ( metadata.scaleMode == "fixed" || metadata.scaleMode == "fixed-y" ) {
		flatten.scale.y *= (float) metadata.size.x / (float) guiSize.y;
	}

	if ( metadata.scaleMode == "relative" || metadata.scaleMode == "relative-x" ) {
		flatten.scale.x *= (float) guiSize.y / (float) guiSize.x;
	}
	if ( metadata.scaleMode == "relative" || metadata.scaleMode == "relative-y" ) {
		flatten.scale.y *= (float) guiSize.x / (float) guiSize.y;
	}

	// bind UBO
	#if UF_USE_OPENGL
	{
		// "flip" the viewport because I can't be assed to do it within the rendermode
		if ( metadata.mode == 0 ) {
			flatten.scale.y = -flatten.scale.y;
			flatten.position.y = -flatten.position.y;
		}
		model = uf::transform::model( transform );
		auto& shader = graphic.material.getShader("vertex");
		pod::Uniform uniform;
		uniform.color = metadata.color;

		if ( metadata.mode == 1 ) {
			uniform.modelView = model; 
			uniform.projection = uf::matrix::identity();
		} else if ( metadata.mode == 2 ) {
			uniform.modelView = camera.getView() * uf::transform::model( transform );
			uniform.projection = camera.getProjection();
		} else if ( metadata.mode == 3 ) {
			uniform.modelView = 
				uf::matrix::translate( uf::matrix::identity(), flatten.position ) *
				uf::matrix::scale( uf::matrix::identity(), flatten.scale ) *
				uf::quaternion::matrix( flatten.orientation ) *
				flatten.model;
			uniform.projection = camera.getProjection();
		} else {
			uniform.modelView = 
				uf::matrix::translate( uf::matrix::identity(), flatten.position ) *
				uf::matrix::scale( uf::matrix::identity(), flatten.scale ) *
				uf::quaternion::matrix( flatten.orientation ) *
				flatten.model;
			uniform.projection = uf::matrix::identity();
		}

		shader.updateUniform( "UBO", (const void*) &uniform, sizeof(uniform) );
	}
	#else
	if ( graphic.material.hasShader("vertex") ) {
		auto& shader = graphic.material.getShader("vertex");
		if ( shader.hasUniform("UBO") ) {
			auto& uniformBuffer = shader.getUniformBuffer("UBO");

			UniformDescriptor<> uniforms{};

			for ( auto i = 0; i < uf::renderer::settings::maxViews; ++i ) {
				if ( metadata.mode == 1 ) {
					uniforms.matrices[i].model = transform.model; 
				} else if ( metadata.mode == 2 ) {
					auto& scene = uf::scene::getCurrentScene();
					auto& controller = scene.getController();
					auto& camera = controller.getComponent<uf::Camera>();
					uniforms.matrices[i].model = camera.getProjection(i) * camera.getView(i) * uf::transform::model( transform );
				} else if ( metadata.mode == 3 ) {
					uniforms.matrices[i].model = 
						camera.getProjection(i) *
						uf::matrix::translate( uf::matrix::identity(), flatten.position ) *
						uf::matrix::scale( uf::matrix::identity(), flatten.scale ) *
						uf::quaternion::matrix( flatten.orientation ) *
						flatten.model;
				} else {
					uniforms.matrices[i].model = 
						uf::matrix::translate( uf::matrix::identity(), flatten.position ) *
						uf::matrix::scale( uf::matrix::identity(), flatten.scale ) *
						uf::quaternion::matrix( flatten.orientation ) *
						flatten.model;
				}
			}

			uniforms.gui = ::UniformDescriptor<>::Gui{
				.offset = metadata.uv,
				.color = metadata.color,

				.mode = 0, // metadata.shader,
				.depth = uf::matrix::reverseInfiniteProjection ? 1 - metadata.depth : metadata.depth,
			};
			shader.updateBuffer( (const void*) &uniforms, sizeof(uniforms), uniformBuffer );

			model = uniforms.matrices[0].model;
		}
	}
	#endif

	// calculate bounding box
	{
		pod::Vector2f min = {  std::numeric_limits<float>::max(),  std::numeric_limits<float>::max() };
		pod::Vector2f max = { -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max() };

		uf::Mesh::Attribute vertexAttribute;
		for ( auto& attribute : mesh.vertex.attributes ) if ( attribute.descriptor.name == "position" ) { vertexAttribute = attribute; break; }
		UF_ASSERT( vertexAttribute.descriptor.name == "position" );

		for ( auto i = 0; i < mesh.vertex.count; ++i ) {
			float* p = (float*) (static_cast<uint8_t*>(vertexAttribute.pointer) + i * vertexAttribute.stride );
		//	pod::Vector4f position = { p[0], p[1], p[2], 1 };
			pod::Vector4f position = { p[0], p[1], 0, 1 };
			pod::Vector4f translated = uf::matrix::multiply<float>( model, position );
			min.x = std::min( min.x, translated.x );
			max.x = std::max( max.x, translated.x );
			
			min.y = std::min( min.y, translated.y );
			max.y = std::max( max.y, translated.y );
		}
		
		metadata.box.min.x = min.x;
		metadata.box.min.y = min.y;
		metadata.box.max.x = max.x;
		metadata.box.max.y = max.y;
	}
}
void ext::GuiBehavior::render( uf::Object& self ){}
void ext::GuiBehavior::destroy( uf::Object& self ){}
void ext::GuiBehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ){
	serializer["color"] = uf::vector::encode( /*this->*/color );
	serializer["uv"] = uf::vector::encode( /*this->*/uv );
//	serializer["scaling"] = uf::vector::encode( /*this->*/scaling );

	serializer["depth"] = /*this->*/depth;
	serializer["mode"] = /*this->*/mode;
	serializer["renderMode"] = /*this->*/renderMode;
	serializer["scaling"] = /*this->*/scaleMode;

	serializer["clickable"] = /*this->*/ui.click.able;
	serializer["clicked"] = /*this->*/ui.click.ed;
	
	serializer["hoverable"] = /*this->*/ui.hover.able;
	serializer["hovered"] = /*this->*/ui.hover.ed;
}
void ext::GuiBehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ){
	/*this->*/color = uf::vector::decode( serializer["color"], /*this->*/color );
	/*this->*/uv = uf::vector::decode( serializer["uv"], /*this->*/uv );
//	/*this->*/scaling = uf::vector::decode( serializer["scaling"], /*this->*/scaling );

	/*this->*/depth = serializer["depth"].as( /*this->*/depth );
	/*this->*/mode = serializer["mode"].as( /*this->*/mode );
	/*this->*/renderMode = serializer["renderMode"].as( /*this->*/renderMode );
	/*this->*/scaleMode = serializer["scaling"].as( /*this->*/scaleMode );
	
	/*this->*/ui.click.able = serializer["clickable"].as( /*this->*/ui.click.able );
	/*this->*/ui.hover.able = serializer["hoverable"].as( /*this->*/ui.hover.able );
}
#undef this