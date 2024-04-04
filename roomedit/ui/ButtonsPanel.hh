#pragma once

#include "../Document.hh"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

class ButtonsPanel
{
	Document &document; //reference to the data, so we can read/modify it from here
	HWND hWnd;

	static LRESULT CALLBACK SubWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

public:
	constexpr ButtonsPanel(Document &document):
		document(document),
		hWnd(nullptr)
	{}

	bool Create(HINSTANCE hInstance, HWND hParentWnd);
	void MoveWindow(int X, int Y, int nWidth, int nHeight);
};