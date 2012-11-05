/* ---------------------------------------------------------------------------------
Implementation file of EDITOR class
Copyright (c) 2011-2012 AnS

(The MIT License)
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------------
Editor - Interface for editing Input and Markers
[Singleton]

* implements operations of changing Input: toggle Input in region, set Input by pattern, toggle selected region, apply pattern to Input selection
* implements operations of changing Markers: toggle Markers in selection, apply patern to Markers in selection, mark/unmark all selected frames
* stores Autofire Patterns data and their loading/generating code
* stores resources: patterns filename, id of buttonpresses in patterns
------------------------------------------------------------------------------------ */

#include "taseditor_project.h"

extern TASEDITOR_WINDOW taseditor_window;
extern TASEDITOR_CONFIG taseditor_config;
extern HISTORY history;
extern MARKERS_MANAGER markers_manager;
extern PLAYBACK playback;
extern GREENZONE greenzone;
extern PIANO_ROLL piano_roll;
extern SELECTION selection;

extern int joysticks_per_frame[NUM_SUPPORTED_INPUT_TYPES];
extern int GetInputType(MovieData& md);

// resources
char patternsFilename[] = "\\taseditor_patterns.txt";
char autofire_patterns_flagpress = 49;	//	"1"

EDITOR::EDITOR()
{
}

void EDITOR::init()
{
	free();
	// load autofire_patterns
	int total_patterns = 0;
	char nameo[2048];
	strncpy(nameo, FCEU_GetPath(FCEUMKF_TASEDITOR).c_str(), 2047);
	strncat(nameo, patternsFilename, 2047 - strlen(nameo));
	EMUFILE_FILE ifs(nameo, "rb");
	if (!ifs.fail())
	{
		std::string tempstr1, tempstr2;
		while (ReadString(&ifs, tempstr1))
		{
			if (ReadString(&ifs, tempstr2))
			{
				total_patterns++;
				// save the name
				autofire_patterns_names.push_back(tempstr1);
				// parse 2nd string to sequence of 1s and 0s
				autofire_patterns.resize(total_patterns);
				autofire_patterns[total_patterns - 1].resize(tempstr2.size());
				for (int i = tempstr2.size() - 1; i >= 0; i--)
				{
					if (tempstr2[i] == autofire_patterns_flagpress)
						autofire_patterns[total_patterns - 1][i] = 1;
					else
						autofire_patterns[total_patterns - 1][i] = 0;
				}
			}
		}
	} else
	{
		FCEU_printf("Could not load tools\\taseditor_patterns.txt!\n");
	}
	if (autofire_patterns.size() == 0)
	{
		FCEU_printf("Will be using default set of patterns...\n");
		autofire_patterns.resize(4);
		autofire_patterns_names.resize(4);
		// Default Pattern 0: Alternating (1010...)
		autofire_patterns_names[0] = "Alternating (1010...)";
		autofire_patterns[0].resize(2);
		autofire_patterns[0][0] = 1;
		autofire_patterns[0][1] = 0;
		// Default Pattern 1: Alternating at 30FPS (11001100...)
		autofire_patterns_names[1] = "Alternating at 30FPS (11001100...)";
		autofire_patterns[1].resize(4);
		autofire_patterns[1][0] = 1;
		autofire_patterns[1][1] = 1;
		autofire_patterns[1][2] = 0;
		autofire_patterns[1][3] = 0;
		// Default Pattern 2: One Quarter (10001000...)
		autofire_patterns_names[2] = "One Quarter (10001000...)";
		autofire_patterns[2].resize(4);
		autofire_patterns[2][0] = 1;
		autofire_patterns[2][1] = 0;
		autofire_patterns[2][2] = 0;
		autofire_patterns[2][3] = 0;
		// Default Pattern 3: Tap'n'Hold (1011111111111111111111111111111111111...)
		autofire_patterns_names[3] = "Tap'n'Hold (101111111...)";
		autofire_patterns[3].resize(1000);
		autofire_patterns[3][0] = 1;
		autofire_patterns[3][1] = 0;
		for (int i = 2; i < 1000; ++i)
			autofire_patterns[3][i] = 1;
	}
	// reset current_pattern if it's outside the range
	if (taseditor_config.current_pattern < 0 || taseditor_config.current_pattern >= (int)autofire_patterns.size())
		taseditor_config.current_pattern = 0;
	taseditor_window.UpdatePatternsMenu();

	reset();
}
void EDITOR::free()
{
	autofire_patterns.resize(0);
	autofire_patterns_names.resize(0);
}
void EDITOR::reset()
{

}
void EDITOR::update()
{



}
// ----------------------------------------------------------------------------------------------
// returns false if couldn't read a string containing at least one char
bool EDITOR::ReadString(EMUFILE *is, std::string& dest)
{
	dest.resize(0);
	int charr;
	while (true)
	{
		charr = is->fgetc();
		if (charr < 0) break;
		if (charr == 10 || charr == 13)		// end of line
		{
			if (dest.size())
				break;		// already collected at least one char
			else
				continue;	// skip the char and continue searching
		} else
		{
			dest.push_back(charr);
		}
	}
	return dest.size() != 0;
}
// ----------------------------------------------------------------------------------------------
// following functions use function parameters to determine range of frames
void EDITOR::InputToggle(int start, int end, int joy, int button, int consecutive_tag)
{
	if (joy < 0 || joy >= joysticks_per_frame[GetInputType(currMovieData)]) return;

	int check_frame = end;
	if (start > end)
	{
		// swap
		int temp_start = start;
		start = end;
		end = temp_start;
	}
	if (start < 0) start = end;
	if (end >= currMovieData.getNumRecords())
		return;

	if (currMovieData.records[check_frame].checkBit(joy, button))
	{
		// clear range
		for (int i = start; i <= end; ++i)
			currMovieData.records[i].clearBit(joy, button);
		greenzone.InvalidateAndCheck(history.RegisterChanges(MODTYPE_UNSET, start, end, 0, NULL, consecutive_tag));
	} else
	{
		// set range
		for (int i = start; i <= end; ++i)
			currMovieData.records[i].setBit(joy, button);
		greenzone.InvalidateAndCheck(history.RegisterChanges(MODTYPE_SET, start, end, 0, NULL, consecutive_tag));
	}
}
void EDITOR::InputSetPattern(int start, int end, int joy, int button, int consecutive_tag)
{
	if (joy < 0 || joy >= joysticks_per_frame[GetInputType(currMovieData)]) return;

	if (start > end)
	{
		// swap
		int temp_start = start;
		start = end;
		end = temp_start;
	}
	if (start < 0) start = end;
	if (end >= currMovieData.getNumRecords())
		return;

	int pattern_offset = 0, current_pattern = taseditor_config.current_pattern;
	bool changes_made = false;
	bool value;

	for (int i = start; i <= end; ++i)
	{
		// skip lag frames
		if (taseditor_config.pattern_skips_lag && greenzone.laglog.GetLagInfoAtFrame(i) == LAGGED_YES)
			continue;
		value = (autofire_patterns[current_pattern][pattern_offset] != 0);
		if (currMovieData.records[i].checkBit(joy, button) != value)
		{
			changes_made = true;
			currMovieData.records[i].setBitValue(joy, button, value);
		}
		pattern_offset++;
		if (pattern_offset >= (int)autofire_patterns[current_pattern].size())
			pattern_offset -= autofire_patterns[current_pattern].size();
	}
	if (changes_made)
		greenzone.InvalidateAndCheck(history.RegisterChanges(MODTYPE_PATTERN, start, end, 0, autofire_patterns_names[current_pattern].c_str(), consecutive_tag));
}

