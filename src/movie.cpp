#include "emufile.h"
#include "version.h"
#include "types.h"
#include "utils/endian.h"
#include "palette.h"
#include "input.h"
#include "fceu.h"
#include "netplay.h"
#include "driver.h"
#include "state.h"
#include "file.h"
#include "video.h"
#include "movie.h"
#include "cart.h"
#include "fds.h"
#include "vsuni.h"
#ifdef _S9XLUA_H
#include "fceulua.h"
#endif
#include "utils/guid.h"
#include "utils/memory.h"
#include "utils/xstring.h"
#include <sstream>
#include <algorithm>

#ifdef CREATE_AVI
#include "drivers/videolog/nesvideos-piece.h"
#endif

#ifdef WIN32
#include <windows.h>

#ifdef __WIN_DRIVER__
#include "./drivers/win/common.h"
#include "./drivers/win/window.h"
extern void AddRecentMovieFile(const char *filename);
#include "./drivers/win/taseditor.h"
extern bool mustEngageTaseditor;
#endif

#endif

#ifdef __QT_DRIVER__
#include "./drivers/Qt/TasEditor/TasEditorWindow.h"
#endif

extern int RAMInitOption;
extern int RAMInitSeed;

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <iomanip>
#include <fstream>
#include <climits>
#include <cstdarg>
#include <zlib.h>

using namespace std;

#define MOVIE_VERSION           3

extern char FileBase[];
extern bool AutoSS;		//Declared in fceu.cpp, keeps track if a auto-savestate has been made

std::vector<int> subtitleFrames;		//Frame numbers for subtitle messages
std::vector<string> subtitleMessages;	//Messages of subtitles

bool subtitlesOnAVI = false;
bool autoMovieBackup = false; //Toggle that determines if movies should be backed up automatically before altering them
bool freshMovie = false;	  //True when a movie loads, false when movie is altered.  Used to determine if a movie has been altered since opening
bool movieFromPoweron = true;

static int _currCommand = 0;

// Function declarations------------------------


//TODO - remove the synchack stuff from the replay gui and require it to be put into the fm2 file
//which the user would have already converted from fcm
//also cleanup the whole emulator version bullshit in replay. we dont support that old stuff anymore

//todo - better error handling for the compressed savestate

//todo - consider a MemoryBackedFile class..
//..a lot of work.. instead lets just read back from the current fcm

//todo - could we, given a field size, over-read from an inputstream and then parse out an integer?
//that would be faster than several reads, perhaps.

//sometimes we accidentally produce movie stop signals while we're trying to do other things with movies..
bool suppressMovieStop=false;

//----movie engine main state
EMOVIEMODE movieMode = MOVIEMODE_INACTIVE;

//this should not be set unless we are in MOVIEMODE_RECORD!
//FILE* fpRecordingMovie = 0;
EMUFILE* osRecordingMovie = NULL;

int currFrameCounter;
uint32 cur_input_display = 0;
int pauseframe = -1;
bool movie_readonly = true;
int input_display = 0;
int frame_display = 0;
int rerecord_display = 0;
bool fullSaveStateLoads = false;	//Option for loading a savestates full contents in read+write mode instead of up to the frame count in the savestate (useful as a recovery option)
int movieRecordMode = 0;			//Option for various movie recording modes such as TRUNCATE (normal), OVERWRITE etc.

SFORMAT FCEUMOV_STATEINFO[]={
	{ &currFrameCounter, 4|FCEUSTATE_RLSB, "FCNT"},
	{ 0 }
};

std::string curMovieFilename;
MovieData currMovieData;
MovieData defaultMovieData;
int currRerecordCount; // Keep the global value

char lagcounterbuf[32] = {0};

void MovieData::clearRecordRange(int start, int len)
{
	for(int i=0;i<len;i++)
	{
		records[i+start].clear();
	}
}

void MovieData::eraseRecords(int at, int frames)
{
	if (at < (int)records.size())
	{
		if (frames == 1)
		{
			// erase 1 frame
			records.erase(records.begin() + at);
		} else
		{
			// erase many frames
			if (at + frames > (int)records.size())
				frames = (int)records.size() - at;
			records.erase(records.begin() + at, records.begin() + (at + frames));
		}
	}
}

void MovieData::insertEmpty(int at, int frames)
{
	if (at == -1)
	{
		records.resize(records.size() + frames);
	} else
	{
		records.insert(records.begin() + at, frames, MovieRecord());
	}
}

void MovieData::cloneRegion(int at, int frames)
{
	if (at < 0) return;

	records.insert(records.begin() + at, frames, MovieRecord());

	for(int i = 0; i < frames; i++)
		records[i + at].Clone(records[i + at + frames]);
}
// ----------------------------------------------------------------------------
MovieRecord::MovieRecord()
{
	commands = 0;
	*(uint32*)&joysticks = 0;
	memset(zappers, 0, sizeof(zappers));
}

void MovieRecord::clear()
{
	commands = 0;
	*(uint32*)&joysticks = 0;
	memset(zappers, 0, sizeof(zappers));
}

bool MovieRecord::Compare(MovieRecord& compareRec)
{
	//Joysticks, Zappers, and commands

	if (this->commands != compareRec.commands)
		return false;
	if ((*(uint32*)&(this->joysticks)) != (*(uint32*)&(compareRec.joysticks)))
		return false;
	if (memcmp(this->zappers, compareRec.zappers, sizeof(zappers)))
		return false;

	/*
	if (this->joysticks != compareRec.joysticks)
		return false;

	//if new commands are ever recordable, they need to be added here if we go with this method
	if(this->command_reset() != compareRec.command_reset()) return false;
	if(this->command_power() != compareRec.command_power()) return false;
	if(this->command_fds_insert() != compareRec.command_fds_insert()) return false;
	if(this->command_fds_select() != compareRec.command_fds_select()) return false;

	if (this->zappers[0].x != compareRec.zappers[0].x) return false;
	if (this->zappers[0].y != compareRec.zappers[0].y) return false;
	if (this->zappers[0].zaphit != compareRec.zappers[0].zaphit) return false;
	if (this->zappers[0].b != compareRec.zappers[0].b) return false;
	if (this->zappers[0].bogo != compareRec.zappers[0].bogo) return false;

	if (this->zappers[1].x != compareRec.zappers[1].x) return false;
	if (this->zappers[1].y != compareRec.zappers[1].y) return false;
	if (this->zappers[1].zaphit != compareRec.zappers[1].zaphit) return false;
	if (this->zappers[1].b != compareRec.zappers[1].b) return false;
	if (this->zappers[1].bogo != compareRec.zappers[1].bogo) return false;
	*/

	return true;
}
void MovieRecord::Clone(MovieRecord& sourceRec)
{
	*(uint32*)&joysticks = *(uint32*)(&(sourceRec.joysticks));
	memcpy(this->zappers, sourceRec.zappers, sizeof(zappers));
	this->commands = sourceRec.commands;
}

const char MovieRecord::mnemonics[8] = {'A','B','S','T','U','D','L','R'};

void MovieRecord::dumpJoy(EMUFILE* os, uint8 joystate)
{
	//these are mnemonics for each joystick bit.
	//since we usually use the regular joypad, these will be more helpful.
	//but any character other than ' ' or '.' should count as a set bit
	//maybe other input types will need to be encoded another way..
	for(int bit=7;bit>=0;bit--)
	{
		int bitmask = (1<<bit);
		char mnemonic = mnemonics[bit];
		//if the bit is set write the mnemonic
		if(joystate & bitmask)
			os->fwrite(&mnemonic,1);
		else //otherwise write an unset bit
			write8le('.',os);
	}
}

void MovieRecord::parseJoy(EMUFILE* is, uint8& joystate)
{
	char buf[8];
	is->fread(buf,8);
	joystate = 0;
	for(int i=0;i<8;i++)
	{
		joystate <<= 1;
		joystate |= ((buf[i]=='.'||buf[i]==' ')?0:1);
	}
}

void MovieRecord::parse(MovieData* md, EMUFILE* is)
{
	//by the time we get in here, the initial pipe has already been extracted

	//extract the commands
	commands = uint32DecFromIstream(is);
	//*is >> commands;
	is->fgetc(); //eat the pipe

	//a special case: if fourscore is enabled, parse four gamepads
	if(md->fourscore)
	{
		parseJoy(is,joysticks[0]); is->fgetc(); //eat the pipe
		parseJoy(is,joysticks[1]); is->fgetc(); //eat the pipe
		parseJoy(is,joysticks[2]); is->fgetc(); //eat the pipe
		parseJoy(is,joysticks[3]); is->fgetc(); //eat the pipe
	}
	else
	{
		for(int port=0;port<2;port++)
		{
			if(md->ports[port] == SI_GAMEPAD)
				parseJoy(is, joysticks[port]);
			else if(md->ports[port] == SI_ZAPPER)
			{
				zappers[port].x = uint32DecFromIstream(is);
				zappers[port].y = uint32DecFromIstream(is);
				zappers[port].b = uint32DecFromIstream(is);
				zappers[port].bogo = uint32DecFromIstream(is);
				zappers[port].zaphit = uint64DecFromIstream(is);
			}

			is->fgetc(); //eat the pipe
		}
	}

	//(no fcexp data is logged right now)
	is->fgetc(); //eat the pipe

	//should be left at a newline
}


