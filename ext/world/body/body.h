#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>

#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>

namespace ext {
	class EXT_API Body {
	public:
		void setBounds( const uf::Mesh& );

		int collide( const ext::Body& ) const;
		bool elastic() const;
	};
}