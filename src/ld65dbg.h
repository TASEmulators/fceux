// ld65dbg.h
//
#pragma once
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

			scope *getParent(void){ return _parent; };
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
			sym( int id, const char *name = nullptr, int size = 0);

		private:
			std::string _name;   // Scope Name
			int   _id;           // Debug ID
			int   _size;

		friend class database;
	};

	class database
	{
		public:
			database(void);

		private:
			std::map<int, scope*> scopeMap;
			std::map<int, segment*> segmentMap;
	};
};