bool MovieRecord::parseBinary(MovieData* md, EMUFILE* is)
{
	commands = (uint8)is->fgetc();

	//check for eof
	if(is->eof()) return false;

	if(md->fourscore)
	{
		is->fread((char*)&joysticks,4);
	}
	else
	{
		for(int port=0;port<2;port++)
		{
			if(md->ports[port] == SI_GAMEPAD)
				joysticks[port] = (uint8)is->fgetc();
			else if(md->ports[port] == SI_ZAPPER)
			{
				zappers[port].x = (uint8)is->fgetc();
				zappers[port].y = (uint8)is->fgetc();
				zappers[port].b = (uint8)is->fgetc();
				zappers[port].bogo = (uint8)is->fgetc();
				read64le(&zappers[port].zaphit,is);
			}
		}
	}

	return true;
}


void MovieRecord::dumpBinary(MovieData* md, EMUFILE* os, int index)
{
	write8le(commands,os);
	if(md->fourscore)
	{
		for(int i=0;i<4;i++)
			os->fwrite(&joysticks[i],sizeof(joysticks[i]));
	}
	else
	{
		for(int port=0;port<2;port++)
		{
			if(md->ports[port] == SI_GAMEPAD)
				os->fwrite(&joysticks[port],sizeof(joysticks[port]));
			else if(md->ports[port] == SI_ZAPPER)
			{
				write8le(zappers[port].x,os);
				write8le(zappers[port].y,os);
				write8le(zappers[port].b,os);
				write8le(zappers[port].bogo,os);
				write64le(zappers[port].zaphit, os);
			}
		}
	}
}

void MovieRecord::dump(MovieData* md, EMUFILE* os, int index)
{
	// dump the commands
	//*os << '|' << setw(1) << (int)commands;
	os->fputc('|');
	putdec<uint8,3,false>(os, commands);	// "variable length decimal integer"

	//a special case: if fourscore is enabled, dump four gamepads
	if(md->fourscore)
	{
		os->fputc('|');
		dumpJoy(os,joysticks[0]); os->fputc('|');
		dumpJoy(os,joysticks[1]); os->fputc('|');
		dumpJoy(os,joysticks[2]); os->fputc('|');
		dumpJoy(os,joysticks[3]); os->fputc('|');
	}
	else
	{
		for(int port=0;port<2;port++)
		{
			os->fputc('|');
			if(md->ports[port] == SI_GAMEPAD)
				dumpJoy(os, joysticks[port]);
			else if(md->ports[port] == SI_ZAPPER)
			{
				putdec<uint8,3,true>(os,zappers[port].x); os->fputc(' ');
				putdec<uint8,3,true>(os,zappers[port].y); os->fputc(' ');
				putdec<uint8,1,true>(os,zappers[port].b); os->fputc(' ');
				putdec<uint8,1,true>(os,zappers[port].bogo); os->fputc(' ');
				putdec<uint64,20,false>(os,zappers[port].zaphit);
			}
		}
		os->fputc('|');
	}

	//(no fcexp data is logged right now)
	os->fputc('|');

	//each frame is on a new line
	os->fputc('\n');
}

MovieData::MovieData()
	: version(MOVIE_VERSION)
	, emuVersion(FCEU_VERSION_NUMERIC)
	, fds(false)
	, palFlag(false)
	, PPUflag(false)
	, rerecordCount(0)
	, binaryFlag(false)
	, loadFrameCount(-1)
	, fourscore(false)
	, microphone(false)
	, RAMInitOption(0)
	, RAMInitSeed(0)
{
	memset(&romChecksum,0,sizeof(MD5DATA));
}

void MovieData::truncateAt(int frame)
{
	records.resize(frame);
}

void MovieData::installValue(std::string& key, std::string& val)
{
	//todo - use another config system, or drive this from a little data structure. because this is gross
	if(key == "FDS")
		installInt(val,fds);
	else if(key == "NewPPU")
		installBool(val,PPUflag);
	else if(key == "RAMInitOption")
		installInt(val,RAMInitOption);
	else if(key == "RAMInitSeed")
		installInt(val,RAMInitSeed);
	else if(key == "version")
		installInt(val,version);
	else if(key == "emuVersion")
		installInt(val,emuVersion);
	else if(key == "rerecordCount")
		installInt(val,rerecordCount);
	else if(key == "palFlag")
		installBool(val,palFlag);
	else if(key == "romFilename")
		romFilename = val;
	else if(key == "romChecksum")
		StringToBytes(val,&romChecksum,MD5DATA::size);
	else if(key == "guid")
		guid = FCEU_Guid::fromString(val);
	else if(key == "fourscore")
		installBool(val,fourscore);
	else if(key == "microphone")
		installBool(val,microphone);
	else if(key == "port0")
		installInt(val,ports[0]);
	else if(key == "port1")
		installInt(val,ports[1]);
	else if(key == "port2")
		installInt(val,ports[2]);
	else if(key == "binary")
		installBool(val,binaryFlag);
	else if(key == "comment")
		comments.push_back(mbstowcs(val));
	else if (key == "subtitle")
		subtitles.push_back(val); //mbstowcs(val));
	else if(key == "savestate")
	{
		int len = Base64StringToBytesLength(val);
		if(len == -1) len = HexStringToBytesLength(val); // wasn't base64, try hex
		if(len >= 1)
		{
			savestate.resize(len);
			StringToBytes(val,&savestate[0],len); // decodes either base64 or hex
		}
	}
	else if(key == "saveram")
	{
		int len = Base64StringToBytesLength(val);
		if(len == -1) len = HexStringToBytesLength(val); // wasn't base64, try hex
		if(len >= 1)
		{
			saveram.resize(len);
			StringToBytes(val,&saveram[0],len); // decodes either base64 or hex
		}
	}
	else if (key == "length")
	{
		installInt(val, loadFrameCount);
	}
}

int MovieData::dump(EMUFILE *os, bool binary, bool seekToCurrFramePos)
{
	int start = os->ftell();
	os->fprintf("version %d\n", version);
	os->fprintf("emuVersion %d\n", emuVersion);
	os->fprintf("rerecordCount %d\n", rerecordCount);
	os->fprintf("palFlag %d\n" , (palFlag?1:0) );
	os->fprintf("romFilename %s\n" , romFilename.c_str() );
	os->fprintf("romChecksum %s\n" , BytesToString(romChecksum.data,MD5DATA::size).c_str() );
	os->fprintf("guid %s\n" , guid.toString().c_str() );
	os->fprintf("fourscore %d\n" , (fourscore?1:0) );
	os->fprintf("microphone %d\n" , (microphone?1:0) );
	os->fprintf("port0 %d\n" , ports[0] );
	os->fprintf("port1 %d\n" , ports[1] );
	os->fprintf("port2 %d\n" , ports[2] );
	os->fprintf("FDS %d\n" , fds?1:0 );
	os->fprintf("NewPPU %d\n" , PPUflag?1:0 );
	os->fprintf("RAMInitOption %d\n", RAMInitOption);
	os->fprintf("RAMInitSeed %d\n", RAMInitSeed);

	for(uint32 i=0;i<comments.size();i++)
		os->fprintf("comment %s\n" , wcstombs(comments[i]).c_str() );

	for(uint32 i=0;i<subtitles.size();i++)
		os->fprintf("subtitle %s\n" , subtitles[i].c_str() );

	if(binary)
		os->fprintf("binary 1\n" );

	if(savestate.size())
		os->fprintf("savestate %s\n" , BytesToString(&savestate[0],savestate.size()).c_str() );

	if(saveram.size())
		os->fprintf("saveram %s\n" , BytesToString(&saveram[0],saveram.size()).c_str() );

	if (this->loadFrameCount >= 0)
		os->fprintf("length %d\n" , this->loadFrameCount);

	int currFramePos = -1;
	if(binary)
	{
		//put one | to start the binary dump
		os->fputc('|');
		for (int i = 0; i < (int)records.size(); i++)
		{
			if (seekToCurrFramePos && currFrameCounter == i)
				currFramePos = os->ftell();
			records[i].dumpBinary(this, os, i);
		}
	} else
	{
		for (int i = 0; i < (int)records.size(); i++)
		{
			if (seekToCurrFramePos && currFrameCounter == i)
				currFramePos = os->ftell();
			records[i].dump(this, os, i);
		}
	}

	int end = os->ftell();
	if (currFramePos >= 0)
		os->fseek(currFramePos, SEEK_SET);
	return end-start;
}

int FCEUMOV_GetFrame(void)
{
	return currFrameCounter;
}

int FCEUI_GetLagCount(void)
{
	return lagCounter;
}

