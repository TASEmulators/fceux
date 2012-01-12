//Implementation file of TASEDITOR_LUA class
#include "taseditor_project.h"

extern TASEDITOR_WINDOW taseditor_window;
extern INPUT_HISTORY history;
extern MARKERS current_markers;
extern BOOKMARKS bookmarks;
extern RECORDER recorder;
extern PLAYBACK playback;
extern TASEDITOR_LIST list;
extern TASEDITOR_SELECTION selection;

extern void TaseditorUpdateManualFunctionStatus();

TASEDITOR_LUA::TASEDITOR_LUA()
{
}

void TASEDITOR_LUA::init()
{
	hwndRunFunction = GetDlgItem(taseditor_window.hwndTasEditor, TASEDITOR_RUN_MANUAL);
	TaseditorUpdateManualFunctionStatus();
	reset();
}
void TASEDITOR_LUA::reset()
{

}
void TASEDITOR_LUA::update()
{

}

void TASEDITOR_LUA::EnableRunFunction()
{
	EnableWindow(hwndRunFunction, true);
}
void TASEDITOR_LUA::DisableRunFunction()
{
	EnableWindow(hwndRunFunction, false);
}
// --------------------------------------------------------------------------------
// Lua functions of taseditor library

// bool taseditor.engaged()
bool TASEDITOR_LUA::engaged()
{
	return FCEUMOV_Mode(MOVIEMODE_TASEDITOR);
}

// bool taseditor.markedframe(int frame)
bool TASEDITOR_LUA::markedframe(int frame)
{
	if (FCEUMOV_Mode(MOVIEMODE_TASEDITOR))
		return current_markers.GetMarker(frame) != 0;
	else
		return false;
}

// int taseditor.getmarker(int frame)
int TASEDITOR_LUA::getmarker(int frame)
{
	if (FCEUMOV_Mode(MOVIEMODE_TASEDITOR))
		return current_markers.GetMarkerUp(frame);
	else
		return -1;
}

// int taseditor.setmarker(int frame)
int TASEDITOR_LUA::setmarker(int frame)
{
	if (FCEUMOV_Mode(MOVIEMODE_TASEDITOR))
	{
		int marker_id = current_markers.GetMarker(frame);
		if(!marker_id)
		{
			marker_id = current_markers.SetMarker(frame);
			if (marker_id)
			{
				// new marker was created - register changes in TAS Editor
				history.RegisterMarkersChange(MODTYPE_LUA_MARKER_SET, frame);
				selection.must_find_current_marker = playback.must_find_current_marker = true;
				list.SetHeaderColumnLight(COLUMN_FRAMENUM, HEADER_LIGHT_MAX);
			}
		}
		return marker_id;
	} else
		return -1;
}

// taseditor.clearmarker(int frame)
void TASEDITOR_LUA::clearmarker(int frame)
{
	if (FCEUMOV_Mode(MOVIEMODE_TASEDITOR))
	{
		if (current_markers.GetMarker(frame))
		{
			current_markers.ClearMarker(frame);
			// marker was deleted - register changes in TAS Editor
			history.RegisterMarkersChange(MODTYPE_LUA_MARKER_UNSET, frame);
			selection.must_find_current_marker = playback.must_find_current_marker = true;
			list.SetHeaderColumnLight(COLUMN_FRAMENUM, HEADER_LIGHT_MAX);
		}
	}
}

// string taseditor.getnote(int index)
const char* TASEDITOR_LUA::getnote(int index)
{
	if (FCEUMOV_Mode(MOVIEMODE_TASEDITOR))
		return current_markers.GetNote(index).c_str();
	else
		return NULL;
}

// taseditor.setnote(int index, string newtext)
void TASEDITOR_LUA::setnote(int index, const char* newtext)
{
	if (FCEUMOV_Mode(MOVIEMODE_TASEDITOR))
	{
		// rename only if newtext is different from old text
		char text[MAX_NOTE_LEN];
		strncpy(text, newtext, MAX_NOTE_LEN - 1);
		if (strcmp(current_markers.GetNote(index).c_str(), text))
		{
			// text differs from old note - rename
			current_markers.SetNote(index, text);
			history.RegisterMarkersChange(MODTYPE_LUA_MARKER_RENAME, current_markers.GetMarkerFrame(index));
			selection.must_find_current_marker = playback.must_find_current_marker = true;
		}
	}
}

// int taseditor.getcurrentbranch()
int TASEDITOR_LUA::getcurrentbranch()
{
	if (FCEUMOV_Mode(MOVIEMODE_TASEDITOR))
		return bookmarks.GetCurrentBranch();
	else
		return -1;
}

// string taseditor.getrecordermode()
const char* TASEDITOR_LUA::getrecordermode()
{
	if (FCEUMOV_Mode(MOVIEMODE_TASEDITOR))
		return recorder.GetRecordingMode();
	else
		return NULL;
}

// int taseditor.getplaybacktarget()
int TASEDITOR_LUA::getplaybacktarget()
{
	if (FCEUMOV_Mode(MOVIEMODE_TASEDITOR))
		return playback.pause_frame - 1;
	else
		return -1;
}

// taseditor.setplayback()
void TASEDITOR_LUA::setplayback(int frame)
{
	if (FCEUMOV_Mode(MOVIEMODE_TASEDITOR))
		playback.JumpToFrame(frame);	// do not trigger lua functions after jump
}

// taseditor.stopseeking()
void TASEDITOR_LUA::stopseeking()
{
	if (FCEUMOV_Mode(MOVIEMODE_TASEDITOR))
		playback.SeekingStop();
}





// --------------------------------------------------------------------------------














