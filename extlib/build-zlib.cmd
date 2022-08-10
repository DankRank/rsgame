@echo off
setlocal
set V=1.2.12
if not exist zlib-%V%.tar.gz curl -LSso zlib-%V%.tar.gz https://zlib.net/zlib-%V%.tar.gz
if exist zlib rd /s /q zlib
tar -xf zlib-%V%.tar.gz
ren zlib-%V% zlib
pushd zlib
if not "%1" == "64" (
	cmake -B build32 -A Win32 -DCMAKE_BUILD_TYPE=Release
	cmake --build build32 --config=Release
	mkdir build32\include
	for %%i in (zlib.h build32\zconf.h) do copy /b %%i build32\include\
)
if not "%1" == "32" (
	cmake -B build64 -A x64 -DCMAKE_BUILD_TYPE=Release
	cmake --build build64 --config=Release
	mkdir build64\include
	for %%i in (zlib.h build64\zconf.h) do copy /b %%i build64\include\
)
popd
endlocal
