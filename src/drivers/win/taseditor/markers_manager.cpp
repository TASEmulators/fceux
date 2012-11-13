/* ---------------------------------------------------------------------------------
Implementation file of Markers_manager class
Copyright (c) 2011-2012 AnS

(The MIT License)
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------------
Markers_manager - Manager of Markers
[Singleton]

* stores one snapshot of Markers, representing current state of Markers in the project
* saves and loads the data from a project file. On error: clears the data
* regularly ensures that the size of current Markers array is not less than the number of frames in current Input
* implements all operations with Markers: setting Marker to a frame, removing Marker, inserting/deleting frames between Markers, truncating Markers array, changing Notes, finding frame for any given Marker, access to the data of Snapshot of Markers state
* implements full/partial copying of data between two Snapshots of Markers state, and searching for first difference between two Snapshots of Markers state
* also here's the code of searching for "similar" Notes
* also here's the code of editing Marker Notes
* also here's the code of Find Note dialog 
* stores resources: save id, properties of searching for similar Notes
------------------------------------------------------------------------------------ */

#include "taseditor_project.h"
#include <Shlwapi.h>		// for StrStrI

#pragma comment(lib, "Shlwapi.lib")

extern TASEDITOR_CONFIG taseditor_config;
extern TASEDITOR_WINDOW taseditor_window;
extern PLAYBACK playback;
extern SELECTION selection;
extern HISTORY history;

// resources
char markers_save_id[MARKERS_ID_LEN] = "MARKERS";
char markers_skipsave_id[MARKERS_ID_LEN] = "MARKERX";
char keywordDelimiters[] = " !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";

MARKERS_MANAGER::MARKERS_MANAGER()
{
	memset(findnote_string, 0, MAX_NOTE_LEN);
}

void MARKERS_MANAGER::init()
{
	reset();
}
void MARKERS_MANAGER::free()
{
	markers.markers_array.resize(0);
	markers.notes.resize(0);
}
void MARKERS_MANAGER::reset()
{
	free();
	marker_note_edit = MARKER_NOTE_EDIT_NONE;
	search_similar_marker = 0;
	markers.notes.resize(1);
	markers.notes[0] = "Power on";
	update();
}
void MARKERS_MANAGER::update()
{
	// the size of current markers_array must be no less then the size of Input
	if ((int)markers.markers_array.size() < currMovieData.getNumRecords())
		markers.markers_array.resize(currMovieData.getNumRecords());
}

void MARKERS_MANAGER::save(EMUFILE *os, bool really_save)
{
	if (really_save)
	{
		// write "MARKERS" string
		os->fwrite(markers_save_id, MARKERS_ID_LEN);
		markers.Reset_already_compressed();		// must recompress data, because most likely it has changed since last compression
		markers.save(os);
	} else
	{
		// write "MARKERX" string, meaning that Markers are not saved
		os->fwrite(markers_skipsave_id, MARKERS_ID_LEN);
	}
}
// returns true if couldn't load
bool MARKERS_MANAGER::load(EMUFILE *is, unsigned int offset)
{
	if (offset)
	{
		if (is->fseek(offset, SEEK_SET)) goto error;
	} else
	{
		reset();
		return false;
	}
	// read "MARKERS" string
	char save_id[MARKERS_ID_LEN];
	if ((int)is->fread(save_id, MARKERS_ID_LEN) < MARKERS_ID_LEN) goto error;
	if (!strcmp(markers_skipsave_id, save_id))
	{
		// string says to skip loading Markers
		FCEU_printf("No Markers in the file\n");
		reset();
		return false;
	}
	if (strcmp(markers_save_id, save_id)) goto error;		// string is not valid
	if (markers.load(is)) goto error;
	// all ok
	return false;
error:
	FCEU_printf("Error loading Markers\n");
	reset();
	return true;
}
// -----------------------------------------------------------------------------------------
int MARKERS_MANAGER::GetMarkersSize()
{
	return markers.markers_array.size();
}
bool MARKERS_MANAGER::SetMarkersSize(int new_size)
{
	// if we are truncating, clear Markers that are gonna be erased (so that obsolete notes will be erased too)
	bool markers_changed = false;
	for (int i = markers.markers_array.size() - 1; i >= new_size; i--)
	{
		if (markers.markers_array[i])
		{
			ClearMarker(i);
			markers_changed = true;
		}
	}
	markers.markers_array.resize(new_size);
	return markers_changed;
}

