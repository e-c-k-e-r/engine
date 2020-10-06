#pragma once

#define UF_BEHAVIOR_VIRTUAL 0

#define UF_BEHAVIOR_ENTITY_H( OBJ )\
	class UF_API OBJ ## Behavior {\
	public:\
		static void attach( uf::Entity& );\
		static void initialize( uf::Object& );\
		static void tick( uf::Object& );\
		static void render( uf::Object& );\
		static void destroy( uf::Object& );\
	};
#if UF_BEHAVIOR_VIRTUAL
	#define UF_BEHAVIOR_VIRTUAL_H() \
		virtual void initialize();\
		virtual void destroy();\
		virtual void tick();\
		virtual void render();

	#define UF_BEHAVIOR_ENTITY_CPP_VIRTUAL_DEFINITIONS( OBJ )\
		void uf::OBJ::initialize(){ uf::Behaviors::initialize(); }\
		void uf::OBJ::tick(){ uf::Behaviors::tick(); }\
		void uf::OBJ::render(){ uf::Behaviors::render(); }\
		void uf::OBJ::destroy(){ uf::Behaviors::destroy(); }
#else
	#define UF_BEHAVIOR_VIRTUAL_H()
	#define UF_BEHAVIOR_ENTITY_CPP_VIRTUAL_DEFINITIONS( OBJ )
#endif
#define UF_BEHAVIOR_ENTITY_CPP_BEGIN( OBJ )\
	uf::OBJ::OBJ() { \
		uf::OBJ ## Behavior::attach( *this );\
	}\
	UF_BEHAVIOR_ENTITY_CPP_VIRTUAL_DEFINITIONS( OBJ )\
	void uf::OBJ ## Behavior::attach( uf::Entity& self ) {\
		self.addBehavior(pod::Behavior{\
			.type = uf::Behaviors::getType<uf::OBJ ## Behavior>(),\
			.initialize = uf::OBJ ## Behavior::initialize,\
			.tick = uf::OBJ ## Behavior::tick,\
			.render = uf::OBJ ## Behavior::render,\
			.destroy = uf::OBJ ## Behavior::destroy,\
		});\
	}
#define UF_BEHAVIOR_ENTITY_CPP_END( OBJ )

#define EXT_BEHAVIOR_ENTITY_CPP_BEGIN( OBJ )\
	void ext::OBJ ## Behavior::attach( uf::Entity& self ) {\
		self.addBehavior(pod::Behavior{\
			.type = uf::Behaviors::getType<ext::OBJ ## Behavior>(),\
			.initialize = ext::OBJ ## Behavior::initialize,\
			.tick = ext::OBJ ## Behavior::tick,\
			.render = ext::OBJ ## Behavior::render,\
			.destroy = ext::OBJ ## Behavior::destroy,\
		});\
	}
#define EXT_BEHAVIOR_ENTITY_CPP_END( OBJ )