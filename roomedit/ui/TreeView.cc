/*
*	Room Editor Application
*	(C) Moczulski Alan, 2023.
*/

#include "TreeView.hh"
#include "colors.h"
#include <stdio.h> //snprintf

bool TreeView::Create(HINSTANCE hInstance, HWND hParentWnd)
{
	hWnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		WC_TREEVIEW,
		"Tree View",
		WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS,// | TVS_CHECKBOXES,
		0, 0, 0, 0,
		hParentWnd,
		nullptr,
		hInstance,
		0
	);
	if (!hWnd)
		return false;
	
	TreeView_SetBkColor(hWnd, Colors::DefaultBackground);
//	TreeView_SetTextColor(hWnd, Colors::DefaultText);

	return true;
}

void TreeView::MoveWindow(int X, int Y, int nWidth, int nHeight)
{
	::MoveWindow(hWnd, X, Y, nWidth, nHeight, TRUE);
}

void TreeView::Empty()
{
	TreeView_DeleteAllItems(hWnd);
}

HTREEITEM TreeView::AddItem(HTREEITEM hParent, const PackedID &id, const char *text)
{
	TVINSERTSTRUCT tvis;
	tvis.hParent = hParent;
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.mask = TVIF_TEXT | TVIF_PARAM; //we will store a pointer to the object in LPARAM
	tvis.item.pszText = (char*)text;
	memcpy(&tvis.item.lParam, &id, sizeof(id)); //store id as user data

	return TreeView_InsertItem(hWnd, &tvis);
}

#if 0
//MARKED AS OBSOLETE
HTREEITEM TreeView::FindItem(HTREEITEM hItem, const PackedID &id)
{
	TVITEM tvi;
	tvi.mask = TVIF_PARAM;
	tvi.hItem = hItem;
	if (TreeView_GetItem(hWnd, &tvi))
	{
		if (memcmp(&tvi.lParam, &id, sizeof(id)) == 0)
		{
			return hItem; //found it
		}
	}

	HTREEITEM hChildItem = TreeView_GetChild(hWnd, hItem);
	while (hChildItem)
	{
		HTREEITEM hFound = FindItem(hChildItem, id);
		if (hFound != nullptr)
			return hFound;

		hChildItem = TreeView_GetNextSibling(hWnd, hChildItem);
	}

	return nullptr;
}

//MARKED AS OBSOLETE
void TreeView::SelectItem(const PackedID &id)
{
	HTREEITEM hRoot = TreeView_GetRoot(hWnd);
	HTREEITEM hFound = FindItem(hRoot, id);
	if (hFound)
		TreeView_SelectItem(hWnd, hFound);
	//technically it is not possible not to find the HTREEITEM...
}
#endif

//only elements of subtype Object can be updated!
void TreeView::Update()
{
	char buf[32];

	Empty();

	const GameWorld &world = document.world;

	//list lights
	{
		snprintf(buf, sizeof(buf), "%u Light(s)", world.lights.Count());
		HTREEITEM hLights = AddItem(TVI_ROOT, PackedID(), buf);
		for (uint32_t i = 0; i < world.lights.Count(); i++)
		{
			const Light &light = world.lights[i];

			snprintf(buf, sizeof(buf), "Light %u", i);
			HTREEITEM hRoom = AddItem(hLights, PackedID(ObjectType::Light, i), buf);
		}
	}

	//list rooms
	{
		snprintf(buf, sizeof(buf), "%u Room(s)", world.rooms.Count());
		HTREEITEM hRooms = AddItem(TVI_ROOT, PackedID(), buf);
		for (uint32_t i = 0; i < world.rooms.Count(); i++)
		{
			const Room &room = world.rooms[i];

			const char *roomName = room.mesh.name;
			HTREEITEM hRoom = AddItem(hRooms, PackedID(ObjectType::Room, i), roomName);

			//if that room is currently selected, highlight it
			if (document.IsSelected(i))
				TreeView_SelectItem(hWnd, hRoom);

//			if (room.mesh.name[0]) //TODO: check if this works (some obsolete rooms may not have a mesh!)
//				AddItem(hRoom, &room.mesh, "Mesh");

			uint32_t numTriggers = room.triggers.Count();
			if (numTriggers)
			{
				snprintf(buf, sizeof(buf), "%u Trigger(s)", numTriggers);
				HTREEITEM hTriggers = AddItem(hRoom, PackedID(), buf);
				for (uint32_t j = 0; j < numTriggers; j++)
				{
					snprintf(buf, sizeof(buf), "Trigger %u", j);
					AddItem(hTriggers, PackedID(ObjectType::Trigger, j), buf);
				}
			}
		}
	}

	//list meshes
	{
		snprintf(buf, sizeof(buf), "%u Mesh(es)", world.meshes.Count());
		HTREEITEM hMeshes = AddItem(TVI_ROOT, PackedID(), buf);
		for (uint32_t i = 0; i < world.meshes.Count(); i++)
		{
			const Mesh &mesh = world.meshes[i];

			HTREEITEM hMesh = AddItem(hMeshes, PackedID(ObjectType::Mesh, i), mesh.name);
		}
	}

	//list emitters
	{
		snprintf(buf, sizeof(buf), "%u Emitter(s)", world.emitters.Count());
		HTREEITEM hEmitters = AddItem(TVI_ROOT, PackedID(), buf);
		for (uint32_t i = 0; i < world.emitters.Count(); i++)
		{
			const Emitter &emitter = world.emitters[i];

			HTREEITEM hEmitter = AddItem(hEmitters, PackedID(ObjectType::Emitter, i), emitter.name);
		}
	}

	//list spot effects
	{
		snprintf(buf, sizeof(buf), "%u Spot Effect(s)", world.spotEffects.Count());
		HTREEITEM hSpotEffects = AddItem(TVI_ROOT, PackedID(), buf);
		for (uint32_t i = 0; i < world.spotEffects.Count(); i++)
		{
			const SpotEffect &spotEffect = world.spotEffects[i];

			HTREEITEM hSpotEffect = AddItem(hSpotEffects, PackedID(ObjectType::SpotEffect, i), spotEffect.name);
		}
	}

	//list objects
	{
		snprintf(buf, sizeof(buf), "%u Object(s)", world.objects.Count());
		HTREEITEM hObjects = AddItem(TVI_ROOT, PackedID(), buf);
		for (uint32_t i = 0; i < world.objects.Count(); i++)
		{
			const Object &object = world.objects[i];

			snprintf(buf, sizeof(buf), "Object %u", i);
			HTREEITEM hObject = AddItem(hObjects, PackedID(ObjectType::Object, i), buf);
		}
	}
#if 0
	//list actors
	{
		snprintf(buf, sizeof(buf), "%u Actor(s)", world.actors.Count());
		HTREEITEM hActors = AddItem(TVI_ROOT, PackedID(), buf);
		for (uint32_t i = 0; i < world.actors.Count(); i++)
		{
			const Actor &actor = world.actors[i];

			snprintf(buf, sizeof(buf), "Actor %u", i);
			HTREEITEM hActor = AddItem(hActors, PackedID(ObjectType::Actor, i), buf);
		}
	}
#endif
}
