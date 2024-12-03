# Engine

An unnamed, almost-entirely from-scratch, engine written in C++, using:
* a Unity-like ECS + scene system
* `?`-like hook system for dispatching events
* Vulkan (or OpenGL) as the rendering backend
* ReactPhysics3D for physics
* OpenAL for audio

## Build

While the build system is fairly barebones and robust under `make`, dependency tracking is not.
* Ensure all requested dependencies under `REQ_DEPS` are available in your build system, as well as a valid compiler under `CC`/`CXX`.

Configuration for build targets are available under `./makefiles/` are available with the naming convention `${system}.${compiler}.make`.
* Additional compiler flags and `make` variables can be specified here.

To compile, run `make`. The outputted libraries and executables will be placed in the right folders under `./bin/`.

## Run

Currently, assets are not provided due to size (but mostly due to being test assets).

*If* adequate assets are provided, run `./program.sh`.

## Documentation

The provided documentation under [./docs/](./docs/) should (eventually) provide thorough coverage over most, if not all, of this project.