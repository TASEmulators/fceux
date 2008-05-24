#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <vector>
#include <map>

#ifdef MSVC
#include <windows.h>
#endif

#include "types.h"
#include "utils/endian.h"
#include "palette.h"
#include "input.h"
#include "fceu.h"
#include "driver.h"
#include "state.h"
#include "file.h"
#include "video.h"
#include "movie.h"
#include "utils/memory.h"
#include "utils/xstring.h"


#define MOVIE_MAGIC             0x1a4d4346      // FCM\x1a
#define MOVIE_VERSION           3 // still at 2 since the format itself is still compatible - to detect which version the movie was made with, check the fceu version stored in the movie header (e.g against FCEU_VERSION_NUMERIC)

#define MOVIE_FLAG_NOSYNCHACK          (1<<4) // set in newer version, used for old movie compatibility

// backwards compat
static void FCEUI_LoadMovie_v1(char *fname, int _read_only);
static int FCEUI_MovieGetInfo_v1(const char* fname, MOVIE_INFO* info);

extern char FileBase[];

//TODO - remove the synchack stuff from the replay gui and require it to be put into the fm2 file
//which the user would have already converted from fcm
//also cleanup the whole emulator version bullshit in replay. we dont support that old stuff anymore

//todo - better error handling for the compressed savestate (handle compression)

/*
struct MovieHeader
{
uint32 magic;						// +0
uint32 version=2;					// +4
uint8 flags[4];					// +8
uint32 length_frames;				// +12
uint32 rerecord_count;				// +16
uint32 movie_data_size;			// +20
uint32 offset_to_savestate;		// +24, should be 4-byte-aligned
uint32 offset_to_movie_data;		// +28, should be 4-byte-aligned
uint8 md5_of_rom_used[16];			// +32
uint32 version_of_emu_used			// +48
char name_of_rom_used[]			// +52, utf-8, null-terminated
char metadata[];					//      utf-8, null-terminated
uint8 padding[];
uint8 savestate[];					//      always present, even in a "from reset" recording
uint8 padding[];					//      used for byte-alignment
uint8 movie_data[];
}
*/

FILE* slots;
static int current = 0;     // > 0 for recording, < 0 for playback
static uint8 joop[4]; //mbg 5/22/08 - seems to be a snapshot of the joystates for delta compression
static uint32 framets = 0;
static uint32 frameptr = 0;
static uint8* moviedata = NULL;
static uint32 moviedatasize = 0;
static uint32 firstframeoffset = 0;
static uint32 savestate_offset = 0;
static uint32 framecount = 0;
static uint32 rerecord_count = 0;
/*static*/ int movie_readonly = 1;
static uint32 pauseframe = 0;
int frame_display = 0;
static uint32 last_frame_display = ~0;
int input_display = 0;
uint32 cur_input_display = 0;
static uint32 last_input_display = ~0;

int resetDMCacc=0;

int justAutoConverted = 0; //PLEASE REMOVE ME

/* Cache variables used for playback. */
static uint32 nextts = 0;
static int32 nextd = 0;

SFORMAT FCEUMOV_STATEINFO[]={
	{ joop, 4,"JOOP"},
	{ &framets, 4|FCEUSTATE_RLSB, "FTS "},
	{ &nextts, 4|FCEUSTATE_RLSB, "NXTS"},
	{ &nextd, 4|FCEUSTATE_RLSB, "NXTD"},
	{ &frameptr, 4|FCEUSTATE_RLSB, "FPTR"},
	{ &framecount, 4|FCEUSTATE_RLSB, "FCNT"},

	{ 0 }
};

static int MovieShow = 0;  



//sometimes we accidentally produce movie stop signals while we're trying to do other things with movies..
bool suppressMovieStop=false;

int movieConvertOffset1=0, movieConvertOffset2=0,movieConvertOK=0,movieSyncHackOn=0;

//todo - consider a MemoryBackedFile class..
//..a lot of work.. instead lets just read back from the current fcm

static enum EMOVIEMODE
{
	MOVIEMODE_INACTIVE, MOVIEMODE_RECORD, MOVIEMODE_PLAY
} movieMode = MOVIEMODE_INACTIVE;

//this should not be set unless we are in MOVIEMODE_RECORD!
FILE* fpRecordingMovie = 0;
int currFrameCounter;

int FCEUMOV_GetFrame(void)
{
	return currFrameCounter;
}


bool FCEUMOV_ShouldPause(void)
{
	if(pauseframe && currFrameCounter == pauseframe)
	{
		pauseframe = 0; //only pause once!
		return true;
	}
	else
	{
		return false;
	}
}

int FCEUMOV_IsPlaying(void)
{
	return movieMode == MOVIEMODE_PLAY;
}

int FCEUMOV_IsRecording(void)
{
	return movieMode == MOVIEMODE_RECORD;
}


char curMovieFilename[512];

class MovieRecord
{
public:
	ValueArray<uint8,4> joysticks;
};

