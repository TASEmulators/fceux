/*
Copyright (C) 2009-2010 DeSmuME team

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

//don't use emufile for files bigger than 2GB! you have been warned! some day this will be fixed.

#ifndef EMUFILE_H
#define EMUFILE_H

#include "emufile_types.h"

#ifdef _MSC_VER
#include <io.h>
#endif

#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <vector>
#include <algorithm>
#include <string>

class EMUFILE {
protected:
	bool failbit;

public:
	EMUFILE()
		: failbit(false)
	{}


	//returns a new EMUFILE which is guranteed to be in memory. the EMUFILE you call this on may be deleted. use the returned EMUFILE in its place
	virtual EMUFILE* memwrap() = 0;

	virtual ~EMUFILE() {}

	static bool readAllBytes(std::vector<u8>* buf, const std::string& fname);

	bool fail(bool unset=false) { bool ret = failbit; if(unset) unfail(); return ret; }
	void unfail() { failbit=false; }

	bool eof() { return size() == static_cast<size_t>(ftell()); }

	size_t fread(const void *ptr, size_t bytes){
		return _fread(ptr,bytes);
	}

	void unget() { fseek(-1,SEEK_CUR); }

	//virtuals
public:

	virtual FILE *get_fp() = 0;

	virtual int fprintf(const char *format, ...) = 0;

	virtual int fgetc() = 0;
	virtual int fputc(int c) = 0;

	virtual size_t _fread(const void *ptr, size_t bytes) = 0;

	//removing these return values for now so we can find any code that might be using them and make sure
	//they handle the return values correctly

	virtual void fwrite(const void *ptr, size_t bytes) = 0;

	void write64le(u64* val);
	void write64le(u64 val);
	size_t read64le(u64* val);
	u64 read64le();
	void write32le(u32* val);
	void write32le(s32* val) { write32le((u32*)val); }
	void write32le(u32 val);
	size_t read32le(u32* val);
	size_t read32le(s32* val);
	u32 read32le();
	void write16le(u16* val);
	void write16le(s16* val) { write16le((u16*)val); }
	void write16le(u16 val);
	size_t read16le(s16* Bufo);
	size_t read16le(u16* val);
	u16 read16le();
	void write8le(u8* val);
	void write8le(u8 val);
	size_t read8le(u8* val);
	u8 read8le();
	void writedouble(double* val);
	void writedouble(double val);
	double readdouble();
	size_t readdouble(double* val);

	virtual int fseek(long int offset, int origin) = 0;

	virtual long int ftell() = 0;
	virtual size_t size() = 0;
	virtual void fflush() = 0;

	virtual void truncate(size_t length) = 0;
};

//todo - handle read-only specially?
class EMUFILE_MEMORY : public EMUFILE {
protected:
	std::vector<u8> *vec;
	bool ownvec;
	long int pos;
	size_t len;

	void reserve(size_t amt) {
		if(vec->size() < amt)
			vec->resize(amt);
	}

public:

	EMUFILE_MEMORY(std::vector<u8> *underlying) : vec(underlying), ownvec(false), pos(0), len((s32)underlying->size()) { }
	EMUFILE_MEMORY(size_t preallocate) : vec(new std::vector<u8>()), ownvec(true), pos(0), len(0) {
		vec->resize(preallocate);
		len = preallocate;
	}
	EMUFILE_MEMORY() : vec(new std::vector<u8>()), ownvec(true), pos(0), len(0) { vec->reserve(1024); }
	EMUFILE_MEMORY(void* buf, size_t size) : vec(new std::vector<u8>()), ownvec(true), pos(0), len(size) {
		vec->resize(size);
		if(size != 0)
			memcpy(&vec->front(),buf,size);
	}

	~EMUFILE_MEMORY() {
		if(ownvec) delete vec;
	}

	virtual EMUFILE* memwrap();

	virtual void truncate(size_t length)
	{
		vec->resize(length);
		len = length;
		if (static_cast<size_t>(pos) > length) pos=static_cast<long int>(length);
	}

	u8* buf() {
		if(size()==0) reserve(1);
		return &(*vec)[0];
	}

	std::vector<u8>* get_vec() { return vec; };

	virtual FILE *get_fp() { return NULL; }

	virtual int fprintf(const char *format, ...) {
		va_list argptr;
		va_start(argptr, format);

		//we dont generate straight into the buffer because it will null terminate (one more byte than we want)
		int amt = vsnprintf(0,0,format,argptr);
		char* tempbuf = new char[amt+1];

		va_end(argptr);
		va_start(argptr, format);
		vsnprintf(tempbuf,amt+1,format,argptr);

		fwrite(tempbuf,amt);
		delete[] tempbuf;

		va_end(argptr);
		return amt;
	};

	virtual int fgetc() {
		u8 temp;

		//need an optimized codepath
		//if(_fread(&temp,1) != 1)
		//	return EOF;
		//else return temp;
		size_t remain = len-pos;
		if(remain<1) {
			failbit = true;
			return -1;
		}
		temp = buf()[pos];
		pos++;
		return temp;
	}
	virtual int fputc(int c) {
		u8 temp = (u8)c;
		//TODO
		//if(fwrite(&temp,1)!=1) return EOF;
		fwrite(&temp,1);

		return 0;
	}

	virtual size_t _fread(const void *ptr, size_t bytes);

	//removing these return values for now so we can find any code that might be using them and make sure
	//they handle the return values correctly

	virtual void fwrite(const void *ptr, size_t bytes){
		reserve(pos+bytes);
		memcpy(buf()+pos,ptr,bytes);
		pos += static_cast<long>(bytes);
		len = std::max<size_t>(pos,len);
	}

	virtual int fseek(long int offset, int origin){
		//work differently for read-only...?
		switch(origin) {
			case SEEK_SET:
				pos = offset;
				break;
			case SEEK_CUR:
				pos += offset;
				break;
			case SEEK_END:
				pos = (long int)(size()+offset);
				break;
			default:
				assert(false);
		}
		reserve(pos);
		return 0;
	}

	virtual long int ftell() {
		return pos;
	}

	virtual void fflush() {}

	void set_len(size_t length)
	{
		len = length;
		if (static_cast<size_t>(pos) > length)
			pos = static_cast<long>(length);
	}
	void trim()
	{
		vec->resize(len);
	}

	virtual size_t size() { return len; }
};

class EMUFILE_FILE : public EMUFILE {
protected:
	FILE* fp;
	std::string fname;
	char mode[16];

private:
	void open(const char* fname, const char* mode);

public:

	EMUFILE_FILE(const std::string& fname, const char* mode) { open(fname.c_str(),mode); }
	EMUFILE_FILE(const char* fname, const char* mode) { open(fname,mode); }

	virtual ~EMUFILE_FILE() {
		if(NULL != fp)
			fclose(fp);
	}

	virtual FILE *get_fp() {
		return fp;
	}

	virtual EMUFILE* memwrap();

	bool is_open() { return fp != NULL; }

	virtual void truncate(size_t length);

	virtual int fprintf(const char *format, ...) {
		va_list argptr;
		va_start(argptr, format);
		int ret = ::vfprintf(fp, format, argptr);
		va_end(argptr);
		return ret;
	};

	virtual int fgetc() {
		return ::fgetc(fp);
	}
	virtual int fputc(int c) {
		return ::fputc(c, fp);
	}

	virtual size_t _fread(const void *ptr, size_t bytes){
		size_t ret = ::fread((void*)ptr, 1, bytes, fp);
		if(ret < bytes)
			failbit = true;
		return ret;
	}

	//removing these return values for now so we can find any code that might be using them and make sure
	//they handle the return values correctly

	virtual void fwrite(const void *ptr, size_t bytes){
		size_t ret = ::fwrite((void*)ptr, 1, bytes, fp);
		if(ret < bytes)
			failbit = true;
	}

	virtual int fseek(long int offset, int origin) {
		return ::fseek(fp, offset, origin);
	}

	virtual long int ftell() {
		return ::ftell(fp);
	}

	virtual size_t size() {
		long int oldpos = ftell();
		fseek(0,SEEK_END);
		long int len = ftell();
		fseek(oldpos,SEEK_SET);
		return static_cast<size_t>(len);
	}

	virtual void fflush() {
		::fflush(fp);
	}

};

#endif
