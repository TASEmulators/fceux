//Implementation file of TASEDITOR_CONFIG class
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
	follow_playback = false;
	turbo_seek = false;
	show_lag_frames = true;
	show_markers = true;
	show_branch_screenshots = true;
	show_branch_descr = true;
	enable_hot_changes = true;
	jump_to_undo = true;
	follow_note_context = true;
	bind_markers = true;
	empty_marker_notes = true;
	combine_consecutive_rec = true;
	use_1p_rec = true;
	columnset_by_keys = false;
	keyboard_for_listview = true;
	superimpose = 0;			// SUPERIMPOSE_UNCHECKED
	superimpose_affects_paste = true;
	branch_full_movie = true;
	branch_only_when_rec = false;
	view_branches_tree = false;
	branch_scr_hud = true;
	restore_position = false;
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
	savecompact_list = true;
	savecompact_selection = false;
	findnote_matchcase = false;
	findnote_search_up = false;
	enable_auto_function = true;
	silent_autosave = true;
	tooltips = true;
	current_pattern = 0;
	pattern_skips_lag = false;
	pattern_recording = false;
	last_author[0] = 0;			// empty name

}




