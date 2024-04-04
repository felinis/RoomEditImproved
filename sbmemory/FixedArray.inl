#pragma once
#include <stdint.h>
#include <assert.h>

template <typename T, uint32_t MAX_SIZE>
class FixedArray
{
	T data[MAX_SIZE];
	uint32_t numElements;

public:
	constexpr FixedArray(): data(), numElements(0) { }

	//do not allow more constructors
	FixedArray(const FixedArray &other) = delete;
	FixedArray &operator=(const FixedArray &other) = delete;
	FixedArray(FixedArray &&other) = delete;

	//add an element to the array
	void Add(T &element)
	{
//		assert(numElements < MAX_SIZE && "No more space in the array.");

		//if not enough space, flush everything
		if (numElements >= MAX_SIZE)
			Flush();

		data[numElements] = element;
		numElements++;
	}

	//empty the array
	void Flush()
	{
		numElements = 0;
	}

	//mutator
	inline T &operator[](uint32_t i)
	{
		assert(i < numElements);
		return data[i];
	}

	//const accessor
	inline const T &operator[](uint32_t i) const
	{
		assert(i < numElements);
		return data[i];
	}

	//raw mutator and accessor
	T *Data()
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
		T *ptr;

	public:
		iterator(T *ptr) : ptr(ptr)
		{
		}

		T &operator*() const
		{
			return *ptr;
		}

		iterator &operator++()
		{
			++ptr;
			return *this;
		}

		bool operator!=(const iterator &other) const
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
		T *ptr = data + numElements;
		return iterator(ptr);
	}

	class const_iterator
	{
		const T *ptr;

	public:
		const_iterator(const T *ptr) : ptr(ptr)
		{
		}

		const T &operator*() const
		{
			return *ptr;
		}

		const_iterator &operator++()
		{
			++ptr;
			return *this;
		}

		bool operator!=(const const_iterator &other) const
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
		const T *ptr = data + numElements;
		return const_iterator(ptr);
	}
};
