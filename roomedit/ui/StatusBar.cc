/*
*	Room Editor Application
*	(C) Moczulski Alan, 2023.
*/

#include "StatusBar.hh"
#include <CommCtrl.h>

bool StatusBar::Create(HINSTANCE hInstance, HWND hParentWnd)
{
	hWnd = CreateWindowEx(
		0,
		STATUSCLASSNAME,
		nullptr,
		WS_CHILD | WS_VISIBLE,
		0, 0, 0, 0,
		hParentWnd,
		(HMENU)4100,
		hInstance,
		nullptr
	);
	if (!hWnd)
		return false;

	int partsWidths[Part::Max] =
	{
		600,
		-1      //the last part takes up the remaining space
	};

	SendMessage(hWnd, SB_SETPARTS, Part::Max, (LPARAM)partsWidths);

	SetDefaultText();

	return true;
}

void StatusBar::SetText(Part part, const char* text)
{
	SendMessage(hWnd, SB_SETTEXT, part, (LPARAM)text);
}

void StatusBar::SetDefaultText()
{
	SetText(Part::FirstPart, "No level loaded.");
	SetText(Part::SecondPart, "");
}

void StatusBar::Resize()
{
	SendMessage(hWnd, WM_SIZE, 0, 0);
}

int StatusBar::GetHeight()
{
	RECT rect;
	SendMessage(hWnd, SB_GETRECT, 0, (LPARAM)&rect);
	return rect.bottom - rect.top;
}
