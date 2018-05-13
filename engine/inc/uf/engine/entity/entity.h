#pragma once

#include <uf/config.h>
#include <uf/utils/component/component.h>
#include <uf/utils/serialize/serializer.h>
#include <vector>

namespace uf {
	class UF_API Entity : public uf::Component {
	public:
		typedef std::vector<uf::Entity*> container_t;
	protected:
		static uf::Entity null;

		uf::Entity* m_parent = NULL;
		uf::Entity::container_t m_children;

		std::string m_name = "Entity";
	public:
		Entity( bool = false );
		virtual ~Entity();
		
		template<typename T=uf::Entity> T& getParent();
		template<typename T=uf::Entity> T& getRootParent();
		template<typename T=uf::Entity> const T& getParent() const;
		template<typename T=uf::Entity> const T& getRootParent() const;
		const std::string& getName() const;
		
		void setParent( uf::Entity& parent = null );
		void addChild( uf::Entity& child );
		void moveChild( uf::Entity& child );
		void removeChild( uf::Entity& child );
		uf::Entity::container_t& getChildren();
		const uf::Entity::container_t& getChildren() const;

		virtual void initialize();
		virtual void destroy();
		virtual void tick();
		virtual void render();

		template<typename T=uf::Entity> void initialize();
		template<typename T=uf::Entity> void destroy();
		template<typename T=uf::Entity> void tick();
		template<typename T=uf::Entity> void render();
	};
/*
namespace uf {
	class UF_API Entity : public uf::Component {
	public:
		typedef std::vector<uf::Entity*> container_t;
	protected:
		static uf::Entity null;

		uf::Entity* m_parent = NULL;
		uf::Entity::container_t m_children;
	public:
		Entity();
		virtual ~Entity();

		template<typename T=uf::Entity> T& getParent();
		template<typename T=uf::Entity> T& getRootParent();
		template<typename T=uf::Entity> const T& getParent() const;
		template<typename T=uf::Entity> const T& getRootParent() const;
		
		void setParent( uf::Entity& parent = null );
		void addChild( uf::Entity& child );

		virtual void initialize();
		virtual void destroy();

		virtual void tick();
		virtual void render();
	};
*/
}
#include "entity.inl"