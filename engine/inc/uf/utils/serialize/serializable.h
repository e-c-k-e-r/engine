#pragma once

#include <uf/config.h>
#include "serializer.h"

namespace uf {
	class UF_API Serializable {
	protected:

	public:
		virtual Serializer::output_t serialize() = 0;
		virtual Serializer::output_t serialize() const = 0;
		virtual void deserialize( const Serializer& input );
		virtual void deserialize( const Serializer::input_t& input ) = 0;

		virtual void operator<<( const Serializer::input_t& input );
		virtual void operator>>( Serializer::output_t& output );
	};
}