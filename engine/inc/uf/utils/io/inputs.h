#pragma once

#include <uf/config.h>
#include <uf/utils/string/string.h>
#include <uf/utils/math/vector.h>

namespace uf {
	namespace inputs {
		#if UF_WINDOW_KEYCODE_STRINGS
			typedef uint_fast8_t key_t;
			#define UF_KEY(x) __LINE__
		#else
			typedef const char* key_t;
			#define UF_KEY(x) x
		#endif

		typedef bool state_t;
		typedef float analog_t;
		typedef pod::Vector2t<analog_t> analog2_t;

		namespace kbm {
			namespace enums {
				constexpr key_t LShift = UF_KEY("LShift");
				constexpr key_t RShift = UF_KEY("RShift");

				constexpr key_t LControl = UF_KEY("LControl");
				constexpr key_t RControl = UF_KEY("RControl");

				constexpr key_t LAlt = UF_KEY("LAlt");
				constexpr key_t RAlt = UF_KEY("RAlt");

				constexpr key_t LSystem = UF_KEY("LSystem");
				constexpr key_t RSystem = UF_KEY("RSystem");

				constexpr key_t Menu = UF_KEY("Menu");
				constexpr key_t SemiColon = UF_KEY("SemiColon");
				constexpr key_t Slash = UF_KEY("Slash");
				constexpr key_t Equal = UF_KEY("Equal");
				constexpr key_t Dash = UF_KEY("Dash");
				constexpr key_t LBracket = UF_KEY("LBracket");
				constexpr key_t RBracket = UF_KEY("RBracket");
				constexpr key_t Comma = UF_KEY("Comma");
				constexpr key_t Period = UF_KEY("Period");
				constexpr key_t Quote = UF_KEY("Quote");
				constexpr key_t BackSlash = UF_KEY("BackSlash");
				constexpr key_t Tilde = UF_KEY("Tilde");
				constexpr key_t Escape = UF_KEY("Escape");
				constexpr key_t Space = UF_KEY("Space");
				constexpr key_t Enter = UF_KEY("Enter");
				constexpr key_t BackSpace = UF_KEY("BackSpace");
				constexpr key_t Tab = UF_KEY("Tab");
				constexpr key_t PageUp = UF_KEY("PageUp");
				constexpr key_t PageDown = UF_KEY("PageDown");
				constexpr key_t End = UF_KEY("End");
				constexpr key_t Home = UF_KEY("Home");
				constexpr key_t Insert = UF_KEY("Insert");
				constexpr key_t Delete = UF_KEY("Delete");
				constexpr key_t Add = UF_KEY("Add");
				constexpr key_t Subtract = UF_KEY("Subtract");
				constexpr key_t Multiply = UF_KEY("Multiply");
				constexpr key_t Divide = UF_KEY("Divide");
				constexpr key_t Pause = UF_KEY("Pause");
				constexpr key_t F1 = UF_KEY("F1");
				constexpr key_t F2 = UF_KEY("F2");
				constexpr key_t F3 = UF_KEY("F3");
				constexpr key_t F4 = UF_KEY("F4");
				constexpr key_t F5 = UF_KEY("F5");
				constexpr key_t F6 = UF_KEY("F6");
				constexpr key_t F7 = UF_KEY("F7");
				constexpr key_t F8 = UF_KEY("F8");
				constexpr key_t F9 = UF_KEY("F9");
				constexpr key_t F10 = UF_KEY("F10");
				constexpr key_t F11 = UF_KEY("F11");
				constexpr key_t F12 = UF_KEY("F12");
				constexpr key_t F13 = UF_KEY("F13");
				constexpr key_t F14 = UF_KEY("F14");
				constexpr key_t F15 = UF_KEY("F15");
				constexpr key_t Left = UF_KEY("Left");
				constexpr key_t Right = UF_KEY("Right");
				constexpr key_t Up = UF_KEY("Up");
				constexpr key_t Down = UF_KEY("Down");
				constexpr key_t Numpad0 = UF_KEY("Numpad0");
				constexpr key_t Numpad1 = UF_KEY("Numpad1");
				constexpr key_t Numpad2 = UF_KEY("Numpad2");
				constexpr key_t Numpad3 = UF_KEY("Numpad3");
				constexpr key_t Numpad4 = UF_KEY("Numpad4");
				constexpr key_t Numpad5 = UF_KEY("Numpad5");
				constexpr key_t Numpad6 = UF_KEY("Numpad6");
				constexpr key_t Numpad7 = UF_KEY("Numpad7");
				constexpr key_t Numpad8 = UF_KEY("Numpad8");
				constexpr key_t Numpad9 = UF_KEY("Numpad9");
				constexpr key_t Q = UF_KEY("Q");
				constexpr key_t W = UF_KEY("W");
				constexpr key_t E = UF_KEY("E");
				constexpr key_t R = UF_KEY("R");
				constexpr key_t T = UF_KEY("T");
				constexpr key_t Y = UF_KEY("Y");
				constexpr key_t U = UF_KEY("U");
				constexpr key_t I = UF_KEY("I");
				constexpr key_t O = UF_KEY("O");
				constexpr key_t P = UF_KEY("P");
				constexpr key_t A = UF_KEY("A");
				constexpr key_t S = UF_KEY("S");
				constexpr key_t D = UF_KEY("D");
				constexpr key_t F = UF_KEY("F");
				constexpr key_t G = UF_KEY("G");
				constexpr key_t H = UF_KEY("H");
				constexpr key_t J = UF_KEY("J");
				constexpr key_t K = UF_KEY("K");
				constexpr key_t L = UF_KEY("L");
				constexpr key_t Z = UF_KEY("Z");
				constexpr key_t X = UF_KEY("X");
				constexpr key_t C = UF_KEY("C");
				constexpr key_t V = UF_KEY("V");
				constexpr key_t B = UF_KEY("B");
				constexpr key_t N = UF_KEY("N");
				constexpr key_t M = UF_KEY("M");
				constexpr key_t Num1 = UF_KEY("Num1");
				constexpr key_t Num2 = UF_KEY("Num2");
				constexpr key_t Num3 = UF_KEY("Num3");
				constexpr key_t Num4 = UF_KEY("Num4");
				constexpr key_t Num5 = UF_KEY("Num5");
				constexpr key_t Num6 = UF_KEY("Num6");
				constexpr key_t Num7 = UF_KEY("Num7");
				constexpr key_t Num8 = UF_KEY("Num8");
				constexpr key_t Num9 = UF_KEY("Num9");
				constexpr key_t Num0 = UF_KEY("Num0");
			};
			namespace states {
				extern UF_API state_t LShift;
				extern UF_API state_t RShift;
				extern UF_API state_t LAlt;
				extern UF_API state_t RAlt;
				extern UF_API state_t LControl;
				extern UF_API state_t RControl;
				extern UF_API state_t LSystem;
				extern UF_API state_t RSystem;
				extern UF_API state_t Menu;
				extern UF_API state_t SemiColon;
				extern UF_API state_t Slash;
				extern UF_API state_t Equal;
				extern UF_API state_t Dash;
				extern UF_API state_t LBracket;
				extern UF_API state_t RBracket;
				extern UF_API state_t Comma;
				extern UF_API state_t Period;
				extern UF_API state_t Quote;
				extern UF_API state_t BackSlash;
				extern UF_API state_t Tilde;
				extern UF_API state_t Escape;
				extern UF_API state_t Space;
				extern UF_API state_t Enter;
				extern UF_API state_t BackSpace;
				extern UF_API state_t Tab;
				extern UF_API state_t PageUp;
				extern UF_API state_t PageDown;
				extern UF_API state_t End;
				extern UF_API state_t Home;
				extern UF_API state_t Insert;
				extern UF_API state_t Delete;
				extern UF_API state_t Add;
				extern UF_API state_t Subtract;
				extern UF_API state_t Multiply;
				extern UF_API state_t Divide;
				extern UF_API state_t Pause;
				extern UF_API state_t F1;
				extern UF_API state_t F2;
				extern UF_API state_t F3;
				extern UF_API state_t F4;
				extern UF_API state_t F5;
				extern UF_API state_t F6;
				extern UF_API state_t F7;
				extern UF_API state_t F8;
				extern UF_API state_t F9;
				extern UF_API state_t F10;
				extern UF_API state_t F11;
				extern UF_API state_t F12;
				extern UF_API state_t F13;
				extern UF_API state_t F14;
				extern UF_API state_t F15;
				extern UF_API state_t Left;
				extern UF_API state_t Right;
				extern UF_API state_t Up;
				extern UF_API state_t Down;
				extern UF_API state_t Numpad0;
				extern UF_API state_t Numpad1;
				extern UF_API state_t Numpad2;
				extern UF_API state_t Numpad3;
				extern UF_API state_t Numpad4;
				extern UF_API state_t Numpad5;
				extern UF_API state_t Numpad6;
				extern UF_API state_t Numpad7;
				extern UF_API state_t Numpad8;
				extern UF_API state_t Numpad9;
				extern UF_API state_t Q;
				extern UF_API state_t W;
				extern UF_API state_t E;
				extern UF_API state_t R;
				extern UF_API state_t T;
				extern UF_API state_t Y;
				extern UF_API state_t U;
				extern UF_API state_t I;
				extern UF_API state_t O;
				extern UF_API state_t P;
				extern UF_API state_t A;
				extern UF_API state_t S;
				extern UF_API state_t D;
				extern UF_API state_t F;
				extern UF_API state_t G;
				extern UF_API state_t H;
				extern UF_API state_t J;
				extern UF_API state_t K;
				extern UF_API state_t L;
				extern UF_API state_t Z;
				extern UF_API state_t X;
				extern UF_API state_t C;
				extern UF_API state_t V;
				extern UF_API state_t B;
				extern UF_API state_t N;
				extern UF_API state_t M;
				extern UF_API state_t Num1;
				extern UF_API state_t Num2;
				extern UF_API state_t Num3;
				extern UF_API state_t Num4;
				extern UF_API state_t Num5;
				extern UF_API state_t Num6;
				extern UF_API state_t Num7;
				extern UF_API state_t Num8;
				extern UF_API state_t Num9;
				extern UF_API state_t Num0;
			};
		}
		namespace controller {
			namespace enums {
				constexpr key_t R_DPAD_UP = UF_KEY("R_DPAD_UP");
				constexpr key_t R_DPAD_DOWN = UF_KEY("R_DPAD_DOWN");
				constexpr key_t R_DPAD_LEFT = UF_KEY("R_DPAD_LEFT");
				constexpr key_t R_DPAD_RIGHT = UF_KEY("R_DPAD_RIGHT");
				constexpr key_t R_JOYSTICK = UF_KEY("R_JOYSTICK");
				constexpr key_t R_A = UF_KEY("R_A");
				constexpr key_t L_DPAD_UP = UF_KEY("L_DPAD_UP");
				constexpr key_t L_DPAD_DOWN = UF_KEY("L_DPAD_DOWN");
				constexpr key_t L_DPAD_LEFT = UF_KEY("L_DPAD_LEFT");
				constexpr key_t L_DPAD_RIGHT = UF_KEY("L_DPAD_RIGHT");
				constexpr key_t L_JOYSTICK = UF_KEY("L_JOYSTICK");
				constexpr key_t L_A = UF_KEY("L_A");
				constexpr key_t DPAD_UP = UF_KEY("DPAD_UP");
				constexpr key_t DPAD_DOWN = UF_KEY("DPAD_DOWN");
				constexpr key_t DPAD_LEFT = UF_KEY("DPAD_LEFT");
				constexpr key_t DPAD_RIGHT = UF_KEY("DPAD_RIGHT");
				constexpr key_t A = UF_KEY("A");
				constexpr key_t B = UF_KEY("B");
				constexpr key_t X = UF_KEY("X");
				constexpr key_t Y = UF_KEY("Y");
				constexpr key_t L_TRIGGER = UF_KEY("L_TRIGGER");
				constexpr key_t R_TRIGGER = UF_KEY("R_TRIGGER");
				constexpr key_t START = UF_KEY("START");
			};
			namespace states {
			// L/R for index controllers
				extern UF_API state_t R_DPAD_UP;
				extern UF_API state_t R_DPAD_DOWN;
				extern UF_API state_t R_DPAD_LEFT;
				extern UF_API state_t R_DPAD_RIGHT;
				extern UF_API state_t R_A;
				extern UF_API state_t R_B;
				extern UF_API state_t R_X;
				extern UF_API state_t R_Y;

