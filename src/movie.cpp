#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <zlib.h>
#include <iomanip>
#include <fstream>
#include <limits.h>
#include <stdarg.h>

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
#include "fds.h"
#ifdef _S9XLUA_H
#include "fceulua.h"
#endif
#include "utils/guid.h"
#include "utils/memory.h"
#include "utils/xstring.h"
#include <sstream>

#ifdef CREATE_AVI
#include "drivers/videolog/nesvideos-piece.h"
#endif

#ifdef WIN32
#include <windows.h>
#include "./drivers/win/common.h"
extern void AddRecentMovieFile(const char *filename);
#endif

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

SFORMAT FCEUMOV_STATEINFO[]={
	{ &currFrameCounter, 4|FCEUSTATE_RLSB, "FCNT"},
	{ 0 }
};

char curMovieFilename[512] = {0};
MovieData currMovieData;
MovieData defaultMovieData;
int currRerecordCount;

void MovieData::clearRecordRange(int start, int len)
{
	for(int i=0;i<len;i++)
		records[i+start].clear();
}

void MovieData::insertEmpty(int at, int frames)
{
	if(at == -1) 
	{
		int currcount = records.size();
		records.resize(records.size()+frames);
		clearRecordRange(currcount,frames);
	}
	else
	{
		records.insert(records.begin()+at,frames,MovieRecord());
		clearRecordRange(at,frames);
	}
}

void MovieData::TryDumpIncremental()
{
	if(movieMode == MOVIEMODE_TASEDIT)
	{
		//only log the savestate if we are appending to the green zone
		if (turbo && pauseframe!=-1 && static_cast<unsigned int>(currFrameCounter)<currMovieData.records.size())
		{
			if (turbo && pauseframe-256>currFrameCounter && ((currFrameCounter-pauseframe)&0xff))
				return;
			MovieData::dumpSavestateTo(&currMovieData.records[currFrameCounter].savestate,Z_DEFAULT_COMPRESSION);
		}
		if(currFrameCounter == currMovieData.greenZoneCount)
		{
			if(currFrameCounter == (int)currMovieData.records.size() || currMovieData.records.size()==0)
			{
				currMovieData.insertEmpty(-1,1);
			}
			
			MovieData::dumpSavestateTo(&currMovieData.records[currFrameCounter].savestate,Z_DEFAULT_COMPRESSION);
			currMovieData.greenZoneCount++;
		} else if (currFrameCounter < currMovieData.greenZoneCount || !movie_readonly)
		{
			MovieData::dumpSavestateTo(&currMovieData.records[currFrameCounter].savestate,Z_DEFAULT_COMPRESSION);
		} else if (currFrameCounter > currMovieData.greenZoneCount && static_cast<unsigned int>(currMovieData.greenZoneCount)<currMovieData.records.size())
		{
			/* May be required in some malformed TAS projects. */
			MovieData::dumpSavestateTo(&currMovieData.records[currFrameCounter].savestate,Z_DEFAULT_COMPRESSION);
			currMovieData.greenZoneCount= currFrameCounter+1;
		}
	}
}

MovieRecord::MovieRecord()
{
	joysticks.data[0] = 0;
	joysticks.data[1] = 0;
	joysticks.data[2] = 0;
	joysticks.data[3] = 0;

	commands = 0;

	zappers[0].b = 0;
	zappers[0].bogo = 0;
	zappers[0].x = 0;
	zappers[0].y = 0;
	zappers[0].zaphit = 0;

	zappers[1].b = 0;
	zappers[1].bogo = 0;
	zappers[1].x = 0;
	zappers[1].y = 0;
	zappers[1].zaphit = 0;
}

void MovieRecord::clear()
{ 
	commands = 0;
	*(uint32*)&joysticks = 0;
	memset(zappers,0,sizeof(zappers));
}

bool MovieRecord::Compare(MovieRecord& compareRec)
{
	//Joysticks, Zappers, and commands

	if (this->joysticks != compareRec.joysticks) 
		return false;
	
	//if (this->commands != compareRec.commands) 
	//	return false;

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


	return true;
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
	if (false/*currMovieData.binaryFlag*/)
	{
		dumpBinary(md, os, index);
		return;
	}
	//dump the misc commands
	//*os << '|' << setw(1) << (int)commands;
	os->fputc('|');
	putdec<uint8,1,true>(os,commands);

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
	, palFlag(false)
	, PPUflag(false)
	, rerecordCount(0)
	, binaryFlag(false)
	, greenZoneCount(0)
	, microphone(false)
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
	else if (key == "length")
	{
		installInt(val, loadFrameCount);
	}
}

