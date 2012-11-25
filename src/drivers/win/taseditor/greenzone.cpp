/* ---------------------------------------------------------------------------------
Implementation file of Greenzone class
Copyright (c) 2011-2012 AnS

(The MIT License)
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------------
Greenzone - Access zone
[Singleton]

* stores array of savestates, used for faster movie navigation by Playback cursor
* also stores LagLog of current movie Input
* saves and loads the data from a project file. On error: truncates Greenzone to last successfully read savestate
* regularly checks if there's a savestate of current emulation state, if there's no such savestate in array then creates one and updates lag info for previous frame
* implements the working of "Auto-adjust Input according to lag" feature
* regularly runs gradual cleaning of the savestates array (for memory saving), deleting oldest savestates
* on demand: (when movie Input was changed) truncates the size of Greenzone, deleting savestates that became irrelevant because of new Input. After truncating it may also move Playback cursor (which must always reside within Greenzone) and may launch Playback seeking
* stores resources: save id, properties of gradual cleaning, timing of cleaning
------------------------------------------------------------------------------------ */

#include "taseditor_project.h"
#include "state.h"
#include "zlib.h"

extern TASEDITOR_CONFIG taseditor_config;
extern TASEDITOR_PROJECT project;
extern PLAYBACK playback;
extern HISTORY history;
extern BOOKMARKS bookmarks;
extern MARKERS_MANAGER markers_manager;
extern PIANO_ROLL piano_roll;
extern SELECTION selection;

extern char lagFlag;

char greenzone_save_id[GREENZONE_ID_LEN] = "GREENZONE";
char greenzone_skipsave_id[GREENZONE_ID_LEN] = "GREENZONX";

GREENZONE::GREENZONE()
{
}

void GREENZONE::init()
{
	reset();
	next_cleaning_time = clock() + TIME_BETWEEN_CLEANINGS;
}
void GREENZONE::free()
{
	savestates.resize(0);
	greenZoneCount = 0;
	laglog.reset();
}
void GREENZONE::reset()
{
	free();
}
void GREENZONE::update()
{
	// keep collecting savestates, this code must be executed at the end of every frame
	if (taseditor_config.enable_greenzoning)
	{
		CollectCurrentState();
	} else
	{
		// just update Greenzone upper limit
		if (greenZoneCount <= currFrameCounter)
			greenZoneCount = currFrameCounter + 1;
	}

	// run cleaning from time to time
	if (clock() > next_cleaning_time)
		RunGreenzoneCleaning();

	// also log lag frames
	if (currFrameCounter > 0)
	{
		// lagFlag indicates that lag was in previous frame
		int old_lagFlag = laglog.GetLagInfoAtFrame(currFrameCounter - 1);
		// Auto-adjust Input according to lag
		if (taseditor_config.adjust_input_due_to_lag && old_lagFlag != LAGGED_DONTKNOW)
		{
			if ((old_lagFlag == LAGGED_YES) && !lagFlag)
			{
				// there's no more lag on previous frame - shift Input up 1 or more frames
				AdjustUp();
			} else if ((old_lagFlag == LAGGED_NO) && lagFlag)
			{
				// there's new lag on previous frame - shift Input down 1 frame
				AdjustDown();
			}
		} else
		{
			if (lagFlag && (old_lagFlag != LAGGED_YES))
				laglog.SetLagInfo(currFrameCounter - 1, true);
			else if (!lagFlag && old_lagFlag != LAGGED_NO)
				laglog.SetLagInfo(currFrameCounter - 1, false);
		}
	}
}

void GREENZONE::CollectCurrentState()
{
	if ((int)savestates.size() <= currFrameCounter)
		savestates.resize(currFrameCounter + 1);
	// if frame is not saved - log savestate
	if (!savestates[currFrameCounter].size())
	{
		EMUFILE_MEMORY ms(&savestates[currFrameCounter]);
		FCEUSS_SaveMS(&ms, Z_DEFAULT_COMPRESSION);
		ms.trim();
	}
	if (greenZoneCount <= currFrameCounter)
		greenZoneCount = currFrameCounter + 1;
}

bool GREENZONE::LoadSavestate(unsigned int frame)
{
	if (frame >= savestates.size() || !savestates[frame].size())
		return false;
	EMUFILE_MEMORY ms(&savestates[frame]);
	return FCEUSS_LoadFP(&ms, SSLOADPARAM_NOBACKUP);
}

