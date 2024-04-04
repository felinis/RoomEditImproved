#pragma once
#include <stdint.h>
#include "Vector3.hh"
#include "sbfilesystem/ReadStream.hh"

constexpr uint32_t MAX_DRAWABLE_NAME_LENGTH = 32;

struct RGBAColor
{
	uint8_t r, g, b, a;

	RGBAColor(uint32_t color = 0) :
		r(color & 0xFF000000),
		g(color & 0x00FF0000),
		b(color & 0x0000FF00),
		a(color & 0x000000FF)
	{}
};

void ConvertHandedness(Vector3 &in);
void ConvertRotation(Vector3 &in);

void LoadDrawableName(ReadStream &rs, char *name);
