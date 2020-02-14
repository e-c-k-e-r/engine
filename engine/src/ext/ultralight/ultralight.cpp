#include <uf/ext/ultralight/ultralight.h>
#if UF_USE_ULTRALIGHT
#include <AppCore/CAPI.h>

#include <uf/utils/io/iostream.h>
#include <uf/utils/hook/hook.h>
// std::string ext::ultralight::resourcesDir = "./data/resources/";

namespace {
	size_t uids = 0;
	ULRenderer renderer;

	std::vector<std::string> queue;
	void OnFinishLoading(void* user_data, ULView caller, unsigned long long frame_id, bool is_main_frame, ULString url) {
		if ( !is_main_frame ) return;
		pod::HTML& container = *((pod::HTML*) user_data);
		container.pending = true;
		if ( container.callbacks.load != "" ) {
			queue.emplace_back( container.callbacks.load );
			container.callbacks.load = "";
		}
	}
	void ConsoleMessageCallback(void* user_data, ULView caller, ULMessageSource _source, ULMessageLevel _level, ULString _message, unsigned int line_number, unsigned int column_number, ULString source_id) {
		std::string level = "?";
		std::string source = "?";
		switch ( _level ) {
			case ULMessageLevel::kMessageLevel_Log: level = "Log"; break;
			case ULMessageLevel::kMessageLevel_Warning: level = "Warning"; break;
			case ULMessageLevel::kMessageLevel_Error: level = "Error"; break;
			case ULMessageLevel::kMessageLevel_Debug: level = "Debug"; break;
			case ULMessageLevel::kMessageLevel_Info: level = "Info"; break;
		}
		switch ( _source ) {
			case ULMessageSource::kMessageSource_XML: source = "XML"; break;
			case ULMessageSource::kMessageSource_JS: source = "JS"; break;
			case ULMessageSource::kMessageSource_Network: source = "Network"; break;
			case ULMessageSource::kMessageSource_ConsoleAPI: source = "ConsoleAPI"; break;
			case ULMessageSource::kMessageSource_Storage: source = "Storage"; break;
			case ULMessageSource::kMessageSource_AppCache: source = "AppCache"; break;
			case ULMessageSource::kMessageSource_Rendering: source = "Rendering"; break;
			case ULMessageSource::kMessageSource_CSS: source = "CSS"; break;
			case ULMessageSource::kMessageSource_Security: source = "Security"; break;
			case ULMessageSource::kMessageSource_ContentBlocker: source = "ContentBlocker"; break;
			case ULMessageSource::kMessageSource_Other: source = "Other"; break;
		}
		uf::String message; {
			size_t len = ulStringGetLength(_message);
			const ULChar16* data = ulStringGetData(_message);
			for ( size_t i = 0; i < len; ++i ) message += data[i];
		}
		if ( _level + 1 >= ext::ultralight::log ) uf::iostream << "[UL-UX] [" << level << "] " << "[" << source << "] " << message << "\n";
	}
}

namespace UL {
	struct String {
		ULString handle;
		String( const std::string& str = "" ) {
			handle = ulCreateString( str.c_str() );
		}
		String( uint32_t c ) {
			handle = ulCreateStringUTF16( (uint16_t*) &c, 2 );
		}
		~String() {
			if ( !handle ) return;
			ulDestroyString(handle);
		}
		operator ULString() {
			return handle;
		}
	};

	uint32_t getModifiers( const uf::Serializer& event ) {
		uint32_t modifiers = 0;
		if ( event["alt"].as<bool>() ) modifiers |= 1 << 0;		// ULKeyEvent::kMod_AltKey;
		if ( event["control"].as<bool>() ) modifiers |= 1 << 1; // ULKeyEvent::kMod_CtrlKey;
		if ( event["system"].as<bool>() ) modifiers |= 1 << 2; 	// ULKeyEvent::kMod_MetaKey;
		if ( event["shift"].as<bool>() ) modifiers |= 1 << 3; 	// ULKeyEvent::kMod_ShiftKey;
		return modifiers;
	}

