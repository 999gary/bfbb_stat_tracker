@echo off 
cl bfbb_stat_tracker.c /DDOLPHIN /MT /Od /Oi /Zi /link /incremental:no /opt:ref user32.lib gdiplus.lib shlwapi.lib 
IF %ERRORLEVEL% == 0 ( 
	bfbb_stat_tracker.exe 
) 
