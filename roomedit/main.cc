/*
*	Room Editor Application
*	(C) Moczulski Alan, 2023.
*/

#include <Windows.h>
#include <objbase.h>		//CoInitialize
#include <commctrl.h>		//InitCommonControls

#include "sbinput/InputCollector.h"
#include "scene_view.h"

static HINSTANCE instance;	//main application instance
static HWND hwnd;			//handle for the main window

static constexpr const char* CLASS_NAME = "RoomEditWindow";
static constexpr const char* WINDOW_TITLE = "Room Editor II";

static InputCollector inputCollector;

//subwindows of the main window
static HWND sceneViewWnd;
static HWND xxxViewWnd;
//menus and submenus
static HMENU hMenu;
static HMENU hFileMenu;
static HMENU hCameraMenu;

//menu item identifiers
enum MenuItem: unsigned int
{
	IDM_FILE_OPEN = 1,
	IDM_FILE_CLOSE,
	IDM_FILE_EXIT,
	IDM_CAMERA_RESET,
	IDM_CAMERA_SETTINGS
};

static void DisableMenuItem(MenuItem mi)
{
	EnableMenuItem(hFileMenu, mi, MF_BYCOMMAND | MF_GRAYED);
	DrawMenuBar(hwnd);
}

static void EnableMenuItem(MenuItem mi)
{
	EnableMenuItem(hFileMenu, mi, MF_BYCOMMAND | MF_ENABLED);
	DrawMenuBar(hwnd);
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_CREATE:
	{
		if (!inputCollector.Create(hWnd))
			return -1;

		// Create a menu
		hMenu = CreateMenu();

		// Create File menu and add items to it
		hFileMenu = CreatePopupMenu();
		AppendMenu(hFileMenu, MF_STRING, IDM_FILE_OPEN, "&Open");
		AppendMenu(hFileMenu, MF_STRING, IDM_FILE_CLOSE, "&Close");
		DisableMenuItem(IDM_FILE_CLOSE);
		AppendMenu(hFileMenu, MF_STRING, IDM_FILE_EXIT, "E&xit");

		// Add File menu to the main menu
		AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hFileMenu, "&File");

		hCameraMenu = CreatePopupMenu();
		AppendMenu(hCameraMenu, MF_STRING, IDM_CAMERA_RESET, "&Reset");
		AppendMenu(hCameraMenu, MF_STRING, IDM_CAMERA_SETTINGS, "&Settings...");

		AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hCameraMenu, "&Camera");

		// Set the menu to the main window
		SetMenu(hWnd, hMenu);

		//create the scene view subwindow
		sceneViewWnd = CreateSceneView(instance, hWnd, "SceneViewWindow");
		if (!sceneViewWnd)
			return -1;

		//create the XXX subwindow
		WNDCLASSEX wc = {};
		wc.cbSize = sizeof(wc);
		wc.hInstance = instance;
//		wc.hIcon = LoadIcon(instance, 0x65);
		wc.hCursor = LoadCursor(0, IDC_SIZEALL);
//		wc.hbrBackground = CreateSolidBrush(RGB(64, 64, 64));
		wc.lpfnWndProc = DefWindowProc;
		wc.lpszClassName = "XXXViewWnd";
		if (!RegisterClassEx(&wc))
			return false;
		xxxViewWnd = CreateWindowEx(
			WS_EX_CLIENTEDGE,
			wc.lpszClassName,
			"XXX View",
			WS_CHILD | WS_VISIBLE,
			800, 0,
			200, 600,
			hWnd,
			(HMENU)4001,				//menu or control identifier
			instance,
			0
		);
		if (!xxxViewWnd)
			return -1;

		return 0; //ok
	}
	case WM_DESTROY:
	{
		//make sure the level has been unloaded
		UnloadLevel();

		inputCollector.Destroy();

		PostQuitMessage(0);
		return 0;
	}
	case WM_SIZE:
	{
		//we need to recalculate the layout of all the subwindows
		//get the new size of the client area of the main window
		int clientWidth = LOWORD(lParam);
		int clientHeight = HIWORD(lParam);

		int sceneViewWidth = clientWidth - 400;
		int sceneViewHeight = clientHeight;
		int xxxViewWidth = clientWidth - sceneViewWidth;
		int xxxViewHeight = clientHeight;

		//move and resize the SceneView subwindow
		MoveWindow(sceneViewWnd, 0, 0, sceneViewWidth, sceneViewHeight, FALSE);

		//move and resize the XXXView subwindow
		MoveWindow(xxxViewWnd, sceneViewWidth, 0, xxxViewWidth, xxxViewHeight, FALSE);
		break;
	}
