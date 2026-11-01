#ifndef _VALUEARRAY_H_
#define _VALUEARRAY_H_

#include <stddef.h>

template<typename T, size_t N>
struct ValueArray
{
	T data[N];
	T &operator[](int index) { return data[index]; }
	static constexpr size_t size = N;
	bool operator!=(ValueArray<T,N> &other) { return !operator==(other); }
	bool operator==(ValueArray<T,N> &other)
	{
		for(size_t i=0;i<size;i++)
			if(data[i] != other[i])
				return false;
		return true;
	}
};

#endif

