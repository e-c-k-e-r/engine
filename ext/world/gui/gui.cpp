#include "gui.h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/window/window.h>
#include <uf/gl/camera/camera.h>
#include <uf/gl/glyph/glyph.h>
#include "../world.h"

#include <unordered_map>

namespace {
	pod::Vector2ui size;
	pod::Matrix4 matrix;
	ext::freetype::Glyph glyph;

	uf::GlyphMesh mesh;
	std::unordered_map<unsigned long, uf::GlyphTexture> glyphs;
}

void ext::Gui::initialize() {
	ext::Object::initialize();
	
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();

	if ( this->m_name == "Gui Manager" )
		for ( uint i = 0; i < metadata["gui"]["list"].size(); ++i ) {
			std::string config = metadata["gui"]["list"][i].asString();
			uf::Entity* entity = new ext::Gui;
			ext::Gui& gui = *((ext::Gui*) entity);
			if ( !gui.load(config) ) { uf::iostream << "Error loading `" << config << "!" << "\n"; delete entity; continue; }
			this->addChild(*entity);
			entity->initialize();
		}
	else {
		if ( metadata["gui"]["text"] != Json::nullValue ) {
			// First time initialization
			if ( ::glyphs.empty() ) {
				ext::freetype::initialize( ::glyph, "./fonts/" + metadata["gui"]["font"].asString() );
			/*
				for ( unsigned long i = ' '; i <= '~'; ++i ) {
					uf::GlyphTexture& texture = ::glyphs[i];
					texture.generate( ::glyph, i, metadata["gui"]["size"].asInt() );
				}
			*/
				std::vector<float> vertices = {
					0, 1, 0, 0,
					0, 0, 0, 1,
					1, 0, 1, 1,

					0, 1, 0, 0,
					1, 0, 1, 1,
					1, 1, 1, 0,
				};
				::mesh.getPoints().getVertices() = vertices;
				::mesh.generate();

				if ( metadata["gui"]["origin"] == "top" )
					metadata["gui"]["text"] = metadata["gui"]["text"].asString();

				std::cout << "Text: " << metadata["gui"]["text"] << std::endl;
				std::cout << uf::String( metadata["gui"]["text"].asString() ) << std::endl;
				
			}
			if ( metadata["gui"]["scale"] == Json::nullValue ) metadata["gui"]["scale"] = 1.0;
		}
		if ( metadata["gui"]["position"] != Json::nullValue ) {
			pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
			transform.position.x = metadata["gui"]["position"][0].asDouble();
			transform.position.y = metadata["gui"]["position"][1].asDouble();
			transform.position.z = metadata["gui"]["position"][2].asDouble();
		}
		metadata["gui"]["render"] = true;
		metadata["hooks"]["window:Resized"][metadata["hooks"].size()] = uf::hooks.addHook( "window:Resized", [&](const std::string& event)->std::string{
			uf::Serializer json = event;

			// Update persistent window sized (size stored to JSON file)
			pod::Vector2ui size; {
				size.x = json["window"]["size"]["x"].asUInt64();
				size.y = json["window"]["size"]["y"].asUInt64();
			}
			uf::Texture& texture = this->getComponent<uf::Texture>();
			pod::Transform<>& transform = this->getComponent<pod::Transform<>>();

			if ( metadata["gui"]["scaling"].asString() == "relative" ) {
				transform.scale = pod::Vector3{ (double) texture.getImage().getDimensions().x / size.x, (double) texture.getImage().getDimensions().y / size.y, 1 };
			} else if ( metadata["gui"]["scaling"].asString() == "fixed" ) {
				transform.scale = pod::Vector3{ (double) texture.getImage().getDimensions().x / 1920, (double) texture.getImage().getDimensions().y / 1080, 1 };
			}

			::matrix = uf::matrix::ortho( 0.0, (double) size.x, 0.0, (double) size.y );
			::size = size;

			return "true";
		} );
	}
}
void ext::Gui::tick() {

}
void ext::Gui::render() {
	ext::World& world = this->getRootParent<ext::World>();
	uf::Serializer& gamestate = world.getComponent<uf::Serializer>();
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	if ( gamestate["state"].asInt() != 2 ) return;

	uf::Entity::render();

	if ( !metadata["gui"]["render"].asBool() ) return;
	if ( metadata["gui"]["text"] != Json::nullValue ) {
		uf::Shader& shader = this->getComponent<uf::Shader>();
		pod::Transform<>& transform = this->getComponent<pod::Transform<>>();

		glDisable(GL_CULL_FACE);
		uf::String string( metadata["gui"]["text"].asString() );
		auto str = string.translate<uf::locale::Utf8>();
	//	auto str = string.getString();
		int X = 0; int Y = 0; uint biggest = 0;
		for ( auto it = str.begin(); it != str.end(); ++it ) {
			unsigned long c = *it; if ( c == '\n' ) continue;
			uf::GlyphTexture& glyph = ::glyphs[c];
			if ( !glyph.generated() ) glyph.generate( ::glyph, c, metadata["gui"]["size"].asInt() );
			biggest = std::max( biggest, glyph.getSize().y);
		}
		if ( metadata["gui"]["origin"] == "top" ) Y = ::size.y - transform.position.y - biggest; else Y += biggest;
		for ( auto it = str.begin(); it != str.end(); ++it ) {
			unsigned long c = *it; if ( c == '\n' ) {
				if ( metadata["gui"]["direction"] == "up" ) Y += biggest; else Y -= biggest;
				X = 0;
				continue;
			}
			uf::GlyphTexture& glyph = ::glyphs[c];
			struct {
				float x, y, w, h;
			} dim;
			float scale = metadata["gui"]["scale"].asFloat();
			dim.x = X + transform.position.x + glyph.getBearing().x * scale;
			dim.w = glyph.getSize().x * scale;
			if ( dim.x + dim.w >= ::size.x ) {
				if ( metadata["gui"]["direction"] == "up" ) Y += biggest; else Y -= biggest;
				dim.x -= X; X = 0;
			}
			dim.y = Y + transform.position.y - (glyph.getSize().y - glyph.getBearing().y) * scale;
			dim.h = glyph.getSize().y * scale;
			

			X += (glyph.getAdvance().x >> 6) * scale;
			Y += (glyph.getAdvance().y >> 6) * scale;


			std::vector<float> vertices = {
				dim.x        , dim.y + dim.h, 0, 0,
				dim.x        , dim.y        , 0, 1,
				dim.x + dim.w, dim.y        , 1, 1,

				dim.x        , dim.y + dim.h, 0, 0,
				dim.x + dim.w, dim.y        , 1, 1,
				dim.x + dim.w, dim.y + dim.h, 1, 0,
			};

			shader.bind(); {
				int i = 0;
				glyph.bind(i);
				shader.push("gui.texture", i++);
				shader.push("gui.color", pod::Vector3{metadata["gui"]["color"][0].asFloat(), metadata["gui"]["color"][1].asFloat(), metadata["gui"]["color"][2].asFloat()});
				shader.push("matrices.model", ::matrix);
			}

			::mesh.getPoints().getVertices() = vertices;
			::mesh.generate();
			::mesh.render();
		}
		glEnable(GL_CULL_FACE);
	}
	else if ( this->hasComponent<uf::Texture>() ) {
		ext::Gui& parent = this->getParent<ext::Gui>();
		uf::GeometryBuffer& buffer = world.getComponent<uf::GeometryBuffer>();
		buffer.unbind();
		
		uf::Shader& shader = this->hasComponent<uf::Shader>() ? this->getComponent<uf::Shader>() : parent.getComponent<uf::Shader>();
		uf::Texture& texture = this->getComponent<uf::Texture>();
		pod::Transform<>& transform = this->getComponent<pod::Transform<>>();
		ext::Player& player = world.getPlayer();
		uf::Camera& camera = player.getComponent<uf::Camera>();

		pod::Vector4 offset = { metadata["gui"]["uv"][0].asDouble(), metadata["gui"]["uv"][1].asDouble(), metadata["gui"]["uv"][2].asDouble(), metadata["gui"]["uv"][3].asDouble() };
		pod::Matrix4 matrix = uf::transform::model(transform);
		if ( metadata["gui"]["world"].asBool() ) {
			pod::Transform<> flatten = uf::transform::flatten(camera.getTransform(), true);
			pod::Matrix4 rotation = uf::quaternion::matrix( uf::vector::multiply( { 1, 1, 1, -1 }, flatten.orientation) );
			matrix = matrix * rotation;
		}

		shader.bind(); { int i = 0; for ( auto& texture : buffer.getBuffers() ) {
				texture.bind(i);
				shader.push("buffers."+texture.getName(), i++);
			}
			texture.bind(i); shader.push("gui.texture", i++);
			shader.push("gui.offset", offset);
			shader.push("parameters.window", camera.getSize());
			shader.push("parameters.depth", metadata["gui"]["depth"].asBool());
			shader.push("matrices.model", matrix);
			if ( metadata["gui"]["world"].asBool() ) {
				shader.push("matrices.projection", camera.getProjection());
				shader.push("matrices.view", camera.getView());
			}
		}
		
		uf::Mesh& mesh = buffer.getComponent<uf::Mesh>();
		mesh.render();
	}
}