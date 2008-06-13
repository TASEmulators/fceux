/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*  TODO: Add (better) file io error checking */
/*  TODO: Change save state file format. */

#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h> //mbg merge 7/17/06 removed

#include <vector>

#include "types.h"
#include "x6502.h"
#include "fceu.h"
#include "sound.h"
#include "utils/endian.h"
#include "utils/memory.h"
#include "utils/memorystream.h"
#include "utils/xstring.h"
#include "file.h"
#include "fds.h"
#include "state.h"
#include "movie.h"
#include "ppu.h"
#include "netplay.h"
#include "video.h"
#include "input.h"
#include "zlib.h"

static void (*SPreSave)(void);
static void (*SPostSave)(void);

static int SaveStateStatus[10];
static int StateShow;

#define SFMDATA_SIZE (64)
static SFORMAT SFMDATA[SFMDATA_SIZE];
static int SFEXINDEX;

#define RLSB 		FCEUSTATE_RLSB	//0x80000000


extern SFORMAT FCEUPPU_STATEINFO[];
extern SFORMAT FCEUSND_STATEINFO[];
extern SFORMAT FCEUCTRL_STATEINFO[];
extern SFORMAT FCEUMOV_STATEINFO[];


SFORMAT SFCPU[]={
 { &X.PC, 2|RLSB, "PC\0"},
 { &X.A, 1, "A\0\0"},
 { &X.P, 1, "P\0\0"},
 { &X.X, 1, "X\0\0"},
 { &X.Y, 1, "Y\0\0"},
 { &X.S, 1, "S\0\0"},
 { &RAM, 0x800 | FCEUSTATE_INDIRECT, "RAM", },
 { 0 }
};

SFORMAT SFCPUC[]={
 { &X.jammed, 1, "JAMM"},
 { &X.IRQlow, 4|RLSB, "IQLB"},
 { &X.tcount, 4|RLSB, "ICoa"},
 { &X.count,  4|RLSB, "ICou"},
 { &timestampbase, sizeof(timestampbase) | RLSB, "TSBS"},
 { &X.mooPI, 1, "MooP"}, // alternative to the "quick and dirty hack"
 { 0 }
};

static int SubWrite(std::ostream* os, SFORMAT *sf)
{
 uint32 acc=0;

 while(sf->v)
 {
  if(sf->s==~0)		//Link to another struct
  {
   uint32 tmp;

   if(!(tmp=SubWrite(os,(SFORMAT *)sf->v)))
    return(0);
   acc+=tmp;
   sf++;
   continue;
  }

  acc+=8;			//Description + size
  acc+=sf->s&(~FCEUSTATE_FLAGS);

  if(os)			//Are we writing or calculating the size of this block?
  {
	  os->write(sf->desc,4);
   write32le(sf->s&(~FCEUSTATE_FLAGS),os);

   #ifndef LSB_FIRST
   if(sf->s&RLSB)
    FlipByteOrder(sf->v,sf->s&(~FCEUSTATE_FLAGS));
   #endif
 
   if(sf->s&FCEUSTATE_INDIRECT)
	   os->write(*(char **)sf->v,sf->s&(~FCEUSTATE_FLAGS));
   else
	os->write((char*)sf->v,sf->s&(~FCEUSTATE_FLAGS));

   //Now restore the original byte order.
   #ifndef LSB_FIRST
   if(sf->s&RLSB)   
    FlipByteOrder(sf->v,sf->s&(~FCEUSTATE_FLAGS));
   #endif
  }
  sf++; 
 }

 return(acc);
}

static int SubWrite(FILE *st, SFORMAT *sf)
{
 uint32 acc=0;

 while(sf->v)
 {
  if(sf->s==~0)		//Link to another struct
  {
   uint32 tmp;

   if(!(tmp=SubWrite(st,(SFORMAT *)sf->v)))
    return(0);
   acc+=tmp;
   sf++;
   continue;
  }

  acc+=8;			//Description + size
  acc+=sf->s&(~FCEUSTATE_FLAGS);

  if(st)			// Are we writing or calculating the size of this block?
  {
   fwrite(sf->desc,1,4,st);
   write32le(sf->s&(~FCEUSTATE_FLAGS),st);

   #ifndef LSB_FIRST
   if(sf->s&RLSB)
    FlipByteOrder(sf->v,sf->s&(~FCEUSTATE_FLAGS));
   #endif
 
   fwrite((uint8 *)sf->v,1,sf->s&(~FCEUSTATE_FLAGS),st);
   //Now restore the original byte order.
   #ifndef LSB_FIRST
   if(sf->s&RLSB)   
    FlipByteOrder(sf->v,sf->s&(~FCEUSTATE_FLAGS));
   #endif
  }
  sf++; 
 }

 return(acc);
}

