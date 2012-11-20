/* ---------------------------------------------------------------------------------
Implementation file of Playback class
Copyright (c) 2011-2012 AnS

(The MIT License)
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------------
Playback - Player of emulation states
[Singleton]

* implements the working of movie player: show any frame (jump), run/cancel seekng. pause, rewinding
* regularly tracks and controls emulation process, prompts redrawing of Piano Roll List rows, finishes seeking when reaching target frame, animates target frame, makes Piano Roll follow Playback cursor, detects if Playback cursor moved to another Marker and updates Note in the upper text field
* implements the working of upper buttons << and >> (jumping on Markers)
* implements the working of buttons < and > (frame-by-frame movement)
* implements the working of button || (pause) and middle mouse button, also reacts on external changes of emulation pause
* implements the working of progressbar: init, reset, set value, click (cancel seeking)
* also here's the code of upper text field (for editing Marker Notes)
* stores resources: upper text field prefix, timings of target frame animation, response times of GUI buttons, progressbar scale
------------------------------------------------------------------------------------ */

#include "taseditor_project.h"
#include "../taseditor.h"

#ifdef _S9XLUA_H
extern void ForceExecuteLuaFrameFunctions();
#endif

extern bool Taseditor_rewind_now;
extern bool turbo;

extern TASEDITOR_CONFIG taseditor_config;
extern TASEDITOR_WINDOW taseditor_window;
extern SELECTION selection;
extern MARKERS_MANAGER markers_manager;
extern GREENZONE greenzone;
extern PIANO_ROLL piano_roll;
extern BOOKMARKS bookmarks;

extern void Update_RAM_Search();

LRESULT APIENTRY UpperMarkerEditWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
WNDPROC playbackMarkerEdit_oldWndproc;

// resources
char upperMarkerText[] = "Marker ";

PLAYBACK::PLAYBACK()
{
}

