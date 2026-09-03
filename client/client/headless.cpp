#include "headless.h"

#include <uf/utils/io/console.h>

void client::headless::configure() {
	if ( !active() ) return;
	uf::config["window"]["refresh rate"] = 60;
	uf::config["engine"]["audio"]["mute"] = true;
	uf::config["engine"]["debug"]["loader"]["assert"] = false;
}

void client::headless::tick() {
	if ( !active() ) return;
	uf::console::pump();
}

void client::headless::terminate() {
	uf::console::terminate();
}