static int WriteStateChunk(std::ostream* os, int type, SFORMAT *sf)
{
	os->put(type);
	int bsize = SubWrite((std::ostream*)0,sf);
	write32le(bsize,os);

	if(!SubWrite(os,sf))
	{
		return 5;
	}
	return (bsize+5);
}

static int WriteStateChunk(FILE *st, int type, SFORMAT *sf)
{
 int bsize;

 fputc(type,st);
 
 bsize=SubWrite((FILE*)0,sf);
 write32le(bsize,st);		

 if(!SubWrite(st,sf))
 {
	 return 5;
 }
 return (bsize+5);
}

static SFORMAT *CheckS(SFORMAT *sf, uint32 tsize, char *desc)
{
 while(sf->v)
 {
  if(sf->s==~0)		/* Link to another SFORMAT structure. */
  {
   SFORMAT *tmp;
   if((tmp= CheckS((SFORMAT *)sf->v, tsize, desc) ))
    return(tmp);
   sf++;
   continue;
  }
  if(!memcmp(desc,sf->desc,4))
  {
   if(tsize!=(sf->s&(~FCEUSTATE_FLAGS)))
    return(0);
   return(sf);
  }
  sf++;
 }
 return(0);
}

static int scan_chunks=0;
int suppress_scan_chunks=0;

static int ReadStateChunk(FILE *st, SFORMAT *sf, int size)
{
 if(scan_chunks)
   return fseek(st,size,SEEK_CUR) == 0;

 SFORMAT *tmp;
 int temp;
 temp=ftell(st);

 while(ftell(st)<temp+size)
 {
  uint32 tsize;
  char toa[4];
  if(fread(toa,1,4,st)<=0)
   return 0;

  read32le(&tsize,st);

  if((tmp=CheckS(sf,tsize,toa)))
  {
	  if(tmp->s&FCEUSTATE_INDIRECT)
		  fread(*(uint8 **)tmp->v,1,tmp->s&(~FCEUSTATE_FLAGS),st);
	  else
		fread((uint8 *)tmp->v,1,tmp->s&(~FCEUSTATE_FLAGS),st);

   #ifndef LSB_FIRST
   if(tmp->s&RLSB)
    FlipByteOrder(tmp->v,tmp->s&(~FCEUSTATE_FLAGS));
   #endif
  }
  else
   fseek(st,tsize,SEEK_CUR);
 } // while(...)
 return 1;
}

static int read_sfcpuc=0, read_snd=0;

void FCEUD_BlitScreen(uint8 *XBuf); //mbg merge 7/17/06 YUCKY had to add
void UpdateFCEUWindow(void);  //mbg merge 7/17/06 YUCKY had to add
static int ReadStateChunks(FILE *st, int32 totalsize)
{
 int t;
 uint32 size;
 int ret=1;
 int warned=0;

 read_sfcpuc=0;
 read_snd=0;

// int moo=X.mooPI;
 if(!scan_chunks)
   X.mooPI=/*X.P*/0xFF;

 while(totalsize > 0)
 {
  t=fgetc(st);
  if(t==EOF) break;
  if(!read32le(&size,st)) break;
  totalsize -= size + 5;

  switch(t)
  {
   case 1:if(!ReadStateChunk(st,SFCPU,size)) ret=0;break;
   case 3:if(!ReadStateChunk(st,FCEUPPU_STATEINFO,size)) ret=0;break;
   case 4:if(!ReadStateChunk(st,FCEUCTRL_STATEINFO,size)) ret=0;break;
   case 7:if(!FCEUMOV_ReadState(st,size))                ret=0;break;
   case 0x10:if(!ReadStateChunk(st,SFMDATA,size)) ret=0; break;

   // now it gets hackier:
   case 5:
	   if(!ReadStateChunk(st,FCEUSND_STATEINFO,size))
		ret=0;
	   else
		read_snd=1;
	   break;
   case 6:
	   if(FCEUMOV_Mode(MOVIEMODE_PLAY|MOVIEMODE_RECORD))
	   {
		   if(!ReadStateChunk(st,FCEUMOV_STATEINFO,size)) ret=0;
	   }
	   else
	   {
		   if(fseek(st,size,SEEK_CUR)!=0) ret=0;
	   }
		break;
   case 8:
		// load back buffer
		{
			extern uint8 *XBackBuf;
	        if(size != fread(XBackBuf,1,size,st))
				ret = 0;
#ifdef WIN32
			else
			{
				FCEUD_BlitScreen(XBuf);
				UpdateFCEUWindow();
			}
#endif
		}
		break;
   case 2:
   {
	   if(!ReadStateChunk(st,SFCPUC,size))
		ret=0;
	   else
		read_sfcpuc=1;
   }  break;
   default:
	   // for somebody's sanity's sake, at least warn about it:
	   if(!warned && !scan_chunks)
	   {
		   char str [256];
		   sprintf(str, "Warning: Found unknown save chunk of type %d.\nThis could indicate the save state is corrupted\nor made with a different (incompatible) emulator version.", t);
		   FCEUD_PrintError(str);
		   warned=1;
	   }
	   if(fseek(st,size,SEEK_CUR)<0) goto endo;break;
  }
 }
 endo:

 if(X.mooPI==0xFF && !scan_chunks)
 {
//	 FCEU_PrintError("prevmoo=%d, p=%d",moo,X.P);
   X.mooPI=X.P; // "Quick and dirty hack." //begone
 }

 extern int resetDMCacc;
 if(read_snd)
	 resetDMCacc=0;
 else
	 resetDMCacc=1;

 return ret;
}