bool FCEUI_GetLagged(void)
{
	if (lagFlag)
		return true;
	else
		return false;
}
void FCEUI_SetLagFlag(bool value)
{
	lagFlag = (value) ? 1 : 0;
}

bool FCEUMOV_ShouldPause(void)
{
	if(pauseframe && currFrameCounter+1 == pauseframe)
	{
		pauseframe = 0;
		return true;
	}
	else
	{
		return false;
	}
}

EMOVIEMODE FCEUMOV_Mode()
{
	return movieMode;
}

bool FCEUMOV_Mode(EMOVIEMODE modemask)
{
	return (movieMode&modemask)!=0;
}

bool FCEUMOV_Mode(int modemask)
{
	return FCEUMOV_Mode((EMOVIEMODE)modemask);
}

static void LoadFM2_binarychunk(MovieData& movieData, EMUFILE* fp, int size)
{
	int recordsize = 1; //1 for the command
	if(movieData.fourscore)
		recordsize += 4; //4 joysticks
	else
	{
		for(int i=0;i<2;i++)
		{
			switch(movieData.ports[i])
			{
			case SI_GAMEPAD: recordsize++; break;
			case SI_ZAPPER: recordsize+=12; break;
			}
		}
	}

	//find out how much remains in the file
	int curr = fp->ftell();
	fp->fseek(0,SEEK_END);
	int end = fp->ftell();
	int flen = end-curr;
	fp->fseek(curr,SEEK_SET);

	//the amount todo is the min of the limiting size we received and the remaining contents of the file
	int todo = std::min<int>(size, flen);

	int numRecords = todo/recordsize;
	if (movieData.loadFrameCount!=-1 && movieData.loadFrameCount<numRecords)
		numRecords=movieData.loadFrameCount;

	movieData.records.resize(numRecords);
	for(int i=0;i<numRecords;i++)
	{
		movieData.records[i].parseBinary(&movieData,fp);
	}
}

//yuck... another custom text parser.
bool LoadFM2(MovieData& movieData, EMUFILE* fp, int size, bool stopAfterHeader)
{
	// if there's no "binary" tag in the movie header, consider it as a movie in text format
	movieData.binaryFlag = false;
	// Non-TASEditor projects consume until EOF
	movieData.loadFrameCount = -1;

	std::ios::pos_type curr = fp->ftell();

	if (!stopAfterHeader)
	{
		// first, look for an fcm signature
		char fcmbuf[3];
		fp->fread(fcmbuf,3);
		fp->fseek(curr,SEEK_SET);
		if(!strncmp(fcmbuf,"FCM",3)) {
			FCEU_PrintError("FCM File format is no longer supported. Please use Tools > Convert FCM");
			return false;
		}
	}

	//movie must start with "version 3"
	char buf[9];
	curr = fp->ftell();
	fp->fread(buf,9);
	fp->fseek(curr,SEEK_SET);
	if(fp->fail()) return false;
	if(memcmp(buf,"version 3",9))
		return false;

	std::string key,value;
	enum {
		NEWLINE, KEY, SEPARATOR, VALUE, RECORD, COMMENT, SUBTITLE
	} state = NEWLINE;
	bool bail = false;
	bool iswhitespace, isrecchar, isnewline;
	int c;
	for(;;)
	{
		if(size--<=0) goto bail;
		c = fp->fgetc();
		if(c == -1)
			goto bail;
		iswhitespace = (c==' '||c=='\t');
		isrecchar = (c=='|');
		isnewline = (c==10||c==13);
		if(isrecchar && movieData.binaryFlag && !stopAfterHeader)
		{
			LoadFM2_binarychunk(movieData, fp, size);
			return true;
		} else if (isnewline && static_cast<size_t>(movieData.loadFrameCount) == movieData.records.size())
			// exit prematurely if loaded the specified amound of records
			return true;
		switch(state)
		{
		case NEWLINE:
			if(isnewline) goto done;
			if(iswhitespace) goto done;
			if(isrecchar)
				goto dorecord;
			//must be a key
			key = "";
			value = "";
			goto dokey;
			break;
		case RECORD:
			{
				dorecord:
				if (stopAfterHeader) return true;
				int currcount = movieData.records.size();
				movieData.records.resize(currcount+1);
				int preparse = fp->ftell();
				movieData.records[currcount].parse(&movieData, fp);
				int postparse = fp->ftell();
				size -= (postparse-preparse);
				state = NEWLINE;
				break;
			}

		case KEY:
			dokey: //dookie
			state = KEY;
			if(iswhitespace) goto doseparator;
			if(isnewline) goto commit;
			key += c;
			break;
		case SEPARATOR:
			doseparator:
			state = SEPARATOR;
			if(isnewline) goto commit;
			if(!iswhitespace) goto dovalue;
			break;
		case VALUE:
			dovalue:
			state = VALUE;
			if(isnewline) goto commit;
			value += c;
			break;
		default:
			break;
		}
		goto done;

		bail:
		bail = true;
		if(state == VALUE) goto commit;
		goto done;
		commit:
		movieData.installValue(key,value);
		state = NEWLINE;
		done: ;
		if(bail) break;
	}

	return true;
}

static const char *GetMovieModeStr()
{
	if (movieMode == MOVIEMODE_INACTIVE)
		return " (no movie)";
	else if (movieMode == MOVIEMODE_PLAY)
		return " (playing)";
	else if (movieMode == MOVIEMODE_RECORD)
		return " (recording)";
	else if (movieMode == MOVIEMODE_FINISHED)
		return " (finished)";
	else if (movieMode == MOVIEMODE_TASEDITOR)
		return " (taseditor)";
	else
		return ".";
}

static const char *GetMovieReadOnlyStr()
{
	if (movieMode == MOVIEMODE_RECORD)
		return movie_readonly ? " R-O" : "";
	else
		return movie_readonly ? "" : " R+W";
}

static const char *GetMovieRecordModeStr()
{
	switch (movieRecordMode)
	{
	case MOVIE_RECORD_MODE_OVERWRITE:
		return " [W]";
	case MOVIE_RECORD_MODE_INSERT:
		return " [I]";
	default:
		return "";
	}
}

static EMUFILE *openRecordingMovie(const char* fname)
{
	if (osRecordingMovie)
		delete osRecordingMovie;

	osRecordingMovie = FCEUD_UTF8_fstream(fname, "wb");
	if (!osRecordingMovie || osRecordingMovie->fail()) {
		FCEU_PrintError("Error opening movie output file: %s", fname);
		return NULL;
	}
	if ( fname != curMovieFilename.c_str() )
	{
		curMovieFilename.assign(fname);
	}

	return osRecordingMovie;
}

static void closeRecordingMovie()
{
	if (osRecordingMovie)
	{
		delete osRecordingMovie;
		osRecordingMovie = 0;
	}
}

// Callers shall set the approriate movieMode before calling this
static void RedumpWholeMovieFile(bool justToggledRecording = false)
{
	bool recording = (movieMode == MOVIEMODE_RECORD);
	assert((NULL != osRecordingMovie) == (recording != justToggledRecording) && "osRecordingMovie should be consistent with movie mode!");

	if (NULL == openRecordingMovie(curMovieFilename.c_str()))
		return;

	currMovieData.dump(osRecordingMovie, false/*currMovieData.binaryFlag*/, recording);
	if (recording)
		osRecordingMovie->fflush();
	else
		closeRecordingMovie();
}

/// Stop movie playback.
static void StopPlayback()
{
	assert(movieMode != MOVIEMODE_RECORD && NULL == osRecordingMovie);

	movieMode = MOVIEMODE_INACTIVE;
	FCEU_DispMessageOnMovie("Movie playback stopped.");
}

// Stop movie playback without closing the movie.
static void FinishPlayback()
{
	assert(movieMode != MOVIEMODE_RECORD);

	extern int closeFinishedMovie;
	if (closeFinishedMovie)
		StopPlayback();
	else
	{
		movieMode = MOVIEMODE_FINISHED;
		FCEU_DispMessage("Movie finished playing.",0);
	}
}

/// Stop movie recording
static void StopRecording()
{
	assert(movieMode == MOVIEMODE_RECORD);

	movieMode = MOVIEMODE_INACTIVE;
	RedumpWholeMovieFile(true);
	FCEU_DispMessage("Movie recording stopped.",0);
}

static void OnMovieClosed()
{
	assert(movieMode == MOVIEMODE_INACTIVE);

	curMovieFilename.clear();			//No longer a current movie filename
	freshMovie = false;					//No longer a fresh movie loaded
	if (bindSavestate) AutoSS = false;	//If bind movies to savestates is true, then there is no longer a valid auto-save to load

#if defined(__WIN_DRIVER__)
	SetMainWindowText();
#endif
}

bool bogorf;

void FCEUI_StopMovie()
{
	if (suppressMovieStop)
		return;

	if (movieMode == MOVIEMODE_PLAY || movieMode == MOVIEMODE_FINISHED)
		StopPlayback();
	else if (movieMode == MOVIEMODE_RECORD)
		StopRecording();

	OnMovieClosed();
}