	struct KeyEvent {
		ULKeyEvent handle;
		KeyEvent( const std::string& payload ) {
			uf::Serializer event = payload;
			ULKeyEventType type;
			if ( event["key"]["state"].is<std::string>() ) {
				std::string s = event["key"]["state"].as<std::string>();
				if ( s == "Down" ) type = ULKeyEventType::kKeyEventType_RawKeyDown;
				else if ( s == "Up" ) type = ULKeyEventType::kKeyEventType_KeyUp;
			}
			if ( event["type"].as<std::string>() == "window:Text.Entered" ) {
				type = ULKeyEventType::kKeyEventType_Char;
				uint32_t modifiers = 0;
				int32_t virtualKeyCode = 0;
				int32_t nativeKeyCode = 0;
				bool isKeypad = false;
				bool isAutoRepeat = false;
				bool isSystemKey = false;

				handle = ulCreateKeyEvent( type, modifiers, virtualKeyCode, nativeKeyCode, UL::String(event["text"]["uint32_t"].as<size_t>()), UL::String(event["text"]["uint32_t"].as<size_t>()), isKeypad, isAutoRepeat, isSystemKey );
				return;
			}
		#if defined(_WIN32) && 0
		//	std::cout << "EVENT: " << event << std::endl;
			handle = ulCreateKeyEventWindows( type, event["key"]["wparam"].as<size_t>(), event["key"]["lparam"].as<size_t>(), event["key"]["modifier"]["system"].as<bool>());
		#else
			uint32_t modifiers = getModifiers( event["key"]["modifier"] );
			int32_t virtualKeyCode = event["key"]["raw"].as<size_t>();
			int32_t nativeKeyCode = event["key"]["lparam"].as<size_t>();
			std::string text = ""; //event["key"]["code"].as<std::string>();
			std::string unmodifiedText = ""; //event["key"]["code"].as<std::string>();
			bool isKeypad = false;
			bool isAutoRepeat = false;
			bool isSystemKey = event["key"]["modifier"]["system"].as<bool>();

			handle = ulCreateKeyEvent( type, modifiers, virtualKeyCode, nativeKeyCode, UL::String(text), UL::String(unmodifiedText), isKeypad, isAutoRepeat, isSystemKey );
		#endif
		}
		~KeyEvent() {
			if ( !handle ) return;
			ulDestroyKeyEvent(handle);
		}
		operator ULKeyEvent() {
			return handle;
		}
	};
	struct MouseEvent {
		ULMouseEvent handle;
		MouseEvent( const std::string& payload ) {
			uf::Serializer event = payload;
			ULMouseEventType type = ULMouseEventType::kMouseEventType_MouseMoved;
			ULMouseButton button = ULMouseButton::kMouseButton_None;

			if ( event["mouse"]["state"].is<std::string>() ) {
				std::string s = event["mouse"]["state"].as<std::string>();
				if ( s == "Up" ) type = ULMouseEventType::kMouseEventType_MouseUp;
				else if ( s == "Down" ) type = ULMouseEventType::kMouseEventType_MouseDown;
			}
			if ( event["mouse"]["button"].is<std::string>() ) {
				std::string s = event["mouse"]["button"].as<std::string>();
				if ( s == "Left" ) button = ULMouseButton::kMouseButton_Left;
				else if ( s == "Middle" ) button = ULMouseButton::kMouseButton_Middle;
				else if ( s == "Right" ) button = ULMouseButton::kMouseButton_Right;
			}
			int x = event["mouse"]["position"]["x"].as<int>() / ext::ultralight::scale;
			int y = event["mouse"]["position"]["y"].as<int>() / ext::ultralight::scale;
			handle = ulCreateMouseEvent( type, x, y, button );
		}
		~MouseEvent() {
			if ( !handle ) return;
			ulDestroyMouseEvent(handle);
		}
		operator ULMouseEvent() {
			return handle;
		}		
	};
	struct ScrollEvent {
		ULScrollEvent handle;
		ScrollEvent( const std::string& payload ) {
			uf::Serializer event = payload;
			ULScrollEventType type = ULScrollEventType::kScrollEventType_ScrollByPixel;
			handle = ulCreateScrollEvent( type, 0, event["mouse"]["delta"].as<int>() );
		}
		~ScrollEvent() {
			if ( !handle ) return;
			ulDestroyScrollEvent(handle);
		}
		operator ULScrollEvent() {
			return handle;
		}
	};
}

