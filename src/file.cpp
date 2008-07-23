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

#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef WIN32
#include <zlib.h>
#endif

#ifdef WIN32
#include <drivers/win/archive.h>
#endif

#include "types.h"
#include "file.h"
#include "utils/endian.h"
#include "utils/memory.h"
#include "utils/md5.h"
#include "utils/unzip.h"
#include "driver.h"
#include "types.h"
#include "fceu.h"
#include "state.h"
#include "movie.h"
#include "driver.h"
#include "utils/xstring.h"

using namespace std;

typedef struct {
           uint8 *data;
           uint32 size;
           uint32 location;
} MEMWRAP;

void ApplyIPS(FILE *ips, MEMWRAP *dest)
{
	uint8 header[5];
	uint32 count=0;

	FCEU_printf(" Applying IPS...\n");
	if(fread(header,1,5,ips)!=5)
	{
		fclose(ips);
		return;
	}
	if(memcmp(header,"PATCH",5))
	{
		fclose(ips);
		return;
	}

	while(fread(header,1,3,ips)==3)
	{
		uint32 offset=(header[0]<<16)|(header[1]<<8)|header[2];
		uint16 size;

		if(!memcmp(header,"EOF",3))
		{
			FCEU_printf(" IPS EOF:  Did %d patches\n\n",count);
			fclose(ips);
			return;
		}

		size=fgetc(ips)<<8;
		size|=fgetc(ips);
		if(!size)	/* RLE */
		{
			uint8 *start;
			uint8 b;
			size=fgetc(ips)<<8;
			size|=fgetc(ips);

			//FCEU_printf("  Offset: %8d  Size: %5d RLE\n",offset,size);

			if((offset+size)>dest->size)
			{
				uint8 *tmp;

				// Probably a little slow.
				tmp=(uint8 *)realloc(dest->data,offset+size);
				if(!tmp)
				{
					FCEU_printf("  Oops.  IPS patch %d(type RLE) goes beyond end of file.  Could not allocate memory.\n",count);
					fclose(ips);
					return;
				}
				dest->size=offset+size;
				dest->data=tmp;
				memset(dest->data+dest->size,0,offset+size-dest->size);
			}
			b=fgetc(ips);
			start=dest->data+offset;
			do
			{
				*start=b;
				start++;
			} while(--size);
		}
		else		/* Normal patch */
		{
			//FCEU_printf("  Offset: %8d  Size: %5d\n",offset,size);
			if((offset+size)>dest->size)
			{
				uint8 *tmp;

				// Probably a little slow.
				tmp=(uint8 *)realloc(dest->data,offset+size);
				if(!tmp)
				{
					FCEU_printf("  Oops.  IPS patch %d(type normal) goes beyond end of file.  Could not allocate memory.\n",count);
					fclose(ips);
					return;
				}
				dest->data=tmp;
				memset(dest->data+dest->size,0,offset+size-dest->size);
			}
			fread(dest->data+offset,1,size,ips);
		}
		count++;
	}
	fclose(ips);
	FCEU_printf(" Hard IPS end!\n");
}

static MEMWRAP *MakeMemWrap(void *tz, int type)
{
	MEMWRAP *tmp;

	if(!(tmp=(MEMWRAP *)FCEU_malloc(sizeof(MEMWRAP))))
		goto doret;
	tmp->location=0;

	if(type==0)
	{
		fseek((FILE *)tz,0,SEEK_END);
		tmp->size=ftell((FILE *)tz);
		fseek((FILE *)tz,0,SEEK_SET);
		if(!(tmp->data=(uint8*)FCEU_malloc(tmp->size)))
		{
			free(tmp);
			tmp=0;
			goto doret;
		}
		fread(tmp->data,1,tmp->size,(FILE *)tz);
	}
	else if(type==1)
	{
		/* Bleck.  The gzip file format has the size of the uncompressed data,
		but I can't get to the info with the zlib interface(?). */
		for(tmp->size=0; gzgetc(tz) != EOF; tmp->size++);
		gzseek(tz,0,SEEK_SET);
		if(!(tmp->data=(uint8 *)FCEU_malloc(tmp->size)))
		{
			free(tmp);
			tmp=0;
			goto doret;
		}
		gzread(tz,tmp->data,tmp->size);
	}
	else if(type==2)
	{
		unz_file_info ufo;
		unzGetCurrentFileInfo(tz,&ufo,0,0,0,0,0,0);

		tmp->size=ufo.uncompressed_size;
		if(!(tmp->data=(uint8 *)FCEU_malloc(ufo.uncompressed_size)))
		{
			free(tmp);
			tmp=0;
			goto doret;
		}
		unzReadCurrentFile(tz,tmp->data,ufo.uncompressed_size);
	}

doret:
	if(type==0)
	{
		fclose((FILE *)tz);
	}
	else if(type==1)
	{
		gzclose(tz);
	}
	else if(type==2)
	{
		unzCloseCurrentFile(tz);
		unzClose(tz);
	}
	return tmp;
}

