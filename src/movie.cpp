#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h> //mbgm erge 7/17/06 removed
#ifdef MSVC
#include <windows.h>
#endif

#include "types.h"
#include "endian.h"
#include "palette.h"
#include "input.h"
#include "fceu.h"
#include "driver.h"
#include "state.h"
#include "general.h"
#include "video.h"
#include "movie.h"
#include "memory.h" //mbg merge 7/17/06 added

#define MOVIE_MAGIC             0x1a4d4346      // FCM\x1a
#define MOVIE_VERSION           2 // still at 2 since the format itself is still compatible - to detect which version the movie was made with, check the fceu version stored in the movie header (e.g against FCEU_VERSION_NUMERIC)

#define MOVIE_FLAG_NOSYNCHACK          (1<<4) // set in newer version, used for old movie compatibility

// backwards compat
static void FCEUI_LoadMovie_v1(char *fname, int _read_only);
static int FCEUI_MovieGetInfo_v1(const char* fname, MOVIE_INFO* info);

extern char FileBase[];

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

static int current = 0;     // > 0 for recording, < 0 for playback
static FILE *slots[10]={0};
static uint8 joop[4];
static uint32 framets = 0;
static uint32 frameptr = 0;
static uint8* moviedata = NULL;
static uint32 moviedatasize = 0;
static uint32 firstframeoffset = 0;
static uint32 savestate_offset = 0;
static uint32 framecount = 0;
static uint32 rerecord_count = 0;
/*static*/ int movie_readonly = 1;
static uint32 stopframe = 0;
int frame_display = 0;
static uint32 last_frame_display = ~0;
int input_display = 0;
uint32 cur_input_display = 0;
static uint32 last_input_display = ~0;

int resetDMCacc=0;

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

static int CurrentMovie = 1;
static int MovieShow = 0;  

static int MovieStatus[10];

static void DoEncode(int joy, int button, int);

int FCEUMOV_GetFrame(void)
{
	return framecount;
}

int FCEUMOV_IsPlaying(void)
{
 if(current < 0) return(1);
 else return(0);
}

int FCEUMOV_IsRecording(void)
{
 if(current > 0) return(1);
 else return(0);
}

int FCEUMOV_ShouldPause(void)
{
	if(stopframe && framecount == stopframe)
	{
		stopframe = 0; //only pause once!
		return 1;
	}
	else
	{
		return 0;
	}
}

int suppressMovieStop=0;
int movieConvertOffset1=0, movieConvertOffset2=0,movieConvertOK=0,movieSyncHackOn=0;

static void StopPlayback(void)
{
  if(suppressMovieStop)
   return;
  resetDMCacc=movieSyncHackOn=0;
  fclose(slots[-1 - current]);
  current=0;
  FCEU_DispMessage("Movie playback stopped.");
}

void MovieFlushHeader(void)
{
	if(current <= 0)
		return;// only write header data if recording

	FILE* fp = slots[current - 1];
	if(fp == 0)
		return;

	unsigned long loc = ftell(fp);
	fseek(fp, 4, SEEK_SET);
	write32le(MOVIE_VERSION, fp);
	fseek(fp, 12, SEEK_SET);
	write32le(framecount, fp);
	write32le(rerecord_count, fp);
	write32le(frameptr, fp);
	fseek(fp, 32, SEEK_SET);
	fwrite(FCEUGameInfo->MD5, 1, 16, fp);	// write ROM checksum
	write32le(FCEU_VERSION_NUMERIC, fp);	// write emu version used

	// write ROM name used
	fseek(fp, 52, SEEK_SET);
	char str[512];
	fgets(str,512,fp);
	str[511]='\0';
	int strdiff=strlen(FileBase)-strlen(str);
	if(strdiff)
	{
		// resize the whole damn movie because the ROM name in the header is of variable length
		int off=52;
		fseek(fp, 52, SEEK_SET);
		do { off++;
		} while(fgetc(fp) && !feof(fp) && !ferror(fp));

		if(feof(fp) || ferror(fp))
		{
			fseek(fp, loc, SEEK_SET);
			return;
		}

		fseek(fp, 0, SEEK_END);
		uint32 fsize=ftell(fp)-off;
		char* ctemp=(char*)FCEU_malloc(fsize*sizeof(char)+4);
		if(!ctemp)
		{
			fseek(fp, loc, SEEK_SET);
			return;
		}
		fseek(fp, off, SEEK_SET);
		fread(ctemp, 1,fsize, fp);
		fseek(fp, 52+strlen(FileBase)+1, SEEK_SET);
		int wrote = fwrite(ctemp, fsize,1, fp);
		FCEU_free(ctemp);
		if(!wrote)
		{
			fseek(fp, loc, SEEK_SET);
			return;
		}

		if(loc >= firstframeoffset)
			loc += strdiff;
		savestate_offset += strdiff;
		firstframeoffset += strdiff;
		fseek(fp, 24, SEEK_SET);
		write32le(savestate_offset, fp);
		write32le(firstframeoffset, fp);
	}
	fseek(fp, 52, SEEK_SET);
	fputs(FileBase, fp);
	fputc('\0', fp);

	fseek(fp, loc, SEEK_SET);
}

