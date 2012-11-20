/* ---------------------------------------------------------------------------------
Implementation file of TASEDITOR_CONFIG class
Copyright (c) 2011-2012 AnS

(The MIT License)
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------------
Config - Current settings
[Singleton]

* stores current state of all TAS Editor settings
* all TAS Editor modules can get or set any data within Config
* when launching FCEUX, the emulator writes data from fceux.cfg file to the Config, when exiting it reads the data back to fceux.cfg
* stores resources: default values of all settings, min/max values of settings
------------------------------------------------------------------------------------ */

#include "../common.h"
#include "taseditor_config.h"

TASEDITOR_CONFIG::TASEDITOR_CONFIG()
{
	// set default values
	wndx = 0;
	wndy = 0;
	wndwidth = 0;
	wndheight = 0;
	saved_wndx = 0;
	saved_wndy = 0;
	saved_wndwidth = 0;
	saved_wndheight = 0;
	wndmaximized = false;
	findnote_wndx = 0;
	findnote_wndy = 0;
	follow_playback = true;
	turbo_seek = false;
	show_branch_screenshots = true;
	show_branch_descr = true;
	enable_hot_changes = true;
	jump_to_undo = true;
	follow_note_context = true;
	bind_markers = true;
	empty_marker_notes = true;
	combine_consecutive = false;
	use_1p_rec = true;
	columnset_by_keys = false;
	superimpose = 0;			// SUPERIMPOSE_UNCHECKED
	branch_full_movie = true;
	old_branching_controls = false;
	view_branches_tree = false;
	branch_scr_hud = true;
	restore_position = false;
	adjust_input_due_to_lag = true;
	greenzone_capacity = GREENZONE_CAPACITY_DEFAULT;
	undo_levels = UNDO_LEVELS_DEFAULT;
	autosave_period = AUTOSAVE_PERIOD_DEFAULT;
	last_export_type = 0;			// INPUT_TYPE_1P
	last_export_subtitles = false;
	savecompact_binary = true;
	savecompact_markers = true;
	savecompact_bookmarks = true;
	savecompact_greenzone = false;
	savecompact_history = false;
	savecompact_piano_roll = true;
	savecompact_selection = false;
	findnote_matchcase = false;
	findnote_search_up = false;
	enable_auto_function = true;
	draw_input = true;
	enable_greenzoning = true;
	silent_autosave = true;
	autopause_at_finish = true;
	tooltips = true;
	current_pattern = 0;
	pattern_skips_lag = true;
	pattern_recording = false;
	last_author[0] = 0;			// empty name

}




