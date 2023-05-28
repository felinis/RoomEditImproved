#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

class InputCollector
{
	LPDIRECTINPUT8 directInput;
	LPDIRECTINPUTDEVICE8 keyboardDevice;
	LPDIRECTINPUTDEVICE8 mouseDevice;

	DIMOUSESTATE2 mouseState;
	BYTE keyStates[256];

public:
	InputCollector() : directInput(nullptr), keyboardDevice(nullptr), mouseDevice(nullptr),
		mouseState(), keyStates() {}

	bool Create(HWND hWnd);
	void Destroy();

	void UpdateInput();
	bool IsKeyDown(int key);
	DIMOUSESTATE2 GetMouseState();
	bool IsRightMouseButtonDown();

	//TODO: add functions and variables for other input devices like XInput here
};
