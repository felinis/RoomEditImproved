#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class PropertyGrid
{
	HWND hWnd;

public:
	constexpr PropertyGrid() : hWnd(nullptr) {}

	bool Create(HINSTANCE hInstance, HWND hParentWnd);

	void AddProperty(const char* propertyName, const char* propertyValue);
	void Empty();

	void MoveWindow(int X, int Y, int nWidth, int nHeight);
};
