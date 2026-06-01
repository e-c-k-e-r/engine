#include <uf/utils/debug/draw.h>

#include <uf/engine/scene/scene.h>
#include <uf/engine/graph/graph.h>
#include <uf/utils/math/physics.h>
#include <uf/utils/text/graphic.h>

namespace impl {
	struct Vertex {
		pod::Vector3f position;
		pod::Vector4f color;

		static uf::stl::vector<uf::renderer::AttributeDescriptor> descriptor;
	};

	struct Line {
		pod::Vector3f start = {};
		pod::Vector3f end = {};
		pod::Vector4f color = { 1, 1, 1, 1 };
		float ttl = 0;
	};


	float decayRate = 1.0f;
	uf::stl::vector<impl::Vertex> lines;
	uf::stl::vector<impl::Line> transientLines;
	uf::Mesh lineMesh;
}

namespace impl {
	struct Text {
		uf::stl::string string = "";
		pod::Vector3f position = {};
		pod::Vector4f color = { 1, 1, 1, 1 };
	};
	uf::stl::vector<impl::Text> texts;
	uf::Mesh textMesh;
	uf::Atlas textAtlas;

	pod::GlyphSettings textSettings = {
		.alignment = "center",
		.font = "FragmentMono.ttf",
		.size = 48,
		.spread = 0,
	};
}

UF_VERTEX_DESCRIPTOR(impl::Vertex,
	UF_VERTEX_DESCRIPTION(impl::Vertex, R32G32B32_SFLOAT, position)
	UF_VERTEX_DESCRIPTION(impl::Vertex, R32G32B32A32_SFLOAT, color)
)

void uf::debug::drawLine( const pod::Vector3f& start, const pod::Vector3f& end, const pod::Vector4f& color ) {
	impl::lines.emplace_back( impl::Vertex{ start, color } );
	impl::lines.emplace_back( impl::Vertex{ end, color } );
}

void uf::debug::drawShape( const pod::AABB& aabb, const pod::Transform<>& transform ) {
	pod::Vector3f corners[8] = {
		{aabb.min.x, aabb.min.y, aabb.min.z}, {aabb.max.x, aabb.min.y, aabb.min.z},
		{aabb.max.x, aabb.max.y, aabb.min.z}, {aabb.min.x, aabb.max.y, aabb.min.z},
		{aabb.min.x, aabb.min.y, aabb.max.z}, {aabb.max.x, aabb.min.y, aabb.max.z},
		{aabb.max.x, aabb.max.y, aabb.max.z}, {aabb.min.x, aabb.max.y, aabb.max.z}
	};

	// bottom face
	uf::debug::drawLine( corners[0], corners[1] ); uf::debug::drawLine( corners[1], corners[2] );
	uf::debug::drawLine( corners[2], corners[3] ); uf::debug::drawLine( corners[3], corners[0] );
	// top face
	uf::debug::drawLine( corners[4], corners[5] ); uf::debug::drawLine( corners[5], corners[6] );
	uf::debug::drawLine( corners[6], corners[7] ); uf::debug::drawLine( corners[7], corners[4] );
	// vertical edges
	uf::debug::drawLine( corners[0], corners[4] ); uf::debug::drawLine( corners[1], corners[5] );
	uf::debug::drawLine( corners[2], corners[6] ); uf::debug::drawLine( corners[3], corners[7] );
}
void uf::debug::drawShape( const pod::OBB& obb, const pod::Transform<>& transform ) {
	auto aabb = pod::AABB{
		.min = obb.center - obb.extent,
		.max = obb.center + obb.extent,
	};
	pod::Vector3f corners[8] = {
		{aabb.min.x, aabb.min.y, aabb.min.z}, {aabb.max.x, aabb.min.y, aabb.min.z},
		{aabb.max.x, aabb.max.y, aabb.min.z}, {aabb.min.x, aabb.max.y, aabb.min.z},
		{aabb.min.x, aabb.min.y, aabb.max.z}, {aabb.max.x, aabb.min.y, aabb.max.z},
		{aabb.max.x, aabb.max.y, aabb.max.z}, {aabb.min.x, aabb.max.y, aabb.max.z}
	};

	FOR_EACH( 8, {
		corners[i] = uf::transform::apply(transform, corners[i]);
	});

	// bottom face
	uf::debug::drawLine( corners[0], corners[1] ); uf::debug::drawLine( corners[1], corners[2] );
	uf::debug::drawLine( corners[2], corners[3] ); uf::debug::drawLine( corners[3], corners[0] );
	// top face
	uf::debug::drawLine( corners[4], corners[5] ); uf::debug::drawLine( corners[5], corners[6] );
	uf::debug::drawLine( corners[6], corners[7] ); uf::debug::drawLine( corners[7], corners[4] );
	// vertical edges
	uf::debug::drawLine( corners[0], corners[4] ); uf::debug::drawLine( corners[1], corners[5] );
	uf::debug::drawLine( corners[2], corners[6] ); uf::debug::drawLine( corners[3], corners[7] );
}

