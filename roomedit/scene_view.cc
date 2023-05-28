#include "scene_view.h"
#include "FreeLookCamera.h"
#include "world_renderer.h"

static sbRenderer renderer;

static FreeLookCamera camera;
static GameWorld world; //the game data
static bool levelLoaded; //has the level been loaded?

static void Render()
{
	if (!levelLoaded)
		return;

	if (renderer.StartFrame(camera.GetProjMatrix() * camera.GetViewMatrix()))
	{
		RenderWorld(world, renderer);

		renderer.EndAndPresentFrame();
	}
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_CREATE:
	{
		if (!renderer.Create(hWnd))
			return -1;

		return 0; //ok
	}
	case WM_DESTROY:
	{
		renderer.Destroy();
		return 0;
	}
	case WM_SIZE:
	{
//		sGC.Resize(); //TODO
		Render();
		return 0;
	}
	case WM_ERASEBKGND:
	{
		return 0;
	}
	default:
		break;
	}

	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

HWND CreateSceneView(HINSTANCE hInstance, HWND hParentWnd, const char *className)
{
	WNDCLASSEX wc;
	memset(&wc, 0, sizeof(wc));
	wc.cbSize = sizeof(wc);
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
//	wc.hIcon = LoadIcon(instance, 0x65);
//	wc.hbrBackground = CreateSolidBrush(0);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.lpszClassName = className;
	if (!RegisterClassEx(&wc))
		return nullptr;

	HWND hwnd = CreateWindow(
		wc.lpszClassName,
		"Scene View",
		WS_CHILD | WS_VISIBLE,
		0, 0,
		800, 600,
		hParentWnd,
		(HMENU)4000,				//menu or control identifier
		hInstance,
		nullptr
	);

	return hwnd;
}

bool LoadLevel(const char* pathToFile)
{
	if (levelLoaded)
		UnloadLevel();

	if (!world.Load(pathToFile))
		return false;

	if (!CreateWorldRenderer(world, renderer))
		return false;

	levelLoaded = true;

	return true;
}

void UnloadLevel()
{
	if (!levelLoaded)
		return;

	levelLoaded = false;

	DestroyWorldRenderer(renderer);
	world.Unload();
}

static POINT prevMousePos;
static bool isMouseInitialized;
static bool isRightMouseButtonDown;
static void UpdateCamera(InputCollector& inputCollector, float dt)
{
	bool dirty = false;

	constexpr float cameraSpeed = 2048.0f;
	constexpr float rotationSpeed = 0.02f;

	//keyboard input for camera movement
	if (inputCollector.IsKeyDown(DIK_W))
	{
		camera.Move(FreeLookCamera::MovementDirection::FORWARD, cameraSpeed * dt);
		dirty = true;
	}
	if (inputCollector.IsKeyDown(DIK_S))
	{
		camera.Move(FreeLookCamera::MovementDirection::BACKWARD, cameraSpeed * dt);
		dirty = true;
	}
	if (inputCollector.IsKeyDown(DIK_A))
	{
		camera.Move(FreeLookCamera::MovementDirection::LEFT, cameraSpeed * dt);
		dirty = true;
	}
	if (inputCollector.IsKeyDown(DIK_D))
	{
		camera.Move(FreeLookCamera::MovementDirection::RIGHT, cameraSpeed * dt);
		dirty = true;
	}

	//mouse input for camera rotation
	if (inputCollector.IsRightMouseButtonDown())
	{
		DIMOUSESTATE2 mouseState = inputCollector.GetMouseState();
		float deltaX = static_cast<float>(mouseState.lX) * rotationSpeed;
		float deltaY = static_cast<float>(mouseState.lY) * rotationSpeed;

		camera.Rotate(deltaX, deltaY);
		dirty = true;
	}

	if (dirty)
		Render();
}

void TickSceneView(InputCollector& inputCollector, float dt)
{
	UpdateCamera(inputCollector, dt);
}

void ResetCamera()
{
	camera.Reset();
	Render();
}
