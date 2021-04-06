@echo off 
cl bfbb_stat_tracker.c /DDOLPHIN /MT /Od /Oi /Zi /link /SUBSYSTEM:WINDOWS /incremental:no /opt:ref user32.lib gdiplus.lib wsock32.lib shlwapi.lib 
IF %ERRORLEVEL% == 0 ( 
	bfbb_stat_tracker.exe 
) 
