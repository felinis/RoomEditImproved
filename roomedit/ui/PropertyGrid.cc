/*
*	Room Editor Application
*	(C) Moczulski Alan, 2023.
*/

#include "PropertyGrid.hh"
#include <CommCtrl.h>
#include "colors.h"

bool PropertyGrid::Create(HINSTANCE hInstance, HWND hParentWnd)
{
	hWnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		WC_LISTVIEW,
		"Property Grid",
		WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_ALIGNTOP,// | LVS_SMALLICON,
		0, 0, 0, 0, //size and position will be set with MoveWindow()
		hParentWnd,
		nullptr,
		hInstance,
		nullptr
	);

	if (!hWnd)
		return false;

	ListView_SetBkColor(hWnd, Colors::DefaultBackground);

	LVCOLUMN lvc;
	lvc.mask = LVCF_TEXT | LVCF_WIDTH;
	lvc.cx = 100;

	lvc.pszText = (char*)"Property";
	ListView_InsertColumn(hWnd, 0, &lvc);

	lvc.pszText = (char *)"Value";
	ListView_InsertColumn(hWnd, 1, &lvc);

	return true;
}

void PropertyGrid::AddProperty(const char* propertyName, const char* propertyValue)
{
	LVITEM lvi;
	lvi.mask = LVIF_TEXT;
	lvi.iItem = ListView_GetItemCount(hWnd);
	lvi.iSubItem = 0;
	lvi.pszText = (char*)propertyName;
	ListView_InsertItem(hWnd, &lvi);

	lvi.iSubItem = 1;
	lvi.pszText = (char*)propertyValue;
	ListView_SetItem(hWnd, &lvi);
}

void PropertyGrid::Empty()
{
	ListView_DeleteAllItems(hWnd);
}

void PropertyGrid::MoveWindow(int X, int Y, int nWidth, int nHeight)
{
	::MoveWindow(hWnd, X, Y, nWidth, nHeight, TRUE);
}
