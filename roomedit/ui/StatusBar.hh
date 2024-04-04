#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

enum Part
{
	FirstPart,
	SecondPart,
	Max
};

class StatusBar
{
	HWND hWnd;

public:
	StatusBar(HWND hWnd = nullptr): hWnd(hWnd) {}

	bool Create(HINSTANCE hInstance, HWND hParentWnd);

	void SetText(Part part, const char* text);
	void SetDefaultText();
	void Resize();
	int GetHeight();
};
