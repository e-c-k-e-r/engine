#pragma once

#include <uf/config.h>

#include <cmath>
#include <stdint.h>

namespace pod {
	// 	Simple angle
	struct UF_API Angle {
		// 	Enums to store unit information
		enum Unit {
			RADIANS,
			DEGREES,
			GRADIANS,
			UNKNOWN,
			DEFAULT = RADIANS,
		};
		// 	Type to store angle measures in
		typedef double type_t;

		Angle::type_t angle; 	// Angle measure
		Angle::Unit unit; 		// Unit angle is stored as
	};
}

namespace uf {
	// 	Provides operations for POD angle
	class UF_API Angle {
	public:
	// 	Easily access POD's type
		typedef pod::Angle pod_t;
	// 	Replicate POD information
		typedef Angle::pod_t::Unit Unit;
		typedef Angle::pod_t::type_t type_t;
	protected:
	// 	POD storage
		Angle::pod_t m_pod;
	public:
	// 	C-tor
		Angle(const Angle::type_t& def = 0, Angle::Unit unit = Angle::Unit::DEFAULT); 					// initializes POD to 'def'
		Angle(const Angle::pod_t& pod); 																// copies POD altogether
	// 	D-tor
		// Unneccesary
	// 	POD access
		Angle::pod_t& data(); 																			// 	Returns a reference of POD
		const Angle::pod_t& data() const; 																// 	Returns a const-reference of POD
	// 	Alternative POD access
		Angle::type_t& get();																			// 	Returns a pointer to the angle measure
		const Angle::type_t& get() const; 																// 	Returns a const-pointer to the angle measure
	// 	POD manipulation
		Angle::type_t& set(const Angle& angle);															// 	Sets the angle measure (from Angle object)
		Angle::type_t& set(const Angle::type_t& val = 0);												// 	Sets the angle measure (from POD Angle)
		Angle::type_t& set(const Angle::pod_t& pod);													// 	Sets the angle measure (from value) (performs no conversion)
		Angle::type_t& set(const Angle::type_t& val = 0, Angle::Unit unit = Angle::Unit::DEFAULT);		// 	Sets the angle measure (from value and enum) (performs conversion (if necessary))
	// 	POD manipulation
		Angle& convert( Angle::Unit unit ); 															// 	Converts from current unit to desired unit (does nothing if current == desired)


	};
}