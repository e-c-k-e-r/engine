#include <uf/spec/controller/controller.h>
#ifdef UF_ENV_DREAMCAST

#include <kos.h>
#include <limits>
#include <uf/utils/string/ext.h>
#include <uf/utils/math/vector.h>

#include <uf/utils/io/inputs.h>

#define NORMALIZE(X) ((float) (X) / (float) std::numeric_limits<decltype(X)>::max())

namespace {
	struct {
		maple_device_t* device = NULL;
		cont_state_t* state = NULL;
	} controller;

	uf::stl::string GetKeyName( uint32_t code ) {
		switch ( code ) {
			case CONT_C: return "C";
			case CONT_B: return "B";
			case CONT_A: return "A";
			case CONT_START: return "START";
			case CONT_DPAD_UP: return "L_DPAD_UP";
			case CONT_DPAD_DOWN: return "L_DPAD_DOWN";
			case CONT_DPAD_LEFT: return "L_DPAD_LEFT";
			case CONT_DPAD_RIGHT: return "L_DPAD_RIGHT";
			case CONT_Z: return "Z";
			case CONT_Y: return "Y";
			case CONT_X: return "X";
			case CONT_D: return "D";
			case CONT_DPAD2_UP: return "R_DPAD_UP";
			case CONT_DPAD2_DOWN: return "R_DPAD_DOWN";
			case CONT_DPAD2_LEFT: return "R_DPAD_LEFT";
			case CONT_DPAD2_RIGHT: return "R_DPAD_RIGHT";
		}
		return "";
	}
	uint32_t GetKeyCode( const uf::stl::string& _name ) {
		uf::stl::string name = uf::string::uppercase( _name );
		if ( name == "C" ) return CONT_C;
		else if ( name == "B" ) return CONT_B;
		else if ( name == "A" ) return CONT_A;
		else if ( name == "START" ) return CONT_START;
		else if ( name == "L_DPAD_UP" ) return CONT_DPAD_UP;
		else if ( name == "L_DPAD_DOWN" ) return CONT_DPAD_DOWN;
		else if ( name == "L_DPAD_LEFT" ) return CONT_DPAD_LEFT;
		else if ( name == "L_DPAD_RIGHT" ) return CONT_DPAD_RIGHT;
		else if ( name == "Z" ) return CONT_Z;
		else if ( name == "Y" ) return CONT_Y;
		else if ( name == "X" ) return CONT_X;
		else if ( name == "D" ) return CONT_D;
		else if ( name == "R_DPAD_UP" ) return CONT_DPAD2_UP;
		else if ( name == "R_DPAD_DOWN" ) return CONT_DPAD2_DOWN;
		else if ( name == "R_DPAD_LEFT" ) return CONT_DPAD2_LEFT;
		else if ( name == "R_DPAD_RIGHT" ) return CONT_DPAD2_RIGHT;
		else if ( name == "DPAD_UP" ) return CONT_DPAD_UP;
		else if ( name == "DPAD_DOWN" ) return CONT_DPAD_DOWN;
		else if ( name == "DPAD_LEFT" ) return CONT_DPAD_LEFT;
		else if ( name == "DPAD_RIGHT" ) return CONT_DPAD_RIGHT;
		return 0;
	}
}

void spec::dreamcast::controller::initialize() {
	::controller.device = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
}
void spec::dreamcast::controller::tick() {
	if ( !::controller.device ) ::controller.device = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
	if ( ::controller.device ) ::controller.state = (cont_state_t*) maple_dev_status(::controller.device);

	if ( !::controller.state ) return;

	uf::inputs::controller::states::A = ::controller.state->buttons & CONT_A;
	uf::inputs::controller::states::B = ::controller.state->buttons & CONT_B;
	uf::inputs::controller::states::X = ::controller.state->buttons & CONT_X;
	uf::inputs::controller::states::Y = ::controller.state->buttons & CONT_Y;
	
	uf::inputs::controller::states::START = ::controller.state->buttons & CONT_START;
	
	uf::inputs::controller::states::DPAD_UP = ::controller.state->buttons & CONT_DPAD_UP;
	uf::inputs::controller::states::DPAD_DOWN = ::controller.state->buttons & CONT_DPAD_DOWN;
	uf::inputs::controller::states::DPAD_LEFT = ::controller.state->buttons & CONT_DPAD_LEFT;
	uf::inputs::controller::states::DPAD_RIGHT = ::controller.state->buttons & CONT_DPAD_RIGHT;

//	uf::inputs::controller::states::C = ::controller.state->buttons & CONT_C;
//	uf::inputs::controller::states::D = ::controller.state->buttons & CONT_D;
//	uf::inputs::controller::states::Z = ::controller.state->buttons & CONT_Z;

	uf::inputs::controller::states::L_TRIGGER = NORMALIZE(::controller.state->ltrig); 
	uf::inputs::controller::states::R_TRIGGER = NORMALIZE(::controller.state->rtrig); 

	uf::inputs::controller::states::L_JOYSTICK.x = NORMALIZE(::controller.state->joyx);
	uf::inputs::controller::states::L_JOYSTICK.y = NORMALIZE(::controller.state->joyy);

	uf::inputs::controller::states::R_JOYSTICK.x = NORMALIZE(::controller.state->joy2x);
	uf::inputs::controller::states::R_JOYSTICK.y = NORMALIZE(::controller.state->joy2y);
}
void spec::dreamcast::controller::terminate() {
}
bool spec::dreamcast::controller::connected( size_t i ) {
	return ::controller.state;
}
/*
bool spec::dreamcast::controller::pressed( const uf::stl::string& str, size_t i ) {
	if ( !::controller.state ) return false;
	return ::controller.state->buttons & GetKeyCode( str );
}
float spec::dreamcast::controller::analog( const uf::stl::string& str, size_t i ) {
	if ( !::controller.state ) return false;
	
	if ( str == "L_TRIGGER" ) return NORMALIZE(::controller.state->ltrig); 
	else if ( str == "R_TRIGGER" ) return NORMALIZE(::controller.state->rtrig); 
	else if ( str == "JOYSTICK_X" ) return NORMALIZE(::controller.state->joyx);
	else if ( str == "JOYSTICK_Y" ) return NORMALIZE(::controller.state->joyy);
	else if ( str == "L_JOYSTICK_X" ) return NORMALIZE(::controller.state->joyx);
	else if ( str == "L_JOYSTICK_Y" ) return NORMALIZE(::controller.state->joyy);
	else if ( str == "R_JOYSTICK_X" ) return NORMALIZE(::controller.state->joy2x);
	else if ( str == "R_JOYSTICK_Y" ) return NORMALIZE(::controller.state->joy2y);
	return 0.0f;
}
*/
#endif