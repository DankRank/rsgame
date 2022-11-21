@echo off
setlocal
set V=2.24.2
if not exist SDL2-%V%.tar.gz curl -LSso SDL2-%V%.tar.gz https://www.libsdl.org/release/SDL2-%V%.tar.gz
if exist SDL2static rd /s /q SDL2static
tar -xf SDL2-%V%.tar.gz
ren SDL2-%V% SDL2static
pushd SDL2static
if not "%1" == "64" (
	cmake -B build32 -A Win32 -DCMAKE_BUILD_TYPE=Release
	cmake --build build32 --config=Release
)
if not "%1" == "32" (
	cmake -B build64 -A x64 -DCMAKE_BUILD_TYPE=Release
	cmake --build build64 --config=Release
)
popd
endlocal
