#include "pause.h"

#include <uf/utils/hook/hook.h>
#include <uf/utils/time/time.h>
#include <uf/utils/serialize/serializer.h>
#include <uf/utils/userdata/userdata.h>
#include <uf/utils/window/window.h>
#include <uf/utils/camera/camera.h>
#include <uf/utils/string/ext.h>
// #include <uf/gl/glyph/glyph.h>
#include <uf/engine/asset/asset.h>

#include <unordered_map>

#include <uf/utils/renderer/renderer.h>

#include <uf/utils/http/http.h>
#include <uf/utils/audio/audio.h>
#include <sys/stat.h>
#include <fstream>

namespace {
	ext::Gui* mainText;
	ext::Gui* commandText;
	ext::Gui* transientSprite;
	ext::Gui* transientSpriteShadow;
	ext::Gui* circleIn;
	ext::Gui* circleOut;
	ext::Gui* coverBar;
	ext::Gui* tenkouseiOption;
	ext::Gui* closeOption;
	ext::Gui* quitOption;
}

namespace {
	void playSound( ext::GuiWorldPauseMenu& entity, const std::string& key ) {
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& metadata = entity.getComponent<uf::Serializer>();
		uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();

		std::string url = "./data/audio/ui/" + key + ".ogg";

		uf::Asset& assetLoader = scene.getComponent<uf::Asset>();
		assetLoader.cache(url, "asset:Cache." + std::to_string(entity.getUid()));
	}

	uf::Serializer masterTableGet( const std::string& table ) {
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& mastertable = scene.getComponent<uf::Serializer>();
		return mastertable["system"]["mastertable"][table];
	}
	uf::Serializer masterDataGet( const std::string& table, const std::string& key ) {
		uf::Scene& scene = uf::scene::getCurrentScene();
		uf::Serializer& mastertable = scene.getComponent<uf::Serializer>();
		return mastertable["system"]["mastertable"][table][key];
	}
	inline int64_t parseInt( const std::string& str ) {
		return atoi(str.c_str());
	}
}


