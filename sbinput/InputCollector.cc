#include "InputCollector.h"
#include <iomanip>

bool InputCollector::Create(HWND hWnd)
{
	HRESULT hr = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (void **)&directInput, NULL);
	if (FAILED(hr))
	{
		MessageBox(hWnd, "Failed to create DirectInput.", 0, MB_ICONERROR);
		return false;
	}

	hr = directInput->CreateDevice(GUID_SysKeyboard, &keyboardDevice, NULL);
	if (FAILED(hr))
	{
		MessageBox(hWnd, "Failed to create keyboard device.", 0, MB_ICONERROR);
		return false;
	}

	hr = keyboardDevice->SetDataFormat(&c_dfDIKeyboard);
	if (FAILED(hr))
	{
		MessageBox(hWnd, "Failed to set keyboard data format.", 0, MB_ICONERROR);
		return false;
	}

	hr = keyboardDevice->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	if (FAILED(hr))
	{
		MessageBox(hWnd, "Failed to set keyboard cooperative level.", 0, MB_ICONERROR);
		return false;
	}

	keyboardDevice->Acquire();

	hr = directInput->CreateDevice(GUID_SysMouse, &mouseDevice, NULL);
	if (FAILED(hr))
	{
		MessageBox(hWnd, "Failed to create mouse device.", 0, MB_ICONERROR);
		return false;
	}

	hr = mouseDevice->SetDataFormat(&c_dfDIMouse2);
	if (FAILED(hr))
	{
		MessageBox(hWnd, "Failed to set mouse data format.", 0, MB_ICONERROR);
		return false;
	}

	hr = mouseDevice->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	if (FAILED(hr))
	{
		MessageBox(hWnd, "Failed to set mouse cooperative level.", 0, MB_ICONERROR);
		return false;
	}

	mouseDevice->Acquire();
	return true;
}

void InputCollector::Destroy()
{
	if (mouseDevice)
	{
		mouseDevice->Unacquire();
		mouseDevice->Release();
	}

	if (keyboardDevice)
	{
		keyboardDevice->Unacquire();
		keyboardDevice->Release();
	}

	if (directInput)
	{
		directInput->Release();
	}
}

void InputCollector::UpdateInput()
{
	if (FAILED(keyboardDevice->GetDeviceState(sizeof(keyStates), keyStates)))
	{
		keyboardDevice->Acquire();
		keyboardDevice->GetDeviceState(sizeof(keyStates), keyStates);
	}

	if (FAILED(mouseDevice->GetDeviceState(sizeof(mouseState), &mouseState)))
	{
		mouseDevice->Acquire();
		mouseDevice->GetDeviceState(sizeof(mouseState), &mouseState);
	}
}

bool InputCollector::IsKeyDown(int key)
{
	return (keyStates[key] & 0x80) != 0;
}

DIMOUSESTATE2 InputCollector::GetMouseState()
{
	return std::move(mouseState);
}

bool InputCollector::IsRightMouseButtonDown()
{
	return (mouseState.rgbButtons[1] & 0x80) != 0;
}