int CurrentState=1;
extern int geniestage;

bool FCEUSS_SaveFP(FILE* fp, int compressionLevel)
{
	memorystream ms;
	bool ret = FCEUSS_SaveMS(&ms,compressionLevel);
	fwrite(ms.buf(),1,ms.size(),fp);
	return ret;
}

bool FCEUSS_SaveMS(std::ostream* outstream, int compressionLevel)
{
	//a temp memory stream. we'll dump some data here and then compress
	//TODO - support dumping directly without compressing to save a buffer copy

	memorystream ms;
	std::ostream* os = (std::ostream*)&ms;

	uint32 totalsize = 0;

	FCEUPPU_SaveState();
	FCEUSND_SaveState();
	totalsize=WriteStateChunk(os,1,SFCPU);
	totalsize+=WriteStateChunk(os,2,SFCPUC);
	totalsize+=WriteStateChunk(os,3,FCEUPPU_STATEINFO);
	totalsize+=WriteStateChunk(os,4,FCEUCTRL_STATEINFO);
	totalsize+=WriteStateChunk(os,5,FCEUSND_STATEINFO);
	if(FCEUMOV_Mode(MOVIEMODE_PLAY|MOVIEMODE_RECORD))
	{
		totalsize+=WriteStateChunk(os,6,FCEUMOV_STATEINFO);

		//MBG tasedit HACK HACK HACK!
		//do not save the movie state if we are in tasedit! that is a huge waste of time and space!
		if(!FCEUMOV_Mode(MOVIEMODE_TASEDIT))
		{
			uint32 size = FCEUMOV_WriteState((std::ostream*)0);
			os->put(7);
			write32le(size, os);
			FCEUMOV_WriteState(os);
			totalsize += 5 + size;
		}
	}
	// save back buffer
	{
		extern uint8 *XBackBuf;
		uint32 size = 256 * 256 + 8;
		os->put(8);
		write32le(size, os);
		os->write((char*)XBackBuf,size);
		totalsize += 5 + size;
	}

	if(SPreSave) SPreSave();
	totalsize+=WriteStateChunk(os,0x10,SFMDATA);
	if(SPreSave) SPostSave();

	//save the length of the file
	int len = ms.size();

	//sanity check: len and totalsize should be the same
	if(len != totalsize)
	{
		FCEUD_PrintError("sanity violation: len != totalsize");
		return false;
	}

	int error = Z_OK;
	uint8* cbuf = (uint8*)ms.buf();
	uLongf comprlen = -1;
	if(compressionLevel != Z_NO_COMPRESSION)
	{
		//worst case compression.
		//zlib says "0.1% larger than sourceLen plus 12 bytes"
		comprlen = (len>>9)+12 + len;
		cbuf = new uint8[comprlen]; 
		error = compress2(cbuf,&comprlen,(uint8*)ms.buf(),len,compressionLevel);
	}

	//dump the header
	uint8 header[16]="FCSX";
	FCEU_en32lsb(header+4, totalsize);
	FCEU_en32lsb(header+8, FCEU_VERSION_NUMERIC);
	FCEU_en32lsb(header+12, comprlen);

	//dump it to the destination file
	outstream->write((char*)header,16);
	outstream->write((char*)cbuf,comprlen==-1?totalsize:comprlen);

	if(cbuf != (uint8*)ms.buf()) delete[] cbuf;
	return error == Z_OK;
}


