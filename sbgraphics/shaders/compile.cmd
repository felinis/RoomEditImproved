@echo off
echo Compiling shaders...
dxc /T vs_6_4 /E VSMain /O2 /Fh vsScreenColored.h vsScreenColored.hlsl
dxc /T ps_6_4 /E PSMain /O2 /Fh psColor.h psColor.hlsl
echo Finished.
