/*
*	Room Editor Application
*	(C) Moczulski Alan, 2023.
*/

#include "InputSystem.hh"
#include "Document.hh"
#include "SceneView.hh"
#include "ui/ButtonsPanel.hh"
#include "ui/TreeView.hh"
#include "ui/PropertyGrid.hh"
#include "ui/StatusBar.hh"
#include "ui/colors.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <commctrl.h> //InitCommonControls

//handle for the main window
static HWND mainWnd;

static InputSystem input;

//menus
static HMENU mainMenu;
//submenus
static HMENU fileSubMenu;
static HMENU cameraSubMenu;

//world data
static Document document;

//scene view
static SceneView sceneView(document);

//buttons panel
static ButtonsPanel buttonsPanel(document);
//scene tree view
static TreeView treeView(document);
//property grid view
static PropertyGrid propertyGrid;
//bar at the bottom of the window
static StatusBar bottomBar;

//menu item identifiers
enum MenuItem : unsigned int
{
	IDM_FILE_OPEN = 1,
	IDM_FILE_CLOSE,
	IDM_FILE_EXIT,
	IDM_CAMERA_RESET,
	IDM_CAMERA_SETTINGS
};

static void DisableMenuItem(HMENU subMenu, MenuItem mi)
{
	EnableMenuItem(subMenu, mi, MF_BYCOMMAND | MF_GRAYED);
	DrawMenuBar(mainWnd);
}

