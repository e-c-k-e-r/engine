#include "scene.h"

#include <uf/utils/time/time.h>
#include <uf/utils/io/iostream.h>
#include <uf/utils/math/vector.h>
#include <uf/utils/math/transform.h>
#include <uf/utils/window/window.h>

#include <uf/utils/audio/audio.h>
#include <uf/utils/thread/thread.h>
#include <uf/utils/camera/camera.h>

#include <uf/engine/asset/asset.h>
#include <uf/engine/asset/masterdata.h>

#include <uf/utils/renderer/renderer.h>

#include <uf/ext/gltf/gltf.h>

#include <uf/utils/math/collision.h>

#include "../../ext.h"
#include "../../gui/gui.h"

EXT_OBJECT_REGISTER_CPP(TestScene_Map)
void ext::TestScene_Map::initialize() {
	uf::Scene::initialize();
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Asset& assetLoader = this->getComponent<uf::Asset>();

	this->addHook( "system:Quit.%UID%", [&](const std::string& event)->std::string{
		std::cout << event << std::endl;
		ext::ready = false;
		return "true";
	});
	{
		static uf::Timer<long long> timer(false);
		if ( !timer.running() ) timer.start();
		this->addHook( "world:Entity.LoadAsset", [&](const std::string& event)->std::string{
			uf::Serializer json = event;

			std::string asset = json["asset"].asString();
			std::string uid = json["uid"].asString();

			assetLoader.load(asset, "asset:Load." + uid);

			return "true";
		});
	}
	{
		static uf::Timer<long long> timer(false);
		if ( !timer.running() ) timer.start();
		this->addHook( "menu:Pause", [&](const std::string& event)->std::string{
			if ( timer.elapsed().asDouble() < 1 ) return "false";
			timer.reset();

			uf::Serializer json = event;
			ext::Gui* manager = (ext::Gui*) this->findByName("Gui Manager");
			if ( !manager ) return "false";
			uf::Serializer payload;
			ext::Gui* gui = (ext::Gui*) manager->findByUid( (payload["uid"] = manager->loadChild("/scenes/worldscape/gui/pause/menu.json", false)).asUInt64() );
			uf::Serializer& metadata = gui->getComponent<uf::Serializer>();
			metadata["menu"] = json["menu"];
			gui->initialize();
			return payload;
		});
	}

	/* store viewport size */ {
		metadata["window"]["size"]["x"] = uf::renderer::width;
		metadata["window"]["size"]["y"] = uf::renderer::height;
		
		this->addHook( "window:Resized", [&](const std::string& event)->std::string{
			uf::Serializer json = event;

			pod::Vector2ui size; {
				size.x = json["window"]["size"]["x"].asUInt64();
				size.y = json["window"]["size"]["y"].asUInt64();
			}

			metadata["window"] = json["window"];

			return "true";
		});
	}

	// lock control
	{
		uf::Serializer payload;
		payload["state"] = true;
		uf::hooks.call("window:Mouse.CursorVisibility", payload);
		uf::hooks.call("window:Mouse.Lock");
	}
}

void ext::TestScene_Map::render() {
	uf::Scene::render();
}
void ext::TestScene_Map::tick() {
	uf::Scene::tick();

	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	uf::Asset& assetLoader = this->getComponent<uf::Asset>();

	/* Regain control if nothing requests it */ {
		ext::Gui* menu = (ext::Gui*) this->findByName("Gui: Menu");
		if ( !menu ) {
			uf::Serializer payload;
			payload["state"] = false;
			uf::hooks.call("window:Mouse.CursorVisibility", payload);
			uf::hooks.call("window:Mouse.Lock");
		}
	}

	/* Updates Sound Listener */ {
		pod::Transform<>& transform = this->getController()->getComponent<pod::Transform<>>();
		
		ext::oal.listener( "POSITION", { transform.position.x, transform.position.y, transform.position.z } );
		ext::oal.listener( "VELOCITY", { 0, 0, 0 } );
		ext::oal.listener( "ORIENTATION", { 0, 0, 1, 1, 0, 0 } );
	}

	/* Collision */ {
		bool local = false;
		bool sort = false;
		bool useStrongest = false;
	//	pod::Thread& thread = uf::thread::fetchWorker();
		pod::Thread& thread = uf::thread::has("Physics") ? uf::thread::get("Physics") : uf::thread::create( "Physics", true, false );
		auto function = [&]() -> int {
			std::vector<uf::Object*> entities;
			std::function<void(uf::Entity*)> filter = [&]( uf::Entity* entity ) {
				auto& metadata = entity->getComponent<uf::Serializer>();
				if ( !metadata["system"]["physics"]["collision"].isNull() && !metadata["system"]["physics"]["collision"].asBool() ) return;
				if ( entity->hasComponent<uf::Collider>() )
					entities.push_back((uf::Object*) entity);
			};
			this->process(filter);
			auto onCollision = []( pod::Collider::Manifold& manifold, uf::Object* a, uf::Object* b ){				
				uf::Serializer payload;
				payload["normal"][0] = manifold.normal.x;
				payload["normal"][1] = manifold.normal.y;
				payload["normal"][2] = manifold.normal.z;
				payload["entity"] = b->getUid();
				payload["depth"] = -manifold.depth;
				a->callHook("world:Collision.%UID%", payload);
				
				payload["entity"] = a->getUid();
				payload["depth"] = manifold.depth;
				b->callHook("world:Collision.%UID%", payload);
			};
			auto testColliders = [&]( uf::Collider& colliderA, uf::Collider& colliderB, uf::Object* a, uf::Object* b, bool useStrongest ){
				pod::Collider::Manifold strongest;
				auto manifolds = colliderA.intersects(colliderB);
				for ( auto manifold : manifolds ) {
					if ( manifold.colliding && manifold.depth > 0 ) {
						if ( !useStrongest ) onCollision(manifold, a, b);
						else if ( strongest.depth < manifold.depth ) strongest = manifold;
					}
				}
				if ( useStrongest && strongest.colliding ) onCollision(strongest, a, b);
			};
			// collide with others
			for ( auto* _a : entities ) {
				uf::Object& entityA = *_a;
				for ( auto* _b : entities ) { if ( _a == _b ) continue;
					uf::Object& entityB = *_b;
					testColliders( entityA.getComponent<uf::Collider>(), entityB.getComponent<uf::Collider>(), &entityA, &entityB, useStrongest );
				}
			}
		
			return 0;
		};
		if ( local ) function(); else uf::thread::add( thread, function, true );
	}
}