//Implementation file of Playback class
#include "taseditproj.h"
#include "..\tasedit.h"		// only for MARKER_NOTE_EDIT_UPPER

#ifdef _S9XLUA_H
extern void ForceExecuteLuaFrameFunctions();
#endif

extern HWND hwndTasEdit;
extern bool Tasedit_rewind_now;
extern bool turbo;
extern bool TASEdit_turbo_seek;
extern int marker_note_edit;
extern int search_similar_marker;

extern MARKERS current_markers;
extern GREENZONE greenzone;
extern TASEDIT_LIST tasedit_list;
extern BOOKMARKS bookmarks;

extern void UpdateMarkerNote();

LRESULT APIENTRY UpperMarkerEditWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
WNDPROC playbackMarkerEdit_oldWndproc;

// resources
char upperMarkerText[] = "Marker ";

PLAYBACK::PLAYBACK()
{
}

void PLAYBACK::init()
{
	hwndProgressbar = GetDlgItem(hwndTasEdit, IDC_PROGRESS1);
	SendMessage(hwndProgressbar, PBM_SETRANGE, 0, MAKELPARAM(0, PROGRESSBAR_WIDTH)); 
	hwndRewind = GetDlgItem(hwndTasEdit, TASEDIT_REWIND);
	hwndForward = GetDlgItem(hwndTasEdit, TASEDIT_FORWARD);
	hwndRewindFull = GetDlgItem(hwndTasEdit, TASEDIT_REWIND_FULL);
	hwndForwardFull = GetDlgItem(hwndTasEdit, TASEDIT_FORWARD_FULL);
	hwndPlaybackMarker = GetDlgItem(hwndTasEdit, IDC_PLAYBACK_MARKER);
	SendMessage(hwndPlaybackMarker, WM_SETFONT, (WPARAM)tasedit_list.hMarkersFont, 0);
	hwndPlaybackMarkerEdit = GetDlgItem(hwndTasEdit, IDC_PLAYBACK_MARKER_EDIT);
	SendMessage(hwndPlaybackMarkerEdit, EM_SETLIMITTEXT, MAX_NOTE_LEN - 1, 0);
	SendMessage(hwndPlaybackMarkerEdit, WM_SETFONT, (WPARAM)tasedit_list.hMarkersEditFont, 0);
	// subclass the edit control
	playbackMarkerEdit_oldWndproc = (WNDPROC)SetWindowLong(hwndPlaybackMarkerEdit, GWL_WNDPROC, (LONG)UpperMarkerEditWndProc);

	reset();
}
void PLAYBACK::reset()
{
	must_find_current_marker = true;
	shown_marker = 0;
	lastCursor = -1;
	pause_frame = old_pauseframe = 0;
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
	jump_was_used_this_frame = false;
	// pause when seeking hit pause_frame
	if(!FCEUI_EmulationPaused())
		if(pause_frame && pause_frame <= currFrameCounter + 1)
			SeekingStop();

	// update flashing pauseframe
	if (old_pauseframe != pause_frame && old_pauseframe)
	{
		// pause_frame was changed, clear old_pauseframe gfx
		tasedit_list.RedrawRow(old_pauseframe-1);
		bookmarks.RedrawChangedBookmarks(old_pauseframe-1);
	}
	old_pauseframe = pause_frame;
	old_show_pauseframe = show_pauseframe;
	if (pause_frame)
	{
		if (emu_paused)
			show_pauseframe = (int)(clock() / PAUSEFRAME_BLINKING_PERIOD_PAUSED) & 1;
		else
			show_pauseframe = (int)(clock() / PAUSEFRAME_BLINKING_PERIOD_SEEKING) & 1;
	} else show_pauseframe = false;
	if (old_show_pauseframe != show_pauseframe)
	{
		// update pauseframe gfx
		tasedit_list.RedrawRow(pause_frame-1);
		bookmarks.RedrawChangedBookmarks(pause_frame-1);
	}

	// update seeking progressbar
	old_emu_paused = emu_paused;
	emu_paused = (FCEUI_EmulationPaused() != 0);
	if (pause_frame)
	{
		if (old_show_pauseframe != show_pauseframe)
			SetProgressbar(currFrameCounter - seeking_start_frame, pause_frame - seeking_start_frame);
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

	// update the playback cursor
	if(currFrameCounter != lastCursor)
	{
		tasedit_list.FollowPlaybackIfNeeded();
		// update gfx of the old and new rows
		tasedit_list.RedrawRow(lastCursor);
		bookmarks.RedrawChangedBookmarks(lastCursor);
		tasedit_list.RedrawRow(currFrameCounter);
		bookmarks.RedrawChangedBookmarks(currFrameCounter);
		// enforce redrawing now
		lastCursor = currFrameCounter;
		UpdateWindow(tasedit_list.hwndList);
		// lazy update of "Playback's Marker text"
		int current_marker = current_markers.GetMarkerUp(currFrameCounter);
		if (shown_marker != current_marker)
		{
			UpdateMarkerNote();
			shown_marker = current_marker;
			RedrawMarker();
			must_find_current_marker = false;
		}
	}

	// [non-lazy] update "Playback's Marker text" if needed
	if (must_find_current_marker)
	{
		UpdateMarkerNote();
		shown_marker = current_markers.GetMarkerUp(currFrameCounter);
		RedrawMarker();
		must_find_current_marker = false;
	}
	
	// update < and > buttons
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
	if (pause_frame)
	{
		SetProgressbar(currFrameCounter - seeking_start_frame, pause_frame - seeking_start_frame);
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
	pause_frame = finish_frame;
	if (TASEdit_turbo_seek)
		turbo = true;
	UnpauseEmulation();
}
void PLAYBACK::SeekingStop()
{
	pause_frame = 0;
	turbo = false;
	PauseEmulation();
	SetProgressbar(1, 1);
}

void PLAYBACK::RewindFrame()
{
	if (pause_frame && !emu_paused) return;
	if (currFrameCounter > 0)
		jump(currFrameCounter-1);
	else
		tasedit_list.FollowPlaybackIfNeeded();
	if (!pause_frame) PauseEmulation();
}
void PLAYBACK::ForwardFrame()
{
	if (pause_frame && !emu_paused) return;
	jump(currFrameCounter+1);
	if (!pause_frame) PauseEmulation();
	turbo = false;
}
void PLAYBACK::RewindFull()
{
	// jump to previous marker
	int index = currFrameCounter - 1;
	for (; index >= 0; index--)
		if (current_markers.GetMarker(index)) break;
	if (index >= 0)
		jump(index);
	else
		jump(0);
	jump_was_used_this_frame = true;
}
void PLAYBACK::ForwardFull()
{
	// jump to next marker
	int last_frame = currMovieData.getNumRecords()-1;
	int index = currFrameCounter + 1;
	for (; index <= last_frame; ++index)
		if (current_markers.GetMarker(index)) break;
	if (index <= last_frame)
		jump(index);
	else
		jump(last_frame);
}

void PLAYBACK::RedrawMarker()
{
	// redraw marker num
	char new_text[MAX_NOTE_LEN] = {0};
	if (shown_marker <= 99999)		// if there's too many digits in the number then don't show the word "Marker" before the number
		strcpy(new_text, upperMarkerText);
	char num[11];
	_itoa(shown_marker, num, 10);
	strcat(new_text, num);
	SetWindowText(hwndPlaybackMarker, new_text);
	// change marker note
	strcpy(new_text, current_markers.GetNote(shown_marker).c_str());
	SetWindowText(hwndPlaybackMarkerEdit, new_text);
	// reset search_similar_marker
	search_similar_marker = 0;
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
		tasedit_list.FollowPlaybackIfNeeded();
	}
}
void PLAYBACK::restorePosition()
{
	if (pause_frame)
		jump(pause_frame-1);
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
		if (pause_frame) SeekingStop();
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
		return pause_frame;
	else
		return 0;
}

void PLAYBACK::SetProgressbar(int a, int b)
{
	SendMessage(hwndProgressbar, PBM_SETPOS, PROGRESSBAR_WIDTH * a / b, 0);
}
// -------------------------------------------------------------------------
LRESULT APIENTRY UpperMarkerEditWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (marker_note_edit == MARKER_NOTE_EDIT_UPPER)
	{
		extern PLAYBACK playback;
		extern TASEDIT_SELECTION selection;
		switch(msg)
		{
		case WM_CHAR:
		case WM_KEYDOWN:
			switch(wParam)
			{
			case VK_ESCAPE:
				// revert text to original note text
				SetWindowText(playback.hwndPlaybackMarkerEdit, current_markers.GetNote(playback.shown_marker).c_str());
				SetFocus(tasedit_list.hwndList);
				return 0;
			case VK_RETURN:
				// exit and save text changes
				SetFocus(tasedit_list.hwndList);
				return 0;
			case VK_TAB:
				// switch to lower edit control (also exit and save text changes)
				SetFocus(selection.hwndSelectionMarkerEdit);
				return 0;
			}
			break;
		}
	}
	return CallWindowProc(playbackMarkerEdit_oldWndproc, hWnd, msg, wParam, lParam);
}






