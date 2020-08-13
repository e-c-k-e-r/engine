#include "heightmap.h"

#include <uf/utils/mesh/mesh.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/noise/noise.h>

namespace {
	typedef uf::ColoredMesh MESH_TYPE;
}

EXT_OBJECT_REGISTER_CPP(Heightmap)
void ext::Heightmap::initialize() {
	uf::Object::initialize();

	{
		this->addAlias<MESH_TYPE, uf::Mesh>();
	}

	this->m_name = "Heightmap";
	auto& mesh = this->getComponent<MESH_TYPE>();
	auto& transform = this->getComponent<pod::Transform<>>();

	// generate heightmap
	{
		pod::Vector3ui size = {
			64,
			8,
			64
		};
		uf::PerlinNoise perlin;
		auto noise = perlin.collect( pod::Vector2ui{size.x, size.z}, {32, 32, 32}, [&](double&d)->double{
			return d * 1.5;
		});
		auto sampleHeightmap = [&]( pod::Vector3f& p )->double{
			std::size_t i = p.x + p.z * (size.x);
			double d = noise[i];
			return d;
		};
		for ( int32_t z = 0; z < size.z - 1; ++z ) {
		for ( int32_t x = 0; x < size.x - 1; ++x ) {
			size_t offsets[6][2] = {
				{0, 0},
				{0, 1},
				{1, 1},
				{1, 1},
				{1, 0},
				{0, 0},
			};
			for ( std::size_t i = 0; i < 6; ++i ) {
				MESH_TYPE::vertex_t vertex;
				vertex.position = {
					x + offsets[i][0], 0, z + offsets[i][1]
				};
				vertex.position.y = sampleHeightmap( vertex.position );
				{
					vertex.position.x += transform.position.x - (size.x / 2.0f);
					vertex.position.y += transform.position.y - (size.y / 2.0f);
					vertex.position.z += transform.position.z - (size.z / 2.0f);
				}
				vertex.uv = { offsets[i][0], offsets[i][1] };
				vertex.normal = { 0, 1, 0 };
				{
					pod::Vector4t<uint8_t>& p = vertex.color;
					uint32_t l = 0xFFFF * fmin( fmax( vertex.position.y, 0.2 ), 1.0 );
					p[0] = (l >> 12) & 0xF;
					p[1] = (l >>  8) & 0xF;
					p[2] = (l >>  4) & 0xF;
					p[3] = (l      ) & 0xF;
					p[0] = (p[0] << 4) | p[0];
					p[1] = (p[1] << 4) | p[1];
					p[2] = (p[2] << 4) | p[2];
					p[3] = (p[3] << 4) | p[3];
				}
				vertex.position.y *= 5;
				mesh.vertices.push_back(vertex);
			}
		}
		}
	}
	{
	//	mesh.graphic.texture.loadFromFile( "./data/textures/texture.png" );
		mesh.graphic.initialize();
		mesh.initialize(true);
		
		auto& texture = mesh.graphic.material.textures.emplace_back();
		texture.loadFromFile( "./data/textures/texture.png" );

		std::string suffix = ""; {
			std::string _ = this->getRootParent<uf::Scene>().getComponent<uf::Serializer>()["shaders"]["region"]["suffix"].asString();
			if ( _ != "" ) suffix = _ + ".";
		}

		mesh.graphic.material.attachShader("./data/shaders/heightmap.stereo.vert.spv", VK_SHADER_STAGE_VERTEX_BIT);
		mesh.graphic.material.attachShader("./data/shaders/heightmap."+suffix+"frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT);
	/*
		mesh.graphic.initializeShaders({
			{"./data/shaders/heightmap.stereo.vert.spv", VK_SHADER_STAGE_VERTEX_BIT},
			{"./data/shaders/heightmap."+suffix+"frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT}
		});
		mesh.initialize(true);
		mesh.graphic.bindUniform<uf::StereoMeshDescriptor>();
		mesh.graphic.initialize();
		mesh.graphic.autoAssign();
	*/
		mesh.graphic.process = true;
	}
}
void ext::Heightmap::tick() {
	uf::Object::tick();
}
void ext::Heightmap::destroy() {
	if ( this->hasComponent<MESH_TYPE>() ) {
		auto& mesh = this->getComponent<MESH_TYPE>();
		mesh.graphic.destroy();
		mesh.destroy();
	}
	uf::Object::destroy();
}
void ext::Heightmap::render() {
	uf::Object::render();
	/* Update uniforms */ if ( this->hasComponent<MESH_TYPE>() ) {
		auto& mesh = this->getComponent<MESH_TYPE>();
		auto& root = this->getRootParent<uf::Scene>();
		auto& player = *root.getController();
		auto& camera = player.getComponent<uf::Camera>();
		auto& transform = player.getComponent<pod::Transform<>>();
		auto& model = this->getComponent<pod::Transform<>>();
		if ( !mesh.generated ) return;
		uf::Serializer& metadata = this->getComponent<uf::Serializer>();
		//auto& uniforms = mesh.graphic.uniforms<uf::StereoMeshDescriptor>();
		// auto& uniforms = mesh.graphic.uniforms<uf::StereoMeshDescriptor>();
		auto& uniforms = mesh.graphic.material.shaders.front().uniforms.front().get<uf::StereoMeshDescriptor>();
		uniforms.matrices.model = uf::transform::model( this->getComponent<pod::Transform<>>() );
		for ( std::size_t i = 0; i < 2; ++i ) {
			uniforms.matrices.view[i] = camera.getView( i );
			uniforms.matrices.projection[i] = camera.getProjection( i );
		}
		// mesh.graphic.updateBuffer( uniforms, 0, false );
		mesh.graphic.material.shaders.front().updateBuffer( uniforms, 0, false );
	};
}