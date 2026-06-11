#pragma once

#include <uf/config.h>
#include <uf/engine/behavior/behavior.h>
//#include <uf/utils/serialize/serializer.h>
#include <uf/utils/memory/vector.h>
#include <uf/utils/resolvable/resolvable.h>
#include <uf/engine/asset/payload.h>
#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>

#include <uf/utils/memory/pool.h>

#include <functional>

namespace uf {
	class Serializer;

	class UF_API Entity : public uf::Behaviors {
	public:
		typedef uf::stl::vector<uf::Entity*> container_t;
		static uf::Entity null;
		static bool deleteChildrenOnDestroy;
		static bool deleteComponentsOnDestroy;

		static uf::Timer<long long> timer;
		static bool assertionLoad;
		static bool deferLazyCalls;

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

		bool operator==( const uf::Entity& ) const ;

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

		//
		bool reload( bool = false );
		bool load( const uf::stl::string&, bool = false );
		bool load( const uf::Serializer& );

		void loadAssets( const uf::Serializer& );

		uf::Entity& createChild( bool = true );

		uf::Entity& loadChild( const uf::Serializer&, bool = true );
		uf::Entity* loadChildPointer( const uf::Serializer&, bool = true );
		std::size_t loadChildUid( const uf::Serializer&, bool = true );

		uf::Entity& loadChild( const uf::stl::string&, bool = true );
		uf::Entity* loadChildPointer( const uf::stl::string&, bool = true );
		std::size_t loadChildUid( const uf::stl::string&, bool = true );
		
		template<typename T> T loadChild( const uf::Serializer&, bool = true );
		template<typename T> T loadChild( const uf::stl::string&, bool = true );

		inline uf::hashed_string formatHookName( const uf::stl::string_view& n );
		static inline uf::hashed_string formatHookName( const uf::stl::string_view& n, size_t uid, bool fetch = false );

		inline size_t resolveHookKey( size_t hash ) const { return hash; }
		inline size_t resolveHookKey( const uf::stl::string& name ) { return this->formatHookName(name); }
	
		template<typename T> size_t addHook( const size_t& name, T function );
		template<typename T> inline size_t addHook( const uf::stl::string& name, T function ) {
			return this->addHook( this->formatHookName( name ), function );
		}

		template<typename K> inline void queueHook( const K& name, float timeout = 0 );
		template<typename K, typename V> inline void queueHook( const K& name, const V&, float = 0 );
		
		template<typename K, typename... Args> inline uf::Hooks::return_t lazyCallHook(const K& name, Args&&... args) {
			if ( uf::Object::deferLazyCalls ) {
				this->queueHook(name, std::forward<Args>(args)..., 0.0f);
				return {};
			}
			return this->callHook( name, std::forward<Args>(args)... );
		}

		template<typename K, typename... Args> inline uf::Hooks::return_t callHook( const K& name, Args&&... args ) {
			return uf::hooks.call( this->resolveHookKey(name), std::forward<Args>(args)... );
		}

		uf::stl::string resolveURI( const uf::stl::string& filename, const uf::stl::string& root = "" );
		uf::asset::Payload resolveToPayload( const uf::stl::string& filename, const uf::stl::string& mime = "" );

		void queueDeletion();
	};

	typedef uf::Entity Object;
}

namespace uf {
	namespace string {
		uf::stl::string UF_API toString( const uf::Entity& object );
	}
}

#include "entity.inl"
#include "behavior.h"