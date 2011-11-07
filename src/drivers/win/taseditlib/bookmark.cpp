//Implementation file of Bookmark class

#include "movie.h"
#include "../common.h"
#include "taseditproj.h"
//#include "../tasedit.h"
#include "zlib.h"

extern GREENZONE greenzone;

BOOKMARK::BOOKMARK()
{

}

void BOOKMARK::init()
{
	not_empty = false;
	flash_type = flash_phase = 0;
	snapshot.jump_frame = -1;
	parent_branch = -1;		// -1 = root
}

void BOOKMARK::set()
{
	snapshot.init(currMovieData, false);
	snapshot.jump_frame = currFrameCounter;
	savestate = greenzone.savestates[currFrameCounter];
	// save screenshot

	not_empty = true;
	flash_phase = FLASH_PHASE_MAX;
	flash_type = FLASH_TYPE_SET;
}

void BOOKMARK::jump()
{
	flash_phase = FLASH_PHASE_MAX;
	flash_type = FLASH_TYPE_JUMP;
}

void BOOKMARK::unleashed()
{
	flash_phase = FLASH_PHASE_MAX;
	flash_type = FLASH_TYPE_UNLEASH;
}

void BOOKMARK::save(EMUFILE *os)
{
	if (not_empty)
	{
		write8le(1, os);
		// write parent_branch
		write8le((uint8)parent_branch, os);
		// write snapshot
		snapshot.save(os);
		// write savestate
		int size = savestate.size();
		write32le(size, os);
		os->fwrite(&savestate[0], size);
		// write saved_screenshot

	} else write8le((uint8)0, os);
}
// returns true if couldn't load
bool BOOKMARK::load(EMUFILE *is)
{
	uint8 tmp;
	if (!read8le(&tmp, is)) return true;
	not_empty = tmp != 0;
	if (not_empty)
	{
		// read parent_branch
		if (!read8le(&tmp, is)) return true;
		parent_branch = *(int8*)(&tmp);			// don't kill me for that
		// read snapshot
		if (snapshot.load(is)) return true;
		// read savestate
		int size;
		if (!read32le((uint32 *)&size, is)) return true;
		savestate.resize(size);
		if ((int)is->fread(&savestate[0], size) < size) return true;
		// read saved_screenshot

	}
	// all ok
	flash_type = flash_phase = 0;
	return false;
}
// ----------------------------------------------------------