static void EnableMenuItem(HMENU subMenu, MenuItem mi)
{
	EnableMenuItem(subMenu, mi, MF_BYCOMMAND | MF_ENABLED);
	DrawMenuBar(mainWnd);
}
#if 0
//only elements of subtype Object can be updated!
static void UpdatePropertyGrid(PropertyGrid& propertyGrid, PackedID pid, const GameWorld &world)
{
	propertyGrid.Empty();

	char buf[64];
	switch (pid.GetType())
	{
	case ObjectType::Light:
	{
		const Light &light = world.lights[pid.GetIndex()];

		strncpy(buf, light.GetTypeString(), sizeof(buf));
		propertyGrid.AddProperty("Light Type", buf);

		snprintf(buf, sizeof(buf), "%d %d %d", light.color[0], light.color[1], light.color[2]);
		propertyGrid.AddProperty("Color", buf);

		snprintf(buf, sizeof(buf), "%f", light.intensity);
		propertyGrid.AddProperty("Intensity", buf);

		snprintf(buf, sizeof(buf), "%f", light.falloff);
		propertyGrid.AddProperty("Falloff", buf);

		snprintf(buf, sizeof(buf), "%f", light.hotspot);
		propertyGrid.AddProperty("Hotspot", buf);

		snprintf(buf, sizeof(buf), "%f", light.ambient);
		propertyGrid.AddProperty("Ambient", buf);

		break;
	}
	case ObjectType::Room:
	{
		const Room &room = world.rooms[pid.GetIndex()];

		snprintf(buf, sizeof(buf), "%f", room.scale);
		propertyGrid.AddProperty("Scale", buf);

		snprintf(buf, sizeof(buf), "%f %f %f", room.position.x, room.position.y, room.position.z);
		propertyGrid.AddProperty("X", buf);

		snprintf(buf, sizeof(buf), "%d", room.sfxAmbient);
		propertyGrid.AddProperty("SFX Ambient", buf);

		snprintf(buf, sizeof(buf), "%u", room.sfxEnvironmentData.type);
		propertyGrid.AddProperty("SFX Env Type", buf);

		snprintf(buf, sizeof(buf), "%u", room.sfxEnvironmentData.delay);
		propertyGrid.AddProperty("SFX Env Delay", buf);

		snprintf(buf, sizeof(buf), "%u", room.sfxEnvironmentData.feedback);
		propertyGrid.AddProperty("SFX Env Feedback", buf);
		break;
	}
	case ObjectType::Mesh:
	{
		const Mesh &mesh = world.meshes[pid.GetIndex()];

		snprintf(buf, sizeof(buf), "%f %f %f", mesh.minExtent.x, mesh.minExtent.y, mesh.minExtent.z);
		propertyGrid.AddProperty("Min Extent", buf);

		snprintf(buf, sizeof(buf), "%f %f %f", mesh.maxExtent.x, mesh.maxExtent.y, mesh.maxExtent.z);
		propertyGrid.AddProperty("Max Extent", buf);

		snprintf(buf, sizeof(buf), "%u", mesh.numVerts);
		propertyGrid.AddProperty("Vertices", buf);

		snprintf(buf, sizeof(buf), "%u", mesh.corners.Count());
		propertyGrid.AddProperty("Corners", buf);

		snprintf(buf, sizeof(buf), "%u", mesh.faces.Count());
		propertyGrid.AddProperty("Faces", buf);
		break;
	}
	case ObjectType::Emitter:
	{
		const Emitter &emitter = world.emitters[pid.GetIndex()];

		propertyGrid.AddProperty("Enable Color Addition", emitter.colorAddition ? "true" : "false");

		snprintf(buf, sizeof(buf), "%f", emitter.gravity);
		propertyGrid.AddProperty("Gravity", buf);

		snprintf(buf, sizeof(buf), "%d", emitter.rate);
		propertyGrid.AddProperty("Rate", buf);

		snprintf(buf, sizeof(buf), "%f", emitter.lifeTime);
		propertyGrid.AddProperty("Life Time", buf);

		snprintf(buf, sizeof(buf), "%d", emitter.timer);
		propertyGrid.AddProperty("Timer", buf);

		snprintf(buf, sizeof(buf), "%d", emitter.pause);
		propertyGrid.AddProperty("Pause", buf);

		snprintf(buf, sizeof(buf), "%d %d %d %d", emitter.startColor.r, emitter.startColor.g, emitter.startColor.b, emitter.startColor.a);
		propertyGrid.AddProperty("RGBA Start Color", buf);

		snprintf(buf, sizeof(buf), "%d %d %d %d", emitter.endColor.r, emitter.endColor.g, emitter.endColor.b, emitter.endColor.a);
		propertyGrid.AddProperty("RGBA End Color", buf);

		snprintf(buf, sizeof(buf), "%f", emitter.startScale);
		propertyGrid.AddProperty("Start Scale", buf);

		snprintf(buf, sizeof(buf), "%f", emitter.endScale);
		propertyGrid.AddProperty("End Scale", buf);

		snprintf(buf, sizeof(buf), "%f", emitter.speed);
		propertyGrid.AddProperty("Speed", buf);

		snprintf(buf, sizeof(buf), "%f", emitter.speedRandom);
		propertyGrid.AddProperty("Speed Random", buf);

		snprintf(buf, sizeof(buf), "%f", emitter.scatter);
		propertyGrid.AddProperty("Scatter", buf);

		snprintf(buf, sizeof(buf), "%f %f", emitter.areaX, emitter.areaY);
		propertyGrid.AddProperty("Area XY", buf);

		snprintf(buf, sizeof(buf), "%f", emitter.spin);
		propertyGrid.AddProperty("Spin", buf);

		snprintf(buf, sizeof(buf), "%d", emitter.imageNumber);
		propertyGrid.AddProperty("Image Number", buf);

		snprintf(buf, sizeof(buf), "%d", emitter.particleType);
		propertyGrid.AddProperty("Particle Type", buf);

		snprintf(buf, sizeof(buf), "%d", emitter.numInitialParticles);
		propertyGrid.AddProperty("Particle Initial Count", buf);

		snprintf(buf, sizeof(buf), "%d", emitter.drawableIndices.Count());
		propertyGrid.AddProperty("Drawable Indices Count", buf);

		snprintf(buf, sizeof(buf), "%f", emitter.particleLength);
		propertyGrid.AddProperty("Particle Length", buf);

		propertyGrid.AddProperty("Enable Deflection Plane", emitter.deflectionPlaneActive ? "true" : "false");

		snprintf(buf, sizeof(buf), "%f", emitter.deflectionPlaneReduction);
		propertyGrid.AddProperty("Deflection Plane Reduction", buf);
		break;
	}
	case ObjectType::SpotEffect:
	{
		const SpotEffect &spotEffect = world.spotEffects[pid.GetIndex()];
		break;
	}
	case ObjectType::Object:
	{
		const Object &object = world.objects[pid.GetIndex()];

		snprintf(buf, sizeof(buf), "%u", object.index);
		propertyGrid.AddProperty("Index", buf);

		snprintf(buf, sizeof(buf), "%u", object.location);
		propertyGrid.AddProperty("Parent Room Index", buf);

		snprintf(buf, sizeof(buf), "%p", (void *)object.next);
		propertyGrid.AddProperty("Next Ptr", buf);

		snprintf(buf, sizeof(buf), "%p", (void *)object.objects);
		propertyGrid.AddProperty("Sub-Objects Ptr", buf);

		snprintf(buf, sizeof(buf), "%u", object.drawableNumber.GetID());
		propertyGrid.AddProperty("Mesh Index", buf);

		snprintf(buf, sizeof(buf), "%f", object.radius);
		propertyGrid.AddProperty("Radius", buf);

		snprintf(buf, sizeof(buf), "%f", object.scale);
		propertyGrid.AddProperty("Scale", buf);

		snprintf(buf, sizeof(buf), "%f %f %f", object.position.x, object.position.y, object.position.z);
		propertyGrid.AddProperty("Position", buf);

		snprintf(buf, sizeof(buf), "%f %f %f", object.rotation.x, object.rotation.y, object.rotation.z);
		propertyGrid.AddProperty("Rotation", buf);

		snprintf(buf, sizeof(buf), "%u", object.type);
		propertyGrid.AddProperty("Type", buf);
		break;
	}
	case ObjectType::Actor:
	{
//		const Actor &actor = world.actors[pid.GetIndex()];
		break;
	}
	default:
		break;
	}
}
#endif

