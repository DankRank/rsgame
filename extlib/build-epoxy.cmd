@echo off
setlocal enabledelayedexpansion
set V=1.5.10
if not exist libepoxy-%V%.tar.gz curl -LSso libepoxy-%V%.tar.gz https://github.com/anholt/libepoxy/archive/refs/tags/%V%.tar.gz
if exist libepoxy rd /s /q libepoxy
tar -xf libepoxy-%V%.tar.gz
ren libepoxy-%V% libepoxy
pushd libepoxy
set MSVC_DIR=
if "%VCInstallDir%" == "" (
	for %%i in (Community Professional Enterprise) do (
		if exist "%PROGRAMFILES%\Microsoft Visual Studio\2022\%%i\VC" (
			set "MSVC_DIR=%PROGRAMFILES%\Microsoft Visual Studio\2022\%%i\VC"
		)
	)
	if "!MSVC_DIR!" == "" (
		echo ERROR: MSVC not found. Set VCInstallDir manually.
		exit /b 1
	)
) else (
	set "MSVC_DIR=%VCInstallDir%"
)
if not "%1" == "64" (
	call "%MSVC_DIR%\Auxiliary\Build\vcvars32"
	meson setup build32
	meson compile -C build32
)
if not "%1" == "32" (
	call "%MSVC_DIR%\Auxiliary\Build\vcvars64"
	meson setup build64
	meson compile -C build64
)
popd
endlocal