void GREENZONE::RunGreenzoneCleaning()
{
	bool changed = false;
	int i = currFrameCounter - taseditor_config.greenzone_capacity;
	if (i <= 0) goto finish;	// zeroth frame should not be cleaned
	int limit;
	// 2x of 1/2
	limit = i - 2 * taseditor_config.greenzone_capacity;
	if (limit < 0) limit = 0;
	for (; i > limit; i--)
	{
		if (i & 0x1)
			changed = changed | ClearSavestateAndFreeMemory(i);
	}
	if (i < 0) goto finish;
	// 4x of 1/4
	limit = i - 4 * taseditor_config.greenzone_capacity;
	if (limit < 0) limit = 0;
	for (; i > limit; i--)
	{
		if (i & 0x3)
			changed = changed | ClearSavestateAndFreeMemory(i);
	}
	if (i < 0) goto finish;
	// 8x of 1/8
	limit = i - 8 * taseditor_config.greenzone_capacity;
	if (limit < 0) limit = 0;
	for (; i > limit; i--)
	{
		if (i & 0x7)
			changed = changed | ClearSavestateAndFreeMemory(i);
	}
	if (i < 0) goto finish;
	// 16x of 1/16
	limit = i - 16 * taseditor_config.greenzone_capacity;
	if (limit < 0) limit = 0;
	for (; i > limit; i--)
	{
		if (i & 0xF)
			changed = changed | ClearSavestateAndFreeMemory(i);
	}
	// clear all remaining
	for (; i > 0; i--)
	{
		changed = changed | ClearSavestateAndFreeMemory(i);
	}
finish:
	if (changed)
	{
		piano_roll.RedrawList();
		bookmarks.RedrawBookmarksList();
	}
	// shedule next cleaning
	next_cleaning_time = clock() + TIME_BETWEEN_CLEANINGS;
}

// returns true if actually cleared savestate data
bool GREENZONE::ClearSavestate(unsigned int index)
{
	if (index < savestates.size() && savestates[index].size())
	{
	    savestates[index].resize(0);
		return true;
	} else
	{
		return false;
	}
}

bool GREENZONE::ClearSavestateAndFreeMemory(unsigned int index)
{
	if (index < savestates.size() && savestates[index].size())
	{
	    savestates[index].swap(std::vector<uint8>());
		return true;
	} else
	{
		return false;
	}
}

void GREENZONE::UnGreenzoneSelectedFrames()
{
	SelectionFrames* current_selection = selection.MakeStrobe();
	if (current_selection->size() == 0) return;
	bool changed = false;
	int size = savestates.size();
	int start_index = *current_selection->begin();
	int end_index = *current_selection->rbegin();
	SelectionFrames::reverse_iterator current_selection_rend = current_selection->rend();
	// degreenzone frames, going backwards
	for (SelectionFrames::reverse_iterator it(current_selection->rbegin()); it != current_selection_rend; it++)
		changed = changed | ClearSavestateAndFreeMemory(*it);
	if (changed)
	{
		piano_roll.RedrawList();
		bookmarks.RedrawBookmarksList();
	}
}

