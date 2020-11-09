#pragma once

#include <uf/config.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/io/iostream.h>

#include <unordered_map>
#include <typeinfo>

namespace pod {
	struct UF_API Component {
		typedef std::size_t id_t;
		typedef std::unordered_map<pod::Component::id_t, pod::Component> container_t;
		typedef std::unordered_map<pod::Component::id_t, pod::Component::id_t> alias_t;

		pod::Component::id_t id;
		pod::Userdata* userdata;
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
		pod::Component::alias_t m_aliases;
	public:
		~Component();

		template<typename T> bool hasAlias() const;
		template<typename T, typename U> bool addAlias();
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

		void destroyComponents();
	};
}

#include "component.inl"