//Implementation file of Markers class

#include "taseditproj.h"
#include "zlib.h"

char markers_save_id[MARKERS_ID_LEN] = "MARKERS";

MARKERS::MARKERS()
{
}

void MARKERS::init()
{
	free();
	update();
}
void MARKERS::free()
{
	markers_array.resize(0);
}

void MARKERS::update()
{
	if ((int)markers_array.size() < currMovieData.getNumRecords())
		markers_array.resize(currMovieData.getNumRecords());
}

void MARKERS::save(EMUFILE *os)
{
	// write "MARKERS" string
	os->fwrite(markers_save_id, MARKERS_ID_LEN);
	// write size
	int size = markers_array.size();
	write32le(size, os);
	// compress and write array
	int len = markers_array.size();
	uLongf comprlen = (len>>9)+12 + len;
	std::vector<uint8> cbuf(comprlen);
	compress(&cbuf[0], &comprlen, &markers_array[0], len);
	write32le(comprlen, os);
	os->fwrite(&cbuf[0], comprlen);
}
// returns true if couldn't load
bool MARKERS::load(EMUFILE *is)
{
	// read "MARKERS" string
	char save_id[MARKERS_ID_LEN];
	if ((int)is->fread(save_id, MARKERS_ID_LEN) < MARKERS_ID_LEN) goto error;
	if (strcmp(markers_save_id, save_id)) goto error;		// string is not valid
	int size;
	if (read32le((uint32 *)&size, is) && size >= currMovieData.getNumRecords())
	{
		markers_array.resize(size);
		// read and uncompress array
		int comprlen;
		uLongf destlen = size;
		if (!read32le(&comprlen, is)) goto error;
		if (comprlen <= 0) goto error;
		std::vector<uint8> cbuf(comprlen);
		if (is->fread(&cbuf[0], comprlen) != comprlen) goto error;
		int e = uncompress(&markers_array[0], &destlen, &cbuf[0], comprlen);
		if (e != Z_OK && e != Z_BUF_ERROR) goto error;
		return false;
	}
error:
	return true;
}
// ----------------------------------------------------------
void MARKERS::ToggleMarker(int frame)
{
	if (markers_array[frame] & MARKER_FLAG_BIT)
		markers_array[frame] &= ~MARKER_FLAG_BIT;
	else
		markers_array[frame] |= MARKER_FLAG_BIT;
}

void MARKERS::insertEmpty(int at, int frames)
{
	if(at == -1) 
	{
		markers_array.resize(markers_array.size() + frames);
	} else
	{
		markers_array.insert(markers_array.begin() + at, frames, 0);
	}
}

void MARKERS::truncateAt(int frame)
{
	markers_array.resize(frame);
}



