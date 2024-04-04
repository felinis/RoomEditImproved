#pragma once

#include "../Document.hh"
#include "../PackedID.hh"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <CommCtrl.h>

class TreeView
{
	Document &document; //reference to the document
	HWND hWnd;

public:
	constexpr TreeView(Document &document):
		document(document), hWnd(nullptr)
	{}

	bool Create(HINSTANCE hInstance, HWND hParentWnd);
	void MoveWindow(int X, int Y, int nWidth, int nHeight);

	void Empty();
	HTREEITEM AddItem(HTREEITEM hParent, const PackedID &id, const char *text);
//	HTREEITEM FindItem(HTREEITEM hItem, const PackedID &pid);
//	void SelectItem(const PackedID &id);

	//updates the tree view with data from document
	void Update();
};