EXT_OBJECT_REGISTER_CPP(GuiWorldPauseMenu)
void ext::GuiWorldPauseMenu::initialize() {
	ext::Gui::initialize();

	uf::Scene& scene = uf::scene::getCurrentScene();
	uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();

	// asset loading
	this->addHook( "asset:Cache." + std::to_string(this->getUid()), [&](const std::string& event)->std::string{	
		uf::Serializer json = event;
		
		std::string filename = json["filename"].asString();

		if ( uf::string::extension(filename) != "ogg" ) return "false";

		if ( filename == "" ) return "false";
		uf::Audio& sfx = this->getComponent<uf::Audio>();
		if ( sfx.playing() ) sfx.stop();
		sfx = uf::Audio();
		sfx.load(filename);
		sfx.setVolume(masterdata["volumes"]["sfx"].asFloat());
		sfx.play();

		return "true";
	});

	// release control
	{
		uf::Serializer payload;
		payload["state"] = true;
		uf::hooks.call("window:Mouse.CursorVisibility", payload);
		uf::hooks.call("window:Mouse.Lock");
	}

	// opened
	playSound(*this, "menu open");

	/* Magic Circle Outter */ {
		circleOut = (ext::Gui*) this->findByUid(this->loadChild("./circle-out.json", true));
	/*
		circleOut = new ext::Gui;
		this->addChild(*circleOut);
		circleOut->load("./circle-out.json");
		circleOut->initialize();
	*/
	}
	/* Magic Circle Inner */ {
		circleIn = (ext::Gui*) this->findByUid(this->loadChild("./circle-in.json", true));
	/*
		circleIn = new ext::Gui;
		this->addChild(*circleIn);
		circleIn->load("./circle-in.json");
		circleIn->initialize();
	*/
	}
	/* set sprite order */ {
		metadata["portraits"]["i"] = 0;
		uf::Serializer& pMetadata = scene.getController().getComponent<uf::Serializer>();
		int i = 0;
		for ( auto& k : pMetadata[""]["party"] ) {
			std::string id = k.asString();
			uf::Serializer member = pMetadata[""]["transients"][id];
			uf::Serializer cardData = masterDataGet("Card", id);
			std::string name = cardData["filename"].asString();
			if ( member["skin"].isString() ) name += "_" + member["skin"].asString();
			std::string filename = "https://cdn..xyz//unity/Android/fg/fg_" + name + ".png";
			if ( cardData["internal"].asBool() ) {
				filename = "/smtsamo/fg/fg_" + name + ".png";
			}
			metadata["portraits"]["list"][i++] = filename;
		}
	}
	{
		std::string portrait = metadata["portraits"]["list"][0].asString();
		/* Transient shadow background */ {		
			transientSpriteShadow = new ext::Gui;
			uf::Asset& assetLoader = transientSpriteShadow->getComponent<uf::Asset>();
			this->addChild(*transientSpriteShadow);
			uf::Serializer& pMetadata = transientSpriteShadow->getComponent<uf::Serializer>();
			pMetadata["system"]["assets"][0] = portrait;

			transientSpriteShadow->load("./transient-shadow.json", true);
			transientSpriteShadow->initialize();
		
		}
		/* Transient transientSprite */ {
			transientSprite = new ext::Gui;
			uf::Asset& assetLoader = transientSprite->getComponent<uf::Asset>();
			this->addChild(*transientSprite);
			uf::Serializer& pMetadata = transientSprite->getComponent<uf::Serializer>();
			pMetadata["system"]["assets"][0] = portrait;
			
			transientSprite->load("./transient-portrait.json", true);
			transientSprite->initialize();
	
		}
	}
	/* Main Text (the one that scrolls) */ {
		mainText = (ext::Gui*) this->findByUid(this->loadChild("./main-text.json", true));
	/*
		mainText = new ext::Gui;
		this->addChild(*mainText);
		mainText->load("./main-text.json");
		mainText->initialize();
	*/
	}
	/* Command text */ {
		commandText = (ext::Gui*) this->findByUid(this->loadChild("./command-text.json", true));
	/*
		commandText = new ext::Gui;
		this->addChild(*commandText);
		commandText->load("./command-text.json");
		commandText->initialize();
	*/
	}
	/* Cover bar */ {
		coverBar = (ext::Gui*) this->findByUid(this->loadChild("./yellow-box.json", true));
	/*
		coverBar = new ext::Gui;
		this->addChild(*coverBar);
		coverBar->load("./yellow-box.json");
		coverBar->initialize();
	*/
	}
	/* Option 1 */ {
		tenkouseiOption = (ext::Gui*) this->findByUid(this->loadChild("./tenkousei.json", true));
	/*
		tenkouseiOption = new ext::Gui;
		this->addChild(*tenkouseiOption);
		tenkouseiOption->load("./tenkousei.json");
		tenkouseiOption->initialize();
	*/
	}
	/* Option 2 */ {
		closeOption = (ext::Gui*) this->findByUid(this->loadChild("./close.json", true));
	/*
		closeOption = new ext::Gui;
		this->addChild(*closeOption);
		closeOption->load("./close.json");
		closeOption->initialize();
	*/
	}
	/* Option 3 */ {
		quitOption = (ext::Gui*) this->findByUid(this->loadChild("./quit.json", true));
	/*
		quitOption = new ext::Gui;
		this->addChild(*quitOption);
		quitOption->load("./quit.json");
		quitOption->initialize();
	*/
	}

	this->addHook("menu:Close.%UID%", [&]( const std::string& event ) -> std::string {
	/*
		// kill
		this->getParent().removeChild(*this);
		this->destroy();
		delete this;
		return "true";
	*/
		metadata["system"]["closing"] = true;
		return "true";
	});
}
void ext::GuiWorldPauseMenu::tick() {
	ext::Gui::tick();

	uf::Scene& scene = this->getRootParent<uf::Scene>();
	uf::Serializer& masterdata = scene.getComponent<uf::Serializer>();
	static uf::Timer<long long> timer(false);
	static uf::Audio sfx;
	if ( !timer.running() ) timer.start();

	static float alpha = 0.0f;
	{
		uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	/*
		if ( uf::Window::isKeyPressed("Escape") && !metadata["system"]["closing"].asBool() ) {
			metadata["system"]["closing"] = true;
			playSound(*this, "menu close");
		}
	*/
		if ( metadata["system"]["closing"].asBool() ) {
			if ( alpha <= 0.0f ) {
				alpha = 0.0f;
				metadata["system"]["closing"] = false;
				metadata["system"]["closed"] = true;
			} else alpha -= uf::physics::time::delta;
			metadata["color"][3] = alpha;
		} else if ( metadata["system"]["closed"].asBool() ) {
			// kill
			timer.stop();
			this->destroy();
			this->getParent().removeChild(*this);
			delete this;
			return;
		} else {
			if ( !metadata["initialized"].asBool() ) alpha = 0.0f;
			metadata["initialized"] = true;
			if ( alpha >= 1.0f ) alpha = 1.0f;
			else alpha += uf::physics::time::delta * 1.5f;
			metadata["color"][3] = alpha;
		}
	}

	if ( mainText ) {
		pod::Transform<>& transform = mainText->getComponent<pod::Transform<>>();
		uf::Serializer& metadata = mainText->getComponent<uf::Serializer>();
		float speed = 0.5f;
		if ( metadata["hovered"].asBool() ) speed = 0.75f;
		transform.position.y += uf::physics::time::delta * speed;
		if ( transform.position.y > 2 ) transform.position.y = -2;

		transform.orientation = uf::quaternion::axisAngle( { 0.0f, 0.0f, 1.0f }, -1.5707963f );
		if ( alpha < 0.6f ) metadata["color"][3] = alpha;
	}
	if ( circleIn ) {
		pod::Transform<>& transform = circleIn->getComponent<pod::Transform<>>();
		static float time = 0.0f;
		float speed = 0.0125f;
		uf::Serializer& metadata = circleIn->getComponent<uf::Serializer>();
		if ( metadata["hovered"].asBool() ) speed = 0.25f;
		time += uf::physics::time::delta * -speed;
		transform.orientation = uf::quaternion::axisAngle( { 0.0f, 0.0f, 1.0f }, time );

		static pod::Vector3f start = { transform.position.x, -2.0f, 0.0f };
		static pod::Vector3f end = transform.position;
		static float delta = 0.0f;
		if ( !metadata["initialized"].asBool() ) delta = 0.0f;
		metadata["initialized"] = true;
		if ( delta >= 1 ) delta = 1;
		else {
			delta += uf::physics::time::delta * 1.5f;
			transform.position = uf::vector::lerp( start, end, delta );
		}
		metadata["color"][3] = alpha;
	}
	if ( circleOut ) {
		pod::Transform<>& transform = circleOut->getComponent<pod::Transform<>>();
		uf::Serializer& metadata = circleOut->getComponent<uf::Serializer>();
		static float time = 0.0f;
		float speed = 0.0125f;
		if ( metadata["hovered"].asBool() ) speed = 0.25f;
		time += uf::physics::time::delta * speed;
		transform.orientation = uf::quaternion::axisAngle( { 0.0f, 0.0f, 1.0f }, time );

		static pod::Vector3f start = { transform.position.x, 2.0f, 0.0f };
		static pod::Vector3f end = transform.position;
		static float delta = 0.0f;
		if ( !metadata["initialized"].asBool() ) delta = 0.0f;
		metadata["initialized"] = true;
		if ( delta >= 1 ) delta = 1;
		else {
			delta += uf::physics::time::delta * 1.5f;
			transform.position = uf::vector::lerp( start, end, delta );
		}
		metadata["color"][3] = alpha;
	}
	if ( coverBar ) {
		pod::Transform<>& transform = coverBar->getComponent<pod::Transform<>>();
		static pod::Vector3f start = { -1.5f, transform.position.y, 0.0f };
		static pod::Vector3f end = transform.position;
		static float delta = 0.0f;
		uf::Serializer& metadata = coverBar->getComponent<uf::Serializer>();
		if ( !metadata["initialized"].asBool() ) delta = 0.0f;
		metadata["initialized"] = true;
		if ( delta >= 1 ) delta = 1;
		else {
			delta += uf::physics::time::delta * 1.5f;
			transform.position = uf::vector::lerp( start, end, delta );
		}
		metadata["color"][3] = alpha;
	}
	if ( transientSprite ) {
		pod::Transform<>& transform = transientSprite->getComponent<pod::Transform<>>();
		static pod::Vector3f start = { 2.0f, transform.position.y, 0.0f };
		static pod::Vector3f end = transform.position;
		static float delta = 0.0f;
		uf::Serializer& metadata = transientSprite->getComponent<uf::Serializer>();
		if ( !metadata["initialized"].asBool() ) delta = 0.0f;
		metadata["initialized"] = true;
		if ( delta >= 1 ) delta = 1;
		else {
			delta += uf::physics::time::delta * 1.5f;
			transform.position = uf::vector::lerp( start, end, delta );
		}
		metadata["color"][3] = alpha;

		// cycle transients
		if ( metadata["clicked"].asBool() && timer.elapsed().asDouble() >= 1 ) {
			timer.reset();
			
			playSound(*this, "menu flip");

			uf::Serializer& master = this->getComponent<uf::Serializer>();
			int index = master["portraits"]["i"].asInt() + 1;
			if ( index >= master["portraits"]["list"].size() ) index = 0;
			master["portraits"]["i"] = index;
			std::string filename = master["portraits"]["list"][index].asString();	

			{
				std::string portrait = master["portraits"]["list"][index].asString();
				/* Transient shadow background */ if ( transientSpriteShadow ) {
					transientSpriteShadow->destroy();
					this->removeChild(*transientSpriteShadow);
					delete transientSpriteShadow;

					transientSpriteShadow = new ext::Gui;
					uf::Serializer& pMetadata = transientSpriteShadow->getComponent<uf::Serializer>();
					pMetadata["system"]["assets"][0] = portrait;

					this->addChild(*transientSpriteShadow);
					transientSpriteShadow->load("./transient-shadow.json", true);
					transientSpriteShadow->initialize();
				}
				/* Transient transientSprite */ if ( transientSprite ) {
					transientSprite->destroy();
					this->removeChild(*transientSprite);
					delete transientSprite;

					transientSprite = new ext::Gui;
					uf::Serializer& pMetadata = transientSprite->getComponent<uf::Serializer>();
					pMetadata["system"]["assets"][0] = portrait;

					this->addChild(*transientSprite);
					transientSprite->load("./transient-portrait.json", true);
					transientSprite->initialize();
				}
			}
			if ( mainText ) {
				pod::Transform cTransform = mainText->getComponent<pod::Transform<>>();
				mainText->destroy();
				this->removeChild(*mainText);
				delete mainText;
				mainText = new ext::Gui;
				this->addChild(*mainText);
				mainText->load("./main-text.json");
				mainText->getComponent<pod::Transform<>>() = cTransform;
				mainText->initialize();
			}
		}
	}
	if ( transientSpriteShadow ) {
		pod::Transform<>& transform = transientSpriteShadow->getComponent<pod::Transform<>>();
		static pod::Vector3f start = { 2.0f, transform.position.y, 0.0f };
		static pod::Vector3f end = transform.position;
		static float delta = 0.0f;
		uf::Serializer& metadata = transientSpriteShadow->getComponent<uf::Serializer>();
		if ( !metadata["initialized"].asBool() ) delta = 0.0f;
		metadata["initialized"] = true;
		if ( delta >= 1 ) delta = 1;
		else {
			delta += uf::physics::time::delta * 1.5f;
			transform.position = uf::vector::lerp( start, end, delta );
		}
		metadata["color"][3] = alpha;
	}

	if ( commandText ) {
		pod::Transform<>& transform = commandText->getComponent<pod::Transform<>>();
		static pod::Vector3f start = { -1.5f, transform.position.y, 0.0f };
		static pod::Vector3f end = transform.position;
		static float delta = 0.0f;
		uf::Serializer& metadata = commandText->getComponent<uf::Serializer>();
		if ( !metadata["initialized"].asBool() ) delta = 0.0f;
		metadata["initialized"] = true;
		if ( delta >= 1 ) delta = 1;
		else {
			delta += uf::physics::time::delta * 1.5f;
			transform.position = uf::vector::lerp( start, end, delta );
		}
		metadata["color"][3] = alpha;

		for ( uf::Entity* pointer : commandText->getChildren()  ) {
			uf::Serializer& cMetadata = pointer->getComponent<uf::Serializer>();
			// std::cout << pointer->getName() << ": " << cMetadata["text settings"]["color"] << std::endl;
			cMetadata["text settings"]["color"][3] = alpha;
		}
	}
	if ( tenkouseiOption ) {
		pod::Transform<>& transform = tenkouseiOption->getComponent<pod::Transform<>>();
		static pod::Vector3f start = { -1.5f, transform.position.y, 0.0f };
		static pod::Vector3f end = transform.position;
		static float delta = 0.0f;
		uf::Serializer& metadata = tenkouseiOption->getComponent<uf::Serializer>();
		if ( !metadata["initialized"].asBool() ) delta = 0.0f;
		metadata["initialized"] = true;
		if ( delta >= 1 ) delta = 1;
		else {
			delta += uf::physics::time::delta * 1.5f;
			transform.position = uf::vector::lerp( start, end, delta );
		}


		if ( metadata["clicked"].asBool() && timer.elapsed().asDouble() >= 1 ) {
			timer.reset();
			
			playSound(*this, "invalid select");
		}
		metadata["color"][3] = alpha;

		for ( uf::Entity* pointer : tenkouseiOption->getChildren()  ) {
			uf::Serializer& cMetadata = pointer->getComponent<uf::Serializer>();
			cMetadata["text settings"]["color"][3] = alpha;
			if ( metadata["hovered"].asBool() ) {
				cMetadata["color"][0] = 0;
				cMetadata["color"][1] = 0;
				cMetadata["color"][2] = 0;
			} else {
				cMetadata["color"][0] = 1;
				cMetadata["color"][1] = 1;
				cMetadata["color"][2] = 1;
			}
		}
	}
	if ( closeOption ) {
		pod::Transform<>& transform = closeOption->getComponent<pod::Transform<>>();
		static pod::Vector3f start = { -1.5f, transform.position.y, 0.0f };
		static pod::Vector3f end = transform.position;
		static float delta = 0.0f;
		uf::Serializer& metadata = closeOption->getComponent<uf::Serializer>();
		if ( !metadata["initialized"].asBool() ) delta = 0.0f;
		metadata["initialized"] = true;
		if ( delta >= 1 ) delta = 1;
		else {
			delta += uf::physics::time::delta * 1.5f;
			transform.position = uf::vector::lerp( start, end, delta );
		}


		if ( metadata["clicked"].asBool() && timer.elapsed().asDouble() >= 1 ) {
			timer.reset();
			this->callHook("menu:Close.%UID%");
			playSound(*this, "menu close");
		}
		metadata["color"][3] = alpha;

		for ( uf::Entity* pointer : closeOption->getChildren()  ) {
			uf::Serializer& cMetadata = pointer->getComponent<uf::Serializer>();
			cMetadata["text settings"]["color"][3] = alpha;
			if ( metadata["hovered"].asBool() ) {
				cMetadata["color"][0] = 0;
				cMetadata["color"][1] = 0;
				cMetadata["color"][2] = 0;
			} else {
				cMetadata["color"][0] = 1;
				cMetadata["color"][1] = 1;
				cMetadata["color"][2] = 1;
			}
		}
	}
	if ( quitOption ) {
		pod::Transform<>& transform = quitOption->getComponent<pod::Transform<>>();
		static pod::Vector3f start = { -1.5f, transform.position.y, 0.0f };
		static pod::Vector3f end = transform.position;
		static float delta = 0.0f;
		uf::Serializer& metadata = quitOption->getComponent<uf::Serializer>();
		if ( !metadata["initialized"].asBool() ) delta = 0.0f;
		metadata["initialized"] = true;
		if ( delta >= 1 ) delta = 1;
		else {
			delta += uf::physics::time::delta * 1.5f;
			transform.position = uf::vector::lerp( start, end, delta );
		}


		if ( metadata["clicked"].asBool() && timer.elapsed().asDouble() >= 1 ) {
			timer.reset();
			this->callHook("menu:Close.%UID%");
			playSound(*this, "menu close");
		/*
			uf::Scene& scene = this->getRootParent<uf::Scene>();
			scene.queueHook("system:Quit.%UID%", "", 1.0f);
		*/
		}
		metadata["color"][3] = alpha;

		for ( uf::Entity* pointer : quitOption->getChildren()  ) {
			uf::Serializer& cMetadata = pointer->getComponent<uf::Serializer>();
			cMetadata["text settings"]["color"][3] = alpha;
			if ( metadata["hovered"].asBool() ) {
				cMetadata["color"][0] = 0;
				cMetadata["color"][1] = 0;
				cMetadata["color"][2] = 0;
			} else {
				cMetadata["color"][0] = 1;
				cMetadata["color"][1] = 1;
				cMetadata["color"][2] = 1;
			}
		}
	}

	// Close menu
	if ( uf::Window::isKeyPressed("Escape") && timer.elapsed().asDouble() >= 1 ) {
		timer.reset();
		this->callHook("menu:Close.%UID%");
		playSound(*this, "menu close");
	}
}
void ext::GuiWorldPauseMenu::render() {
	ext::Gui::render();
}

void ext::GuiWorldPauseMenu::destroy() {
	uf::Serializer& metadata = this->getComponent<uf::Serializer>();
	ext::Gui::destroy();
}