static void UpdateAllViews()
{
	treeView.Update();

	//we could also update/render the SceneView here, but it is updated/rendered every frame anyway
}

static constexpr const char* CLASS_NAME = "RoomEditWindow";
static constexpr const char* WINDOW_TITLE = "Room Editor II";
static LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_CREATE:
	{
		if (!input.Create(hWnd))
			return -1;

		//create a menu
		mainMenu = CreateMenu();

		//create File menu and add items to it
		fileSubMenu = CreatePopupMenu();
		AppendMenu(fileSubMenu, MF_STRING, IDM_FILE_OPEN, "&Open");
		AppendMenu(fileSubMenu, MF_STRING, IDM_FILE_CLOSE, "&Close");
		DisableMenuItem(fileSubMenu, IDM_FILE_CLOSE);
		AppendMenu(fileSubMenu, MF_STRING, IDM_FILE_EXIT, "E&xit");

		//add File menu to the main menu
		AppendMenu(mainMenu, MF_POPUP, (UINT_PTR)fileSubMenu, "&File");

		cameraSubMenu = CreatePopupMenu();
		AppendMenu(cameraSubMenu, MF_STRING, IDM_CAMERA_RESET, "&Reset");
		AppendMenu(cameraSubMenu, MF_STRING, IDM_CAMERA_SETTINGS, "&Settings...");

		AppendMenu(mainMenu, MF_POPUP, (UINT_PTR)cameraSubMenu, "&Camera");

		//set the menu to the main window
		SetMenu(hWnd, mainMenu);

		HINSTANCE instance = GetModuleHandle(nullptr);

		//create the scene view
		if (!sceneView.Create(instance, hWnd))
			return -1;

		//create the buttons panel
		if (!buttonsPanel.Create(instance, hWnd))
			return -1;

		//create the tree subwindow
		if (!treeView.Create(instance, hWnd))
			return -1;

		//create the property grid
		if (!propertyGrid.Create(instance, hWnd))
			return -1;

		//create the bottom status bar
		if (!bottomBar.Create(instance, hWnd))
			return -1;

		return 0; //ok
	}
	case WM_DESTROY:
	{
		sceneView.Destroy();

		//make sure the level has been unloaded
		sceneView.DestroyWorldRenderer();
		document.Reset();

		PostQuitMessage(0);
		return 0;
	}
	case WM_SIZE:
	{
		//we need to recalculate the layout of all the subwindows

		//get the new size of the client area of the main window
		int clientWidth = LOWORD(lParam);
		int clientHeight = HIWORD(lParam) - bottomBar.GetHeight();

		//define the layout sections
		int leftSectionWidth = clientWidth - 400;
		int rightSectionWidth = clientWidth - leftSectionWidth;

		//compute sizes and positions for the controls
		int sceneViewWidth = leftSectionWidth;
		int sceneViewHeight = clientHeight - 100;

		int buttonsPanelWidth = leftSectionWidth;
		int buttonsPanelHeight = clientHeight - sceneViewHeight;
		int buttonsPanelY = sceneViewHeight;

		int treeViewWidth = rightSectionWidth;
		int treeViewHeight = clientHeight - 600;

		int propertyViewWidth = rightSectionWidth;
		int propertyViewHeight = clientHeight - treeViewHeight;
		int propertyViewY = treeViewHeight;

		//move and resize child windows
		sceneView.MoveWindow(0, 0, sceneViewWidth, sceneViewHeight);
		buttonsPanel.MoveWindow(0, buttonsPanelY, buttonsPanelWidth, buttonsPanelHeight);
		treeView.MoveWindow(leftSectionWidth, 0, treeViewWidth, treeViewHeight);
		propertyGrid.MoveWindow(leftSectionWidth, propertyViewY, propertyViewWidth, propertyViewHeight);

		bottomBar.Resize();
		return 0;
	}
	case WM_INPUT:
	{
		input.Collect((HRAWINPUT)lParam);
		return 0;
	}
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
			ofn.lpstrFilter = "Project Eden Package Files\0*.EDN\0All Files\0*.*\0";
			ofn.nFilterIndex = 1;
			ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
			if (!GetOpenFileName(&ofn))
				break;

			bool loaded = document.Load(ofn.lpstrFile);
			if (!loaded)
			{
				MessageBox(hWnd, "Failed to load the specified file.", WINDOW_TITLE, MB_ICONERROR);
				break;
			}

			if (!sceneView.CreateWorldRenderer())
			{
				MessageBox(hWnd, "Failed to upload the level resources to the graphics device.", WINDOW_TITLE, MB_ICONERROR);
				document.Reset();
				break;
			}

			bottomBar.SetText(Part::FirstPart, ofn.lpstrFile);
			EnableMenuItem(fileSubMenu, IDM_FILE_CLOSE); //allow closing the level file
			UpdateAllViews();
			document.camera.Reset();
			break;
		}
		case IDM_FILE_CLOSE:
			sceneView.DestroyWorldRenderer();
			document.Reset();
			bottomBar.SetDefaultText();
			UpdateAllViews();
			DisableMenuItem(fileSubMenu, IDM_FILE_CLOSE); //forbid closing the level file
			break;
		case IDM_FILE_EXIT:
			PostQuitMessage(0);
			break;
		case IDM_CAMERA_RESET:
			document.camera.Reset();
			break;
		case IDM_CAMERA_SETTINGS:
			MessageBox(hWnd, "Code me!", WINDOW_TITLE, MB_ICONINFORMATION);
			break;
		default:
			break;
		}
		break;
	}
