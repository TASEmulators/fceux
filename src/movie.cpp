#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <zlib.h>
#include <iomanip>
#include <fstream>
#include <limits.h>
#include <stdarg.h>


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
#include "utils/memorystream.h"
#include "utils/xstring.h"
#include <sstream>

#ifdef CREATE_AVI
#include "drivers/videolog/nesvideos-piece.h"
#endif

#ifdef WIN32
#include <windows.h>
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

//todo - handle case where read+write is requested, but the file is actually readonly (could be confusing)

//todo - could we, given a field size, over-read from an inputstream and then parse out an integer?
//that would be faster than several reads, perhaps.

//sometimes we accidentally produce movie stop signals while we're trying to do other things with movies..
bool suppressMovieStop=false;

//----movie engine main state

EMOVIEMODE movieMode = MOVIEMODE_INACTIVE;

//this should not be set unless we are in MOVIEMODE_RECORD!
//FILE* fpRecordingMovie = 0;
std::ostream* osRecordingMovie = 0;

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
		if (turbo && pauseframe!=-1 && currFrameCounter<currMovieData.records.size())
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
		} else if (currFrameCounter > currMovieData.greenZoneCount && currMovieData.greenZoneCount<currMovieData.records.size())
		{
			/* May be required in some malformed TAS projects. */
			MovieData::dumpSavestateTo(&currMovieData.records[currFrameCounter].savestate,Z_DEFAULT_COMPRESSION);
			currMovieData.greenZoneCount= currFrameCounter+1;
		}
	}
}

void MovieRecord::clear()
{ 
	commands = 0;
	*(uint32*)&joysticks = 0;
	memset(zappers,0,sizeof(zappers));
}

const char MovieRecord::mnemonics[8] = {'A','B','S','T','U','D','L','R'};
void MovieRecord::dumpJoy(std::ostream* os, uint8 joystate)
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
			os->put(mnemonic);
		else //otherwise write an unset bit
			os->put('.');
	}
}

void MovieRecord::parseJoy(std::istream* is, uint8& joystate)
{
	char buf[8];
	is->read(buf,8);
	joystate = 0;
	for(int i=0;i<8;i++)
	{
		joystate <<= 1;
		joystate |= ((buf[i]=='.'||buf[i]==' ')?0:1);
	}
}

void MovieRecord::parse(MovieData* md, std::istream* is)
{
	//by the time we get in here, the initial pipe has already been extracted

	//extract the commands
	commands = uint32DecFromIstream(is);
	//*is >> commands;
	is->get(); //eat the pipe

	//a special case: if fourscore is enabled, parse four gamepads
	if(md->fourscore)
	{
		parseJoy(is,joysticks[0]); is->get(); //eat the pipe
		parseJoy(is,joysticks[1]); is->get(); //eat the pipe
		parseJoy(is,joysticks[2]); is->get(); //eat the pipe
		parseJoy(is,joysticks[3]); is->get(); //eat the pipe
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
			
			is->get(); //eat the pipe
		}
	}

	//(no fcexp data is logged right now)
	is->get(); //eat the pipe

	//should be left at a newline
}


bool MovieRecord::parseBinary(MovieData* md, std::istream* is)
{
	commands = (uint8)is->get();

	//check for eof
	if(is->gcount() != 1) return false;

	if(md->fourscore)
	{
		is->read((char*)&joysticks,4);
	}
	else
	{
		for(int port=0;port<2;port++)
		{
			if(md->ports[port] == SI_GAMEPAD)
				joysticks[port] = (uint8)is->get();
			else if(md->ports[port] == SI_ZAPPER)
			{
				zappers[port].x = (uint8)is->get();
				zappers[port].y = (uint8)is->get();
				zappers[port].b = (uint8)is->get();
				zappers[port].bogo = (uint8)is->get();
				read64le(&zappers[port].zaphit,is);
			}
		}
	}

	return true;
}


void MovieRecord::dumpBinary(MovieData* md, std::ostream* os, int index)
{
	os->put(commands);
	if(md->fourscore)
	{
		os->put(joysticks[0]);
		os->put(joysticks[1]);
		os->put(joysticks[2]);
		os->put(joysticks[3]);
	}
	else
	{
		for(int port=0;port<2;port++)
		{
			if(md->ports[port] == SI_GAMEPAD)
				os->put(joysticks[port]);
			else if(md->ports[port] == SI_ZAPPER)
			{
				os->put(zappers[port].x);
				os->put(zappers[port].y);
				os->put(zappers[port].b);
				os->put(zappers[port].bogo);
				write64le(zappers[port].zaphit, os);
			}
		}
	}
}

