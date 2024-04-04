/*
*	Room Editor Application
*	(C) Moczulski Alan, 2023.
*/

#include "ButtonsPanel.hh"
#include <CommCtrl.h>
#include <stdint.h>
#include "colors.h"

enum CONTROLID : uint32_t
{
	FIRST,

	FIRST_BUTTON = FIRST,
	MOVE = FIRST_BUTTON,
	ROTATE,
	SCALE,
	UNDO,
	REDO,

	FIRST_CHECKBOX,
	DRAW_LIGHTS = FIRST_CHECKBOX,
	DRAW_TRIGGERS,
	DRAW_ROOMS,
	DRAW_EMITTERS,
	DRAW_SPOT_EFFECTS,
	DRAW_OBJECTS,
	DRAW_ACTORS,
	DRAW_HELPERS,

	LAST
};
static constexpr uint32_t NUM_CONTROLS = CONTROLID::LAST - CONTROLID::FIRST;
static constexpr uint32_t NUM_BUTTONS = CONTROLID::FIRST_CHECKBOX - CONTROLID::FIRST;
static constexpr uint32_t NUM_CHECKBOXES = CONTROLID::LAST - CONTROLID::FIRST_CHECKBOX;

struct CONTROL
{
	HWND hWnd;
	const char *title;
};

static CONTROL CONTROLS[NUM_CONTROLS] =
{
	{nullptr, "Move"},
	{nullptr, "Rotate"},
	{nullptr, "Scale"},
	{nullptr, "Undo"},
	{nullptr, "Redo"},
	{nullptr, "Lighting"},
	{nullptr, "Draw Triggers"},
	{nullptr, "Draw Rooms"},
//	{nullptr, "Draw Visible Rooms Only"},
	{nullptr, "Draw Emitters"},
	{nullptr, "Draw Spot Effects"},
	{nullptr, "Draw Objects"},
	{nullptr, "Draw Actors"},
	{nullptr, "Draw Visible Rooms Only"} //THIS TEMPORARILY REPLACES "Draw Helpers"
};

static HWND CreateButton(HWND hParentWnd, const char *text, CONTROLID id, HINSTANCE hInstance)
{
	return CreateWindow(
		"BUTTON",
		text,
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		0, 0, 0, 0,
		hParentWnd,
		(HMENU)id,
		hInstance,
		nullptr
	);
}

static HWND CreateCheckBox(HWND hParentWnd, const char *text, CONTROLID id, HINSTANCE hInstance)
{
	return CreateWindow(
		"BUTTON",
		text,
		WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
		0, 0, 0, 0,
		hParentWnd,
		(HMENU)id,
		hInstance,
		nullptr
	);
}

