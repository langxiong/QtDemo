@echo off
REM One-click demo: backend + frontend
REM Run from project root or demo directory

set SCRIPT_DIR=%~dp0
set DEMO_DIR=%SCRIPT_DIR%..
set BUILD_BIN=%DEMO_DIR%\build\bin\Release
set FRONTEND=%DEMO_DIR%\frontend

echo [1/2] Starting cef_host (API-only backend)...
start "cef_host" cmd /k "%BUILD_BIN%\cef_host.exe"
timeout /t 2 /nobreak >nul

echo [2/2] Starting frontend dev server...
cd /d "%FRONTEND%"
if not exist node_modules (
  echo Installing npm dependencies...
  call npm install
)
start "frontend" cmd /k "npm run dev"
timeout /t 3 /nobreak >nul

echo.
echo Demo launched. Open http://localhost:5173 in your browser.
echo Verify: Start/Stop/Reset, readApp status, readStream charts, logs.
pause
