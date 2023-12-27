@echo off
setlocal
set V=1.6.40
if not exist libpng-%V%.tar.gz curl -LSso libpng-%V%.tar.gz https://download.sourceforge.net/libpng/libpng-%V%.tar.gz
if exist libpng rd /s /q libpng
tar -xf libpng-%V%.tar.gz
ren libpng-%V% libpng
pushd libpng
if not "%1" == "64" (
	cmake -B build32 -A Win32 -DCMAKE_BUILD_TYPE=Release -DCMAKE_FIND_PACKAGE_PREFER_CONFIG=TRUE -DZLIB_DIR=%CD%\..
	cmake --build build32 --config=Release
	mkdir build32\include
	for %%i in (png.h pngconf.h build32\pnglibconf.h) do copy /b %%i build32\include\
)
if not "%1" == "32" (
	cmake -B build64 -A x64 -DCMAKE_BUILD_TYPE=Release -DCMAKE_FIND_PACKAGE_PREFER_CONFIG=TRUE -DZLIB_DIR=%CD%\..
	cmake --build build64 --config=Release
	mkdir build64\include
	for %%i in (png.h pngconf.h build64\pnglibconf.h) do copy /b %%i build64\include\
)
popd
endlocal
