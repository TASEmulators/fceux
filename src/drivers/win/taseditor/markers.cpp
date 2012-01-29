//Implementation file of Markers class
#include "../common.h"
#include "markers.h"
#include "zlib.h"

MARKERS::MARKERS()
{
}

void MARKERS::save(EMUFILE *os)
{
	// write size
	int size = markers_array.size();
	write32le(size, os);
	// compress and write array
	int len = markers_array.size() * sizeof(int);
	uLongf comprlen = (len>>9)+12 + len;
	std::vector<uint8> cbuf(comprlen);
	compress(&cbuf[0], &comprlen, (uint8*)&markers_array[0], len);
	write32le(comprlen, os);
	os->fwrite(&cbuf[0], comprlen);
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
		int comprlen, len;
		uLongf destlen = size * sizeof(int);
		if (!read32le(&comprlen, is)) return true;
		if (comprlen <= 0) return true;
		std::vector<uint8> cbuf(comprlen);
		if (is->fread(&cbuf[0], comprlen) != comprlen) return true;
		int e = uncompress((uint8*)&markers_array[0], &destlen, &cbuf[0], comprlen);
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
	int size;
	if (!(is->fseek(sizeof(int), SEEK_CUR)))
	{
		// read array
		int comprlen, len;
		if (!read32le(&comprlen, is)) return true;
		if (is->fseek(comprlen, SEEK_CUR) != 0) return true;
		// read notes
		if (read32le(&size, is) && size >= 0)
		{
			for (int i = 0; i < size; ++i)
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
