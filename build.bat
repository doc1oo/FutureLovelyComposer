@echo off
REM cmake -S . -B ./build -D RTMIDI_API_WINMM=ON
REM cmake -S . -B ./build -D RTMIDI_API_WINMM=ON -D CMAKE_BUILD_TYPE=Release
cmake -S . -B ./build -D RTMIDI_API_WINMM=ON -D CMAKE_BUILD_TYPE=Debug
cmake --build ./build

REM  -D CMAKE_BUILD_TYPE=Release -D RTMIDI_API_WINMM=ON
REM * for Relaase Build * 
REM cmake --build ./build --config Release

if %ERRORLEVEL% equ 0 (
    copy /Y "build\gdtune\Debug\gdtune.dll" "godot_project\bin\"
    "C:\MyData\Works40s\Godot\Godot_v4.3-stable_win64.exe\Godot_v4.3-stable_win64.exe" --path "./godot_project/" --verbose
) else (
    echo "Build Error!!"
)

REM -- MEMO ---
REM copy /Y "build/gameplay/Debug/gameplay.dll" "godot_project/bin/gameplay.dll"
REM "C:\MyData\Works40s\Godot\Godot_v4.3-stable_win64.exe\Godot_v4.3-stable_win64.exe" -e godot_project/project.godot
REM "C:\MyData\Works40s\Godot\Godot_v4.3-stable_win64.exe\Godot_v4.3-stable_win64.exe" godot_project/project.godot
REM rmdir /S /Q "..\..\Dodge_Sample\.godot\"