void PLAYBACK::init()
{
	hwndProgressbar = GetDlgItem(taseditor_window.hwndTasEditor, IDC_PROGRESS1);
	SendMessage(hwndProgressbar, PBM_SETRANGE, 0, MAKELPARAM(0, PROGRESSBAR_WIDTH)); 
	hwndRewind = GetDlgItem(taseditor_window.hwndTasEditor, TASEDITOR_REWIND);
	hwndForward = GetDlgItem(taseditor_window.hwndTasEditor, TASEDITOR_FORWARD);
	hwndRewindFull = GetDlgItem(taseditor_window.hwndTasEditor, TASEDITOR_REWIND_FULL);
	hwndForwardFull = GetDlgItem(taseditor_window.hwndTasEditor, TASEDITOR_FORWARD_FULL);
	hwndPlaybackMarker = GetDlgItem(taseditor_window.hwndTasEditor, IDC_PLAYBACK_MARKER);
	SendMessage(hwndPlaybackMarker, WM_SETFONT, (WPARAM)piano_roll.hMarkersFont, 0);
	hwndPlaybackMarkerEdit = GetDlgItem(taseditor_window.hwndTasEditor, IDC_PLAYBACK_MARKER_EDIT);
	SendMessage(hwndPlaybackMarkerEdit, EM_SETLIMITTEXT, MAX_NOTE_LEN - 1, 0);
	SendMessage(hwndPlaybackMarkerEdit, WM_SETFONT, (WPARAM)piano_roll.hMarkersEditFont, 0);
	// subclass the edit control
	playbackMarkerEdit_oldWndproc = (WNDPROC)SetWindowLong(hwndPlaybackMarkerEdit, GWL_WNDPROC, (LONG)UpperMarkerEditWndProc);

	reset();
}
void PLAYBACK::reset()
{
	must_autopause_at_the_end = true;
	must_find_current_marker = true;
	shown_marker = 0;
	lastCursor = currFrameCounter;
	lost_position_frame = pause_frame = old_pauseframe = 0;
	lost_position_is_stable = old_show_pauseframe = show_pauseframe = false;
	old_rewind_button_state = rewind_button_state = false;
	old_forward_button_state = forward_button_state = false;
	old_rewind_full_button_state = rewind_full_button_state = false;
	old_forward_full_button_state = forward_full_button_state = false;
	old_emu_paused = emu_paused = true;
	SeekingStop();
}
void PLAYBACK::update()
{
	// update < and > buttons
	old_rewind_button_state = rewind_button_state;
	rewind_button_state = ((Button_GetState(hwndRewind) & BST_PUSHED) != 0 || Taseditor_rewind_now);
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

	// pause when seeking hits pause_frame
	if (pause_frame && currFrameCounter + 1 >= pause_frame)
		SeekingStop();
	else if (currFrameCounter >= GetLostPosition() && currFrameCounter >= currMovieData.getNumRecords() - 1 && must_autopause_at_the_end && taseditor_config.autopause_at_finish)
		// pause at the end of the movie
		PauseEmulation();

	// update flashing pauseframe
	if (old_pauseframe != pause_frame && old_pauseframe)
	{
		// pause_frame was changed, clear old_pauseframe gfx
		piano_roll.RedrawRow(old_pauseframe-1);
		bookmarks.RedrawChangedBookmarks(old_pauseframe-1);
	}
	old_pauseframe = pause_frame;
	old_show_pauseframe = show_pauseframe;
	if (pause_frame)
	{
		if (emu_paused)
			show_pauseframe = (int)(clock() / PAUSEFRAME_BLINKING_PERIOD_WHEN_PAUSED) & 1;
		else
			show_pauseframe = (int)(clock() / PAUSEFRAME_BLINKING_PERIOD_WHEN_SEEKING) & 1;
	} else show_pauseframe = false;
	if (old_show_pauseframe != show_pauseframe)
	{
		// update pauseframe gfx
		piano_roll.RedrawRow(pause_frame - 1);
		bookmarks.RedrawChangedBookmarks(pause_frame - 1);
	}

	// update seeking progressbar
	old_emu_paused = emu_paused;
	emu_paused = (FCEUI_EmulationPaused() != 0);
	if (pause_frame)
	{
		if (old_show_pauseframe != show_pauseframe)		// update progressbar from time to time
			// display seeking progress
			SetProgressbar(currFrameCounter - seeking_start_frame, pause_frame - seeking_start_frame);
	} else if (old_emu_paused != emu_paused)
	{
		// emulator got paused/unpaused externally
		if (old_emu_paused && !emu_paused)
		{
			// externally unpaused - show empty progressbar
			SetProgressbar(0, 1);
		} else
		{
			// externally paused - progressbar should be full
			SetProgressbar(1, 1);
		}
	}

	// prepare to stop at the end of the movie if user unpauses emulator
	if (emu_paused)
	{
		if (currFrameCounter < currMovieData.getNumRecords() - 1)
			must_autopause_at_the_end = true;
		else
			must_autopause_at_the_end = false;
	}

	// update the Playback cursor
	if (currFrameCounter != lastCursor)
	{
		// update gfx of the old and new rows
		piano_roll.RedrawRow(lastCursor);
		bookmarks.RedrawChangedBookmarks(lastCursor);
		piano_roll.RedrawRow(currFrameCounter);
		bookmarks.RedrawChangedBookmarks(currFrameCounter);
		lastCursor = currFrameCounter;
		piano_roll.FollowPlaybackIfNeeded();
		// enforce redrawing now
		UpdateWindow(piano_roll.hwndList);
		// lazy update of "Playback's Marker text"
		int current_marker = markers_manager.GetMarkerUp(currFrameCounter);
		if (shown_marker != current_marker)
		{
			markers_manager.UpdateMarkerNote();
			shown_marker = current_marker;
			RedrawMarker();
			must_find_current_marker = false;
		}
	}

	// [non-lazy] update "Playback's Marker text" if needed
	if (must_find_current_marker)
	{
		markers_manager.UpdateMarkerNote();
		shown_marker = markers_manager.GetMarkerUp(currFrameCounter);
		RedrawMarker();
		must_find_current_marker = false;
	}

	// This logic is very important for adequate "green arrow" and "Restore position"
	if (!emu_paused)
		// when emulating, lost_position_frame becomes unstable
		lost_position_is_stable = false;
}

