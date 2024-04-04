#pragma once
#include <stdint.h>

class ReadStream
{
public:
	ReadStream() :
		handle(nullptr),
		size(0),
		originalPointer(nullptr),
		currentPointer(nullptr)
	{}

	int Open(const char* filePath);
	void Close();

	void Read(void* out, uint32_t size);
	void GetPointer(void** out, uint32_t size);
	void* GetDataAt(uint32_t offset) const;
	void AdvanceBy(uint32_t size);
	void AdvanceTo(uint32_t size);
	float GetAdvancement();

	template <typename T>
	void operator>>(T& val)
	{
		Read(&val, sizeof(T));
	}

private:
	void* handle;
	uint32_t size;
	char* originalPointer;
	char* currentPointer;
};
