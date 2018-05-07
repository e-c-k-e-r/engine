#pragma once

#include <uf/config.h>
#include "../mesh.h"

#include <assimp/Importer.hpp>
namespace uf {
	class UF_API Skeleton {
	public:
		struct Bone {
			uint index;
			uint parent;
			std::vector<uint> children;
			std::string name;
			pod::Matrix4 offset;
			pod::Matrix4 matrix;
			pod::Matrix4 animation;
			pod::Matrix4 inverse;
		};
		typedef std::vector<Bone> container_t;
	protected:
		uf::Skeleton::container_t m_bones;
		pod::Matrix4 m_globalTransform;
	public:
		bool isSorted();
		void sort();
		bool hasBone( uint ) const;
		bool hasBone( const std::string& ) const;
		const uf::Skeleton::Bone& getBone( uint ) const;
		const uf::Skeleton::Bone& getBone( const std::string& ) const;
		uf::Skeleton::Bone& getBone( uint );
		uf::Skeleton::Bone& getBone( const std::string& );

		uf::Skeleton::container_t& getBones();
		const uf::Skeleton::container_t& getBones() const;

		void addBone( const uf::Skeleton::Bone& );

		const pod::Matrix4& getGlobalTransform() const;
		void setGlobalTransform( const pod::Matrix4& );
	};
	class UF_API SkeletalMesh : public uf::Mesh {
	public:
		static uint MAX_BONES;
	protected:
		uf::Vertices4i 	m_bones_id;
		uf::Vertices4f 	m_bones_weight;
		uf::Skeleton m_skeleton;
		
		Assimp::Importer* m_importer;
		const aiScene* m_scene;
	public:
		SkeletalMesh();
		void clear();
		void destroy();

		// OpenGL ops
		void generate();
		void bindAttributes();
		// OpenGL Getters
		bool loaded() const;

		// Indexed ops
		void index();
		void expand();

		// Move Setters
		void setBonesId( uf::Vertices4i&& bones );
		void setBonesWeight( uf::Vertices4f&& bones );
		void setSkeleton( uf::Skeleton&& skeleton );
		// Copy Setters
		void setBonesId( const uf::Vertices4i& bones );
		void setBonesWeight( const uf::Vertices4f& bones );
		void setSkeleton( const uf::Skeleton& skeleton );
		void setScene( const aiScene* );

		// Non-const Getters
		uf::Vertices4i& getBonesId();
		uf::Vertices4f& getBonesWeight();
		uf::Skeleton& getSkeleton();
		Assimp::Importer& getImporter();
		// Const Getters
		const uf::Vertices4i& getBonesId() const;
		const uf::Vertices4f& getBonesWeight() const;
		const uf::Skeleton& getSkeleton() const;
		const aiScene* getScene() const;

		void animate( float );
	};
}