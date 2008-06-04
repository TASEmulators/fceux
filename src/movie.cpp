#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <zlib.h>

#ifdef WIN32
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
#include "utils/memorystream.h"
#include "utils/xstring.h"

using namespace std;


#define MOVIE_VERSION           3 // still at 2 since the format itself is still compatible - to detect which version the movie was made with, check the fceu version stored in the movie header (e.g against FCEU_VERSION_NUMERIC)

extern char FileBase[];
extern int EmulationPaused;
extern bool moviePleaseLogSavestates;

using namespace std;

//TODO - remove the synchack stuff from the replay gui and require it to be put into the fm2 file
//which the user would have already converted from fcm
//also cleanup the whole emulator version bullshit in replay. we dont support that old stuff anymore

//todo - better error handling for the compressed savestate

//todo - consider a MemoryBackedFile class..
//..a lot of work.. instead lets just read back from the current fcm

//todo - handle case where read+write is requested, but the file is actually readonly (could be confusing)

//sometimes we accidentally produce movie stop signals while we're trying to do other things with movies..
bool suppressMovieStop=false;

//----movie engine main state

static enum EMOVIEMODE
{
	MOVIEMODE_INACTIVE, MOVIEMODE_RECORD, MOVIEMODE_PLAY
} movieMode = MOVIEMODE_INACTIVE;

//this should not be set unless we are in MOVIEMODE_RECORD!
FILE* fpRecordingMovie = 0;

int currFrameCounter;
uint32 cur_input_display = 0;
int pauseframe;
bool movie_readonly = true;
int input_display = 0;
int frame_display = 0;

SFORMAT FCEUMOV_STATEINFO[]={
	{ &currFrameCounter, 4|FCEUSTATE_RLSB, "FCNT"},
	{ 0 }
};

char curMovieFilename[512] = {0};
MovieData currMovieData;

void MovieData::clearRecordRange(int start, int len)
{
	for(int i=0;i<len;i++)
		records[i+start].clear();
}

void MovieData::TryDumpIncremental()
{
	if(moviePleaseLogSavestates)
	{
		//only log the savestate if we are appending to the green zone
		if(currFrameCounter == currMovieData.greenZoneCount)
		{
			if(currFrameCounter < (int)currMovieData.records.size())
			{
				MovieData::dumpSavestateTo(&currMovieData.records[currFrameCounter].savestate,Z_DEFAULT_COMPRESSION);
				currMovieData.greenZoneCount++;
			}
		}
	}
}

const char MovieRecord::mnemonics[8] = {'A','B','S','T','U','D','L','R'};
void MovieRecord::dump(std::ostream* os, int index)
{
	//todo: if we want frame numbers in the output (which we dont since we couldnt cut and paste in movies)
	//but someone would need to change the parser to ignore it
	//fputc('|',fp);
	//fprintf(fp,"%08d",index);

	os->put('|');

	//for each joystick
	for(int i=0;i<4;i++)
	{
		//these are mnemonics for each joystick bit.
		//since we usually use the regular joypad, these will be more helpful.
		//but any character other than ' ' should count as a set bit
		//maybe other input types will need to be encoded another way..
		for(int bit=7;bit>=0;bit--)
		{
			uint8 &joystate = joysticks[i];
			int bitmask = (1<<bit);
			char mnemonic = mnemonics[bit];
			//if the bit is set write the mnemonic
			if(joystate & bitmask)
				os->put(mnemonic);
			else //otherwise write a space
				os->put(' ');
		}

		//separate the joysticks
		if(i != 3) os->put('|');
	}

	//each frame is on a new line
	os->put('\n');
}

void MovieRecord::dump(FILE* fp, int index)
{
	//todo: if we want frame numbers in the output (which we dont since we couldnt cut and paste in movies)
	//but someone would need to change the parser to ignore it
	//fputc('|',fp);
	//fprintf(fp,"%08d",index);

	fputc('|',fp);

	//for each joystick
	for(int i=0;i<4;i++)
	{
		//these are mnemonics for each joystick bit.
		//since we usually use the regular joypad, these will be more helpful.
		//but any character other than ' ' should count as a set bit
		//maybe other input types will need to be encoded another way..
		for(int bit=7;bit>=0;bit--)
		{
			uint8 &joystate = joysticks[i];
			int bitmask = (1<<bit);
			char mnemonic = mnemonics[bit];
			//if the bit is set write the mnemonic
			if(joystate & bitmask)
				fputc(mnemonic,fp);
			else //otherwise write a space
				fputc(' ',fp);
		}

		//separate the joysticks
		if(i != 3) fputc('|',fp);
	}

	//each frame is on a new line
	fputc('\n',fp);
}

