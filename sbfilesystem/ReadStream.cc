/*
*	File System Module for Sabre Engine
*	Allows for reading a file using memory mapping, which is faster than reading the file using multiple fread calls.
*	(C) Moczulski Alan, 2023.
*/

#include "ReadStream.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string.h>

int ReadStream::Open(const char* filePath)
{
	handle = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (handle == INVALID_HANDLE_VALUE)
		return 0;

	size = GetFileSize(handle, 0);

	HANDLE hMap = CreateFileMapping(handle, 0, PAGE_READONLY, 0, 0, 0);
	if (!hMap)
		return 0;

	originalPointer = (char*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
	if (!originalPointer)
		return 0;

	currentPointer = originalPointer;

	CloseHandle(hMap);
	return 1;
}

void ReadStream::Close()
{
	UnmapViewOfFile(originalPointer);
	CloseHandle(handle);
}

void ReadStream::Read(void* out, uint32_t size)
{
	memcpy(out, currentPointer, size);
	AdvanceBy(size);
}

void ReadStream::GetPointer(void** out, uint32_t size)
{
	*out = currentPointer;
	AdvanceBy(size);
}

void* ReadStream::GetDataAt(uint32_t offset) const
{
	return (void*)(originalPointer + offset);
}

void ReadStream::AdvanceBy(uint32_t size)
{
	currentPointer += size;
}

void ReadStream::AdvanceTo(uint32_t size)
{
	currentPointer = originalPointer + size;
}

float ReadStream::GetAdvancement()
{
	return (float)((size_t)currentPointer - (size_t)originalPointer) / (float)size;
}
