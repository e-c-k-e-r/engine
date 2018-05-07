#include <uf/gl/mesh/skeletal/mesh.h>
#include <algorithm>


uint uf::SkeletalMesh::MAX_BONES = 64;
bool uf::Skeleton::isSorted() {
	uint sequence = 0;
	for ( const auto& bone : this->m_bones ) {
		if ( bone.index <= sequence ) return false;
		sequence = bone.index;
	}
	return true;
}
void uf::Skeleton::sort() {
	std::sort( this->m_bones.begin(), this->m_bones.end(), []( const uf::Skeleton::Bone& left, const uf::Skeleton::Bone& right ) { return left.index < right.index; });
}
bool uf::Skeleton::hasBone( uint index ) const {
	for ( auto& bone : this->m_bones ) if ( bone.index == index ) return true;
	return false;
}
bool uf::Skeleton::hasBone( const std::string& name ) const {
	for ( auto& bone : this->m_bones ) if ( bone.name == name ) return true;
	return false;
}
const uf::Skeleton::Bone& uf::Skeleton::getBone( uint index ) const {
	static uf::Skeleton::Bone null;
	for ( const auto& bone : this->m_bones ) {
		if ( bone.index == index ) return bone;
	}
	return null;
}
const uf::Skeleton::Bone& uf::Skeleton::getBone( const std::string& name ) const {
	static uf::Skeleton::Bone null;
	for ( const auto& bone : this->m_bones ) {
		if ( bone.name == name ) return bone;
	}
	return null;
}
uf::Skeleton::Bone& uf::Skeleton::getBone( uint index ) {
	static uf::Skeleton::Bone null;
	for ( auto& bone : this->m_bones ) {
		if ( bone.index == index ) return bone;
	}
	return null;
}
uf::Skeleton::Bone& uf::Skeleton::getBone( const std::string& name ) {
	static uf::Skeleton::Bone null;
	for ( auto& bone : this->m_bones ) {
		if ( bone.name == name ) return bone;
	}
	return null;
}

uf::Skeleton::container_t& uf::Skeleton::getBones() {
	if ( !this->isSorted() ) this->sort();
	return this->m_bones;
}
const uf::Skeleton::container_t& uf::Skeleton::getBones() const {
	return this->m_bones;
}

void uf::Skeleton::addBone( const uf::Skeleton::Bone& bone ) {
	this->m_bones.push_back( bone );
}

const pod::Matrix4& uf::Skeleton::getGlobalTransform() const {
	return this->m_globalTransform;
}
void uf::Skeleton::setGlobalTransform( const pod::Matrix4& matrix ) {
	this->m_globalTransform = matrix;
}
uf::SkeletalMesh::SkeletalMesh() : Mesh() {
	this->m_importer = new Assimp::Importer;
}

void uf::SkeletalMesh::clear() {
	if ( this->m_importer ) delete this->m_importer;
	this->m_vao.clear();
	this->m_ibo.clear();
	this->m_position.clear();
	this->m_normal.clear();
	this->m_color.clear();
	this->m_uv.clear();
	this->m_bones_id.clear();
	this->m_bones_weight.clear();
}
void uf::SkeletalMesh::destroy() {
	if ( this->m_importer ) delete this->m_importer;
	this->m_vao.destroy();
	this->m_ibo.destroy();
	this->m_position.destroy();
	this->m_normal.destroy();
	this->m_color.destroy();
	this->m_uv.destroy();
	this->m_bones_id.destroy();
	this->m_bones_weight.destroy();
}

