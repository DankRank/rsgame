@echo off
if "%2" == "32" (
	copy /b "%~dp0\zlib\build32\Release\zlib.dll" "%1"
	copy /b "%~dp0\libpng\build32\Release\libpng16.dll" "%1"
	copy /b "%~dp0\SDL2\lib\x86\SDL2.dll" "%1"
	copy /b "%~dp0\libepoxy\build32\src\epoxy-0.dll" "%1"
)
if "%2" == "64" (
	copy /b "%~dp0\zlib\build64\Release\zlib.dll" "%1"
	copy /b "%~dp0\libpng\build64\Release\libpng16.dll" "%1"
	copy /b "%~dp0\SDL2\lib\x64\SDL2.dll" "%1"
	copy /b "%~dp0\libepoxy\build64\src\epoxy-0.dll" "%1"
)
