@echo off
setlocal
set V=2.0.22
if not exist SDL2-devel-%V%-VC.zip curl -LSso SDL2-devel-%V%-VC.zip https://www.libsdl.org/release/SDL2-devel-%V%-VC.zip
if exist SDL2 rd /s /q SDL2
tar -xf SDL2-devel-%V%-VC.zip
ren SDL2-%V% SDL2
endlocal
