#pragma once

#include <algorithm>
#include <cstring>
#include <type_traits>

// Faster replacement functions for sprintf. It's uglier, but much faster.
class StringBuilder
{
public:
	// Helper struct. Do not use directly.
	template <unsigned Radix, typename T, typename Prefix = nullptr_t>
	struct IntInfo
	{
		T x;
		int minLen;
		char leadChar;
		bool upperCase;
		Prefix prefix;
	};

	inline StringBuilder(char *str) : start(str), end(str)
	{
		*end = '\0';
	}

	inline char *str() const
	{
		return start;
	}

	inline size_t size() const
	{
		return size_t(end - start);
	}

	inline StringBuilder &operator <<(char ch)
	{
		*(end++) = ch;
		*end = '\0';

		return *this;
	}

	inline StringBuilder &operator <<(const char *src)
	{
		size_t len = std::strlen(src);
		std::memcpy(end, src, len + 1);

		end += len;

		return *this;
	}

	template<unsigned Radix, typename T, typename Prefix>
	inline StringBuilder &operator <<(const IntInfo<Radix, T, Prefix> intInfo)
	{
		*this << intInfo.prefix;
		return appendInt<Radix>(intInfo.x, intInfo.minLen, intInfo.leadChar, intInfo.upperCase);
	}

protected:
	inline StringBuilder &operator <<(nullptr_t)
	{
		return *this;
	}

	template<unsigned Radix, typename T>
	StringBuilder &appendInt(T x, int minLen = 0, char leadChar = ' ', bool upperCase = false)
	{
		static_assert(Radix >= 2 && Radix <= 36, "Radix must be between 2 and 36");
		static_assert(std::is_integral<T>::value, "T must be an integral type");

		static const char *upperCaseDigits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ",
			*lowerCaseDigits = "0123456789abcdefghijklmnopqrstuvwxyz";
		const char *digits = upperCase ? upperCaseDigits : lowerCaseDigits;

		const bool isSigned = std::is_signed<T>::value;
		bool isNeg = isSigned && x < 0;
		if (isSigned && isNeg)
			// This convoluted expression is to silence an unnecessary compiler warning when unsigned
			x = T(-((typename std::make_signed<T>::type)(x)));

		unsigned i = 0;
		if (x != 0)
		{
			do
			{
				end[i++] = digits[x % Radix];
				x /= Radix;
			} while (x != 0);
		}
		else
			end[i++] = '0';

		if (isNeg)
			end[i++] = '-';

		if (minLen < 0)
		{
			end[i] = '\0';

			std::reverse(end, end + i);

			for (; i < (unsigned)-minLen; i++)
				end[i] = leadChar;

			end[i] = '\0';
		}
		else
		{
			for (; i < (unsigned)minLen; i++)
				end[i] = leadChar;

			end[i] = '\0';

			std::reverse(end, end + i);
		}

		end += i;

		return *this;
	}

	char *start;
	char *end;
};

// Formatters for numbers

// Generic integer with any radius
template<unsigned Radix, typename T>
inline StringBuilder::IntInfo<Radix, T> sb_int(T x, int minLen = 0, char leadChar = ' ', bool upperCase = false)
{
	return { x, minLen, leadChar, upperCase, nullptr };
}

// A decimal number
template <typename T>
inline StringBuilder::IntInfo<10, T> sb_dec(T x, int minLen = 0, char leadChar = ' ')
{
	return { x, minLen, leadChar, false, nullptr };
}

// A hex number
template <typename T>
inline StringBuilder::IntInfo<16, T> sb_hex(T x, int minLen = 0, bool upperCase = true, char leadChar = '0')
{
	return { x, minLen, leadChar, upperCase, nullptr };
}

// An address of the basic form $%x
template <typename T>
inline StringBuilder::IntInfo<16, T, char> sb_addr(T x, int minLen = 4, bool upperCase = true)
{
	return { x, minLen, '0', upperCase, '$' };
}

// A literal value of the form #$%x
template <typename T>
inline StringBuilder::IntInfo<16, T, const char *> sb_lit(T x, int minLen = 2, bool upperCase = true)
{
	return { x, minLen, '0', upperCase, "#$" };
}
