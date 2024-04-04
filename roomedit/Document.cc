/*
*	Room Editor Application
*	(C) Moczulski Alan, 2023.
*/

#include "Document.hh"

bool Document::Load(const char *pathToFile)
{
	if (levelLoaded)
		Reset();

	RESULT result = world.Load(pathToFile);
	if (!result.IsOK())
		return false;

	levelLoaded = true;

	return true;
}

void Document::Reset()
{
	if (!levelLoaded)
		return;

	levelLoaded = false;

	world.Release();
}

void Document::Tick(float dt)
{
}

bool Document::IsSelected(uint32_t roomIndex) const
{
	for (const auto &index : roomsSelection)
	{
		if (index == roomIndex)
			return true;
	}
	return false;
}

void Document::MarkAsSelected(uint32_t roomIndex)
{
	roomsSelection.Add(roomIndex);
}

void Document::ResetSelection()
{
	roomsSelection.Flush();
}
