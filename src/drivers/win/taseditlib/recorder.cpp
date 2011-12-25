//Implementation file of RECORDER class
#include "taseditproj.h"
#include "zlib.h"

extern HWND hwndTasEdit;

extern uint32 GetGamepadPressedImmediate();
extern void ColumnSet(int column);

extern BOOKMARKS bookmarks;
extern INPUT_HISTORY history;
extern GREENZONE greenzone;
extern TASEDIT_LIST tasedit_list;

extern void RedrawWindowCaption();
extern bool TASEdit_branch_only_when_rec;
extern bool TASEdit_use_1p_rec;
extern int TASEdit_superimpose;
extern bool TASEdit_columnset_by_keys;
extern bool TASEdit_focus;

// resources
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
	hwndRecCheckbox = GetDlgItem(hwndTasEdit, IDC_RECORDING);
	hwndRB_RecAll = GetDlgItem(hwndTasEdit, IDC_RADIO2);
	hwndRB_Rec1P = GetDlgItem(hwndTasEdit, IDC_RADIO3);
	hwndRB_Rec2P = GetDlgItem(hwndTasEdit, IDC_RADIO4);
	hwndRB_Rec3P = GetDlgItem(hwndTasEdit, IDC_RADIO5);
	hwndRB_Rec4P = GetDlgItem(hwndTasEdit, IDC_RADIO6);
	reset();
	old_multitrack_recording_joypad = multitrack_recording_joypad;
	old_movie_readonly = movie_readonly;
	old_joy.resize(4);
	new_joy.resize(4);
	current_joy.resize(4);
}
void RECORDER::reset()
{
	movie_readonly = true;
	multitrack_recording_joypad = MULTITRACK_RECORDING_ALL;
	UncheckRecordingRadioButtons();
	RecheckRecordingRadioButtons();
	if (currMovieData.fourscore)
	{
		// enable radiobuttons for 3P/4P multitracking
		EnableWindow(hwndRB_Rec3P, true);
		EnableWindow(hwndRB_Rec4P, true);
	} else
	{
		// disable radiobuttons for 3P/4P multitracking
		EnableWindow(hwndRB_Rec3P, false);
		EnableWindow(hwndRB_Rec4P, false);
	}
}
void RECORDER::update()
{
	// update window caption if needed
	if (old_movie_readonly != movie_readonly || old_multitrack_recording_joypad != multitrack_recording_joypad)
		RedrawWindowCaption();
	// update Bookmarks/Branches groupbox caption if needed
	if (TASEdit_branch_only_when_rec && old_movie_readonly != movie_readonly)
		bookmarks.RedrawBookmarksCaption();
	// update recording radio buttons if user used hotkey to switch R/W
	if (old_movie_readonly != movie_readonly || old_multitrack_recording_joypad != multitrack_recording_joypad)
	{
		UncheckRecordingRadioButtons();
		RecheckRecordingRadioButtons();
	}

	old_joy[0] = current_joy[0];
	old_joy[1] = current_joy[1];
	old_joy[2] = current_joy[2];
	old_joy[3] = current_joy[3];
	// fill current_joy data for listview header lights
	uint32 joypads = GetGamepadPressedImmediate();
	current_joy[0] = (joypads & 0xFF);
	current_joy[1] = ((joypads >> 8) & 0xFF);
	current_joy[2] = ((joypads >> 16) & 0xFF);
	current_joy[3] = ((joypads >> 24) & 0xFF);
	if (multitrack_recording_joypad != MULTITRACK_RECORDING_ALL)
	{
		int joy = multitrack_recording_joypad - 1;
		// substitute target joypad with 1p joypad
		if (multitrack_recording_joypad > MULTITRACK_RECORDING_1P && TASEdit_use_1p_rec)
			current_joy[joy] = current_joy[0];
		// clear all other joypads (pressing them does not count)
		for (int i = 0; i < NUM_JOYPADS; ++i)
			if (i != joy)
				current_joy[i] = 0;
	}
	// call ColumnSet if needed
	if (TASEdit_columnset_by_keys && movie_readonly && TASEdit_focus)
	{
		int num_joys;
		if (currMovieData.fourscore)
			num_joys = NUM_JOYPADS;
		else
			num_joys = 2;
		for (int joy = 0; joy < num_joys; ++joy)
		{
			for (int button = 0; button < NUM_JOYPAD_BUTTONS; ++button)
			{
				// if the button was pressed right now
				if ((current_joy[joy] & (1 << button)) && !(old_joy[joy] & (1 << button)))
					ColumnSet(COLUMN_JOYPAD1_A + joy * NUM_JOYPAD_BUTTONS + button);
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
	old_movie_readonly = movie_readonly;
	old_multitrack_recording_joypad = multitrack_recording_joypad;
	Button_SetCheck(hwndRecCheckbox, movie_readonly?BST_UNCHECKED : BST_CHECKED);
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
	}
}

void RECORDER::InputChanged()
{
	bool changes_made = false;
	// take previous values from current snapshot, new input from current movie
	old_joy[0] = history.GetCurrentSnapshot().GetJoystickInfo(currFrameCounter, 0);
	new_joy[0] = currMovieData.records[currFrameCounter].joysticks[0];
	old_joy[1] = history.GetCurrentSnapshot().GetJoystickInfo(currFrameCounter, 1);
	new_joy[1] = currMovieData.records[currFrameCounter].joysticks[1];
	if (currMovieData.fourscore)
	{
		old_joy[2] = history.GetCurrentSnapshot().GetJoystickInfo(currFrameCounter, 2);
		new_joy[2] = currMovieData.records[currFrameCounter].joysticks[2];
		old_joy[3] = history.GetCurrentSnapshot().GetJoystickInfo(currFrameCounter, 3);
		new_joy[3] = currMovieData.records[currFrameCounter].joysticks[3];
	}
	if (multitrack_recording_joypad == MULTITRACK_RECORDING_ALL)
	{
		int i;
		if (currMovieData.fourscore) i = 3; else i = 1;
		for (; i >= 0; i--)
		{
			// superimpose (bitwise OR) if needed
			if (TASEdit_superimpose == BST_CHECKED || (TASEdit_superimpose == BST_INDETERMINATE && new_joy[i] == 0))
				new_joy[i] |= old_joy[i];
			// change this joystick
			currMovieData.records[currFrameCounter].joysticks[i] = new_joy[i];
			if (new_joy[i] != old_joy[i])
			{
				changes_made = true;
				// set lights for changed buttons
				for (int button = 0; button < NUM_JOYPAD_BUTTONS; ++button)
					if ((new_joy[i] & (1 << button)) && !(old_joy[i] & (1 << button)))
						tasedit_list.SetHeaderColumnLight(COLUMN_JOYPAD1_A + i * NUM_JOYPAD_BUTTONS + button, HEADER_LIGHT_MAX);
			}
		}
	} else
	{
		int joy = multitrack_recording_joypad - 1;
		// substitute target joypad with 1p joypad
		if (multitrack_recording_joypad > MULTITRACK_RECORDING_1P && TASEdit_use_1p_rec)
			new_joy[joy] = new_joy[0];
		// superimpose (bitwise OR) if needed
		if (TASEdit_superimpose == BST_CHECKED || (TASEdit_superimpose == BST_INDETERMINATE && new_joy[joy] == 0))
			new_joy[joy] |= old_joy[joy];
		// other joysticks should not be changed
		currMovieData.records[currFrameCounter].joysticks[0] = old_joy[0];
		currMovieData.records[currFrameCounter].joysticks[1] = old_joy[1];
		if (currMovieData.fourscore)
		{
			currMovieData.records[currFrameCounter].joysticks[2] = old_joy[2];
			currMovieData.records[currFrameCounter].joysticks[3] = old_joy[3];
		}
		// change only this joystick
		currMovieData.records[currFrameCounter].joysticks[joy] = new_joy[joy];
		if (new_joy[joy] != old_joy[joy])
		{
			changes_made = true;
			// set lights for changed buttons
			for (int button = 0; button < NUM_JOYPAD_BUTTONS; ++button)
				if ((new_joy[joy] & (1 << button)) && !(old_joy[joy] & (1 << button)))
					tasedit_list.SetHeaderColumnLight(COLUMN_JOYPAD1_A + joy * NUM_JOYPAD_BUTTONS + button, HEADER_LIGHT_MAX);
		}
	}
	if (changes_made)
	{
		history.RegisterRecording(currFrameCounter);
		greenzone.Invalidate(currFrameCounter);
	}
}

const char* RECORDER::GetRecordingMode()
{
	return recordingModes[multitrack_recording_joypad];
}
const char* RECORDER::GetRecordingCaption()
{
	return recordingCaptions[multitrack_recording_joypad];
}