void FCEU_SplitArchiveFilename(std::string src, std::string& archive, std::string& file, std::string& fileToOpen)
{
	size_t pipe = src.find_first_of('|');
	if(pipe == std::string::npos)
	{
		archive = "";
		file = src;
		fileToOpen = src;
	}
	else
	{
		archive = src.substr(0,pipe);
		file = src.substr(pipe+1);
		fileToOpen = archive;
	}
}

FCEUFILE * FCEU_fopen(const char *path, const char *ipsfn, char *mode, char *ext)
{
	FILE *ipsfile=0;
	FCEUFILE *fceufp=0;

	bool read = (std::string)mode == "rb";
	bool write = (std::string)mode == "wb";
	if(read&&write || (!read&&!write))
	{
		FCEU_PrintError("invalid file open mode specified (only wb and rb are supported)");
		return 0;
	}

	std::string archive,fname,fileToOpen;
	FCEU_SplitArchiveFilename(path,archive,fname,fileToOpen);
	

	//try to setup the ips file
	if(ipsfn && read)
		ipsfile=FCEUD_UTF8fopen(ipsfn,"rb");
	#ifdef WIN32
	if(read)
	{
		ArchiveScanRecord asr = FCEUD_ScanArchive(fileToOpen);
		if(asr.numFiles == 0)
		{
			//if the archive contained no files, try to open it the old fashioned way
			std::fstream* fp = FCEUD_UTF8_fstream(fileToOpen,mode);
			if(!fp)
			{
				return 0;
			}
			fceufp = new FCEUFILE();
			fceufp->filename = fileToOpen;
			fceufp->archiveIndex = -1;
			fceufp->stream = (std::iostream*)fp;
			FCEU_fseek(fceufp,0,SEEK_END);
			fceufp->size = FCEU_ftell(fceufp);
			FCEU_fseek(fceufp,0,SEEK_SET);
			return fceufp;
		}
		else
		{
			//open an archive file
			if(archive == "")
				fceufp = FCEUD_OpenArchive(asr, fileToOpen, 0);
			else
				fceufp = FCEUD_OpenArchive(asr, archive, &fname);
			return fceufp;
		}
	}
	#else
	std::fstream* fp = FCEUD_UTF8_fstream(fileToOpen,mode);
	if(!fp)
	{
		return 0;
	}
	fceufp = new FCEUFILE();
	fceufp->filename = fileToOpen;
	fceufp->archiveIndex = -1;
	fceufp->stream = (std::iostream*)fp;
	FCEU_fseek(fceufp,0,SEEK_END);
	fceufp->size = FCEU_ftell(fceufp);
	FCEU_fseek(fceufp,0,SEEK_SET);
	return fceufp;
	#endif
	

	return 0;
}

int FCEU_fclose(FCEUFILE *fp)
{
	delete fp;
	return 1;
}

uint64 FCEU_fread(void *ptr, size_t size, size_t nmemb, FCEUFILE *fp)
{
	fp->stream->read((char*)ptr,size*nmemb);
	uint32 read = fp->stream->gcount();
	return read/size;
}

uint64 FCEU_fwrite(void *ptr, size_t size, size_t nmemb, FCEUFILE *fp)
{
	fp->stream->write((char*)ptr,size*nmemb);
	//todo - how do we tell how many bytes we wrote?
	return nmemb;
}