int MARKERS_MANAGER::GetMarker(int frame)
{
	if (frame >= 0 && frame < (int)markers.markers_array.size())
		return markers.markers_array[frame];
	else
		return 0;
}
// finds and returns # of Marker starting from start_frame and searching up
int MARKERS_MANAGER::GetMarkerUp(int start_frame)
{
	if (start_frame >= (int)markers.markers_array.size())
		start_frame = markers.markers_array.size() - 1;
	for (; start_frame >= 0; start_frame--)
		if (markers.markers_array[start_frame]) return markers.markers_array[start_frame];
	return 0;
}
// special version of the function
int MARKERS_MANAGER::GetMarkerUp(MARKERS& target_markers, int start_frame)
{
	if (start_frame >= (int)target_markers.markers_array.size())
		start_frame = target_markers.markers_array.size() - 1;
	for (; start_frame >= 0; start_frame--)
		if (target_markers.markers_array[start_frame]) return target_markers.markers_array[start_frame];
	return 0;
}
// finds frame where the Marker is set
int MARKERS_MANAGER::GetMarkerFrame(int marker_id)
{
	for (int i = markers.markers_array.size() - 1; i >= 0; i--)
		if (markers.markers_array[i] == marker_id) return i;
	// didn't find
	return -1;
}
// returns number of new Marker
int MARKERS_MANAGER::SetMarker(int frame)
{
	if (frame < 0)
		return 0;
	else if (frame >= (int)markers.markers_array.size())
		markers.markers_array.resize(frame + 1);
	else if (markers.markers_array[frame])
		return markers.markers_array[frame];

	int marker_num = GetMarkerUp(frame) + 1;
	markers.markers_array[frame] = marker_num;
	if (taseditor_config.empty_marker_notes)
		markers.notes.insert(markers.notes.begin() + marker_num, 1, "");
	else
		// copy previous Marker note
		markers.notes.insert(markers.notes.begin() + marker_num, 1, markers.notes[marker_num - 1]);
	// increase following Markers' ids
	int size = markers.markers_array.size();
	for (frame++; frame < size; ++frame)
		if (markers.markers_array[frame])
			markers.markers_array[frame]++;
	return marker_num;
}
void MARKERS_MANAGER::ClearMarker(int frame)
{
	if (markers.markers_array[frame])
	{
		// erase corresponding note
		markers.notes.erase(markers.notes.begin() + markers.markers_array[frame]);
		// clear Marker
		markers.markers_array[frame] = 0;
		// decrease following Markers' ids
		int size = markers.markers_array.size();
		for (frame++; frame < size; ++frame)
			if (markers.markers_array[frame])
				markers.markers_array[frame]--;
	}
}
void MARKERS_MANAGER::ToggleMarker(int frame)
{
	if (frame >= 0 && frame < (int)markers.markers_array.size())
	{
		if (markers.markers_array[frame])
			ClearMarker(frame);
		else
			SetMarker(frame);
	}
}