LRESULT CALLBACK ButtonsPanel::SubWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	ButtonsPanel *thiss = (ButtonsPanel*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	switch (uMsg)
	{
	case WM_COMMAND:
	{
		auto id = (CONTROLID)LOWORD(wParam) - 4200;
		int wmEvent = HIWORD(wParam);

		if (wmEvent == BN_CLICKED)
		{
			bool isChecked = (SendMessage((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED);
			switch (id)
			{
			case DRAW_LIGHTS:
				thiss->document.drawLights = isChecked;
				break;
			case DRAW_TRIGGERS:
				thiss->document.drawTriggers = isChecked;
				break;
			case DRAW_ROOMS:
				thiss->document.drawRooms = isChecked;
				break;
			case DRAW_EMITTERS:
				thiss->document.drawEmitters = isChecked;
				break;
			case DRAW_SPOT_EFFECTS:
				thiss->document.drawSpotEffects = isChecked;
				break;
			case DRAW_OBJECTS:
				thiss->document.drawObjects = isChecked;
				break;
			case DRAW_ACTORS:
				thiss->document.drawActors = isChecked;
				break;
			case DRAW_HELPERS:
				thiss->document.drawHelpers = isChecked;
				break;
			default:
				break;
			}
		}
		break;
	}
	default:
		break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

bool ButtonsPanel::Create(HINSTANCE hInstance, HWND hParentWnd)
{
	//create the window panel
	hWnd = CreateWindowEx(WS_EX_CLIENTEDGE, "STATIC", nullptr, WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hParentWnd, nullptr, hInstance, nullptr);
	if (!hWnd)
		return false;

	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)this);
	SetWindowSubclass(hWnd, SubWndProc, 0, 0);

	//create the buttons
	uint32_t i = 0;
	for (; i < NUM_BUTTONS; i++)
	{
		CONTROLS[i].hWnd = CreateButton(hWnd, CONTROLS[i].title, (CONTROLID)(4200 + i), hInstance);
		if (!CONTROLS[i].hWnd)
			return false;
	}

	//create the check boxes
	for (; i < NUM_CONTROLS; i++)
	{
		CONTROLS[i].hWnd = CreateCheckBox(hWnd, CONTROLS[i].title, (CONTROLID)(4200 + i), hInstance);
		if (!CONTROLS[i].hWnd)
			return false;

		auto id = i;
		bool isChecked = false;
		switch (id)
		{
		case DRAW_LIGHTS:
			isChecked = document.drawLights;
			break;
		case DRAW_TRIGGERS:
			isChecked = document.drawTriggers;
			break;
		case DRAW_ROOMS:
			isChecked = document.drawRooms;
			break;
		case DRAW_EMITTERS:
			isChecked = document.drawEmitters;
			break;
		case DRAW_SPOT_EFFECTS:
			isChecked = document.drawSpotEffects;
			break;
		case DRAW_OBJECTS:
			isChecked = document.drawObjects;
			break;
		case DRAW_ACTORS:
			isChecked = document.drawActors;
			break;
		case DRAW_HELPERS:
			isChecked = document.drawHelpers;
			break;
		default:
			break;
		}

		SendMessage(CONTROLS[i].hWnd, BM_SETCHECK, isChecked, 0);
	}

	return true;
}

void ButtonsPanel::MoveWindow(int X, int Y, int nWidth, int nHeight)
{
	::MoveWindow(hWnd, X, Y, nWidth, nHeight, TRUE);

	const uint32_t buttonWidth = nWidth / 5;
	constexpr uint32_t buttonHeight = 24;
	constexpr uint32_t maxColumns = 5;

	//place buttons
	uint32_t buttonY;
	uint32_t i = 0;
	for (; i < NUM_BUTTONS; ++i)
	{
		uint32_t column = i % maxColumns;
		uint32_t row = i / maxColumns;

		uint32_t buttonX = column * buttonWidth;
		buttonY = row * buttonHeight;

		::MoveWindow(CONTROLS[i].hWnd, buttonX, buttonY, buttonWidth, buttonHeight, TRUE);
	}
	uint32_t checkBoxesButtonYStart = buttonY + buttonHeight;

	//place checkboxes just under the buttons
	for (; i < NUM_CONTROLS; ++i)
	{
		uint32_t column = i % maxColumns;
		uint32_t row = i / maxColumns;

		uint32_t buttonX = column * buttonWidth;
		uint32_t buttonY = checkBoxesButtonYStart + row * buttonHeight;

		::MoveWindow(CONTROLS[i].hWnd, buttonX, buttonY, buttonWidth, buttonHeight, TRUE);
	}
}

#if 0
bool ButtonsPanel::CanDrawLights()
{
	return SendMessage(CONTROLS[DRAW_LIGHTS].hWnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

bool ButtonsPanel::CanDrawTriggers()
{
	return SendMessage(CONTROLS[DRAW_TRIGGERS].hWnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

bool ButtonsPanel::CanDrawRooms()
{
	return SendMessage(CONTROLS[DRAW_ROOMS].hWnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

bool ButtonsPanel::CanDrawEmitters()
{
	return SendMessage(CONTROLS[DRAW_EMITTERS].hWnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

bool ButtonsPanel::CanDrawSpotEffects()
{
	return SendMessage(CONTROLS[DRAW_SPOT_EFFECTS].hWnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

bool ButtonsPanel::CanDrawObjects()
{
	return SendMessage(CONTROLS[DRAW_OBJECTS].hWnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

bool ButtonsPanel::CanDrawActors()
{
	return SendMessage(CONTROLS[DRAW_ACTORS].hWnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

bool ButtonsPanel::CanDrawHelpers()
{
	return SendMessage(CONTROLS[DRAW_HELPERS].hWnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
}
#endif
