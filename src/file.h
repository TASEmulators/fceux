#ifndef _FCEU_FILE_H_
#define _FCEU_FILE_H_

#include <string>
#include <iostream>
#include "types.h"

struct FCEUFILE {
	//void *fp;       // FILE* or ptr to ZIPWRAP
	//uint32 type;    // 0=normal file, 1=gzip, 2=zip
	std::iostream *stream;
	std::string filename;
	std::string archiveFilename;
	int archiveIndex;
	int size;

	FCEUFILE()
		: stream(0)
	{}
	
	~FCEUFILE()
	{
		if(stream) delete stream;
	}

	enum {
		READ, WRITE, READWRITE
	} mode;
};

struct ArchiveScanRecord
{	
	ArchiveScanRecord()
		: type(-1)
		, numFiles(0)
	{}
	ArchiveScanRecord(int _type, int _numFiles)
	{
		type = _type;
		numFiles = _numFiles;
	}
	int type;
	int numFiles;
};


FCEUFILE *FCEU_fopen(const char *path, const char *ipsfn, char *mode, char *ext);
int FCEU_fclose(FCEUFILE*);
uint64 FCEU_fread(void *ptr, size_t size, size_t nmemb, FCEUFILE*);
uint64 FCEU_fwrite(void *ptr, size_t size, size_t nmemb, FCEUFILE*);
int FCEU_fseek(FCEUFILE*, long offset, int whence);
uint64 FCEU_ftell(FCEUFILE*);
int FCEU_read32le(uint32 *Bufo, FCEUFILE*);
int FCEU_read16le(uint16 *Bufo, FCEUFILE*);
int FCEU_fgetc(FCEUFILE*);
uint64 FCEU_fgetsize(FCEUFILE*);
int FCEU_fisarchive(FCEUFILE*);



void GetFileBase(const char *f);
std::string FCEU_GetPath(int type);
std::string FCEU_MakePath(int type, const char* filebase);
std::string FCEU_MakeFName(int type, int id1, char *cd1);
void FCEU_SplitArchiveFilename(std::string src, std::string& archive, std::string& file, std::string& fileToOpen);

#define FCEUMKF_STATE        1
#define FCEUMKF_SNAP         2
#define FCEUMKF_SAV          3
#define FCEUMKF_CHEAT        4
#define FCEUMKF_FDSROM       5
#define FCEUMKF_PALETTE      6
#define FCEUMKF_GGROM        7
#define FCEUMKF_IPS          8
#define FCEUMKF_FDS          9
#define FCEUMKF_MOVIE        10
//#define FCEUMKF_NPTEMP       11 //mbg 6/21/08 - who needs this..?
#define FCEUMKF_MOVIEGLOB    12
#define FCEUMKF_STATEGLOB    13
#define FCEUMKF_MOVIEGLOB2   14
#define FCEUMKF_AUTOSTATE	 15
#define FCEUMKF_MEMW         16
#define FCEUMKF_BBOT         17
#define FCEUMKF_ROMS         18
#define FCEUMKF_INPUT        19
#define FCEUMKF_LUA          20

#endif
