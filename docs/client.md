# `client/`

This section of the project solely governs the executable and the outer layer of the program.

Its sole purpose is to:
* attach signal handlers
* initialize the engine
* create the window and hook its events into the engine
* the engine tick loop
* terminate the engine

Additional logic *can* be added such as:
* integrity checks
* update handling
* loading additional DLLs