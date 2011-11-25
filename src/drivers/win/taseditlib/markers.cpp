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
void MARKERS::MakeCopy(std::vector<uint8> &destination_array)
{
	// copy array
	destination_array = markers_array;
	// copy notes


}
void MARKERS::RestoreFromCopy(std::vector<uint8> &source_array, int until_frame)
{
	if (until_frame >= 0)
	{
		// restore array up to and including the frame
		if ((int)markers_array.size() <= until_frame) markers_array.resize(until_frame+1);
		for (int i = until_frame; i >= 0; i--)
			markers_array[i] = source_array[i];
		// restore notes

	} else
	{
		// restore array
		markers_array = source_array;
		// restore notes

	}
}

int MARKERS::GetMarkersSize()
{
	return markers_array.size();
}

bool MARKERS::GetMarker(int frame)
{
	if (frame >= 0 && frame < (int)markers_array.size())
		return markers_array[frame] & MARKER_FLAG_BIT;
	return false;
}
void MARKERS::SetMarker(int frame)
{
	markers_array[frame] |= MARKER_FLAG_BIT;
}
void MARKERS::ClearMarker(int frame)
{
	markers_array[frame] &= ~MARKER_FLAG_BIT;
}
void MARKERS::EraseMarker(int frame)
{
	// check if there's a marker, delete note if needed
	markers_array.erase(markers_array.begin() + frame);
}
void MARKERS::ToggleMarker(int frame)
{
	if (frame >= 0 && frame < (int)markers_array.size())
	{
		if (markers_array[frame] & MARKER_FLAG_BIT)
			markers_array[frame] &= ~MARKER_FLAG_BIT;
		else
			markers_array[frame] |= MARKER_FLAG_BIT;
	}
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