void uf::debug::drawShape( const pod::Sphere& sphere, const pod::Transform<>& transform ) {
	const int segments = 16;
	const float angleIncrement = (2.0f * M_PI) / segments;
	for ( auto i = 0; i < segments; ++i ) {
		float theta1 = i * angleIncrement;
		float theta2 = (i + 1) * angleIncrement;

		float c1 = std::cos(theta1) * sphere.radius;
		float s1 = std::sin(theta1) * sphere.radius;
		float c2 = std::cos(theta2) * sphere.radius;
		float s2 = std::sin(theta2) * sphere.radius;

		pod::Vector3f xy1 = uf::quaternion::rotate(transform.orientation, pod::Vector3f{c1, s1, 0.0f});
		pod::Vector3f xy2 = uf::quaternion::rotate(transform.orientation, pod::Vector3f{c2, s2, 0.0f});

		pod::Vector3f xz1 = uf::quaternion::rotate(transform.orientation, pod::Vector3f{c1, 0.0f, s1});
		pod::Vector3f xz2 = uf::quaternion::rotate(transform.orientation, pod::Vector3f{c2, 0.0f, s2});

		pod::Vector3f yz1 = uf::quaternion::rotate(transform.orientation, pod::Vector3f{0.0f, c1, s1});
		pod::Vector3f yz2 = uf::quaternion::rotate(transform.orientation, pod::Vector3f{0.0f, c2, s2});

		uf::debug::drawLine( transform.position + xy1, transform.position + xy2 );
		uf::debug::drawLine( transform.position + xz1, transform.position + xz2 );
		uf::debug::drawLine( transform.position + yz1, transform.position + yz2 );
	}
}