void FCEUSS_Save(char *fname)
{
	std::fstream* st = 0;
	char *fn;

	if(geniestage==1)
	{
		FCEU_DispMessage("Cannot save FCS in GG screen.");
		return;
	}

	if(fname)
		st =FCEUD_UTF8_fstream(fname, "wb");
	else
	{
		//FCEU_PrintError("daCurrentState=%d",CurrentState);
		st = FCEUD_UTF8_fstream(fn=FCEU_MakeFName(FCEUMKF_STATE,CurrentState,0),"wb");
		free(fn);
	}

	if(st == NULL)
	{
		FCEU_DispMessage("State %d save error.",CurrentState);
		return;
	}


	if(FCEUMOV_Mode(MOVIEMODE_INACTIVE))
		FCEUSS_SaveMS(st,-1);
	else
		FCEUSS_SaveMS(st,0);

	st->close();
	delete st;

	if(!fname)
	{
		SaveStateStatus[CurrentState]=1;
		FCEU_DispMessage("State %d saved.",CurrentState);
	}
}

bool FCEUSS_LoadFP(std::istream* is, ENUM_SSLOADPARAMS params)
{
	if(params==SSLOADPARAM_DUMMY && suppress_scan_chunks)
		return true;

	bool x;
	uint8 header[16];
	char* fn=0;

	//Make temporary savestate in case something screws up during the load
	if(params == SSLOADPARAM_BACKUP)
	{
		fn=FCEU_MakeFName(FCEUMKF_NPTEMP,0,0);
		FILE *fp;
		
		if((fp=fopen(fn,"wb")))
		{
			if(FCEUSS_SaveFP(fp,0))
			{
				fclose(fp);
			}
			else
			{
				fclose(fp);
				unlink(fn);
				free(fn);
				fn=0;
			}
		}
	}

	if(params!=SSLOADPARAM_DUMMY)
		FCEUMOV_PreLoad();

	//read and analyze the header
	is->read((char*)&header,16);
	if(memcmp(header,"FCSX",4))
		return false;
	int totalsize = FCEU_de32lsb(header + 4);
	int stateversion = FCEU_de32lsb(header + 8);
	int comprlen = FCEU_de32lsb(header + 12);

	std::vector<uint8> buf(totalsize);

	//not compressed:
	if(comprlen != -1)
	{
		//load the compressed chunk and decompress
		std::vector<uint8> cbuf(comprlen);
		is->read((char*)&cbuf[0],comprlen);

		uLongf uncomprlen = totalsize;
		int error = uncompress(&buf[0],&uncomprlen,&cbuf[0],comprlen);
		if(error != Z_OK || uncomprlen != totalsize)
			return false;
	}
	else
	{
		is->read((char*)&buf[0],totalsize);
	}

	//dump it back to a tempfile
	FILE* tmp = tmpfile();
	fwrite(&buf[0],1,totalsize,tmp);
	fseek(tmp,0,SEEK_SET);

	if(params == SSLOADPARAM_DUMMY)
	{
		scan_chunks=1;
	}
	
	x = ReadStateChunks(tmp,totalsize)!=0;
	
	if(params == SSLOADPARAM_DUMMY)
	{
		scan_chunks=0;
		fclose(tmp);
		return true;
	}

	//mbg 5/24/08 - we don't support old states, so this shouldnt matter.
	//if(read_sfcpuc && stateversion<9500)
	//	X.IRQlow=0;

	if(GameStateRestore)
	{
		GameStateRestore(stateversion);
	}
	if(x)
	{
		FCEUPPU_LoadState(stateversion);
		FCEUSND_LoadState(stateversion);  
		x=FCEUMOV_PostLoad()!=0;
	}

	if(fn)
	{
		if(!x || params == SSLOADPARAM_DUMMY)  //is make_backup==2 possible??  oh well.
		{
			/* Oops!  Load the temporary savestate */
			FILE *fp;
				
			if((fp=fopen(fn,"rb")))
			{
				FCEUSS_LoadFP(fp,SSLOADPARAM_NOBACKUP);
				fclose(fp);
			}
			unlink(fn);
		}
		free(fn);
	}

	fclose(tmp);
	return x;

	return false;
}