// OpenGL ops
void uf::SkeletalMesh::generate() {
	this->m_vao.generate();
	this->m_vao.bind();

	if ( this->m_position.loaded() && !this->m_position.generated() ) this->m_position.generate();
	if ( this->m_uv.loaded() && !this->m_uv.generated() ) this->m_uv.generate();
	if ( this->m_normal.loaded() && !this->m_normal.generated() ) this->m_normal.generate();
	if ( this->m_color.loaded() && !this->m_color.generated() ) this->m_color.generate();
	if ( this->m_bones_id.loaded() && !this->m_bones_id.generated() ) this->m_bones_id.generate();
	if ( this->m_bones_weight.loaded() && !this->m_bones_weight.generated() ) this->m_bones_weight.generate();
	this->bindAttributes();
	
	if ( this->m_indexed ) this->m_ibo.generate();
}
void uf::SkeletalMesh::bindAttributes() {
	GLuint i = 0;
	if ( this->m_position.generated() ) {
		this->m_position.bindAttribute(i);
		this->m_vao.bindAttribute(i);
		++i;
	}
	if ( this->m_uv.generated() ) {
		this->m_uv.bindAttribute(i);
		this->m_vao.bindAttribute(i);
		++i;
	}
	if ( this->m_normal.generated() ) {
		this->m_normal.bindAttribute(i);
		this->m_vao.bindAttribute(i);
		++i;
	}
	if ( this->m_color.generated() ) {
		this->m_color.bindAttribute(i);
		this->m_vao.bindAttribute(i);
		++i;
	}
	if ( this->m_bones_id.generated() ) {
		this->m_bones_id.bindAttribute(i);
		this->m_vao.bindAttribute(i);
		++i;
	}
	if ( this->m_bones_weight.generated() ) {
		this->m_bones_weight.bindAttribute(i);
		this->m_vao.bindAttribute(i);
		++i;
	}
}
// OpenGL uf::SkeletalMesh::Getters
bool uf::SkeletalMesh::loaded() const {
	return 	this->m_position.loaded() ||
			this->m_normal.loaded() ||
			this->m_color.loaded() ||
			this->m_uv.loaded() ||
			this->m_bones_id.loaded() ||
			this->m_bones_weight.loaded();
}

// Indexed ops
void uf::SkeletalMesh::index() {
	if ( this->m_indexed ) return;
	this->m_indexed = true;
	this->m_ibo.index( this->m_position, this->m_normal, this->m_color, this->m_uv, this->m_bones_id, this->m_bones_weight );
}
// Move Setters
void uf::SkeletalMesh::setBonesId( uf::Vertices4i&& bones ) {
	this->m_bones_id = std::move(bones);
}
void uf::SkeletalMesh::setBonesWeight( uf::Vertices4f&& bones ) {
	this->m_bones_weight = std::move(bones);
}
void uf::SkeletalMesh::setSkeleton( uf::Skeleton&& bones ) {
	this->m_skeleton = std::move(bones);
}
// Copy Setters
void uf::SkeletalMesh::setBonesId( const uf::Vertices4i& bones ) {
	this->m_bones_id = bones;
}
void uf::SkeletalMesh::setBonesWeight( const uf::Vertices4f& bones ) {
	this->m_bones_weight = bones;
}
void uf::SkeletalMesh::setSkeleton( const uf::Skeleton& bones ) {
	this->m_skeleton = bones;
}
void uf::SkeletalMesh::setScene( const aiScene* scene ) {
	this->m_scene = scene;
}

// Non-const Getters
uf::Vertices4i& uf::SkeletalMesh::getBonesId() {
	return this->m_bones_id;
}
uf::Vertices4f& uf::SkeletalMesh::getBonesWeight() {
	return this->m_bones_weight;
}
uf::Skeleton& uf::SkeletalMesh::getSkeleton() {
	return this->m_skeleton;
}
Assimp::Importer& uf::SkeletalMesh::getImporter() {
	static Assimp::Importer importer;
	if ( this->m_importer ) return *this->m_importer;
	return importer;
}
// Const \Getters
const uf::Vertices4i& uf::SkeletalMesh::getBonesId() const {
	return this->m_bones_id;
}
const uf::Vertices4f& uf::SkeletalMesh::getBonesWeight() const {
	return this->m_bones_weight;
}
const uf::Skeleton& uf::SkeletalMesh::getSkeleton() const {
	return this->m_skeleton;
}
const aiScene* uf::SkeletalMesh::getScene() const {
	return this->m_scene;
}