MovieData::MovieData()
	: version(MOVIE_VERSION)
	, emuVersion(FCEU_VERSION_NUMERIC)
	, palFlag(false)
	, poweronFlag(false)
	, resetFlag(false)
	, recordCount(1)
	, greenZoneCount(0)
{
	memset(&romChecksum,0,sizeof(MD5DATA));
}

void MovieData::truncateAt(int frame)
{
	records.resize(frame);
}

void MovieData::installDictionary(TDictionary& dictionary)
{
	dictionary.tryInstallInt("version",version);
	dictionary.tryInstallInt("emuVersion",emuVersion);
	dictionary.tryInstallInt("recordCount",recordCount);
	dictionary.tryInstallBool("palFlag",palFlag);
	dictionary.tryInstallBool("poweronFlag",poweronFlag);
	dictionary.tryInstallBool("resetFlag",resetFlag);
	dictionary.tryInstallString("romFilename",romFilename);
	if(dictionary.containsKey("romChecksum"))
		StringToBytes(dictionary["romChecksum"],&romChecksum,MD5DATA::size);
	if(dictionary.containsKey("guid"))
		guid = FCEU_Guid::fromString(dictionary["guid"]);
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

void MovieData::dump(std::ostream *os)
{
	*os << "version " << version << endl;
	*os << "emuVersion " << emuVersion << endl;
	*os << "recordCount " << recordCount << endl;
	*os << "palFlag " << (palFlag?1:0) << endl;
	*os << "poweronFlag " << (poweronFlag?1:0) << endl;
	*os << "resetFlag " << (resetFlag?1:0) << endl;
	*os << "romFilename " << romFilename << endl;
	*os << "romChecksum " << BytesToString(romChecksum.data,MD5DATA::size) << endl;
	*os << "guid " << guid.toString() << endl;
		
	if(savestate.size() != 0)
		*os << "savestate " << BytesToString(&savestate[0],savestate.size()) << endl;
	for(int i=0;i<(int)records.size();i++)
		records[i].dump(os,i);
}

void MovieData::dump(FILE *fp)
{
	fprintf(fp,"version %d\n", version);
	fprintf(fp,"emuVersion %d\n", emuVersion);
	fprintf(fp,"recordCount %d\n", recordCount);
	fprintf(fp,"palFlag %d\n", palFlag?1:0);
	fprintf(fp,"poweronFlag %d\n", poweronFlag?1:0);
	fprintf(fp,"resetFlag %d\n", resetFlag?1:0);
	fprintf(fp,"romFilename %s\n", romFilename.c_str());
	fprintf(fp,"romChecksum %s\n", BytesToString(romChecksum.data,MD5DATA::size).c_str());
	fprintf(fp,"guid %s\n", guid.toString().c_str());
	if(savestate.size() != 0)
		fprintf(fp,"savestate %s\n", BytesToString(&savestate[0],savestate.size()).c_str());
	for(int i=0;i<(int)records.size();i++)
		records[i].dump(fp,i);
}

int MovieData::dumpLen()
{
	memorystream ms;
	dump(&ms);
	return ms.size();
}


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
		bool iswhitespace, isrecchar, isnewline;
		int c = fgetc(fp);
		if(c == -1)
			goto bail;
		iswhitespace = (c==' '||c=='\t');
		isrecchar = (c=='|');
		isnewline = (c==10||c==13);
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
		case RECORD: {
				dorecord:
				MovieRecord record;
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
	FCEU_DispMessageOnMovie("Movie playback stopped.");
	movieMode = MOVIEMODE_INACTIVE;
}

/// Stop movie recording
void StopRecording()
{
	FCEU_DispMessage("Movie recording stopped.");

	movieMode = MOVIEMODE_INACTIVE;
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

	curMovieFilename[0] = 0;
}


void ParseGIInput(FCEUGI *GI); //mbg merge 7/17/06 - had to add. gross.
void InitOtherInput(void); //mbg merge 7/17/06 - had to add. gross.
static void ResetInputTypes()
{
#ifdef WIN32
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

bool MovieData::loadSavestateFrom(std::vector<char>* buf)
{
	//dump the savestate to disk
	FILE* fp = tmpfile();
	fwrite(&(*buf)[0],1,buf->size(),fp);
	fseek(fp,0,SEEK_SET);

	//and load the state
	bool success = FCEUSS_LoadFP(fp,SSLOADPARAM_BACKUP);
	fclose(fp);
	return success;
}

void MovieData::dumpSavestateTo(std::vector<char>* buf, int compressionLevel)
{
	memorystream ms(buf);
	FCEUSS_SaveMS(&ms,compressionLevel);
	ms.trim();
}

//begin playing an existing movie
void FCEUI_LoadMovie(char *fname, bool _read_only, int _pauseframe)
{
	assert(fname);

	FCEUI_StopMovie();

	currMovieData = MovieData();
	
	strcpy(curMovieFilename, fname);
	FILE* fp = FCEUD_UTF8fopen(fname, "rb");
	if (!fp) return;
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
			//we need to save the pause state through this process
			int oldPaused = EmulationPaused;

			FCEUGI * gi = FCEUI_LoadGame(lastLoadedGameName, 0);
			//mbg 5/23/08 - wtf? why would this return null?
			//if(!gi) PowerNES();
			assert(gi);

			EmulationPaused = oldPaused;
		}
		suppressMovieStop=false;
		suppressAddPowerCommand=0;
	}

	//todo - if reset flag is set, will the poweron flag be set?
	//WE NEED TO LOAD A SAVESTATE
	if(!currMovieData.poweronFlag)
	{
		//and load the state
		bool success = MovieData::loadSavestateFrom(&currMovieData.savestate);
		if(!success) return;
	}

	//TODO - handle reset flag

	//if there is no savestate, we won't have this crucial piece of information at the start of the movie.
	//so, we have to include it with the movie
	if(currMovieData.palFlag)
		FCEUI_SetVidSystem(1);
	else
		FCEUI_SetVidSystem(0);

	//we really need to research this...
	ResetInputTypes();

	//stuff that should only happen when we're ready to positively commit to the replay
	currFrameCounter = 0;
	pauseframe = _pauseframe;
	movie_readonly = _read_only;
	movieMode = MOVIEMODE_PLAY;

	currMovieData.TryDumpIncremental();

	if(movie_readonly)
		FCEU_DispMessage("Replay started Read-Only.");
	else
		FCEU_DispMessage("Replay started Read+Write.");
}

