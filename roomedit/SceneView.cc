/*
*	Room Editor Application
*	(C) Moczulski Alan, 2023.
*/

#include "SceneView.hh"

void SceneView::FromScreenPointToWorldRays(const POINTS &screenPoint, Vector &rayStart, Vector &rayDir)
{
	//unproject from front camera plane
	Vector worldStart;
	document.camera.UnProjectFromScreen(Vector(screenPoint.x, screenPoint.y, 0.0f), worldStart);
	if (!worldStart.Valid())
		return;

	//unproject from back camera plane
	Vector worldEnd;
	document.camera.UnProjectFromScreen(Vector(screenPoint.x, screenPoint.y, 1.0f), worldEnd);
	if (!worldEnd.Valid())
		return;

	//compute direction
	Vector dir = (worldEnd - worldStart);
	dir.Normalise();

	rayStart = worldStart;
	rayDir = dir;
}

LRESULT CALLBACK SceneView::WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	SceneView *thiss = (SceneView*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	switch (Msg)
	{
	case WM_MOUSEMOVE:
	{
		thiss->OnMouseMove(wParam, lParam);
		return 0;
	}
	case WM_LBUTTONDOWN:
	{
		thiss->OnLButtonDown(wParam, lParam);
		thiss->document.isDirty = true;
		return 0;
	}
	case WM_LBUTTONUP:
	{
		thiss->OnLButtonUp();
		return 0;
	}
	case WM_RBUTTONDOWN:
	{
//		CreatePopupMenu();
		return 0;
	}
	default:
		break;
	}

	return DefWindowProc(hWnd, Msg, wParam, lParam);
}

bool SceneView::Create(HINSTANCE hInstance, HWND hParentWnd)
{
	WNDCLASS wc{};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
//	wc.hIcon = LoadIcon(instance, 0x65);
//	wc.hbrBackground = CreateSolidBrush(0);
	wc.hCursor = LoadCursor(0, IDC_ARROW); //TODO: change to something different, like a cross for instance
	wc.lpszClassName = "SceneViewWindow";
	if (!RegisterClass(&wc))
		return false;

	hWnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		wc.lpszClassName,
		"Scene View",
		WS_CHILD,
		0, 0,
		0, 0,
		hParentWnd,
		nullptr,
		hInstance,
		nullptr
	);
	if (!hWnd)
		return false;

	SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)this);
	//now we can show the window, since we set the GWLP_USERDATA
	ShowWindow(hWnd, SW_SHOW);

	if (!renderer.Create(hWnd))
		return false;

	if (!worldRenderer.Create())
		return false;

	return true;
}

void SceneView::Destroy()
{
	worldRenderer.Destroy();
	renderer.Destroy();
	DestroyWindow(hWnd);
}

bool SceneView::CreateWorldRenderer()
{
	assert(document.levelLoaded);
	return worldRenderer.LoadWorld(document.world);
}

//TODO: check if this is the same as previous code version,
//since we refactored a lot of stuff since then, and it is possible
//that we forgot one IF somewhere here...
void SceneView::DestroyWorldRenderer()
{
	if (document.levelLoaded)
		worldRenderer.UnloadWorld();
}

void SceneView::OnMouseMove(WPARAM wParam, LPARAM lParam)
{
	bool lButtonDown = wParam & MK_LBUTTON;
	if (lButtonDown)
	{
		//get the 2D point we just clicked on
		RECT r;
		GetClientRect(hWnd, &r);
		POINTS p = MAKEPOINTS(lParam);
		p.y = (SHORT)r.bottom - p.y;

		//convert from screen to world space
		Vector rayStart, rayDir;
		FromScreenPointToWorldRays(p, rayStart, rayDir);

		//...
	}
}

void SceneView::OnLButtonDown(WPARAM wParam, LPARAM lParam)
{
	SetCapture(hWnd);
	bool shiftDown = wParam & MK_SHIFT;
	if (!shiftDown)
		document.ResetSelection();

	bool ctrlDown = wParam & MK_CONTROL;
	bool altDown = (GetAsyncKeyState(VK_MENU) < 0);
	bool deselect = altDown; //we want to deselect stuff when ALT is held down

	//get the 2D point we just clicked on
	RECT r;
	GetClientRect(hWnd, &r);
	POINTS p = MAKEPOINTS(lParam);
	p.y = (SHORT)r.bottom - p.y; //we need to invert the Y so that it grows upwards

	//convert from screen to world space
	Vector rayStart, rayDir;
	FromScreenPointToWorldRays(p, rayStart, rayDir);

	//test for hits against every room
	//NEW IDEA FROM GUIDO: click twice to send ray further (handle DBLCLICK message)
	for (uint32_t i = 0; i < document.world.rooms.Count(); i++)
	{
		const auto &room = document.world.rooms[i];

		BBox box = ComputeRoomBBox(room);

		Vector tempPnt;
		//broad-phase intersection test
		if (!box.IsIntersectRay(rayStart, rayDir, tempPnt))
			continue;

		//select this room
		document.MarkAsSelected(i);

		//TODO: narrow-phase

//		float mindist = MathConstants::FLOAT_MAX;
//		if (hc.dist < mindist)
//		{
//			mindist = hc.dist;
//			selector.AddRoom();
//		}
	}
}

void SceneView::OnLButtonUp()
{
	//HITTEST
//		selector.Add(object);
	ReleaseCapture();
}

void SceneView::MoveWindow(int x, int y, int width, int height)
{
	::MoveWindow(hWnd, x, y, width, height, TRUE);

	if (renderer.IsReady())
	{
		document.camera.ResizeViewport(width, height);
		renderer.ResizeFramebuffer(width, height);
	}
}

void SceneView::Tick(InputSystem &is, float dt)
{
	//process eventual input changes
	{
		constexpr float cameraSpeed = 2048.0f;
		constexpr float rotationSpeed = 0.04f;

		//keyboard input for camera movement
		if (is.IsKeyPressed('W'))
		{
			document.camera.Move(FreeLookCamera::MovementDirection::FORWARD, cameraSpeed * dt);
		}
		if (is.IsKeyPressed('A'))
		{
			document.camera.Move(FreeLookCamera::MovementDirection::LEFT, cameraSpeed * dt);
		}
		if (is.IsKeyPressed('S'))
		{
			document.camera.Move(FreeLookCamera::MovementDirection::BACKWARD, cameraSpeed * dt);
		}
		if (is.IsKeyPressed('D'))
		{
			document.camera.Move(FreeLookCamera::MovementDirection::RIGHT, cameraSpeed * dt);
		}

		//process mouse right click
		if (is.IsRightMousePressed())
		{
			float deltaX = (float)(is.GetMouseX()) * rotationSpeed;
			float deltaY = (float)(is.GetMouseY()) * rotationSpeed;
			document.camera.Rotate(deltaX, deltaY);
		}
		is.ZeroMouse();
	}

	//tick world contents
	document.Tick(dt);

	Render();
}

void SceneView::Render()
{
	if (document.levelLoaded)
	{
		if (renderer.StartFrame())
		{
			Matrix viewProjection = document.camera.GetProjMatrix() * document.camera.GetViewMatrix();
			
			//render the world
			worldRenderer.Render(document, viewProjection);

			renderer.EndAndPresentFrame();
		}
	}
	else
	{
		//there is no level loaded so just clear the screen
//		renderer.ClearAndPresentImmediately();
	}
}
