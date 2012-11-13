// Specification file for History class

#define UNDO_HINT_TIME 200						// in milliseconds

#define SAVING_HISTORY_PROGRESSBAR_UPDATE_RATE 10
#define TIME_BETWEEN_AUTOCOMPRESSIONS 500		// in milliseconds

#define HISTORY_LIST_WIDTH 500

enum MOD_TYPES
{
	MODTYPE_INIT,
	MODTYPE_UNDEFINED,
	MODTYPE_SET,
	MODTYPE_UNSET,
	MODTYPE_PATTERN,
	MODTYPE_INSERT,
	MODTYPE_INSERTNUM,
	MODTYPE_DELETE,
	MODTYPE_TRUNCATE,
	MODTYPE_CLEAR,
	MODTYPE_CUT,
	MODTYPE_PASTE,
	MODTYPE_PASTEINSERT,
	MODTYPE_CLONE,
	MODTYPE_RECORD,
	MODTYPE_IMPORT,
	MODTYPE_BOOKMARK_0,
	MODTYPE_BOOKMARK_1,
	MODTYPE_BOOKMARK_2,
	MODTYPE_BOOKMARK_3,
	MODTYPE_BOOKMARK_4,
	MODTYPE_BOOKMARK_5,
	MODTYPE_BOOKMARK_6,
	MODTYPE_BOOKMARK_7,
	MODTYPE_BOOKMARK_8,
	MODTYPE_BOOKMARK_9,
	MODTYPE_BRANCH_0,
	MODTYPE_BRANCH_1,
	MODTYPE_BRANCH_2,
	MODTYPE_BRANCH_3,
	MODTYPE_BRANCH_4,
	MODTYPE_BRANCH_5,
	MODTYPE_BRANCH_6,
	MODTYPE_BRANCH_7,
	MODTYPE_BRANCH_8,
	MODTYPE_BRANCH_9,
	MODTYPE_BRANCH_MARKERS_0,
	MODTYPE_BRANCH_MARKERS_1,
	MODTYPE_BRANCH_MARKERS_2,
	MODTYPE_BRANCH_MARKERS_3,
	MODTYPE_BRANCH_MARKERS_4,
	MODTYPE_BRANCH_MARKERS_5,
	MODTYPE_BRANCH_MARKERS_6,
	MODTYPE_BRANCH_MARKERS_7,
	MODTYPE_BRANCH_MARKERS_8,
	MODTYPE_BRANCH_MARKERS_9,
	MODTYPE_MARKER_SET,
	MODTYPE_MARKER_REMOVE,
	MODTYPE_MARKER_PATTERN,
	MODTYPE_MARKER_RENAME,
	MODTYPE_MARKER_DRAG,
	MODTYPE_MARKER_SWAP,
	MODTYPE_MARKER_SHIFT,
	MODTYPE_LUA_MARKER_SET,
	MODTYPE_LUA_MARKER_REMOVE,
	MODTYPE_LUA_MARKER_RENAME,
	MODTYPE_LUA_CHANGE,

	MODTYPES_TOTAL
};

enum CATEGORIES_OF_OPERATIONS
{
	CATEGORY_OTHER,
	CATEGORY_INPUT_CHANGE,
	CATEGORY_MARKERS_CHANGE,
	CATEGORY_INPUT_MARKERS_CHANGE,

	CATEGORIES_OF_OPERATIONS_TOTAL
};

#define HISTORY_NORMAL_COLOR 0x000000
#define HISTORY_NORMAL_BG_COLOR 0xFFFFFF
#define HISTORY_RELATED_BG_COLOR 0xF9DDE6

#define WM_MOUSEWHEEL_RESENT WM_APP+123

#define HISTORY_ID_LEN 8

class HISTORY
{
public:
	HISTORY();
	void init();
	void free();
	void reset();
	void update();		// called every frame

	void HistorySizeChanged();

	void save(EMUFILE *os, bool really_save = true);
	bool load(EMUFILE *is, unsigned int offset);

	void undo();
	void redo();

	int RegisterChanges(int mod_type, int start = 0, int end =-1, int size = 0, const char* comment = NULL, int consecutive_tag = 0, SelectionFrames* frameset = NULL);
	int RegisterAdjustLag(int start, int size);
	void RegisterMarkersChange(int mod_type, int start = 0, int end =-1, const char* comment = 0);

	void RegisterBookmarkSet(int slot, BOOKMARK& backup_copy, int old_current_branch);
	
	int RegisterBranching(int slot, bool markers_changed);
	void RegisterRecording(int frame_of_change);
	int RegisterImport(MovieData& md, char* filename);
	int RegisterLuaChanges(const char* name, int start, bool InsertionDeletion_was_made);

	int GetCategoryOfOperation(int mod_type);

	SNAPSHOT& GetCurrentSnapshot();
	SNAPSHOT& GetNextToCurrentSnapshot();
	char* GetItemDesc(int pos);
	int GetUndoHint();

	void GetDispInfo(NMLVDISPINFO* nmlvDispInfo);
	LONG CustomDraw(NMLVCUSTOMDRAW* msg);
	void Click(int row_index);

	void RedrawHistoryList();
	void UpdateHistoryList();

	bool CursorOverHistoryList();

	HWND hwndHistoryList;

private:
	int JumpInTime(int new_pos);

	void AddItemToHistory(SNAPSHOT &snap, int cur_branch = 0);
	void AddItemToHistory(SNAPSHOT &snap, int cur_branch, BOOKMARK &bookm);

	// saved variables
	std::vector<SNAPSHOT> snapshots;
	std::vector<BOOKMARK> backup_bookmarks;
	std::vector<int8> backup_current_branch;
	int history_cursor_pos;
	int history_total_items;

	// not saved variables
	int history_start_pos;
	int history_size;

	int undo_hint_pos, old_undo_hint_pos;
	int undo_hint_time;
	bool old_show_undo_hint, show_undo_hint;
	int next_autocompress_time;

};

