@echo off
if exist bfbb_stat_tracker.exe del bfbb_stat_tracker.exe
pushd build
::cl ..\source\bfbb_stat_tracker.c /DDOLPHIN /MT /Od /Oi /Zi /link /SUBSYSTEM:WINDOWS /incremental:no /opt:ref user32.lib gdiplus.lib shlwapi.lib d3d9.lib
cl ..\source\bfbb_stat_tracker.c /DDOLPHIN /MT /Od /Oi /Zi /link /incremental:no /opt:ref user32.lib gdiplus.lib shlwapi.lib d3d9.lib
if not errorlevel 0 goto EndBuild 
copy bfbb_stat_tracker.exe ..
:EndBuild
popd

