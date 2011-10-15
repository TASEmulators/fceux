//Implementation file of Markers class

#include "movie.h"
#include "../common.h"
#include "taseditproj.h"
//#include "../tasedit.h"


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
	if (markers_array.size() < currMovieData.getNumRecords())
		markers_array.resize(currMovieData.getNumRecords());
}

void MARKERS::save(EMUFILE *os)
{
	// write size
	int size = markers_array.size();
	write32le(size, os);
	// write array
	os->fwrite(markers_array.data(), size);
}
bool MARKERS::load(EMUFILE *is)
{
	markers_array.resize(currMovieData.getNumRecords());
	int size;
	if (read32le((uint32 *)&size, is) && size == currMovieData.getNumRecords())
	{
		// read array
		if ((int)is->fread(markers_array.data(), size) == size) return true;
	}
error:
	FCEU_printf("Error loading markers\n");
	return false;
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