void poweron(bool shouldDisableBatteryLoading)
{
	//// make a for-movie-recording power-on clear the game's save data, too
	//extern char lastLoadedGameName [2048];
	//extern int disableBatteryLoading, suppressAddPowerCommand;
	//suppressAddPowerCommand=1;
	//if(shouldDisableBatteryLoading) disableBatteryLoading=1;
	//suppressMovieStop=true;
	//{
	//	//we need to save the pause state through this process
	//	int oldPaused = EmulationPaused;

	//	// NOTE:  this will NOT write an FCEUNPCMD_POWER into the movie file
	//	FCEUGI* gi = FCEUI_LoadGame(lastLoadedGameName, 0);
	//	assert(gi);
	//	PowerNES();

	//	EmulationPaused = oldPaused;
	//}
	//suppressMovieStop=false;
	//if(shouldDisableBatteryLoading) disableBatteryLoading=0;
	//suppressAddPowerCommand=0;

	extern int disableBatteryLoading;
	if(!bogorf) disableBatteryLoading = 1;
	PowerNES();
	if(!bogorf) disableBatteryLoading = 0;
}

void FCEUMOV_CreateCleanMovie()
{
	currMovieData = MovieData();
	currMovieData.palFlag = FCEUI_GetCurrentVidSystem(0,0)!=0;
	currMovieData.romFilename = FileBase;
	if ( GameInfo )
	{
		currMovieData.romChecksum = GameInfo->MD5;
	}
	currMovieData.guid.newGuid();
	currMovieData.fourscore = FCEUI_GetInputFourscore();
	currMovieData.microphone = FCEUI_GetInputMicrophone();
	currMovieData.ports[0] = joyports[0].type;
	currMovieData.ports[1] = joyports[1].type;
	currMovieData.ports[2] = portFC.type;
	currMovieData.fds = isFDS;
	currMovieData.PPUflag = (newppu != 0);
	currMovieData.RAMInitOption = RAMInitOption;
	currMovieData.RAMInitSeed = RAMInitSeed;
}
void FCEUMOV_ClearCommands()
{
	_currCommand = 0;
}

bool FCEUMOV_FromPoweron()
{
	return movieFromPoweron;
}
bool MovieData::loadSavestateFrom(std::vector<uint8>* buf)
{
	EMUFILE_MEMORY ms(buf);
	return FCEUSS_LoadFP(&ms,SSLOADPARAM_BACKUP);
}

void MovieData::dumpSavestateTo(std::vector<uint8>* buf, int compressionLevel)
{
	EMUFILE_MEMORY ms(buf);
	FCEUSS_SaveMS(&ms,compressionLevel);
	ms.trim();
}

bool MovieData::loadSaveramFrom(std::vector<uint8>* buf)
{
	EMUFILE_MEMORY ms(buf);

	bool hasBattery = !!ms.read32le();
	if(hasBattery != !!currCartInfo->battery)
	{
		FCEU_PrintError("movie battery load mismatch 1");
		return false;
	}

	for (size_t i=0;i<currCartInfo->SaveGame.size();i++)
	{
		int len = ms.read32le();

		if( (currCartInfo->SaveGame[i].bufptr == nullptr) && (len!=0) )
		{
			FCEU_PrintError("movie battery load mismatch 2");
			return false;
		}

		if(currCartInfo->SaveGame[i].buflen != static_cast<size_t>(len))
		{
			FCEU_PrintError("movie battery load mismatch 3");
			return false;
		}

		ms.fread(currCartInfo->SaveGame[i].bufptr, len);
	}

	return true;
}

void MovieData::dumpSaveramTo(std::vector<uint8>* buf, int compressionLevel)
{
	EMUFILE_MEMORY ms(buf);

	ms.write32le(currCartInfo->battery?1:0);
	for(size_t i=0;i<currCartInfo->SaveGame.size();i++)
	{
		if (!currCartInfo->SaveGame[i].bufptr)
		{
			ms.write32le((u32)0);
			continue;
		}
		ms.write32le( static_cast<uint32>(currCartInfo->SaveGame[i].buflen) );
		ms.fwrite(currCartInfo->SaveGame[i].bufptr, currCartInfo->SaveGame[i].buflen);
	}

}


//begin playing an existing movie
bool FCEUI_LoadMovie(const char *fname, bool _read_only, int _pauseframe)
{
	if(!FCEU_IsValidUI(FCEUI_PLAYMOVIE))
		return true;	//adelikat: file did not fail to load, so let's return true here, just do nothing

	assert(fname);

	//mbg 6/10/08 - we used to call StopMovie here, but that cleared curMovieFilename and gave us crashes...
	if(movieMode == MOVIEMODE_PLAY || movieMode == MOVIEMODE_FINISHED)
		StopPlayback();
	else if(movieMode == MOVIEMODE_RECORD)
		StopRecording();
	//--------------

	currMovieData = MovieData();

	curMovieFilename.assign(fname);
	FCEUFILE *fp = FCEU_fopen(fname,0,"rb",0);
	if (!fp) return false;
	if(fp->isArchive() && !_read_only) {
		FCEU_PrintError("Cannot open a movie in read+write from an archive.");
		return true;	//adelikat: file did not fail to load, so return true (false is only for file not exist/unable to open errors
	}

#ifdef __WIN_DRIVER__
	//Fix relative path if necessary and then add to the recent movie menu
	extern std::string BaseDirectory;

	string name = fname;
	if (IsRelativePath(fname))
	{
		name = ConvertRelativePath(name);
	}
	AddRecentMovieFile(name.c_str());
#endif

	LoadFM2(currMovieData, fp->stream, fp->size, false);
	LoadSubtitles(currMovieData);
	delete fp;

	RAMInitOption = currMovieData.RAMInitOption;
	RAMInitSeed = currMovieData.RAMInitSeed;

	freshMovie = true;	//Movie has been loaded, so it must be unaltered
	if (bindSavestate) AutoSS = false;	//If bind savestate to movie is true, then their isn't a valid auto-save to load, so flag it
	cur_input_display = 0; //clear previous input display
	//fully reload the game to reinitialize everything before playing any movie
	poweron(true);

	if(currMovieData.savestate.size())
	{
		//WE NEED TO LOAD A SAVESTATE
		movieFromPoweron = false;
		bool success = MovieData::loadSavestateFrom(&currMovieData.savestate);
		if(!success) return true;	//adelikat: I guess return true here?  False is only for a bad movie filename, if it got this far the file was good?
	}
	else if(currMovieData.saveram.size())
	{
		movieFromPoweron = true;
		bool success = MovieData::loadSaveramFrom(&currMovieData.saveram);
		if(!success) return true;	//adelikat: I guess return true here?  False is only for a bad movie filename, if it got this far the file was good?
	}
	else {
		movieFromPoweron = true;
	}

	//if there is no savestate, we won't have this crucial piece of information at the start of the movie.
	//so, we have to include it with the movie
	if(currMovieData.palFlag)
		FCEUI_SetVidSystem(1);
	else
		FCEUI_SetVidSystem(0);



	//force the input configuration stored in the movie to apply
	FCEUD_SetInput(currMovieData.fourscore, currMovieData.microphone, (ESI)currMovieData.ports[0], (ESI)currMovieData.ports[1], (ESIFC)currMovieData.ports[2]);

	//stuff that should only happen when we're ready to positively commit to the replay
	currFrameCounter = 0;
	pauseframe = _pauseframe;
	movie_readonly = _read_only;
	movieMode = MOVIEMODE_PLAY;
	if (movieMode != MOVIEMODE_TASEDITOR)
		currRerecordCount = currMovieData.rerecordCount;

	if(movie_readonly)
		FCEU_DispMessage("Replay started Read-Only.",0);
	else
		FCEU_DispMessage("Replay started Read+Write.",0);

#ifdef __WIN_DRIVER__
	SetMainWindowText();
#endif

	#ifdef CREATE_AVI
	if(LoggingEnabled)
	{
	    FCEU_DispMessage("Video recording enabled.\n",0);
	    LoggingEnabled = 2;
	}
	#endif

	return true;
}


