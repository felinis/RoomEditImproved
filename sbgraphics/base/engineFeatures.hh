#pragma once

#define WIN32_LEAN_AND_MEAN
#include <d3d12.h>

/*
*	This function checks whether all the features used throughout the engine are supported
*	by the given ID3D12Device.
*/
bool AreAllEngineFeaturesSupported(ID3D12Device *device, HWND hWnd);
