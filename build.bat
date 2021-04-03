@echo off 
cl bfbb_stat_tracker.c /DDOLPHIN /MT /Od /Oi /Zi /link /incremental:no /opt:ref 
IF %ERRORLEVEL% == 0 ( 
	bfbb_stat_tracker.exe 
) 
