#ifndef __MOVIE_H_
#define __MOVIE_H_

#include <vector>
#include <map>
#include <string>

void FCEUMOV_AddJoy(uint8 *, int SkipFlush);
void FCEUMOV_AddCommand(int cmd);
void FCEU_DrawMovies(uint8 *);
int FCEUMOV_IsPlaying(void);
int FCEUMOV_IsRecording(void);
bool FCEUMOV_ShouldPause(void);
int FCEUMOV_GetFrame(void);

int FCEUMOV_WriteState(FILE* st);
bool FCEUMOV_ReadState(FILE* st, uint32 size);
void FCEUMOV_PreLoad(void);
int FCEUMOV_PostLoad(void);


class MovieRecord
{
public:
	ValueArray<uint8,4> joysticks;

	void toggleBit(int joy, int bit)
	{
		int mask = (1<<bit);
		joysticks[joy] ^= mask;
	}

	void clear() { 
		*(uint32*)&joysticks = 0;
	}
	
	//a waste of memory in lots of cases..  maybe make it a pointer later?
	std::vector<uint8> savestate;

	void dump(FILE* fp, int index);

	static const char mnemonics[8];
};

class MovieData
{
public:
	MovieData();
	

	int emuVersion;
	int version;
	//todo - somehow force mutual exclusion for poweron and reset (with an error in the parser)
	bool palFlag;
	bool poweronFlag;
	bool resetFlag;
	MD5DATA romChecksum;
	std::string romFilename;
	std::vector<uint8> savestate;
	std::vector<MovieRecord> records;
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
	void dump(FILE *fp);
	int dumpLen();
	void clearRecordRange(int start, int len);
	
	static bool loadSavestateFrom(std::vector<uint8>* buf);
	static void dumpSavestateTo(std::vector<uint8>* buf);
	void TryDumpIncremental();
};

extern MovieData currMovieData;
extern int currFrameCounter;
//---------

#endif //__MOVIE_H_
