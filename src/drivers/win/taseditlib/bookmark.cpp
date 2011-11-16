//Implementation file of Bookmark class

#include "movie.h"
#include "../common.h"
#include "taseditproj.h"
//#include "../tasedit.h"
#include "zlib.h"

extern GREENZONE greenzone;
extern INPUT_HISTORY history;

extern bool TASEdit_branch_scr_hud;
extern bool TASEdit_enable_hot_changes;

extern uint8 *XBuf;
extern uint8 *XBackBuf;

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
	// copy input and hotchanges
	snapshot.init(currMovieData, TASEdit_enable_hot_changes);
	snapshot.jump_frame = currFrameCounter;
	if (TASEdit_enable_hot_changes)
		snapshot.copyHotChanges(&history.GetCurrentSnapshot());
	// copy savestate
	savestate = greenzone.savestates[currFrameCounter];
	// save screenshot
	uLongf comprlen = (SCREENSHOT_SIZE>>9)+12 + SCREENSHOT_SIZE;
	saved_screenshot.resize(comprlen);
	// compress screenshot data
	if (TASEdit_branch_scr_hud)
		compress(&saved_screenshot[0], &comprlen, XBuf, SCREENSHOT_SIZE);
	else
		compress(&saved_screenshot[0], &comprlen, XBackBuf, SCREENSHOT_SIZE);
	saved_screenshot.resize(comprlen);

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
		size = saved_screenshot.size();
		write32le(size, os);
		os->fwrite(&saved_screenshot[0], size);
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
		parent_branch = *(int8*)(&tmp);
		// read snapshot
		if (snapshot.load(is)) return true;
		// read savestate
		int size;
		if (!read32le((uint32 *)&size, is)) return true;
		savestate.resize(size);
		if ((int)is->fread(&savestate[0], size) < size) return true;
		// read saved_screenshot
		if (!read32le((uint32 *)&size, is)) return true;
		saved_screenshot.resize(size);
		if ((int)is->fread(&saved_screenshot[0], size) < size) return true;
	}
	// all ok
	flash_type = flash_phase = 0;
	return false;
}
// ----------------------------------------------------------



