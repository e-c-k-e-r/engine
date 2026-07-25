# Dependencies

This folder stores any code for external dependencies because my environment has been a nightmare under MSYS2 for years.

Separate per-target copies of dependencies may exist here as well, such as:
* Dreamcast

All code under this folder is not mine and are licensed under their respective licenses.

## Forked Dependencies

Some dependencies of mine rely on some modifications of the original dependencies for extra features or to function on other platforms.

* [nlohmann/json](https://github.com/nlohmann/json): ~~Dreamcast only; requires some hacks for `m4-single-only` due to `sizeof(float) == sizeof(double)` (for prior toolchains, this allegedly changed but I still get a crash with an unmodified `nlohmann/json`)~~ should be fixed with proper templating
	* this is a header-only project so its changes live locally under `./dep/dreamcast/include/nlohmann/`
* [DanielChappuis/reactphysics3d](https://git.ecker.tech/ecker/reactphysics3d): adds in support for `float16`/`bfloat16`/quantized `uint16_t` triangle meshes
	* Dreamcast requires some edits due to `sizeof(int) != sizeof(int32)` / `sizeof(uint) != sizeof(uint32)`, extra compile flags, and disabling exceptions
* [simulant/GLdc](https://git.ecker.tech/ecker/GLdc/): Dreamcast only; adds in support for `float16`/`bfloat16`/quantized `uint16_t` vertex data
* [GPUOpen-Effects/FidelityFX-FSR2](https://git.ecker.tech/ecker/FidelityFX-FSR2): modifications required for compiling under GCC (despite being a soft-dependency that I'm not making use of anyways)
* [KallistiOS/KallistiOS](https://github.com/KallistiOS/KallistiOS/tree/e4171afbdaf20574a554d8d40f08552e0680db14): Dreamcast only; specifically commit `e4171afb` .
	* this version requires commenting out this line at [kernel/arch/dreamcast/hardware/cdrom.c#L839](https://github.com/KallistiOS/KallistiOS/blob/e4171afbdaf20574a554d8d40f08552e0680db14/kernel/arch/dreamcast/hardware/cdrom.c#L839) to boot under emulators (I've yet to test )
	* later version require compiling with `kos-c++` instead of `$(KOS_CCPLUS)` and removing the `-T/opt/dreamcast/kos/utils/ldscripts/shlelf.xc` line when compiling the `.elf`, but crashes later during some `maple` initialization.
* [tvspelsfreak/texconv](https://github.com/e-c-k-e-r/texconv): For converting images to `.dtex` for the Dreamcast.
	* this *is* actually included to allow scene processing to automatically output processed `.dtex` textures...
	* ...*or* allow files to be converted on the fly on the Dreamcast (although this is a *bad* idea for large image files)