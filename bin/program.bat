set /p arch=<.\exe\default\arch
set /p cc=<.\exe\default\cc
set /p renderer=<.\exe\default\renderer
set PATH=%CD%\exe\lib\%arch%\;%CD%\exe\lib\%arch%\%cc%\;%CD%\exe\lib\%arch%\%cc%\%renderer%\;%PATH%
set _NO_DEBUG_HEAP=1
.\exe\program.%arch%.%cc%.%renderer%.exe %*
