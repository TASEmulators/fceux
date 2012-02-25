// Specification file for History class

#define UNDO_HINT_TIME 200

enum
{
	MODTYPE_INIT,
	MODTYPE_CHANGE,		// deprecated
	MODTYPE_SET,
	MODTYPE_UNSET,
	MODTYPE_PATTERN,
	MODTYPE_INSERT,
	MODTYPE_DELETE,
	MODTYPE_TRUNCATE,
	MODTYPE_CLEAR,
	MODTYPE_CUT,
	MODTYPE_PASTE,
	MODTYPE_PASTEINSERT,
	MODTYPE_CLONE,
	MODTYPE_RECORD,
	MODTYPE_IMPORT,
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
	MODTYPE_MARKER_UNSET,
	MODTYPE_MARKER_PATTERN,
	MODTYPE_MARKER_RENAME,
	MODTYPE_MARKER_MOVE,
	MODTYPE_LUA_MARKER_SET,
	MODTYPE_LUA_MARKER_UNSET,
	MODTYPE_LUA_MARKER_RENAME,
	MODTYPE_LUA_CHANGE,

	MODTYPES_TOTAL
};
#define HISTORY_NORMAL_COLOR 0x000000

#define HISTORY_ID_LEN 8

class HISTORY
{
public:
	HISTORY();
	void init();
	void free();
	void reset();
	void update();		// called every frame

	void save(EMUFILE *os, bool really_save = true);
	bool load(EMUFILE *is);

	int undo();
	int redo();
	int jump(int new_pos);

	int RegisterChanges(int mod_type, int start = 0, int end =-1, const char* comment = 0);
	int RegisterPasteInsert(int start, SelectionFrames& inserted_set);
	void RegisterMarkersChange(int mod_type, int start = 0, int end =-1, const char* comment = 0);
	void RegisterBranching(int mod_type, int first_change, int slot);
	void RegisterRecording(int frame_of_change);
	void RegisterImport(MovieData& md, char* filename);
	int RegisterLuaChanges(const char* name, int start, bool InsertionDeletion_was_made);

	SNAPSHOT& GetCurrentSnapshot();
	SNAPSHOT& GetNextToCurrentSnapshot();
	char* GetItemDesc(int pos);
	int GetUndoHint();

	void GetDispInfo(NMLVDISPINFO* nmlvDispInfo);
	LONG CustomDraw(NMLVCUSTOMDRAW* msg);
	void Click(int row_index);

	void RedrawHistoryList();
	void UpdateHistoryList();

	HWND hwndHistoryList;

private:
	void AddSnapshotToHistory(SNAPSHOT &inp);

	std::vector<SNAPSHOT> snapshots;

	int history_cursor_pos;
	int history_start_pos;
	int history_total_items;
	int history_size;

	int undo_hint_pos, old_undo_hint_pos;
	int undo_hint_time;
	bool old_show_undo_hint, show_undo_hint;

};

