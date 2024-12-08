# Engine Docs

## Features

To be filled.

* a Unity-like ECS + scene + asset system
	* entities stores components and behaviors
	* scenes load a tree of entities
	* entities are defined through JSON files (which are processed as gunzipped msgpacks for subsequent loads) which can reference assets or further entities
* Lua to extend the engine
* Garry's Mod-like hook system for dispatching events
	* events are dispatched through the hook system by sending a payload to hooks tied to a given name / payload type
	* by default, these are JSON payloads, but most internal hooks send unique structs instead
* Vulkan (or OpenGL) as the rendering backend
	* the Vulkan backend heavily makes use of an almost-GPU driven deferred rendering system
		* "G-buffer" can consist solely of IDs, barycentrics, and depth, and all geometry information is then reconstructed during the deferred compute pass
		* shadow maps are rendered to each light's shadow maps
			* point lights are treated as a cubemap
		* basic PBR shading
		* a sovlful (sloppy) VXGI for GI and reflections
		* (partially broken) hardware RT support
		* bloom and additional post processing
		* FSR2 or something cringe
	* OpenGL uses a very, very naive OpenGL 1.2 API with a homebrewed command recording system
* ReactPhysics3D for physics
	* *very* loosely integrated
	* basic shapes and triangulated mesh collision and some form of ray queries
* OpenAL for audio
	* Currently only loads from ogg (vorbis) files
	* Supports loading in full and streaming
	* *very* loosely integrated

## Supported Systems

* Windows
	* *technically* also Linux under Proton
* Sega Dreamcast

## Notices and Citations

Unless otherwise credited/noted in this repo or within the designated file/folder, this repository is [licensed](/LICENSE) under AGPLv3 (I do not have a master record of dependencies).