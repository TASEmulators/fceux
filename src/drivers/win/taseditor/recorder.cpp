/* ---------------------------------------------------------------------------------
Implementation file of RECORDER class
Copyright (c) 2011-2012 AnS

(The MIT License)
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------------
Recorder - Tool for Input recording
[Singleton]

* at the moment of recording movie Input (at the very end of a frame) by emulator's call the Recorder intercepts Input data and applies its filters (multitracking/etc), then reflects Input changes into History and Greenzone
* regularly tracks virtual joypad buttonpresses and provides data for Piano Roll List Header lights. Also reacts on external changes of Recording status, and updates GUI (Recorder panel and Bookmarks/Branches caption)
* implements Input editing in Read-only mode (ColumnSet by pressing buttons on virtual joypad)
* stores resources: ids and names of multitracking modes, suffixes for TAS Editor window caption
------------------------------------------------------------------------------------ */

#include "taseditor_project.h"

extern int joysticks_per_frame[NUM_SUPPORTED_INPUT_TYPES];

extern uint32 GetGamepadPressedImmediate();
extern int GetInputType(MovieData& md);

extern char lagFlag;

extern TASEDITOR_CONFIG taseditor_config;
extern TASEDITOR_WINDOW taseditor_window;
extern BOOKMARKS bookmarks;
extern HISTORY history;
extern GREENZONE greenzone;
extern PIANO_ROLL piano_roll;
extern EDITOR editor;

// resources
const char recordingCheckbox[10] = "Recording";
const char recordingCheckboxBlankPattern[16] = "Recording blank";

const char recordingModes[5][4] = {	"All",
								"1P",
								"2P",
								"3P",
								"4P"};
const char recordingCaptions[5][17] = {	" (Recording All)",
									" (Recording 1P)",
									" (Recording 2P)",
									" (Recording 3P)",
									" (Recording 4P)"};
RECORDER::RECORDER()
{
}