//begin recording a new movie
//TODO - BUG - the record-from-another-savestate doesnt work.
void FCEUI_SaveMovie(const char *fname, EMOVIE_FLAG flags, std::wstring author)
{
	if(!FCEU_IsValidUI(FCEUI_RECORDMOVIE))
		return;

	assert(fname);

	FCEUI_StopMovie();

	if (NULL == openRecordingMovie(fname))
		return;

#ifdef __WIN_DRIVER__
	//Add to the recent movie menu
	AddRecentMovieFile(fname);
#endif

	currFrameCounter = 0;
	LagCounterReset();
	FCEUMOV_CreateCleanMovie();
	if(author != L"") currMovieData.comments.push_back(L"author " + author);

	if(flags & MOVIE_FLAG_FROM_POWERON)
	{
		movieFromPoweron = true;
		poweron(true);
	}
	else if(flags & MOVIE_FLAG_FROM_SAVERAM)
	{
		movieFromPoweron = true;
		MovieData::dumpSaveramTo(&currMovieData.saveram,Z_NO_COMPRESSION); //i guess with this there's a chance someone could hack the file, at least, so maybe it's helpfu
		bogorf = true;
		poweron(false);
		bogorf = false;
	}
	else //from savestate
	{
		movieFromPoweron = false;
		MovieData::dumpSavestateTo(&currMovieData.savestate,Z_BEST_COMPRESSION);
	}

	FCEUMOV_ClearCommands();

	//we are going to go ahead and dump the header. from now on we will only be appending frames
	currMovieData.dump(osRecordingMovie, false);

	movieMode = MOVIEMODE_RECORD;
	movie_readonly = false;
	if (movieMode != MOVIEMODE_TASEDITOR)
		currRerecordCount = 0;

	FCEU_DispMessage("Movie recording started.",0);
}


//the main interaction point between the emulator and the movie system.
//either dumps the current joystick state or loads one state from the movie
void FCEUMOV_AddInputState()
{
#if defined(__WIN_DRIVER__) || defined(__QT_DRIVER__)
	if (movieMode == MOVIEMODE_TASEDITOR)
	{
		// if movie length is less or equal to currFrame, pad it with empty frames
		if (((int)currMovieData.records.size() - 1) < (currFrameCounter + 1))
			currMovieData.insertEmpty(-1, (currFrameCounter + 1) - ((int)currMovieData.records.size() - 1));

		MovieRecord* mr = &currMovieData.records[currFrameCounter];
		if (isTaseditorRecording())
		{
			// record commands and buttons
			mr->commands |= _currCommand;
			joyports[0].log(mr);
			joyports[1].log(mr);
			recordInputByTaseditor();
		}
		// replay buttons
		joyports[0].load(mr);
		joyports[1].load(mr);
		// replay commands
		if (mr->command_power())
			PowerNES();
		if (mr->command_reset())
			ResetNES();
		if (mr->command_fds_insert())
			FCEU_FDSInsert();
		if (mr->command_fds_select())
			FCEU_FDSSelect();
		if (mr->command_vs_insertcoin())
			FCEU_VSUniCoin(0);
		if (mr->command_vs_insertcoin2())
			FCEU_VSUniCoin(1);
		if (mr->command_vs_service())
			FCEU_VSUniService();
		_currCommand = 0;
	} else
#endif
	if (movieMode == MOVIEMODE_PLAY)
	{
		//stop when we run out of frames
		if (currFrameCounter >= (int)currMovieData.records.size())
		{
			FinishPlayback();
			//tell all drivers to poll input and set up their logical states
			for(int port=0;port<2;port++)
				joyports[port].driver->Update(port,joyports[port].ptr,joyports[port].attrib);
			portFC.driver->Update(portFC.ptr,portFC.attrib);
		} else
		{
			MovieRecord* mr = &currMovieData.records[currFrameCounter];

			//reset and power cycle if necessary
			if(mr->command_power())
				PowerNES();
			if(mr->command_reset())
				ResetNES();
			if(mr->command_fds_insert())
				FCEU_FDSInsert();
			if(mr->command_fds_select())
				FCEU_FDSSelect();
			if (mr->command_vs_insertcoin())
				FCEU_VSUniCoin(0);
			if (mr->command_vs_insertcoin2())
				FCEU_VSUniCoin(1);
			if (mr->command_vs_service())
				FCEU_VSUniService();

			joyports[0].load(mr);
			joyports[1].load(mr);
		}

		//if we are on the last frame, then pause the emulator if the player requested it
		if ( static_cast<size_t>(currFrameCounter) == currMovieData.records.size()-1)
		{
			if(FCEUD_PauseAfterPlayback())
			{
				FCEUI_ToggleEmulationPause();
			}
		}

		//pause the movie at a specified frame
		if (FCEUMOV_ShouldPause() && FCEUI_EmulationPaused()==0)
		{
			FCEUI_ToggleEmulationPause();
			FCEU_DispMessage("Paused at specified movie frame",0);
		}

	} else if (movieMode == MOVIEMODE_RECORD)
	{
		MovieRecord mr;

		joyports[0].log(&mr);
		joyports[1].log(&mr);
		mr.commands = _currCommand;
		_currCommand = 0;

		//aquanull: now it supports other recording modes that don't necessarily truncate further frame data
		//If the user chooses it can be delayed to here
		if (currFrameCounter < (int)currMovieData.records.size())
			switch (movieRecordMode)
			{
			case MOVIE_RECORD_MODE_OVERWRITE:
				currMovieData.records[currFrameCounter].Clone(mr);
				break;
			case MOVIE_RECORD_MODE_INSERT:
				//FIXME: this could be very insufficient
				currMovieData.records.insert(currMovieData.records.begin() + currFrameCounter, mr);
				break;
			//case MOVIE_RECORD_MODE_TRUNCATE:
			default:
				//Adelikat: in normal mode, this is done at the time of loading a savestate in read+write mode
				currMovieData.truncateAt(currFrameCounter);
				currMovieData.records.push_back(mr);
				break;
			}
		else
			currMovieData.records.push_back(mr);

		mr.dump(&currMovieData, osRecordingMovie, currFrameCounter);	// to disk
	}

	currFrameCounter++;

	extern uint8 joy[4];
	memcpy(&cur_input_display,joy,4);
}


//TODO
void FCEUMOV_AddCommand(int cmd)
{
	// do nothing if not recording a movie
	if(movieMode != MOVIEMODE_RECORD && movieMode != MOVIEMODE_TASEDITOR)
		return;

	// translate "FCEU NetPlay" command to "FCEU Movie" command
	switch (cmd)
	{
		case FCEUNPCMD_RESET: cmd = MOVIECMD_RESET; break;
		case FCEUNPCMD_POWER: cmd = MOVIECMD_POWER; break;
		case FCEUNPCMD_FDSINSERT: cmd = MOVIECMD_FDS_INSERT; break;
		case FCEUNPCMD_FDSSELECT: cmd = MOVIECMD_FDS_SELECT; break;
		case FCEUNPCMD_VSUNICOIN: cmd = MOVIECMD_VS_INSERTCOIN; break;
		case FCEUNPCMD_VSUNICOIN2: cmd = MOVIECMD_VS_INSERTCOIN2; break;
		case FCEUNPCMD_VSUNISERVICE: cmd = MOVIECMD_VS_SERVICE; break;
		// all other netplay commands (e.g. FCEUNPCMD_VSUNIDIP0) are not supported by movie recorder for now
		default: return;
	}

	_currCommand |= cmd;
}

void FCEU_DrawMovies(uint8 *XBuf)
{
	// not the best place, but just working
	assert((NULL != osRecordingMovie) == (movieMode == MOVIEMODE_RECORD));

	if (frame_display)
	{
		char counterbuf[32] = {0};
		int color = 0x20;
		
		if (movieMode == MOVIEMODE_PLAY)
		{
			snprintf(counterbuf, sizeof(counterbuf), "%d/%d%s%s", currFrameCounter, (int)currMovieData.records.size(), GetMovieRecordModeStr(), GetMovieReadOnlyStr());
		} else if (movieMode == MOVIEMODE_RECORD)
		{
			if (movieRecordMode == MOVIE_RECORD_MODE_TRUNCATE)
				snprintf(counterbuf, sizeof(counterbuf), "%d%s%s (record)", currFrameCounter, GetMovieRecordModeStr(), GetMovieReadOnlyStr()); // nearly classic
			else
				snprintf(counterbuf, sizeof(counterbuf), "%d/%d%s%s (record)", currFrameCounter, (int)currMovieData.records.size(), GetMovieRecordModeStr(), GetMovieReadOnlyStr());
		} else if (movieMode == MOVIEMODE_FINISHED)
		{
			snprintf(counterbuf, sizeof(counterbuf), "%d/%d%s%s (finished)",currFrameCounter,(int)currMovieData.records.size(), GetMovieRecordModeStr(), GetMovieReadOnlyStr());
			color = 0x17; //Show red to get attention
		} else if (movieMode == MOVIEMODE_TASEDITOR)
		{
			snprintf(counterbuf, sizeof(counterbuf),"%d",currFrameCounter);
		} else
			snprintf(counterbuf, sizeof(counterbuf),"%d (no movie)",currFrameCounter);

		if (counterbuf[0])
			DrawTextTrans(ClipSidesOffset+XBuf+FCEU_TextScanlineOffsetFromBottom(30)+1, 256, (uint8*)counterbuf, color+0x80);
	}
	if (rerecord_display && movieMode != MOVIEMODE_INACTIVE)
	{
		char counterbuf[32] = {0};
		snprintf(counterbuf, sizeof(counterbuf), "%d", currMovieData.rerecordCount);

		if (counterbuf[0])
			DrawTextTrans(ClipSidesOffset+XBuf+FCEU_TextScanlineOffsetFromBottom(50)+1, 256, (uint8*)counterbuf, 0x28+0x80);
	}
}