int FCEU_fseek(FCEUFILE *fp, long offset, int whence)
{
	//if(fp->type==1)
	//{
	//	return( (gzseek(fp->fp,offset,whence)>0)?0:-1);
	//}
	//else if(fp->type>=2)
	//{
	//	MEMWRAP *wz;
	//	wz=(MEMWRAP*)fp->fp;

	//	switch(whence)
	//	{
	//	case SEEK_SET:if(offset>=(long)wz->size) //mbg merge 7/17/06 - added cast to long
	//					  return(-1);
	//		wz->location=offset;break;
	//	case SEEK_CUR:if(offset+wz->location>wz->size)
	//					  return (-1);
	//		wz->location+=offset;
	//		break;
	//	}
	//	return 0;
	//}
	//else
	//	return fseek((FILE *)fp->fp,offset,whence);

	fp->stream->seekg(offset,(std::ios_base::seekdir)whence);
	fp->stream->seekp(offset,(std::ios_base::seekdir)whence);

	return FCEU_ftell(fp);
}

uint64 FCEU_ftell(FCEUFILE *fp)
{
	if(fp->mode == FCEUFILE::READ)
		return fp->stream->tellg();
	else
		return fp->stream->tellp();
}

int FCEU_read16le(uint16 *val, FCEUFILE *fp)
{
	return read16le(val,fp->stream);
}

int FCEU_read32le(uint32 *Bufo, FCEUFILE *fp)
{
	return read32le(Bufo, fp->stream);
}

int FCEU_fgetc(FCEUFILE *fp)
{
	return fp->stream->get();
}

uint64 FCEU_fgetsize(FCEUFILE *fp)
{
	return fp->size;
}

int FCEU_fisarchive(FCEUFILE *fp)
{
	if(fp->archiveIndex==0) return 0;
	else return 1;
}

std::string GetMfn() //Retrieves the movie filename from curMovieFilename (for adding to savestate and auto-save files)
{
	std::string movieFilenamePart;
	extern char curMovieFilename[512];
	if(*curMovieFilename)
		{
		char drv[PATH_MAX], dir[PATH_MAX], name[PATH_MAX], ext[PATH_MAX];
		splitpath(curMovieFilename,drv,dir,name,ext);
		movieFilenamePart = std::string(".") + name;
		}
	return movieFilenamePart;
}

static std::string BaseDirectory;
char FileBase[2048];
static char FileExt[2048];	//Includes the . character, as in ".nes"

static char FileBaseDirectory[2048];

/// Updates the base directory
void FCEUI_SetBaseDirectory(std::string const & dir)
{
	BaseDirectory = dir;
}

static char *odirs[FCEUIOD__COUNT]={0,0,0,0,0,0,0,0,0,0,0,0};     // odirs, odors. ^_^

void FCEUI_SetDirOverride(int which, char *n)
{
	//	FCEU_PrintError("odirs[%d]=%s->%s", which, odirs[which], n);
	if (which < FCEUIOD__COUNT)
	{
		odirs[which] = n;
	}

	if(GameInfo)  //Rebuild cache of present states/movies. 
	{
		if(which==FCEUIOD_STATES)
		{
			FCEUSS_CheckStates();
		}
	}
}

	#ifndef HAVE_ASPRINTF
	static int asprintf(char **strp, const char *fmt, ...)
	{
		va_list ap;
		int ret;

		va_start(ap,fmt);
		if(!(*strp=(char*)malloc(2048))) //mbg merge 7/17/06 cast to char*
			return(0);
		ret=vsnprintf(*strp,2048,fmt,ap);
		va_end(ap);
		return(ret);
	}
	#endif

std::string FCEU_GetPath(int type)
{
	char ret[FILENAME_MAX];
	switch(type)
	{
		case FCEUMKF_STATE:
			if(odirs[FCEUIOD_STATES])
				return (odirs[FCEUIOD_STATES]);
			else
				return BaseDirectory + PSS + "fcs";
			break;
		case FCEUMKF_MOVIE:
			if(odirs[FCEUIOD_MOVIES])
				return (odirs[FCEUIOD_MOVIES]);
			else
				return BaseDirectory + PSS + "movies";
			break;
		case FCEUMKF_MEMW:
			if(odirs[FCEUIOD_MEMW])
				return (odirs[FCEUIOD_MEMW]);
			else
				return BaseDirectory + PSS + "tools";
			break;
		case FCEUMKF_BBOT:
			if(odirs[FCEUIOD_BBOT])
				return (odirs[FCEUIOD_BBOT]);
			else
				return BaseDirectory + PSS + "tools";
			break;
		case FCEUMKF_ROMS:
			if(odirs[FCEUIOD_ROMS])
				return (odirs[FCEUIOD_ROMS]);
			else
				return BaseDirectory;
			break;
		case FCEUMKF_INPUT:
			if(odirs[FCEUIOD_INPUT])
				return (odirs[FCEUIOD_INPUT]);
			else
				return BaseDirectory + PSS + "tools";
			break;
		case FCEUMKF_LUA:
			if(odirs[FCEUIOD_LUA])
				return (odirs[FCEUIOD_LUA]);
			else
				return BaseDirectory + PSS + "tools";
			break;
	}

	return ret;
}

