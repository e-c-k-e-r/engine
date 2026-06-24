#pragma once

#include <uf/config.h>
#include <uf/utils/memory/map.h>
#include <memory>
#include "thread.h"

namespace uf {
	template<typename T>
	class UF_API ThreadUnique {
	public:
		typedef T type_t;
		typedef std::shared_ptr<std::mutex> mutex_type_t;
		typedef std::thread::id id_t;
		typedef uf::stl::map<id_t, type_t> container_t;
		typedef uf::stl::map<id_t, mutex_type_t> mutex_container_t;
	protected:
		container_t m_container;
		mutex_container_t m_mutex_container;
		mutex_type_t m_mutex;
	public:
		ThreadUnique() : m_mutex(std::make_shared<std::mutex>()) {}

		bool has( id_t id = std::this_thread::get_id() ) const;
		T& get( id_t id = std::this_thread::get_id() );
		container_t& container();
	
		mutex_type_t getMutex( id_t id = std::this_thread::get_id() );
		void lockMutex( id_t id = std::this_thread::get_id() );
		bool tryMutex( id_t id = std::this_thread::get_id() );
		void unlockMutex( id_t id = std::this_thread::get_id() );
		std::lock_guard<std::mutex> guardMutex( id_t id = std::this_thread::get_id() );
		void cleanup( id_t id = std::this_thread::get_id() );
	};
}

#include "perthread.inl"