#pragma once
#include "FreeLookCamera.hh"
#include "GameWorld.hh"
#include "sbmemory/FixedArray.inl"

//describes the world data
struct Document
{
	FreeLookCamera camera;
	GameWorld world;
	bool levelLoaded; //has the level been loaded?
	bool isDirty; //has something changed internally, that needs to be reflected in the UI?

	bool drawLights;
	bool drawTriggers;
	bool drawRooms; //do we render the rooms?
	bool drawEmitters;
	bool drawSpotEffects;
	bool drawObjects; //do we render the objects?
	bool drawActors;
	bool drawHelpers;

	FixedArray<uint32_t, 16> roomsSelection; //currently selected rooms, by index

	Document():
		levelLoaded(false),
		isDirty(false),
		drawLights(true),
		drawTriggers(true),
		drawRooms(true),
		drawEmitters(false),
		drawSpotEffects(false),
		drawObjects(true),
		drawActors(false),
		drawHelpers(false)
	{}
	
	bool Load(const char *pathToFile);
	void Reset();

	//update the document data
	void Tick(float dt);

	//ROOMS
	bool IsSelected(uint32_t roomIndex) const;
	void MarkAsSelected(uint32_t roomIndex);

	//...

	//discard all of the currently selected entities
	void ResetSelection();
};