int MovieData::dump(EMUFILE *os, bool binary)
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
	os->fprintf("FDS %d\n" , isFDS?1:0 );
	os->fprintf("NewPPU %d\n" , newppu?1:0 );

	for(uint32 i=0;i<comments.size();i++)
		os->fprintf("comment %s\n" , wcstombs(comments[i]).c_str() );

	for(uint32 i=0;i<subtitles.size();i++)
		os->fprintf("subtitle %s\n" , subtitles[i].c_str() );
	
	if(binary)
		os->fprintf("binary 1\n" );
		
	if(savestate.size() != 0)
		os->fprintf("savestate %s\n" , BytesToString(&savestate[0],savestate.size()).c_str() );

	if(FCEUMOV_Mode(MOVIEMODE_TASEDIT))
		os->fprintf("length %d\n" , this->records.size() );

	if(binary)
	{
		//put one | to start the binary dump
		os->fputc('|');
		for(int i=0;i<(int)records.size();i++)
			records[i].dumpBinary(this,os,i);
	}
	else
		for(int i=0;i<(int)records.size();i++)
			records[i].dump(this,os,i);

	int end = os->ftell();
	return end-start;
}

int MovieData::dumpGreenzone(EMUFILE *os, bool binary)
{
	int start = os->ftell();
	int frame, size;
	for (int i=0; i<(int)records.size(); ++i)
	{
		if (records[i].savestate.empty())
			continue;
		frame=i;
		size=records[i].savestate.size();
		write32le(frame, os);
		write32le(size, os);

		os->fwrite(&records[i].savestate[0], size);
	}
	frame=-1;
	size=currMovieData.greenZoneCount;
	write32le(frame, os);
	write32le(size, os);

	int end= os->ftell();

	return end-start;
}

int MovieData::loadGreenzone(EMUFILE *is, bool binary)
{
	int frame, size;
	while(1)
	{
		if (!read32le((uint32 *)&frame, is)) {size=0; break;}
		if (!read32le((uint32 *)&size, is)) {size=0; break;} 
		if (frame==-1) break;
		int pos = is->ftell();
		FCEUSS_LoadFP(is, SSLOADPARAM_NOBACKUP);
		is->fseek(pos+size,SEEK_SET);
	}
	greenZoneCount=size;

	return 1;
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
	if (lagFlag) return true;
	else return false;
}

