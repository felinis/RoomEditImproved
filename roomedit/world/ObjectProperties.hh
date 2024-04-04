#pragma once
#include "sbfilesystem/ReadStream.hh"
#include "sbmemory/MemoryPool.hh"

struct OBJECT_PROPERTIES
{
	class OBJECT_PROPERTY
	{
		uint32_t type;
		uint32_t number;
		char string[256];

	public:
		void Load(ReadStream &rs, MemoryPool &pool)
		{
			rs >> type;
			rs >> number;

			uint32_t length;
			rs >> length;
			assert(length < 256); //the ShoppingMall level has one of length 160
			rs.Read(string, length);
		}

		inline const char *GetString() const
		{
			return string;
		}
	};
	Array<OBJECT_PROPERTY> properties;

	//func at 4C8F20, EdenDemo
	void Load(ReadStream &rs, MemoryPool &pool)
	{
		uint32_t numProperties;
		rs >> numProperties;
		properties = pool.CreateArray<OBJECT_PROPERTY>(numProperties);
		for (auto &property : properties)
			property.Load(rs, pool);
	}
};
