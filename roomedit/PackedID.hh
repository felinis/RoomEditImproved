#pragma once
#include <stdint.h>

enum class ObjectType : uint16_t
{
	None,
//	AINetwork,
	Light,
	Room,
	Mesh,
	Trigger,
	Emitter,
	SpotEffect,
	Object,
	Actor
};

//this class defines a small 4-byte data structure (used in the "user-data" field of TreeView, but not only),
//it allows to find the GameWorld object back (for example, from a TreeView element click event)
class PackedID
{
	ObjectType type;
	uint16_t index;

public:
	constexpr PackedID(ObjectType type = ObjectType::None, uint16_t index = 0):
		type(type), index(index)
	{}

	const ObjectType GetType() const
	{
		return type;
	}

	const uint16_t GetIndex() const
	{
		return index;
	}
};
static_assert(sizeof(PackedID) == 4 && "This structure must be of size 4 bytes!");