bool MARKERS_MANAGER::EraseMarker(int frame, int frames)
{
	bool markers_changed = false;
	if (frame < (int)markers.markers_array.size())
	{
		if (frames == 1)
		{
			// erase 1 frame
			// if there's a Marker, first clear it
			if (markers.markers_array[frame])
			{
				ClearMarker(frame);
				markers_changed = true;
			}
			// erase 1 frame
			markers.markers_array.erase(markers.markers_array.begin() + frame);
		} else
		{
			// erase many frames
			if (frame + frames > (int)markers.markers_array.size())
				frames = (int)markers.markers_array.size() - frame;
			// if there are Markers at those frames, first clear them
			for (int i = frame; i < (frame + frames); ++i)
			{
				if (markers.markers_array[i])
				{
					ClearMarker(i);
					markers_changed = true;
				}
			}
			// erase frames
			markers.markers_array.erase(markers.markers_array.begin() + frame, markers.markers_array.begin() + (frame + frames));
		}
		// check if there were some Markers after this frame
		// since these Markers were shifted, markers_changed should be set to true
		if (!markers_changed)
		{
			for (int i = markers.markers_array.size() - 1; i >= frame; i--)
			{
				if (markers.markers_array[i])
				{
					markers_changed = true;		// Markers moved
					break;
				}
			}
		}
	}
	return markers_changed;
}
bool MARKERS_MANAGER::insertEmpty(int at, int frames)
{
	if (at == -1) 
	{
		// append blank frames
		markers.markers_array.resize(markers.markers_array.size() + frames);
		return false;
	} else
	{
		bool markers_changed = false;
		// first check if there are Markers after the frame
		for (int i = markers.markers_array.size() - 1; i >= at; i--)
		{
			if (markers.markers_array[i])
			{
				markers_changed = true;		// Markers moved
				break;
			}
		}
		markers.markers_array.insert(markers.markers_array.begin() + at, frames, 0);
		return markers_changed;
	}
}

int MARKERS_MANAGER::GetNotesSize()
{
	return markers.notes.size();
}
std::string MARKERS_MANAGER::GetNote(int index)
{
	if (index >= 0 && index < (int)markers.notes.size())
		return markers.notes[index];
	else
		return markers.notes[0];
}
// special version of the function
std::string MARKERS_MANAGER::GetNote(MARKERS& target_markers, int index)
{
	if (index >= 0 && index < (int)target_markers.notes.size())
		return target_markers.notes[index];
	else
		return target_markers.notes[0];
}
void MARKERS_MANAGER::SetNote(int index, const char* new_text)
{
	if (index >= 0 && index < (int)markers.notes.size())
		markers.notes[index] = new_text;
}
// ---------------------------------------------------------------------------------------
void MARKERS_MANAGER::MakeCopyTo(MARKERS& destination)
{
	destination.markers_array = markers.markers_array;
	destination.notes = markers.notes;
	destination.Reset_already_compressed();
}
void MARKERS_MANAGER::RestoreFromCopy(MARKERS& source)
{
	markers.markers_array = source.markers_array;
	markers.notes = source.notes;
}

// return true only when difference is found before end frame (not including end frame)
bool MARKERS_MANAGER::checkMarkersDiff(MARKERS& their_markers)
{
	int end_my = GetMarkersSize() - 1;
	int end_their = their_markers.markers_array.size() - 1;
	int min_end = end_my;
	int i;
	// 1 - check if there are any Markers after min_end
	if (end_my < end_their)
	{
		for (i = end_their; i > min_end; i--)
			if (their_markers.markers_array[i])
				return true;
	} else if (end_my > end_their)
	{
		min_end = end_their;
		for (i = end_my; i > min_end; i--)
			if (markers.markers_array[i])
				return true;
	}
	// 2 - check if there's any difference before min_end
	for (i = min_end; i >= 0; i--)
	{
		if (markers.markers_array[i] != their_markers.markers_array[i])
			return true;
		else if (markers.markers_array[i] &&	// not empty
			markers.notes[markers.markers_array[i]].compare(their_markers.notes[their_markers.markers_array[i]]))	// notes differ
			return true;
	}
	// 3 - check if there's difference between 0th Notes
	if (markers.notes[0].compare(their_markers.notes[0]))
		return true;
	// no difference found
	return false;
}
// ------------------------------------------------------------------------------------
// custom ordering function, used by std::sort
bool ordering(const std::pair<int, double>& d1, const std::pair<int, double>& d2)
{
  return d1.second < d2.second;
}

