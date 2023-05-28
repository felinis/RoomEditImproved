#pragma once
#include <stdint.h>

/*
*	A simple array whose size is known.
*/
template <typename T>
class Array
{
	T* data;			//pointer to the start of the data
	uint32_t numElements;

public:
	constexpr Array() : data(nullptr), numElements(0)
	{
	}
	constexpr Array(void* data, uint32_t numElements) : data((T*)data), numElements(numElements)
	{
	}

	//mutator
	T& operator[](unsigned int i)
	{
		return data[i];
	}

	//const accessor
	const T& operator[](unsigned int i) const
	{
		return data[i];
	}

	//raw mutator/accessor
	T* Data()
	{
		return data;
	}

	const uint32_t Count() const
	{
		return numElements;
	}

	//handy for-loop iterator
	class iterator
	{
		T* ptr;

	public:
		iterator(T* ptr) : ptr(ptr) {}

		T& operator*() const
		{
			return *ptr;
		}

		iterator& operator++()
		{
			++ptr;
			return *this;
		}

		bool operator!=(const iterator& other) const
		{
			return ptr != other.ptr;
		}
	};

	iterator begin()
	{
		return iterator(data);
	}

	iterator end()
	{
		T* ptr = data + numElements;
		return iterator(ptr);
	}

	class const_iterator
	{
		const T* ptr;

	public:
		const_iterator(const T* ptr) : ptr(ptr)
		{
		}

		const T& operator*() const
		{
			return *ptr;
		}

		const_iterator& operator++()
		{
			++ptr;
			return *this;
		}

		bool operator!=(const const_iterator& other) const
		{
			return ptr != other.ptr;
		}
	};

	const_iterator begin() const
	{
		return const_iterator(data);
	}

	const_iterator end() const
	{
		T* ptr = data + numElements;
		return const_iterator(ptr);
	}
};
