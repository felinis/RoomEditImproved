#pragma once
#include <Windows.h>
#include <hidusage.h>
#include <stdint.h>
#include <assert.h>

//input collection system using RawInput
class InputSystem
{
	int32_t xPosRelative;
	int32_t yPosRelative;
	bool leftButtonPressed;
	bool rightButtonPressed;
	bool middleButtonPressed;
	bool keys[128];

public:
	InputSystem():
		xPosRelative(0),
		yPosRelative(0),
		leftButtonPressed(false),
		rightButtonPressed(false),
		middleButtonPressed(false),
		keys()
	{}

	//do not allow copy constructor and move constructor
	InputSystem(const InputSystem &other) = delete;
	InputSystem(InputSystem &&other) = delete;

	bool Create(HWND hWnd)
	{
		RAWINPUTDEVICE rid[2]{};
		rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC; //HID_USAGE_PAGE_GAME
		rid[0].usUsage = HID_USAGE_GENERIC_MOUSE;
		rid[0].hwndTarget = hWnd;
		rid[1].usUsagePage = HID_USAGE_PAGE_GENERIC; //HID_USAGE_PAGE_GAME
		rid[1].usUsage = HID_USAGE_GENERIC_KEYBOARD;
		rid[1].dwFlags = RIDEV_NOLEGACY;
		rid[1].hwndTarget = hWnd;
		return RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE));
	}

	void Collect(HRAWINPUT hRawInput)
	{
		char tempData[128];

		UINT dataSize;
		GetRawInputData(hRawInput, RID_INPUT, nullptr, &dataSize, sizeof(RAWINPUTHEADER));
		assert(dataSize <= 128);
		//w dokumentacji Microsoftu mówi¹ ¿e trzeba przydzieliæ pamiêæ rozmiaru dataSize za pomoc¹ operatora new,
		//i to przy ka¿dym wywo³aniu tej funkcji... chyba im siê w g³owie poprzewraca³o

		GetRawInputData(hRawInput, RID_INPUT, tempData, &dataSize, sizeof(RAWINPUTHEADER));
		RAWINPUT *raw = (RAWINPUT *)tempData;
		if (raw->header.dwType == RIM_TYPEMOUSE)
		{
			RAWMOUSE &mouse = raw->data.mouse;
			if (mouse.usFlags == MOUSE_MOVE_RELATIVE)
			{
				xPosRelative = mouse.lLastX;
				yPosRelative = mouse.lLastY;
			}
			if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)
				leftButtonPressed = true;
			if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)
				leftButtonPressed = false;
			if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)
				rightButtonPressed = true;
			if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
				rightButtonPressed = false;
			if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN)
				middleButtonPressed = true;
			if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP)
				middleButtonPressed = false;
//			if (mouse.usButtonFlags & RI_MOUSE_WHEEL)
//			{
//				delta = (short)mouse.usButtonData;
//			}
		}
		else if (raw->header.dwType == RIM_TYPEKEYBOARD)
		{
			RAWKEYBOARD &keyboard = raw->data.keyboard;
			if (keyboard.Message == WM_KEYDOWN || keyboard.Message == WM_SYSKEYDOWN) //key pressed
			{
				keys[keyboard.VKey] = true;
			}
			else if (keyboard.Message == WM_KEYUP || keyboard.Message == WM_SYSKEYDOWN) //key depressed
			{
				keys[keyboard.VKey] = false;
			}
		}
	}

	bool IsLeftMousePressed() const
	{
		return leftButtonPressed;
	}

	bool IsRightMousePressed() const
	{
		return rightButtonPressed;
	}

	bool IsMiddleMousePressed() const
	{
		return middleButtonPressed;
	}

	int32_t GetMouseX() const
	{
		return xPosRelative;
	}

	int32_t GetMouseY() const
	{
		return yPosRelative;
	}

	void ZeroMouse()
	{
		xPosRelative = 0;
		yPosRelative = 0;
	}

	bool IsKeyPressed(uint32_t key) const
	{
		return keys[key];
	}
};