void RECORDER::init()
{
	hwndRecCheckbox = GetDlgItem(taseditor_window.hwndTasEditor, IDC_RECORDING);
	hwndRB_RecAll = GetDlgItem(taseditor_window.hwndTasEditor, IDC_RADIO_ALL);
	hwndRB_Rec1P = GetDlgItem(taseditor_window.hwndTasEditor, IDC_RADIO_1P);
	hwndRB_Rec2P = GetDlgItem(taseditor_window.hwndTasEditor, IDC_RADIO_2P);
	hwndRB_Rec3P = GetDlgItem(taseditor_window.hwndTasEditor, IDC_RADIO_3P);
	hwndRB_Rec4P = GetDlgItem(taseditor_window.hwndTasEditor, IDC_RADIO_4P);
	old_multitrack_recording_joypad = multitrack_recording_joypad;
	old_current_pattern = old_pattern_offset = 0;
	must_increase_pattern_offset = false;
	old_movie_readonly = movie_readonly;
	old_joy.resize(MAX_NUM_JOYPADS);
	new_joy.resize(MAX_NUM_JOYPADS);
	current_joy.resize(MAX_NUM_JOYPADS);
}
void RECORDER::reset()
{
	movie_readonly = true;
	state_was_loaded_in_readwrite_mode = false;
	multitrack_recording_joypad = MULTITRACK_RECORDING_ALL;
	pattern_offset = 0;
	must_increase_pattern_offset = false;
	UncheckRecordingRadioButtons();
	RecheckRecordingRadioButtons();
	switch (GetInputType(currMovieData))
	{
		case INPUT_TYPE_FOURSCORE:
		{
			// enable all 4 radiobuttons
			EnableWindow(hwndRB_Rec1P, true);
			EnableWindow(hwndRB_Rec2P, true);
			EnableWindow(hwndRB_Rec3P, true);
			EnableWindow(hwndRB_Rec4P, true);
			break;
		}
		case INPUT_TYPE_2P:
		{
			// enable radiobuttons 1 and 2
			EnableWindow(hwndRB_Rec1P, true);
			EnableWindow(hwndRB_Rec2P, true);
			// disable radiobuttons 3 and 4
			EnableWindow(hwndRB_Rec3P, false);
			EnableWindow(hwndRB_Rec4P, false);
			break;
		}
		case INPUT_TYPE_1P:
		{
			// enable radiobutton 1
			EnableWindow(hwndRB_Rec1P, true);
			// disable radiobuttons 2, 3 and 4
			EnableWindow(hwndRB_Rec2P, false);
			EnableWindow(hwndRB_Rec3P, false);
			EnableWindow(hwndRB_Rec4P, false);
			break;
		}
	}
}
void RECORDER::update()
{
	// update window caption if needed
	if (old_movie_readonly != movie_readonly || old_multitrack_recording_joypad != multitrack_recording_joypad)
		taseditor_window.UpdateCaption();
	// update Bookmarks/Branches groupbox caption if needed
	if (taseditor_config.old_branching_controls && old_movie_readonly != movie_readonly)
		bookmarks.RedrawBookmarksCaption();
	// update "Recording" checkbox state
	if (old_movie_readonly != movie_readonly)
	{
		Button_SetCheck(hwndRecCheckbox, movie_readonly?BST_UNCHECKED : BST_CHECKED);
		old_movie_readonly = movie_readonly;
		if (movie_readonly)
			state_was_loaded_in_readwrite_mode = false;
	}
	// reset pattern_offset if current_pattern has changed
	if (old_current_pattern != taseditor_config.current_pattern)
		pattern_offset = 0;
	// increase pattern_offset if needed
	if (must_increase_pattern_offset)
	{
		must_increase_pattern_offset = false;
		if (!taseditor_config.pattern_skips_lag || lagFlag == 0)
		{
			pattern_offset++;
			if (pattern_offset >= (int)editor.autofire_patterns[old_current_pattern].size())
				pattern_offset -= editor.autofire_patterns[old_current_pattern].size();
		}
	}
	// update "Recording" checkbox text if something changed in pattern
	if (old_current_pattern != taseditor_config.current_pattern || old_pattern_offset != pattern_offset)
	{
		old_current_pattern = taseditor_config.current_pattern;
		old_pattern_offset = pattern_offset;
		if (!taseditor_config.pattern_recording || editor.autofire_patterns[old_current_pattern][pattern_offset])
			// either not using Patterns or current pattern has 1 in current offset
			SetWindowText(hwndRecCheckbox, recordingCheckbox);
		else
			// current pattern has 0 in current offset, this means next recorded frame will be blank
			SetWindowText(hwndRecCheckbox, recordingCheckboxBlankPattern);
	}
	// update recording radio buttons if user changed multitrack_recording_joypad
	if (old_multitrack_recording_joypad != multitrack_recording_joypad)
	{
		UncheckRecordingRadioButtons();
		RecheckRecordingRadioButtons();
	}

	int num_joys = joysticks_per_frame[GetInputType(currMovieData)];
	// save previous state
	old_joy[0] = current_joy[0];
	old_joy[1] = current_joy[1];
	old_joy[2] = current_joy[2];
	old_joy[3] = current_joy[3];
	// fill current_joy data for Piano Roll header lights
	uint32 joypads = GetGamepadPressedImmediate();
	current_joy[0] = (joypads & 0xFF);
	current_joy[1] = ((joypads >> 8) & 0xFF);
	current_joy[2] = ((joypads >> 16) & 0xFF);
	current_joy[3] = ((joypads >> 24) & 0xFF);
	// filter out joysticks that should not be recorded (according to multitrack_recording_joypad)
	if (multitrack_recording_joypad != MULTITRACK_RECORDING_ALL)
	{
		int joy = multitrack_recording_joypad - 1;
		// substitute target joypad with 1p joypad
		if (multitrack_recording_joypad > MULTITRACK_RECORDING_1P && taseditor_config.use_1p_rec)
			current_joy[joy] = current_joy[0];
		// clear all other joypads (pressing them does not count)
		for (int i = 0; i < num_joys; ++i)
			if (i != joy)
				current_joy[i] = 0;
	}
	// call ColumnSet if needed
	if (taseditor_config.columnset_by_keys && movie_readonly && taseditor_window.TASEditor_focus)
	{
		// if Ctrl or Shift is held, do not call ColumnSet, because maybe this is accelerator
		if ((GetAsyncKeyState(VK_CONTROL) >= 0) && (GetAsyncKeyState(VK_SHIFT) >= 0))
		{
			bool alt_pressed = ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0);
			for (int joy = 0; joy < num_joys; ++joy)
			{
				for (int button = 0; button < NUM_JOYPAD_BUTTONS; ++button)
				{
					// if the button was pressed right now
					if ((current_joy[joy] & (1 << button)) && !(old_joy[joy] & (1 << button)))
						piano_roll.ColumnSet(COLUMN_JOYPAD1_A + joy * NUM_JOYPAD_BUTTONS + button, alt_pressed);
				}
			}
		}
	}
}
// ------------------------------------------------------------------------------------
void RECORDER::UncheckRecordingRadioButtons()
{
	Button_SetCheck(hwndRB_RecAll, BST_UNCHECKED);
	Button_SetCheck(hwndRB_Rec1P, BST_UNCHECKED);
	Button_SetCheck(hwndRB_Rec2P, BST_UNCHECKED);
	Button_SetCheck(hwndRB_Rec3P, BST_UNCHECKED);
	Button_SetCheck(hwndRB_Rec4P, BST_UNCHECKED);
}
void RECORDER::RecheckRecordingRadioButtons()
{
	old_multitrack_recording_joypad = multitrack_recording_joypad;
	switch(multitrack_recording_joypad)
	{
	case MULTITRACK_RECORDING_ALL:
		Button_SetCheck(hwndRB_RecAll, BST_CHECKED);
		break;
	case MULTITRACK_RECORDING_1P:
		Button_SetCheck(hwndRB_Rec1P, BST_CHECKED);
		break;
	case MULTITRACK_RECORDING_2P:
		Button_SetCheck(hwndRB_Rec2P, BST_CHECKED);
		break;
	case MULTITRACK_RECORDING_3P:
		Button_SetCheck(hwndRB_Rec3P, BST_CHECKED);
		break;
	case MULTITRACK_RECORDING_4P:
		Button_SetCheck(hwndRB_Rec4P, BST_CHECKED);
		break;
	default:
		multitrack_recording_joypad = MULTITRACK_RECORDING_ALL;
		Button_SetCheck(hwndRB_RecAll, BST_CHECKED);
		break;
	}
}

