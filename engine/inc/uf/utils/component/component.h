#pragma once

#include <uf/config.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/io/iostream.h>

#include <uf/utils/memory/unordered_map.h>
#include <typeinfo>

#define UF_COMPONENT_POINTERED_USERDATA 1 //

namespace pod {
	struct UF_API Component {
		typedef TYPE_HASH_T id_t;
		typedef uf::stl::unordered_map<pod::Component::id_t, pod::Component> container_t;

	#if UF_COMPONENT_POINTERED_USERDATA
		typedef pod::PointeredUserdata userdata_t;
	#else
		typedef pod::Userdata* userdata_t;
	#endif

		pod::Component::id_t id;
		pod::Component::userdata_t userdata;
	};
}
namespace uf {
	namespace component {
		extern UF_API uf::MemoryPool memoryPool;
		template<typename T> pod::Component::id_t type();
		template<typename T> bool is( const pod::Component& );
	}

	class UF_API Component {
	protected:
		static const bool m_addOn404 = true;
		pod::Component::container_t m_container;
	//	pod::Component::alias_t m_aliases;
	public:
		~Component();
	/*
		template<typename T> bool hasAlias() const;
		template<typename T, typename U> bool addAlias();
	*/
		template<typename T> pod::Component::id_t getType() const;
		template<typename T> bool hasComponent() const;

		template<typename T> pod::Component* getRawComponentPointer();
		template<typename T> const pod::Component* getRawComponentPointer() const;

		template<typename T> T* getComponentPointer();
		template<typename T> const T* getComponentPointer() const;

		template<typename T> T& getComponent();
		template<typename T> const T& getComponent() const;

		template<typename T> T& addComponent( const T& = T() );
		template<typename T> void deleteComponent();

	#if UF_COMPONENT_POINTERED_USERDATA
		pod::PointeredUserdata addComponent( pod::PointeredUserdata& userdata );
		pod::PointeredUserdata moveComponent( pod::PointeredUserdata& userdata );
		
		template<typename T>
		pod::PointeredUserdata detachComponent() {
			pod::Component::id_t id = uf::component::type<T>();
			auto userdata = this->m_container[id].userdata;
			this->m_container.erase( id );
			return userdata;
		}

		template<typename T> T& moveComponent( pod::PointeredUserdata& userdata ) {
			this->moveComponent( userdata );
			return this->getComponent<T>();
		}
	#else
		pod::Userdata* addComponent( pod::Userdata* userdata );
		pod::Userdata* moveComponent( pod::Userdata*& userdata );
		template<typename T>
		pod::Userdata* detachComponent() {
			pod::Component::id_t id = uf::component::type<T>();
			auto userdata = this->m_container[id].userdata;
			this->m_container.erase( id );
			return userdata;
		}

		template<typename T> T& moveComponent( pod::Userdata*& userdata ) {
			this->moveComponent( userdata );
			return this->getComponent<T>();
		}
	#endif

		void destroyComponents();
	};
}

#include "component.inl"