void GREENZONE::save(EMUFILE *os, bool really_save)
{
	if (really_save)
	{
		// write "GREENZONE" string
		os->fwrite(greenzone_save_id, GREENZONE_ID_LEN);
		// write LagLog
		laglog.save(os);
		// write size
		write32le(greenZoneCount, os);
		// write Playback cursor position
		write32le(currFrameCounter, os);
		CollectCurrentState();
		// write savestates
		int frame, size;
		int last_tick = 0;
		RunGreenzoneCleaning();
		if (greenZoneCount > (int)savestates.size()) greenZoneCount = savestates.size();
		for (frame = 0; frame < greenZoneCount; ++frame)
		{
			// update TASEditor progressbar from time to time
			if (frame / PROGRESSBAR_UPDATE_RATE > last_tick)
			{
				playback.SetProgressbar(frame, greenZoneCount);
				last_tick = frame / PROGRESSBAR_UPDATE_RATE;
			}
			if (!savestates[frame].size()) continue;
			write32le(frame, os);
			// write savestate
			size = savestates[frame].size();
			write32le(size, os);
			os->fwrite(&savestates[frame][0], size);
		}
		// write -1 as eof for greenzone
		write32le(-1, os);
	} else
	{
		// write "GREENZONX" string
		os->fwrite(greenzone_skipsave_id, GREENZONE_ID_LEN);
		// write LagLog
		laglog.save(os);
		// write Playback cursor position
		write32le(currFrameCounter, os);
		if (currFrameCounter > 0)
		{
			// write ONE savestate for currFrameCounter
			CollectCurrentState();
			int size = savestates[currFrameCounter].size();
			write32le(size, os);
			os->fwrite(&savestates[currFrameCounter][0], size);
		}
	}
}
// returns true if couldn't load
bool GREENZONE::load(EMUFILE *is, unsigned int offset)
{
	free();
	if (offset)
	{
		if (is->fseek(offset, SEEK_SET)) goto error;
	} else
	{
		reset();
		playback.StartFromZero();		// reset Playback cursor to frame 0
		return false;
	}
	int frame = 0, prev_frame = -1, size = 0;
	int last_tick = 0;
	// read "GREENZONE" string
	char save_id[GREENZONE_ID_LEN];
	if ((int)is->fread(save_id, GREENZONE_ID_LEN) < GREENZONE_ID_LEN) goto error;
	if (!strcmp(greenzone_skipsave_id, save_id))
	{
		// string says to skip loading Greenzone
		// read LagLog
		laglog.load(is);
		// read Playback cursor position
		if (read32le(&frame, is))
		{
			currFrameCounter = frame;
			greenZoneCount = currFrameCounter + 1;
			savestates.resize(greenZoneCount);
			if (currFrameCounter)
			{
				// there must be one savestate in the file
				if (read32le(&size, is) && size >= 0)
				{
					savestates[frame].resize(size);
					if (is->fread(&savestates[frame][0], size) == size)
					{
						if (LoadSavestate(currFrameCounter))
						{
							FCEU_printf("No Greenzone in the file\n");
							return false;
						}
					}
				}
			} else
			{
				// literally no Greenzone in the file, but this is still not a error
				reset();
				playback.StartFromZero();		// reset Playback cursor to frame 0
				FCEU_printf("No Greenzone in the file, Playback at frame 0\n");
				return false;
			}
		}
		goto error;
	}
	if (strcmp(greenzone_save_id, save_id)) goto error;		// string is not valid
	// read LagLog
	laglog.load(is);
	// read size
	if (read32le(&size, is) && size >= 0 && size <= currMovieData.getNumRecords())
	{
		greenZoneCount = size;
		savestates.resize(greenZoneCount);
		// read Playback cursor position
		if (read32le(&frame, is))
		{
			currFrameCounter = frame;
			int greenzone_tail_frame = currFrameCounter - taseditor_config.greenzone_capacity;
			int greenzone_tail_frame2 = greenzone_tail_frame - 2 * taseditor_config.greenzone_capacity;
			int greenzone_tail_frame4 = greenzone_tail_frame - 4 * taseditor_config.greenzone_capacity;
			int greenzone_tail_frame8 = greenzone_tail_frame - 8 * taseditor_config.greenzone_capacity;
			int greenzone_tail_frame16 = greenzone_tail_frame - 16 * taseditor_config.greenzone_capacity;
			// read savestates
			while(1)
			{
				if (!read32le(&frame, is)) break;
				if (frame < 0) break;		// -1 = eof
				// update TASEditor progressbar from time to time
				if (frame / PROGRESSBAR_UPDATE_RATE > last_tick)
				{
					playback.SetProgressbar(frame, greenZoneCount);
					last_tick = frame / PROGRESSBAR_UPDATE_RATE;
				}
				// read savestate
				if (!read32le(&size, is)) break;
				if (size < 0) break;
				if (frame <= greenzone_tail_frame16
					|| (frame <= greenzone_tail_frame8 && (frame & 0xF))
					|| (frame <= greenzone_tail_frame4 && (frame & 0x7))
					|| (frame <= greenzone_tail_frame2 && (frame & 0x3))
					|| (frame <= greenzone_tail_frame && (frame & 0x1)))
				{
					// skip loading this savestate
					if (is->fseek(size, SEEK_CUR) != 0) break;
				} else
				{
					// load this savestate
					savestates[frame].resize(size);
					if ((int)is->fread(&savestates[frame][0], size) < size) break;
					prev_frame = frame;			// successfully read one Greenzone frame info
				}
			}
			if (prev_frame+1 == greenZoneCount)
			{
				// everything went fine - load savestate at cursor position
				if (LoadSavestate(currFrameCounter))
					return false;
			}
			// uh, okay, but maybe we managed to read at least something useful from the file
			// first see if original position of currFrameCounter was read successfully
			if (LoadSavestate(currFrameCounter))
			{
				greenZoneCount = prev_frame+1;		// cut greenZoneCount to last good frame
				FCEU_printf("Greenzone loaded partially\n");
				return false;
			}
			// then at least jump to some frame that was read successfully
			for (; prev_frame >= 0; prev_frame--)
			{
				if (LoadSavestate(prev_frame))
				{
					currFrameCounter = prev_frame;
					greenZoneCount = prev_frame+1;		// cut greenZoneCount to this good frame
					FCEU_printf("Greenzone loaded partially, Playback moved to the end of greenzone\n");
					return false;
				}
			}
		}
	}
error:
	FCEU_printf("Error loading Greenzone\n");
	reset();
	playback.StartFromZero();		// reset Playback cursor to frame 0
	return true;
}
// -------------------------------------------------------------------------------------------------
void GREENZONE::AdjustUp()
{
	int at = currFrameCounter - 1;		// at = the frame above currFrameCounter
	// find how many consequent lag frames there are
	int num_frames_to_erase = 0;
	while (laglog.GetLagInfoAtFrame(at++) == LAGGED_YES)
		num_frames_to_erase++;

	if (num_frames_to_erase > 0)
	{
		bool markers_changed = false;
		// delete these frames of lag
		currMovieData.eraseRecords(currFrameCounter - 1, num_frames_to_erase);
		laglog.EraseFrame(currFrameCounter - 1, num_frames_to_erase);
		if (taseditor_config.bind_markers)
		{
			if (markers_manager.EraseMarker(currFrameCounter - 1, num_frames_to_erase))
				markers_changed = true;
		}
		// update movie data size, because Playback cursor must always be inside the movie
		// if movie length is less or equal to currFrame, pad it with empty frames
		if (((int)currMovieData.records.size() - 1) <= currFrameCounter)
			currMovieData.insertEmpty(-1, currFrameCounter - ((int)currMovieData.records.size() - 1));
		// update Piano Roll (reduce it if needed)
		piano_roll.UpdateItemCount();
		// register changes
		int first_input_changes = history.RegisterAdjustLag(currFrameCounter - 1, 0 - num_frames_to_erase);
		// if Input in the frame above currFrameCounter has changed then invalidate Greenzone (rewind 1 frame back)
		// also if the frame above currFrameCounter is lag frame then rewind 1 frame (invalidate Greenzone), because maybe this frame also needs lag removal
		if ((first_input_changes >= 0 && first_input_changes < currFrameCounter) || (laglog.GetLagInfoAtFrame(currFrameCounter - 1) != LAGGED_NO))
		{
			// custom invalidation procedure, not retriggering LostPosition/PauseFrame
			Invalidate(first_input_changes);
			bool emu_was_paused = (FCEUI_EmulationPaused() != 0);
			int saved_pause_frame = playback.GetPauseFrame();
			playback.EnsurePlaybackIsInsideGreenzone();
			if (saved_pause_frame >= 0)
				playback.SeekingStart(saved_pause_frame);
			if (emu_was_paused)
				playback.PauseEmulation();
		} else
		{
			// just invalidate Greenzone after currFrameCounter (this is necessary in order to force user to re-emulate everything after the point, because the lag log data after the currFrameCounter is now in unknown state and it should be collected again)
			Invalidate(currFrameCounter);
		}
		if (markers_changed)
			selection.must_find_current_marker = playback.must_find_current_marker = true;
	}
}
void GREENZONE::AdjustDown()
{
	int at = currFrameCounter - 1;
	bool markers_changed = false;
	// clone frame and insert lag
	currMovieData.cloneRegion(at, 1);
	laglog.InsertFrame(at, true, 1);
	if (taseditor_config.bind_markers)
	{
		if (markers_manager.insertEmpty(at, 1))
			markers_changed = true;
	}
	// register changes
	int first_input_changes = history.RegisterAdjustLag(at, +1);
	// If Input in the frame above currFrameCounter has changed then invalidate Greenzone (rewind 1 frame back)
	// This should never actually happen, because we clone the frame, so the Input doesn't change
	// But the check should remain, in case we decide to insert blank frame instead of cloning
	if (first_input_changes >= 0 && first_input_changes < currFrameCounter)
	{
		// custom invalidation procedure, not retriggering LostPosition/PauseFrame
		Invalidate(first_input_changes);
		bool emu_was_paused = (FCEUI_EmulationPaused() != 0);
		int saved_pause_frame = playback.GetPauseFrame();
		playback.EnsurePlaybackIsInsideGreenzone();
		if (saved_pause_frame >= 0)
			playback.SeekingStart(saved_pause_frame);
		if (emu_was_paused)
			playback.PauseEmulation();
	} else
	{
		// just invalidate Greenzone after currFrameCounter
		Invalidate(currFrameCounter);
	}
	if (markers_changed)
		selection.must_find_current_marker = playback.must_find_current_marker = true;
}
// -------------------------------------------------------------------------------------------------
// invalidate and restore playback
void GREENZONE::InvalidateAndCheck(int after)
{
	if (after >= 0)
	{
		if (after >= currMovieData.getNumRecords())
			after = currMovieData.getNumRecords() - 1;
		// clear all savestates that became irrelevant
		for (int i = savestates.size() - 1; i > after; i--)
			ClearSavestate(i);
		if (greenZoneCount > after + 1 || currFrameCounter > after)
		{
			greenZoneCount = after + 1;
			currMovieData.rerecordCount++;
			// either set Playback cursor to be inside the Greenzone or run seeking to restore Playback cursor position
			if (currFrameCounter >= greenZoneCount)
			{
				if (playback.GetPauseFrame() >= 0 && !FCEUI_EmulationPaused())
				{
					// emulator was running, so continue seeking
					playback.jump(playback.GetPauseFrame());
				} else
				{
					playback.SetLostPosition(currFrameCounter);
					if (taseditor_config.restore_position)
						// start seeking to the green arrow
						playback.jump(playback.GetLostPosition());
					else
						playback.EnsurePlaybackIsInsideGreenzone();
				}
			}
		}
	}
	// redraw Piano Roll even if Greenzone didn't change
	piano_roll.RedrawList();
	bookmarks.RedrawBookmarksList();
}
// This version doesn't restore playback, may be used only by Branching, Recording and AdjustLag functions!
void GREENZONE::Invalidate(int after)
{
	if (after >= 0)
	{
		if (after >= currMovieData.getNumRecords())
			after = currMovieData.getNumRecords() - 1;
		// clear all savestates that became irrelevant
		for (int i = savestates.size() - 1; i > after; i--)
			ClearSavestate(i);
		if (greenZoneCount > after + 1)
		{
			greenZoneCount = after + 1;
			currMovieData.rerecordCount++;
		}
	}
	// redraw Piano Roll even if Greenzone didn't change
	piano_roll.RedrawList();
	bookmarks.RedrawBookmarksList();
}
// -------------------------------------------------------------------------------------------------
int GREENZONE::FindBeginningOfGreenZone(int starting_index)
{
	for (int i = starting_index; i < greenZoneCount; ++i)
		if (savestates[i].size()) return i;
	return -1;	// error
}

// getters
int GREENZONE::GetSize()
{
	return greenZoneCount;
}

// this should only be used by Bookmark Set procedure
std::vector<uint8>& GREENZONE::GetSavestate(int frame)
{
	return savestates[frame];
}
// this function should only be used by Bookmark Deploy procedure
void GREENZONE::WriteSavestate(int frame, std::vector<uint8>& savestate)
{
	if ((int)savestates.size() <= frame)
		savestates.resize(frame + 1);
	savestates[frame] = savestate;
	if (greenZoneCount <= frame)
		greenZoneCount = frame + 1;
}

bool GREENZONE::SavestateIsEmpty(unsigned int frame)
{
	if ((int)frame < greenZoneCount && frame < savestates.size() && savestates[frame].size())
		return false;
	else
		return true;
}