uint8_t ext::ultralight::log = 0;
double ext::ultralight::scale = 1.0;

void ext::ultralight::initialize() {
	ULConfig config = ulCreateConfig();

	ulConfigSetDeviceScale(config, ext::ultralight::scale);

	ulConfigSetFontFamilyStandard(config, UL::String("Arial"));
	ulConfigSetResourcePath(config, UL::String("./data/html/resources/"));
	ulConfigSetUseGPURenderer(config, false);
	ulEnablePlatformFontLoader();
	ulEnablePlatformFileSystem(UL::String("./data/html/assets/"));
	ulEnableDefaultLogger(UL::String("./data/logs/ultralight.log"));


	::renderer = ulCreateRenderer(config);
}
void ext::ultralight::tick() {
	for ( auto& callback : queue ) uf::hooks.call(callback);
	queue.clear();

	ulUpdate(renderer);
}
void ext::ultralight::render() {
	ulRender(renderer);
}
void ext::ultralight::terminate() {
	ulDestroyRenderer(renderer);
}

pod::HTML ext::ultralight::create( const std::string& html, const pod::Vector2ui& size ) {
	pod::HTML container;
	return create( container, html, size );
}
pod::HTML& ext::ultralight::create( pod::HTML& container, const std::string& html, const pod::Vector2ui& size ) {
	container.uid = ++uids;
	container.view = ulCreateView(::renderer, size.x, size.y, false, 0, false);
	container.size = size;

	// is URI
	if ( html.find("://") != std::string::npos ) {
		ulViewLoadURL(container.view, UL::String(html));
	// is filename
	} else if ( uf::io::extension(html) == "html" ) {
		std::string html = uf::io::readAsString( uf::io::resolveURI( html ) );
		ulViewLoadHTML(container.view, UL::String(html));
	// is HTML
	} else {
		ulViewLoadHTML(container.view, UL::String(html));
	}

	ulViewSetFinishLoadingCallback(container.view, &OnFinishLoading, &container);
	ulViewSetAddConsoleMessageCallback(container.view, &ConsoleMessageCallback, &container);

	return container;
}
void ext::ultralight::destroy( pod::HTML& container ) {
	ulDestroyView(container.view);
}
void ext::ultralight::on( pod::HTML& container, const std::string& name, const std::string& hook ) {
	if ( name == "load" ) container.callbacks.load = hook;
}
void ext::ultralight::resize( pod::HTML& container, const pod::Vector2ui& size ) {
	container.size = size;
	container.pending = true;
	ulViewResize( container.view, size.x, size.y );
}
void ext::ultralight::input( pod::HTML& container, const std::string& payload ) {
	uf::Serializer event = payload;

	// std::cout << payload << std::endl;
	if  ( event["type"].as<std::string>() == "window:Key.Pressed" || event["type"].as<std::string>() == "window:Key.Released" || event["type"].as<std::string>() == "window:Text.Entered" ) {
		ulViewFireKeyEvent( container.view, UL::KeyEvent( payload ) );
	} else if ( event["type"].as<std::string>() == "window:Mouse.Moved" || event["type"].as<std::string>() == "window:Mouse.Click" ) {
		ulViewFireMouseEvent( container.view, UL::MouseEvent( payload ) );
	} else if ( event["type"].as<std::string>() == "window:Mouse.Wheel" )	{
		ulViewFireScrollEvent( container.view, UL::ScrollEvent( payload ) );
	} else {
	//	std::cout << "Unknown event: " << event << std::endl;
	}
	container.pending = true;
}
uf::Image ext::ultralight::capture( pod::HTML& container ) {
	uf::Image image;

	ULSurface surface = ulViewGetSurface(container.view);
	ULBitmap bitmap = ulBitmapSurfaceGetBitmap(surface);
	ulBitmapSwapRedBlueChannels(bitmap);
	ulBitmapLockPixels(bitmap);
	image.loadFromBuffer( (uint8_t*) ulBitmapRawPixels(bitmap), {ulBitmapGetWidth(bitmap), ulBitmapGetHeight(bitmap)}, ulBitmapGetBpp(bitmap) * 2, 4, true );
	ulBitmapUnlockPixels(bitmap);
	ulBitmapSwapRedBlueChannels(bitmap);
	return image;
}
#endif