class MovieData
{
public:
	MovieData()
		: version(MOVIE_VERSION)
		, emuVersion(FCEU_VERSION_NUMERIC)
		, palFlag(false)
		, poweronFlag(false)
		, resetFlag(false)
	{
		memset(&romChecksum,0,sizeof(MD5DATA));
	}

	int emuVersion;
	int version;
	//todo - somehow force mutual exclusion for poweron and reset (with an error in the parser)
	bool palFlag;
	bool poweronFlag;
	bool resetFlag;
	MD5DATA romChecksum;
	std::string romFilename;
	std::vector<char> savestate;
	std::vector<MovieRecord> records;

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

	void installDictionary(TDictionary& dictionary)
	{
		dictionary.tryInstallInt("version",version);
		dictionary.tryInstallInt("emuVersion",emuVersion);
		dictionary.tryInstallBool("palFlag",palFlag);
		dictionary.tryInstallBool("poweronFlag",poweronFlag);
		dictionary.tryInstallBool("resetFlag",resetFlag);
		dictionary.tryInstallString("romFilename",romFilename);
		if(dictionary.containsKey("romChecksum"))
			StringToBytes(dictionary["romChecksum"],&romChecksum,MD5DATA::size);
		if(dictionary.containsKey("savestate"))
		{
			std::string& str = dictionary["savestate"];
			int len = HexStringToBytesLength(str);
			if(len >= 1)
			{
				savestate.resize(len);
				StringToBytes(str,&savestate[0],len);
			}
		}
	}

} currMovieData;

void DumpCurrentHeader(FILE* fp)
{
	fprintf(fp,":version %d\n", currMovieData.version);
	fprintf(fp,":emuVersion %d\n", currMovieData.emuVersion);
	fprintf(fp,":palFlag %d\n", currMovieData.palFlag?1:0);
	fprintf(fp,":poweronFlag %d\n", currMovieData.poweronFlag?1:0);
	fprintf(fp,":resetFlag %d\n", currMovieData.resetFlag?1:0);
	fprintf(fp,":romFilename %s\n", currMovieData.romFilename.c_str());
	fprintf(fp,":romChecksum %s\n", BytesToString(currMovieData.romChecksum.data,MD5DATA::size).c_str());
	if(currMovieData.savestate.size() != 0)
		fprintf(fp,":savestate %s\n", BytesToString(&currMovieData.savestate[0],currMovieData.savestate.size()).c_str());
}