bool FCEUMOV_ShouldPause(void)
{
	if(pauseframe && currFrameCounter == (pauseframe-1))	//adelikat: changed to pauseframe -1 to prevent an off by 1 error.  THis is probably the hackiest solution but I think it would cause some major restructuring to fix it properly.
	{
		pauseframe = 0; //only pause once!
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
	int todo = std::min(size, flen);

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
    std::string a("length"), b("-1");
	// Non-TAS projects consume until EOF
	movieData.installValue(a, b);

	//first, look for an fcm signature
	char fcmbuf[3];
	std::ios::pos_type curr = fp->ftell();
	fp->fread(fcmbuf,3);
	fp->fseek(curr,SEEK_SET);
	if(!strncmp(fcmbuf,"FCM",3)) {
		FCEU_PrintError("FCM File format is no longer supported. Please use Tools > Convert FCM");
		return false;
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
	for(;;)
	{
		bool iswhitespace, isrecchar, isnewline;
		int c;
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
		}
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

static void closeRecordingMovie()
{
	if(osRecordingMovie)
	{
		delete osRecordingMovie;
		osRecordingMovie = 0;
	}
}

/// Stop movie playback.
static void StopPlayback()
{
	FCEU_DispMessageOnMovie("Movie playback stopped.");
	movieMode = MOVIEMODE_INACTIVE;
}

/// Stop movie recording
static void StopRecording()
{
	FCEU_DispMessage("Movie recording stopped.",0);

	movieMode = MOVIEMODE_INACTIVE;
	
	closeRecordingMovie();
}


void FCEUI_StopMovie()
{
	if(suppressMovieStop)
		return;
	
	if(movieMode == MOVIEMODE_PLAY || movieMode == MOVIEMODE_FINISHED)
		StopPlayback();
	else if(movieMode == MOVIEMODE_RECORD)
		StopRecording();

	curMovieFilename[0] = 0;			//No longer a current movie filename
	freshMovie = false;					//No longer a fresh movie loaded
	if (bindSavestate) AutoSS = false;	//If bind movies to savestates is true, then there is no longer a valid auto-save to load
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
	disableBatteryLoading = 1;
	PowerNES();
	disableBatteryLoading = 0;
}



void FCEUMOV_EnterTasEdit()
{
	if (movieMode == MOVIEMODE_INACTIVE)
	{
		//stop any current movie activity
		FCEUI_StopMovie();

		//clear the current movie
		currFrameCounter = 0;
		currMovieData = MovieData();
		currMovieData.guid.newGuid();
		currMovieData.palFlag = FCEUI_GetCurrentVidSystem(0,0)!=0;
		currMovieData.romChecksum = GameInfo->MD5;
		currMovieData.romFilename = FileBase;

		//reset the rom
		poweron(false);
	} else {
		FCEUI_StopMovie();

		currMovieData.greenZoneCount=currFrameCounter;
	}

	//todo - think about this
	//ResetInputTypes();
	//todo - maybe this instead
	//FCEUD_SetInput(currMovieData.fourscore,currMovieData.microphone,(ESI)currMovieData.ports[0],(ESI)currMovieData.ports[1],(ESIFC)currMovieData.ports[2]);

	//pause the emulator
	FCEUI_SetEmulationPaused(1);

	//and enter tasedit mode
	movieMode = MOVIEMODE_TASEDIT;
	
	currMovieData.TryDumpIncremental();

	FCEU_DispMessage("Tasedit engaged",0);
}

void FCEUMOV_ExitTasEdit()
{
	movieMode = MOVIEMODE_INACTIVE;
	FCEU_DispMessage("Tasedit disengaged",0);
	currMovieData = MovieData();
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

//begin playing an existing movie
bool FCEUI_LoadMovie(const char *fname, bool _read_only, bool tasedit, int _pauseframe)
{
	if(!tasedit && !FCEU_IsValidUI(FCEUI_PLAYMOVIE))
		return true;	//adelikat: file did not fail to load, so let's return true here, just do nothing

	assert(fname);

	//mbg 6/10/08 - we used to call StopMovie here, but that cleared curMovieFilename and gave us crashes...
	if(movieMode == MOVIEMODE_PLAY || movieMode == MOVIEMODE_FINISHED)
		StopPlayback();
	else if(movieMode == MOVIEMODE_RECORD)
		StopRecording();
	//--------------

	currMovieData = MovieData();
	
	strcpy(curMovieFilename, fname);
	FCEUFILE *fp = FCEU_fopen(fname,0,"rb",0);
	if (!fp) return false;
	if(fp->isArchive() && !_read_only) {
		FCEU_PrintError("Cannot open a movie in read+write from an archive.");
		return true;	//adelikat: file did not fail to load, so return true (false is only for file not exist/unable to open errors
	}

#ifdef WIN32
	//Fix relative path if necessary and then add to the recent movie menu
	extern std::string BaseDirectory;

	string name = fname;
	if (IsRelativePath(fname))
	{
		name = ConvertRelativePath(name);
	}
	AddRecentMovieFile(name.c_str());
#endif

	LoadFM2(currMovieData, fp->stream, INT_MAX, false);
	LoadSubtitles(currMovieData);
	delete fp;

	freshMovie = true;	//Movie has been loaded, so it must be unaltered
	if (bindSavestate) AutoSS = false;	//If bind savestate to movie is true, then their isn't a valid auto-save to load, so flag it
	//fully reload the game to reinitialize everything before playing any movie
	poweron(true);

	//WE NEED TO LOAD A SAVESTATE
	if(currMovieData.savestate.size() != 0)
	{
		movieFromPoweron = false;
		bool success = MovieData::loadSavestateFrom(&currMovieData.savestate);
		if(!success) return true;	//adelikat: I guess return true here?  False is only for a bad movie filename, if it got this far the file was god?
	} else {
		movieFromPoweron = true;
	}

	//if there is no savestate, we won't have this crucial piece of information at the start of the movie.
	//so, we have to include it with the movie
	if(currMovieData.palFlag)
		FCEUI_SetVidSystem(1);
	else
		FCEUI_SetVidSystem(0);

	//force the input configuration stored in the movie to apply
	FCEUD_SetInput(currMovieData.fourscore,currMovieData.microphone,(ESI)currMovieData.ports[0],(ESI)currMovieData.ports[1],(ESIFC)currMovieData.ports[2]);

	//stuff that should only happen when we're ready to positively commit to the replay
	if(tasedit)
	{
		currFrameCounter = 0;
		pauseframe = _pauseframe;

		currMovieData.TryDumpIncremental();
	}
	else
	{
		currFrameCounter = 0;
		pauseframe = _pauseframe;
		movie_readonly = _read_only;
		movieMode = MOVIEMODE_PLAY;
		currRerecordCount = currMovieData.rerecordCount;

		if(movie_readonly)
			FCEU_DispMessage("Replay started Read-Only.",0);
		else
			FCEU_DispMessage("Replay started Read+Write.",0);
	}
	
	#ifdef CREATE_AVI
	if(LoggingEnabled)
	{
	    FCEU_DispMessage("Video recording enabled.\n",0);
	    LoggingEnabled = 2;
	}
	#endif
	
	return true;
}

static void openRecordingMovie(const char* fname)
{
	osRecordingMovie = FCEUD_UTF8_fstream(fname, "wb");
	if(!osRecordingMovie)
		FCEU_PrintError("Error opening movie output file: %s",fname);
	strcpy(curMovieFilename, fname);
#ifdef WIN32
	//Add to the recent movie menu
	AddRecentMovieFile(fname);
#endif
}


//begin recording a new movie
//TODO - BUG - the record-from-another-savestate doesnt work.
void FCEUI_SaveMovie(const char *fname, EMOVIE_FLAG flags, std::wstring author)
{
	if(!FCEU_IsValidUI(FCEUI_RECORDMOVIE))
		return;

	assert(fname);

	FCEUI_StopMovie();

	openRecordingMovie(fname);

	currFrameCounter = 0;
	LagCounterReset();

	currMovieData = MovieData();
	currMovieData.guid.newGuid();

	if(author != L"") currMovieData.comments.push_back(L"author " + author);
	currMovieData.palFlag = FCEUI_GetCurrentVidSystem(0,0)!=0;
	currMovieData.romChecksum = GameInfo->MD5;
	currMovieData.romFilename = FileBase;
	currMovieData.fourscore = FCEUI_GetInputFourscore();
	currMovieData.microphone = FCEUI_GetInputMicrophone();
	currMovieData.ports[0] = joyports[0].type;
	currMovieData.ports[1] = joyports[1].type;
	currMovieData.ports[2] = portFC.type;

	if(flags & MOVIE_FLAG_FROM_POWERON)
	{
		movieFromPoweron = true;
		poweron(true);
	}
	else
	{
		movieFromPoweron = false;
		MovieData::dumpSavestateTo(&currMovieData.savestate,Z_BEST_COMPRESSION);
	}

	//we are going to go ahead and dump the header. from now on we will only be appending frames
	currMovieData.dump(osRecordingMovie, false);

	movieMode = MOVIEMODE_RECORD;
	movie_readonly = false;
	currRerecordCount = 0;
	
	FCEU_DispMessage("Movie recording started.",0);
}

static int _currCommand = 0;


// Stop movie playback without closing the movie.
static void FinishPlayback()
{
	FCEU_DispMessage("Movie finished playing.",0);
	movieMode = MOVIEMODE_FINISHED;
}

//the main interaction point between the emulator and the movie system.
//either dumps the current joystick state or loads one state from the movie
void FCEUMOV_AddInputState()
{
	//todo - for tasedit, either dump or load depending on whether input recording is enabled
	//or something like that
	//(input recording is just like standard read+write movie recording with input taken from gamepad)
	//otherwise, it will come from the tasedit data.

	if(movieMode == MOVIEMODE_TASEDIT)
	{
		MovieRecord* mr = &currMovieData.records[currFrameCounter];
		if(movie_readonly)
		{
			//reset if necessary
			if(mr->command_reset())
				ResetNES();

			joyports[0].load(mr);
			joyports[1].load(mr);
		}
		else
		{
			joyports[0].log(mr);
			joyports[1].log(mr);
			mr->commands = 0;
		}
	}
	else if(movieMode == MOVIEMODE_PLAY)
	{
		//stop when we run out of frames
		if(currFrameCounter >= (int)currMovieData.records.size())
		{
			FinishPlayback();
		}
		else
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

			joyports[0].load(mr);
			joyports[1].load(mr);
		}

		//if we are on the last frame, then pause the emulator if the player requested it
		if(currFrameCounter == currMovieData.records.size()-1)
		{
			if(FCEUD_PauseAfterPlayback())
			{
				FCEUI_ToggleEmulationPause();
			}
		}

		//pause the movie at a specified frame 
		if(FCEUMOV_ShouldPause() && FCEUI_EmulationPaused()==0)
		{
			FCEUI_ToggleEmulationPause();
			FCEU_DispMessage("Paused at specified movie frame",0);
		}
		
	}
	else if(movieMode == MOVIEMODE_RECORD)
	{
		MovieRecord mr;

		joyports[0].log(&mr);
		joyports[1].log(&mr);
		mr.commands = _currCommand;
		_currCommand = 0;

		mr.dump(&currMovieData, osRecordingMovie,currMovieData.records.size());
		currMovieData.records.push_back(mr);
	}

	currFrameCounter++;

	extern uint8 joy[4];
	memcpy(&cur_input_display,joy,4);
}


//TODO 
void FCEUMOV_AddCommand(int cmd)
{
	// do nothing if not recording a movie
	if(movieMode != MOVIEMODE_RECORD)
		return;

	//NOTE: EMOVIECMD matches FCEUNPCMD_RESET and FCEUNPCMD_POWER
	//we are lucky (well, I planned it that way)

	switch(cmd) {
		case FCEUNPCMD_FDSINSERT: cmd = MOVIECMD_FDS_INSERT; break;
		case FCEUNPCMD_FDSSELECT: cmd = MOVIECMD_FDS_SELECT; break;
	}

	_currCommand |= cmd;
}

void FCEU_DrawMovies(uint8 *XBuf)
{
	if(frame_display && movieMode != MOVIEMODE_TASEDIT)
	{
		char counterbuf[32] = {0};

		if(movieMode == MOVIEMODE_PLAY)
			sprintf(counterbuf,"%d/%d",currFrameCounter,currMovieData.records.size());
		else if(movieMode == MOVIEMODE_RECORD) 
			sprintf(counterbuf,"%d",currMovieData.records.size());
		else if (movieMode == MOVIEMODE_FINISHED)
			sprintf(counterbuf,"%d/%d (finished)",currFrameCounter,currMovieData.records.size());
		else
			sprintf(counterbuf,"%d (no movie)",currFrameCounter);

		if(counterbuf[0])
			DrawTextTrans(ClipSidesOffset+XBuf+FCEU_TextScanlineOffsetFromBottom(30)+1, 256, (uint8*)counterbuf, 0x20+0x80);
	}
}

void FCEU_DrawLagCounter(uint8 *XBuf)
{
	uint8 color;
	
	if (lagFlag) color = 0x16+0x80; //If currently lagging display red
	else color = 0x2A+0x80;         //else display green

	if(lagCounterDisplay)
	{
		char counterbuf[32] = {0};	
		sprintf(counterbuf,"%d",lagCounter);
		
		if(counterbuf[0])
			DrawTextTrans(ClipSidesOffset+XBuf+FCEU_TextScanlineOffsetFromBottom(40)+1, 256, (uint8*)counterbuf, color); //0x20+0x80
	}
}

int FCEUMOV_WriteState(EMUFILE* os)
{
	//we are supposed to dump the movie data into the savestate
	if(movieMode == MOVIEMODE_RECORD || movieMode == MOVIEMODE_PLAY || movieMode == MOVIEMODE_FINISHED)
		return currMovieData.dump(os, true);
	else return 0;
}

bool CheckTimelines(MovieData& stateMovie, MovieData& currMovie, int& errorFr)
{
	bool isInTimeline = true;
	int length;

	//First check, make sure we are checking is for a post-movie savestate, we just want to adjust the length for now, we will handle this situation later in another function
	if (currFrameCounter <= stateMovie.getNumRecords())
		length = currFrameCounter;
	else
		length = stateMovie.getNumRecords();

	//Now that we know the length of the records of the savestate we plan to load, let's match the length against the movie
	//If length < currMovie records then this is a "future" event from the current movie, againt we will handle this situation later, we just want to find the right number of frames to compare
	if (length > currMovie.getNumRecords())
		length = currMovie.getNumRecords();

	for (int x = 0; x < length; x++)
	{
		if (!stateMovie.records[x].Compare(currMovie.records[x]))
		{
			isInTimeline = false;
			errorFr = x;
			break;
		}
	}

	return isInTimeline;
}


static bool load_successful;

bool FCEUMOV_ReadState(EMUFILE* is, uint32 size)
{
	load_successful = false;

	//a little rule: cant load states in read+write mode with a movie from an archive.
	//so we are going to switch it to readonly mode in that case
	if(!movie_readonly && FCEU_isFileInArchive(curMovieFilename)) {
		FCEU_PrintError("Cannot loadstate in Read+Write with movie from archive. Movie is now Read-Only.");
		movie_readonly = true;
	}

	MovieData tempMovieData = MovieData();
	std::ios::pos_type curr = is->ftell();
	if(!LoadFM2(tempMovieData, is, size, false)) {
		is->fseek((uint32)curr+size,SEEK_SET);
		extern bool FCEU_state_loading_old_format;
		if(FCEU_state_loading_old_format) {
			if(movieMode == MOVIEMODE_PLAY || movieMode == MOVIEMODE_RECORD || movieMode == MOVIEMODE_FINISHED) {
				FCEUI_StopMovie();			
				FCEU_PrintError("You have tried to use an old savestate while playing a movie. This is unsupported (since the old savestate has old-format movie data in it which can't be converted on the fly)");
			}
		}
		return false;
	}

	//complex TAS logic for when a savestate is loaded:
	//----------------
	//if we are playing or recording and toggled read-only:
	//  then, the movie we are playing must match the guid of the one stored in the savestate or else error.
	//  ---the savestate is assumed to be in the same timeline as the current movie.---
	//		adelikat: correction: the savestate timeline should not be assumed, because a read-only desync is still potentially destructive
	//			playing from a desynced state, making a new savestate, and loading it in write mode will destroy a movie, and it isn't always obvious that there is a desync
	//  if the current movie is not long enough to get to the savestate's frame#, then it is an error. 
	//  the movie contained in the savestate will be discarded.
	//  the emulator will be put into play mode.
	//if we are playing or recording and toggled read+write
	//  then, the movie we are playing must match the guid of the one stored in the savestate or else error, give the option to load it anyway.
	//  the movie contained in the savestate will be loaded into memory
	//  the frames in the movie after the savestate frame will be discarded
	//  the in-memory movie will have its rerecord count incremented
	//  the in-memory movie will be dumped to disk as fcm.
	//  the emulator will be put into record mode.
	//if we are doing neither:
	//  then, we must discard this movie and just load the savestate


	if(movieMode == MOVIEMODE_PLAY || movieMode == MOVIEMODE_RECORD || movieMode == MOVIEMODE_FINISHED)
	{
		//handle moviefile mismatch
		if(tempMovieData.guid != currMovieData.guid)
		{
			//mbg 8/18/08 - this code  can be used to turn the error message into an OK/CANCEL
			#ifdef WIN32
				std::string msg = "There is a mismatch between savestate's movie and current movie.\ncurrent: " + currMovieData.guid.toString() + "\nsavestate: " + tempMovieData.guid.toString() + "\n\nThis means that you have loaded a savestate belonging to a different movie than the one you are playing now.\n\nContinue loading this savestate anyway?";
				extern HWND pwindow;
				int result = MessageBox(pwindow,msg.c_str(),"Error loading savestate",MB_OKCANCEL);
				if(result == IDCANCEL)
					return false;
			#else
				FCEU_PrintError("Mismatch between savestate's movie and current movie.\ncurrent: %s\nsavestate: %s\n",currMovieData.guid.toString().c_str(),tempMovieData.guid.toString().c_str());
				return false;
			#endif
		}

		closeRecordingMovie();

		if(movie_readonly)
		{
			int errorFrame = 0;
			bool sameTimeline =  CheckTimelines(tempMovieData, currMovieData, errorFrame);

			if (sameTimeline)
			{
				//if we made it this far, then the savestate has identical movie data but we want to know now if the stateMOVIE size is greater than current movie size and make this error
				
				//currFrameCounter at this point represents the savestate framecount
				if(currFrameCounter > (int)currMovieData.records.size()) 
				{
					//TODO: turn frame counter to red to get attention
					FCEU_PrintError("Savestate is from a frame (%d) after the final frame in the movie (%d). This is not permitted.", tempMovieData.records.size(), currMovieData.records.size()-1);
					return false;
				}
				else
				{
					//Final test, if the savestate frame count is > savestate movie length, this was a post movie savestate
					//currFrameCounter is currently savestate frame counter (not savestate movie size
					if (currFrameCounter > (int)tempMovieData.records.size())
					{
						FinishPlayback();
					}
					else
					{
						//Finally, this is a savestate file for this movie
						movieMode = MOVIEMODE_PLAY;
					}

				}
				
			}
			else
			{
				//Wrong timeline, do apprioriate logic here
				FCEU_PrintError("Error: Savestate not in the same timeline as movie!\nFrame %d branches from current timeline", errorFrame);
				return false;
			}
		}
		else //Read + write
		{
			if (currFrameCounter > (int)tempMovieData.records.size())
			{
				//This is a post movie savestate, handle it differently
				//Recplae movie contents but then switch to movie finished mode
				currMovieData = tempMovieData;	
				openRecordingMovie(curMovieFilename);
				currMovieData.dump(osRecordingMovie, false/*currMovieData.binaryFlag*/);
				FinishPlayback();
			}
			else
			{
				//truncate before we copy, just to save some time
				tempMovieData.truncateAt(currFrameCounter); //we can only assume this here since we have checked that the frame counter is not greater than the movie data
				currMovieData = tempMovieData;				
#ifdef _S9XLUA_H
				if(!FCEU_LuaRerecordCountSkip())
					currRerecordCount++;
#else
				currRerecordCount++;
#endif
				currMovieData.rerecordCount = currRerecordCount;
				openRecordingMovie(curMovieFilename);
				currMovieData.dump(osRecordingMovie, false/*currMovieData.binaryFlag*/);
				movieMode = MOVIEMODE_RECORD;

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
	if(movieMode == MOVIEMODE_INACTIVE || movieMode == MOVIEMODE_TASEDIT)
		return true;
	else
		return load_successful;
}

void FCEUI_MovieToggleFrameDisplay(void)
{
	frame_display=!frame_display;
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
void FCEUI_MovieToggleReadOnly()
{
	char message[260];
	
	if(movie_readonly)
		strcpy(message, "Movie is now Read+Write");
	else
	{
		strcpy(message, "Movie is now Read-Only");
	}
	
	if(movieMode == MOVIEMODE_INACTIVE)
		strcat(message, " (no movie)");
	else if (movieMode == MOVIEMODE_FINISHED)
		strcat(message, " (finished)");
	
	FCEU_DispMessage(message,0);
	movie_readonly = !movie_readonly;
}

void FCEUI_MoviePlayFromBeginning(void)
{
	if (movieMode != MOVIEMODE_INACTIVE && movieMode != MOVIEMODE_TASEDIT)
	{
		movie_readonly=true;
		poweron(true);
		
		currFrameCounter=0;
		movieMode = MOVIEMODE_PLAY;

		FCEU_DispMessage("Movie is now Read-Only. Playing from beginning.",0);
	}
}

string FCEUI_GetMovieName(void)
{
	return curMovieFilename;
}

bool FCEUI_MovieGetInfo(FCEUFILE* fp, MOVIE_INFO& info, bool skipFrameCount)
{
	MovieData md;
	if(!LoadFM2(md, fp->stream, INT_MAX, skipFrameCount))
		return false;
	
	info.movie_version = md.version;
	info.poweron = md.savestate.size()==0;
	info.pal = md.palFlag;
	info.ppuflag = md.PPUflag;
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

void FCEU_DisplaySubtitles(char *format, ...)
{
	va_list ap;

	va_start(ap,format);
	vsnprintf(subtitleMessage.errmsg,sizeof(subtitleMessage.errmsg),format,ap);
	va_end(ap);

	subtitleMessage.howlong = 300;
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
	string currentFn;					//Current movie fillename
	string backupFn;					//Target backup filename
	string tempFn;						//temp used in back filename creation
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

