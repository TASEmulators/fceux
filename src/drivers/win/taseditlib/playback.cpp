//Implementation file of Playback class

#include "movie.h"
#include "../common.h"
#include "taseditproj.h"
#include "../tasedit.h"

#ifdef _S9XLUA_H
extern void ForceExecuteLuaFrameFunctions();
#endif

extern HWND hwndProgressbar, hwndList, hwndRewind, hwndForward, hwndRewindFull, hwndForwardFull;
extern void FCEU_printf(char *format, ...);
extern bool turbo;
extern MARKERS markers;
extern GREENZONE greenzone;
extern bool Tasedit_rewind_now;

PLAYBACK::PLAYBACK()
{

}

void PLAYBACK::init()
{

	reset();
}
void PLAYBACK::reset()
{
	lastCursor = -1;
	pauseframe = old_pauseframe = 0;
	old_show_pauseframe = show_pauseframe = false;
	old_rewind_button_state = rewind_button_state = false;
	old_forward_button_state = forward_button_state = false;
	old_rewind_full_button_state = rewind_full_button_state = false;
	old_forward_full_button_state = forward_full_button_state = false;
	old_emu_paused = emu_paused = true;
	SeekingStop();
}
void PLAYBACK::update()
{
	// pause when seeking hit pauseframe
	if(!FCEUI_EmulationPaused())
		if(pauseframe && pauseframe <= currFrameCounter + 1)
			SeekingStop();

	// update flashing pauseframe
	if (old_pauseframe != pauseframe && old_pauseframe) RedrawRow(old_pauseframe-1);
	old_pauseframe = pauseframe;
	old_show_pauseframe = show_pauseframe;
	if (pauseframe)
	{
		if (emu_paused)
			show_pauseframe = (int)(clock() / PAUSEFRAME_BLINKING_PERIOD_PAUSED) & 1;
		else
			show_pauseframe = (int)(clock() / PAUSEFRAME_BLINKING_PERIOD_SEEKING) & 1;
	} else show_pauseframe = false;
	if (old_show_pauseframe != show_pauseframe) RedrawRow(pauseframe-1);

	// update seeking progressbar
	old_emu_paused = emu_paused;
	emu_paused = (FCEUI_EmulationPaused() != 0);
	if (pauseframe)
	{
		if (old_show_pauseframe != show_pauseframe)
			SetProgressbar(currFrameCounter - seeking_start_frame, pauseframe-seeking_start_frame);
	} else if (old_emu_paused != emu_paused)
	{
		// emulator got paused/unpaused externally
		if (old_emu_paused && !emu_paused)
			// externally unpaused - progressbar should be empty
			SetProgressbar(0, 1);
		else
			// externally paused - progressbar should be full
			SetProgressbar(1, 1);
	}

	//update the playback cursor
	if(currFrameCounter != lastCursor)
	{
		FollowPlayback();
		//update the old and new rows
		RedrawRow(lastCursor);
		RedrawRow(currFrameCounter);
		UpdateWindow(hwndList);
		lastCursor = currFrameCounter;
	}
	
	// update < and > buttons
	if(emu_paused)
	{
		old_rewind_button_state = rewind_button_state;
		rewind_button_state = ((Button_GetState(hwndRewind) & BST_PUSHED) != 0 || Tasedit_rewind_now);
		if (rewind_button_state)
		{
			if (!old_rewind_button_state)
			{
				button_hold_time = clock();
				RewindFrame();
			} else if (button_hold_time + HOLD_REPEAT_DELAY < clock())
			{
				RewindFrame();
			}
		}
		old_forward_button_state = forward_button_state;
		forward_button_state = (Button_GetState(hwndForward) & BST_PUSHED) != 0;
		if (forward_button_state && !rewind_button_state)
		{
			if (!old_forward_button_state)
			{
				button_hold_time = clock();
				ForwardFrame();
			} else if (button_hold_time + HOLD_REPEAT_DELAY < clock())
			{
				ForwardFrame();
			}
		}
	}
	// update << and >> buttons
	old_rewind_full_button_state = rewind_full_button_state;
	rewind_full_button_state = ((Button_GetState(hwndRewindFull) & BST_PUSHED) != 0);
	if (rewind_full_button_state && !rewind_button_state && !forward_button_state)
	{
		if (!old_rewind_full_button_state)
		{
			button_hold_time = clock();
			RewindFull();
		} else if (button_hold_time + HOLD_REPEAT_DELAY < clock())
		{
			RewindFull();
		}
	}
	old_forward_full_button_state = forward_full_button_state;
	forward_full_button_state = (Button_GetState(hwndForwardFull) & BST_PUSHED) != 0;
	if (forward_full_button_state && !rewind_button_state && !forward_button_state && !rewind_full_button_state)
	{
		if (!old_forward_full_button_state)
		{
			button_hold_time = clock();
			ForwardFull();
		} else if (button_hold_time + HOLD_REPEAT_DELAY < clock())
		{
			ForwardFull();
		}
	}

}

