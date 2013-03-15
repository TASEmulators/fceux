/* ---------------------------------------------------------------------------------
Implementation file of Markers class
Copyright (c) 2011-2012 AnS

(The MIT License)
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------------
Markers - Snapshot of Markers state

* stores the data about Markers state: array of distributing Markers among movie frames, and array of Notes
* implements compression and decompression of stored data
* saves and loads the data from a project file. On error: sends warning to caller
* stores resources: max length of a Note
------------------------------------------------------------------------------------ */

#include "../common.h"
#include "markers.h"
#include "zlib.h"

MARKERS::MARKERS()
{
	already_compressed = false;
}

void MARKERS::save(EMUFILE *os)
{
	// write size
	int size = markers_array.size();
	int len;
	write32le(size, os);
	// write array
	if (!already_compressed)
		compress_data();
	write32le(markers_array_compressed.size(), os);
	os->fwrite(&markers_array_compressed[0], markers_array_compressed.size());
	// write notes
	size = notes.size();
	write32le(size, os);
	for (int i = 0; i < size; ++i)
	{
		len = notes[i].length() + 1;
		if (len > MAX_NOTE_LEN) len = MAX_NOTE_LEN;
		write32le(len, os);
		os->fwrite(notes[i].c_str(), len);
	}
}
// returns true if couldn't load
bool MARKERS::load(EMUFILE *is)
{
	int size;
	if (read32le(&size, is))
	{
		markers_array.resize(size);
		// read and uncompress array
		already_compressed = true;
		int comprlen, len;
		uLongf destlen = size * sizeof(int);
		if (!read32le(&comprlen, is)) return true;
		if (comprlen <= 0) return true;
		markers_array_compressed.resize(comprlen);
		if (is->fread(&markers_array_compressed[0], comprlen) != comprlen) return true;
		int e = uncompress((uint8*)&markers_array[0], &destlen, &markers_array_compressed[0], comprlen);
		if (e != Z_OK && e != Z_BUF_ERROR) return true;
		// read notes
		if (read32le(&size, is) && size >= 0)
		{
			notes.resize(size);
			char temp_str[MAX_NOTE_LEN];
			for (int i = 0; i < size; ++i)
			{
				if (!read32le(&len, is) || len < 0) return true;
				if ((int)is->fread(temp_str, len) < len) return true;
				notes[i] = temp_str;
			}
			// all ok
			return false;
		}
	}
	return true;
}
bool MARKERS::skipLoad(EMUFILE *is)
{
	if (!(is->fseek(sizeof(int), SEEK_CUR)))
	{
		// read array
		int comprlen, len;
		if (!read32le(&comprlen, is)) return true;
		if (is->fseek(comprlen, SEEK_CUR) != 0) return true;
		// read notes
		if (read32le(&comprlen, is) && comprlen >= 0)
		{
			for (int i = 0; i < comprlen; ++i)
			{
				if (!read32le(&len, is) || len < 0) return true;
				if (is->fseek(len, SEEK_CUR) != 0) return true;
			}
			// all ok
			return false;
		}
	}
	return true;
}

void MARKERS::compress_data()
{
	int len = markers_array.size() * sizeof(int);
	uLongf comprlen = (len>>9)+12 + len;
	markers_array_compressed.resize(comprlen);
	compress(&markers_array_compressed[0], &comprlen, (uint8*)&markers_array[0], len);
	markers_array_compressed.resize(comprlen);
	already_compressed = true;
}
bool MARKERS::Get_already_compressed()
{
	return already_compressed;
}
void MARKERS::Reset_already_compressed()
{
	already_compressed = false;
}