// called after saving the project, because saving uses the progressbar for itself
void PLAYBACK::UpdateProgressbar()
{
	if (pause_frame)
	{
		SetProgressbar(currFrameCounter - seeking_start_frame, pause_frame - seeking_start_frame);
	} else
	{
		if (emu_paused)
			// full progressbar
			SetProgressbar(1, 1);
		else
			// cleared progressbar
			SetProgressbar(0, 1);
	}
	RedrawWindow(hwndProgressbar, NULL, NULL, RDW_INVALIDATE);
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
}
void PLAYBACK::UnpauseEmulation()
{
	FCEUI_SetEmulationPaused(0);
}
void PLAYBACK::RestorePosition()
{
	if (GetLostPosition() > currFrameCounter)
	{
		if (emu_paused)
			SeekingStart(GetLostPosition());
		else
			PauseEmulation();
	}
}
void PLAYBACK::MiddleButtonClick()
{
	if (emu_paused)
	{
		// Unpause or start seeking
		// works only when right mouse button is released
		if (GetAsyncKeyState(GetSystemMetrics(SM_SWAPBUTTON) ? VK_LBUTTON : VK_RBUTTON) >= 0)
		{
			if (GetAsyncKeyState(VK_SHIFT) < 0)
			{
				// if Shift is held, seek to nearest Marker
				int last_frame = markers_manager.GetMarkersSize() - 1;	// the end of movie Markers
				int target_frame = currFrameCounter + 1;
				for (; target_frame <= last_frame; ++target_frame)
					if (markers_manager.GetMarker(target_frame)) break;
				if (target_frame <= last_frame)
					SeekingStart(target_frame);
			} else if (GetAsyncKeyState(VK_CONTROL) < 0)
			{
				// if Ctrl is held, seek to Selection cursor or replay from Selection cursor
				int selection_beginning = selection.GetCurrentSelectionBeginning();
				if (selection_beginning > currFrameCounter)
				{
					SeekingStart(selection_beginning);
				} else if (selection_beginning < currFrameCounter)
				{
					int saved_currFrameCounter = currFrameCounter;
					if (selection_beginning < 0)
						selection_beginning = 0;
					jump(selection_beginning);
					SeekingStart(saved_currFrameCounter);
				}
			} else if (GetLostPosition() >= greenzone.GetSize())
			{
				RestorePosition();
			} else
			{
				UnpauseEmulation();
			}
		}
	} else
	{
		PauseEmulation();
	}
}

void PLAYBACK::SeekingStart(int finish_frame)
{
	if ((pause_frame - 1) != finish_frame)
	{
		seeking_start_frame = currFrameCounter;
		pause_frame = finish_frame + 1;
	}
	if (taseditor_config.turbo_seek)
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
		jump(currFrameCounter - 1);
	else
		// cursor is at frame 0 - can't rewind, but still must make cursor visible if needed
		piano_roll.FollowPlaybackIfNeeded();
	if (!pause_frame) PauseEmulation();
}
void PLAYBACK::ForwardFrame()
{
	if (pause_frame && !emu_paused) return;
	jump(currFrameCounter + 1);
	if (!pause_frame) PauseEmulation();
	turbo = false;
}
void PLAYBACK::RewindFull(int speed)
{
	int index = currFrameCounter - 1;
	// jump trough "speed" amount of previous Markers
	while (speed > 0)
	{
		for (; index >= 0; index--)
			if (markers_manager.GetMarker(index)) break;
		speed--;
	}
	int lastCursor = currFrameCounter;
	if (index >= 0)
		jump(index);						// jump to the Marker
	else
		jump(0);							// jump to the beginning of Piano Roll
	if (lastCursor != currFrameCounter)
	{
		// redraw row where Playback cursor was (in case there's two or more RewindFulls before playback.update())
		piano_roll.RedrawRow(lastCursor);
		bookmarks.RedrawChangedBookmarks(lastCursor);
	}
}
void PLAYBACK::ForwardFull(int speed)
{
	int last_frame = markers_manager.GetMarkersSize() - 1;	// the end of movie Markers
	int index = currFrameCounter + 1;
	// jump trough "speed" amount of next Markers
	while (speed > 0)
	{
		for (; index <= last_frame; ++index)
			if (markers_manager.GetMarker(index)) break;
		speed--;
	}
	int lastCursor = currFrameCounter;
	if (index <= last_frame)
		jump(index);								// jump to Marker
	else
		jump(currMovieData.getNumRecords() - 1);	// jump to the end of Piano Roll
	if (lastCursor != currFrameCounter)
	{
		// redraw row where Playback cursor was (in case there's two or more ForwardFulls before playback.update())
		piano_roll.RedrawRow(lastCursor);
		bookmarks.RedrawChangedBookmarks(lastCursor);
	}
}