void PLAYBACK::updateProgressbar()
{
	if (pauseframe)
	{
		SetProgressbar(currFrameCounter - seeking_start_frame, pauseframe-seeking_start_frame);
	} else
	{
		if (emu_paused)
			SetProgressbar(1, 1);
		else
			SetProgressbar(0, 1);
	}
}

void PLAYBACK::ToggleEmulationPause()
{
	if (FCEUI_EmulationPaused())
		UnpauseEmulation();
	else
		PauseEmulation();
}
void PLAYBACK::PauseEmulation()
{
	FCEUI_SetEmulationPaused(1);
	//RedrawList();	// to show some "pale" greenzone
	// make some additional stuff
}
void PLAYBACK::UnpauseEmulation()
{
	FCEUI_SetEmulationPaused(0);
	// make some additional stuff
}

void PLAYBACK::SeekingStart(int finish_frame)
{
	seeking_start_frame = currFrameCounter;
	pauseframe = finish_frame;
	turbo = true;
	UnpauseEmulation();
}
void PLAYBACK::SeekingStop()
{
	pauseframe = 0;
	turbo = false;
	PauseEmulation();
	SetProgressbar(1, 1);
	//RedrawList();	// to show some "pale" greenzone
}

void PLAYBACK::RewindFrame()
{
	if (currFrameCounter > 0) jump(currFrameCounter-1);
}
void PLAYBACK::ForwardFrame()
{
	jump(currFrameCounter+1);
	turbo = false;
}
void PLAYBACK::RewindFull()
{
	// jump to previous marker
	if (currFrameCounter > 0)
	{
		int index = currFrameCounter - 1;
		for (; index >= 0; index--)
			if (markers.markers_array[index] & MARKER_FLAG_BIT) break;
		if (index >= 0)
			jump(index);
		else if (currFrameCounter > 0)
			jump(0);
	}
}
void PLAYBACK::ForwardFull()
{
	// jump to next marker
	int last_frame = currMovieData.getNumRecords()-1;
	if (currFrameCounter < last_frame)
	{
		int index = currFrameCounter + 1;
		for (; index < last_frame; ++index)
			if (markers.markers_array[index] & MARKER_FLAG_BIT) break;
		if (index <= last_frame)
			jump(index);
		else if (currFrameCounter < last_frame)
			jump(last_frame);
	}
}

void PLAYBACK::StartFromZero()
{
	poweron(true);
	currFrameCounter = 0;
	greenzone.TryDumpIncremental();
}

void PLAYBACK::jump(int frame)
{
	if (JumpToFrame(frame))
	{
		ForceExecuteLuaFrameFunctions();
		FollowPlayback();
	}
}
void PLAYBACK::restorePosition()
{
	if (pauseframe)
		jump(pauseframe-1);
	else
		jump(currFrameCounter);
}

bool PLAYBACK::JumpToFrame(int index)
{
	// Returns true if a jump to the frame is made, false if started seeking outside greenzone or if nothing's done
	if (index < 0) return false;

	if (index >= greenzone.greenZoneCount)
	{
		// handle jump outside greenzone
		if (currFrameCounter == greenzone.greenZoneCount-1 || JumpToFrame(greenzone.greenZoneCount-1))
			// seek from the end of greenzone
			SeekingStart(index+1);
		return false;
	}
	/* Handle jumps inside greenzone. */
	if (greenzone.loadTasSavestate(index))
	{
		turbo = false;
		// if playback was seeking, pause emulation right here
		if (pauseframe) SeekingStop();
		return true;
	}
	//Search for an earlier frame with savestate
	int i = (index>0)? index-1 : 0;
	for (; i > 0; i--)
	{
		if (greenzone.loadTasSavestate(i)) break;
	}
	if (!i)
		StartFromZero();
	else
		currFrameCounter = i;
	// continue from the frame
	if (index != currFrameCounter)
		SeekingStart(index+1);
	return true;
}

int PLAYBACK::GetPauseFrame()
{
	if (show_pauseframe)
		return pauseframe-1;
	else
		return -1;
}

void PLAYBACK::SetProgressbar(int a, int b)
{
	SendMessage(hwndProgressbar, PBM_SETPOS, PROGRESSBAR_WIDTH * a / b, 0);
}