static void StopRecording(void)
{
  if(suppressMovieStop)
   return;
  resetDMCacc=movieSyncHackOn=0;
 DoEncode(0,0,1);   /* Write a dummy timestamp value so that the movie will keep
                       "playing" after user input has stopped. */
 // finish header
 MovieFlushHeader();

 // FIXME:  truncate movie to length
 // ftruncate();
 fclose(slots[current - 1]);
 MovieStatus[current - 1] = 1;
 current=0;
 FCEU_DispMessage("Movie recording stopped.");
}

void FCEUI_StopMovie(void)
{
 if(current < 0) StopPlayback();
 if(current > 0) StopRecording();
}

#ifdef MSVC
#include "process.h"
void executeCommand(const char* cmd)
{
	if(!cmd || !*cmd)
		return;

	const char *argv[4];
	argv[0] = getenv("COMSPEC");
	argv[1] = "/c";
	argv[2] = cmd;
	argv[3] = NULL;
	if(*argv && *(*argv))
		_spawnve(_P_WAIT, argv[0], argv, NULL);
}
#endif

int justAutoConverted=0;
static const char* convertToFCM(const char *fname, char *buffer)
{
#ifdef MSVC
	justAutoConverted=0;

	// convert to fcm if not already
	const char* dot = strrchr(fname, '.');
	if(dot)
	{
		int fmv = !stricmp(dot, ".fmv");
		int nmv = !stricmp(dot, ".nmv");
		int vmv = !stricmp(dot, ".vmv");
		if(fmv || nmv || vmv)
		{
			strcpy(buffer, fname);
			buffer[dot-fname]='\0';
			strcat(buffer,"-autoconverted.fcm");

			int fceuver=0;
			if(fmv)
				fceuver=1;
			else if(nmv)
				fceuver=2;
			else if(vmv)
				fceuver=3;

			extern char lastLoadedGameName [2048];
			char cmd [1024], offset[64], romInfo[1024];
			if(movieConvertOK)
				sprintf(romInfo, "-smd5=\"%s\" -sromname=\"%s (MAYBE)\" -s", lastLoadedGameName, FileBase);
			else
				sprintf(romInfo, "-sromname=\"(unknown)\" -s");
			if(movieConvertOffset2) sprintf(offset, "-o %d:%d", movieConvertOffset2,movieConvertOffset1);
			else sprintf(offset, "-o %d", movieConvertOffset1);
			sprintf(cmd, ".\\util\\nesmock\\nesmock.exe %s %s -spal=%c -sfceuver=%d \"%s\" \"%s\" ", offset, romInfo, FCEUI_GetCurrentVidSystem(0,0)?'1':'0', fceuver, fname, buffer);
//				FCEU_PrintError(cmd);
			executeCommand(cmd);

			FILE* file = FCEUD_UTF8fopen(buffer,"rb");
			if(file)
			{
				fseek(file, 12, SEEK_SET);
				uint32 frames=0; //mbg merge 7/17/06 changed to uint32
				read32le(&frames, file);
				if(frames)
				{
					fname = buffer;
					justAutoConverted=1;
				}
				else
				{
					static int errAlready=0;
					if(!errAlready)
					{
						errAlready=1;
						FCEU_PrintError("For some reason, nesmock was unable to create a valid FCM from the given file.\nThe command given was:\n%s\nPerhaps the file specified is not a movie file or contains no input data,\nor perhaps it is a movie file of a version unsupported by nesmock.\n\n(This error message will self-destruct until you restart FCEU.)", cmd);
					}
				}
				fclose(file);
			}
			else
			{
				char str [512];
				str[0] = '\0';
				GetCurrentDirectory(512,str);
				strcat(str, "\\util\\nesmock\\nesmock.exe");
				file = FCEUD_UTF8fopen(str, "rb");
				if(file)
				{
					static int errAlready=0;
					if(!errAlready)
					{
						errAlready=1;
						FCEU_PrintError("For some reason, nesmock was unable to convert the movie to FCM format.\nThe command given was:\n%s\n\n(This error message will self-destruct until you restart FCEU.)", cmd);
						fclose(file);
					}
				}
				else
				{
					static int errAlready=0;
					if(!errAlready)
					{
						errAlready=1;
						FCEU_PrintError("Nesmock not found, so the movie could not be converted to FCM format.\nYou must place nesmock.exe at this location so FCEU can find it:\n%s\n\n(This error message will self-destruct until you restart FCEU.)", str);
					}
				}
			}
		}
	}
#endif
	return fname;
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

 ParseGIInput(NULL/*FCEUGameInfo*/);
 extern int cspec, gametype;
 cspec=FCEUGameInfo->cspecial;
 gametype=FCEUGameInfo->type;

 InitOtherInput();
#endif
}