//yuck... another custom text parser.
void LoadFM2(MovieData& movieData, FILE *fp)
{
	MovieData::TDictionary dictionary;

	std::string key,value;
	enum {
		NEWLINE, KEY, SEPARATOR, VALUE, RECORD
	} state = NEWLINE;
	bool bail = false;
	for(;;)
	{
		int c = fgetc(fp);
		if(c == -1)
			goto bail;
		bool iswhitespace = (c==' '||c=='\t');
		bool iskeychar = (c==':');
		bool isnewline = (c==10||c==13);
		switch(state)
		{
		case NEWLINE:
			if(isnewline) goto done;
			if(iskeychar) 
			{
				key = "";
				value = "";
				state = KEY;
				goto done;
			}
			goto dorecord;
			break;
		case RECORD: {
				dorecord:
				MovieRecord record;
				ungetc(c,fp);
				//for each joystick
				for(int i=0;i<4;i++)
				{
					uint8& joystate = record.joysticks[i];
					joystate = 0;
					for(int bit=7;bit>=0;bit--)
					{
						int c = fgetc(fp);
						if(c == -1)
							goto bail;
						if(c != ' ')
							joystate |= (1<<bit);
					}
					//eat the separator (a pipe or a newline)
					fgetc(fp);
				}
				movieData.records.push_back(record);
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
		commit:
		dictionary[key] = value;
		state = NEWLINE;
		if(bail) break;
		done: ;
	}

	movieData.installDictionary(dictionary);
}


/// Stop movie playback.
void StopPlayback()
{
	FCEU_DispMessage("Movie playback stopped.");
	movieMode = MOVIEMODE_INACTIVE;
}

/// Stop movie recording
void StopRecording()
{
	FCEU_DispMessage("Movie recording stopped.");
	movieMode = MOVIEMODE_INACTIVE;

	//mbg todo - think about this
	resetDMCacc = movieSyncHackOn = 0;

	//mbg todo - think about this
	//DoEncode(0,0,1);   // Write a dummy timestamp value so that the movie will keep "playing" after user input has stopped.
	
	//mbg todo - add the equivalent
	// finish header
	//MovieFlushHeader();
	// FIXME:  truncate movie to length
	// ftruncate();

	fclose(fpRecordingMovie);
	fpRecordingMovie = 0;
}


void FCEUI_StopMovie()
{
	if(suppressMovieStop)
		return;
	
	if(movieMode == MOVIEMODE_PLAY)
		StopPlayback();
	else if(movieMode == MOVIEMODE_RECORD)
		StopRecording();
}


void ParseGIInput(FCEUGI *GI); //mbg merge 7/17/06 - had to add. gross.
void InitOtherInput(void); //mbg merge 7/17/06 - had to add. gross.
static void ResetInputTypes()
{
#ifdef MSVC
	extern int UsrInputType[3];
	UsrInputType[0] = SI_GAMEPAD;
	UsrInputType[1] = SI_GAMEPAD;
	UsrInputType[2] = SIFC_NONE;

	ParseGIInput(NULL/*GameInfo*/);
	extern int cspec, gametype;
	cspec=GameInfo->cspecial;
	gametype=GameInfo->type;

	InitOtherInput();
#endif
}


//begin playing an existing movie
void FCEUI_LoadMovie(char *fname, int _read_only, int _pauseframe)
{
	assert(fname);

	pauseframe = _pauseframe;

	FCEUI_StopMovie();

	currMovieData = MovieData();
	
	FILE* fp = FCEUD_UTF8fopen(fname, "rb");
	LoadFM2(currMovieData, fp);
	fclose(fp);

	// fully reload the game to reinitialize everything before playing any movie
	// to try fixing nondeterministic playback of some games
	{
		extern char lastLoadedGameName [2048];
		extern int disableBatteryLoading, suppressAddPowerCommand;
		suppressAddPowerCommand=1;
		suppressMovieStop=true;
		{
			FCEUGI * gi = FCEUI_LoadGame(lastLoadedGameName, 0);
			//mbg 5/23/08 - wtf? why would this return null?
			//if(!gi) PowerNES();
			assert(gi);
		}
		suppressMovieStop=false;
		suppressAddPowerCommand=0;
	}

	//todo - if reset flag is set, will the poweron flag be set?
	//WE NEED TO LOAD A SAVESTATE
	if(!currMovieData.poweronFlag)
	{
		//dump the savestate to disk
		FILE* fp = tmpfile();
		fwrite(&currMovieData.savestate[0],1,currMovieData.savestate.size(),fp);
		fseek(fp,0,SEEK_SET);

		//and load the state
		bool success = FCEUSS_LoadFP(fp,SSLOADPARAM_BACKUP);

		fclose(fp);

		if(!success) return;
	}

	//TODO - handle reset flag

	//...why do we have to do this. isnt it setup by the rom?
	if(currMovieData.palFlag)
		FCEUI_SetVidSystem(1);
	else
		FCEUI_SetVidSystem(0);

	//we really need to research this...
	ResetInputTypes();

	currFrameCounter = 0;
	movieMode = MOVIEMODE_PLAY;
}



//begin recording a new movie
void FCEUI_SaveMovie(char *fname, uint8 flags, const char* metadata)
{
	assert(fname);

	FCEUI_StopMovie();

	fpRecordingMovie = FCEUD_UTF8fopen(fname, "wb");
	if(!fpRecordingMovie)
	{
		FCEU_PrintError("Error opening movie output file: %s",fname);
	}

	movieMode = MOVIEMODE_RECORD;
	currMovieData = MovieData();

	//old sync management code:
	//flags |= MOVIE_FLAG_NOSYNCHACK;
	resetDMCacc=movieSyncHackOn=0;

	currMovieData.palFlag = FCEUI_GetCurrentVidSystem(0,0);
	currMovieData.poweronFlag = (flags & MOVIE_FLAG_FROM_POWERON)!=0;
	currMovieData.resetFlag = (flags & MOVIE_FLAG_FROM_RESET)!=0;
	currMovieData.romChecksum = GameInfo->MD5;
	currMovieData.romFilename = FileBase;

	if(currMovieData.poweronFlag)
	{
		// make a for-movie-recording power-on clear the game's save data, too
		extern char lastLoadedGameName [2048];
		extern int disableBatteryLoading, suppressAddPowerCommand;
		suppressAddPowerCommand=1;
		disableBatteryLoading=1;
		suppressMovieStop=true;
		{
			// NOTE:  this will NOT write an FCEUNPCMD_POWER into the movie file
			FCEUGI* gi = FCEUI_LoadGame(lastLoadedGameName, 0);
			//mbg 5/23/08 - wtf? why would this return null?
			//if(!gi) PowerNES();
			assert(gi);
		}
		suppressMovieStop=false;
		disableBatteryLoading=0;
		suppressAddPowerCommand=0;
	}
	else
	{
		//dump a savestate to a tempfile..
		FILE* tmp = tmpfile();
		FCEUSS_SaveFP(tmp); 

		//reloading the savestate into the data structure
		fseek(tmp,0,SEEK_END);
		int len = (int)ftell(tmp);
		fseek(tmp,0,SEEK_SET);
		currMovieData.savestate.resize(len);
		fread(&currMovieData.savestate[0],1,len,tmp);
		fclose(tmp);
	}

	//we are going to go ahead and dump the header. from now on we will only be appending frames
	DumpCurrentHeader(fpRecordingMovie);

	//todo - think about this
	ResetInputTypes();

	//todo - think about this
	// trigger a reset
	if(flags & MOVIE_FLAG_FROM_RESET)
	{
		ResetNES();	// NOTE:  this will write an FCEUNPCMD_RESET into the movie file
	}

	FCEU_DispMessage("Movie recording started.");

	strcpy(curMovieFilename, fname);
}

static void movie_writechar(int c)
{
	fputc(c,fpRecordingMovie);
}



//the main interaction point between the emulator and the movie system.
//either dumps the current joystick state or loads one state from the movie
void FCEUMOV_AddJoy(uint8 *js, int SkipFlush)
{
	int x,y;

	if(movieMode == MOVIEMODE_PLAY)
	{
		//TODO - rock solid stability and error detection

		//stop when we run out of frames
		if(currFrameCounter == currMovieData.records.size())
		{
			StopPlayback();
		}
		else
		{
			MovieRecord& mr = currMovieData.records[currFrameCounter];
			for(int i=0;i<4;i++)
				js[i] = mr.joysticks[i];
			currFrameCounter++;
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
		//for each joystick
		for(int i=0;i<4;i++)
		{
			//these are mnemonics for each joystick bit.
			//since we usually use the regular joypad, these will be more helpful.
			//but any character other than ' ' should count as a set bit
			//maybe other input types will need to be encoded another way..
			const char mnemonics[] = {'A','B','S','T','U','D','L','R'};
			for(int bit=7;bit>=0;bit--)
			{
				uint8 &joystate = js[i];
				int bitmask = (1<<bit);
				char mnemonic = mnemonics[bit];
				//if the bit is set write the mnemonic
				if(joystate & bitmask)
					movie_writechar(mnemonic);
				else //otherwise write a space
					movie_writechar(' ');
			}

			//separate the joysticks
			if(i != 3) movie_writechar('|');
		}

		//each frame is on a new line
		movie_writechar('\n');
	}

	memcpy(&cur_input_display,js,4);



}

void FCEUMOV_AddCommand(int cmd)
{
	if(current <= 0) return;   // Return if not recording a movie
	//printf("%d\n",cmd);

	//MBG TODO BIG TODO TODO TODO
	//DoEncode((cmd>>3)&0x3,cmd&0x7,1);
}

void FCEU_DrawMovies(uint8 *XBuf)
{
	//mbg todo

//	int frameDisplayOn = current != 0 && frame_display;
//	extern int howlong;
//#if MSVC	
//	extern int32 fps_scale;
//#else
//	int32 fps_scale=256;
//#endif
//	int howl=(180-(FCEUI_EmulationPaused()?(60):(20*fps_scale/256)));
//	if(howl>176) howl=180;
//	if(howl<1) howl=1;
//	if((howlong<howl || movcounter)
//		&& (frameDisplayOn && 
//		(!movcounter || last_frame_display!=framecount)))
//		//|| input_display && (!movcounter || last_input_display!=cur_input_display)))
//	{
//		//Old input display code
//		/*
//		char inputstr [32];
//		if(input_display)
//		{
//		uint32 c = cur_input_display;
//		sprintf(inputstr, "%c%c%c%c%c%c%c%c %c%c%c%c%c%c%c%c",
//		(c&0x40)?'<':' ', (c&0x10)?'^':' ', (c&0x80)?'>':' ', (c&0x20)?'v':' ',
//		(c&0x01)?'A':' ', (c&0x02)?'B':' ', (c&0x08)?'S':' ', (c&0x04)?'s':' ',
//		(c&0x4000)?'<':' ', (c&0x1000)?'^':' ', (c&0x8000)?'>':' ', (c&0x2000)?'v':' ',
//		(c&0x0100)?'A':' ', (c&0x0200)?'B':' ', (c&0x0800)?'S':' ', (c&0x0400)?'s':' ');
//		if(!(c&0xff00))
//		inputstr[8] = '\0';
//		}
//		if(frameDisplayOn && !input_display)
//		FCEU_DispMessage("%s frame %u",current >= 0?"Recording":"Playing",framecount);
//		else if(input_display && !frameDisplayOn)
//		FCEU_DispMessage("Input: %s",inputstr);
//		else //if(input_display && frame_display)
//		FCEU_DispMessage("%s %u %s",current >= 0?"Recording":"Playing",framecount,inputstr);
//		*/
//		if(frameDisplayOn)
//		{
//			FCEU_DispMessage("%s frame %u",current >= 0?"Recording":"Playing",framecount);
//		}
//		last_frame_display = framecount;
//		last_input_display = cur_input_display;
//		movcounter=180-1;
//		return;
//	}
//
//	if(movcounter) movcounter--;
//
//	if(!MovieShow) return;
//
//	FCEU_DrawNumberRow(XBuf,MovieStatus, CurrentMovie);
//	MovieShow--;
}

int FCEUMOV_WriteState(FILE* st)
{
	uint32 to_write = 0;
	if(current < 0)
		to_write = moviedatasize;
	else if(current > 0)
		to_write = frameptr;

	if(!st)
		return to_write;

	if(to_write)
		fwrite(moviedata, 1, to_write, st);
	return to_write;
}

static int load_successful;

int FCEUMOV_ReadState(FILE* st, uint32 size)
{
	//// if this savestate was made while replaying,
	//// we need to "undo" nextd and nextts
	//if(nextd != -1)
	//{
	//	int d = 1;
	//	if(nextts > 65536)
	//		d = 4;
	//	else if(nextts > 256)
	//		d = 3;
	//	else if(nextts > 0)
	//		d = 2;
	//	frameptr -= d;
	//	nextd = -1;
	//}

	//// if(current > 0 || (!movie_readonly && current < 0))            /* Recording or Playback (!read-only) */
	//if(current!=0 && !movie_readonly)
	//{
	//	// copy movie data from savestate
	//	moviedata = (uint8*)realloc(moviedata, size);
	//	moviedatasize = size;
	//	if(size && fread(moviedata, 1, size, st)<size)
	//		return 0;
	//	if(current < 0)                   // switch to recording
	//		current = -current;
	//	fseek(slots[current - 1], firstframeoffset, SEEK_SET);
	//	fwrite(moviedata, 1, frameptr, slots[current - 1]);
	//	if(!FCEU_BotMode())
	//	{
	//		rerecord_count++;
	//	}
	//}
	//// else if(current < 0)       /* Playback (read-only) */
	//else if(current!=0 && movie_readonly)
	//{
	//	if(current > 0)                   // switch to playback
	//		current = -current;
	//	// allow frameptr to be updated but keep movie data
	//	fseek(st, size, SEEK_CUR);
	//	// prevent seeking beyond the end of the movie
	//	if(frameptr > moviedatasize)
	//		frameptr = moviedatasize;
	//}
	//else						/* Neither recording or replaying */
	//{
	//	// skip movie data
	//	fseek(st, size, SEEK_CUR);
	//}

	//load_successful=1;

	//// Maximus: Show the last input combination entered from the
	//// movie within the state
	//if(current!=0) // <- mz: only if playing or recording a movie
	//	memcpy(&cur_input_display, joop, 4);

	return 1;
}

void FCEUMOV_PreLoad(void)
{
	load_successful=0;
}

int FCEUMOV_PostLoad(void)
{
	if(!FCEUI_IsMovieActive())
		return 1;
	else
		return load_successful;
}

int FCEUI_IsMovieActive(void)
{
	//this is a lame method. we should change all the fceu code that uses it to call the 
	//IsRecording or IsPlaying methods
	//return > 0 for recording, < 0 for playback
	if(FCEUMOV_IsRecording()) return 1;
	else if(FCEUMOV_IsPlaying()) return -1;
	else return 0;
}

void FCEUI_MovieToggleFrameDisplay(void)
{
	frame_display=!frame_display;
	if(!(current != 0 && frame_display))// && !input_display)
		FCEU_ResetMessages();
	else
	{
		last_frame_display = ~framecount;
		last_input_display = ~cur_input_display;
	}
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

	//if(!input_display && !(current != 0 && frame_display))
	//FCEU_ResetMessages();
	//else
	if(input_display)
	{
		last_frame_display = ~framecount;
		last_input_display = ~cur_input_display;
	}
}

void FCEUI_MovieToggleReadOnly(void)
{
	if(movie_readonly < 2)
	{
		movie_readonly = !movie_readonly;
		if(movie_readonly)
			FCEU_DispMessage("Movie is now Read-Only.");
		else
			FCEU_DispMessage("Movie is now Read+Write.");
	}
	else
	{
		FCEU_DispMessage("Movie file is Read-Only.");
	}
}

int FCEUI_MovieGetInfo(const char* fname, MOVIE_INFO* info)
{
	memset(info,0,sizeof(MOVIE_INFO));

	MovieData md;
	FILE* fp = FCEUD_UTF8fopen(fname, "rb");
	if(!fp) return 0;
	LoadFM2(md, fp);
	fclose(fp);

	info->movie_version = md.version;
	info->poweron = md.poweronFlag;
	info->reset = md.resetFlag;
	info->pal = md.palFlag;
	info->nosynchack = true;
	info->num_frames = md.records.size();
	info->md5_of_rom_used = md.romChecksum;
	info->md5_of_rom_used_present = 1;
	info->emu_version_used = md.emuVersion;
	info->name_of_rom_used = md.romFilename;

	return 1;
}


//
//int _old_FCEUI_MovieGetInfo(const char* fname, MOVIE_INFO* info)
//{
//	//mbg: wtf?
//	//MovieFlushHeader();
//
//	// main get info part of function
//	{
//		uint32 magic;
//		uint32 version;
//		uint8 _flags[4];
//
//		FILE* fp = FCEUD_UTF8fopen(fname, "rb");
//		if(!fp)
//			return 0;
//
//		read32le(&magic, fp);
//		if(magic != MOVIE_MAGIC)
//		{
//			fclose(fp);
//			return 0;
//		}
//
//		read32le(&version, fp);
//		if(version != MOVIE_VERSION)
//		{
//			fclose(fp);
//			if(version == 1)
//				return FCEUI_MovieGetInfo_v1(fname, info);
//			else
//				return 0;
//		}
//
//		info->movie_version = MOVIE_VERSION;
//
//		fread(_flags, 1, 4, fp);
//
//		info->flags = _flags[0];
//		read32le(&info->num_frames, fp);
//		read32le(&info->rerecord_count, fp);
//
//		if(access(fname, W_OK))
//			info->read_only = 1;
//		else
//			info->read_only = 0;
//
//		fseek(fp, 12, SEEK_CUR);			// skip movie_data_size, offset_to_savestate, and offset_to_movie_data
//
//		fread(&info->md5_of_rom_used, 1, 16, fp);
//		info->md5_of_rom_used_present = 1;
//
//		read32le(&info->emu_version_used, fp);
//
//		// I probably could have planned this better...
//		{
//			char str[256];
//			size_t r;
//			uint32 p; //mbg merge 7/17/06 change to uint32
//			int p2=0;
//			char last_c=32;
//
//			if(info->name_of_rom_used && info->name_of_rom_used_size)
//				info->name_of_rom_used[0]='\0';
//
//			r=fread(str, 1, 256, fp);
//			while(r > 0)
//			{
//				for(p=0; p<r && last_c != '\0'; ++p)
//				{
//					if(info->name_of_rom_used && info->name_of_rom_used_size && (p2 < info->name_of_rom_used_size-1))
//					{
//						info->name_of_rom_used[p2]=str[p];
//						p2++;
//						last_c=str[p];
//					}
//				}
//				if(p<r)
//				{
//					memmove(str, str+p, r-p);
//					r -= p;
//					break;
//				}
//				r=fread(str, 1, 256, fp);
//			}
//
//			p2=0;
//			last_c=32;
//
//			if(info->metadata && info->metadata_size)
//				info->metadata[0]='\0';
//
//			while(r > 0)
//			{
//				for(p=0; p<r && last_c != '\0'; ++p)
//				{
//					if(info->metadata && info->metadata_size && (p2 < info->metadata_size-1))
//					{
//						info->metadata[p2]=str[p];
//						p2++;
//						last_c=str[p];
//					}
//				}
//				if(p != r)
//					break;
//				r=fread(str, 1, 256, fp);
//			}
//
//			if(r<=0)
//			{
//				// somehow failed to read romname and metadata
//				fclose(fp);
//				return 0;
//			}
//		}
//
//		// check what hacks are necessary
//		fseek(fp, 24, SEEK_SET);			// offset_to_savestate offset
//		uint32 temp_savestate_offset;
//		read32le(&temp_savestate_offset, fp);
//		if(temp_savestate_offset != 0xFFFFFFFF)
//		{
//			if(fseek(fp, temp_savestate_offset, SEEK_SET))
//			{
//				fclose(fp);
//				return 0;
//			}
//
//			//don't really load, just load to find what's there then load backup
//			if(!FCEUSS_LoadFP(fp,SSLOADPARAM_DUMMY)) return 0; 
//		}
//
//		fclose(fp);
//		return 1;
//	}
//}
/*
Backwards compat
*/


/*
struct MovieHeader_v1
{
uint32 magic;
uint32 version=1;
uint8 flags[4];
uint32 length_frames;
uint32 rerecord_count;
uint32 movie_data_size;
uint32 offset_to_savestate;
uint32 offset_to_movie_data;
uint16 metadata_ucs2[];     // ucs-2, ick!  sizeof(metadata) = offset_to_savestate - MOVIE_HEADER_SIZE
}
*/
//
//#define MOVIE_V1_HEADER_SIZE	32
//
//static void FCEUI_LoadMovie_v1(char *fname, int _read_only)
//{
//	FILE *fp;
//	char *fn = NULL;
//
//	FCEUI_StopMovie();
//
//	if(!fname)
//		fname = fn = FCEU_MakeFName(FCEUMKF_MOVIE,0,0);
//
//	// check movie_readonly
//	movie_readonly = _read_only;
//	if(access(fname, W_OK))
//		movie_readonly = 2;
//
//	fp = FCEUD_UTF8fopen(fname, (movie_readonly>=2) ? "rb" : "r+b");
//
//	if(fn)
//	{
//		free(fn);
//		fname = NULL;
//	}
//
//	if(!fp) return;
//
//	// read header
//	{
//		uint32 magic;
//		uint32 version;
//		uint8 flags[4];
//		uint32 fc;
//
//		read32le(&magic, fp);
//		if(magic != MOVIE_MAGIC)
//		{
//			fclose(fp);
//			return;
//		}
//
//		read32le(&version, fp);
//		if(version != 1)
//		{
//			fclose(fp);
//			return;
//		}
//
//		fread(flags, 1, 4, fp);
//		read32le(&fc, fp);
//		read32le(&rerecord_count, fp);
//		read32le(&moviedatasize, fp);
//		read32le(&savestate_offset, fp);
//		read32le(&firstframeoffset, fp);
//		if(fseek(fp, savestate_offset, SEEK_SET))
//		{
//			fclose(fp);
//			return;
//		}
//
//		if(flags[0] & MOVIE_FLAG_NOSYNCHACK)
//			movieSyncHackOn=0;
//		else
//			movieSyncHackOn=1;
//	}
//
//	// fully reload the game to reinitialize everything before playing any movie
//	// to try fixing nondeterministic playback of some games
//	{
//		extern char lastLoadedGameName [2048];
//		extern int disableBatteryLoading, suppressAddPowerCommand;
//		suppressAddPowerCommand=1;
//		suppressMovieStop=true;
//		{
//			FCEUGI * gi = FCEUI_LoadGame(lastLoadedGameName, 0);
//			if(!gi)
//				PowerNES();
//		}
//		suppressMovieStop=false;
//		suppressAddPowerCommand=0;
//	}
//
//	if(!FCEUSS_LoadFP(fp,SSLOADPARAM_BACKUP)) return;
//
//	ResetInputTypes();
//
//	fseek(fp, firstframeoffset, SEEK_SET);
//	moviedata = (uint8*)realloc(moviedata, moviedatasize);
//	fread(moviedata, 1, moviedatasize, fp);
//
//	framecount = 0;		// movies start at frame 0!
//	frameptr = 0;
//	current = 0;
//	slots = fp;
//
//	memset(joop,0,sizeof(joop));
//	current = -1 - current;
//	framets=0;
//	nextts=0;
//	nextd = -1;
//	FCEU_DispMessage("Movie playback started.");
//}
//
//static int FCEUI_MovieGetInfo_v1(const char* fname, MOVIE_INFO* info)
//{
//	uint32 magic;
//	uint32 version;
//	uint8 _flags[4];
//	uint32 savestateoffset;
//	uint8 tmp[MOVIE_MAX_METADATA<<1];
//	int metadata_length;
//
//	FILE* fp = FCEUD_UTF8fopen(fname, "rb");
//	if(!fp)
//		return 0;
//
//	read32le(&magic, fp);
//	if(magic != MOVIE_MAGIC)
//	{
//		fclose(fp);
//		return 0;
//	}
//
//	read32le(&version, fp);
//	if(version != 1)
//	{
//		fclose(fp);
//		return 0;
//	}
//
//	info->movie_version = 1;
//	info->emu_version_used = 0;			// unknown
//
//	fread(_flags, 1, 4, fp);
//
//	info->flags = _flags[0];
//	read32le(&info->num_frames, fp);
//	read32le(&info->rerecord_count, fp);
//
//	if(access(fname, W_OK))
//		info->read_only = 1;
//	else
//		info->read_only = 0;
//
//	fseek(fp, 4, SEEK_CUR);
//	read32le(&savestateoffset, fp);
//
//	metadata_length = (int)savestateoffset - MOVIE_V1_HEADER_SIZE;
//	if(metadata_length > 0)
//	{
//		//int i; //mbg merge 7/17/06 removed
//
//		metadata_length >>= 1;
//		if(metadata_length >= MOVIE_MAX_METADATA)
//			metadata_length = MOVIE_MAX_METADATA-1;
//
//		fseek(fp, MOVIE_V1_HEADER_SIZE, SEEK_SET);
//		fread(tmp, 1, metadata_length<<1, fp);
//	}
//
//	// turn old ucs2 metadata into utf8
//	if(info->metadata && info->metadata_size)
//	{
//		char* ptr=info->metadata;
//		char* ptr_end=info->metadata+info->metadata_size-1;
//		int c_ptr=0;
//		while(ptr<ptr_end && c_ptr<metadata_length)
//		{
//			uint16 c=(tmp[c_ptr<<1] | (tmp[(c_ptr<<1)+1] << 8));
//			//mbg merge 7/17/06 changed to if..elseif
//			if(c<=0x7f)
//				*ptr++ = (char)(c&0x7f);
//			else if(c<=0x7FF)
//				if(ptr+1>=ptr_end)
//					ptr_end=ptr;
//				else
//			 {
//				 *ptr++=(0xc0 | (c>>6));
//				 *ptr++=(0x80 | (c & 0x3f));
//			 }
//			else
//				if(ptr+2>=ptr_end)
//					ptr_end=ptr;
//				else
//			 {
//				 *ptr++=(0xe0 | (c>>12));
//				 *ptr++=(0x80 | ((c>>6) & 0x3f));
//				 *ptr++=(0x80 | (c & 0x3f));
//			 }
//
//				c_ptr++;
//		}
//		*ptr='\0';
//	}
//
//	// md5 info not available from v1
//	info->md5_of_rom_used_present = 0;
//	// rom name used for the movie not available from v1
//	if(info->name_of_rom_used && info->name_of_rom_used_size)
//		info->name_of_rom_used[0] = '\0';
//
//	// check what hacks are necessary
//	fseek(fp, 24, SEEK_SET);			// offset_to_savestate offset
//	uint32 temp_savestate_offset;
//	read32le(&temp_savestate_offset, fp);
//	if(fseek(fp, temp_savestate_offset, SEEK_SET))
//	{
//		fclose(fp);
//		return 0;
//	}
//	if(!FCEUSS_LoadFP(fp,SSLOADPARAM_DUMMY)) return 0; // 2 -> don't really load, just load to find what's there then load backup
//
//
//	fclose(fp);
//	return 1;
//}


//
//// PlayMovie / MoviePlay function
//void _old_FCEUI_LoadMovie(char *fname, int _read_only, int _stopframe)
//{
//	char buffer [512];
//	//fname = (char*)convertToFCM(fname,buffer);
//
//	FILE *fp;
//	char *fn = NULL;
//
//	FCEUI_StopMovie();
//
//	if(!fname)
//		fname = fn = FCEU_MakeFName(FCEUMKF_MOVIE,0,0);
//
//	char origname[512];
//	strcpy(origname,fname);
//
//	pauseframe = _stopframe;
//
//	// check movie_readonly
//	movie_readonly = _read_only;
//	if(access(fname, W_OK))
//		movie_readonly = 2;
//
//	fp = FCEUD_UTF8fopen(fname, (movie_readonly>=2) ? "rb" : "r+b");
//
//	if(fn)
//	{
//		free(fn);
//		fname = NULL;
//	}
//
//	if(!fp) return;
//
//	// read header
//
//	uint32 magic;
//	uint32 version;
//	uint8 flags[4];
//
//	read32le(&magic, fp);
//	if(magic != MOVIE_MAGIC)
//	{
//		fclose(fp);
//		return;
//	}
//	//DEBUG_COMPARE_RAM(__LINE__);
//
//	read32le(&version, fp);
//	if(version == 1)
//	{
//		// attempt to load previous version's format
//		fclose(fp);
//		FCEUI_LoadMovie_v1(fname, _read_only);
//		return;
//	}
//	else if(version == MOVIE_VERSION)
//	{}
//	else
//	{
//		// unsupported version
//		fclose(fp);
//		return;
//	}
//
//	fread(flags, 1, 4, fp);
//	read32le(&framecount, fp);
//	read32le(&rerecord_count, fp);
//	read32le(&moviedatasize, fp);
//	read32le(&savestate_offset, fp);
//	read32le(&firstframeoffset, fp);
//
//	//  FCEU_PrintError("flags[0] & MOVIE_FLAG_NOSYNCHACK=%d",flags[0] & MOVIE_FLAG_NOSYNCHACK);
//	if(flags[0] & MOVIE_FLAG_NOSYNCHACK)
//		movieSyncHackOn=0;
//	else
//		movieSyncHackOn=1;
//
//	if(flags[0] & MOVIE_FLAG_PAL)
//	{
//		FCEUI_SetVidSystem(1);
//	}
//	else
//	{
//		FCEUI_SetVidSystem(0);
//	}
//
//
//	// fully reload the game to reinitialize everything before playing any movie
//	// to try fixing nondeterministic playback of some games
//	{
//		extern char lastLoadedGameName [2048];
//		extern int disableBatteryLoading, suppressAddPowerCommand;
//		suppressAddPowerCommand=1;
//		suppressMovieStop=true;
//		{
//			FCEUGI * gi = FCEUI_LoadGame(lastLoadedGameName, 0);
//			if(!gi)
//				PowerNES();
//		}
//		suppressMovieStop=false;
//		suppressAddPowerCommand=0;
//	}
//
//	if(flags[0] & MOVIE_FLAG_FROM_POWERON)
//	{
//		//don't need to load a savestate
//		//there shouldn't be a savestate!
//		if(savestate_offset != 0xFFFFFFFF)
//			FCEU_PrintError("Savestate found in a start-from-poweron movie!");
//	}
//	else
//	{
//		if(savestate_offset == 0xFFFFFFFF)
//			FCEU_PrintError("No savestate found in a start-from-savestate movie!");
//
//		if(fseek(fp, savestate_offset, SEEK_SET))
//		{
//			fclose(fp);
//			return;
//		}
//
//		if(!FCEUSS_LoadFP(fp,SSLOADPARAM_BACKUP)) 
//			return;
//	}
//
//	if(flags[0] & MOVIE_FLAG_PAL)
//	{
//		FCEUI_SetVidSystem(1);
//	}
//	else
//	{
//		FCEUI_SetVidSystem(0);
//	}
//	ResetInputTypes();
//
//	fseek(fp, firstframeoffset, SEEK_SET);
//	moviedata = (uint8*)realloc(moviedata, moviedatasize);
//	fread(moviedata, 1, moviedatasize, fp);
//
//	framecount = 0;		// movies start at frame 0!
//	frameptr = 0;
//	current = 0;
//	slots = fp;
//
//	memset(joop,0,sizeof(joop));
//	current = -1 - current;
//	framets=0;
//	nextts=0;
//	nextd = -1;
//
//	FCEU_DispMessage("Movie playback started.");
//
//	strcpy(curMovieFilename, origname);
//}


//static void DoEncode(int joy, int button, int dummy)
//{
//	uint8 d;
//
//	d = 0;
//
//	if(framets >= 65536)
//		d = 3 << 5;
//	else if(framets >= 256)
//		d = 2 << 5;
//	else if(framets > 0)
//		d = 1 << 5;
//
//	if(dummy) d|=0x80;
//
//	d |= joy << 3;
//	d |= button;
//
//	movie_writechar(d);
//	//printf("Wr: %02x, %d\n",d,slots[current-1]);
//	while(framets)
//	{
//		movie_writechar(framets & 0xff);
//		//printf("Wrts: %02x\n",framets & 0xff);
//		framets >>= 8;
//	}
//}
