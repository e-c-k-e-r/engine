#include <uf/utils/graphic/mesh.h>

// Used for per-vertex colors
std::vector<uf::renderer::VertexDescriptor> pod::Vertex_3F2F3F4F::descriptor = {
	{
		uf::renderer::enums::Format::R32G32B32_SFLOAT,
		offsetof(pod::Vertex_3F2F3F4F, position)
	},
	{
		uf::renderer::enums::Format::R32G32_SFLOAT,
		offsetof(pod::Vertex_3F2F3F4F, uv)
	},
	{
		uf::renderer::enums::Format::R32G32B32_SFLOAT,
		offsetof(pod::Vertex_3F2F3F4F, normal)
	},
	{
		uf::renderer::enums::Format::R32_UINT,
		offsetof(pod::Vertex_3F2F3F4F, color)
	}
};
// Used for terrain
std::vector<uf::renderer::VertexDescriptor> pod::Vertex_3F2F3F32B::descriptor = {
	{
		uf::renderer::enums::Format::R32G32B32_SFLOAT,
		offsetof(pod::Vertex_3F2F3F32B, position)
	},
	{
		uf::renderer::enums::Format::R32G32_SFLOAT,
		offsetof(pod::Vertex_3F2F3F32B, uv)
	},
	{
		uf::renderer::enums::Format::R32G32B32_SFLOAT,
		offsetof(pod::Vertex_3F2F3F32B, normal)
	},
	{
		uf::renderer::enums::Format::R32_UINT,
		offsetof(pod::Vertex_3F2F3F32B, color)
	}
};
// Used for normal meshses
std::vector<uf::renderer::VertexDescriptor> pod::Vertex_3F2F3F::descriptor = {
	{
		uf::renderer::enums::Format::R32G32B32_SFLOAT,
		offsetof(pod::Vertex_3F2F3F, position)
	},
	{
		uf::renderer::enums::Format::R32G32_SFLOAT,
		offsetof(pod::Vertex_3F2F3F, uv)
	},
	{
		uf::renderer::enums::Format::R32G32B32_SFLOAT,
		offsetof(pod::Vertex_3F2F3F, normal)
	}
};
// (Typically) used for displaying textures
std::vector<uf::renderer::VertexDescriptor> pod::Vertex_3F2F::descriptor = {
	{
		uf::renderer::enums::Format::R32G32B32_SFLOAT,
		offsetof(pod::Vertex_3F2F, position)
	},
	{
		uf::renderer::enums::Format::R32G32_SFLOAT,
		offsetof(pod::Vertex_3F2F, uv)
	}
};
std::vector<uf::renderer::VertexDescriptor> pod::Vertex_2F2F::descriptor = {
	{
		uf::renderer::enums::Format::R32G32B32_SFLOAT,
		offsetof(pod::Vertex_2F2F, position)
	},
	{
		uf::renderer::enums::Format::R32G32_SFLOAT,
		offsetof(pod::Vertex_2F2F, uv)
	}
};
// used for texture arrays
std::vector<uf::renderer::VertexDescriptor> pod::Vertex_3F3F3F::descriptor = {
	{
		uf::renderer::enums::Format::R32G32B32_SFLOAT,
		offsetof(pod::Vertex_3F3F3F, position)
	},
	{
		uf::renderer::enums::Format::R32G32B32_SFLOAT,
		offsetof(pod::Vertex_3F3F3F, uv)
	},
	{
		uf::renderer::enums::Format::R32G32B32_SFLOAT,
		offsetof(pod::Vertex_3F3F3F, normal)
	}
};
std::vector<uf::renderer::VertexDescriptor> pod::Vertex_3F2F3F1UI::descriptor = {
	{
		uf::renderer::enums::Format::R32G32B32_SFLOAT,
		offsetof(pod::Vertex_3F2F3F1UI, position)
	},
	{
		uf::renderer::enums::Format::R32G32_SFLOAT,
		offsetof(pod::Vertex_3F2F3F1UI, uv)
	},
	{
		uf::renderer::enums::Format::R32G32B32_SFLOAT,
		offsetof(pod::Vertex_3F2F3F1UI, normal)
	},
	{
		uf::renderer::enums::Format::R32_UINT,
		offsetof(pod::Vertex_3F2F3F1UI, id)
	}
};
// Basic
std::vector<uf::renderer::VertexDescriptor> pod::Vertex_3F::descriptor = {
	{
		uf::renderer::enums::Format::R32G32B32_SFLOAT,
		offsetof(pod::Vertex_3F, position)
	}
};
/*
#include <uf/ext/vulkan/graphics/mesh.h>
#include <uf/ext/vulkan/graphics/gui.h>
#include <uf/ext/vulkan/vulkan.h>

#include <unordered_map>

void uf::Mesh::initialize( bool compress ) {
	if ( this->graphic ) this->destroy(false);
	if ( compress ) {
		std::unordered_map<vertex_t, uint32_t> unique;
		std::vector<vertex_t> _vertices = std::move( this->vertices );
		
		this->indices.clear();
		this->vertices.clear();
		this->indices.reserve(_vertices.size());
		this->vertices.reserve(_vertices.size());

		for ( vertex_t& vertex : _vertices ) {
			if ( unique.count(vertex) == 0 ) {
				unique[vertex] = static_cast<uint32_t>(this->vertices.size());
				this->vertices.push_back( vertex );
			}
			this->indices.push_back( unique[vertex] );
		}
	} else {
		this->indices.clear();
		this->indices.reserve(vertices.size());
		for ( size_t i = 0; i < vertices.size(); ++i ) {
			this->indices.push_back(i);
		}
	}

	uf::renderer::MeshGraphic* graphic = new uf::renderer::BaseGraphic;
	this->graphic = (void*) graphic;

	graphic->device = &uf::renderer::device;
	// graphic->describe

	graphic->initializeBuffer(
		(void*) vertices.data(),
		vertices.size() * sizeof(vertex_t),
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, //VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		false
	);
	graphic->initializeBuffer(
		(void*) indices.data(),
		indices.size() * sizeof(uint32_t),
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, //VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		false
	);
	graphic->indices = indices.size();
	// wait for shaders and unifors
}
void uf::Mesh::destroy( bool clear ) {
	if ( this->graphic ) {
		uf::renderer::MeshGraphic* graphic = (uf::renderer::MeshGraphic*) this->graphic;
		if ( graphic ) {
			graphic->destroy();
			delete graphic;
			this->graphic = nullptr;
		}
	}
	if ( clear ) {
		this->indices.clear();
		this->vertices.clear();
	}
}
uf::Mesh::~Mesh() {
	this->destroy();
}



void uf::GuiMesh::initialize( bool compress ) {
	if ( this->graphic ) this->destroy(false);
	if ( compress ) {
		std::unordered_map<pod::Vertex2f2f, uint32_t> unique;
		std::vector<pod::Vertex2f2f> _vertices = std::move( this->vertices );
		
		this->indices.clear();
		this->vertices.clear();
		this->indices.reserve(_vertices.size());
		this->vertices.reserve(_vertices.size());

		for ( pod::Vertex2f2f& vertex : _vertices ) {
			if ( unique.count(vertex) == 0 ) {
				unique[vertex] = static_cast<uint32_t>(this->vertices.size());
				this->vertices.push_back( vertex );
			}
			this->indices.push_back( unique[vertex] );
		}
	} else {
		this->indices.clear();
		this->indices.reserve(vertices.size());
		for ( size_t i = 0; i < vertices.size(); ++i ) {
			this->indices.push_back(i);
		}
	}

	uf::renderer::GuiGraphic* graphic = new uf::renderer::GuiGraphic;
	this->graphic = (void*) graphic;

	graphic->device = &uf::renderer::device;
	graphic->initializeBuffer(
		(void*) vertices.data(),
		vertices.size() * sizeof(pod::Vertex2f2f),
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, //VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		false
	);
	graphic->initializeBuffer(
		(void*) indices.data(),
		indices.size() * sizeof(uint32_t),
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, //VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		false
	);
	graphic->indices = indices.size();
	// wait for shaders
}
void uf::GuiMesh::destroy( bool clear ) {
	if ( this->graphic ) {
		uf::renderer::GuiGraphic* graphic = (uf::renderer::GuiGraphic*) this->graphic;
		if ( graphic ) {
			graphic->destroy();
			delete graphic;
			this->graphic = nullptr;
		}
	}
	if ( clear ) {
		this->indices.clear();
		this->vertices.clear();
	}
}
uf::GuiMesh::~GuiMesh() {
	this->destroy();
}

*/