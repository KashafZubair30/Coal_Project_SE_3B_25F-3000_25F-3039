@echo off
cd /d "%~dp0"
set NASM=nasm
where nasm >nul 2>nul || set NASM=%LOCALAPPDATA%\bin\NASM\nasm.exe
"%NASM%" -f bin -I src/ src/mario.asm -o MARIO.COM -l build.lst
if errorlevel 1 (echo BUILD FAILED) else (echo MARIO.COM built)
pause
