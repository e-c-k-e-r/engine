#include <uf/spec/controller/controller.h>
#include <uf/utils/string/ext.h>

#ifdef UF_ENV_WINDOWS
#if UF_USE_OPENVR
	#include <uf/ext/openvr/openvr.h>
#endif

void spec::win32::controller::initialize() {}
void spec::win32::controller::tick() {
#if 0 || UF_USE_OPENVR
	if ( ext::openvr::context ) {
		uf::stl::string key = "";
		if ( name == "R_DPAD_UP" ) { i = vr::Controller_Hand::Hand_Right; key = "dpadUp"; }
		else if ( name == "R_DPAD_DOWN" ) { i = vr::Controller_Hand::Hand_Right; key = "dpadDown"; }
		else if ( name == "R_DPAD_LEFT" ) { i = vr::Controller_Hand::Hand_Right; key = "dpadLeft"; }
		else if ( name == "R_DPAD_RIGHT" ) { i = vr::Controller_Hand::Hand_Right; key = "dpadRight"; }
		else if ( name == "R_JOYSTICK" ) { i = vr::Controller_Hand::Hand_Right; key = "thumbclick"; }
		else if ( name == "R_A" ) { i = vr::Controller_Hand::Hand_Right; key = "a"; }
		else if ( name == "R_B" ) { i = vr::Controller_Hand::Hand_Right; key = "b"; }
		else if ( name == "DPAD_UP" && i == vr::Controller_Hand::Hand_Right ) key = "dpadUp";
		else if ( name == "DPAD_DOWN" && i == vr::Controller_Hand::Hand_Right ) key = "dpadDown";
		else if ( name == "DPAD_LEFT" && i == vr::Controller_Hand::Hand_Right ) key = "dpadLeft";
		else if ( name == "DPAD_RIGHT" && i == vr::Controller_Hand::Hand_Right ) key = "dpadRight";
		else if ( name == "JOYSTICK" && i == vr::Controller_Hand::Hand_Right ) key = "thumbclick";
		else if ( name == "A" && i == vr::Controller_Hand::Hand_Right ) key = "a";
		else if ( name == "B" && i == vr::Controller_Hand::Hand_Right ) key = "b";

		else if ( name == "L_DPAD_UP" ) { i = vr::Controller_Hand::Hand_Left; key = "dpadUp"; }
		else if ( name == "L_DPAD_DOWN" ) { i = vr::Controller_Hand::Hand_Left; key = "dpadDown"; }
		else if ( name == "L_DPAD_LEFT" ) { i = vr::Controller_Hand::Hand_Left; key = "dpadLeft"; }
		else if ( name == "L_DPAD_RIGHT" ) { i = vr::Controller_Hand::Hand_Left; key = "dpadRight"; }
		else if ( name == "L_JOYSTICK" ) { i = vr::Controller_Hand::Hand_Left; key = "thumbclick"; }
		else if ( name == "L_A" ) { i = vr::Controller_Hand::Hand_Left; key = "a"; }
		else if ( name == "L_B" ) { i = vr::Controller_Hand::Hand_Left; key = "b"; }
		else if ( name == "DPAD_UP" && i == vr::Controller_Hand::Hand_Left ) key = "dpadUp";
		else if ( name == "DPAD_DOWN" && i == vr::Controller_Hand::Hand_Left ) key = "dpadDown";
		else if ( name == "DPAD_LEFT" && i == vr::Controller_Hand::Hand_Left ) key = "dpadLeft";
		else if ( name == "DPAD_RIGHT" && i == vr::Controller_Hand::Hand_Left ) key = "dpadRight";
		else if ( name == "JOYSTICK" && i == vr::Controller_Hand::Hand_Left ) key = "thumbclick";
		else if ( name == "A" && i == vr::Controller_Hand::Hand_Left ) key = "a";
		else if ( name == "B" && i == vr::Controller_Hand::Hand_Left ) key = "b";

		if ( name != "" ) return ext::openvr::controllerState( i == 0 ? vr::Controller_Hand::Hand_Left : vr::Controller_Hand::Hand_Right, key )["state"].as<bool>();
	}
	if ( ext::openvr::context ) {
		size_t offset = 0;
		uf::stl::string key = "";
		if ( name == "R_JOYSTICK_X" ) { i = vr::Controller_Hand::Hand_Right; key = "thumbstick"; offset = 0; }
		else if ( name == "JOYSTICK_X" && i == vr::Controller_Hand::Hand_Right ) { key = "thumbstick"; offset = 0;}
		else if ( name == "R_JOYSTICK_Y" ) { i = vr::Controller_Hand::Hand_Right; key = "thumbstick"; offset = 1; }
		else if ( name == "JOYSTICK_Y" && i == vr::Controller_Hand::Hand_Right ) { key = "thumbstick"; offset = 1; }
		
		else if ( name == "L_JOYSTICK_X" ) { i = vr::Controller_Hand::Hand_Left; key = "thumbstick"; offset = 0; }
		else if ( name == "JOYSTICK_X" && i == vr::Controller_Hand::Hand_Left ) { key = "thumbstick"; offset = 0;}
		else if ( name == "L_JOYSTICK_Y" ) { i = vr::Controller_Hand::Hand_Left; key = "thumbstick"; offset = 1; }
		else if ( name == "JOYSTICK_Y" && i == vr::Controller_Hand::Hand_Left ) { key = "thumbstick"; offset = 1; }

		if ( name != "" ) return ext::openvr::controllerState( i == 0 ? vr::Controller_Hand::Hand_Left : vr::Controller_Hand::Hand_Right, key )["analog"]["position"][offset].as<float>();
	}
#endif
}
void spec::win32::controller::terminate() {}
bool spec::win32::controller::connected( size_t i ) {
#if UF_USE_OPENVR
	if ( ext::openvr::controllerActive( i == 0 ? vr::Controller_Hand::Hand_Left : vr::Controller_Hand::Hand_Right ) ) return true;
#endif
	return false;
}
/*
bool spec::win32::controller::pressed( const uf::stl::string& _name, size_t i ) {
	uf::stl::string name = uf::string::uppercase(_name);
#if UF_USE_OPENVR
	if ( ext::openvr::context ) {
		uf::stl::string key = "";
		if ( name == "R_DPAD_UP" ) { i = vr::Controller_Hand::Hand_Right; key = "dpadUp"; }
		else if ( name == "R_DPAD_DOWN" ) { i = vr::Controller_Hand::Hand_Right; key = "dpadDown"; }
		else if ( name == "R_DPAD_LEFT" ) { i = vr::Controller_Hand::Hand_Right; key = "dpadLeft"; }
		else if ( name == "R_DPAD_RIGHT" ) { i = vr::Controller_Hand::Hand_Right; key = "dpadRight"; }
		else if ( name == "R_JOYSTICK" ) { i = vr::Controller_Hand::Hand_Right; key = "thumbclick"; }
		else if ( name == "R_A" ) { i = vr::Controller_Hand::Hand_Right; key = "a"; }
		else if ( name == "R_B" ) { i = vr::Controller_Hand::Hand_Right; key = "b"; }
		else if ( name == "DPAD_UP" && i == vr::Controller_Hand::Hand_Right ) key = "dpadUp";
		else if ( name == "DPAD_DOWN" && i == vr::Controller_Hand::Hand_Right ) key = "dpadDown";
		else if ( name == "DPAD_LEFT" && i == vr::Controller_Hand::Hand_Right ) key = "dpadLeft";
		else if ( name == "DPAD_RIGHT" && i == vr::Controller_Hand::Hand_Right ) key = "dpadRight";
		else if ( name == "JOYSTICK" && i == vr::Controller_Hand::Hand_Right ) key = "thumbclick";
		else if ( name == "A" && i == vr::Controller_Hand::Hand_Right ) key = "a";
		else if ( name == "B" && i == vr::Controller_Hand::Hand_Right ) key = "b";

		else if ( name == "L_DPAD_UP" ) { i = vr::Controller_Hand::Hand_Left; key = "dpadUp"; }
		else if ( name == "L_DPAD_DOWN" ) { i = vr::Controller_Hand::Hand_Left; key = "dpadDown"; }
		else if ( name == "L_DPAD_LEFT" ) { i = vr::Controller_Hand::Hand_Left; key = "dpadLeft"; }
		else if ( name == "L_DPAD_RIGHT" ) { i = vr::Controller_Hand::Hand_Left; key = "dpadRight"; }
		else if ( name == "L_JOYSTICK" ) { i = vr::Controller_Hand::Hand_Left; key = "thumbclick"; }
		else if ( name == "L_A" ) { i = vr::Controller_Hand::Hand_Left; key = "a"; }
		else if ( name == "L_B" ) { i = vr::Controller_Hand::Hand_Left; key = "b"; }
		else if ( name == "DPAD_UP" && i == vr::Controller_Hand::Hand_Left ) key = "dpadUp";
		else if ( name == "DPAD_DOWN" && i == vr::Controller_Hand::Hand_Left ) key = "dpadDown";
		else if ( name == "DPAD_LEFT" && i == vr::Controller_Hand::Hand_Left ) key = "dpadLeft";
		else if ( name == "DPAD_RIGHT" && i == vr::Controller_Hand::Hand_Left ) key = "dpadRight";
		else if ( name == "JOYSTICK" && i == vr::Controller_Hand::Hand_Left ) key = "thumbclick";
		else if ( name == "A" && i == vr::Controller_Hand::Hand_Left ) key = "a";
		else if ( name == "B" && i == vr::Controller_Hand::Hand_Left ) key = "b";

		if ( name != "" ) return ext::openvr::controllerState( i == 0 ? vr::Controller_Hand::Hand_Left : vr::Controller_Hand::Hand_Right, key )["state"].as<bool>();
	}
#endif
	return false;
}
float spec::win32::controller::analog( const uf::stl::string& _name, size_t i ) {
	uf::stl::string name = uf::string::uppercase(_name);
#if UF_USE_OPENVR
	if ( ext::openvr::context ) {
		size_t offset = 0;
		uf::stl::string key = "";
		if ( name == "R_JOYSTICK_X" ) { i = vr::Controller_Hand::Hand_Right; key = "thumbstick"; offset = 0; }
		else if ( name == "JOYSTICK_X" && i == vr::Controller_Hand::Hand_Right ) { key = "thumbstick"; offset = 0;}
		else if ( name == "R_JOYSTICK_Y" ) { i = vr::Controller_Hand::Hand_Right; key = "thumbstick"; offset = 1; }
		else if ( name == "JOYSTICK_Y" && i == vr::Controller_Hand::Hand_Right ) { key = "thumbstick"; offset = 1; }
		
		else if ( name == "L_JOYSTICK_X" ) { i = vr::Controller_Hand::Hand_Left; key = "thumbstick"; offset = 0; }
		else if ( name == "JOYSTICK_X" && i == vr::Controller_Hand::Hand_Left ) { key = "thumbstick"; offset = 0;}
		else if ( name == "L_JOYSTICK_Y" ) { i = vr::Controller_Hand::Hand_Left; key = "thumbstick"; offset = 1; }
		else if ( name == "JOYSTICK_Y" && i == vr::Controller_Hand::Hand_Left ) { key = "thumbstick"; offset = 1; }

		if ( name != "" ) return ext::openvr::controllerState( i == 0 ? vr::Controller_Hand::Hand_Left : vr::Controller_Hand::Hand_Right, key )["analog"]["position"][offset].as<float>();
	}
#endif
	return 0;
}
*/
#endif