void FCEU_DrawLagCounter(uint8 *XBuf)
{
	if (lagCounterDisplay)
	{
		// If currently lagging - display red, else display green
		uint8 color = (lagFlag) ? (0x16+0x80) : (0x2A+0x80);
		snprintf(lagcounterbuf, sizeof(lagcounterbuf), "%d", lagCounter);
		if(lagcounterbuf[0])
			DrawTextTrans(ClipSidesOffset + XBuf + FCEU_TextScanlineOffsetFromBottom(40) + 1, 256, (uint8*)lagcounterbuf, color);
	}
}

int FCEUMOV_WriteState(EMUFILE* os)
{
	//we are supposed to dump the movie data into the savestate
	if(movieMode == MOVIEMODE_RECORD || movieMode == MOVIEMODE_PLAY || movieMode == MOVIEMODE_FINISHED)
		return currMovieData.dump(os, true);
	else return 0;
}

// returns
int CheckTimelines(MovieData& stateMovie, MovieData& currMovie)
{
	// end_frame = min(urrMovie.records.size(), stateMovie.records.size(), currFrameCounter)
	int end_frame = currMovie.records.size();
	if (end_frame > (int)stateMovie.records.size())
		end_frame = stateMovie.records.size();
	if (end_frame > currFrameCounter)
		end_frame = currFrameCounter;

	for (int x = 0; x < end_frame; x++)
	{
		if (!stateMovie.records[x].Compare(currMovie.records[x]))
			return x;
	}
	// no mismatch found
	return -1;
}


static bool load_successful = false;

bool FCEUMOV_ReadState(EMUFILE* is, uint32 size)
{
	load_successful = false;

	if (!movie_readonly)
	{
		if (currMovieData.loadFrameCount >= 0)
		{
#ifdef __WIN_DRIVER__
			int result = MessageBox(hAppWnd, "This movie is a TAS Editor project file.\nIt can be modified in TAS Editor only.\n\nOpen it in TAS Editor now?", "Movie Replay", MB_YESNO);
			if (result == IDYES)
				mustEngageTaseditor = true;
#else
			FCEUI_printf("This movie is a TAS Editor project file! It can be modified in TAS Editor only.\nMovie is now Read-Only.\n");
#endif
			movie_readonly = true;
		}
		if (FCEU_isFileInArchive(curMovieFilename.c_str()))
		{
			//a little rule: cant load states in read+write mode with a movie from an archive.
			//so we are going to switch it to readonly mode in that case
			FCEU_PrintError("Cannot loadstate in Read+Write with movie from archive. Movie is now Read-Only.");
			movie_readonly = true;
		}
	}

	MovieData tempMovieData = MovieData();
	std::ios::pos_type curr = is->ftell();
	if(!LoadFM2(tempMovieData, is, size, false)) {
		is->fseek((uint32)curr+size,SEEK_SET);
		extern bool FCEU_state_loading_old_format;
		if(FCEU_state_loading_old_format) {
			if(movieMode == MOVIEMODE_PLAY || movieMode == MOVIEMODE_RECORD || movieMode == MOVIEMODE_FINISHED) {
				//FCEUI_StopMovie();  //No reason to stop the movie, nothing destructive has happened yet.
				FCEU_PrintError("You have tried to use an old savestate while playing a movie. This is unsupported (since the old savestate has old-format movie data in it which can't be converted on the fly)");
			}
		}
		return false;
	}

	//----------------
	//complex TAS logic for loadstate
	//fully conforms to the savestate logic documented in the Laws of TAS
	//http://tasvideos.org/LawsOfTAS/OnSavestates.html
	//----------------

	/*
	Playback or Recording + Read-only

    * Check that GUID of movie and savestate-movie must match or else error
          o on error: a message informing that the savestate doesn't belong to this movie. This is a GUID mismatch error. Give user a choice to load it anyway.
                + failstate: if use declines, loadstate attempt canceled, movie can resume as if not attempted if user has backup savstates enabled else stop movie
    * Check that movie and savestate-movie are in same timeline. If not then this is a wrong timeline error.
          o on error: a message informing that the savestate doesn't belong to this movie
                + failstate: loadstate attempt canceled, movie can resume as if not attempted if user has backup savestates enabled else stop movie
    * Check that savestate-movie is not greater than movie. If it's greater then this is a future event error and is not allowed in read-only
          o on error: message informing that the savestate is from a frame after the last frame of the movie
                + failstate - loadstate attempt cancelled, movie can resume if user has backup savesattes enabled, else stop movie
    * Check that savestate framcount <= savestate movie length. If not this is a post-movie savestate and is not allowed in read-only
          o on error: message informing that the savestate is from a frame after the last frame of the savestated movie
                + failstate - loadstate attempt cancelled, movie can resume if user has backup savesattes enabled, else stop movie
    * All error checks have passed, state will be loaded
    * Movie contained in the savestate will be discarded
    * Movie is now in Playback mode

	Playback, Recording + Read+write

    * Check that GUID of movie and savestate-movie must match or else error
          o on error: a message informing that the savestate doesn't belong to this movie. This is a GUID mismatch error. Give user a choice to load it anyway.
                + failstate: if use declines, loadstate attempt canceled, movie can resume as if not attempted (stop movie if resume fails)canceled, movie can resume if backup savestates enabled else stopmovie
    * Check that savestate framcount <= savestate movie length. If not this is a post-movie savestate
          o on post-movie: See post-movie event section.
    * savestate passed all error checks and will now be loaded in its entirety and replace movie (note: there will be no truncation yet)
    * current framecount will be set to savestate framecount
    * on the next frame of input, movie will be truncated to framecount
          o (note: savestate-movie can be a future event of the movie timeline, or a completely new timeline and it is still allowed)
    * Rerecord count of movie will be incremented
    * Movie is now in record mode

	Post-movie savestate event

    * Whan a savestate is loaded and is determined that the savestate-movie length is less than the savestate framecount then it is a post-movie savestate. These occur when a savestate was made during Movie Finished mode.
	* If read+write, the entire savestate movie will be loaded and replace current movie.
    * If read-only, the savestate movie will be discarded
    * Mode will be switched to Move Finished
    * Savestate will be loaded
    * Current framecount changes to savestate framecount
    * User will have control of input as if Movie inactive mode
	*/

	if(movieMode == MOVIEMODE_PLAY || movieMode == MOVIEMODE_RECORD || movieMode == MOVIEMODE_FINISHED)
	{
		//handle moviefile mismatch
		if(tempMovieData.guid != currMovieData.guid)
		{
			//mbg 8/18/08 - this code  can be used to turn the error message into an OK/CANCEL
			#ifdef __WIN_DRIVER__
				std::string msg = "There is a mismatch between savestate's movie and current movie.\ncurrent: " + currMovieData.guid.toString() + "\nsavestate: " + tempMovieData.guid.toString() + "\n\nThis means that you have loaded a savestate belonging to a different movie than the one you are playing now.\n\nContinue loading this savestate anyway?";
				int result = MessageBox(hAppWnd, msg.c_str(), "Error loading savestate", MB_OKCANCEL);
				if(result == IDCANCEL)
				{
					if (!backupSavestates) //If backups are disabled we can just resume normally since we can't restore so stop movie and inform user
					{
						FCEU_PrintError("Unable to restore backup, movie playback stopped.");
						FCEUI_StopMovie();
					}

					return false;
				}
			#else
				if (!backupSavestates) //If backups are disabled we can just resume normally since we can't restore so stop movie and inform user
				{
					FCEU_PrintError("Mismatch between savestate's movie and current movie.\ncurrent: %s\nsavestate: %s\nUnable to restore backup, movie playback stopped.\n",currMovieData.guid.toString().c_str(),tempMovieData.guid.toString().c_str());
					FCEUI_StopMovie();
				}
				else
				FCEU_PrintError("Mismatch between savestate's movie and current movie.\ncurrent: %s\nsavestate: %s\n",currMovieData.guid.toString().c_str(),tempMovieData.guid.toString().c_str());

				return false;
			#endif
		}

		if (movie_readonly)
		{
			if (movieMode == MOVIEMODE_RECORD)
			{
				movieMode = MOVIEMODE_PLAY;
				RedumpWholeMovieFile(true);
				closeRecordingMovie();
			}

			// currFrameCounter at this point represents the savestate framecount
			int frame_of_mismatch = CheckTimelines(tempMovieData, currMovieData);
			if (frame_of_mismatch >= 0)
			{
				// Wrong timeline, do apprioriate logic here
				if (!backupSavestates)	//If backups are disabled we can just resume normally since we can't restore so stop movie and inform user
				{
					FCEU_PrintError("Error: Savestate not in the same timeline as movie!\nFrame %d branches from current timeline\nUnable to restore backup, movie playback stopped.", frame_of_mismatch);
					FCEUI_StopMovie();
				} else
					FCEU_PrintError("Error: Savestate not in the same timeline as movie!\nFrame %d branches from current timeline", frame_of_mismatch);
				return false;
			} else if ((int)tempMovieData.records.size() < currFrameCounter)
			{
				// this is post-movie savestate and must be checked further
				if (tempMovieData.records.size() < currMovieData.records.size())
				{
					// this savestate doesn't contain enough input to be checked
					//TODO: turn frame counter to red to get attention
					if (!backupSavestates)	//If backups are disabled we can just resume normally since we can't restore so stop movie and inform user
					{
						FCEU_PrintError("Error: Savestate taken from a frame (%d) after the final frame in the savestated movie (%zi) cannot be verified against current movie (%zi). This is not permitted.\nUnable to restore backup, movie playback stopped.", currFrameCounter, tempMovieData.records.size() - 1, currMovieData.records.size() - 1);
						FCEUI_StopMovie();
					} else
						FCEU_PrintError("Savestate taken from a frame (%d) after the final frame in the savestated movie (%zi) cannot be verified against current movie (%zi). This is not permitted.", currFrameCounter, tempMovieData.records.size() - 1, currMovieData.records.size() - 1);
					return false;
				}
			}

			// Finally, this is a savestate file for this movie
			// We'll allow loading post-movie savestates that were made after finishing current movie
			if (currFrameCounter < (int)currMovieData.records.size())
				movieMode = MOVIEMODE_PLAY;
			else
				FinishPlayback();
		} else
		{
			//Read+Write mode
			closeRecordingMovie();

			if (currFrameCounter > (int)tempMovieData.records.size())
			{
				//This is a post movie savestate, handle it differently
				//Replace movie contents but then switch to movie finished mode
				currMovieData = tempMovieData;
				movieMode = MOVIEMODE_PLAY;
				FCEUMOV_IncrementRerecordCount();
				RedumpWholeMovieFile();
				FinishPlayback();
			} else
			{
				//truncate before we copy, just to save some time, unless the user selects a full copy option
				if (!fullSaveStateLoads)
					//we can only assume this here since we have checked that the frame counter is not greater than the movie data
					tempMovieData.truncateAt(currFrameCounter);
				
				currMovieData = tempMovieData;
				movieMode = MOVIEMODE_RECORD;
				FCEUMOV_IncrementRerecordCount();
				RedumpWholeMovieFile(true);
			}
		}
	}

	load_successful = true;

	return true;
}

