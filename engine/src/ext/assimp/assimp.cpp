
#include <uf/config.h>
#if defined(UF_USE_ASSIMP)

#include <uf/ext/assimp/assimp.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <uf/utils/io/iostream.h>
#include <uf/utils/math/matrix.h>

UF_API bool ext::assimp::load( const std::string& filename, uf::Mesh& mesh, bool flip ) {
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile( filename, aiProcess_Triangulate );

	if( !scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode ) {
		uf::iostream << "Failed to load model @ `" << filename << "`" << "\n";
		return false;
	}

	struct {
		std::vector<float> points;
		std::vector<float> normals;
		std::vector<float> uvs;
	} model;

	std::function<void(const aiNode*)> readNodes = [&]( const aiNode* node ) {
		for ( uint m = 0; m < node->mNumMeshes; ++m ) {
			const aiMesh* assMesh = scene->mMeshes[node->mMeshes[m]];
			for ( uint i = 0; i < assMesh->mNumVertices; i++ ) {
				if ( assMesh->HasPositions() ) {
					const aiVector3D* vp = &(assMesh->mVertices[i]);
					model.points.push_back((float) vp->x);
					model.points.push_back((float) vp->y);
					model.points.push_back((float) vp->z);
				
					if ( flip ) {
						uint offset = model.points.size();
						std::swap( model.points[offset-1], model.points[offset-2] );
						model.points[offset-1] *= -1;
					}

					if ( assMesh->HasNormals() ) {
						const aiVector3D* vn = &(assMesh->mNormals[i]);
						model.normals.push_back((float) vn->x);
						model.normals.push_back((float) vn->y);
						model.normals.push_back((float) vn->z);
						if ( flip ) {
							uint offset = model.normals.size();
							std::swap( model.normals[offset-1], model.normals[offset-2] );
							model.normals[offset-1] *= -1;
						}
					} else {
						model.normals.push_back(0);	 model.normals.push_back(0); model.normals.push_back(0);	
					}
					if ( assMesh->HasTextureCoords(0) ) {
						const aiVector3D* vt = &(assMesh->mTextureCoords[0][i]);
						model.uvs.push_back((float) vt->x);
						model.uvs.push_back((float) vt->y);
					} else {
						model.uvs.push_back(0);	model.uvs.push_back(0);
					}
				}
			}
		}
		for ( uint i = 0; i < node->mNumChildren; ++i ) readNodes(node->mChildren[i]);
	}; readNodes(scene->mRootNode);

	/* Load */ {
		if ( !model.points.empty() ) mesh.getPositions().add( &model.points[0], model.points.size() );
		if ( !model.normals.empty() ) mesh.getNormals().add( &model.normals[0], model.normals.size() );
		if ( !model.uvs.empty() ) mesh.getUvs().add( &model.uvs[0], model.uvs.size() );
	}

	return true;
}
#include <iomanip>
UF_API bool ext::assimp::load( const std::string& filename, uf::SkeletalMesh& mesh, bool flip ) {
	Assimp::Importer& importer = mesh.getImporter();
	mesh.setScene( importer.ReadFile( filename, aiProcess_Triangulate | aiProcess_GenSmoothNormals ));
	const aiScene* scene = mesh.getScene();

	if( !scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode ) {
		uf::iostream << "Failed to load model @ `" << filename << "`" << "\n";
		return false;
	}

	std::function<pod::Matrix4(const aiMatrix4x4&, bool)> convertMatrix = [&]( const aiMatrix4x4& m, bool t = false) {
		return t ? uf::matrix::initialize({	
			m.a1, m.b1, m.c1, m.d1,
			m.a2, m.b2, m.c2, m.d2,
			m.a3, m.b3, m.c3, m.d3,
			m.a4, m.b4, m.c4, m.d4
		}) : uf::matrix::initialize({	
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			m.a4, m.b4, m.c4, m.d4
		});
	};

	uf::Skeleton& skel = mesh.getSkeleton();
	uf::Skeleton::container_t& skeleton = skel.getBones();

	struct {
		std::vector<float> points;
		std::vector<float> normals;
		std::vector<float> uvs;
		std::vector<int> bones_id;
		std::vector<float> bones_weight;
	} model;
	/* */ {
		std::function<bool(const aiNode*)> readNodes = [&]( const aiNode* node ) {
			int startIndex = model.points.size() / 3;
			for ( uint m = 0; m < node->mNumMeshes; ++m ) {
				const aiMesh* assMesh = scene->mMeshes[node->mMeshes[m]];
				if ( false && assMesh->HasBones()) {
					const int WEIGHTS_PER_VERTEX = 4;
					int size = assMesh->mNumVertices * WEIGHTS_PER_VERTEX;
					std::vector<int> boneIDs; boneIDs.resize(size, 0);
					std::vector<float> boneWeights; boneWeights.resize(size, 0);
					for ( uint i = 0; i < assMesh->mNumBones; ++i ) {
						const aiBone* bone = assMesh->mBones[i]; 
						const aiMatrix4x4& m = bone->mOffsetMatrix;

						uint index = i;
						std::string name = bone->mName.data;
	  					pod::Matrix4 matrix = convertMatrix(m, true);
	  					if ( !skel.hasBone( name ) ) {
	  						index = skeleton.size();
							uf::Skeleton::Bone b;
							b.index = index;
							b.name = name;
							b.matrix = uf::matrix::identity();
							b.animation = uf::matrix::identity();
							b.offset = matrix;
							b.inverse = matrix; b.inverse = uf::matrix::inverse( b.inverse );
							b.parent = 0;
							skeleton.push_back(b);
	  					} else {
	  						index = skel.getBone( name ).index;
	  					}
						for ( uint j = 0; j < bone->mNumWeights; ++j) {
							aiVertexWeight weight = bone->mWeights[j];
							int start = weight.mVertexId * WEIGHTS_PER_VERTEX;
							int lowest = -1;
							for ( int k = 0; k < WEIGHTS_PER_VERTEX; ++k ) { int offset = start + k;
								if ( boneWeights.at(offset) == 0.0 ) {
									boneWeights.at(offset) = weight.mWeight;
									boneIDs.at(offset) = index;
									break;
								}
							//	if ( boneWeights.at(offset) < weight.mWeight ) lowest = k;
							}
							if ( lowest >= 0 ) {
								boneWeights.at(start + lowest) = weight.mWeight;
								boneIDs.at(start + lowest) = index;
							}
						}
					}
					for ( auto id : boneIDs ) model.bones_id.push_back(id);
					for ( auto weight : boneWeights ) model.bones_weight.push_back(weight);
				}
				for ( uint i = 0; i < assMesh->mNumVertices; i++ ) {
					if ( assMesh->HasPositions() ) {
						const aiVector3D* vp = &(assMesh->mVertices[i]);
						model.points.push_back((float) vp->x);
						model.points.push_back((float) vp->y);
						model.points.push_back((float) vp->z);
					
						if ( flip ) {
							uint offset = model.points.size();
							std::swap( model.points[offset-1], model.points[offset-2] );
							model.points[offset-1] *= -1;
						}

						if ( assMesh->HasNormals() ) {
							const aiVector3D* vn = &(assMesh->mNormals[i]);
							model.normals.push_back((float) vn->x);
							model.normals.push_back((float) vn->y);
							model.normals.push_back((float) vn->z);
							if ( flip ) {
								uint offset = model.normals.size();
								std::swap( model.normals[offset-1], model.normals[offset-2] );
								model.normals[offset-1] *= -1;
							}
						} else {
							model.normals.push_back(0);	 model.normals.push_back(0); model.normals.push_back(0);	
						}
						if ( assMesh->HasTextureCoords(0) ) {
							const aiVector3D* vt = &(assMesh->mTextureCoords[0][i]);
							model.uvs.push_back((float) vt->x);
							model.uvs.push_back((float) vt->y);
						} else {
							model.uvs.push_back(0);	model.uvs.push_back(0);
						}
					}
				}
				if ( true && assMesh->HasBones()) {
					const int WEIGHTS_PER_VERTEX = 4;
					int size = assMesh->mNumVertices * WEIGHTS_PER_VERTEX;
					model.bones_id.resize( model.bones_id.size() + size, 0 );
					model.bones_weight.resize( model.bones_weight.size() + size, 0 );
					for ( uint i = 0; i < assMesh->mNumBones; ++i ) {
						const aiBone* bone = assMesh->mBones[i]; 
						const aiMatrix4x4& m = bone->mOffsetMatrix;

						uint index = i;
						std::string name = bone->mName.data;
	  					pod::Matrix4 matrix = convertMatrix(m, true);
	  					if ( !skel.hasBone( name ) ) {
							index = skeleton.size();
							uf::Skeleton::Bone b;
							b.index = index;
							b.name = name;
							b.matrix = uf::matrix::identity();
							b.animation = uf::matrix::identity();
							b.offset = matrix;
							b.inverse = matrix; b.inverse = uf::matrix::inverse( b.inverse );
							b.parent = 0;
							skeleton.push_back(b);
	  					} else {
	  						index = skel.getBone( name ).index;
	  					}
						for ( uint j = 0; j < bone->mNumWeights; ++j) {
							aiVertexWeight weight = bone->mWeights[j];
							int lowest = -1;
							int start = startIndex + weight.mVertexId * WEIGHTS_PER_VERTEX;
							for ( int k = 0; k < WEIGHTS_PER_VERTEX; ++k ) { int offset = start + k;
								if ( model.bones_weight.at(offset) == 0 ) {
									model.bones_weight.at(offset) = weight.mWeight;
									model.bones_id.at(offset) = index;
									break;
								}
							//	if ( model.bones_weight.at(offset) < weight.mWeight ) lowest = k;
							}
							if ( lowest >= 0 ) {
								model.bones_weight.at(start + lowest) = weight.mWeight;
								model.bones_id.at(start + lowest) = index;
							}
						}
					}
				}
			}
			for ( uint i = 0; i < node->mNumChildren; ++i ) if (!readNodes(node->mChildren[i])) return false;
			return true;
		}; if ( !readNodes(scene->mRootNode) ) return false;
	}

	/* Load */ {
		if ( !model.points.empty() ) mesh.getPositions().add( &model.points[0], model.points.size() );
		if ( !model.normals.empty() ) mesh.getNormals().add( &model.normals[0], model.normals.size() );
		if ( !model.uvs.empty() ) mesh.getUvs().add( &model.uvs[0], model.uvs.size() );
		if ( !model.bones_id.empty() ) {
			mesh.getBonesId().add( &model.bones_id[0], model.bones_id.size() );
			mesh.getBonesWeight().add( &model.bones_weight[0], model.bones_weight.size() );
			mesh.getSkeleton().getBones() = skeleton; if ( true ) {
				std::vector<const aiNode*> nodes; {
					std::function<void(const aiNode*)> fillNodes = [&]( const aiNode* node ) {
						nodes.push_back(node); for ( uint i = 0; i < node->mNumChildren; ++i ) fillNodes(node->mChildren[i]);
					}; fillNodes(scene->mRootNode);
				}
				std::function<const aiNode*(const std::string&)> nameToBoneNode = [&]( const std::string& name ) {
					for ( const aiNode* node : nodes ) if ( node->mName.data == name ) return node;
					return (const aiNode*) NULL;
				};

				auto& skeleton = mesh.getSkeleton();
				for ( auto& bone : skeleton.getBones() ) {
					const aiNode* node = nameToBoneNode(bone.name);
					if ( node && node->mParent && skeleton.hasBone( node->mParent->mName.data ) ) {
						bone.parent = skeleton.getBone(node->mParent->mName.data).index;
						skeleton.getBone(node->mParent->mName.data).children.push_back(bone.index);
					} else bone.parent = bone.index;
				}
				if ( false ) { std::cout << "Map of " << filename << std::endl;
					std::function<void(const uf::Skeleton::Bone&, int)> recurseChild = [&]( const uf::Skeleton::Bone& bone, int indent ) {
						for ( int i = 0; i < indent; ++i ) std::cout << "\t";
						std::cout << bone.name << "\n";
						for ( auto child : bone.children ) recurseChild( skeleton.getBone(child), indent+1 );
					};
					for ( auto& bone : skeleton.getBones() ) {
						if ( bone.index != bone.parent ) continue;
						recurseChild( bone, 0 );
					}
					std::cout << std::endl;
				}
			}
		}
	}

	return true;
}

#endif