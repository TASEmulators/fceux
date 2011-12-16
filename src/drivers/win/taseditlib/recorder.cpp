//Implementation file of RECORDER class
#include "taseditproj.h"
#include "zlib.h"

extern HWND hwndTasEdit;

extern BOOKMARKS bookmarks;
extern INPUT_HISTORY history;
extern GREENZONE greenzone;

extern void RedrawWindowCaption();
extern bool TASEdit_branch_only_when_rec;
extern bool TASEdit_use_1p_rec;
extern int TASEdit_superimpose;

// resources
char recordingCaptions[5][30] = {	" (Recording All)",
								" (Recording 1P)",
								" (Recording 2P)",
								" (Recording 3P)",
								" (Recording 4P)"};
RECORDER::RECORDER()
{
}

void RECORDER::init()
{
	hwndRB_RecOff = GetDlgItem(hwndTasEdit, IDC_RADIO1);
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
	// update Bookmarks/Branches groupbox caption
	if (TASEdit_branch_only_when_rec && old_movie_readonly != movie_readonly)
		bookmarks.RedrawBookmarksCaption();
	// update recording radio buttons if user used hotkey to switch R/W
	if (old_movie_readonly != movie_readonly || old_multitrack_recording_joypad != multitrack_recording_joypad)
	{
		UncheckRecordingRadioButtons();
		RecheckRecordingRadioButtons();
	}

}
// ------------------------------------------------------------------------------------
void RECORDER::UncheckRecordingRadioButtons()
{
	Button_SetCheck(hwndRB_RecOff, BST_UNCHECKED);
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
	if (movie_readonly)
	{
		Button_SetCheck(hwndRB_RecOff, BST_CHECKED);
	} else
	{
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
				changes_made = true;
		}
	} else
	{
		int joy = multitrack_recording_joypad - 1;
		// substitute targed joypad with 1p joypad
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
			changes_made = true;
	}
	if (changes_made)
	{
		history.RegisterRecording(currFrameCounter);
		greenzone.Invalidate(currFrameCounter);
	}
}


