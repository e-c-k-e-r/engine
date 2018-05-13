#pragma once

#include <uf/config.h>
#include <uf/ext/ext.h>
#include <uf/engine/entity/entity.h>

#include <uf/gl/texture/texture.h>
#include <uf/gl/camera/camera.h>
#include <uf/gl/gbuffer/gbuffer.h>

#include <uf/utils/math/transform.h>
#include <uf/utils/math/physics.h>

#include "../object/object.h"

namespace ext {
	class EXT_API Light : public ext::Object {
	protected:
		pod::Vector3 m_color = {1, 1, 1};
		pod::Vector3 m_specular = {1, 1, 1};
		float m_attenuation = 0.00125f;
		float m_power = 100.0f;
		uint m_state = 0;
	public:
		void initialize();
		void tick();
		void render();

		void setColor( const pod::Vector3& );
		const pod::Vector3& getColor() const;

		void setSpecular( const pod::Vector3& );
		const pod::Vector3& getSpecular() const;
		
		void setAttenuation( float );
		float getAttenuation() const;

		void setPower( float );
		float getPower() const;
	};
}