#if 0
	case WM_PARENTNOTIFY:
	{
		OutputDebugString("Parent\n");
		//TODO: fix me: this is called too early (the child should act first on the document, and then the tree view would need to be updated)
		bool updateChildren = LOWORD(wParam) == WM_LBUTTONDOWN;
		if (updateChildren)
			treeView.Update();
		break;
	}
#endif
	default:
		break;
	}

	//process eventual notifications from children
//	if (ProcessChildrenNotifications(Msg, wParam, lParam))
//		return 0; //ok

	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

class Application
{
	HINSTANCE instance;

public:
	constexpr Application():
		instance(nullptr)
	{}

	bool Initialise(HINSTANCE hInstance, int nShowCmd = SW_SHOW)
	{
		//initialise common Windows controls
		//Win95-era controls are sufficient
		INITCOMMONCONTROLSEX iccex;
		iccex.dwSize = sizeof(iccex);
		iccex.dwICC = ICC_WIN95_CLASSES;
		InitCommonControlsEx(&iccex);

		WNDCLASS wc{};
		wc.lpfnWndProc = WndProc;
		wc.hInstance = instance;
		//	wc.hIcon = LoadIcon(instance, 0x65);
		//	wc.hbrBackground = CreateSolidBrush(0);
		wc.hCursor = LoadCursor(0, IDC_ARROW);
		wc.lpszClassName = CLASS_NAME;
		if (!RegisterClass(&wc))
			return false;

		//compute the window X and Y so that it shows up in the center of the screen
		int windowWidth = GetSystemMetrics(SM_CXSCREEN) >> 1;
		int windowHeight = GetSystemMetrics(SM_CYSCREEN) >> 1;
		int windowX = windowWidth >> 1;
		int windowY = windowHeight >> 1;

		mainWnd = CreateWindowEx(
			WS_EX_CLIENTEDGE,
			CLASS_NAME,
			WINDOW_TITLE,
			WS_OVERLAPPEDWINDOW,
			windowX,
			windowY,
			windowWidth,
			windowHeight,
			nullptr,
			nullptr,
			instance,
			nullptr
		);
		if (!mainWnd)
			return false;

		ShowWindow(mainWnd, nShowCmd);
		return true;
	}

	void Shutdown()
	{
		DestroyWindow(mainWnd);
		UnregisterClass(CLASS_NAME, instance);
	}

	bool Tick(float dt)
	{
		//process all Windows messages
		MSG msg;
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				return false;

			//make sure all the messages are passed to our main WndProc, which will dispatch them further later on
	//		msg.hwnd = hWnd;

			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (document.isDirty)
			{
				UpdateAllViews();
				document.isDirty = false;
			}
		}

		sceneView.Tick(input, dt);
		return true;
	}
};

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

/*
*	Main entry-point.
*/
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	//avoid bluriness on high-DPI monitors
	SetProcessDPIAware();

	Application app;
	if (!app.Initialise(hInstance, nShowCmd))
		return 1;
	constexpr float targetFPS = 120.0f; //TODO: investigate why this needs to be that high!
	constexpr float targetDeltaTime = 1.0f / targetFPS;
	Timer timer;

	//main event loop
	float dtAccumulator = 0.0f;
	while (true)
	{

		//get time elapsed since last call
		float dt = timer.GetDeltaTime();
		dtAccumulator += dt;

		//if enough time has passed, we can tick
		if (dtAccumulator >= targetDeltaTime)
		{
			dtAccumulator = 0.0f;

			if (!app.Tick(dt))
				break;

			//sleep if there's still some time left
			timer.SleepForRemainingTime(targetDeltaTime);
		}
	}
	app.Shutdown();
	return 0;
}
