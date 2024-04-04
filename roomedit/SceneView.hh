#pragma once

#include "Document.hh"
#include "sbmemory/FixedArray.inl"
#include "sbgraphics/sbRenderer.hh"
#include "WorldRenderer.hh"
#include "BBox.hh"
#include "InputSystem.hh"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class SceneView
{
	Document &document; //reference to the data, so we can read/modify it from here
	HWND hWnd;
	sbRenderer renderer;
	WorldRenderer worldRenderer;

	void FromScreenPointToWorldRays(const POINTS &screenPoint, Vector &rayStart, Vector &rayDir);
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

public:
	SceneView(Document &document):
		document(document), hWnd(nullptr), worldRenderer(renderer)
	{}

	bool Create(HINSTANCE hInstance, HWND hParentWnd);
	void Destroy();

	bool CreateWorldRenderer();
	void DestroyWorldRenderer();

	void OnMouseMove(WPARAM wParam, LPARAM lParam);
	void OnLButtonDown(WPARAM wParam, LPARAM lParam);
	void OnLButtonUp();

	void MoveWindow(int x, int y, int width, int height);

	void Tick(InputSystem &is, float dt);
	void Render();
};
