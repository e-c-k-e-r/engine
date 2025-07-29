// Doesn't have a home

namespace pod {
	struct UF_API SceneTextures {
		uf::renderer::Texture3D noise;
		uf::renderer::TextureCube skybox;
		struct {
			uf::stl::vector<uf::renderer::Texture3D> drawId;
			uf::stl::vector<uf::renderer::Texture3D> instanceId;
			uf::stl::vector<uf::renderer::Texture3D> normalX;
			uf::stl::vector<uf::renderer::Texture3D> normalY;
			uf::stl::vector<uf::renderer::Texture3D> radianceR;
			uf::stl::vector<uf::renderer::Texture3D> radianceG;
			uf::stl::vector<uf::renderer::Texture3D> radianceB;
			uf::stl::vector<uf::renderer::Texture3D> radianceA;
			uf::stl::vector<uf::renderer::Texture3D> count;
			uf::stl::vector<uf::renderer::Texture3D> output;
		} voxels;
	};
}