char curMovieFilename[512];


// PlayMovie / MoviePlay function
void FCEUI_LoadMovie(char *fname, int _read_only, int _stopframe)
{
 char buffer [512];
 fname = (char*)convertToFCM(fname,buffer);

 FILE *fp;
 char *fn = NULL;

 FCEUI_StopMovie();

 if(!fname)
  fname = fn = FCEU_MakeFName(FCEUMKF_MOVIE,CurrentMovie,0);

char origname[512];
strcpy(origname,fname);

 stopframe = _stopframe;

 // check movie_readonly
 movie_readonly = _read_only;
 if(access(fname, W_OK))
  movie_readonly = 2;

 fp = FCEUD_UTF8fopen(fname, (movie_readonly>=2) ? "rb" : "r+b");

 if(fn)
 {
  free(fn);
  fname = NULL;
 }

 if(!fp) return;

 // read header
 
  uint32 magic;
  uint32 version;
  uint8 flags[4];

  read32le(&magic, fp);
  if(magic != MOVIE_MAGIC)
  {
   fclose(fp);
   return;
  }
//DEBUG_COMPARE_RAM(__LINE__);

  read32le(&version, fp);
  if(version == 1)
  {
	  // attempt to load previous version's format
	  fclose(fp);
	  FCEUI_LoadMovie_v1(fname, _read_only);
	  return;
  }
  else if(version == MOVIE_VERSION)
  {}
  else
  {
	  // unsupported version
	  fclose(fp);
	  return;
  }

  fread(flags, 1, 4, fp);
  read32le(&framecount, fp);
  read32le(&rerecord_count, fp);
  read32le(&moviedatasize, fp);
  read32le(&savestate_offset, fp);
  read32le(&firstframeoffset, fp);
  if(fseek(fp, savestate_offset, SEEK_SET))
  {
   fclose(fp);
   return;
  }

//  FCEU_PrintError("flags[0] & MOVIE_FLAG_NOSYNCHACK=%d",flags[0] & MOVIE_FLAG_NOSYNCHACK);
  if(flags[0] & MOVIE_FLAG_NOSYNCHACK)
	  movieSyncHackOn=0;
  else
	  movieSyncHackOn=1;

  if(flags[0] & MOVIE_FLAG_PAL)
  {
	FCEUI_SetVidSystem(1);
  }
  else
  {
	FCEUI_SetVidSystem(0);
  }
 

 // fully reload the game to reinitialize everything before playing any movie
 // to try fixing nondeterministic playback of some games
 {
	extern char lastLoadedGameName [2048];
	extern int disableBatteryLoading, suppressAddPowerCommand;
	suppressAddPowerCommand=1;
	suppressMovieStop=1;
	{
		FCEUGI * gi = FCEUI_LoadGame(lastLoadedGameName, 0);
		if(!gi)
			PowerNES();
	}
	suppressMovieStop=0;
	suppressAddPowerCommand=0;
 }

 if(!FCEUSS_LoadFP(fp,1)) return;
  if(flags[0] & MOVIE_FLAG_PAL)
  {
	FCEUI_SetVidSystem(1);
  }
  else
  {
	FCEUI_SetVidSystem(0);
  }
 ResetInputTypes();

 fseek(fp, firstframeoffset, SEEK_SET);
 moviedata = (uint8*)realloc(moviedata, moviedatasize);
 fread(moviedata, 1, moviedatasize, fp);

 framecount = 0;		// movies start at frame 0!
 frameptr = 0;
 current = CurrentMovie;
 slots[current] = fp;

 memset(joop,0,sizeof(joop));
 current = -1 - current;
 framets=0;
 nextts=0;
 nextd = -1;

 MovieStatus[CurrentMovie] = 1;
 if(!fname)
  FCEUI_SelectMovie(CurrentMovie,1);       /* Quick hack to display status. */
 else
  FCEU_DispMessage("Movie playback started.");

 strcpy(curMovieFilename, origname);
}