void PLAYBACK::RedrawMarker()
{
	// redraw Marker num
	char new_text[MAX_NOTE_LEN] = {0};
	if (shown_marker <= 9999)		// if there's too many digits in the number then don't show the word "Marker" before the number
		strcpy(new_text, upperMarkerText);
	char num[11];
	_itoa(shown_marker, num, 10);
	strcat(new_text, num);
	strcat(new_text, " ");
	SetWindowText(hwndPlaybackMarker, new_text);
	// change Marker Note
	strcpy(new_text, markers_manager.GetNote(shown_marker).c_str());
	SetWindowText(hwndPlaybackMarkerEdit, new_text);
	// reset search_similar_marker, because source Marker changed
	markers_manager.search_similar_marker = 0;
}

void PLAYBACK::StartFromZero()
{
	poweron(true);
	FCEUMOV_ClearCommands();		// clear POWER SWITCH command caused by poweron()
	currFrameCounter = 0;
	// if there's no frames in current movie, create initial frame record
	if (currMovieData.getNumRecords() == 0)
		currMovieData.insertEmpty(-1, 1);
}

void PLAYBACK::EnsurePlaybackIsInsideGreenzone(bool execute_lua, bool follow_cursor)
{
	// set the Playback cursor to the frame or at least above the frame
	if (SetPlaybackAboveOrToFrame(greenzone.GetSize() - 1))
	{
		// since the game state was changed by this jump, we must update possible Lua callbacks and other tools that would normally only update in FCEUI_Emulate
		if (execute_lua)
			ForceExecuteLuaFrameFunctions();
		if (follow_cursor)
			piano_roll.FollowPlaybackIfNeeded();
		Update_RAM_Search(); // Update_RAM_Watch() is also called.
	}
}

// an interface for sending Playback cursor to any frame
void PLAYBACK::jump(int frame, bool force_reload, bool execute_lua, bool follow_cursor)
{
	if (frame < 0) return;

	// 1 - set the Playback cursor to the frame or at least above the frame
	if (SetPlaybackAboveOrToFrame(frame, force_reload))
	{
		// since the game state was changed by this jump, we must update possible Lua callbacks and other tools that would normally only update in FCEUI_Emulate
		if (execute_lua)
			ForceExecuteLuaFrameFunctions();
		if (follow_cursor)
			piano_roll.FollowPlaybackIfNeeded();
		Update_RAM_Search(); // Update_RAM_Watch() is also called.
	}

	// 2 - seek from the current frame if we still aren't at the needed frame
	if (frame > currFrameCounter)
	{
		SeekingStart(frame);
	} else
	{
		// the Playback is already at the needed frame
		if (pause_frame)	// if Playback was seeking, pause emulation right here
			SeekingStop();
	}
}

