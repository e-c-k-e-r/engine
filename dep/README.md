# Dependencies

This folder stores any code for external dependencies because my environment has been a nightmare under MSYS2 for years.

Separate per-target copies of dependencies may exist here as well, such as:
* Dreamcast

All code under this folder is not mine and are licensed under their respective licenses.

## Forked Dependencies

Some dependencies of mine rely on some modifications of the original dependencies for extra features or to function on other platforms.

* [nlohmann/json](https://github.com/nlohmann/json): Dreamcast only; requires some hacks for `m4-single-only` due to `sizeof(float) == sizeof(double)` (for prior toolchains, this allegedly changed but I still get a crash with an unmodified `nlohmann/json`)
	* this is a header-only project so its changes live locally under `./dep/dreamcast/include/nlohmann/`
* [DanielChappuis/reactphysics3d](https://git.ecker.tech/ecker/reactphysics3d): adds in support for `float16`/`bfloat16`/quantized `uint16_t` triangle meshes
	* Dreamcast requires some edits due to `sizeof(int) != sizeof(int32)` / `sizeof(uint) != sizeof(uint32)`, extra compile flags, and disabling exceptions
* [simulant/GLdc](https://git.ecker.tech/ecker/GLdc/): Dreamcast only; adds in support for `float16`/`bfloat16`/quantized `uint16_t` vertex data
* [GPUOpen-Effects/FidelityFX-FSR2](https://git.ecker.tech/ecker/FidelityFX-FSR2): modifications required for compiling under GCC (despite being a soft-dependency that I'm not making use of anyways)