void uf::debug::drawShape( const pod::Capsule& capsule, const pod::Transform<>& transform ) {
	const pod::Vector3f up = uf::quaternion::rotate( transform.orientation, capsule.up );
	auto p1 = transform.position + up;
	auto p2 = transform.position - up;

	const int segments = 16;
	const float angleIncrement = (2.0f * M_PI) / segments;

	for ( auto i = 0; i < segments; ++i ) {
		float theta1 = i * angleIncrement;
		float theta2 = (i + 1) * angleIncrement;

		float c1 = std::cos(theta1) * capsule.radius;
		float s1 = std::sin(theta1) * capsule.radius;
		float c2 = std::cos(theta2) * capsule.radius;
		float s2 = std::sin(theta2) * capsule.radius;

		pod::Vector3f xy1 = uf::quaternion::rotate(transform.orientation, pod::Vector3f{c1, s1, 0.0f});
		pod::Vector3f xy2 = uf::quaternion::rotate(transform.orientation, pod::Vector3f{c2, s2, 0.0f});

		pod::Vector3f xz1 = uf::quaternion::rotate(transform.orientation, pod::Vector3f{c1, 0.0f, s1});
		pod::Vector3f xz2 = uf::quaternion::rotate(transform.orientation, pod::Vector3f{c2, 0.0f, s2});

		pod::Vector3f yz1 = uf::quaternion::rotate(transform.orientation, pod::Vector3f{0.0f, c1, s1});
		pod::Vector3f yz2 = uf::quaternion::rotate(transform.orientation, pod::Vector3f{0.0f, c2, s2});

		uf::debug::drawLine( p1 + xy1, p1 + xy2 );
		uf::debug::drawLine( p1 + xz1, p1 + xz2 );
		uf::debug::drawLine( p1 + yz1, p1 + yz2 );

		uf::debug::drawLine( p2 + xy1, p2 + xy2 );
		uf::debug::drawLine( p2 + xz1, p2 + xz2 );
		uf::debug::drawLine( p2 + yz1, p2 + yz2 );
	}

	pod::Vector3f rx = uf::quaternion::rotate(transform.orientation, pod::Vector3f{capsule.radius, 0.0f, 0.0f});
	pod::Vector3f ry = uf::quaternion::rotate(transform.orientation, pod::Vector3f{0.0f, capsule.radius, 0.0f});
	pod::Vector3f rz = uf::quaternion::rotate(transform.orientation, pod::Vector3f{0.0f, 0.0f, capsule.radius});

	uf::debug::drawLine( p1 + rx, p2 + rx );
	uf::debug::drawLine( p1 - rx, p2 - rx );

	uf::debug::drawLine( p1 + ry, p2 + ry );
	uf::debug::drawLine( p1 - ry, p2 - ry );

	uf::debug::drawLine( p1 + rz, p2 + rz );
	uf::debug::drawLine( p1 - rz, p2 - rz );
}
// to-do: properly implement this
void uf::debug::drawShape( const pod::Plane& plane, const pod::Transform<>& transform ) {
	pod::Vector3f right = uf::quaternion::rotate(transform.orientation, pod::Vector3f{1, 0, 0});
	pod::Vector3f forward = uf::quaternion::rotate(transform.orientation, pod::Vector3f{0, 0, 1});

	float size = 10.0f;
	pod::Vector3f p0 = transform.position + (right * size) + (forward * size);
	pod::Vector3f p1 = transform.position - (right * size) + (forward * size);
	pod::Vector3f p2 = transform.position - (right * size) - (forward * size);
	pod::Vector3f p3 = transform.position + (right * size) - (forward * size);

	uf::debug::drawLine( p0, p1 );
	uf::debug::drawLine( p1, p2 );
	uf::debug::drawLine( p2, p3 );
	uf::debug::drawLine( p3, p0 );

	uf::debug::drawLine( p0, p2 );
	uf::debug::drawLine( p1, p3 );
}
void uf::debug::drawShape( const pod::Triangle& tri, const pod::Transform<>& transform ) {
	pod::Vector3f v0 = uf::transform::apply(transform, tri.points[0]);
	pod::Vector3f v1 = uf::transform::apply(transform, tri.points[1]);
	pod::Vector3f v2 = uf::transform::apply(transform, tri.points[2]);

	uf::debug::drawLine( v0, v1 );
	uf::debug::drawLine( v1, v2 );
	uf::debug::drawLine( v2, v0 );
}
void uf::debug::addLine( const pod::Vector3f& start, const pod::Vector3f& end, const pod::Vector4f& color, float ttl ) {
	impl::transientLines.emplace_back(impl::Line{ start, end, color, ttl });
}

void uf::debug::drawLines( float dt ) {
	for ( auto i = 0; i < impl::transientLines.size(); ) {
		auto& line = impl::transientLines[i];
		if ( line.ttl <= 0 ) {
			line = impl::transientLines.back();
			impl::transientLines.pop_back();
		} else {
			uf::debug::drawLine( line.start, line.end, line.color * pod::Vector4f{ 1, 1, 1, CLAMP( line.ttl, 0, 1 ) } );
			line.ttl -= dt * impl::decayRate;
			++i;
		}
	}

	STATIC_THREAD_LOCAL(uf::stl::vector<impl::Vertex>, lines);
	std::swap( impl::lines, lines );

	impl::lineMesh.clear();
	impl::lineMesh.bind<impl::Vertex>();

	// to-do: only do this when the previous mesh already had lines in it
	// and if it was already empty just return
	if ( lines.empty() ) {
		lines.emplace_back( impl::Vertex{} );
		lines.emplace_back( impl::Vertex{} );
	}

	impl::lineMesh.insertVertices<impl::Vertex>(lines);
	impl::lineMesh.generateIndirect();

	auto& scene = uf::scene::getCurrentScene();
	auto& graphics = scene.getComponent<uf::renderer::Graphics>();
	auto& graphic = graphics["immediate:lines"];

	if ( !graphic.initialized ) {
		graphic.device = &uf::renderer::device;
		graphic.staged = false;
		graphic.material.device = &uf::renderer::device;

		// to-do: bin by descriptor instead of one global set
		graphic.descriptor.depth.test = uf::physics::settings.debugDraw.depthTest;
		graphic.descriptor.depth.write = false;
		graphic.descriptor.renderTarget = 1; // "forward";
		graphic.descriptor.topology = uf::renderer::enums::PrimitiveTopology::LINE_LIST;
		graphic.descriptor.fill = uf::renderer::enums::PolygonMode::LINE;
		graphic.descriptor.lineWidth = 2;

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
		graphic.initializeMesh( impl::lineMesh );
	} else {
		bool rebuild = graphic.updateMesh( impl::lineMesh );
		if ( rebuild ) uf::renderer::states::rebuild = true; // to-do: rebuild the defer mode only
	}
}

