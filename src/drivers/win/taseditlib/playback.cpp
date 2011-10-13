//Implementation file of Playback class

#include "movie.h"
#include "../common.h"
#include "taseditproj.h"
#include "../tasedit.h"

extern HWND hwndProgressbar, hwndList, hwndRewind, hwndForward;
extern void FCEU_printf(char *format, ...);
extern bool turbo;
extern GREENZONE greenzone;

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
	old_emu_paused = emu_paused = true;
	SeekingStop();
}
void PLAYBACK::update()
{
	// pause when seeking hit pauseframe
	if(!FCEUI_EmulationPaused())
		if(pauseframe && pauseframe <= currFrameCounter + 1)
			SeekingStop();

	// update seeking progressbar
	old_emu_paused = emu_paused;
	emu_paused = (FCEUI_EmulationPaused() != 0);
	if (pauseframe && !emu_paused)
	{
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

	// update flashing pauseframe
	if (old_pauseframe != pauseframe && old_pauseframe) RedrawRow(old_pauseframe-1);
	old_pauseframe = pauseframe;
	old_show_pauseframe = show_pauseframe;
	if (pauseframe)
		show_pauseframe = (int)(clock() / PAUSEFRAME_BLINKING_PERIOD) & 1;
	else
		show_pauseframe = false;
	if (old_show_pauseframe != show_pauseframe) RedrawRow(pauseframe-1);

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
		rewind_button_state = (Button_GetState(hwndRewind) & BST_PUSHED) != 0;
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
		if (forward_button_state)
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
}

void PLAYBACK::RewindFrame()
{
	if (currFrameCounter > 0) JumpToFrame(currFrameCounter-1);
	turbo = false;
	FollowPlayback();
}
void PLAYBACK::ForwardFrame()
{
	JumpToFrame(currFrameCounter+1);
	turbo = false;
	FollowPlayback();
}

void PLAYBACK::StartFromZero()
{
	poweron(true);
	currFrameCounter = 0;
	greenzone.TryDumpIncremental();
}

bool PLAYBACK::JumpToFrame(int index)
{
	// Returns true if a jump to the frame is made, false if started seeking or if nothing's done
	if (index<0) return false;

	if (index >= greenzone.greenZoneCount)
	{
		// handle jump outside greenzone
		if (JumpToFrame(greenzone.greenZoneCount-1))
			// seek from the end of greenzone
			SeekingStart(index+1);
		return false;
	}
	/* Handle jumps inside greenzone. */
	if (greenzone.loadTasSavestate(index))
	{
		currFrameCounter = index;
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
	SeekingStart(index+1);
	return false;
}

void PLAYBACK::restorePosition()
{
	if (pauseframe)
		JumpToFrame(pauseframe-1);
	else
		JumpToFrame(currFrameCounter);
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






