/* ---------------------------------------------------------------------------------
Implementation file of Bookmark class
Copyright (c) 2011-2012 AnS

(The MIT License)
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------------
Bookmark - Single Bookmark data

* stores all info of one specific Bookmark: movie snapshot, a savestate of 1 frame, a screenshot of the frame, a state of flashing for this Bookmark's row
* saves and loads the data from a project file. On error: sends warning to caller
* implements procedure of "Bookmark set": creating movie snapshot, setting key frame on current Playback position, copying savestate from Greenzone, making and compressing screenshot, launching flashing animation
* launches respective flashings for "Bookmark jump" and "Branch deploy"
------------------------------------------------------------------------------------ */

#include "taseditor_project.h"
#include "zlib.h"

extern TASEDITOR_CONFIG taseditor_config;
extern GREENZONE greenzone;
extern HISTORY history;

extern uint8 *XBuf;
extern uint8 *XBackBuf;

BOOKMARK::BOOKMARK()
{
	not_empty = false;
}

void BOOKMARK::init()
{
	free();
}
void BOOKMARK::free()
{
	not_empty = false;
	flash_type = flash_phase = floating_phase = 0;
	SNAPSHOT tmp;
	snapshot = tmp;
	savestate.resize(0);
	saved_screenshot.resize(0);
}

bool BOOKMARK::checkDiffFromCurrent()
{
	// check if the Bookmark data differs from current project/MovieData/Markers/settings
	if (not_empty && snapshot.keyframe == currFrameCounter)
	{
		if (snapshot.inputlog.size == currMovieData.getNumRecords() && snapshot.inputlog.findFirstChange(currMovieData) < 0)
		{
			if (!snapshot.MarkersDifferFromCurrent())
			{
				if (snapshot.inputlog.has_hot_changes == taseditor_config.enable_hot_changes)
				{
					return false;
				}
			}
		}
	}
	return true;
}

void BOOKMARK::set()
{
	// copy Input and Hotchanges
	snapshot.init(currMovieData, taseditor_config.enable_hot_changes);
	snapshot.keyframe = currFrameCounter;
	if (taseditor_config.enable_hot_changes)
		snapshot.inputlog.copyHotChanges(&history.GetCurrentSnapshot().inputlog);
	// copy savestate
	savestate = greenzone.GetSavestate(currFrameCounter);
	// save screenshot
	uLongf comprlen = (SCREENSHOT_SIZE>>9)+12 + SCREENSHOT_SIZE;
	saved_screenshot.resize(comprlen);
	// compress screenshot data
	if (taseditor_config.branch_scr_hud)
		compress(&saved_screenshot[0], &comprlen, XBuf, SCREENSHOT_SIZE);
	else
		compress(&saved_screenshot[0], &comprlen, XBackBuf, SCREENSHOT_SIZE);
	saved_screenshot.resize(comprlen);

	not_empty = true;
	flash_phase = FLASH_PHASE_MAX;
	flash_type = FLASH_TYPE_SET;
}

void BOOKMARK::jumped()
{
	flash_phase = FLASH_PHASE_MAX;
	flash_type = FLASH_TYPE_JUMP;
}

void BOOKMARK::deployed()
{
	flash_phase = FLASH_PHASE_MAX;
	flash_type = FLASH_TYPE_DEPLOY;
}

void BOOKMARK::save(EMUFILE *os)
{
	if (not_empty)
	{
		write8le(1, os);
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
	not_empty = (tmp != 0);
	if (not_empty)
	{
		// read snapshot
		if (snapshot.load(is)) return true;
		// read savestate
		int size;
		if (!read32le(&size, is)) return true;
		savestate.resize(size);
		if ((int)is->fread(&savestate[0], size) < size) return true;
		// read saved_screenshot
		if (!read32le(&size, is)) return true;
		saved_screenshot.resize(size);
		if ((int)is->fread(&saved_screenshot[0], size) < size) return true;
	} else
	{
		free();
	}
	// all ok - reset vars
	flash_type = flash_phase = floating_phase = 0;
	return false;
}
bool BOOKMARK::skipLoad(EMUFILE *is)
{
	uint8 tmp;
	if (!read8le(&tmp, is)) return true;
	if (tmp != 0)
	{
		// read snapshot
		if (snapshot.skipLoad(is)) return true;
		// read savestate
		int size;
		if (!read32le(&size, is)) return true;
		if (is->fseek(size, SEEK_CUR)) return true;
		// read saved_screenshot
		if (!read32le(&size, is)) return true;
		if (is->fseek(size, SEEK_CUR)) return true;
	}
	// all ok
	return false;
}
// ----------------------------------------------------------

