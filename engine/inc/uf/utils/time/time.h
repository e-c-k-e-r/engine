#pragma once

#include <uf/config.h>
#include <uf/spec/time/time.h>
#include <cmath>

namespace uf {
	template<typename T = spec::Time::time_t>
	class UF_API Time {
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

		UF_API_CALL Time(uf::Time<T>::time_t t = 0, uf::Time<T>::exp_t b = spec::Time::unit );

		void UF_API_CALL set(uf::Time<T>::time_t t);
		void UF_API_CALL set(uf::Time<T>::time_t t, uf::Time<T>::exp_t b);
		inline void UF_API_CALL fromSeconds(uf::Time<T>::time_t t);
		inline void UF_API_CALL fromMilliseconds(uf::Time<T>::time_t t);
		inline void UF_API_CALL fromMicroseconds(uf::Time<T>::time_t t);
		inline void UF_API_CALL fromNanoseconds(uf::Time<T>::time_t t);

		uf::Time<T>::time_t UF_API_CALL get() const;
		uf::Time<T>::exp_t UF_API_CALL getBase() const;

		uf::Time<T>::time_t UF_API_CALL asBase(uf::Time<T>::exp_t base = spec::Time::unit );
		inline uf::Time<T>::time_t UF_API_CALL asSeconds();
		inline uf::Time<T>::time_t UF_API_CALL asMilliseconds();
		inline uf::Time<T>::time_t UF_API_CALL asMicroseconds();
		inline uf::Time<T>::time_t UF_API_CALL asNanoseconds();
		double UF_API_CALL asDouble();

		uf::Time<T>& UF_API_CALL operator=( const uf::Time<T>::time_t& t );
		UF_API_CALL operator double();
		bool UF_API_CALL operator>( const uf::Time<T>::time_t& t );
		bool UF_API_CALL operator>=( const uf::Time<T>::time_t& t );
		bool UF_API_CALL operator<( const uf::Time<T>::time_t& t );
		bool UF_API_CALL operator<=( const uf::Time<T>::time_t& t );
		bool UF_API_CALL operator==( const uf::Time<T>::time_t& t );
		bool UF_API_CALL operator>( const uf::Time<T>& t );
		bool UF_API_CALL operator>=( const uf::Time<T>& t );
		bool UF_API_CALL operator<( const uf::Time<T>& t );
		bool UF_API_CALL operator<=( const uf::Time<T>& t );
		bool UF_API_CALL operator==( const uf::Time<T>& t );
		uf::Time<T> UF_API_CALL operator-( const uf::Time<T>& t );
		uf::Time<T> UF_API_CALL operator+( const uf::Time<T>& t );
	};
	template<typename T = spec::Time::time_t>
	class UF_API Timer {
	public:
		typedef T time_t;
		typedef spec::Time::exp_t exp_t;
	protected:
		bool m_running;
		uf::Time<T> m_begin;
		uf::Time<T> m_end;
	public:
		Timer(bool running = true, uf::Timer<T>::exp_t b = spec::Time::unit);

		void start();
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
}

#include "time.inl"
#include "timer.inl"