std::string FCEU_MakePath(int type, const char* filebase)
{
	char ret[FILENAME_MAX];

	switch(type)
	{
		case FCEUMKF_MOVIE:
			if(odirs[FCEUIOD_MOVIES])
				return (string)odirs[FCEUIOD_MOVIES] + PSS + filebase;
			else
				return BaseDirectory + PSS + "movies" + PSS + filebase;
			break;
		case FCEUMKF_STATE:
			if(odirs[FCEUIOD_STATES])
				return (string)odirs[FCEUIOD_STATES] + PSS + filebase;
			else
				return BaseDirectory + PSS + "fcs" + PSS + filebase;
			break;
	}
	return ret;
}

std::string FCEU_MakeFName(int type, int id1, char *cd1)
{
	char ret[FILENAME_MAX];
	struct stat tmpstat;
	std::string mfnString;
	const char* mfn;

	switch(type)
	{
		case FCEUMKF_MOVIE:
			if(odirs[FCEUIOD_MOVIES])
				sprintf(ret,"%s"PSS"%s.fm2",odirs[FCEUIOD_MOVIES],FileBase);
			else
				sprintf(ret,"%s"PSS"movies"PSS"%s.fm2",BaseDirectory.c_str(),FileBase);
			break;
		case FCEUMKF_STATE:
			{
				mfnString = GetMfn();
				mfn = mfnString.c_str();
				if(odirs[FCEUIOD_STATES])
				{
					sprintf(ret,"%s"PSS"%s%s.fc%d",odirs[FCEUIOD_STATES],FileBase,mfn,id1);
				}
				else
				{
					sprintf(ret,"%s"PSS"fcs"PSS"%s%s.fc%d",BaseDirectory.c_str(),FileBase,mfn,id1);
				}
				if(stat(ret,&tmpstat)==-1)
				{
					if(odirs[FCEUIOD_STATES])
					{
						sprintf(ret,"%s"PSS"%s%s.fc%d",odirs[FCEUIOD_STATES],FileBase,mfn,id1);
					}
					else
					{
						sprintf(ret,"%s"PSS"fcs"PSS"%s%s.fc%d",BaseDirectory.c_str(),FileBase,mfn,id1);
					}
				}
			}
			break;
		case FCEUMKF_SNAP:
			if(FSettings.SnapName)
			{
				if(odirs[FCEUIOD_SNAPS])
					sprintf(ret,"%s"PSS"%s-%d.%s",odirs[FCEUIOD_SNAPS],FileBase,id1,cd1);
				else
					sprintf(ret,"%s"PSS"snaps"PSS"%s-%d.%s",BaseDirectory.c_str(),FileBase,id1,cd1);
			}
			else
			{
				if(odirs[FCEUIOD_SNAPS])
					sprintf(ret,"%s"PSS"%d.%s",odirs[FCEUIOD_SNAPS],id1,cd1);
				else
					sprintf(ret,"%s"PSS"snaps"PSS"%d.%s",BaseDirectory.c_str(),id1,cd1);
			}
			break;
		case FCEUMKF_FDS:
			if(odirs[FCEUIOD_NV])
				sprintf(ret,"%s"PSS"%s.fds",odirs[FCEUIOD_NV],FileBase);
			else
				sprintf(ret,"%s"PSS"sav"PSS"%s.fds",BaseDirectory.c_str(),FileBase);
			break;
		case FCEUMKF_SAV:
			if(odirs[FCEUIOD_NV])
				sprintf(ret,"%s"PSS"%s.%s",odirs[FCEUIOD_NV],FileBase,cd1);
			else
				sprintf(ret,"%s"PSS"sav"PSS"%s.%s",BaseDirectory.c_str(),FileBase,cd1);
			if(stat(ret,&tmpstat)==-1)
			{
				if(odirs[FCEUIOD_NV])
					sprintf(ret,"%s"PSS"%s.%s",odirs[FCEUIOD_NV],FileBase,cd1);
				else
					sprintf(ret,"%s"PSS"sav"PSS"%s.%s",BaseDirectory.c_str(),FileBase,cd1);
			}
			break;
		case FCEUMKF_AUTOSTATE:
			extern char curMovieFilename[512];
			mfnString = GetMfn();
			mfn = mfnString.c_str();
			if(odirs[FCEUIOD_STATES])
			{
				sprintf(ret,"%s"PSS"%s%s-autosave%d.fcs",odirs[FCEUIOD_STATES],FileBase,mfn,id1);
			}
			else
			{
				sprintf(ret,"%s"PSS"fcs"PSS"%s%s-autosave%d.fcs",BaseDirectory.c_str(),FileBase,mfn,id1);
			}
			if(stat(ret,&tmpstat)==-1)
			{
				if(odirs[FCEUIOD_STATES])
				{
					sprintf(ret,"%s"PSS"%s%s-autosave%d.fcs",odirs[FCEUIOD_STATES],FileBase,mfn,id1);
				}
				else
				{
					sprintf(ret,"%s"PSS"fcs"PSS"%s%s-autosave%d.fcs",BaseDirectory.c_str(),FileBase,mfn,id1);
				}
			}
			break;
		case FCEUMKF_CHEAT:
			if(odirs[FCEUIOD_CHEATS])
				sprintf(ret,"%s"PSS"%s.cht",odirs[FCEUIOD_CHEATS],FileBase);
			else
				sprintf(ret,"%s"PSS"cheats"PSS"%s.cht",BaseDirectory.c_str(),FileBase);
			break;
		case FCEUMKF_IPS:sprintf(ret,"%s"PSS"%s%s.ips",FileBaseDirectory,FileBase,FileExt);break;
		case FCEUMKF_GGROM:sprintf(ret,"%s"PSS"gg.rom",BaseDirectory.c_str());break;
		case FCEUMKF_FDSROM:
			if(odirs[FCEUIOD_FDSROM])
				sprintf(ret,"%s"PSS"disksys.rom",odirs[FCEUIOD_FDSROM]);
			else
				sprintf(ret,"%s"PSS"disksys.rom",BaseDirectory.c_str());
			break;
		case FCEUMKF_PALETTE:sprintf(ret,"%s"PSS"%s.pal",BaseDirectory.c_str(),FileBase);break;
		case FCEUMKF_MOVIEGLOB:
			//these globs use ??? because we can load multiple formats
			if(odirs[FCEUIOD_MOVIES])
				sprintf(ret,"%s"PSS"*.???",odirs[FCEUIOD_MOVIES]);
			else
				sprintf(ret,"%s"PSS"movies"PSS"*.???",BaseDirectory.c_str());
			break;
		case FCEUMKF_MOVIEGLOB2:sprintf(ret,"%s"PSS"*.???",BaseDirectory.c_str());break;
		case FCEUMKF_STATEGLOB:
			if(odirs[FCEUIOD_STATES])
				sprintf(ret,"%s"PSS"%s*.fc?",odirs[FCEUIOD_STATES],FileBase);
			else
				sprintf(ret,"%s"PSS"fcs"PSS"%s*.fc?",BaseDirectory.c_str(),FileBase);
			break;
	}
	
	return ret;
}

void GetFileBase(const char *f)
{
	const char *tp1,*tp3;

	#if PSS_STYLE==4
		tp1=((char *)strrchr(f,':'));
	#elif PSS_STYLE==1
		tp1=((char *)strrchr(f,'/'));
	#else
		tp1=((char *)strrchr(f,'\\'));
	#if PSS_STYLE!=3
		tp3=((char *)strrchr(f,'/'));
		if(tp1<tp3) tp1=tp3;
	#endif
	#endif
	if(!tp1)
	{
		tp1=f;
		strcpy(FileBaseDirectory,".");
	}
	else
	{
		memcpy(FileBaseDirectory,f,tp1-f);
		FileBaseDirectory[tp1-f]=0;
		tp1++;
	}

	if(((tp3=strrchr(f,'.'))!=NULL) && (tp3>tp1))
	{
		memcpy(FileBase,tp1,tp3-tp1);
		FileBase[tp3-tp1]=0;
		strcpy(FileExt,tp3);
	}
	else
	{
		strcpy(FileBase,tp1);
		FileExt[0]=0;
	}
}