void RECORDER::InputChanged()
{
	bool changes_made = false;
	int num_joys = joysticks_per_frame[GetInputType(currMovieData)];
	// take previous values from current snapshot, new Input from current movie
	for (int i = 0; i < num_joys; ++i)
	{
		old_joy[i] = history.GetCurrentSnapshot().inputlog.GetJoystickInfo(currFrameCounter, i);
		if (!taseditor_config.pattern_recording || editor.autofire_patterns[old_current_pattern][pattern_offset])
			new_joy[i] = currMovieData.records[currFrameCounter].joysticks[i];
		else
			new_joy[i] = 0;		// blank
	}
	if (taseditor_config.pattern_recording)
		// postpone incrementing pattern_offset to the end of the frame (when lagFlag will be known)
		must_increase_pattern_offset = true;
	// combine old and new data (superimpose) and filter out joystics that should not be recorded
	if (multitrack_recording_joypad == MULTITRACK_RECORDING_ALL)
	{
		for (int i = num_joys-1; i >= 0; i--)
		{
			// superimpose (bitwise OR) if needed
			if (taseditor_config.superimpose == SUPERIMPOSE_CHECKED || (taseditor_config.superimpose == SUPERIMPOSE_INDETERMINATE && new_joy[i] == 0))
				new_joy[i] |= old_joy[i];
			// change this joystick
			currMovieData.records[currFrameCounter].joysticks[i] = new_joy[i];
			if (new_joy[i] != old_joy[i])
			{
				changes_made = true;
				// set lights for changed buttons
				for (int button = 0; button < NUM_JOYPAD_BUTTONS; ++button)
					if ((new_joy[i] & (1 << button)) && !(old_joy[i] & (1 << button)))
						piano_roll.SetHeaderColumnLight(COLUMN_JOYPAD1_A + i * NUM_JOYPAD_BUTTONS + button, HEADER_LIGHT_MAX);
			}
		}
	} else
	{
		int joy = multitrack_recording_joypad - 1;
		// substitute target joypad with 1p joypad
		if (multitrack_recording_joypad > MULTITRACK_RECORDING_1P && taseditor_config.use_1p_rec)
			new_joy[joy] = new_joy[0];
		// superimpose (bitwise OR) if needed
		if (taseditor_config.superimpose == SUPERIMPOSE_CHECKED || (taseditor_config.superimpose == SUPERIMPOSE_INDETERMINATE && new_joy[joy] == 0))
			new_joy[joy] |= old_joy[joy];
		// other joysticks should not be changed
		for (int i = num_joys-1; i >= 0; i--)
			currMovieData.records[currFrameCounter].joysticks[i] = old_joy[i];	// revert to old
		// change only this joystick
		currMovieData.records[currFrameCounter].joysticks[joy] = new_joy[joy];
		if (new_joy[joy] != old_joy[joy])
		{
			changes_made = true;
			// set lights for changed buttons
			for (int button = 0; button < NUM_JOYPAD_BUTTONS; ++button)
				if ((new_joy[joy] & (1 << button)) && !(old_joy[joy] & (1 << button)))
					piano_roll.SetHeaderColumnLight(COLUMN_JOYPAD1_A + joy * NUM_JOYPAD_BUTTONS + button, HEADER_LIGHT_MAX);
		}
	}

	if (!changes_made)
	{
		// check if new commands were recorded
		if (currMovieData.records[currFrameCounter].commands != history.GetCurrentSnapshot().inputlog.GetCommandsInfo(currFrameCounter))
			changes_made = true;
	}

	// register changes
	if (changes_made)
	{
		history.RegisterRecording(currFrameCounter);
		greenzone.Invalidate(currFrameCounter);
	}
}

// getters
const char* RECORDER::GetRecordingMode()
{
	return recordingModes[multitrack_recording_joypad];
}
const char* RECORDER::GetRecordingCaption()
{
	return recordingCaptions[multitrack_recording_joypad];
}