void FCEUI_SaveMovie(char *fname, uint8 flags, const char* metadata)
{
 FILE *fp;
 char *fn;
 int poweron=0;
 uint8 padding[4] = {0,0,0,0};
 int n_padding;

 FCEUI_StopMovie();

 char origname[512];
 if(fname)
 {
  fp = FCEUD_UTF8fopen(fname, "wb");
  strcpy(origname,fname);
 }
 else
 {
  fp=FCEUD_UTF8fopen(fn=FCEU_MakeFName(FCEUMKF_MOVIE,CurrentMovie,0),"wb");
  strcpy(origname,fn);
  free(fn);
 }

 if(!fp) return;

 // don't need the movieSyncHackOn sync hack for newly recorded movies
 flags |= MOVIE_FLAG_NOSYNCHACK;
 resetDMCacc=movieSyncHackOn=0;

 // add PAL flag
 if(FCEUI_GetCurrentVidSystem(0,0))
  flags |= MOVIE_FLAG_PAL;

 if(flags & MOVIE_FLAG_FROM_POWERON)
 {
	 poweron=1;
	 flags &= ~MOVIE_FLAG_FROM_POWERON;
	 flags |= MOVIE_FLAG_FROM_RESET;
 }

 // write header
 write32le(MOVIE_MAGIC, fp);
 write32le(MOVIE_VERSION, fp);
 fputc(flags, fp);
 fputc(0, fp);                      // reserved
 fputc(0, fp);                      // reserved
 fputc(0, fp);                      // reserved
 write32le(0, fp);                  // leave room for length frames
 write32le(0, fp);                  // leave room for rerecord count
 write32le(0, fp);                  // leave room for movie data size
 write32le(0, fp);                  // leave room for savestate_offset
 write32le(0, fp);                  // leave room for offset_to_controller_data
 fwrite(FCEUGameInfo->MD5, 1, 16, fp);	// write ROM checksum
 write32le(FCEU_VERSION_NUMERIC, fp);	// write emu version used
 fputs(FileBase, fp);					// write ROM name used
 fputc(0, fp);
 if(metadata)
 {
  if(strlen(metadata) < MOVIE_MAX_METADATA)
   fputs(metadata, fp);
  else
   fwrite(metadata, 1, MOVIE_MAX_METADATA-1, fp);
 }
 fputc(0, fp);

 // add padding
 n_padding = (4 - (ftell(fp) & 0x3)) & 0x3;
 fwrite(padding, 1, n_padding, fp);

 if(flags & MOVIE_FLAG_FROM_RESET)
 {
	 if(poweron)
	 {
		// make a for-movie-recording power-on clear the game's save data, too
		// (note: FCEU makes a save state immediately after this and that loads that on movie playback)
		extern char lastLoadedGameName [2048];
		extern int disableBatteryLoading, suppressAddPowerCommand;
		suppressAddPowerCommand=1;
		disableBatteryLoading=1;
		suppressMovieStop=1;
		{
			// NOTE:  this will NOT write an FCEUNPCMD_POWER into the movie file
			FCEUGI * gi = FCEUI_LoadGame(lastLoadedGameName, 0);
			if(!gi)
				PowerNES(); // and neither will this, if it can even happen
		}
		suppressMovieStop=0;
		disableBatteryLoading=0;
		suppressAddPowerCommand=0;
	 }
 }

 savestate_offset = ftell(fp);
 FCEUSS_SaveFP(fp);
 fseek(fp, 0, SEEK_END);

 ResetInputTypes();

 // add padding
 n_padding = (4 - (ftell(fp) & 0x3)) & 0x3;
 fwrite(padding, 1, n_padding, fp);

 firstframeoffset = ftell(fp);

 // finish header
 fseek(fp, 24, SEEK_SET);			// offset_to_savestate offset
 write32le(savestate_offset, fp);
 write32le(firstframeoffset, fp);

 fseek(fp, firstframeoffset, SEEK_SET);

 // set recording flag
 current=CurrentMovie;

 movie_readonly = 0;
 frameptr = 0;
 framecount = 0;
 rerecord_count = 0;
 slots[current] = fp;
 memset(joop,0,sizeof(joop));
 current++;
 framets=0;
 nextd = -1;

 // trigger a reset
 if(flags & MOVIE_FLAG_FROM_RESET)
 {
	 if(poweron)
	 {
		PowerNES();							// NOTE:  this will write an FCEUNPCMD_POWER into the movie file
	 }
	 else
		ResetNES();							// NOTE:  this will write an FCEUNPCMD_RESET into the movie file
 }
 if(!fname)
  FCEUI_SelectMovie(CurrentMovie,1);       /* Quick hack to display status. */
 else
  FCEU_DispMessage("Movie recording started.");

 strcpy(curMovieFilename, origname);
}

static void movie_writechar(int c)
{
 if(frameptr == moviedatasize)
 {
  moviedatasize += 4096;
  moviedata = (uint8*)realloc(moviedata, moviedatasize);
 }
 moviedata[frameptr++] = (uint8)(c & 0xff);
 fputc(c, slots[current - 1]);
}

static int movie_readchar()
{
 if(frameptr >= moviedatasize)
 {
  return -1;
 }
 return (int)(moviedata[frameptr++]);
}

