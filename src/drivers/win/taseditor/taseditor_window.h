// Specification file for TASEDITOR_WINDOW class

enum Window_items_enum
{
	WINDOWITEMS_PIANO_ROLL,
	WINDOWITEMS_PLAYBACK_MARKER,
	WINDOWITEMS_PLAYBACK_MARKER_EDIT,
	WINDOWITEMS_SELECTION_MARKER,
	WINDOWITEMS_SELECTION_MARKER_EDIT,
	WINDOWITEMS_PLAYBACK_BOX,
	WINDOWITEMS_PROGRESS_BUTTON,
	WINDOWITEMS_REWIND_FULL,
	WINDOWITEMS_REWIND,
	WINDOWITEMS_PAUSE,
	WINDOWITEMS_FORWARD,
	WINDOWITEMS_FORWARD_FULL,
	WINDOWITEMS_PROGRESS_BAR,
	WINDOWITEMS_FOLLOW_CURSOR,
	WINDOWITEMS_TURBO_SEEK,
	WINDOWITEMS_AUTORESTORE_PLAYBACK,
	WINDOWITEMS_RECORDER_BOX,
	WINDOWITEMS_RECORDING,
	WINDOWITEMS_RECORD_ALL,
	WINDOWITEMS_RECORD_1P,
	WINDOWITEMS_RECORD_2P,
	WINDOWITEMS_RECORD_3P,
	WINDOWITEMS_RECORD_4P,
	WINDOWITEMS_SUPERIMPOSE,
	WINDOWITEMS_USE_PATTERN,
	WINDOWITEMS_SPLICER_BOX,
	WINDOWITEMS_SELECTION_TEXT,
	WINDOWITEMS_CLIPBOARD_TEXT,
	WINDOWITEMS_LUA_BOX,
	WINDOWITEMS_RUN_MANUAL,
	WINDOWITEMS_RUN_AUTO,
	WINDOWITEMS_BRANCHES_BUTTON,
	WINDOWITEMS_BOOKMARKS_BOX,
	WINDOWITEMS_BOOKMARKS_LIST,
	WINDOWITEMS_BRANCHES_BITMAP,
	WINDOWITEMS_HISTORY_BOX,
	WINDOWITEMS_HISTORY_LIST,
	WINDOWITEMS_PREVIOUS_MARKER,
	WINDOWITEMS_SIMILAR,
	WINDOWITEMS_MORE,
	WINDOWITEMS_NEXT_MARKER,
	// ---
	TASEDITOR_WINDOW_TOTAL_ITEMS
};


#define TOOLTIP_TEXT_MAX_LEN 127
#define TOOLTIPS_AUTOPOP_TIMEOUT 30000

#define PATTERNS_MENU_POS 5
#define PATTERNS_MAX_VISIBLE_NAME 50

struct Window_items_struct
{
	int number;
	int id;
	int x;
	int y;
	int width;
	int height;
	char tooltip_text_base[TOOLTIP_TEXT_MAX_LEN];
	char tooltip_text[TOOLTIP_TEXT_MAX_LEN];
	bool static_rect;
	int hotkey_emucmd;
	HWND tooltip_hwnd;
};

class TASEDITOR_WINDOW
{
public:
	TASEDITOR_WINDOW();
	void init();
	void exit();
	void reset();
	void update();

	void ResizeItems();
	void WindowMovedOrResized();
	void ChangeBookmarksListHeight(int new_height);

	void UpdateTooltips();

	void UpdateCaption();
	void RedrawTaseditor();

	void UpdateCheckedItems();

	void UpdateRecentProjectsMenu();
	void UpdateRecentProjectsArray(const char* addString);
	void RemoveRecentProject(unsigned int which);
	void LoadRecentProject(int slot);

	void UpdatePatternsMenu();
	void RecheckPatternsMenu();

	HWND hwndTasEditor, hwndFindNote;
	bool TASEditor_focus;
	bool ready_for_resizing;
	int min_width;
	int min_height;

	bool must_update_mouse_cursor;

private:
	void CalculateItems();


	HWND hToolTipWnd;
	HMENU hmenu, patterns_menu;
	HICON hTaseditorIcon;

};