// following functions use current Selection to determine range of frames
bool EDITOR::FrameColumnSet()
{
	SelectionFrames* current_selection = selection.MakeStrobe();
	if (current_selection->size() == 0) return false;
	SelectionFrames::iterator current_selection_begin(current_selection->begin());
	SelectionFrames::iterator current_selection_end(current_selection->end());

	// inspect the selected frames, if they are all set, then unset all, else set all
	bool unset_found = false, changes_made = false;
	for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
	{
		if (!markers_manager.GetMarker(*it))
		{
			unset_found = true;
			break;
		}
	}
	if (unset_found)
	{
		// set all
		for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
		{
			if (!markers_manager.GetMarker(*it))
			{
				if (markers_manager.SetMarker(*it))
				{
					changes_made = true;
					piano_roll.RedrawRow(*it);
				}
			}
		}
		if (changes_made)
			history.RegisterMarkersChange(MODTYPE_MARKER_SET, *current_selection_begin, *current_selection->rbegin());
	} else
	{
		// unset all
		for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
		{
			if (markers_manager.GetMarker(*it))
			{
				markers_manager.ClearMarker(*it);
				changes_made = true;
				piano_roll.RedrawRow(*it);
			}
		}
		if (changes_made)
			history.RegisterMarkersChange(MODTYPE_MARKER_REMOVE, *current_selection_begin, *current_selection->rbegin());
	}
	if (changes_made)
		selection.must_find_current_marker = playback.must_find_current_marker = true;
	return changes_made;
}
bool EDITOR::FrameColumnSetPattern()
{
	SelectionFrames* current_selection = selection.MakeStrobe();
	if (current_selection->size() == 0) return false;
	SelectionFrames::iterator current_selection_begin(current_selection->begin());
	SelectionFrames::iterator current_selection_end(current_selection->end());
	int pattern_offset = 0, current_pattern = taseditor_config.current_pattern;
	bool changes_made = false;

	for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
	{
		// skip lag frames
		if (taseditor_config.pattern_skips_lag && greenzone.laglog.GetLagInfoAtFrame(*it) == LAGGED_YES)
			continue;
		if (autofire_patterns[current_pattern][pattern_offset])
		{
			if (!markers_manager.GetMarker(*it))
			{
				if (markers_manager.SetMarker(*it))
				{
					changes_made = true;
					piano_roll.RedrawRow(*it);
				}
			}
		} else
		{
			if (markers_manager.GetMarker(*it))
			{
				markers_manager.ClearMarker(*it);
				changes_made = true;
				piano_roll.RedrawRow(*it);
			}
		}
		pattern_offset++;
		if (pattern_offset >= (int)autofire_patterns[current_pattern].size())
			pattern_offset -= autofire_patterns[current_pattern].size();
	}
	if (changes_made)
	{
		history.RegisterMarkersChange(MODTYPE_MARKER_PATTERN, *current_selection_begin, *current_selection->rbegin(), autofire_patterns_names[current_pattern].c_str());
		selection.must_find_current_marker = playback.must_find_current_marker = true;
		return true;
	} else
		return false;
}

