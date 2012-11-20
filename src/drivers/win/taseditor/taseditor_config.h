// Specification file for TASEDITOR_CONFIG class

#define GREENZONE_CAPACITY_MIN 1
#define GREENZONE_CAPACITY_MAX 50000
#define GREENZONE_CAPACITY_DEFAULT 10000

#define UNDO_LEVELS_MIN 1
#define UNDO_LEVELS_MAX 1000
#define UNDO_LEVELS_DEFAULT 100

#define AUTOSAVE_PERIOD_MIN 0			// 0 = no autosave
#define AUTOSAVE_PERIOD_MAX 1440		// 24 hours
#define AUTOSAVE_PERIOD_DEFAULT 15		// in minutes

#define AUTHOR_MAX_LEN 100

class TASEDITOR_CONFIG
{
public:
	TASEDITOR_CONFIG();

	// vars saved in cfg file
	int wndx;
	int wndy;
	int wndwidth;
	int wndheight;
	int saved_wndx;
	int saved_wndy;
	int saved_wndwidth;
	int saved_wndheight;
	bool wndmaximized;
	int findnote_wndx;
	int findnote_wndy;
	bool follow_playback;
	bool turbo_seek;
	bool show_branch_screenshots;
	bool show_branch_descr;
	bool enable_hot_changes;
	bool jump_to_undo;
	bool follow_note_context;
	bool bind_markers;
	bool empty_marker_notes;
	bool combine_consecutive;
	bool use_1p_rec;
	bool columnset_by_keys;
	int superimpose;
	bool branch_full_movie;
	bool old_branching_controls;
	bool view_branches_tree;
	bool branch_scr_hud;
	bool restore_position;
	bool adjust_input_due_to_lag;
	int greenzone_capacity;
	int undo_levels;
	int autosave_period;
	int last_export_type;
	bool last_export_subtitles;
	bool savecompact_binary;
	bool savecompact_markers;
	bool savecompact_bookmarks;
	bool savecompact_greenzone;
	bool savecompact_history;
	bool savecompact_piano_roll;
	bool savecompact_selection;
	bool findnote_matchcase;
	bool findnote_search_up;
	bool enable_auto_function;
	bool draw_input;
	bool enable_greenzoning;
	bool silent_autosave;
	bool autopause_at_finish;
	bool tooltips;
	int current_pattern;
	bool pattern_skips_lag;
	bool pattern_recording;
	char last_author[AUTHOR_MAX_LEN];

private:

};