void uf::debug::drawText( const uf::stl::string& string, const pod::Vector3f& position, const pod::Vector4f& color ) {
	impl::texts.emplace_back( impl::Text{ string, position, color } );
}

void uf::debug::drawTexts( float dt ) {
	STATIC_THREAD_LOCAL(uf::stl::vector<impl::Text>, texts);
	std::swap( impl::texts, texts );
	
	auto& scene = uf::scene::getCurrentScene();
	auto& controller = scene.getController();
	auto& camera = scene.getCamera( controller );
	auto transform = uf::transform::flatten( camera.getTransform() );

	uf::stl::vector<pod::GlyphBox> textLayout;
	// pre-init with ASCII characters
	if ( !impl::textAtlas.generated() || texts.empty() ) {
		uf::stl::string ascii = "";
		for ( char c = 32; c < 127; ++c ) ascii += c;
		auto tokens = uf::glyph::parseTextTokens( ascii, {0,0,0,0} );
		auto layout = uf::glyph::calculateLayout( tokens, impl::textSettings );
		for ( auto& g : layout ) {
			g.box.x = 0;
			g.box.y = 0;
			g.box.w = 0;
			g.box.h = 0;

			textLayout.emplace_back( g );
		}
	}

	for ( auto& text : texts ) {
		auto tokens = uf::glyph::parseTextTokens( text.string, text.color );
		auto layout = uf::glyph::calculateLayout( tokens, impl::textSettings );
		for ( auto& g : layout ) {
			g.anchor += text.position;
			textLayout.emplace_back( g );
		}
	}

	bool dirty = uf::glyph::generateAtlas( textLayout, impl::textSettings, impl::textAtlas );
	uf::glyph::generateMesh( textLayout, impl::textSettings, impl::textAtlas, impl::textMesh );
	impl::textMesh.generateIndirect();

	auto& graphics = scene.getComponent<uf::renderer::Graphics>();	
	auto& graphic = graphics["immediate:texts"];
	if ( !graphic.initialized ) {
		graphic.device = &uf::renderer::device;
		graphic.staged = false;
		graphic.material.device = &uf::renderer::device;

		graphic.descriptor.depth.test = uf::physics::settings.debugDraw.depthTest;
		graphic.descriptor.depth.write = false;
		graphic.descriptor.renderTarget = 1; // "forward";
		graphic.descriptor.blend.enabled = true;
		graphic.descriptor.cullMode = uf::renderer::enums::CullMode::NONE;

		uf::stl::string vertexShaderFilename = uf::io::resolveURI(uf::io::root+"/shaders/base/textured/vert.spv");
		uf::stl::string fragmentShaderFilename = uf::io::resolveURI(uf::io::root+"/shaders/base/textured/frag.spv");

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
		// fragment shader
		auto& texture = graphic.material.textures.emplace_back();
		texture.loadFromImage( impl::textAtlas.getAtlas() );

		graphic.initialize();
		graphic.initializeMesh( impl::textMesh );
	} else {
		if ( dirty ) {
			graphic.material.textures.clear();
			
			auto& texture = graphic.material.textures.emplace_back();
			texture.loadFromImage( impl::textAtlas.getAtlas() );
		}

		bool rebuild = graphic.updateMesh( impl::textMesh );
		if ( rebuild ) uf::renderer::states::rebuild = true; // to-do: rebuild the defer mode only
	}
}
void uf::debug::draw( float dt ) {
	uf::debug::drawLines( dt );
	uf::debug::drawTexts( dt );
}