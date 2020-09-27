#include "behavior_test.h"

#include <iostream>

#define this (&self)
void ext::TestBehavior::attach( uf::Object& self ) {
	self.addBehavior({
		.type = uf::Behaviors::getType<ext::TestBehavior>(),
		.initialize = [&]() {
			ext::TestBehavior::initialize( self );
		},
		.tick = [&]() {
			ext::TestBehavior::tick( self );
		},
		.render = [&]() {
			ext::TestBehavior::render( self );
		},
		.destroy = [&]() {
			ext::TestBehavior::destroy( self );
		},
	});
}
void ext::TestBehavior::initialize( uf::Object& self ) {

}
void ext::TestBehavior::tick( uf::Object& self ) {

}
void ext::TestBehavior::render( uf::Object& self ) {

}
void ext::TestBehavior::destroy( uf::Object& self ) {

}
#undef this