void FCEUMOV_PreLoad(void)
{
	load_successful=0;
}

bool FCEUMOV_PostLoad(void)
{
	if(movieMode == MOVIEMODE_INACTIVE || movieMode == MOVIEMODE_TASEDITOR)
		return true;
	else
		return load_successful;
}

void FCEUMOV_IncrementRerecordCount()
{
	bool skip = false;
#ifdef _S9XLUA_H
	skip = skip || FCEU_LuaRerecordCountSkip();
#endif
#ifdef __FCEU_QSCRIPT_ENABLE__
	extern bool FCEU_JSRerecordCountSkip();
	skip = skip || FCEU_JSRerecordCountSkip();
#endif

	if(!skip)
		if (movieMode != MOVIEMODE_TASEDITOR)
			currRerecordCount++;
		else
			currMovieData.rerecordCount++;
	if (movieMode != MOVIEMODE_TASEDITOR)
		currMovieData.rerecordCount = currRerecordCount;
}

void FCEUI_MovieToggleFrameDisplay(void)
{
	frame_display=!frame_display;
}

void FCEUI_MovieToggleRerecordDisplay()
{
	rerecord_display ^= 1;
}

void FCEUI_ToggleInputDisplay(void)
{
	switch(input_display)
	{
	case 0:
		input_display = 1;
		break;
	case 1:
		input_display = 2;
		break;
	case 2:
		input_display = 4;
		break;
	default:
		input_display = 0;
		break;
	}
}

int FCEUI_GetMovieLength()
{
	return currMovieData.records.size();
}

int FCEUI_GetMovieRerecordCount()
{
	return currMovieData.rerecordCount;
}

bool FCEUI_GetMovieToggleReadOnly()
{
	return movie_readonly;
}

void FCEUI_SetMovieToggleReadOnly(bool which)
{
	if (which)	//If set to readonly
	{
		if (!movie_readonly)	//If not already set
		{
			movie_readonly = true;
			FCEU_DispMessage("Movie is now Read-Only.",0);
		}
		else					//Else restate message
			FCEU_DispMessage("Movie is Read-Only.",0);
	}
	else		//If set to read+write
	{
		if (movie_readonly)		//If not already set
		{
			movie_readonly = false;
			FCEU_DispMessage("Movie is now Read+Write.",0);
		}
		else					//Else restate message
			FCEU_DispMessage("Movie is Read+Write.",0);
	}
}

//auqnull: What's the point to toggle Read-Only without a movie loaded?
void FCEUI_MovieToggleReadOnly()
{
	char message[260];

	movie_readonly = !movie_readonly;
	if (movie_readonly)
		strcpy(message, "Movie is now Read-Only");
	else
		strcpy(message, "Movie is now Read+Write");
	
	strcat(message, GetMovieModeStr());
	FCEU_DispMessage("%s",0,message);
}

void FCEUI_MovieToggleRecording()
{
	char message[260] = "";

	if (movieMode == MOVIEMODE_INACTIVE)
		strcpy(message, "Cannot toggle Recording");
	else if (currFrameCounter > (int)currMovieData.records.size())
	{
		movie_readonly = !movie_readonly;
		if (movie_readonly)
			strcpy(message, "Movie is now Read-Only (finished)");
		else
			strcpy(message, "Movie is now Read+Write (finished)");
	} else if (movieMode == MOVIEMODE_PLAY || (movieMode == MOVIEMODE_FINISHED && currFrameCounter == (int)currMovieData.records.size()))
	{
		strcpy(message, "Movie is now Read+Write");
		movie_readonly = false;
		FCEUMOV_IncrementRerecordCount();
		movieMode = MOVIEMODE_RECORD;
		RedumpWholeMovieFile(true);
	} else if (movieMode == MOVIEMODE_RECORD)
	{
		strcpy(message, "Movie is now Read-Only");
		movie_readonly = true;
		movieMode = MOVIEMODE_PLAY;
		RedumpWholeMovieFile(true);
		if (currFrameCounter >= (int)currMovieData.records.size())
		{
			extern int closeFinishedMovie;
			if (closeFinishedMovie)
			{
				movieMode = MOVIEMODE_INACTIVE;
				OnMovieClosed();
			} else
				movieMode = MOVIEMODE_FINISHED;
		}
	} else
		strcpy(message, "Nothing to do in this mode");

	strcat(message, GetMovieModeStr());

	FCEU_DispMessage("%s",0,message);
}

void FCEUI_MovieInsertFrame()
{
	char message[260] = "";

	if (movieMode == MOVIEMODE_INACTIVE)
		strcpy(message, "No movie to insert a frame.");
	else if (movie_readonly)
		strcpy(message, "Cannot modify movie in Read-Only mode.");
	else if (currFrameCounter > (int)currMovieData.records.size())
		strcpy(message, "Cannot insert a frame here.");
	else if (movieMode == MOVIEMODE_RECORD || movieMode == MOVIEMODE_PLAY || movieMode == MOVIEMODE_FINISHED)
	{
		strcpy(message, "1 frame inserted");
		strcat(message, GetMovieModeStr());
		std::vector<MovieRecord>::iterator iter = currMovieData.records.begin();
		currMovieData.records.insert(iter + currFrameCounter, MovieRecord());
		FCEUMOV_IncrementRerecordCount();
		RedumpWholeMovieFile();
	} else
	{
		strcpy(message, "Nothing to do in this mode");
		strcat(message, GetMovieModeStr());
	}

	FCEU_DispMessage("%s",0,message);
}

void FCEUI_MovieDeleteFrame()
{
	char message[260] = "";

	if (movieMode == MOVIEMODE_INACTIVE)
		strcpy(message, "No movie to delete a frame.");
	else if (movie_readonly)
		strcpy(message, "Cannot modify movie in Read-Only mode.");
	else if (currFrameCounter >= (int)currMovieData.records.size())
		strcpy(message, "Nothing to delete past movie end.");
	else if (movieMode == MOVIEMODE_RECORD || movieMode == MOVIEMODE_PLAY)
	{
		strcpy(message, "1 frame deleted");
		std::vector<MovieRecord>::iterator iter = currMovieData.records.begin();
		currMovieData.records.erase(iter + currFrameCounter);
		FCEUMOV_IncrementRerecordCount();
		RedumpWholeMovieFile();

		if (movieMode != MOVIEMODE_RECORD && currFrameCounter >= (int)currMovieData.records.size())
		{
			extern int closeFinishedMovie;
			if (closeFinishedMovie)
			{
				movieMode = MOVIEMODE_INACTIVE;
				OnMovieClosed();
			} else
				movieMode = MOVIEMODE_FINISHED;
		}
		strcat(message, GetMovieModeStr());
	} else
	{
		strcpy(message, "Nothing to do in this mode");
		strcat(message, GetMovieModeStr());
	}

	FCEU_DispMessage("%s",0,message);
}