void MARKERS_MANAGER::FindSimilar()
{
	search_similar_marker = 0;
	FindNextSimilar();
}
void MARKERS_MANAGER::FindNextSimilar()
{
	int i, t;
	int sourceMarker = playback.shown_marker;
	char sourceNote[MAX_NOTE_LEN];
	strcpy(sourceNote, GetNote(sourceMarker).c_str());

	// check if playback_marker_text is empty
	if (!sourceNote[0])
	{
		MessageBox(taseditor_window.hwndTasEditor, "Marker Note under Playback cursor is empty!", "Find Similar Note", MB_OK);
		return;
	}
	// check if there's at least one note (not counting zeroth note)
	if (markers.notes.size() <= 0)
	{
		MessageBox(taseditor_window.hwndTasEditor, "This project doesn't have any Markers!", "Find Similar Note", MB_OK);
		return;
	}

	// 0 - divide source string into keywords
	int totalSourceKeywords = 0;
	char sourceKeywords[MAX_NUM_KEYWORDS][MAX_NOTE_LEN] = {0};
	int current_line_pos = 0;
	char sourceKeywordsLine[MAX_NUM_KEYWORDS] = {0};
	char* pch;
	// divide into tokens
	pch = strtok(sourceNote, keywordDelimiters);
	while (pch != NULL)
	{
		if (strlen(pch) >= KEYWORD_MIN_LEN)
		{
			// check if same keyword already appeared in the string
			for (t = totalSourceKeywords - 1; t >= 0; t--)
				if (!_stricmp(sourceKeywords[t], pch)) break;
			if (t < 0)
			{
				// save new keyword
				strcpy(sourceKeywords[totalSourceKeywords], pch);
				// also set its id into the line
				sourceKeywordsLine[current_line_pos++] = totalSourceKeywords + 1;
				totalSourceKeywords++;
			} else
			{
				// same keyword found
				sourceKeywordsLine[current_line_pos++] = t + 1;
			}
		}
		pch = strtok(NULL, keywordDelimiters);
	}
	// we found the line (sequence) of keywords
	sourceKeywordsLine[current_line_pos] = 0;
	
	if (!totalSourceKeywords)
	{
		MessageBox(taseditor_window.hwndTasEditor, "Marker Note under Playback cursor doesn't have keywords!", "Find Similar Note", MB_OK);
		return;
	}

	// 1 - find how frequently each keyword appears in notes
	std::vector<int> keywordFound(totalSourceKeywords);
	char checkedNote[MAX_NOTE_LEN];
	for (i = markers.notes.size() - 1; i > 0; i--)
	{
		if (i != sourceMarker)
		{
			strcpy(checkedNote, markers.notes[i].c_str());
			for (t = totalSourceKeywords - 1; t >= 0; t--)
				if (StrStrI(checkedNote, sourceKeywords[t]))
					keywordFound[t]++;
		}
	}
	// findmax
	int maxFound = 0;
	for (t = totalSourceKeywords - 1; t >= 0; t--)
		if (maxFound < keywordFound[t])
			maxFound = keywordFound[t];
	// and then calculate weight of each keyword: the more often it appears in Markers, the less weight it has
	std::vector<double> keywordWeight(totalSourceKeywords);
	for (t = totalSourceKeywords - 1; t >= 0; t--)
		keywordWeight[t] = KEYWORD_WEIGHT_BASE + KEYWORD_WEIGHT_FACTOR * (keywordFound[t] / (double)maxFound);

	// start accumulating priorities
	std::vector<std::pair<int, double>> notePriority(markers.notes.size());

	// 2 - find keywords in notes (including cases when keyword appears inside another word)
	for (i = notePriority.size() - 1; i > 0; i--)
	{
		notePriority[i].first = i;
		if (i != sourceMarker)
		{
			strcpy(checkedNote, markers.notes[i].c_str());
			for (t = totalSourceKeywords - 1; t >= 0; t--)
			{
				if (StrStrI(checkedNote, sourceKeywords[t]))
					notePriority[i].second += KEYWORD_CASEINSENTITIVE_BONUS_PER_CHAR * keywordWeight[t] * strlen(sourceKeywords[t]);
				if (strstr(checkedNote, sourceKeywords[t]))
					notePriority[i].second += KEYWORD_CASESENTITIVE_BONUS_PER_CHAR * keywordWeight[t] * strlen(sourceKeywords[t]);
			}
		}
	}

	// 3 - search sequences of keywords from all other notes
	current_line_pos = 0;
	char checkedKeywordsLine[MAX_NUM_KEYWORDS] = {0};
	int keyword_id;
	for (i = markers.notes.size() - 1; i > 0; i--)
	{
		if (i != sourceMarker)
		{
			strcpy(checkedNote, markers.notes[i].c_str());
			// divide into tokens
			pch = strtok(checkedNote, keywordDelimiters);
			while (pch != NULL)
			{
				if (strlen(pch) >= KEYWORD_MIN_LEN)
				{
					// check if the keyword is one of sourceKeywords
					for (t = totalSourceKeywords - 1; t >= 0; t--)
						if (!_stricmp(sourceKeywords[t], pch)) break;
					if (t >= 0)
					{
						// the keyword is one of sourceKeywords - set its id into the line
						checkedKeywordsLine[current_line_pos++] = t + 1;
					} else
					{
						// found keyword that doesn't appear in sourceNote, give penalty
						notePriority[i].second -= KEYWORD_PENALTY_FOR_STRANGERS * strlen(pch);
						// since the keyword breaks our sequence of coincident keywords, check if that sequence is similar to sourceKeywordsLine
						if (current_line_pos >= KEYWORDS_LINE_MIN_SEQUENCE)
						{
							checkedKeywordsLine[current_line_pos] = 0;
							// search checkedKeywordsLine in sourceKeywordsLine
							if (strstr(sourceKeywordsLine, checkedKeywordsLine))
							{
								// found same sequence of keywords! add priority to this checkedNote
								for (t = current_line_pos - 1; t >= 0; t--)
								{
									// add bonus for every keyword in the sequence
									keyword_id = checkedKeywordsLine[t] - 1;
									notePriority[i].second += current_line_pos * KEYWORD_SEQUENCE_BONUS_PER_CHAR * keywordWeight[keyword_id] * strlen(sourceKeywords[keyword_id]);
								}
							}
						}
						// clear checkedKeywordsLine
						memset(checkedKeywordsLine, 0, MAX_NUM_KEYWORDS);
						current_line_pos = 0;
					}
				}
				pch = strtok(NULL, keywordDelimiters);
			}
			// finished dividing into tokens
			if (current_line_pos >= KEYWORDS_LINE_MIN_SEQUENCE)
			{
				checkedKeywordsLine[current_line_pos] = 0;
				// search checkedKeywordsLine in sourceKeywordsLine
				if (strstr(sourceKeywordsLine, checkedKeywordsLine))
				{
					// found same sequence of keywords! add priority to this checkedNote
					for (t = current_line_pos - 1; t >= 0; t--)
					{
						// add bonus for every keyword in the sequence
						keyword_id = checkedKeywordsLine[t] - 1;
						notePriority[i].second += current_line_pos * KEYWORD_SEQUENCE_BONUS_PER_CHAR * keywordWeight[keyword_id] * strlen(sourceKeywords[keyword_id]);
					}
				}
			}
			// clear checkedKeywordsLine
			memset(checkedKeywordsLine, 0, MAX_NUM_KEYWORDS);
			current_line_pos = 0;
		}
	}

	// 4 - sort notePriority by second member of the pair
	std::sort(notePriority.begin(), notePriority.end(), ordering);

	/*
	// debug trace
	FCEU_printf("\n\n\n\n\n\n\n\n\n\n");
	for (t = totalSourceKeywords - 1; t >= 0; t--)
		FCEU_printf("Keyword: %s, %d, %f\n", sourceKeywords[t], keywordFound[t], keywordWeight[t]);
	for (i = notePriority.size() - 1; i > 0; i--)
	{
		int marker_id = notePriority[i].first;
		FCEU_printf("Result: %s, %d, %f\n", notes[marker_id].c_str(), marker_id, notePriority[i].second);
	}
	*/

	// Send Selection to the Marker found
	int index = notePriority.size()-1 - search_similar_marker;
	if (index >= 0 && notePriority[index].second >= MIN_PRIORITY_TRESHOLD)
	{
		int marker_id = notePriority[index].first;
		int frame = GetMarkerFrame(marker_id);
		if (frame >= 0)
			selection.JumpToFrame(frame);
	} else
	{
		if (search_similar_marker)
			MessageBox(taseditor_window.hwndTasEditor, "Could not find more Notes similar to Marker Note under Playback cursor!", "Find Similar Note", MB_OK);
		else
			MessageBox(taseditor_window.hwndTasEditor, "Could not find anything similar to Marker Note under Playback cursor!", "Find Similar Note", MB_OK);
	}

	// increase search_similar_marker so that next time we'll find another note
	search_similar_marker++;
}
// ------------------------------------------------------------------------------------
void MARKERS_MANAGER::UpdateMarkerNote()
{
	if (!marker_note_edit) return;
	char new_text[MAX_NOTE_LEN];
	if (marker_note_edit == MARKER_NOTE_EDIT_UPPER)
	{
		int len = SendMessage(playback.hwndPlaybackMarkerEdit, WM_GETTEXT, MAX_NOTE_LEN, (LPARAM)new_text);
		new_text[len] = 0;
		// check changes
		if (strcmp(GetNote(playback.shown_marker).c_str(), new_text))
		{
			SetNote(playback.shown_marker, new_text);
			if (playback.shown_marker)
				history.RegisterMarkersChange(MODTYPE_MARKER_RENAME, GetMarkerFrame(playback.shown_marker), -1, new_text);
			else
				// zeroth Marker - just assume it's set on frame 0
				history.RegisterMarkersChange(MODTYPE_MARKER_RENAME, 0, -1, new_text);
			// notify Selection to change text in the lower Marker (in case both are showing same Marker)
			selection.must_find_current_marker = true;
		}
	} else if (marker_note_edit == MARKER_NOTE_EDIT_LOWER)
	{
		int len = SendMessage(selection.hwndSelectionMarkerEdit, WM_GETTEXT, MAX_NOTE_LEN, (LPARAM)new_text);
		new_text[len] = 0;
		// check changes
		if (strcmp(GetNote(selection.shown_marker).c_str(), new_text))
		{
			SetNote(selection.shown_marker, new_text);
			if (selection.shown_marker)
				history.RegisterMarkersChange(MODTYPE_MARKER_RENAME, GetMarkerFrame(selection.shown_marker), -1, new_text);
			else
				// zeroth Marker - just assume it's set on frame 0
				history.RegisterMarkersChange(MODTYPE_MARKER_RENAME, 0, -1, new_text);
			// notify Playback to change text in upper Marker (in case both are showing same Marker)
			playback.must_find_current_marker = true;
		}
	}
}
// ------------------------------------------------------------------------------------
BOOL CALLBACK FindNoteProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	extern MARKERS_MANAGER markers_manager;
	switch (message)
	{
		case WM_INITDIALOG:
		{
			if (taseditor_config.findnote_wndx == -32000) taseditor_config.findnote_wndx = 0; //Just in case
			if (taseditor_config.findnote_wndy == -32000) taseditor_config.findnote_wndy = 0;
			SetWindowPos(hwndDlg, 0, taseditor_config.findnote_wndx, taseditor_config.findnote_wndy, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);

			CheckDlgButton(hwndDlg, IDC_MATCH_CASE, taseditor_config.findnote_matchcase?MF_CHECKED : MF_UNCHECKED);
			if (taseditor_config.findnote_search_up)
				Button_SetCheck(GetDlgItem(hwndDlg, IDC_RADIO_UP), BST_CHECKED);
			else
				Button_SetCheck(GetDlgItem(hwndDlg, IDC_RADIO_DOWN), BST_CHECKED);
			HWND hwndEdit = GetDlgItem(hwndDlg, IDC_NOTE_TO_FIND);
			SendMessage(hwndEdit, EM_SETLIMITTEXT, MAX_NOTE_LEN - 1, 0);
			SetWindowText(hwndEdit, markers_manager.findnote_string);
			if (GetDlgCtrlID((HWND)wParam) != IDC_NOTE_TO_FIND)
		    {
				SetFocus(hwndEdit);
				return false;
			}
			return true;
		}
		case WM_MOVE:
		{
			if (!IsIconic(hwndDlg))
			{
				RECT wrect;
				GetWindowRect(hwndDlg, &wrect);
				taseditor_config.findnote_wndx = wrect.left;
				taseditor_config.findnote_wndy = wrect.top;
				WindowBoundsCheckNoResize(taseditor_config.findnote_wndx, taseditor_config.findnote_wndy, wrect.right);
			}
			break;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDC_NOTE_TO_FIND:
				{
					if (HIWORD(wParam) == EN_CHANGE) 
					{
						if (GetWindowTextLength(GetDlgItem(hwndDlg, IDC_NOTE_TO_FIND)))
							EnableWindow(GetDlgItem(hwndDlg, IDOK), true);
						else
							EnableWindow(GetDlgItem(hwndDlg, IDOK), false);
					}
					break;
				}
				case IDC_RADIO_UP:
					taseditor_config.findnote_search_up = true;
					break;
				case IDC_RADIO_DOWN:
					taseditor_config.findnote_search_up = false;
					break;
				case IDC_MATCH_CASE:
					taseditor_config.findnote_matchcase ^= 1;
					CheckDlgButton(hwndDlg, IDC_MATCH_CASE, taseditor_config.findnote_matchcase?MF_CHECKED : MF_UNCHECKED);
					break;
				case IDOK:
				{
					int len = SendMessage(GetDlgItem(hwndDlg, IDC_NOTE_TO_FIND), WM_GETTEXT, MAX_NOTE_LEN, (LPARAM)markers_manager.findnote_string);
					markers_manager.findnote_string[len] = 0;
					// scan frames from current Selection to the border
					int cur_marker = 0;
					bool result;
					int movie_size = currMovieData.getNumRecords();
					int current_frame = selection.GetCurrentSelectionBeginning();
					if (current_frame < 0 && taseditor_config.findnote_search_up)
						current_frame = movie_size;
					while (true)
					{
						// move forward
						if (taseditor_config.findnote_search_up)
						{
							current_frame--;
							if (current_frame < 0)
							{
								MessageBox(taseditor_window.hwndFindNote, "Nothing was found.", "Find Note", MB_OK);
								break;
							}
						} else
						{
							current_frame++;
							if (current_frame >= movie_size)
							{
								MessageBox(taseditor_window.hwndFindNote, "Nothing was found!", "Find Note", MB_OK);
								break;
							}
						}
						// scan marked frames
						cur_marker = markers_manager.GetMarker(current_frame);
						if (cur_marker)
						{
							if (taseditor_config.findnote_matchcase)
								result = (strstr(markers_manager.GetNote(cur_marker).c_str(), markers_manager.findnote_string) != 0);
							else
								result = (StrStrI(markers_manager.GetNote(cur_marker).c_str(), markers_manager.findnote_string) != 0);
							if (result)
							{
								// found note containing searched string - jump there
								selection.JumpToFrame(current_frame);
								break;
							}
						}
					}
					return TRUE;
				}
				case IDCANCEL:
					DestroyWindow(taseditor_window.hwndFindNote);
					taseditor_window.hwndFindNote = 0;
					return TRUE;
			}
			break;
		}
		case WM_CLOSE:
		case WM_QUIT:
		{
			DestroyWindow(taseditor_window.hwndFindNote);
			taseditor_window.hwndFindNote = 0;
			break;
		}
	}
	return FALSE; 
} 