static void DoEncode(int joy, int button, int dummy)
{
 uint8 d;

 d = 0;

 if(framets >= 65536)
  d = 3 << 5;
 else if(framets >= 256)
  d = 2 << 5;
 else if(framets > 0)
  d = 1 << 5;

 if(dummy) d|=0x80;

 d |= joy << 3;
 d |= button;

 movie_writechar(d);
 //printf("Wr: %02x, %d\n",d,slots[current-1]);
 while(framets)
 {
  movie_writechar(framets & 0xff);
  //printf("Wrts: %02x\n",framets & 0xff);
  framets >>= 8;
 }
}

// TODO: make this function legible! (what are all these magic numbers and weirdly named variables and crazy unexplained loops?)
void FCEUMOV_AddJoy(uint8 *js, int SkipFlush)
{
 int x,y;

// if(!current) return;   // Not playback nor recording.

 if(current < 0)    // Playback
 {
  while(nextts == framets || nextd == -1)
  {
   int tmp,ti;
   uint8 d;

   if(nextd != -1)
   {
    if(nextd&0x80)
    {
    //puts("Egads");
     FCEU_DoSimpleCommand(nextd&0x1F);
    }
    else
     joop[(nextd >> 3)&0x3] ^= 1 << (nextd&0x7);
   }


   tmp = movie_readchar();
   d = tmp;

   if(tmp < 0)
   {
    StopPlayback();
    memcpy(&cur_input_display,js,4);
    return;
   }

   nextts = 0;
   tmp >>= 5;
   tmp &= 0x3;
   ti=0;

   int tmpfix = tmp;
   while(tmp--) { nextts |= movie_readchar() << (ti * 8); ti++; }

   // This fixes a bug in movies recorded before version 0.98.11
   // It's probably not necessary, but it may keep away CRAZY PEOPLE who recorded
   // movies on <= 0.98.10 and don't work on playback.
   if(tmpfix == 1 && !nextts) 
   {nextts |= movie_readchar()<<8; }
   else if(tmpfix == 2 && !nextts) {nextts |= movie_readchar()<<16; }

   if(nextd != -1)
    framets = 0;
   nextd = d;
  }

  memcpy(js,joop,4);
 }
 else if(current > 0)			// Recording 
 {
    // flush header info every 300 frames in case of crash
	//Don't if in bot mode--we don't need extra overhead
	if(!SkipFlush)
	{
		static int fcounter=0;
		fcounter++;
		if(!(fcounter%300))
			MovieFlushHeader();
	}

  for(x=0;x<4;x++)
  {
   if(js[x] != joop[x])
   {
    for(y=0;y<8;y++)
     if((js[x] ^ joop[x]) & (1 << y))
      DoEncode(x, y, 0);
    joop[x] = js[x];
   }
   else if(framets == ((1<<24)-1)) DoEncode(0,0,1); // Overflow will happen, so do dummy update. 
  }
 }

 if(current)
 {
  framets++;
  framecount++;
 }

 memcpy(&cur_input_display,js,4);

 //Stop the movie at a specified frame 
 if(current < 0 && FCEUMOV_ShouldPause() && FCEUI_EmulationPaused()==0)
 {
	 FCEUI_ToggleEmulationPause();
 }

}

void FCEUMOV_AddCommand(int cmd)
{
 if(current <= 0) return;   // Return if not recording a movie
 //printf("%d\n",cmd);
 DoEncode((cmd>>3)&0x3,cmd&0x7,1);
}

void FCEUMOV_CheckMovies(void)
{
        FILE *st=NULL;
        char *fn;
        int ssel;
        
        for(ssel=0;ssel<10;ssel++)
        {
         st=FCEUD_UTF8fopen(fn=FCEU_MakeFName(FCEUMKF_MOVIE,ssel,0),"rb");
         free(fn);
         if(st)
         {
          MovieStatus[ssel]=1;
          fclose(st);
         }   
         else
          MovieStatus[ssel]=0;
        }

}

void FCEUI_SelectMovieNext(int n)
{
	if(n>0)
		CurrentMovie=(CurrentMovie+1)%10;
	else
		CurrentMovie=(CurrentMovie+9)%10;
	FCEUI_SelectMovie(CurrentMovie, 1);
}


int FCEUI_SelectMovie(int w, int show)
{
 int oldslot=CurrentMovie;
 if(w == -1) { MovieShow = 0; return 0; } //mbg merge 7/17/06 had to add return value
 FCEUI_SelectState(-1,0);

 CurrentMovie=w;
 MovieShow=180;

 if(show)
 {
  MovieShow=180;
  if(current > 0)
   FCEU_DispMessage("-recording movie %d-",current-1);
  else if (current < 0)
   FCEU_DispMessage("-playing movie %d-",-1 - current);
  else
   FCEU_DispMessage("-select movie-");
 }
 return 0; //mbg merge 7/17/06 had to add return value
}

