#pragma once

#include <uf/utils/mesh/mesh.h>

namespace pod {
	struct /*UF_API*/ Vertex_3F2F3F4F {
		/*alignas(16)*/ pod::Vector3f position;
		/*alignas(8)*/ pod::Vector2f uv;
		/*alignas(16)*/ pod::Vector3f normal;
		/*alignas(16)*/ pod::Vector4f color;

		static UF_API std::vector<uf::renderer::VertexDescriptor> descriptor;
	};
	struct /*UF_API*/ Vertex_3F2F3F32B {
		/*alignas(16)*/ pod::Vector3f position;
		/*alignas(8)*/ pod::Vector2f uv;
		/*alignas(16)*/ pod::Vector3f normal;
		/*alignas(16)*/ pod::Vector4t<uint8_t> color;

		static UF_API std::vector<uf::renderer::VertexDescriptor> descriptor;
	};
	struct /*UF_API*/ Vertex_3F3F3F {
		/*alignas(16)*/ pod::Vector3f position;
		/*alignas(16)*/ pod::Vector3f uv;
		/*alignas(16)*/ pod::Vector3f normal;

		static UF_API std::vector<uf::renderer::VertexDescriptor> descriptor;
	};
	struct /*UF_API*/ Vertex_3F2F3F1UI {
		/*alignas(16)*/ pod::Vector3f position;
		/*alignas(8)*/ pod::Vector2f uv;
		/*alignas(16)*/ pod::Vector3f normal;
		/*alignas(4)*/ pod::Vector1ui id;

		static UF_API std::vector<uf::renderer::VertexDescriptor> descriptor;
	};
	struct /*UF_API*/ Vertex_3F2F3F {
		/*alignas(16)*/ pod::Vector3f position;
		/*alignas(8)*/ pod::Vector2f uv;
		/*alignas(16)*/ pod::Vector3f normal;

		static UF_API std::vector<uf::renderer::VertexDescriptor> descriptor;
	};
	struct /*UF_API*/ Vertex_3F2F {
		/*alignas(16)*/ pod::Vector3f position;
		/*alignas(8)*/ pod::Vector2f uv;

		static UF_API std::vector<uf::renderer::VertexDescriptor> descriptor;
	};
	struct /*UF_API*/ Vertex_2F2F {
		/*alignas(8)*/ pod::Vector2f position;
		/*alignas(8)*/ pod::Vector2f uv;

		static UF_API std::vector<uf::renderer::VertexDescriptor> descriptor;
	};
	struct /*UF_API*/ Vertex_3F {
		/*alignas(16)*/ pod::Vector3f position;

		static UF_API std::vector<uf::renderer::VertexDescriptor> descriptor;
	};
}

namespace uf {
	typedef BaseMesh<pod::Vertex_3F2F3F32B> ColoredMesh;
	typedef BaseMesh<pod::Vertex_3F2F3F> Mesh;
	typedef BaseMesh<pod::Vertex_3F> LineMesh;
#if 0
	template<size_t N = 6>
	struct MeshDescriptor {
		struct {
			/*alignas(16)*/ pod::Matrix4f model;
			/*alignas(16)*/ pod::Matrix4f view[N];
			/*alignas(16)*/ pod::Matrix4f projection[N];
		} matrices;
		/*alignas(16)*/ pod::Vector4f color = { 1, 1, 1, 0 };
	};
#endif
}