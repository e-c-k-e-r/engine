#pragma once

#include <uf/config.h>
#include <uf/engine/behavior/behavior.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/memory/vector.h>
#include <uf/utils/resolvable/resolvable.h>

#include <uf/utils/memory/pool.h>

#include <functional>

namespace uf {
	class UF_API Entity : public uf::Behaviors {
	public:
		typedef uf::stl::vector<uf::Entity*> container_t;
		static uf::Entity null;
		static bool deleteChildrenOnDestroy;
		static bool deleteComponentsOnDestroy;
	protected:
		static std::size_t uids;

		uf::Entity* m_parent = NULL;
		uf::Entity::container_t m_children;

		std::size_t m_uid = 0;
		uf::stl::string m_name = "Entity";

	public:
		static uf::MemoryPool memoryPool;

		Entity();
		~Entity();
		// identifiers
		bool isValid() const;
		const uf::stl::string& getName() const;
		std::size_t getUid() const;
		void setName( const uf::stl::string& );
		// cast to other Entity classes, avoid nasty shit like *((uf::Object*) &entity)
		template<typename T> T& as();
		template<typename T> const T& as() const;
		// parent-child relationship
		bool hasParent() const;
		template<typename T=uf::Entity> T& getParent();
		template<typename T=uf::Entity> T& getRootParent();
		template<typename T=uf::Entity> const T& getParent() const;
		template<typename T=uf::Entity> const T& getRootParent() const;

		template<typename T=uf::Entity> pod::Resolvable<T> resolvable();

		void setUid();
		void unsetUid();

		void setParent();
		void setParent( uf::Entity& parent );
		uf::Entity& addChild( uf::Entity& child );
		void addChild( uf::Entity* child );
		void moveChild( uf::Entity& child );
		void removeChild( uf::Entity& child );

		uf::Entity::container_t& getChildren();
		const uf::Entity::container_t& getChildren() const;
		// use instantiate to allocate
		void* operator new(size_t, const uf::stl::string& = "");
		void operator delete(void*);
		// search
		uf::Entity* findByName( const uf::stl::string& name );
		uf::Entity* findByUid( std::size_t id );
		const uf::Entity* findByName( const uf::stl::string& name ) const;
		const uf::Entity* findByUid( std::size_t id ) const;
		// filters
		void process( std::function<void(uf::Entity*)> );
		void process( std::function<void(uf::Entity*, int)>, int depth = 0 );
		// globals
		static uf::Entity* globalFindByUid( size_t id );
		static uf::Entity* globalFindByName( const uf::stl::string& name );

		template<typename T=uf::Entity> static T& resolve( const pod::Resolvable<T>& );
	};
}

#include "entity.inl"
#include "behavior.h"