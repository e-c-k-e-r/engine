#pragma once

#include <uf/config.h>
#include "thread.h"

namespace uf {
	template<typename T>
	class UF_API ThreadUnique {
	public:
		typedef T type_t;
		typedef std::thread::id id_t;
		typedef std::unordered_map<id_t, type_t> container_t;
	protected:
		container_t m_container;
	public:
		bool has( id_t id = std::this_thread::get_id() ) const;
		T& get( id_t id = std::this_thread::get_id() );
		container_t& container();
	};
}

#include "perthread.inl"