@echo off
echo Compiling shaders...
dxc /T vs_6_0 /E VSMain /O2 /Fh compiled\ColoredVS.h ColoredVS.hlsl
dxc /T ps_6_0 /E PSMain /O2 /Fh compiled\ColoredPS.h ColoredPS.hlsl
dxc /T vs_6_0 /E VSMain /O2 /Fh compiled\LightMappedVS.h LightMappedVS.hlsl
dxc /T ps_6_0 /E PSMain /O2 /Fh compiled\LightMappedPS.h LightMappedPS.hlsl
dxc /T vs_6_0 /E VSMain /O2 /Fh compiled\PhongVS.h PhongVS.hlsl
dxc /T ps_6_0 /E PSMain /O2 /Fh compiled\PhongPS.h PhongPS.hlsl
echo Finished
