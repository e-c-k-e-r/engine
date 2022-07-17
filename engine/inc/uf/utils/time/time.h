#pragma once

#include <uf/config.h>
#include <uf/spec/time/time.h>
#include <cmath>

namespace uf {
	template<typename T = spec::Time::time_t>
	class /*UF_API*/ Time {
	public:
		typedef T time_t;
		typedef spec::Time::exp_t exp_t;
	protected:
		uf::Time<T>::time_t m_time;
		uf::Time<T>::exp_t m_exp;
	public:
		static const uf::Time<T>::exp_t seconds = 0;
		static const uf::Time<T>::exp_t milliseconds = -3;
		static const uf::Time<T>::exp_t microseconds = -6;
		static const uf::Time<T>::exp_t nanoseconds = -9;

		Time(uf::Time<T>::time_t t = 0, uf::Time<T>::exp_t b = spec::Time::unit );

		void set(uf::Time<T>::time_t t);
		void set(uf::Time<T>::time_t t, uf::Time<T>::exp_t b);
		inline void fromSeconds(uf::Time<T>::time_t t);
		inline void fromMilliseconds(uf::Time<T>::time_t t);
		inline void fromMicroseconds(uf::Time<T>::time_t t);
		inline void fromNanoseconds(uf::Time<T>::time_t t);

		uf::Time<T>::time_t get() const;
		uf::Time<T>::exp_t getBase() const;

		uf::Time<T>::time_t asBase(uf::Time<T>::exp_t base = spec::Time::unit );
		inline uf::Time<T>::time_t asSeconds();
		inline uf::Time<T>::time_t asMilliseconds();
		inline uf::Time<T>::time_t asMicroseconds();
		inline uf::Time<T>::time_t asNanoseconds();
		float asFloat();
		double asDouble();

		uf::Time<T>& operator=( const uf::Time<T>::time_t& t );
		operator float();
		operator double();
		bool operator>( const uf::Time<T>::time_t& t );
		bool operator>=( const uf::Time<T>::time_t& t );
		bool operator<( const uf::Time<T>::time_t& t );
		bool operator<=( const uf::Time<T>::time_t& t );
		bool operator==( const uf::Time<T>::time_t& t );
		bool operator>( const uf::Time<T>& t );
		bool operator>=( const uf::Time<T>& t );
		bool operator<( const uf::Time<T>& t );
		bool operator<=( const uf::Time<T>& t );
		bool operator==( const uf::Time<T>& t );
		uf::Time<T> operator-( const uf::Time<T>& t );
		uf::Time<T> operator+( const uf::Time<T>& t );
	};
	template<typename T = spec::Time::time_t>
	class /*UF_API*/ Timer {
	public:
		typedef T time_t;
		typedef spec::Time::exp_t exp_t;
	protected:
		bool m_running;
		uf::Time<T> m_begin;
		uf::Time<T> m_end;
	public:
		Timer(bool running = true, uf::Timer<T>::exp_t b = spec::Time::unit);

		void start( uf::Time<T> = 0 );
		void stop();
		void reset();
		void update();
		bool running() const;
	
		uf::Time<T>& getStarting();
		uf::Time<T>& getEnding();
	
		uf::Time<T> elapsed();
		uf::Time<T> elapsed() const;
		const uf::Time<T>& getStarting() const;
		const uf::Time<T>& getEnding() const;
	};

	namespace time {
		extern UF_API uf::Timer<> timer;
		extern UF_API double current;
		extern UF_API double previous;
		extern UF_API float delta;
		extern UF_API float clamp;
	}
}

#include "time.inl"
#include "timer.inl"