void MovieRecord::dump(MovieData* md, std::ostream* os, int index)
{
	//dump the misc commands
	//*os << '|' << setw(1) << (int)commands;
	os->put('|');
	putdec<uint8,1,true>(os,commands);

	//a special case: if fourscore is enabled, dump four gamepads
	if(md->fourscore)
	{
		os->put('|');
		dumpJoy(os,joysticks[0]); os->put('|');
		dumpJoy(os,joysticks[1]); os->put('|');
		dumpJoy(os,joysticks[2]); os->put('|');
		dumpJoy(os,joysticks[3]); os->put('|');
	}
	else
	{
		for(int port=0;port<2;port++)
		{
			os->put('|');
			if(md->ports[port] == SI_GAMEPAD)
				dumpJoy(os, joysticks[port]);
			else if(md->ports[port] == SI_ZAPPER)
			{
				putdec<uint8,3,true>(os,zappers[port].x); os->put(' ');
				putdec<uint8,3,true>(os,zappers[port].y); os->put(' ');
				putdec<uint8,1,true>(os,zappers[port].b); os->put(' ');
				putdec<uint8,1,true>(os,zappers[port].bogo); os->put(' ');
				putdec<uint64,20,false>(os,zappers[port].zaphit);
			}
		}
		os->put('|');
	}
	
	//(no fcexp data is logged right now)
	os->put('|');

	//each frame is on a new line
	os->put('\n');
}

MovieData::MovieData()
	: version(MOVIE_VERSION)
	, emuVersion(FCEU_VERSION_NUMERIC)
	, palFlag(false)
	, rerecordCount(0)
	, binaryFlag(false)
	, greenZoneCount(0)
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

int MovieData::dump(std::ostream *os, bool binary)
{
	int start = os->tellp();
	*os << "version " << version << endl;
	*os << "emuVersion " << emuVersion << endl;
	*os << "rerecordCount " << rerecordCount << endl;
	*os << "palFlag " << (palFlag?1:0) << endl;
	*os << "romFilename " << romFilename << endl;
	*os << "romChecksum " << BytesToString(romChecksum.data,MD5DATA::size) << endl;
	*os << "guid " << guid.toString() << endl;
	*os << "fourscore " << (fourscore?1:0) << endl;
	*os << "port0 " << ports[0] << endl;
	*os << "port1 " << ports[1] << endl;
	*os << "port2 " << ports[2] << endl;
	*os << "FDS " << isFDS << endl;

	for(uint32 i=0;i<comments.size();i++)
		*os << "comment " << wcstombs(comments[i]) << endl;

	for(uint32 i=0;i<subtitles.size();i++)
		*os << "subtitle " << subtitles[i] << endl;
	
	if(binary)
		*os << "binary 1" << endl;
		
	if(savestate.size() != 0)
		*os << "savestate " << BytesToString(&savestate[0],savestate.size()) << endl;

	if(FCEUMOV_Mode(MOVIEMODE_TASEDIT))
		*os << "length " << this->records.size() << endl;

	if(binary)
	{
		//put one | to start the binary dump
		os->put('|');
		for(int i=0;i<(int)records.size();i++)
			records[i].dumpBinary(this,os,i);
	}
	else
		for(int i=0;i<(int)records.size();i++)
			records[i].dump(this,os,i);

	int end = os->tellp();
	return end-start;
}

int MovieData::dumpGreenzone(std::ostream *os, bool binary)
{
	int start = os->tellp();
	int frame, size;
	for (int i=0; i<(int)records.size(); ++i)
	{
		if (records[i].savestate.empty())
			continue;
		frame=i;
		size=records[i].savestate.size();
		write32le(frame, os);
		write32le(size, os);

		os->write(&records[i].savestate[0], size);
	}
	frame=-1;
	size=currMovieData.greenZoneCount;
	write32le(frame, os);
	write32le(size, os);

	int end= os->tellp();

	return end-start;
}

