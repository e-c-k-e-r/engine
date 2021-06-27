#pragma once

#include <uf/config.h>

#include <string>
#include <uf/utils/math/vector.h>

namespace pod {
	namespace payloads {
		struct windowEvent {
			std::string type = "";
			std::string invoker = "";
		};
		struct windowResized : public windowEvent {
			struct {
				pod::Vector2ui size{};
			} window;
		};
		struct windowFocusedChanged : public windowEvent {
			struct {
				int_fast8_t state{};
			} window;
		};
		struct windowTextEntered : public windowEvent {
			struct {
				uint32_t utf32;
				std::string unicode;
			} text;
		};
		struct windowKey : public windowEvent {
			struct {
				std::string code;
				uint32_t raw;
				
				int_fast8_t state = -1;
				bool async;
				struct {
					bool alt;
					bool ctrl;
					bool shift;
					bool sys;
				} modifier;
			} key;
		};
		struct windowMouseWheel : public windowEvent {
			struct {
				pod::Vector2ui 	position{};
				float			delta = 0;
			} mouse;
		};
		struct windowMouseClick : public windowEvent {
			struct {
				pod::Vector2i 	position = {};
				pod::Vector2i 	delta = {};
				std::string 	button = "";
				int_fast8_t 	state = 0;
			} mouse;
		};
		struct windowMouseMoved : public windowResized {
			struct {
				pod::Vector2ui position{};
				pod::Vector2i delta{};
				int_fast8_t state{};
			} mouse;
		};
	}
}