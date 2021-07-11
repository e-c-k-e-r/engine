// Doesn't have a home

namespace pod {
	struct UF_API SceneTextures {
		uf::renderer::Texture3D noise;
		uf::renderer::TextureCube skybox;
		struct {
			uf::stl::vector<uf::renderer::Texture3D> id;
			uf::stl::vector<uf::renderer::Texture3D> uv;
			uf::stl::vector<uf::renderer::Texture3D> normal;
			uf::stl::vector<uf::renderer::Texture3D> radiance;
			uf::stl::vector<uf::renderer::Texture3D> depth;
		} voxels;
	};
}