int movcounter=0;

void FCEU_DrawMovies(uint8 *XBuf)
{
	int frameDisplayOn = current != 0 && frame_display;
	extern int howlong;
#if MSVC	
	extern int32 fps_scale;
#else
	int32 fps_scale=256;
#endif
	int howl=(180-(FCEUI_EmulationPaused()?(60):(20*fps_scale/256)));
	if(howl>176) howl=180;
	if(howl<1) howl=1;
	if((howlong<howl || movcounter)
	&& (frameDisplayOn && 
	(!movcounter || last_frame_display!=framecount)))
	//|| input_display && (!movcounter || last_input_display!=cur_input_display)))
	{
		//Old input display code
		/*
		char inputstr [32];
		if(input_display)
		{
			uint32 c = cur_input_display;
			sprintf(inputstr, "%c%c%c%c%c%c%c%c %c%c%c%c%c%c%c%c",
				(c&0x40)?'<':' ', (c&0x10)?'^':' ', (c&0x80)?'>':' ', (c&0x20)?'v':' ',
				(c&0x01)?'A':' ', (c&0x02)?'B':' ', (c&0x08)?'S':' ', (c&0x04)?'s':' ',
				(c&0x4000)?'<':' ', (c&0x1000)?'^':' ', (c&0x8000)?'>':' ', (c&0x2000)?'v':' ',
				(c&0x0100)?'A':' ', (c&0x0200)?'B':' ', (c&0x0800)?'S':' ', (c&0x0400)?'s':' ');
			if(!(c&0xff00))
				inputstr[8] = '\0';
		}
		if(frameDisplayOn && !input_display)
			FCEU_DispMessage("%s frame %u",current >= 0?"Recording":"Playing",framecount);
		else if(input_display && !frameDisplayOn)
			FCEU_DispMessage("Input: %s",inputstr);
		else //if(input_display && frame_display)
			FCEU_DispMessage("%s %u %s",current >= 0?"Recording":"Playing",framecount,inputstr);
		*/
		if(frameDisplayOn)
		{
			FCEU_DispMessage("%s frame %u",current >= 0?"Recording":"Playing",framecount);
		}
		last_frame_display = framecount;
		last_input_display = cur_input_display;
		movcounter=180-1;
		return;
	}

	if(movcounter) movcounter--;

	if(!MovieShow) return;

	FCEU_DrawNumberRow(XBuf,MovieStatus, CurrentMovie);
	MovieShow--;
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
 // if this savestate was made while replaying,
 // we need to "undo" nextd and nextts
 if(nextd != -1)
 {
  int d = 1;
  if(nextts > 65536)
   d = 4;
  else if(nextts > 256)
   d = 3;
  else if(nextts > 0)
   d = 2;
  frameptr -= d;
  nextd = -1;
 }

// if(current > 0 || (!movie_readonly && current < 0))            /* Recording or Playback (!read-only) */
 if(current!=0 && !movie_readonly)
 {
  // copy movie data from savestate
  moviedata = (uint8*)realloc(moviedata, size);
  moviedatasize = size;
  if(size && fread(moviedata, 1, size, st)<size)
   return 0;
  if(current < 0)                   // switch to recording
   current = -current;
  fseek(slots[current - 1], firstframeoffset, SEEK_SET);
  fwrite(moviedata, 1, frameptr, slots[current - 1]);
  if(!FCEU_BotMode())
  {
	  rerecord_count++;
  }
 }
// else if(current < 0)       /* Playback (read-only) */
 else if(current!=0 && movie_readonly)
 {
  if(current > 0)                   // switch to playback
   current = -current;
  // allow frameptr to be updated but keep movie data
  fseek(st, size, SEEK_CUR);
  // prevent seeking beyond the end of the movie
  if(frameptr > moviedatasize)
   frameptr = moviedatasize;
 }
 else						/* Neither recording or replaying */
 {
  // skip movie data
  fseek(st, size, SEEK_CUR);
 }

 load_successful=1;
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

char* FCEUI_MovieGetCurrentName(int addSlotNumber)
{
 return FCEU_MakeFName(FCEUMKF_MOVIE,(addSlotNumber ? CurrentMovie : -1),0);
}

int FCEUI_IsMovieActive(void)
{
  return current;
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

char lastMovieInfoFilename [512] = {'\0',};
int FCEUI_MovieGetInfo(const char* fname, MOVIE_INFO* info)
{
	MovieFlushHeader();

	char buffer [512];
	fname = (const char*)convertToFCM(fname,buffer);
	strncpy(lastMovieInfoFilename, fname, 512);

// main get info part of function
{
 uint32 magic;
 uint32 version;
 uint8 _flags[4];

 FILE* fp = FCEUD_UTF8fopen(fname, "rb");
 if(!fp)
  return 0;

 read32le(&magic, fp);
 if(magic != MOVIE_MAGIC)
 {
  fclose(fp);
  return 0;
 }

 read32le(&version, fp);
 if(version != MOVIE_VERSION)
 {
  fclose(fp);
  if(version == 1)
   return FCEUI_MovieGetInfo_v1(fname, info);
  else
   return 0;
 }

 info->movie_version = MOVIE_VERSION;

 fread(_flags, 1, 4, fp);

 info->flags = _flags[0];
 read32le(&info->num_frames, fp);
 read32le(&info->rerecord_count, fp);

 if(access(fname, W_OK))
  info->read_only = 1;
 else
  info->read_only = 0;

 fseek(fp, 12, SEEK_CUR);			// skip movie_data_size, offset_to_savestate, and offset_to_movie_data

 fread(info->md5_of_rom_used, 1, 16, fp);
 info->md5_of_rom_used_present = 1;

 read32le(&info->emu_version_used, fp);

 // I probably could have planned this better...
 {
  char str[256];
  size_t r;
  uint32 p; //mbg merge 7/17/06 change to uint32
  int p2=0;
  char last_c=32;

  if(info->name_of_rom_used && info->name_of_rom_used_size)
   info->name_of_rom_used[0]='\0';

  r=fread(str, 1, 256, fp);
  while(r > 0)
  {
   for(p=0; p<r && last_c != '\0'; ++p)
   {
    if(info->name_of_rom_used && info->name_of_rom_used_size && (p2 < info->name_of_rom_used_size-1))
    {
     info->name_of_rom_used[p2]=str[p];
     p2++;
	 last_c=str[p];
    }
   }
   if(p<r)
   {
    memmove(str, str+p, r-p);
    r -= p;
    break;
   }
   r=fread(str, 1, 256, fp);
  }

  p2=0;
  last_c=32;

  if(info->metadata && info->metadata_size)
   info->metadata[0]='\0';

  while(r > 0)
  {
   for(p=0; p<r && last_c != '\0'; ++p)
   {
    if(info->metadata && info->metadata_size && (p2 < info->metadata_size-1))
    {
     info->metadata[p2]=str[p];
     p2++;
	 last_c=str[p];
    }
   }
   if(p != r)
    break;
   r=fread(str, 1, 256, fp);
  }

  if(r<=0)
  {
   // somehow failed to read romname and metadata
   fclose(fp);
   return 0;
  }
 }

 // check what hacks are necessary
  fseek(fp, 24, SEEK_SET);			// offset_to_savestate offset
  uint32 temp_savestate_offset;
  read32le(&temp_savestate_offset, fp);
  if(fseek(fp, temp_savestate_offset, SEEK_SET))
  {
   fclose(fp);
   return 0;
  }
  if(!FCEUSS_LoadFP(fp,2)) return 0; // 2 -> don't really load, just load to find what's there then load backup

 fclose(fp);
 return 1;
}
}
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

#define MOVIE_V1_HEADER_SIZE	32

static void FCEUI_LoadMovie_v1(char *fname, int _read_only)
{
 FILE *fp;
 char *fn = NULL;

 FCEUI_StopMovie();

 if(!fname)
  fname = fn = FCEU_MakeFName(FCEUMKF_MOVIE,CurrentMovie,0);

 // check movie_readonly
 movie_readonly = _read_only;
 if(access(fname, W_OK))
  movie_readonly = 2;

 fp = FCEUD_UTF8fopen(fname, (movie_readonly>=2) ? "rb" : "r+b");

 if(fn)
 {
  free(fn);
  fname = NULL;
 }

 if(!fp) return;

 // read header
 {
  uint32 magic;
  uint32 version;
  uint8 flags[4];
  uint32 fc;

  read32le(&magic, fp);
  if(magic != MOVIE_MAGIC)
  {
   fclose(fp);
   return;
  }

  read32le(&version, fp);
  if(version != 1)
  {
   fclose(fp);
   return;
  }

  fread(flags, 1, 4, fp);
  read32le(&fc, fp);
  read32le(&rerecord_count, fp);
  read32le(&moviedatasize, fp);
  read32le(&savestate_offset, fp);
  read32le(&firstframeoffset, fp);
  if(fseek(fp, savestate_offset, SEEK_SET))
  {
   fclose(fp);
   return;
  }

  if(flags[0] & MOVIE_FLAG_NOSYNCHACK)
	  movieSyncHackOn=0;
  else
	  movieSyncHackOn=1;
 }

 // fully reload the game to reinitialize everything before playing any movie
 // to try fixing nondeterministic playback of some games
 {
	extern char lastLoadedGameName [2048];
	extern int disableBatteryLoading, suppressAddPowerCommand;
	suppressAddPowerCommand=1;
	suppressMovieStop=1;
	{
		FCEUGI * gi = FCEUI_LoadGame(lastLoadedGameName, 0);
		if(!gi)
			PowerNES();
	}
	suppressMovieStop=0;
	suppressAddPowerCommand=0;
 }

 if(!FCEUSS_LoadFP(fp,1)) return;

 ResetInputTypes();

 fseek(fp, firstframeoffset, SEEK_SET);
 moviedata = (uint8*)realloc(moviedata, moviedatasize);
 fread(moviedata, 1, moviedatasize, fp);

 framecount = 0;		// movies start at frame 0!
 frameptr = 0;
 current = CurrentMovie;
 slots[current] = fp;

 memset(joop,0,sizeof(joop));
 current = -1 - current;
 framets=0;
 nextts=0;
 nextd = -1;
 MovieStatus[CurrentMovie] = 1;
 if(!fname)
  FCEUI_SelectMovie(CurrentMovie,1);       /* Quick hack to display status. */
 else
  FCEU_DispMessage("Movie playback started.");
}

static int FCEUI_MovieGetInfo_v1(const char* fname, MOVIE_INFO* info)
{
 uint32 magic;
 uint32 version;
 uint8 _flags[4];
 uint32 savestateoffset;
 uint8 tmp[MOVIE_MAX_METADATA<<1];
 int metadata_length;

 FILE* fp = FCEUD_UTF8fopen(fname, "rb");
 if(!fp)
  return 0;

 read32le(&magic, fp);
 if(magic != MOVIE_MAGIC)
 {
  fclose(fp);
  return 0;
 }

 read32le(&version, fp);
 if(version != 1)
 {
  fclose(fp);
  return 0;
 }

 info->movie_version = 1;
 info->emu_version_used = 0;			// unknown

 fread(_flags, 1, 4, fp);

 info->flags = _flags[0];
 read32le(&info->num_frames, fp);
 read32le(&info->rerecord_count, fp);

 if(access(fname, W_OK))
  info->read_only = 1;
 else
  info->read_only = 0;

 fseek(fp, 4, SEEK_CUR);
 read32le(&savestateoffset, fp);

 metadata_length = (int)savestateoffset - MOVIE_V1_HEADER_SIZE;
 if(metadata_length > 0)
 {
  //int i; //mbg merge 7/17/06 removed

  metadata_length >>= 1;
  if(metadata_length >= MOVIE_MAX_METADATA)
   metadata_length = MOVIE_MAX_METADATA-1;

  fseek(fp, MOVIE_V1_HEADER_SIZE, SEEK_SET);
  fread(tmp, 1, metadata_length<<1, fp);
 }

 // turn old ucs2 metadata into utf8
 if(info->metadata && info->metadata_size)
 {
	 char* ptr=info->metadata;
	 char* ptr_end=info->metadata+info->metadata_size-1;
	 int c_ptr=0;
	 while(ptr<ptr_end && c_ptr<metadata_length)
	 {
		 uint16 c=(tmp[c_ptr<<1] | (tmp[(c_ptr<<1)+1] << 8));
		 //mbg merge 7/17/06 changed to if..elseif
		 if(c<=0x7f)
			 *ptr++ = (char)(c&0x7f);
		 else if(c<=0x7FF)
			 if(ptr+1>=ptr_end)
				 ptr_end=ptr;
			 else
			 {
				 *ptr++=(0xc0 | (c>>6));
				 *ptr++=(0x80 | (c & 0x3f));
			 }
		 else
			 if(ptr+2>=ptr_end)
				 ptr_end=ptr;
			 else
			 {
				 *ptr++=(0xe0 | (c>>12));
				 *ptr++=(0x80 | ((c>>6) & 0x3f));
				 *ptr++=(0x80 | (c & 0x3f));
			 }

		 c_ptr++;
	 }
	 *ptr='\0';
 }

 // md5 info not available from v1
 info->md5_of_rom_used_present = 0;
 // rom name used for the movie not available from v1
 if(info->name_of_rom_used && info->name_of_rom_used_size)
  info->name_of_rom_used[0] = '\0';

 // check what hacks are necessary
  fseek(fp, 24, SEEK_SET);			// offset_to_savestate offset
  uint32 temp_savestate_offset;
  read32le(&temp_savestate_offset, fp);
  if(fseek(fp, temp_savestate_offset, SEEK_SET))
  {
   fclose(fp);
   return 0;
  }
  if(!FCEUSS_LoadFP(fp,2)) return 0; // 2 -> don't really load, just load to find what's there then load backup


 fclose(fp);
 return 1;
}

