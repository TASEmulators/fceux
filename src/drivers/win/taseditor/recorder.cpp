//Implementation file of RECORDER class
#include "taseditor_project.h"
#include "zlib.h"

extern int joysticks_per_frame[NUM_SUPPORTED_INPUT_TYPES];

extern uint32 GetGamepadPressedImmediate();
extern void ColumnSet(int column);
extern int GetInputType(MovieData& md);

extern TASEDITOR_CONFIG taseditor_config;
extern TASEDITOR_WINDOW taseditor_window;
extern BOOKMARKS bookmarks;
extern INPUT_HISTORY history;
extern GREENZONE greenzone;
extern TASEDITOR_LIST list;

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
	hwndRecCheckbox = GetDlgItem(taseditor_window.hwndTasEditor, IDC_RECORDING);
	hwndRB_RecAll = GetDlgItem(taseditor_window.hwndTasEditor, IDC_RADIO_ALL);
	hwndRB_Rec1P = GetDlgItem(taseditor_window.hwndTasEditor, IDC_RADIO_1P);
	hwndRB_Rec2P = GetDlgItem(taseditor_window.hwndTasEditor, IDC_RADIO_2P);
	hwndRB_Rec3P = GetDlgItem(taseditor_window.hwndTasEditor, IDC_RADIO_3P);
	hwndRB_Rec4P = GetDlgItem(taseditor_window.hwndTasEditor, IDC_RADIO_4P);
	old_multitrack_recording_joypad = multitrack_recording_joypad;
	old_movie_readonly = movie_readonly;
	old_joy.resize(MAX_NUM_JOYPADS);
	new_joy.resize(MAX_NUM_JOYPADS);
	current_joy.resize(MAX_NUM_JOYPADS);
}
void RECORDER::reset()
{
	movie_readonly = true;
	multitrack_recording_joypad = MULTITRACK_RECORDING_ALL;
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
	if (taseditor_config.branch_only_when_rec && old_movie_readonly != movie_readonly)
		bookmarks.RedrawBookmarksCaption();
	// update recording radio buttons if user used hotkey to switch R/W
	if (old_movie_readonly != movie_readonly || old_multitrack_recording_joypad != multitrack_recording_joypad)
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
	// fill current_joy data for listview header lights
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
	int num_joys = joysticks_per_frame[GetInputType(currMovieData)];
	// take previous values from current snapshot, new input from current movie
	for (int i = 0; i < num_joys; ++i)
	{
		old_joy[i] = history.GetCurrentSnapshot().GetJoystickInfo(currFrameCounter, i);
		new_joy[i] = currMovieData.records[currFrameCounter].joysticks[i];
	}
	// combine old and new data (superimpose) and filter out joystics that should not be recorded
	if (multitrack_recording_joypad == MULTITRACK_RECORDING_ALL)
	{
		for (int i = num_joys-1; i >= 0; i--)
		{
			// superimpose (bitwise OR) if needed
			if (taseditor_config.superimpose == BST_CHECKED || (taseditor_config.superimpose == BST_INDETERMINATE && new_joy[i] == 0))
				new_joy[i] |= old_joy[i];
			// change this joystick
			currMovieData.records[currFrameCounter].joysticks[i] = new_joy[i];
			if (new_joy[i] != old_joy[i])
			{
				changes_made = true;
				// set lights for changed buttons
				for (int button = 0; button < NUM_JOYPAD_BUTTONS; ++button)
					if ((new_joy[i] & (1 << button)) && !(old_joy[i] & (1 << button)))
						list.SetHeaderColumnLight(COLUMN_JOYPAD1_A + i * NUM_JOYPAD_BUTTONS + button, HEADER_LIGHT_MAX);
			}
		}
	} else
	{
		int joy = multitrack_recording_joypad - 1;
		// substitute target joypad with 1p joypad
		if (multitrack_recording_joypad > MULTITRACK_RECORDING_1P && taseditor_config.use_1p_rec)
			new_joy[joy] = new_joy[0];
		// superimpose (bitwise OR) if needed
		if (taseditor_config.superimpose == BST_CHECKED || (taseditor_config.superimpose == BST_INDETERMINATE && new_joy[joy] == 0))
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
					list.SetHeaderColumnLight(COLUMN_JOYPAD1_A + joy * NUM_JOYPAD_BUTTONS + button, HEADER_LIGHT_MAX);
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

