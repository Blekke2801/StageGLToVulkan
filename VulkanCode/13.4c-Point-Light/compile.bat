@echo off
set GLSLC="%~dp0\..\base\VulkanSDK\Bin\glslc.exe"
set SRC=shaders
set DST=compiled

if not exist %DST% (
    mkdir %DST%
)

echo Compiling vertex shaders...
for %%f in (%SRC%\*.vert) do (
    %GLSLC% %%f -o %DST%\%%~nxf.spv
)

echo Compiling fragment shaders...
for %%f in (%SRC%\*.frag) do (
    %GLSLC% %%f -o %DST%\%%~nxf.spv
)

echo Done!