bool FCEUSS_LoadFP(FILE *st, ENUM_SSLOADPARAMS params)
{
	if(params==SSLOADPARAM_DUMMY && suppress_scan_chunks)
		return true;

	bool x;
	uint8 header[16];
	char* fn=0;

	//Make temporary savestate in case something screws up during the load
	if(params == SSLOADPARAM_BACKUP)
	{
		fn=FCEU_MakeFName(FCEUMKF_NPTEMP,0,0);
		FILE *fp;
		
		if((fp=fopen(fn,"wb")))
		{
			if(FCEUSS_SaveFP(fp,0))
			{
				fclose(fp);
			}
			else
			{
				fclose(fp);
				unlink(fn);
				free(fn);
				fn=0;
			}
		}
	}

	if(params!=SSLOADPARAM_DUMMY)
		FCEUMOV_PreLoad();
	
	//read and analyze the header
	fread(&header,1,16,st);
	if(memcmp(header,"FCSX",4))
		return false;
	int totalsize = FCEU_de32lsb(header + 4);
	int stateversion = FCEU_de32lsb(header + 8);
	int comprlen = FCEU_de32lsb(header + 12);

	//load the compressed chunk and decompress if necessary
	std::vector<uint8> buf(totalsize);
	if(comprlen == -1)
	{
		int ret = fread(&buf[0],1,totalsize,st);
		if(ret != totalsize)
			return false;
	}
	else
	{
		std::vector<uint8> cbuf(comprlen);
		if(fread(&cbuf[0],1,comprlen,st) != comprlen)
			return false;

		uLongf uncomprlen = totalsize;
		int error = uncompress(&buf[0],&uncomprlen,&cbuf[0],comprlen);
		if(error != Z_OK || uncomprlen != totalsize)
			return false;
	}

	//dump it back to a tempfile
	FILE* tmp = tmpfile();
	fwrite(&buf[0],1,totalsize,tmp);
	fseek(tmp,0,SEEK_SET);

	if(params == SSLOADPARAM_DUMMY)
	{
		scan_chunks=1;
	}
	
	x = ReadStateChunks(tmp,totalsize)!=0;
	
	if(params == SSLOADPARAM_DUMMY)
	{
		scan_chunks=0;
		fclose(tmp);
		return true;
	}

	//mbg 5/24/08 - we don't support old states, so this shouldnt matter.
	//if(read_sfcpuc && stateversion<9500)
	//	X.IRQlow=0;

	if(GameStateRestore)
	{
		GameStateRestore(stateversion);
	}
	if(x)
	{
		FCEUPPU_LoadState(stateversion);
		FCEUSND_LoadState(stateversion);  
		x=FCEUMOV_PostLoad()!=0;
	}

	if(fn)
	{
		if(!x || params == SSLOADPARAM_DUMMY)  //is make_backup==2 possible??  oh well.
		{
			/* Oops!  Load the temporary savestate */
			FILE *fp;
				
			if((fp=fopen(fn,"rb")))
			{
				FCEUSS_LoadFP(fp,SSLOADPARAM_NOBACKUP);
				fclose(fp);
			}
			unlink(fn);
		}
		free(fn);
	}

	fclose(tmp);
	return x;
}

int FCEUSS_Load(char *fname)
{
	FILE *st;
	char *fn;

	//mbg movie - this needs to be overhauled
	////this fixes read-only toggle problems
	//if(FCEUMOV_IsRecording()) {
	//	FCEUMOV_AddCommand(0);
	//	MovieFlushHeader();
	//}

	if(geniestage==1)
	{
		FCEU_DispMessage("Cannot load FCS in GG screen.");
		return(0);
	}
	if(fname)
	{
		st=FCEUD_UTF8fopen(fname, "rb");
	}
	else
	{
		st=FCEUD_UTF8fopen(fn=FCEU_MakeFName(FCEUMKF_STATE,CurrentState,fname),"rb");
		free(fn);
	}

	if(st == NULL)
	{
		FCEU_DispMessage("State %d load error.",CurrentState);
		SaveStateStatus[CurrentState]=0;
		return(0);
	}

	//If in bot mode, don't do a backup when loading.
	//Otherwise you eat at the hard disk, since so many
	//states are being loaded.
	if(FCEUSS_LoadFP(st,FCEU_BotMode()?SSLOADPARAM_NOBACKUP:SSLOADPARAM_BACKUP))
	{
		if(fname)
		{
			char szFilename[260]={0};
			splitpath(fname, 0, 0, szFilename, 0);
			FCEU_DispMessage("State %s loaded.",szFilename);
		}
		else
		{
			//This looks redudant to me... but why bother deleting it:)
			SaveStateStatus[CurrentState]=1;

			FCEU_DispMessage("State %d loaded.",CurrentState);
			SaveStateStatus[CurrentState]=1;
		}
		fclose(st);
		return(1);
	}   
	else
	{
		if(!fname)
		{
			SaveStateStatus[CurrentState]=1;
		}
		FCEU_DispMessage("Error(s) reading state %d!",CurrentState);
		fclose(st);
		return(0);
	}
}