int MovieData::loadGreenzone(std::istream *is, bool binary)
{
	int frame, size;
	while(1)
	{
		if (!read32le((uint32 *)&frame, is)) {size=0; break;}
		if (!read32le((uint32 *)&size, is)) {size=0; break;} 
		if (frame==-1) break;
		int pos = is->tellg();
		FCEUSS_LoadFP(is, SSLOADPARAM_NOBACKUP);
		is->seekg(pos+size);
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

static void LoadFM2_binarychunk(MovieData& movieData, std::istream* fp, int size)
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
	int curr = fp->tellg();
	fp->seekg(0,std::ios::end);
	int end = fp->tellg();
	int flen = end-curr;
	fp->seekg(curr,std::ios::beg);

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
bool LoadFM2(MovieData& movieData, std::istream* fp, int size, bool stopAfterHeader)
{
    std::string a("length"), b("-1");
	// Non-TAS projects consume until EOF
	movieData.installValue(a, b);

	//first, look for an fcm signature
	char fcmbuf[3];
	std::ios::pos_type curr = fp->tellg();
	fp->read(fcmbuf,3);
	fp->seekg(curr);
	if(!strncmp(fcmbuf,"FCM",3)) {
		FCEU_PrintError("FCM File format is no longer supported. Please use Tools > Convert FCM");
		return false;
	}

	//movie must start with "version 3"
	char buf[9];
	curr = fp->tellg();
	fp->read(buf,9);
	fp->seekg(curr);
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
		c = fp->get();
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
				int preparse = fp->tellg();
				movieData.records[currcount].parse(&movieData, fp);
				int postparse = fp->tellg();
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
	FCEU_DispMessage("Movie recording stopped.");

	movieMode = MOVIEMODE_INACTIVE;
	
	closeRecordingMovie();
}


void FCEUI_StopMovie()
{
	if(suppressMovieStop)
		return;
	
	if(movieMode == MOVIEMODE_PLAY)
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
	//FCEUD_SetInput(currMovieData.fourscore,(ESI)currMovieData.ports[0],(ESI)currMovieData.ports[1],(ESIFC)currMovieData.ports[2]);

	//pause the emulator
	FCEUI_SetEmulationPaused(1);

	//and enter tasedit mode
	movieMode = MOVIEMODE_TASEDIT;
	
	currMovieData.TryDumpIncremental();

	FCEU_DispMessage("Tasedit engaged");
}

void FCEUMOV_ExitTasEdit()
{
	movieMode = MOVIEMODE_INACTIVE;
	FCEU_DispMessage("Tasedit disengaged");
	currMovieData = MovieData();
}

bool FCEUMOV_FromPoweron()
{
	return movieFromPoweron;
}
bool MovieData::loadSavestateFrom(std::vector<char>* buf)
{
	memorystream ms(buf);
	return FCEUSS_LoadFP(&ms,SSLOADPARAM_BACKUP);
}

void MovieData::dumpSavestateTo(std::vector<char>* buf, int compressionLevel)
{
	memorystream ms(buf);
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
	if(movieMode == MOVIEMODE_PLAY)
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
	//Add to the recent movie menu
	AddRecentMovieFile(fname);
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
	FCEUD_SetInput(currMovieData.fourscore,(ESI)currMovieData.ports[0],(ESI)currMovieData.ports[1],(ESIFC)currMovieData.ports[2]);

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
			FCEU_DispMessage("Replay started Read-Only.");
		else
			FCEU_DispMessage("Replay started Read+Write.");
	}
	
	#ifdef CREATE_AVI
	if(LoggingEnabled)
	{
	    FCEU_DispMessage("Video recording enabled.\n");
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
	
	FCEU_DispMessage("Movie recording started.");
}

static int _currCommand = 0;

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
		if(currFrameCounter == currMovieData.records.size())
		{
			StopPlayback();
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
			FCEU_DispMessage("Paused at specified movie frame");
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

int FCEUMOV_WriteState(std::ostream* os)
{
	//we are supposed to dump the movie data into the savestate
	if(movieMode == MOVIEMODE_RECORD || movieMode == MOVIEMODE_PLAY)
		return currMovieData.dump(os, true);
	else return 0;
}

static bool load_successful;

bool FCEUMOV_ReadState(std::istream* is, uint32 size)
{
	load_successful = false;

	//a little rule: cant load states in read+write mode with a movie from an archive.
	//so we are going to switch it to readonly mode in that case
	if(!movie_readonly && FCEU_isFileInArchive(curMovieFilename)) {
		FCEU_PrintError("Cannot loadstate in Read+Write with movie from archive. Movie is now Read-Only.");
		movie_readonly = true;
	}

	MovieData tempMovieData = MovieData();
	std::ios::pos_type curr = is->tellg();
	if(!LoadFM2(tempMovieData, is, size, false)) {
		is->seekg((uint32)curr+size);
		extern bool FCEU_state_loading_old_format;
		if(FCEU_state_loading_old_format) {
			if(movieMode == MOVIEMODE_PLAY || movieMode == MOVIEMODE_RECORD) {
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
	//  the savestate is assumed to be in the same timeline as the current movie.
	//  if the current movie is not long enough to get to the savestate's frame#, then it is an error. 
	//  the movie contained in the savestate will be discarded.
	//  the emulator will be put into play mode.
	//if we are playing or recording and toggled read+write
	//  then, the movie we are playing must match the guid of the one stored in the savestate or else error.
	//  the movie contained in the savestate will be loaded into memory
	//  the frames in the movie after the savestate frame will be discarded
	//  the in-memory movie will have its rerecord count incremented
	//  the in-memory movie will be dumped to disk as fcm.
	//  the emulator will be put into record mode.
	//if we are doing neither:
	//  then, we must discard this movie and just load the savestate


	if(movieMode == MOVIEMODE_PLAY || movieMode == MOVIEMODE_RECORD)
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
			//if the frame counter is longer than our current movie, then error
			if(currFrameCounter > (int)currMovieData.records.size())
			{
				FCEU_PrintError("Savestate is from a frame (%d) after the final frame in the movie (%d). This is not permitted.", currFrameCounter, currMovieData.records.size()-1);
				return false;
			}
			movieMode = MOVIEMODE_PLAY;
		}
		else
		{
			//truncate before we copy, just to save some time
			tempMovieData.truncateAt(currFrameCounter);
			currMovieData = tempMovieData;
			
			#ifdef _S9XLUA_H
			if(!FCEU_LuaRerecordCountSkip())
				currRerecordCount++;
			#endif
			
			currMovieData.rerecordCount = currRerecordCount;

			openRecordingMovie(curMovieFilename);
			currMovieData.dump(osRecordingMovie, currMovieData.binaryFlag);
			movieMode = MOVIEMODE_RECORD;
		}
	}
	
	load_successful = true;

	//// Maximus: Show the last input combination entered from the
	//// movie within the state
	//if(current!=0) // <- mz: only if playing or recording a movie
	//	memcpy(&cur_input_display, joop, 4);

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
			FCEU_DispMessage("Movie is now Read-Only.");
		}
		else					//Else restate message
			FCEU_DispMessage("Movie is Read-Only.");
	}
	else		//If set to read+write
	{
		if (movie_readonly)		//If not already set
		{
			movie_readonly = false;
			FCEU_DispMessage("Movie is now Read+Write.");
		}
		else					//Else restate message
			FCEU_DispMessage("Movie is Read+Write.");
	}
}
void FCEUI_MovieToggleReadOnly()
{
	if(movie_readonly)
		FCEU_DispMessage("Movie is now Read+Write.");
	else
		FCEU_DispMessage("Movie is now Read-Only.");
	
	movie_readonly = !movie_readonly;
}

void FCEUI_MoviePlayFromBeginning(void)
{
	if (movieMode != MOVIEMODE_INACTIVE)
	{
		char *fname = strdup(curMovieFilename);
		FCEUI_LoadMovie(fname, true, false, 0);
		FCEU_DispMessage("Movie is now Read-Only. Playing from beginning.");
		free(fname);
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
}

void FCEUI_CreateMovieFile(std::string fn)
{
	MovieData md = currMovieData;							//Get current movie data
	std::fstream* outf = FCEUD_UTF8_fstream(fn, "wb");		//open/create file
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
			FCEUI_DispMessage("Backup overflow, overwriting %s",backupFn.c_str()); //Inform user of overflow
		else
			FCEUI_DispMessage("%s created",backupFn.c_str()); //Inform user of backup filename
	}
}

bool CheckFileExists(const char* filename)
{
	//This function simply checks to see if the given filename exists
	string checkFilename; 

	if (filename)
		checkFilename = filename;
			
	//Check if this filename exists
	fstream test;
	test.open(checkFilename.c_str(),fstream::in);
		
	if (test.fail())
	{
		test.close();
		return false; 
	}
	else
	{
		test.close();
		return true; 
	}
}
