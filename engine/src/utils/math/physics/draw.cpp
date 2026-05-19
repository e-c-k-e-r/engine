#include <uf/utils/math/physics/common.h>
#include <uf/utils/math/physics/narrowphase.h>
#include <uf/engine/scene/scene.h>
#include <uf/engine/graph/graph.h>

namespace {
	// define a struct for a line because I hate hate hate tuple syntax
	struct Line {
		pod::Vector3f start = {};
		pod::Vector3f end = {};
		pod::Vector4f color = { 1, 1, 1, 1 };
		float ttl = 0;
	};

	uf::stl::vector<impl::Vertex> lines;
	uf::stl::unordered_map<size_t, Line> transientLines;

	size_t getHash( const pod::Vector3f& start, const pod::Vector3f& end, const pod::Vector4f& color, const pod::PhysicsBody* a, const pod::PhysicsBody* b ) {
		size_t hash = 0;
	/*
		int qx = static_cast<int>(start.x * 10.0f);
		int qy = static_cast<int>(start.y * 10.0f);
		int qz = static_cast<int>(start.z * 10.0f);
		uf::hash(hash, a, b, qx, qy, qz);
	*/
		uf::hash(hash, start, end, color);
		return hash;
	}
}

UF_VERTEX_DESCRIPTOR(impl::Vertex,
	UF_VERTEX_DESCRIPTION(impl::Vertex, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(impl::Vertex, R32G32B32A32_SFLOAT, color)
)

void impl::addLine( const pod::Vector3f& start, const pod::Vector3f& end, const pod::Vector4f& color ) {
	::lines.emplace_back( impl::Vertex{ start, color } );
	::lines.emplace_back( impl::Vertex{ end, color } );
}
void impl::addTransientLine( const pod::Vector3f& start, const pod::Vector3f& end, const pod::Vector4f& color, const pod::PhysicsBody* a, const pod::PhysicsBody* b ) {
	::transientLines[::getHash( start, end, color, a, b )] = Line{ start, end, color, 1.0f };
}

void impl::drawManifold( const pod::Manifold& manifold ) {
	for ( auto& contact : manifold.points ) {
		auto& start = contact.point;
		auto end = contact.point + (contact.normal * MIN(contact.penetration, 0.1f));

		impl::addTransientLine( start, end, pod::Vector4f{ 1, 0, 0, 1 }, manifold.a, manifold.b );
	}
}
void impl::drawBody( const pod::PhysicsBody& body ) {
	switch( body.collider.type ) {
		case pod::ShapeType::AABB:
			impl::drawAabb( body );
		break;
		case pod::ShapeType::OBB:
			impl::drawObb( body );
		break;
		case pod::ShapeType::SPHERE:
			impl::drawSphere( body );
		break;
		case pod::ShapeType::CAPSULE:
			impl::drawCapsule( body );
		break;
		case pod::ShapeType::PLANE:
			impl::drawPlane( body );
		break;
		case pod::ShapeType::TRIANGLE:
			impl::drawTriangle( body );
		break;
		case pod::ShapeType::MESH:
			impl::drawMesh( body );
		break;
		case pod::ShapeType::CONVEX_HULL:
			impl::drawHull( body );
		break;
	}
}

void impl::draw( const pod::World& world, float dt ) {
	if ( world.bodies.empty() ) return;

	::lines.clear();

	for ( auto* body : world.bodies ) impl::drawBody( *body );
	for ( auto it = ::transientLines.begin(); it != ::transientLines.end(); ) {
		auto& line = it->second;
		
		if ( line.ttl <= 0 ) it = ::transientLines.erase( it );
		else {
			impl::addLine( line.start, line.end, line.color * pod::Vector4f{ 1, 1, 1, line.ttl } );
			line.ttl -= dt;
			++it;
		}
	}

	if ( ::lines.empty() ) return;

	uf::Mesh mesh;
	mesh.bind<impl::Vertex>();
	mesh.insertVertices<impl::Vertex>(::lines);

	auto& scene = uf::scene::getCurrentScene();
	auto& graphics = scene.getComponent<uf::renderer::Graphics>();
	auto& graphic = graphics["physics"];
	if ( !graphic.initialized ) {
		graphic.device = &uf::renderer::device;
		graphic.material.device = &uf::renderer::device;

		graphic.descriptor.depth.test = false;		
		graphic.descriptor.depth.write = false;		
		graphic.descriptor.renderTarget = 1; // "forward";
		graphic.descriptor.topology = uf::renderer::enums::PrimitiveTopology::LINE_LIST;
		graphic.descriptor.fill = uf::renderer::enums::PolygonMode::LINE;
		graphic.descriptor.lineWidth = 4;

		uf::stl::string vertexShaderFilename = uf::io::resolveURI(uf::io::root+"/shaders/base/line/vert.spv");
		uf::stl::string fragmentShaderFilename = uf::io::resolveURI(uf::io::root+"/shaders/base/line/frag.spv");

		graphic.material.metadata.autoInitializeUniformBuffers = false;
		graphic.material.attachShader(vertexShaderFilename, uf::renderer::enums::Shader::VERTEX);
		graphic.material.attachShader(fragmentShaderFilename, uf::renderer::enums::Shader::FRAGMENT);
		graphic.material.metadata.autoInitializeUniformBuffers = true;
		
		auto& storage = uf::graph::globalStorage ? uf::graph::storage : scene.getComponent<pod::Graph::Storage>();
		
		// vertex shader
		{
			auto& shader = graphic.material.getShader("vertex");
			shader.aliasBuffer( storage.buffers.camera );
		}

		graphic.initialize();
		graphic.initializeMesh( mesh );
		UF_MSG_DEBUG("Initialized graphic");
	} else {
		bool rebuild = graphic.updateMesh( mesh );
		if ( rebuild ) uf::renderer::states::rebuild = true;
	}
}