void FCEUI_MovieTruncate()
{
	char message[260] = "";

	if (movieMode == MOVIEMODE_INACTIVE)
		strcpy(message, "No movie to truncate.");
	else if (movie_readonly)
		strcpy(message, "Cannot modify movie in Read-Only mode.");
	else if (currFrameCounter >= (int)currMovieData.records.size())
		strcpy(message, "Nothing to truncate past movie end.");
	else if (movieMode == MOVIEMODE_RECORD || movieMode == MOVIEMODE_PLAY)
	{
		strcpy(message, "Movie truncated");
		currMovieData.truncateAt(currFrameCounter);
		FCEUMOV_IncrementRerecordCount();
		RedumpWholeMovieFile();

		if (movieMode != MOVIEMODE_RECORD)
		{
			extern int closeFinishedMovie;
			if (closeFinishedMovie)
			{
				movieMode = MOVIEMODE_INACTIVE;
				OnMovieClosed();
			}
			else
				movieMode = MOVIEMODE_FINISHED;
		}
		strcat(message, GetMovieModeStr());
	} else
	{
		strcpy(message, "Nothing to do in this mode");
		strcat(message, GetMovieModeStr());
	}

	FCEU_DispMessage("%s",0,message);
}

void FCEUI_MovieNextRecordMode()
{
	movieRecordMode = (movieRecordMode + 1) % MOVIE_RECORD_MODE_MAX;
}

void FCEUI_MoviePrevRecordMode()
{
	movieRecordMode = (movieRecordMode + MOVIE_RECORD_MODE_MAX - 1) % MOVIE_RECORD_MODE_MAX;
}

void FCEUI_MovieRecordModeTruncate()
{
	movieRecordMode = MOVIE_RECORD_MODE_TRUNCATE;
}

void FCEUI_MovieRecordModeOverwrite()
{
	movieRecordMode = MOVIE_RECORD_MODE_OVERWRITE;
}

void FCEUI_MovieRecordModeInsert()
{
	movieRecordMode = MOVIE_RECORD_MODE_INSERT;
}

void FCEUI_MoviePlayFromBeginning(void)
{
	if (movieMode == MOVIEMODE_TASEDITOR)
	{
#ifdef __WIN_DRIVER__
		handleEmuCmdByTaseditor(EMUCMD_MOVIE_PLAY_FROM_BEGINNING);
#endif
	} else if (movieMode != MOVIEMODE_INACTIVE)
	{
		if (movieMode == MOVIEMODE_RECORD)
		{
			movieMode = MOVIEMODE_PLAY;
			RedumpWholeMovieFile(true);
		}
		if (currMovieData.savestate.empty())
		{
			movie_readonly = true;
			movieMode = MOVIEMODE_PLAY;
			cur_input_display = 0; //clear previous input display
			poweron(true);
			currFrameCounter = 0;
			FCEU_DispMessage("Movie is now Read-Only. Playing from beginning.",0);
		}
		else
		{
			// movie starting from savestate - reload movie file
			string str = curMovieFilename;
			FCEUI_StopMovie();
			if (FCEUI_LoadMovie(str.c_str(), 1, 0))
			{
				movieMode = MOVIEMODE_PLAY;
				movie_readonly = true;
				FCEU_DispMessage("Movie is now Read-Only. Playing from beginning.",0);
			}
			//currMovieData.loadSavestateFrom(&currMovieData.savestate); //TODO: make something like this work instead so it doesn't have to reload
		}
	}
#ifdef __WIN_DRIVER__
	SetMainWindowText();
#endif
}

std::string FCEUI_GetMovieName(void)
{
	return curMovieFilename;
}

bool FCEUI_MovieGetInfo(FCEUFILE* fp, MOVIE_INFO& info, bool skipFrameCount)
{
	MovieData md;
	if(!LoadFM2(md, fp->stream, fp->size, skipFrameCount))
		return false;

	info.movie_version = md.version;
	info.poweron = md.savestate.size()==0;
	info.reset = false; //Soft-reset isn't used from starting movies anymore, so this will be false, better for FCEUFILE to have that info (as |1| on the first frame indicates it
	info.pal = md.palFlag;
	info.ppuflag = md.PPUflag;
	info.RAMInitOption = md.RAMInitOption;
	info.RAMInitSeed = md.RAMInitSeed;
	info.nosynchack = true;
	info.num_frames = md.records.size();
	info.md5_of_rom_used = md.romChecksum;
	info.emu_version_used = md.emuVersion;
	info.name_of_rom_used = md.romFilename;
	info.rerecord_count = md.rerecordCount;
	info.comments = md.comments;
	info.subtitles = md.subtitles;

	return true;
}

//This function creates an array of frame numbers and corresponding strings for displaying subtitles
void LoadSubtitles(MovieData &moviedata)
{
	subtitleFrames.resize(0);
	subtitleMessages.resize(0);
	extern std::vector<string> subtitles;
	for(uint32 i=0; i < moviedata.subtitles.size() ; i++)
	{
		std::string& subtitle = moviedata.subtitles[i];
		size_t splitat = subtitle.find_first_of(' ');
		std::string key, value;

		//If we can't split them, then don't process this one
		if(splitat == std::string::npos)
		{
		}
		//Else split the subtitle into the int and string arrays
		else
		{
			key = subtitle.substr(0,splitat);
			value = subtitle.substr(splitat+1);
			subtitleFrames.push_back(atoi(key.c_str()));
			subtitleMessages.push_back(value);
		}
	}

}

//Every frame, this will be called to determine if a subtitle should be displayed, which one, and then to display it
void ProcessSubtitles(void)
{
	if (movieMode == MOVIEMODE_INACTIVE) return;

	for(uint32 i=0;i<currMovieData.subtitles.size();i++)
	{
		if (currFrameCounter == subtitleFrames[i])
			FCEU_DisplaySubtitles("%s",subtitleMessages[i].c_str());
	}
}

void FCEU_DisplaySubtitles(const char *format, ...)
{
	va_list ap;

	va_start(ap,format);
	vsnprintf(subtitleMessage.errmsg,sizeof(subtitleMessage.errmsg),format,ap);
	va_end(ap);

	subtitleMessage.howlong = 400;
	subtitleMessage.isMovieMessage = subtitlesOnAVI;
	subtitleMessage.linesFromBottom = 0;
}

void FCEUI_CreateMovieFile(std::string fn)
{
	MovieData md = currMovieData;							//Get current movie data
	EMUFILE* outf = FCEUD_UTF8_fstream(fn, "wb");		//open/create file
	md.dump(outf,false);									//dump movie data
	delete outf;											//clean up, delete file object
}

void FCEUI_MakeBackupMovie(bool dispMessage)
{
	//This function generates backup movie files
	std::string currentFn;					//Current movie fillename
	std::string backupFn;					//Target backup filename
	std::string tempFn;						//temp used in back filename creation
	stringstream stream;
	int x;								//Temp variable for string manip
	bool exist = false;					//Used to test if filename exists
	bool overflow = false;				//Used for special situation when backup numbering exceeds limit

	currentFn = curMovieFilename;		//Get current moviefilename
	backupFn = curMovieFilename;		//Make backup filename the same as current moviefilename
	x = backupFn.find_last_of(".");		 //Find file extension
	backupFn = backupFn.substr(0,x);	//Remove extension
	tempFn = backupFn;					//Store the filename at this point
	for (unsigned int backNum=0;backNum<999;backNum++) //999 = arbituary limit to backup files
	{
		stream.str("");					 //Clear stream
		if (backNum > 99)
			stream << "-" << backNum;	 //assign backNum to stream
		else if (backNum <=99 && backNum >= 10)
			stream << "-0";				//Make it 010, etc if two digits
		else
			stream << "-00" << backNum;	 //Make it 001, etc if single digit
		backupFn.append(stream.str());	 //add number to bak filename
		backupFn.append(".bak");		 //add extension

		exist = CheckFileExists(backupFn.c_str());	//Check if file exists

		if (!exist)
			break;						//Yeah yeah, I should use a do loop or something
		else
		{
			backupFn = tempFn;			//Before we loop again, reset the filename

			if (backNum == 999)			//If 999 exists, we have overflowed, let's handle that
			{
				backupFn.append("-001.bak"); //We are going to simply overwrite 001.bak
				overflow = true;		//Flag that we have exceeded limit
				break;					//Just in case
			}
		}
	}
	FCEUI_CreateMovieFile(backupFn);

	//TODO, decide if fstream successfully opened the file and print error message if it doesn't

	if (dispMessage)	//If we should inform the user
	{
		if (overflow)
			FCEUI_DispMessage("Backup overflow, overwriting %s",0,backupFn.c_str()); //Inform user of overflow
		else
			FCEUI_DispMessage("%s created",0,backupFn.c_str()); //Inform user of backup filename
	}
}

