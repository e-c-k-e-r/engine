# `ext/`

This section of the project orchestrates the engine's systems and extends it for the overall program, and is intended to hold anything that isn't crucial to the underlying engine itself.

In the past, this compiled to its own DLL, but for the most part it's better to compile as one DLL.

## Files

### `ext/main.cpp`

This file handles loading settings from `./data/config.json` and initializes the engine's systems accordingly.

After everything is initialized, the main scene is requested for loading. From there, the tick and render loops are handled, as well as """garbage collection""" and thread orchestration.

On exit, cleanup is handled gracefully, as segfaulting is not a valid exit path.

### `ext/behaviors/`

This folder handles extended engine logic for the program. While most of these can live within the engine (and later should), they're separated off to here to serve as references when implementing the engine for use in a program.

#### `ext/behaviors/audio/bgm/`

The `ext::BgmEmitterBehavior` behavior handles logic for playing the background music for a scene.

Features such as looping, transitions from an `intro` to the main track OR fading to other tracks, are handled here.

Invoking the `bgm:Play.%UID%` hook will signal for this behavior to play a track, but tracks can be defined through an entity's `bgm.tracks` metadata.

#### `ext/behaviors/audio/emitter`

The `ext::SoundEmitterBehavior` behavior handles logic for playing (emitting) sound from an entity.

Invoking the `sound:Emit.%UID%` hook will signal for this behavior to emit a sound. Most of the parameters through OpenAL can be set through this payload.

#### `ext/behaviors/baking/`

The `ext::BakingBehavior` behavior handles orchestrating the rendering system to bake lightmaps for a scene.

#### `ext/behaviors/craeture/`

The `ext::CraetureBehavior` behavior serves as a barebone behavior for NPCs.

In reality it's barebones due to most of an entity's data being handled through its JSON.

Additional player logic is defined through `/data/entity/scripts/craeture.lua`

#### `ext/behaviors/gui/`

The `ext::GuiBehavior` behavior governs everything for handling GUI elements, from loading, to rendering, to handling mouse events.

This GUI system is still a bit of a mess as its a relic from prior iterations of this engine having messy configuration settings.

Menus can be implemented through Lua scripts, or additional behaviors.

##### `ext/behaviors/gui/glyph/`

The `ext::GuiGlyphBehavior` behavior extends the `ext::GuiBehavior` by handling text through glyphs.

Currently, this behavior loads glyphs through `FreeType2`, and handles the gorey bits of generating an SDF (if requested), combining the glyphs into an atlas, configuring the metadata to utilize the glyph shader, and updating the GUI when the text updates.

Like the overall GUI system, most of this is still a bit of a mess from its past iterations.

##### `ext/behavior/gui/manager/`

The `ext::GuiManagerBehavior` behavior handles the `gui` rendermode layer, in which all GUI elements render to, and the shader used when blitting to the swapchain.

#### `ext/behavior/light/`

The `ext::LightBehavior` behavior handles the logic for light sources and its shadow maps.

For the most part, this signals to the scene that this entity is a light source. If the entity is marked as a shadow caster, then the behavior handles everything for generating a shadow map. Point lights are implemented as a cubemap.

Additional logic is included to handle determining which lights get loaded when signaled through the scene, to reduce frame times.

#### `ext/behavior/player/`

The `ext::PlayerBehavior` behavior handles the primary controller, the player.

While inputs *can* be handled through hooks, it's better to instead read from `uf::inputs::kbm` (or `uf::inputs::controller`) directly to handle it. Movement is also handled here in a """Source""" style system.

Additional player logic is defined through `/data/entity/scripts/player.lua` (primarily `E`-use interactions and `F`-flashlight toggling).

This is a bit of a mess as it's a relic from prior engine iterations.

##### `ext/behavior/player/model/`

The `ext::PlayerModelBehavior` behavior handles the player model, as it shouldn't render in the primary render mode, but should in other render modes.

Additionally, the model should bind to the player model's `pod::Transform<>`.

#### `ext/behavior/raytrace/`

