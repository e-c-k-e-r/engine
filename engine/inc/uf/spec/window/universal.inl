template<typename T>
void spec::uni::Window::pushEvent( const uf::Hooks::name_t& name, const T& payload ) {
	auto& event = this->m_events.emplace();
	event.name = name;
	event.payload.create<T>(payload);
}