// ld65dbg.h
//
#pragma once
#include <stdio.h>
#include <string>
#include <map>

namespace ld65
{
	class database;

	class segment
	{
		public:
			static constexpr unsigned char  READ = 0x01;
			static constexpr unsigned char WRITE = 0x02;

			segment( int id, const char *name = nullptr, int startAddr = 0, int size = 0, int ofs = -1, unsigned char type = READ );

			const char *name(void){ return _name.c_str(); };

			int addr(void){ return _startAddr; };

			int ofs(void){ return _ofs; };

		private:
			std::string _name;   // Segment Name
			int   _id;           // Debug ID
			int   _startAddr;    // Start Address CPU
			int   _size;         // Memory region size
			int   _ofs;          // ROM Offset
			unsigned char _type; // ro or rw


		friend class database;
	};

	class scope
	{
		public:
			scope( int id, const char *name = nullptr, int size = 0, int parentID = -1);

			const char *name(void){ return _name.c_str(); };

			scope *getParent(void){ return _parent; };

			void getFullName( std::string &out );

		private:
			std::string _name;   // Scope Name
			int   _id;           // Debug ID
			int   _parentID;     // Parent ID
			int   _size;

			scope *_parent;


		friend class database;
	};

	class sym
	{
		public:
			enum 
			{
				IMPORT = 0,
				LABEL,
				EQU
			};

			sym( int id, const char *name = nullptr, int size = 0, int value = 0, int type = IMPORT);

			int id(void){ return _id; };

			const char *name(void){ return _name.c_str(); };

			int size(void){ return _size; };

			int value(void){ return _value; };

			int type(void){ return _type; };

			scope *getScope(void){ return _scope; };

			segment *getSegment(void){ return _segment; };

		private:
			std::string _name;   // Scope Name
			int   _id;           // Debug ID
			int   _size;
			int   _value;
			int   _type;

			scope   *_scope;
			segment *_segment;

		friend class database;
	};

	class database
	{
		public:
			database(void);
			~database(void);

			int dbgFileLoad( const char *dbgFilePath );

			int iterateSymbols( void *userData, void (*cb)( void *userData, sym *s ) );

		private:
			std::map<int, scope*> scopeMap;
			std::map<int, segment*> segmentMap;
			std::map<int, sym*> symMap;

			class dbgLine
			{
				public:
					dbgLine(size_t bufferSize = 1024);
					~dbgLine(void);

					const char *readFromFile( FILE *fp );

					const char *getLine(void){ return buf; };

					int readToken( char *tk, size_t tkSize );

					int readKeyValuePair( char *keyValueBuffer, size_t keyValueBufferSize );

					static int splitKeyValuePair( char *keyValueBuffer, char **keyPtr, char **valuePtr );

				private:
					void allocBuffer(size_t bufferSize);

					size_t readPtr;
					size_t bufSize;
					char  *buf;
			};
	};
};
