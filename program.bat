set /p prefix=<.\exe\default.config
set PATH=%CD%\exe\lib\win64\;%CD%\exe\lib\win64\%prefix%\;%PATH%
.\exe\program.%prefix%.exe %*