static void closeRecordingMovie()
{
	if(fpRecordingMovie)
		fclose(fpRecordingMovie);
}

static void openRecordingMovie(const char* fname)
{
	fpRecordingMovie = FCEUD_UTF8fopen(fname, "wb");
	if(!fpRecordingMovie)
		FCEU_PrintError("Error opening movie output file: %s",fname);
	strcpy(curMovieFilename, fname);
}

//begin recording a new movie
void FCEUI_SaveMovie(char *fname, uint8 flags)
{
	assert(fname);

	FCEUI_StopMovie();

	openRecordingMovie(fname);

	currFrameCounter = 0;

	currMovieData = MovieData();
	currMovieData.guid.newGuid();

	currMovieData.palFlag = FCEUI_GetCurrentVidSystem(0,0)!=0;
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
			//we need to save the pause state through this process
			int oldPaused = EmulationPaused;

			// NOTE:  this will NOT write an FCEUNPCMD_POWER into the movie file
			FCEUGI* gi = FCEUI_LoadGame(lastLoadedGameName, 0);
			//mbg 5/23/08 - wtf? why would this return null?
			//if(!gi) PowerNES();
			assert(gi);

			EmulationPaused = oldPaused;
		}
		suppressMovieStop=false;
		disableBatteryLoading=0;
		suppressAddPowerCommand=0;
	}
	else
	{
		MovieData::dumpSavestateTo(&currMovieData.savestate,Z_BEST_COMPRESSION);
	}

	//we are going to go ahead and dump the header. from now on we will only be appending frames
	currMovieData.dump(fpRecordingMovie);

	//todo - think about this
	ResetInputTypes();

	//todo - think about this
	// trigger a reset
	if(flags & MOVIE_FLAG_FROM_RESET)
	{
		ResetNES();	// NOTE:  this will write an FCEUNPCMD_RESET into the movie file
	}

	movieMode = MOVIEMODE_RECORD;
	movie_readonly = false;
	
	FCEU_DispMessage("Movie recording started.");
}

//the main interaction point between the emulator and the movie system.
//either dumps the current joystick state or loads one state from the movie
void FCEUMOV_AddJoy(uint8 *js, int SkipFlush)
{
	if(movieMode == MOVIEMODE_PLAY)
	{
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

		//for each joystick
		for(int i=0;i<4;i++)
			mr.joysticks[i] = js[i];

		mr.dump(fpRecordingMovie,currMovieData.records.size());
		currMovieData.records.push_back(mr);
	}

	currFrameCounter++;

	memcpy(&cur_input_display,js,4);
}

