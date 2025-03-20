@echo off
set PROJECT_DIR=%~dp0


set BUILD_DIR=%PROJECT_DIR%build_debug

if exist "%BUILD_DIR%" (
    echo Cleaning up build directory...
    rmdir /s /q "%BUILD_DIR%"
) else (
    echo Build directory does not exist. Skipping cleanup.
)

echo Creating build directory...
mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

echo Configuring the project with CMake...

REM  if use GNU
REM cmake  -DCMAKE_BUILD_TYPE=Debug \
REM        -DCMAKE_C_COMPILER=/path/to/gcc \
REM        -DCMAKE_CXX_COMPILER=/path/to/g++ \ .. 
REM  if use Visual Studio
REM cmake -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Debug ..

REM choose default compiler
cmake -DCMAKE_BUILD_TYPE=Debug  .. -D GLFW_BUILD_DOCS=OFF || (
    echo CMake configuration failed. Exiting.
    exit /b 1
)

echo Building the project...
cmake --build . --config Debug || (
    echo Build failed. Exiting.
    exit /b 1
)

echo Build completed successfully!
pause
