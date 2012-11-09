/* ---------------------------------------------------------------------------------
Implementation file of LagLog class
Copyright (c) 2011-2012 AnS

(The MIT License)
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------------
LagLog - Log of Lag appearance

* stores the frame-by-frame log of lag appearance
* implements compression and decompression of stored data
* saves and loads the data from a project file. On error: sends warning to caller
* provides interface for reading and writing log data
------------------------------------------------------------------------------------ */

#include "taseditor_project.h"
#include "zlib.h"

LAGLOG::LAGLOG()
{
	already_compressed = false;
}

void LAGLOG::reset()
{
	lag_log.resize(0);
	already_compressed = false;
}

void LAGLOG::compress_data()
{
	int len = lag_log.size() * sizeof(uint8);
	if (len)
	{
		uLongf comprlen = (len>>9)+12 + len;
		lag_log_compressed.resize(comprlen, LAGGED_DONTKNOW);
		compress(&lag_log_compressed[0], &comprlen, (uint8*)&lag_log[0], len);
		lag_log_compressed.resize(comprlen);
	} else
	{
		// LagLog can be empty
		lag_log_compressed.resize(0);
	}
	already_compressed = true;
}
bool LAGLOG::Get_already_compressed()
{
	return already_compressed;
}
void LAGLOG::Reset_already_compressed()
{
	already_compressed = false;
}

void LAGLOG::save(EMUFILE *os)
{
	// write size
	int size = lag_log.size();
	write32le(size, os);
	if (size)
	{
		// write array
		if (!already_compressed)
			compress_data();
		write32le(lag_log_compressed.size(), os);
		os->fwrite(&lag_log_compressed[0], lag_log_compressed.size());
	}
}
// returns true if couldn't load
bool LAGLOG::load(EMUFILE *is)
{
	int size;
	if (read32le(&size, is))
	{
		already_compressed = true;
		lag_log.resize(size, LAGGED_DONTKNOW);
		if (size)
		{
			// read and uncompress array
			int comprlen;
			uLongf destlen = size * sizeof(int);
			if (!read32le(&comprlen, is)) return true;
			if (comprlen <= 0) return true;
			lag_log_compressed.resize(comprlen);
			if (is->fread(&lag_log_compressed[0], comprlen) != comprlen) return true;
			int e = uncompress((uint8*)&lag_log[0], &destlen, &lag_log_compressed[0], comprlen);
			if (e != Z_OK && e != Z_BUF_ERROR) return true;
		} else
		{
			lag_log_compressed.resize(0);
		}
		// all ok
		return false;
	}
	return true;
}
bool LAGLOG::skipLoad(EMUFILE *is)
{
	int size;
	if (read32le(&size, is))
	{
		if (size)
		{
			// skip array
			if (!read32le(&size, is)) return true;
			if (is->fseek(size, SEEK_CUR) != 0) return true;
		}
		// all ok
		return false;
	}
	return true;
}
// -------------------------------------------------------------------------------------------------
void LAGLOG::InvalidateFrom(int frame)
{
	if (frame >= 0 && frame < (int)lag_log.size())
	{
		lag_log.resize(frame);
		already_compressed = false;
	}
}

void LAGLOG::SetLagInfo(int frame, bool lagFlag)
{
	if ((int)lag_log.size() <= frame)
		lag_log.resize(frame + 1, LAGGED_DONTKNOW);

	if (lagFlag)
		lag_log[frame] = LAGGED_YES;
	else
		lag_log[frame] = LAGGED_NO;

	already_compressed = false;
}
void LAGLOG::EraseFrame(int frame, int frames)
{
	if (frame < (int)lag_log.size())
	{
		if (frames == 1)
		{
			// erase 1 frame
			lag_log.erase(lag_log.begin() + frame);
			already_compressed = false;
		} else if (frames > 1)
		{
			// erase many frames
			if (frame + frames > (int)lag_log.size())
				frames = (int)lag_log.size() - frame;
			lag_log.erase(lag_log.begin() + frame, lag_log.begin() + (frame + frames));
			already_compressed = false;
		}
	}
}
void LAGLOG::InsertFrame(int frame, bool lagFlag, int frames)
{
	if (frame < (int)lag_log.size())
	{
		// insert
		lag_log.insert(lag_log.begin() + frame, frames, (lagFlag) ? LAGGED_YES : LAGGED_NO);
	} else
	{
		// append
		lag_log.resize(frame + 1, LAGGED_DONTKNOW);
		if (lagFlag)
			lag_log[frame] = LAGGED_YES;
		else
			lag_log[frame] = LAGGED_NO;
	}
	already_compressed = false;
}

// getters
int LAGLOG::GetSize()
{
	return lag_log.size();
}
int LAGLOG::GetLagInfoAtFrame(int frame)
{
	if (frame < (int)lag_log.size())
		return lag_log[frame];
	else
		return LAGGED_DONTKNOW;
}

int LAGLOG::findFirstChange(LAGLOG& their_log)
{
	// search for differences to the end of this or their LagLog, whichever is less
	int end = lag_log.size() - 1;
	int their_log_end = their_log.GetSize() - 1;
	if (end > their_log_end)
		end = their_log_end;

	uint8 my_lag_info, their_lag_info;
	for (int i = 0; i <= end; ++i)
	{
		// if old info != new info and both infos are known
		my_lag_info = lag_log[i];
		their_lag_info = their_log.GetLagInfoAtFrame(i);
		if ((my_lag_info == LAGGED_YES && their_lag_info == LAGGED_NO) || (my_lag_info == LAGGED_NO && their_lag_info == LAGGED_YES))
			return i;
	}
	// no difference was found
	return -1;
}