				extern UF_API state_t L_DPAD_UP;
				extern UF_API state_t L_DPAD_DOWN;
				extern UF_API state_t L_DPAD_LEFT;
				extern UF_API state_t L_DPAD_RIGHT;
				extern UF_API state_t L_A;
				extern UF_API state_t L_B;
				extern UF_API state_t L_X;
				extern UF_API state_t L_Y;
				
			// default controller mappings, aliased to L/R sides
				extern UF_API state_t& DPAD_UP; // = L_DPAD_UP;
				extern UF_API state_t& DPAD_DOWN; // = L_DPAD_DOWN;
				extern UF_API state_t& DPAD_LEFT; // = L_DPAD_LEFT;
				extern UF_API state_t& DPAD_RIGHT; // = L_DPAD_RIGHT;

				extern UF_API state_t& A; // = R_A;
				extern UF_API state_t& B; // = R_B;
				extern UF_API state_t& X; // = R_X;
				extern UF_API state_t& Y; // = R_Y;
			// analog inputs
				extern UF_API analog_t L_TRIGGER;
				extern UF_API analog_t R_TRIGGER;
				
				extern UF_API analog2_t L_JOYSTICK;
				extern UF_API analog2_t R_JOYSTICK;

				extern UF_API state_t START;
			};
		}
	}
}