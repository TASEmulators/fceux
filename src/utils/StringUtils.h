// StringUtils.h
#pragma once

#include <string.h>
#include <stdlib.h>

#include "types.h"

extern "C"
{
	// C Style string copy function that guarantees null termination
	inline char *Strlcpy( char* dest, const char* src, size_t n )
	{
		if (n > 0)
		{
			size_t i=0;
			n--; // minus 1 to make sure we have room for null byte
			while ( (i < n) && (src[i] != 0) )
			{
				dest[i] = src[i]; i++;
			}
			dest[i] = 0; // Force Null
		}
		return dest;
	}
};

#ifdef  __cplusplus

#include <string>

// Custom string type that will allows for fixed size strings on using stack memory with a similar C++ std::string API
namespace FCEU
{
	template <typename T>
	class BaseString
	{
		public:
			BaseString(T* buffer = nullptr, size_t size = 0)
			{
				buf = buffer;
				bufSize = size;
				end = 0;
			}

			BaseString& assign( const T* str )
			{
				if (bufSize > 0)
				{
					size_t i=0;
					const size_t bufEnd = bufSize - 1;
					while ( (i < bufEnd) && (str[i] != 0) )
					{
						buf[i] = str[i]; i++;
					}
					buf[i] = 0;
					end = i;
				}
				return *this;
			}

			BaseString& assign( BaseString& s )
			{
				assign( s.buf );

				return *this;
			}

			BaseString& assign( std::string& s )
			{
				assign( s.c_str() );

				return *this;
			}

			BaseString& append( const T* str )
			{
				if (bufSize > 0)
				{
					size_t j=0;
					size_t i=end;
					const size_t bufEnd = bufSize - 1;
					while ( (i < bufEnd) && (str[j] != 0) )
					{
						buf[i] = str[j]; i++; j++;
					}
					buf[i] = 0;
					end = i;
				}
				return *this;
			}

			BaseString& append( BaseString& s )
			{
				append( s.buf );

				return *this;
			}

			BaseString& append( std::string& s )
			{
				append( s.c_str() );

				return *this;
			}

			BaseString& operator= (const T* op)
			{
				assign(op);

				return *this;
			}

			BaseString& operator= (BaseString& op)
			{
				assign(op);

				return *this;
			}

			T& operator[] (int idx)
			{
				return buf[idx];
			}

			void clear()
			{
				if (bufSize > 0)
				{
					buf[0] = 0;
				}
				end = 0;
			}

			const T* c_str()
			{
				return buf;
			}

			size_t size()
			{
				return end;
			}

			std::string toStdString()
			{
				return std::string(c_str());
			}

			int sprintf( __FCEU_PRINTF_FORMAT const char *format, ...) __FCEU_PRINTF_ATTRIBUTE( 2, 3 )
			{
				int retval;
				va_list args;
				va_start(args, format);
				retval = ::vsnprintf( buf, bufSize, format, args);
				va_end(args);
				return retval;
			}

			int append_sprintf( __FCEU_PRINTF_FORMAT const char *format, ...) __FCEU_PRINTF_ATTRIBUTE( 2, 3 )
			{
				int retval;
				va_list args;
				va_start(args, format);
				size_t sizeAvail = 0;
				if (bufSize > end)
				{
					sizeAvail = bufSize - end;
				}
				retval = ::vsnprintf( &buf[end], sizeAvail, format, args);
				va_end(args);
				return retval;
			}

			long strtol(int base = 10)
			{
				return ::strtol(buf, nullptr, base);
			}

			long long strtoll(int base = 10)
			{
				return ::strtoll(buf, nullptr, base);
			}

			unsigned long strtoul(int base = 10)
			{
				return ::strtoul(buf, nullptr, base);
			}

			unsigned long long strtoull(int base = 10)
			{
				return ::strtoull(buf, nullptr, base);
			}

		private:
			T*      buf;
			size_t  bufSize;
			size_t  end;

	};

	template <size_t N, typename T = char>
	class FixedString : public BaseString<T>
	{
		public:
			FixedString()
				: BaseString<T>( fixedBuffer, N )
			{
			}

			FixedString(const char* initValue)
				: BaseString<T>( fixedBuffer, N )
			{
				if (initValue != nullptr)
				{
					BaseString<T>::assign(initValue);
				}
			}

			BaseString<T>& operator= (const T* op)
			{
				BaseString<T>::assign(op);

				return *this;
			}

			BaseString<T>& operator= (BaseString<T>& op)
			{
				BaseString<T>::assign(op);

				return *this;
			}

			BaseString<T>& operator= (std::string& op)
			{
				BaseString<T>::assign(op);

				return *this;
			}

			BaseString<T>& operator+= (const T* op)
			{
				BaseString<T>::append(op);

				return *this;
			}

			BaseString<T>& operator+= (BaseString<T>& op)
			{
				BaseString<T>::append(op);

				return *this;
			}

			BaseString<T>& operator+= (std::string& op)
			{
				BaseString<T>::append(op);

				return *this;
			}

			T& operator[] (int idx)
			{
				return fixedBuffer[idx];
			}

		private:
			T fixedBuffer[N];
	};


};

#endif
