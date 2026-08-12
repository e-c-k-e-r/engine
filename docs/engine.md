# `engine/`

This section of the project handles all the systems and structs needed for the engine.

This is further divided into four parts:
* `utils`: lower level systems and structs that the engine may need
* `engine`: contains the highest level of objects used for the engine
* `ext`: any abstraction that implements an external library
* `spec`: any abstraction that requires specialization / platform-specific code

Confusingly, these namespaces are exposed:
* `pod`: (mostly) contains plain-old-data structs, as they're safe to directly interact with its members, and get operated on through functions under `uf::{namespace}`
* `uf`: contains complex classes whose underlying members are not safe to directly interact with; contains their own member functions
* `ext`: contains code that directly interacts with external libraries; typically shouldn't be directly invoked

This documentation is barebones as actual more examples and detailing of the functions provided are required, rather than just the classes and namespaces exposed. However, some examples are provided.

## `engine/utils/`

### `engine/utils/audio/`

This provides the `uf::Audio` and `uf::AudioEmitter` classes.

### `engine/utils/camera/`

This provides the `uf::Camera` class.

### `engine/utils/component/`

This provides the `uf::Component` system, allowing an entity to own `pod::Component<T>`s.

A `uf::Component` maps a `pod::Component<T>` by its underlying userdata typing.

`.hasComponent<T>()` can be used to check if an entity has a component.

By default, `.getComponent<T>()` will automatically invoke `.addComponent<T>()` if that component isn't already added.

Example:

```
auto& transform = this->getComponent<pod::Transform<>>();
auto& camera = this->getComponent<uf::Camera>();

// only do something if there's already a physics state
if ( this->hasComponent<pod::PhysicsState>() ) {
	auto& physics = this->getComponent<pod::PhysicsState>();
	// ...
}
```

## `engine/utils/debug/`

This provides debug functions for rendering lines, shapes (via lines), and text, via an immediate mode rather than a retained mode.

Simply call `uf::debug::drawLine` or `uf::debug::drawShape`, or `uf::debug::drawText` once a frame, and it'll render into the engine's world.

Additionally, `uf::checkpoint` is handled here as well, but only used for Vulkan (once upon a time at least).

### `engine/utils/hook/`

This provides the `uf::hooks` system, allowing data and events to dispatch across the engine to anyone listening to the hook name and payload type.

Confusingly, this is a singleton and not a namespace to allow for separate hook pools to be utilized, although this usecase is currently not used.

While it's not usually used this way, return types from a hook are collected into a `uf::stl::vector<uf::Userdata>` returned by `uf::hooks.callHook`.

Arguments passed by non-`const` reference can mutate along for other hooks to see.

Examples:
```
// add a hook
uf::hooks.addHook( "llm:VALL-E.synthesize", [&](ext::json::Value& json){
	auto text = json["text"].as<uf::stl::string>();
	auto prom = json["prom"].as<uf::stl::string>();
	auto play = json["play"].as<bool>();
	auto callback = json["callback"].as<uf::stl::string>("");

	uf::thread::queue( uf::thread::asyncThreadName, [=](){
		auto waveform = ext::vall_e::generate( text, prom );
		if ( callback != "" ) {
			uf::hooks.call( callback, waveform );
		} else if ( play ) {
			uf::Audio audio;
			audio.load( waveform );
			audio.play();
		}
	});
});

// dispatch via JSON
ext::json::Value payload;
payload["text"] = "The birch canoe slid on the smooth planks.";
payload["prom"] = "./data/tmp/prom.wav";
payload["play"] = true;
uf::hooks.call( "llm:VALL-E.synthesize", payload );
```

```
// or through a payload struct
struct TTSPayload {
	uf::stl::string text;
	uf::stl::string prom;
};

uf::hooks.addHook( "llm:VALL-E.synthesize", [&]( TTSPayload& payload ){
	return ext::vall_e::generate( payload.text, payload.prom );
});

auto waveform = uf::hooks.callHook("llm:VALL-E.synthesize", {
	"The birch canoe slid on the smooth planks.", "./data/tmp/prom.wav"
})[0];
```