//	case WM_ERASEBKGND:
//	{
//		return 0;
//	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam))
		{
		case IDM_FILE_OPEN:
		{
			OPENFILENAME ofn = {};
			char szFile[MAX_PATH] = "";

			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = hWnd;
			ofn.lpstrFile = szFile;
			ofn.nMaxFile = sizeof(szFile);
			ofn.lpstrFilter = "Eden Level Files\0*.EDN\0All Files\0*.*\0";
			ofn.nFilterIndex = 1;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
			if (GetOpenFileName(&ofn))
			{
				if (!LoadLevel(ofn.lpstrFile))
					MessageBox(hWnd, "Failed to load the level file.", WINDOW_TITLE, MB_ICONERROR);
				else
					EnableMenuItem(IDM_FILE_CLOSE); //allow closing the level file
			}

			break;
		}
		case IDM_FILE_CLOSE:
			UnloadLevel();
			DisableMenuItem(IDM_FILE_CLOSE); //forbid closing the level file
			break;
		case IDM_FILE_EXIT:
			PostQuitMessage(0);
			break;
		case IDM_CAMERA_RESET:
			ResetCamera();
			break;
		case IDM_CAMERA_SETTINGS:
			MessageBox(hWnd, "Code me!", WINDOW_TITLE, MB_ICONINFORMATION);
			break;
		default:
			break;
		}
		break;
	}
	default:
		break;
	}

	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

bool CreateApp(int nShowCmd = SW_SHOW)
{
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = WndProc;
	wc.hInstance = instance;
//	wc.hIcon = LoadIcon(instance, 0x65);
//	wc.hbrBackground = CreateSolidBrush(0);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.lpszClassName = CLASS_NAME;
	if (!RegisterClassEx(&wc))
		return false;

	hwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		CLASS_NAME,
		WINDOW_TITLE,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		1920,
		1080,
		0,
		0,
		instance,
		nullptr
	);
	if (!hwnd)
		return false;

	ShowWindow(hwnd, nShowCmd);

	return true;
}

void DestroyApp()
{
	DestroyWindow(hwnd);
	UnregisterClass(CLASS_NAME, instance);
}

class Timer
{
	LARGE_INTEGER perfCounterFrequency;
	LARGE_INTEGER lastPerfCounter;

public:
	Timer()
	{
		QueryPerformanceFrequency(&perfCounterFrequency);
		QueryPerformanceCounter(&lastPerfCounter);
	}

	float GetDeltaTime()
	{
		LARGE_INTEGER currentPerfCounter;
		QueryPerformanceCounter(&currentPerfCounter);

		float deltaTime = static_cast<float>(currentPerfCounter.QuadPart - lastPerfCounter.QuadPart) / perfCounterFrequency.QuadPart;
		lastPerfCounter = currentPerfCounter;

		return deltaTime;
	}

	bool ShouldSleep(float targetDeltaTime)
	{
		LARGE_INTEGER currentPerfCounter;
		QueryPerformanceCounter(&currentPerfCounter);

		float elapsedTime = static_cast<float>(currentPerfCounter.QuadPart - lastPerfCounter.QuadPart) / perfCounterFrequency.QuadPart;
		return elapsedTime < targetDeltaTime;
	}

	void SleepForRemainingTime(float targetDeltaTime)
	{
		LARGE_INTEGER currentPerfCounter;
		QueryPerformanceCounter(&currentPerfCounter);

		float elapsedTime = static_cast<float>(currentPerfCounter.QuadPart - lastPerfCounter.QuadPart) / perfCounterFrequency.QuadPart;
		if (elapsedTime < targetDeltaTime)
		{
			DWORD sleepTime = static_cast<DWORD>((targetDeltaTime - elapsedTime) * 1000.0f);
			Sleep(sleepTime);
		}
	}
};

void RunApp()
{
	constexpr float targetFPS = 60.0f;
	constexpr float targetDeltaTime = 1.0f / targetFPS;
	Timer timer;

	//main event loop
	bool isRunning = true;
	while (isRunning)
	{
		float dt = timer.GetDeltaTime();

		//if enough time has passed, we can fetch input and update the scene
		if (dt >= targetDeltaTime)
		{
			inputCollector.UpdateInput();

			TickSceneView(inputCollector, dt);
		}

		//we also have to process Windows messages
		{
			MSG msg;
			while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT)
				{
					//end the loop if WM_QUIT message is received
					isRunning = false;
					break;
				}

				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		//sleep if there's still some time left
		timer.SleepForRemainingTime(targetDeltaTime);
	}
}

/*
*	Main entry-point.
*/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	instance = hInstance;

	//avoid bluriness on high-DPI monitors
	SetProcessDPIAware();

	//initialise COM early (for Direct3D), this can result in a speedup later on
	if FAILED(CoInitialize(0))
	{
		MessageBox(0, "Failed to initialise COM.", WINDOW_TITLE, MB_ICONERROR);
		return 1;
	}

	//initialise common Windows controls
	//Win95-era controls are sufficient
	INITCOMMONCONTROLSEX iccex;
	iccex.dwSize = sizeof(iccex);
	iccex.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&iccex);

	bool ok = CreateApp();
	if (ok)
		RunApp();
	else
		MessageBox(hwnd, "An error occured.", WINDOW_TITLE, MB_ICONERROR);
	DestroyApp();

	CoUninitialize();
	return 0;
}
