#ifndef __MOVIE_H_
#define __MOVIE_H_

#include <vector>
#include <map>
#include <string>
#include <ostream>

#include "input/zapper.h"

void FCEUMOV_AddInputState();
void FCEUMOV_AddCommand(int cmd);
void FCEU_DrawMovies(uint8 *);

enum EMOVIEMODE
{
	MOVIEMODE_INACTIVE = 1,
	MOVIEMODE_RECORD = 2,
	MOVIEMODE_PLAY = 4,
	MOVIEMODE_TASEDIT = 8
};

EMOVIEMODE FCEUMOV_Mode();
bool FCEUMOV_Mode(EMOVIEMODE modemask);
bool FCEUMOV_Mode(int modemask);

bool FCEUMOV_ShouldPause(void);
int FCEUMOV_GetFrame(void);

//int FCEUMOV_WriteState(FILE* st);
int FCEUMOV_WriteState(std::ostream* os);
bool FCEUMOV_ReadState(FILE* st, uint32 size);
void FCEUMOV_PreLoad();
bool FCEUMOV_PostLoad();

void FCEUMOV_EnterTasEdit();
void FCEUMOV_ExitTasEdit();


class MovieRecord
{
public:
	ValueArray<uint8,4> joysticks;
	
	struct {
		uint8 x,y,b;
	} zappers[2];

	void toggleBit(int joy, int bit)
	{
		joysticks[joy] ^= mask(bit);
	}

	void setBit(int joy, int bit)
	{
		joysticks[joy] |= mask(bit);
	}

	void clearBit(int joy, int bit)
	{
		joysticks[joy] &= ~mask(bit);
	}

	void setBitValue(int joy, int bit, bool val)
	{
		if(val) setBit(joy,bit);
		else clearBit(joy,bit);
	}

	bool checkBit(int joy, int bit)
	{
		return (joysticks[joy] & mask(bit))!=0;
	}

	void clear() { 
		*(uint32*)&joysticks = 0;
	}
	
	//a waste of memory in lots of cases..  maybe make it a pointer later?
	std::vector<char> savestate;

	void dump(std::ostream* os, int index);

	static const char mnemonics[8];

private:
	int mask(int bit) { return 1<<bit; }
};

class MovieData
{
public:
	MovieData();
	

	int version;
	int emuVersion;
	//todo - somehow force mutual exclusion for poweron and reset (with an error in the parser)
	bool palFlag;
	bool poweronFlag;
	bool resetFlag;
	MD5DATA romChecksum;
	std::string romFilename;
	std::vector<char> savestate;
	std::vector<MovieRecord> records;
	//this is the RERECORD COUNT. please rename variable.
	int recordCount;
	FCEU_Guid guid;
	
	//----TasEdit stuff---
	int greenZoneCount;
	//----

	int getNumRecords() { return records.size(); }

	class TDictionary : public std::map<std::string,std::string>
	{
	public:
		bool containsKey(std::string key)
		{
			return find(key) != end();
		}

		void tryInstallBool(std::string key, bool& val)
		{
			if(containsKey(key))
				val = atoi(operator [](key).c_str())!=0;
		}

		void tryInstallString(std::string key, std::string& val)
		{
			if(containsKey(key))
				val = operator [](key);
		}

		void tryInstallInt(std::string key, int& val)
		{
			if(containsKey(key))
				val = atoi(operator [](key).c_str());
		}

	};

	void truncateAt(int frame);
	void installDictionary(TDictionary& dictionary);
	void dump(std::ostream* os);
	int dumpLen();
	void clearRecordRange(int start, int len);
	
	static bool loadSavestateFrom(std::vector<char>* buf);
	static void dumpSavestateTo(std::vector<char>* buf, int compressionLevel);
	void TryDumpIncremental();
};

extern MovieData currMovieData;
extern int currFrameCounter;
//---------

#endif //__MOVIE_H_
