@echo off
setlocal enabledelayedexpansion
set V=1.5.10
if not exist libepoxy-%V%.tar.gz curl -LSso libepoxy-%V%.tar.gz https://github.com/anholt/libepoxy/archive/refs/tags/%V%.tar.gz
if exist libepoxy rd /s /q libepoxy
tar -xf libepoxy-%V%.tar.gz
ren libepoxy-%V% libepoxy
pushd libepoxy
set MSVC_DIR=
if "%PROGRAMFILES(X86)%" == "" set "PROGRAMFILES(X86)=%PROGRAMFILES%"
if "%VCInstallDir%" == "" (
	for %%i in (2017 2019) do (
		for %%j in (Community Professional Enterprise) do (
			if exist "%PROGRAMFILES(X86)%\Microsoft Visual Studio\%%i\%%j\VC" (
				set "MSVC_DIR=%PROGRAMFILES(X86)%\Microsoft Visual Studio\%%i\%%j\VC"
			)
		)
	)
	for %%i in (2022) do (
		for %%j in (Community Professional Enterprise) do (
			if exist "%PROGRAMFILES%\Microsoft Visual Studio\%%i\%%j\VC" (
				set "MSVC_DIR=%PROGRAMFILES%\Microsoft Visual Studio\%%i\%%j\VC"
			)
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
	meson setup --default-library=static -Dc_args=-DEPOXY_PUBLIC=extern build32static
	meson compile -C build32static
)
if not "%1" == "32" (
	call "%MSVC_DIR%\Auxiliary\Build\vcvars64"
	meson setup build64
	meson compile -C build64
	meson setup --default-library=static -Dc_args=-DEPOXY_PUBLIC=extern build64static
	meson compile -C build64static
)
popd
endlocal