### `engine/utils/http/`

This provides the `uf::http` system through cURL.

Basic `GET`, `POST`, and `HEAD` are supported.

This can (should) also be accessed through the VFS via reading/writing to `https://${url}`.

### `engine/utils/image/`

This provides the `uf::Image` class (wraps `pod::Image`) for storing image data.

Images are loaded as `RGBA8` buffers using [nothings/stb](https://github.com/nothings/stb)'s `stb_image`.

#### `engine/utils/image/atlas`

This provides the `uf::Atlas` class (wraps `pod::Image`) for storing images into an atlas, using [nothings/stb](https://github.com/nothings/stb/blob/master/stb_rect_pack.h) as the packer.

### `engine/utils/io/`

This folder provies:
* `uf::console`
	* a barebone REPL for interfacing with the engine via commands
* `uf::io`
	* higher level access of I/O interfacing
* `uf::vfs`
	* lower-ish level access to the underlying VFS
* `uf::inputs`
	* exposes values from KB/M or controllers

### `engine/utils/math/`

This folder provides:
* `pod::Vector<>`
	* examples: `pod::Vector3f`, `pod::Vector4f`, `pod::Vector2ui`, `pod::Vector<6, char>`
* `pod::Quaternion<>`
* `pod::Matrix<>`
	* example: `pod::Matrix4f`
* `pod::Transform<>`
* `uf::quant`
* `uf::physics`
* and other things

To-do: document the `uf::vector`, `uf::quaternion`, `uf::matrix`, `uf::transform` namespaces

To-do: document the `uf::physics` system

### `engine/utils/memory/`

This folder provides wrappers for STL containers to optionally integrate with the internal memory pools:
* `uf::stl::vector`
* `uf::stl::unordered_map`
* `uf::stl::fifo_map`
* `uf::stl::KeyMap`
* `uf::stl::deque`
* `uf::stl::stack`
* `uf::stl::queue`
* `uf::stl::map`

And the memory pool through `uf::memoryPool`.

### `engine/utils/mesh/`

This contains
* a struct for attribute descriptors
* vertex descriptors for meshes
* structs for draw commands, instances, and primitives
* a generic class `uf::Mesh` for inserting and binding vertex data for rendering
* views for reading/writing to each submesh (defined as `pod::DrawCommand`s in the `indirect` buffer)

#### `engine/mesh/grid`

This contains `uf::meshgrid` for slicing a mesh into grids.

This is used during processing external model formats to the `worldspawn` mesh when importing into the engine's internal format.

This *can* be invoked outside of this context, but currently does is not used beyond the above use-case.

### `engine/utils/noise/`

This provides noising functions, such as perlin noise through `uf::PerlinNoise`.

However, this implementation is quite cringe as it's from 2013 or 2014.

### `engine/utils/resolveable/`

This provides a wrapper class `pod::Resolveable<T>` that either returns its ID or its pointer type. This is seldomly used for handling entity information, where it can be defined by its UID or pointer directly.

### `engine/utils/serialize/`

This provides `uf::Serializer`, a wrapper around `ext::json::Value` to read from / write to disk.

For the most part, this is used interchangeably with `ext::json::Value`.

### `engine/utils/singletons/`

This provides `uf::StaticInitialization`, a cringe way to implement pre-`main()` static initialization through singletons.

This is primarily used for registering component enums through Lua.

### `engine/utils/string/`

This provides a myriad of string functions through `uf::string`.

### `engine/utils/tests/`

This handles statically initialized unit tests, primarily to assert interoperability between platforms, and regression testing.

### `engine/utils/text/`

This provides `uf::Glyph`, a wrapper around [nothings/stb](https://github.com/nothings/stb/blob/master/stb_truetype.h)'s `stb_truetype` for holding glyph information, and handles generating its corresponding mesh and texture atlas.

This shouldn't directly be interfaced with, as the GUI system handles this.

### `engine/utils/thread/`

This provides `uf::thread` system for multithreading.

For the most part, this should only be interfaced per the example:
```
// something asynchronously
uf::thread::queue( uf::thread::asyncThreadName, [=](){
	// ...
});
```
```
// something via a worker thread
uf::thread::queue( uf::thread::fetchWorker(), [=](){
	// ...
});
```
```
auto tasks = uf::thread::schedule(true); // true to multithread, false to not
tasks.queue([&](){
	// ...
})
uf::thread::execute( tasks );
```

### `engine/utils/time/`

This provides `uf::Timer<>` and `uf::time`, time and timer related functions.

For the most part, timers are used via a macro, and deltaTime is grabbed through `uf::physics::time::delta`.

### `engine/utils/userdata/`

This provides `uf::userdata` and `uf::pointeredUserdata` (to-do: alias the latter into the former, as the former is just a gimmick), where `pod::Userdata` (and the `uf::Userdata` wrapper) contains arbitrary userdata identified by its size and type.

For the most part, this shouldn't directly be interfaced, as this is used for other systems to operate, but an example:
```
someMap[k] = uf::userdata::create<uf::Camera>();
// ...
auto& v = uf::userdata::get<T>( someMap[key] );
// ...
if ( someMap[k] ) uf::userdata::destroy( someMap[k] );
```

## `engine/engine/`

### `engine/engine/asset/`

This implements the `uf::asset` system which governs asset loading and optimal, but deferred I/O reading.

Assets can be cached for subsequent loads.

When an asset is loaded, it's dispatched through the hook system by invoking the hook name specified as its callback (usually `asset:Load.%UID%`).

`uf::asset::load` *can* be directly invoked, but it's prefered to utilize an entity's JSON configuration or existing hooks (usually `asset:QueueLoad.%UID%`).

Assets can implicitly resolve absolutely based on the extension (or VFS prefix) per `uf::io::resolveURI`.

Additionally, `uf::asset::read`/`uf::asset::stream` offers deferred disk reads via a callback lambda.

### `engine/engine/behavior/`

This implements the `pod::Behavior` struct and `uf::Behaviors` system.

`pod::Behavior` stores function pointers for the primary entity functions:
* `initialize`: called on entity initialization
* `tick`: called when the engine ticks
* `render`: called when a render mode ticks for rendering
* `destroy`: called when the entity is to be destroyed

`uf::Behaviors` is a class for instantiation to handle an entity's list of behaviors. A graph is generated that flattens a scene graph tree for subsequent use.

This should ***never*** be directly interfaced with. Macros handle everything required, and reference to these macros can be found in any `behavior.cpp` and `behavior.h` pairs.

### `engine/engine/entity/`

This implements `uf::Entity` and `uf::EntityBehavior`, the core for an entity in this engine.

`uf::Entity` *should* handle the lowest level functions needed, such as UID and name, its parent and children, and containing its components and behaviors.

### `engine/engine/graph/`

This system implements everything for rendering objects in a scene via indirection.

A `pod::Graph` contains all the information needed to render a scene, from its textures and material information, to meshes, instance information, animations, to draw calls and how additional entities are to be loaded.

*Anything* non-GUI that interfaces with the engine's world is expected to be registered through here. Having said that, the object system should handle the gorey details of passing an entity through here.

Additional functions:
* `uf::graph::convert` handles "importing" a naive entity scene graph into the `uf::graph` graph system.
* `uf::graph::load` handles loading, from disk, a pre-processed scene graph into the engine.
* `uf::graph::save` handles saving, to disk, a scene graph to disk.

### `engine/engine/instantiator/`

This implements the `uf::instantiator` system, which governs instantiating entities as objects against a given list of object names and behavior names.

This system is responsible for binding behaviors to a named entity type, but use of this feature seems to have been removed in favor of another implementation.

For the most part, this does handle allocation and garbage collection of entities.

### `engine/engine/object/`

This implements `uf::Object` and `uf::ObjectBehavior`, where an object extends upon an entity by implementing additional features, such as:
* instantiating the object and initializing components per the configuration JSON 
* pre-defined cleanup behaviors for given components
* resolving relative pathnames and relative hook names
* asset loading
* processing hooks (adding, calling, queueing)
* reloading from the JSON when its updated

If `UF_ENTITY_OBJECT_UNIFIED` is set, then an entity and an object are truly interchangeable, but for the most part one can be casted to the other.


### `engine/engine/scene/`

This implements `uf::Scene` and `uf::SceneBehavior` structs and the `uf::scene` system.

A scene contains all of its entities, and orchestrates the scene graph. A scene is responsible for finding a controller (and thus camera) for a given render mode,

A scene's JSON is defined under `./data/scene/{name}/` and can be directly loaded on startup under `./data/config.json`'s `engine.scene.start` value.

While it's not utilized, additional scenes can be loaded onto the stack, or unloaded to "go back" to the prior scene.

## `engine/ext/`

### `engine/ext/aica`

This provides an OpenAL-equivalent abstraction for the Dreamcast's AICA (to replace ALdc).

For the most part, it's at parity with the full x86_64 target's OpenAL abstraction, sans EFX.

Streamed audio is buffered via `snd_stream`, while loaded sound effects are dispatched via `snd_sfx`.

Theoretically, spatial audio is supported for streamed audio, but sound effects need to be tested.

### `engine/ext/audio/`

This abstracts audio codecs for use under OpenAL.

Currently implemented:
* `pcm`: processes raw PCM audio into buffers
* `vorbis`: processes audio from an `.ogg` file through `libvorbis` (or `libtremor`) into buffers
* `wav`: processes audio from a `.wav` file through `dr_wav` into buffers
* `adp`: SEGA ADPCM, primarily for the Dreamcast target, but usable in the full x86_64 target as well

Audio is first read for its metadata, then either fully loaded into one buffer, or streamed into smaller buffers.

Enabled through their respective compile feature flag, and flagged through their respective `UF_USE_{NAME}` flags.

Encoders are also availbe if `UF_USE_{}_ENCODER` is set (automatically via `UF_DEV_ENV` in the `Makefile`).

### `engine/ext/discord/`

This abstracts around the Discord library for activity integration.

For the most part this is left uncompiled as it was just a loose gimmick by dispatching through the  `discord:Activity.Update` hook.

Enabled through the `discord` compile feature flag, and flagged through `UF_USE_DISCORD`.

### `engine/ext/ffx/`

This abstracts around [GPUOpen-LibrariesAndSDKs/FidelityFX-SDK](https://github.com/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK).

Enabled through the `ffx:fsr` compile feature flag, and flagged automatically through `UF_USE_FFX_SDK`.

*Mostly* working with interpolation and frame-gen, but hasn't been tested in a while.

### `engine/ext/gltf/`

This abstracts around the GTLf file specification through `tiny_gltf`.

For the most part, this handles loading a `.gltf` (or `.glb`) and processes it through the engine's `uf::graph` system, from which it can be saved to disk using its own internal format.

Enabled through the `gltf` compile feature flag, and flagged through `UF_USE_GLTF`.

### `engine/ext/imgui/`

This abstracts around the `Dear ImGui` library for a more-in-depth GUI system.

For the most part, this only (poorly) implements a console window.

Enabled through the `imgui` compile feature flag, and flagged through `UF_USE_IMGUI`.

### `engine/ext/json/`

This abstracts around [nlohmann/json](https://github.com/nlohmann/json) to provide an `ext::json::Value` object and `ext::json` namespace for operating on JSON objects.

Enabled through the `json:nlohmann` compile feature flag, and flagged through `UF_JSON_USE_NLOHMANN`.
* it is ***highly*** recommended to enable this.

### `engine/ext/lgs/`

This handles loading DarkEngine `.mis` files to import them into the engine.

While *a lot* has been implemented:
* animated meshes are ignored
* a good majority of a `.mis`'s inventory is neglected
* a good majority of the OSM need re-implementation via Lua

For the most part, most levels are functional and somewhat interactable.

While confusingly named `lgs` for Looking Glass Studios instead of `dark` for DarkEngine, it maps to how `valve` for Valve-formatted files isn't `source` for Source Engine files.

This is automatically handled via the graph system.

Enabled through the `lgs` compile feature flag, and flagged through `UF_USE_LGS`.

### `engine/ext/lua/`

This abstracts around Lua through [ThePhD/sol2](https://github.com/ThePhD/sol2) to extend the engine through Lua scripts.

Usertypes are provided under `engine/ext/lua/usertypes`:
* `uf::Asset`
* `uf::Audio`
* `uf::Camera`
* `pod::Matrix4f`
* `uf::Object`
* `pod::Physics`
* `pod::Quaternion<>`
* `uf::Timer<>`
* `pod::Transform<>`
* `pod::Vector<>`

On load, `./data/scripts/main.lua` is called, although nothing is directly in there yet.

Additional library tables are provided that map to internal engine functions through:
* `hooks.add`: `uf::hooks::addHook`
* `hooks.call`: `uf::hooks::callHook`
* `entities.get`
* `entities.currentScene`: `uf::scene::getCurrentScene`
* `entities.controller`: `uf::scene::getCurrentScene().getController`
* `entities.destroy`: `self:queueDeletion`
* `string.extension`: `uf::string::extension`
* `string.resolveURI`: `uf::string::resolveURI`
* `string.si`: `uf::string::si`
* `string.match`: `uf::string::match`
* `string.matched`: `uf::string::matched`
* `io.print`
* `math.clamp`: `math::clamp`
* `time.current`: `uf::physics::time::current`
* `time.previous`: `uf::physics::time::previous`
* `time.delta`: `uf::physics::time::delta`
* `json.pretty`: `uf::Serializer::serialize`
* `json.readFromFile`: `uf::Serializer::readFromFile`
* `json.writeToFile`: `uf::Serializer::writeToFile`
* `window.keyPressed`: `uf::Window::isKeyPressed`
* `os.arch`: `UF_ENV`
* `inputs.key`: `uf::inputs::key`
* `inputs.analog`: `uf::inputs::analog`
* `inputs.analog2`: `uf::inputs::analog2`
* and others I might be neglecting

Enabled through the `lua` compile feature flag, and flagged through `UF_USE_LUA`.

### `engine/ext/lz4/`

This abstracts around `lz4` for the compression and decompression of `.lz4` files.

Enabled through the `lz4` compile feature flag, and flagged through `UF_USE_LZ4`.

### `engine/ext/meshopt/`

This abstracts around [zeux/meshoptimizer](https://github.com/zeux/meshoptimizer) to optimize meshes and generating LODs ahead-of-time.

For the most part, this is automatically called when loading from a gLTF file.

The `ext::meshopt` namespace is provided if needed.

Enabled through the `meshopt` compile feature flag, and flagged through `UF_USE_MESHOPT`.

### `engine/ext/oal/`

This abstracts around OpenAL for the audio system through `ext::al`.

Most OpenAL parameters are handled. This shouldn't directly be invoked, as it's handled through `uf::Audio`

Enabled through the `openal` compile feature flag, and flagged through `UF_USE_OPENAL`.

### `engine/ext/opengl/`

This abstracts around OpenGL to provide the rendering system.

Most of this is enough to allow the engine to render using OpenGL 1.2.

A hand-crafted command buffer system is provided through `engine/ext/opengl/commands.cpp` to keep it seamlessly integrated with the engine that was primarily Vulkan-integrated.

Enabled through the `opengl` compile feature flag, and flagged through `UF_USE_OPENGL`.

### `engine/ext/openvr/`

This abstracts around [ValveSoftware/openvr](https://github.com/ValveSoftware/openvr) to provide VR support.

Enabled through the `openvr` compile feature flag, and flagged through `UF_USE_OPENVR`.

### `engine/ext/texconv/`

This supplies an interface to my fork of [texconv](https://github.com/e-c-k-e-r/texconv), a utility to convert images into `.dtex` for GLdc.

For the most part, the asset system and graph encoders should handle interfacing with this automatically.

Enabled through the `dc:texconv` compile feature flag, and flagged through `UF_USE_DC_TEXCONV`.

This *could* be enabled in the Dreamcast target itself to on-the-fly compress loaded `.png` into an optimal format.

### `engine/ext/truetype/`

This abstracts around TrueType fonts for creating bitmaps for a given font file, as well as provide metrics for a given glyph.

Enabled through the `truetype` compile feature flag, and flagged through `UF_USE_TRUETYPE`.

### `engine/ext/toml/`

This abstracts around [marzer/tomlplusplus](https://github.com/marzer/tomlplusplus) to provide TOML support.

For the most part, this is left unused and just wraps around to `ext::json::Value`, as it was explored as an alternative to JSON for reading configuration data.

Enabled through the `toml` compile feature flag, and flagged through `UF_USE_TOML`.

### `engine/ext/ultralight/`

This abstracts around [ultralight-ux/Ultralight](https://github.com/ultralight-ux/Ultralight) to provide rendering websites.

For the most part, this is left unused as it was a one-off gimmick in lieu of using something like Awesomium (or Chromium).

Enabled through the `ultralight-ux` compile feature flag, and flagged through `UF_USE_ULTRALIGHT_UX`.

### `engine/ext/vall_e/`

This abstracts around [e-c-k-e-r/vall-e/](https://github.com/e-c-k-e-r/vall-e/) to provide TTS support.

For the most part, this is added to provide an example on how to integrate a C++ project, and to justify burning two years on this meme.

Enabled through the `vall_e` compile feature flag, and flagged through `UF_USE_VALL_E`.

### `engine/ext/vulkan/`

This abstracts around Vulkan to provide the rendering system.

There's a *ton* of stuff it exposes that should necessitate its own documentation.

Enabled through the `vulkan` compile feature flag, and flagged through `UF_USE_VULKAN`.

### `engine/ext/valve`

This handles Valve's file formats, including:
* `.bsp` maps
* `.mdl` models
* `.vpk` archives (and can be mounted via the VFS)
* `.vtf` texture files
* `.vmt` material definitions

For the most part, maps do load and basic classes are implemented.

This is automatically handled via the graph system.

Enabled through the `valve` compile feature flag, and flagged through `UF_USE_VALVE`.

### `engine/ext/xatlas/`

This abstracts around [jpcy/xatlas](https://github.com/jpcy/xatlas) to provide UV unwrapping used for lightmap baking.

Enabled through the `xatlas` compile feature flag, and flagged through `UF_USE_XATLAS`.

### `engine/ext/zlib/`

This abstracts around `zlib` to provide compression and decompression of `.gz` files.

Enabled through the `zlib` compile feature flag, and flagged through `UF_USE_ZLIB`.

## `engine/spec/`

### `engine/spec/context/`

This handles per-platform OpenGL context code.

### `engine/spec/controller/`

This handles per-platform code for handling controllers.

For now, only the Dreacmast has code for its controllers. Controller support via OpenVR *was* handled, but it lives in its own code.

### `engine/spec/renderer/`

This provides an alias to which rendering backend to utilize.

### `engine/spec/terminal/`

This handles per-platform code for configuring the terminal window the program loads under.

For the most part, this *was* used for `engine/utils/io/iostream` with locale black magic and Ncurses, but Ncurses has been dropped for a while now. 

### `engine/spec/time/`

This handles per-platform code for getting time information.

For the most part, this is quasi-necessary for the Dreamcast to get a timestamp for timing, but I'm pretty sure the universal approach can work too.

### `engine/spec/window/`

This handles per-platform code for providing a window to run the program within.

Windows is mainly supported through `win32` black magic, but Dreamcast-specific code lives within here as well (despite there not being a windowing system).