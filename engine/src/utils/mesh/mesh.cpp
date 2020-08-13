#include <uf/utils/mesh/mesh.h>
/*
uf::Graphic::~Graphic() {
	this->destroy();
}
void uf::Graphic::destroy( bool clear ) {
}
*/
// Used for terrain
std::vector<ext::vulkan::Graphic::VertexDescriptor> pod::Vertex_3F2F3F32B::descriptor = {
	{
		VK_FORMAT_R32G32B32_SFLOAT,
		offsetof(pod::Vertex_3F2F3F32B, position)
	},
	{
		VK_FORMAT_R32G32_SFLOAT,
		offsetof(pod::Vertex_3F2F3F32B, uv)
	},
	{
		VK_FORMAT_R32G32B32_SFLOAT,
		offsetof(pod::Vertex_3F2F3F32B, normal)
	},
	{
		VK_FORMAT_R32_UINT,
		offsetof(pod::Vertex_3F2F3F32B, color)
	}
};
// Used for normal meshses
std::vector<ext::vulkan::Graphic::VertexDescriptor> pod::Vertex_3F2F3F::descriptor = {
	{
		VK_FORMAT_R32G32B32_SFLOAT,
		offsetof(pod::Vertex_3F2F3F, position)
	},
	{
		VK_FORMAT_R32G32_SFLOAT,
		offsetof(pod::Vertex_3F2F3F, uv)
	},
	{
		VK_FORMAT_R32G32B32_SFLOAT,
		offsetof(pod::Vertex_3F2F3F, normal)
	}
};
// (Typically) used for displaying textures
std::vector<ext::vulkan::Graphic::VertexDescriptor> pod::Vertex_3F2F::descriptor = {
	{
		VK_FORMAT_R32G32B32_SFLOAT,
		offsetof(pod::Vertex_3F2F, position)
	},
	{
		VK_FORMAT_R32G32_SFLOAT,
		offsetof(pod::Vertex_3F2F, uv)
	}
};
// Basic
std::vector<ext::vulkan::Graphic::VertexDescriptor> pod::Vertex_3F::descriptor = {
	{
		VK_FORMAT_R32G32B32_SFLOAT,
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

	ext::vulkan::MeshGraphic* graphic = new ext::vulkan::BaseGraphic;
	this->graphic = (void*) graphic;

	graphic->device = &ext::vulkan::device;
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
		ext::vulkan::MeshGraphic* graphic = (ext::vulkan::MeshGraphic*) this->graphic;
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

	ext::vulkan::GuiGraphic* graphic = new ext::vulkan::GuiGraphic;
	this->graphic = (void*) graphic;

	graphic->device = &ext::vulkan::device;
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
		ext::vulkan::GuiGraphic* graphic = (ext::vulkan::GuiGraphic*) this->graphic;
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