#include <uf/ext/assimp/assimp.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <uf/utils/math/quaternion.h>
namespace {
	uint FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim) {	
		for (uint i = 0 ; i < pNodeAnim->mNumPositionKeys - 1 ; i++)
			if (AnimationTime < (float)pNodeAnim->mPositionKeys[i + 1].mTime) return i;
	//	assert(0); return 0;
		return 0;
	}


	uint FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim) {
	//	assert(pNodeAnim->mNumRotationKeys > 0);

		for (uint i = 0 ; i < pNodeAnim->mNumRotationKeys - 1 ; i++)
			if (AnimationTime < (float)pNodeAnim->mRotationKeys[i + 1].mTime) return i;
	//	assert(0); return 0;
		return 0;
	}


	uint FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim) {
	//	assert(pNodeAnim->mNumScalingKeys > 0);
		
		for (uint i = 0 ; i < pNodeAnim->mNumScalingKeys - 1 ; i++)
			if (AnimationTime < (float)pNodeAnim->mScalingKeys[i + 1].mTime) return i;
	//	assert(0); return 0;
		return 0;
	}

	pod::Matrix4 convertMatrix( const aiMatrix4x4& m ) {
		pod::Matrix4 matrix = uf::matrix::identity();
		if ( true ) { 	
			matrix[(4*0)+0] = m.a1; matrix[(4*0)+1] = m.a2; matrix[(4*0)+2] = m.a3; matrix[(4*0)+3] = m.a4;
			matrix[(4*1)+0] = m.b1; matrix[(4*1)+1] = m.b2; matrix[(4*1)+2] = m.b3; matrix[(4*1)+3] = m.b4;
			matrix[(4*2)+0] = m.c1; matrix[(4*2)+1] = m.c2; matrix[(4*2)+2] = m.c3; matrix[(4*2)+3] = m.c4;
			matrix[(4*3)+0] = m.d1; matrix[(4*3)+1] = m.d2; matrix[(4*3)+2] = m.d3; matrix[(4*3)+3] = m.d4;
		} else {
			matrix[(4*3)+0] = m.a4;
			matrix[(4*3)+1] = m.b4;
			matrix[(4*3)+2] = m.c4;
			matrix[(4*3)+3] = m.d4;
		}
		return matrix;
	};


	void CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim) {
		if (pNodeAnim->mNumPositionKeys == 1) {
			Out = pNodeAnim->mPositionKeys[0].mValue; return;
		}	
		uint PositionIndex = FindPosition(AnimationTime, pNodeAnim);
		uint NextPositionIndex = (PositionIndex + 1);
	//	assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);
		float DeltaTime = (float)(pNodeAnim->mPositionKeys[NextPositionIndex].mTime - pNodeAnim->mPositionKeys[PositionIndex].mTime);
		float Factor = (AnimationTime - (float)pNodeAnim->mPositionKeys[PositionIndex].mTime) / DeltaTime;
	//	assert(Factor >= 0.0f && Factor <= 1.0f);
		const aiVector3D& Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
		const aiVector3D& End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
		aiVector3D Delta = End - Start;
		Out = Start + Factor * Delta;
	}
	void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim) {
		if (pNodeAnim->mNumRotationKeys == 1) {
			Out = pNodeAnim->mRotationKeys[0].mValue; return;
		}
		uint RotationIndex = FindRotation(AnimationTime, pNodeAnim);
		uint NextRotationIndex = (RotationIndex + 1);
	//	assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);
		float DeltaTime = (float)(pNodeAnim->mRotationKeys[NextRotationIndex].mTime - pNodeAnim->mRotationKeys[RotationIndex].mTime);
		float Factor = (AnimationTime - (float)pNodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;
	//	assert(Factor >= 0.0f && Factor <= 1.0f);
		const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
		const aiQuaternion& EndRotationQ   = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;	
		aiQuaternion::Interpolate(Out, StartRotationQ, EndRotationQ, Factor);
		Out = Out.Normalize();
	}
	void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim) {
		if (pNodeAnim->mNumScalingKeys == 1) {
			Out = pNodeAnim->mScalingKeys[0].mValue; return;
		}
		uint ScalingIndex = FindScaling(AnimationTime, pNodeAnim);
		uint NextScalingIndex = (ScalingIndex + 1);
	//	assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);
		float DeltaTime = (float)(pNodeAnim->mScalingKeys[NextScalingIndex].mTime - pNodeAnim->mScalingKeys[ScalingIndex].mTime);
		float Factor = (AnimationTime - (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime) / DeltaTime;
	//	assert(Factor >= 0.0f && Factor <= 1.0f);
		const aiVector3D& Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
		const aiVector3D& End   = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
		aiVector3D Delta = End - Start;
		Out = Start + Factor * Delta;
	}
	const aiNodeAnim* FindNodeAnim(const aiAnimation* pAnimation, const std::string NodeName) {
		for (uint i = 0 ; i < pAnimation->mNumChannels ; i++) {
			const aiNodeAnim* pNodeAnim = pAnimation->mChannels[i];
			if ( std::string(pNodeAnim->mNodeName.data) == NodeName) return pNodeAnim;
		}
	return NULL;
	}
	void ReadNodeHeirarchy( uf::SkeletalMesh& mesh, float AnimationTime, const aiNode* pNode, const pod::Matrix4& ParentTransform) {
		std::string NodeName = pNode->mName.data;
		pod::Matrix4 NodeTransformation = convertMatrix(pNode->mTransformation);

		const aiAnimation* pAnimation = mesh.getScene()->mAnimations[0];
		const aiNodeAnim* pNodeAnim = FindNodeAnim(pAnimation, NodeName);
		if (pNodeAnim) {
			// Interpolate scaling and generate scaling transformation matrix
			aiVector3D Scaling; CalcInterpolatedScaling(Scaling, AnimationTime, pNodeAnim);
			pod::Matrix4 ScalingM = uf::matrix::scale( uf::matrix::identity(), { Scaling.x, Scaling.y, Scaling.z } );

			// Interpolate rotation and generate rotation transformation matrix
			aiQuaternion RotationQ; CalcInterpolatedRotation(RotationQ, AnimationTime, pNodeAnim);        
			pod::Quaternion<> q = { RotationQ.x, RotationQ.y, RotationQ.z, RotationQ.w };
			pod::Matrix4 RotationM = uf::quaternion::matrix(q); // = pod::Matrix4(RotationQ.GetMatrix());

			// Interpolate translation and generate translation transformation matrix
			aiVector3D Translation;
			CalcInterpolatedPosition(Translation, AnimationTime, pNodeAnim);
			pod::Matrix4 TranslationM; uf::matrix::translate( uf::matrix::identity(), { Translation.x, Translation.y, Translation.z } );

			// Combine the above transformations
			NodeTransformation = TranslationM * RotationM * ScalingM;
		}
		pod::Matrix4 GlobalInverseTransform = uf::matrix::inverse( convertMatrix( mesh.getScene()->mRootNode->mTransformation ) );
		pod::Matrix4 GlobalTransformation = ParentTransform * NodeTransformation;
		uf::Skeleton::container_t& skeleton = mesh.getSkeleton().getBones();
		for ( uf::Skeleton::Bone& bone : skeleton ) {
			if ( bone.name == NodeName ) bone.matrix = GlobalInverseTransform * GlobalTransformation * bone.offset;
		}
		for (uint i = 0 ; i < pNode->mNumChildren ; i++) ReadNodeHeirarchy(mesh, AnimationTime, pNode->mChildren[i], GlobalTransformation);
	}
}

void uf::SkeletalMesh::animate( float TimeInSeconds ) { if ( !this->m_scene->HasAnimations() ) return;
	pod::Matrix4 Identity = uf::matrix::identity();

	float TicksPerSecond = (float)(this->m_scene->mAnimations[0]->mTicksPerSecond != 0 ? this->m_scene->mAnimations[0]->mTicksPerSecond : 25.0f);
	float TimeInTicks = TimeInSeconds * TicksPerSecond;
	float AnimationTime = fmod(TimeInTicks, (float)this->m_scene->mAnimations[0]->mDuration);

	static bool first = true; if ( first ) { first = false;
		std::cout << TicksPerSecond << ": " << TimeInTicks << ": " << AnimationTime << std::endl;
	}

	ReadNodeHeirarchy(*this, AnimationTime, this->m_scene->mRootNode, Identity);
//	Transforms.resize(m_NumBones);
//	for (uint i = 0 ; i < m_NumBones ; i++) Transforms[i] = m_BoneInfo[i].FinalTransformation;
}