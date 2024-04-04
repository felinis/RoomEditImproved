#pragma once
#include <stdint.h>
#include <assert.h>

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
	
	//do not allow copy constructors
	Array(const Array& other) = delete;
	Array& operator=(const Array& other) = delete;

	//move constructor
	inline Array(Array&& other) noexcept: data(other.data), numElements(other.numElements) {}

	//move assignment operator
	inline Array& operator=(Array&& other) noexcept
	{
		data = other.data;
		numElements = other.numElements;
		return *this;
	}

	//mutator
	inline T& operator[](uint32_t i)
	{
		assert(i < numElements);
		return data[i];
	}

	//const accessor
	inline const T& operator[](uint32_t i) const
	{
		assert(i < numElements);
		return data[i];
	}

	//raw mutator and accessor
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