bool EDITOR::InputColumnSet(int joy, int button)
{
	if (joy < 0 || joy >= joysticks_per_frame[GetInputType(currMovieData)]) return false;

	SelectionFrames* current_selection = selection.MakeStrobe();
	if (current_selection->size() == 0) return false;
	SelectionFrames::iterator current_selection_begin(current_selection->begin());
	SelectionFrames::iterator current_selection_end(current_selection->end());

	//inspect the selected frames, if they are all set, then unset all, else set all
	bool newValue = false;
	for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
	{
		if (!(currMovieData.records[*it].checkBit(joy,button)))
		{
			newValue = true;
			break;
		}
	}
	// apply newValue
	for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
		currMovieData.records[*it].setBitValue(joy,button,newValue);

	int first_changes;
	if (newValue)
	{
		first_changes = history.RegisterChanges(MODTYPE_SET, *current_selection_begin, *current_selection->rbegin());
	} else
	{
		first_changes = history.RegisterChanges(MODTYPE_UNSET, *current_selection_begin, *current_selection->rbegin());
	}
	if (first_changes >= 0)
	{
		greenzone.InvalidateAndCheck(first_changes);
		return true;
	} else
		return false;
}
bool EDITOR::InputColumnSetPattern(int joy, int button)
{
	if (joy < 0 || joy >= joysticks_per_frame[GetInputType(currMovieData)]) return false;

	SelectionFrames* current_selection = selection.MakeStrobe();
	if (current_selection->size() == 0) return false;
	SelectionFrames::iterator current_selection_begin(current_selection->begin());
	SelectionFrames::iterator current_selection_end(current_selection->end());
	int pattern_offset = 0, current_pattern = taseditor_config.current_pattern;

	for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
	{
		// skip lag frames
		if (taseditor_config.pattern_skips_lag && greenzone.laglog.GetLagInfoAtFrame(*it) == LAGGED_YES)
			continue;
		currMovieData.records[*it].setBitValue(joy, button, autofire_patterns[current_pattern][pattern_offset] != 0);
		pattern_offset++;
		if (pattern_offset >= (int)autofire_patterns[current_pattern].size())
			pattern_offset -= autofire_patterns[current_pattern].size();
	}
	int first_changes = history.RegisterChanges(MODTYPE_PATTERN, *current_selection_begin, *current_selection->rbegin(), 0, autofire_patterns_names[current_pattern].c_str());
	if (first_changes >= 0)
	{
		greenzone.InvalidateAndCheck(first_changes);
		return true;
	} else
		return false;
}
void EDITOR::SetMarkers()
{
	SelectionFrames* current_selection = selection.MakeStrobe();
	if (current_selection->size())
	{
		SelectionFrames::iterator current_selection_begin(current_selection->begin());
		SelectionFrames::iterator current_selection_end(current_selection->end());
		bool changes_made = false;
		for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
		{
			if (!markers_manager.GetMarker(*it))
			{
				if (markers_manager.SetMarker(*it))
				{
					changes_made = true;
					piano_roll.RedrawRow(*it);
				}
			}
		}
		if (changes_made)
		{
			selection.must_find_current_marker = playback.must_find_current_marker = true;
			history.RegisterMarkersChange(MODTYPE_MARKER_SET, *current_selection_begin, *current_selection->rbegin());
		}
	}
}
void EDITOR::RemoveMarkers()
{
	SelectionFrames* current_selection = selection.MakeStrobe();
	if (current_selection->size())
	{
		SelectionFrames::iterator current_selection_begin(current_selection->begin());
		SelectionFrames::iterator current_selection_end(current_selection->end());
		bool changes_made = false;
		for(SelectionFrames::iterator it(current_selection_begin); it != current_selection_end; it++)
		{
			if (markers_manager.GetMarker(*it))
			{
				markers_manager.ClearMarker(*it);
				changes_made = true;
				piano_roll.RedrawRow(*it);
			}
		}
		if (changes_made)
		{
			selection.must_find_current_marker = playback.must_find_current_marker = true;
			history.RegisterMarkersChange(MODTYPE_MARKER_REMOVE, *current_selection_begin, *current_selection->rbegin());
		}
	}
}
// ----------------------------------------------------------------------------------------------