// returns true if the game state was changed (loaded)
bool PLAYBACK::SetPlaybackAboveOrToFrame(int frame, bool force_reload)
{
	bool state_changed = false;
	// search backwards for an earlier frame with valid savestate
	int i = greenzone.GetSize() - 1;
	if (i > frame)
		i = frame;
	for (; i >= 0; i--)
	{
		if (!force_reload && !state_changed && i == currFrameCounter)
		{
			// we can remain at current game state
			break;
		} else if (!greenzone.SavestateIsEmpty(i))
		{
			state_changed = true;	// after we once tried loading a savestate, we cannot use currFrameCounter state anymore, because the game state might have been corrupted by this loading attempt
			if (greenzone.LoadSavestate(i))
				break;
		}
	}
	if (i < 0)
	{
		// couldn't find a savestate
		StartFromZero();
		state_changed = true;
	}
	return state_changed;
}

void PLAYBACK::SetLostPosition(int frame)
{
	if ((lost_position_frame - 1 < frame) || (lost_position_frame - 1 >= frame && !lost_position_is_stable))
	{
		if (lost_position_frame)
			piano_roll.RedrawRow(lost_position_frame - 1);
		lost_position_frame = frame + 1;
		lost_position_is_stable = true;
	}
}
int PLAYBACK::GetLostPosition()
{
	return lost_position_frame - 1;
}

int PLAYBACK::GetPauseFrame()
{
	return pause_frame - 1;
}
int PLAYBACK::GetFlashingPauseFrame()
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
void PLAYBACK::CancelSeeking()
{
	if (pause_frame)
		SeekingStop();
}
// -------------------------------------------------------------------------
LRESULT APIENTRY UpperMarkerEditWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	extern PLAYBACK playback;
	extern SELECTION selection;
	switch(msg)
	{
		case WM_SETFOCUS:
		{
			markers_manager.marker_note_edit = MARKER_NOTE_EDIT_UPPER;
			// enable editing
			SendMessage(playback.hwndPlaybackMarkerEdit, EM_SETREADONLY, false, 0);
			// disable FCEUX keyboard
			ClearTaseditorInput();
			break;
		}
		case WM_KILLFOCUS:
		{
			// if we were editing, save and finish editing
			if (markers_manager.marker_note_edit == MARKER_NOTE_EDIT_UPPER)
			{
				markers_manager.UpdateMarkerNote();
				markers_manager.marker_note_edit = MARKER_NOTE_EDIT_NONE;
			}
			// disable editing (make the bg grayed)
			SendMessage(playback.hwndPlaybackMarkerEdit, EM_SETREADONLY, true, 0);
			// enable FCEUX keyboard
			if (taseditor_window.TASEditor_focus)
				SetTaseditorInput();
			break;
		}
		case WM_CHAR:
		case WM_KEYDOWN:
		{
			if (markers_manager.marker_note_edit == MARKER_NOTE_EDIT_UPPER)
			{
				switch(wParam)
				{
					case VK_ESCAPE:
						// revert text to original note text
						SetWindowText(playback.hwndPlaybackMarkerEdit, markers_manager.GetNote(playback.shown_marker).c_str());
						SetFocus(piano_roll.hwndList);
						return 0;
					case VK_RETURN:
						// exit and save text changes
						SetFocus(piano_roll.hwndList);
						return 0;
					case VK_TAB:
					{
						// switch to lower edit control (also exit and save text changes)
						SetFocus(selection.hwndSelectionMarkerEdit);
						// scroll to the Marker
						if (taseditor_config.follow_note_context)
							piano_roll.FollowMarker(selection.shown_marker);
						return 0;
					}
				}
			}
			break;
		}
		case WM_MBUTTONDOWN:
		case WM_MBUTTONDBLCLK:
		{
			playback.MiddleButtonClick();
			return 0;
		}
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		{
			// scroll to the Marker
			if (taseditor_config.follow_note_context)
				piano_roll.FollowMarker(playback.shown_marker);
			break;
		}
	}
	return CallWindowProc(playbackMarkerEdit_oldWndproc, hWnd, msg, wParam, lParam);
}