The `ext::RayTraceSceneBehavior` behavior extends the scene's behavior to handle everything for use of `VK_KHR_ray_tracing`.

#### `ext/behavior/region/`

The `ext::RegionBehavior` behavior *should* handle everything related to the chunk system. Additional chunks are loaded when the player (or other entities) requests them to load, and unloads unused chunks.

This was implemented in prior iterations of the engine, but requires a rewrite.

##### `ext/behavior/region/chunk/`

The `ext::RegionChunkBehavior` behavior *should* handle everything related to the chunk loaded through `ext::RegionBehavior`.

#### `ext/behavior/scene/`

The `ext::ExtSceneBehavior` behavior extends a scene's behavior to handle everything else related to the scene, such as:
* menus
* entity spawning
* noise generation
* binding descriptors to shaders
* skybox loading
* lag detection
* screenshots
* updating the OpenAL listener
* locking the cursor when not in a menu
* processing light information to shader buffers, and enabling/disabling used/unused lights
* gamma correction, ambient light, fog color, volume levels, and shader modes

#### `ext/behavior/voxelizer/`

The `ext::VoxelizerSceneBehavior` behavior extends the scene's behavior to handle everything to implement VXGI (voxel global illumination).

The scene's graph gets an additional pipeline `vxgi` to "voxelize" all geometry into a voxel map (atomically to integer buffers). This voxel map is then processed accordingly and lit to output a radiance map, which is then mipmapped and bound to the deferred render pass for VXGI queries.

Voxel cascades are centered around the player, where additional cascades are scaled to a specified power at reduced LOD, but a consistent resolution.

## Behaviors

A behavior can easily be implemented and registered through macros.

An example `behavior.h`:
```
namespace ext {
	namespace SomeNameForABehavior {
		UF_BEHAVIOR_DEFINE_TYPE();
		EXT_BEHAVIOR_DEFINE_TRAITS();
		EXT_BEHAVIOR_DEFINE_FUNCTIONS();
		UF_BEHAVIOR_DEFINE_METADATA(
			pod::Vector3ui someVector = {};
			uf::stl::string someString = "";
		);
	}
}
```

An example `behavior.cpp`:
```
UF_BEHAVIOR_REGISTER_CPP(ext::SomeNameForABehavior)
UF_BEHAVIOR_TRAITS_CPP(ext::SomeNameForABehavior, ticks = true, renders = false, multithread = false)
#define this (&self)
void ext::SomeNameForABehavior::initialize( uf::Object& self ) {
	// attach some components
	auto& metadata = this->getComponent<ext::SomeNameForABehavior::Metadata>();
	auto& metadataJson = this->getComponent<uf::Serializer>();

	// bind some hooks
	UF_BEHAVIOR_METADATA_BIND_SERIALIZER_HOOKS(metadata, metadataJson);
}
void ext::SomeNameForABehavior::tick( uf::Object& self ) {}
void ext::SomeNameForABehavior::render( uf::Object& self ){}
void ext::SomeNameForABehavior::destroy( uf::Object& self ){}
void ext::SomeNameForABehavior::Metadata::serialize( uf::Object& self, uf::Serializer& serializer ){
	serializer["some vector"] = uf::vector::encode(/*this->*/someVector);
	serializer["some string"] = /*this->*/someString;
}
void ext::SomeNameForABehavior::Metadata::deserialize( uf::Object& self, uf::Serializer& serializer ){
	/*this->*/someVector = uf::vector::decode( serializer["some vector"], /*this->*/someVector );
	/*this->*/someString = serializer["some string"].as(/*this->*/someString);
}
```

While it's not required to specify a behavior's metadata struct (as you *can* use the JSON state of an entity), it's heavily encouraged to keep a behavior's state within the `UF_BEHAVIOR_DEFINE_METADATA` metadata struct. If you do, it's imperative to call `UF_BEHAVIOR_METADATA_BIND_SERIALIZER_HOOKS` as it attaches hooks when an entity needs to serialize/deserialize to its JSON state (for example, for access within Lua), and the `serialize`/`deserialize` functions must be filled.