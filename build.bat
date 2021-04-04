@echo off 
cl bfbb_stat_tracker.c user32.lib gdiplus.lib shlwapi.lib /DDOLPHIN /MT /Od /Oi /Zi /link /incremental:no /opt:ref 
IF %ERRORLEVEL% == 0 ( 
	bfbb_stat_tracker.exe 
) 
