template<typename T>
void spec::uni::Window::pushEvent( const uf::Hooks::name_t& name, const T& payload ) {
	pod::Hook::userdata_t userdata;
	userdata.create<T>( payload );
	this->pushEvent( name, userdata );
}