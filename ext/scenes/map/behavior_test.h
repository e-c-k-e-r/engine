#pragma once

#include "../base.h"

namespace ext {
	class EXT_API TestBehavior {
	public:
		static void attach( uf::Object& );
		
		static void initialize( uf::Object& );
		static void tick( uf::Object& );
		static void render( uf::Object& );
		static void destroy( uf::Object& );
	};
}