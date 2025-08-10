# Engine Docs

## Features

To be filled.

* a Unity-like ECS + scene + asset system
	* entities stores components and behaviors
	* scenes load a tree of entities
	* entities are defined through JSON files which can reference assets or other JSON files, and can instantiate other entities
* Lua to extend the engine
* Garry's Mod-like hook system for dispatching events
	* events are dispatched through the hook system by sending objects hooks tied to a given name + payload type
* Vulkan (or OpenGL) as the rendering backend
	* the Vulkan backend heavily makes use of an almost-GPU driven deferred rendering system
		* "G-buffer" can consist solely of IDs and depth, and all geometry information is then reconstructed during the deferred compute pass
			* optionally can fall back to a quasi-traditional setup of IDs, UVs, normals, and depth
		* shadow maps are rendered to each light's shadow maps
			* point lights are treated as a cubemap
		* optional light map baking
		* basic PBR shading
		* a sovlful (sloppy) VXGI for GI and reflections
		* hardware RT support via `VK_KHR_ray_query`
			* "full" `VK_KHR_ray_tracing` mode is partially broken
		* bloom and additional post processing
		* FSR2 or something cringe
	* OpenGL uses a very, very naive OpenGL 1.2 API with a homebrewed command recording system
		* pre-baked light maps can be loaded for lighting
* ReactPhysics3D for physics
	* *very* loosely integrated
	* basic shapes and triangulated mesh collision and some form of ray queries
* OpenAL for audio
	* Loads from `.ogg` or `.wav` files
		* Supports loading in full and streaming
	* *very* loosely integrated
* Speech synthesis using [vall_e.cpp](https://github.com/e-c-k-e-r/vall-e/)
	* `win64.gcc.vulkan` binaries can be found [here](https://github.com/e-c-k-e-r/vall-e/releases/tag/vall_e.cpp), if compiled with `-DUF_USE_VALL_E`
	* asynchronously synthesizes speech and plays back the PCM audio

## Supported Systems

* Windows
	* *technically* also Linux under Proton
* Sega Dreamcast

## Notices and Citations

Unless otherwise credited/noted in this repo or within the designated file/folder, this repository is [licensed](/LICENSE) under AGPLv3 (I do not have a master record of dependencies).