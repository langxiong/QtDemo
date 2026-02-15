@echo off
REM 最小测试：Docker fastddsgen 生成 IDL (Windows)
REM 用法: run_test.bat [docker_image]
REM 例:   run_test.bat fastddsgen-arch

setlocal
set IMAGE=%1
if "%IMAGE%"=="" set IMAGE=fastddsgen-arch

set SCRIPT_DIR=%~dp0
set IDL_DIR=%SCRIPT_DIR%
set OUT_DIR=%SCRIPT_DIR%generated

if not exist "%OUT_DIR%" mkdir "%OUT_DIR%"

echo === Docker fastddsgen 测试 ===
echo 镜像: %IMAGE%
echo IDL:  %IDL_DIR%Test.idl
echo 输出: %OUT_DIR%
echo.

echo ^>^>^> 执行: docker run --rm -v IDL:/idl -v OUT:/out %IMAGE% -ppDisable -d /out -replace /idl/Test.idl
docker run --rm ^
  -v "%IDL_DIR%:/idl" ^
  -v "%OUT_DIR%:/out" ^
  %IMAGE% ^
  -ppDisable -d /out -replace /idl/Test.idl

if errorlevel 1 (
  echo 失败: docker 返回非零
  exit /b 1
)

echo.
echo ^>^>^> 检查生成文件
set MISSING=0
for %%f in (Test.hpp TestPubSubTypes.cxx TestPubSubTypes.hpp) do (
  if exist "%OUT_DIR%\%%f" (
    echo   OK: %%f
  ) else (
    echo   缺失: %%f
    set MISSING=1
  )
)

if %MISSING%==1 (
  echo.
  echo 测试失败: 有文件缺失
  exit /b 1
)

echo.
echo === 测试通过 ===
dir "%OUT_DIR%"
exit /b 0
