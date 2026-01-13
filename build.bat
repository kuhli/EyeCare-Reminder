@echo off
echo Compiling reminder tool...
echo Compiling resource file...
windres resource.rc -o resource.o
if %errorlevel% neq 0 (
    echo Resource compilation failed!
    pause
    exit /b 1
)
echo Compiling main program ...
echo Features:
echo - Pure Win32 API, no dependencies
echo - Reminds every 1180 seconds
echo - Large 48pt font display
echo - System tray operation
echo.
g++ -O2 -s -static -mwindows -o EyeCareReminder.exe EyeCareReminder.cpp resource.o
if %errorlevel% == 0 (
    echo Build successful! Generated: EyeCareReminder.exe
    echo File size:
    dir EyeCareReminder.exe | findstr EyeCareReminder.exe
    echo.
    del resource.o
    goto end
) else (
    echo Build failed!
    del resource.o 2>nul
)
:end
pause