void FCEUSS_CheckStates(void)
{
        FILE *st=NULL;
        char *fn;
        int ssel;

        for(ssel=0;ssel<10;ssel++)
        {
         st=FCEUD_UTF8fopen(fn=FCEU_MakeFName(FCEUMKF_STATE,ssel,0),"rb");
         free(fn);
         if(st)
         {
          SaveStateStatus[ssel]=1;
          fclose(st);
         }
         else
          SaveStateStatus[ssel]=0;
        }

	CurrentState=1;
	StateShow=0;
}

void ResetExState(void (*PreSave)(void), void (*PostSave)(void))
{
 int x;
 for(x=0;x<SFEXINDEX;x++)
 {
  if(SFMDATA[x].desc)
   free(SFMDATA[x].desc);
 }
 SPreSave = PreSave;
 SPostSave = PostSave;
 SFEXINDEX=0;
}

void AddExState(void *v, uint32 s, int type, char *desc)
{
 if(desc)
 {
  SFMDATA[SFEXINDEX].desc=(char *)FCEU_malloc(5);
  strcpy(SFMDATA[SFEXINDEX].desc,desc);
 }
 else
  SFMDATA[SFEXINDEX].desc=0;
 SFMDATA[SFEXINDEX].v=v;
 SFMDATA[SFEXINDEX].s=s;
 if(type) SFMDATA[SFEXINDEX].s|=RLSB;
 if(SFEXINDEX<SFMDATA_SIZE-1)
	 SFEXINDEX++;
 else
 {
	 static int once=1;
	 if(once)
	 {
		 once=0;
		 FCEU_PrintError("Error in AddExState: SFEXINDEX overflow.\nSomebody made SFMDATA_SIZE too small.");
	 }
 }
 SFMDATA[SFEXINDEX].v=0;		// End marker.
}

void FCEUI_SelectStateNext(int n)
{
	if(n>0)
		CurrentState=(CurrentState+1)%10;
	else
		CurrentState=(CurrentState+9)%10;
	FCEUI_SelectState(CurrentState, 1);
}

int FCEUI_SelectState(int w, int show)
{
 int oldstate=CurrentState;
 if(w == -1) { StateShow = 0; return 0; } //mbg merge 7/17/06 had to make return a value
 
 CurrentState=w;
 if(show)
 {
  StateShow=180;
  FCEU_DispMessage("-select state-");
 }
 return oldstate;
}  

void FCEUI_SaveState(char *fname)
{
	if(!FCEU_IsValidUI(FCEUI_SAVESTATE)) return;
	
	StateShow=0;
	FCEUSS_Save(fname);
}

int loadStateFailed = 0; // hack, this function should return a value instead

void FCEUI_LoadState(char *fname)
{
	if(!FCEU_IsValidUI(FCEUI_LOADSTATE)) return;

	StateShow = 0;
	loadStateFailed = 0;

	/* For network play, be load the state locally, and then save the state to a temporary file,
	and send that.  This insures that if an older state is loaded that is missing some
	information expected in newer save states, desynchronization won't occur(at least not
	from this ;)).
	*/

	if(FCEUSS_Load(fname))
	{
		if(FCEUnetplay)
		{
			char *fn = FCEU_MakeFName(FCEUMKF_NPTEMP, 0, 0);
			FILE *fp;

			if((fp = fopen(fn," wb")))
			{
				if(FCEUSS_SaveFP(fp,0))
				{
					fclose(fp);
					FCEUNET_SendFile(FCEUNPCMD_LOADSTATE, fn);    
				}
				else
				{
					fclose(fp);
				}

				unlink(fn);
			}

			free(fn);
		}
	}
	else
	{
		loadStateFailed = 1;
	}
}

void FCEU_DrawSaveStates(uint8 *XBuf)
{
 if(!StateShow) return;

 FCEU_DrawNumberRow(XBuf,SaveStateStatus,CurrentState);
 StateShow--;
}

