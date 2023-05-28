#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "sbinput/InputCollector.h"

HWND CreateSceneView(HINSTANCE hInstance, HWND hParentWnd, const char* className);

bool LoadLevel(const char* pathToFile);
void UnloadLevel();

void TickSceneView(InputCollector& inputCollector, float dt);
void ResetCamera();
