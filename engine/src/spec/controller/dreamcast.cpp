#include <uf/spec/controller/controller.h>
#ifdef UF_ENV_DREAMCAST

#include <kos.h>
#include <limits>
#include <uf/utils/string/ext.h>
#include <uf/utils/math/vector.h>
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
#if 0
	uf::stl::vector<uf::stl::string> str;
	pod::Vector2f joystick = { analog("L_JOYSTICK_X"), analog("L_JOYSTICK_Y") };
	if ( joystick ) str.emplace_back("Joystick: " + uf::vector::toString(joystick));
	if ( pressed("C") ) str.emplace_back("C");
	if ( pressed("B") ) str.emplace_back("B");
	if ( pressed("A") ) str.emplace_back("A");
	if ( pressed("START") ) str.emplace_back("START");
	if ( pressed("L_DPAD_UP") ) str.emplace_back("L_DPAD_UP");
	if ( pressed("L_DPAD_DOWN") ) str.emplace_back("L_DPAD_DOWN");
	if ( pressed("L_DPAD_LEFT") ) str.emplace_back("L_DPAD_LEFT");
	if ( pressed("L_DPAD_RIGHT") ) str.emplace_back("L_DPAD_RIGHT");
	if ( pressed("Z") ) str.emplace_back("Z");
	if ( pressed("Y") ) str.emplace_back("Y");
	if ( pressed("X") ) str.emplace_back("X");
	if ( pressed("D") ) str.emplace_back("D");
	if ( pressed("R_DPAD_UP") ) str.emplace_back("R_DPAD_UP");
	if ( pressed("R_DPAD_DOWN") ) str.emplace_back("R_DPAD_DOWN");
	if ( pressed("R_DPAD_LEFT") ) str.emplace_back("R_DPAD_LEFT");
	if ( pressed("R_DPAD_RIGHT") ) str.emplace_back("R_DPAD_RIGHT");
	if ( !str.empty() ) UF_MSG_DEBUG(uf::string::join( str, " | " ));
#endif
}
void spec::dreamcast::controller::terminate() {
}
bool spec::dreamcast::controller::connected( size_t i ) {
	return ::controller.state;
}
bool spec::dreamcast::controller::pressed( const uf::stl::string& str, size_t i ) {
	if ( !::controller.state ) return false;
	return ::controller.state->buttons & GetKeyCode( str );
}
float spec::dreamcast::controller::analog( const uf::stl::string& str, size_t i ) {
#define NORMALIZE(X) ((float) (X) / (float) std::numeric_limits<decltype(X)>::max())
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

#endif