//TODO 
void FCEUMOV_AddCommand(int cmd)
{
	// do nothing if not recording a movie
	if(movieMode != MOVIEMODE_RECORD)
		return;
	
	//printf("%d\n",cmd);

	//MBG TODO BIG TODO TODO TODO
	//DoEncode((cmd>>3)&0x3,cmd&0x7,1);
}

void FCEU_DrawMovies(uint8 *XBuf)
{
	if(frame_display)
	{
		char counterbuf[32] = {0};
		if(movieMode == MOVIEMODE_PLAY)
			sprintf(counterbuf,"%d/%d",currFrameCounter,currMovieData.records.size());
		else if(movieMode == MOVIEMODE_RECORD)
			sprintf(counterbuf,"%d",currMovieData.records.size());

		if(counterbuf[0])
			DrawTextTrans(XBuf+FCEU_TextScanlineOffsetFromBottom(24), 256, (uint8*)counterbuf, 0x20+0x80);
	}
}

int FCEUMOV_WriteState(std::ostream* os)
{
	//we are supposed to dump the movie data into the savestate

	if(movieMode == MOVIEMODE_RECORD || movieMode == MOVIEMODE_PLAY)
	{
		int todo = currMovieData.dumpLen();
		
		if(os)
			currMovieData.dump(os);
		
		return todo;
	}
	else return 0;
}

int FCEUMOV_WriteState(FILE* st)
{
	//we are supposed to dump the movie data into the savestate

	if(movieMode == MOVIEMODE_RECORD || movieMode == MOVIEMODE_PLAY)
	{
		int todo = currMovieData.dumpLen();
		
		if(st)
			currMovieData.dump(st);
		
		return todo;
	}
	else return 0;
}

static bool load_successful;

bool FCEUMOV_ReadState(FILE* st, uint32 size)
{
	load_successful = false;

	//write the state to disk so we can reload
	std::vector<uint8> buf(size);
	fread(&buf[0],1,size,st);
	FILE* tmp = tmpfile();
	fwrite(&buf[0],1,size,tmp);
	//---------
	//(debug)
	//FILE* wtf = fopen("d:\\wtf.txt","wb");
	//fwrite(&buf[0],1,size,wtf);
	//fclose(wtf);
	//---------
	fseek(tmp,0,SEEK_SET);
	MovieData tempMovieData = MovieData();
	LoadFM2(tempMovieData, tmp);
	fclose(tmp);

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
			FCEU_PrintError("Mismatch between savestate's movie and current movie.\ncurrent: %s\nsavestate: %s\n",currMovieData.guid.toString().c_str(),tempMovieData.guid.toString().c_str());
			return false;
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
			currMovieData.recordCount++;

			openRecordingMovie(curMovieFilename);
			currMovieData.dump(fpRecordingMovie);
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

int FCEUMOV_PostLoad(void)
{
	if(!FCEUI_IsMovieActive())
		return 1;
	else
		//mbg tasedit hack!!!!!!!!!
		return load_successful || moviePleaseLogSavestates;
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

bool FCEUI_GetMovieToggleReadOnly()
{
	return movie_readonly;
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
		FCEUI_LoadMovie(fname, true, 0);
		FCEU_DispMessage("Movie is now Read-Only. Playing from beginning.");
		free(fname);
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
	info->rerecord_count = md.recordCount;

	return 1;
}


//struct MovieHeader
//{
//uint32 magic;						// +0
//uint32 version=2;					// +4
//uint8 flags[4];					// +8
//uint32 length_frames;				// +12
//uint32 rerecord_count;				// +16
//uint32 movie_data_size;			// +20
//uint32 offset_to_savestate;		// +24, should be 4-byte-aligned
//uint32 offset_to_movie_data;		// +28, should be 4-byte-aligned
//uint8 md5_of_rom_used[16];			// +32
//uint32 version_of_emu_used			// +48
//char name_of_rom_used[]			// +52, utf-8, null-terminated
//char metadata[];					//      utf-8, null-terminated
//uint8 padding[];
//uint8 savestate[];					//      always present, even in a "from reset" recording
//uint8 padding[];					//      used for byte-alignment
//uint8 movie_data[];
//}


// backwards compat
//static void FCEUI_LoadMovie_v1(char *fname, int _read_only);
//static int FCEUI_MovieGetInfo_v1(const char* fname, MOVIE_INFO* info);
//
//#define MOVIE_MAGIC             0x1a4d4346      // FCM\x1a
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
