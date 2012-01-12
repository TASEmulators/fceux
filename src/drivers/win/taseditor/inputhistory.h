//Specification file for Input History class
#define UNDO_HINT_TIME 200

enum
{
	MODTYPE_INIT = 0,
	MODTYPE_CHANGE = 1,	// deprecated
	MODTYPE_SET = 2,
	MODTYPE_UNSET = 3,
	MODTYPE_INSERT = 4,
	MODTYPE_DELETE = 5,
	MODTYPE_TRUNCATE = 6,
	MODTYPE_CLEAR = 7,
	MODTYPE_CUT = 8,
	MODTYPE_PASTE = 9,
	MODTYPE_PASTEINSERT = 10,
	MODTYPE_CLONE = 11,
	MODTYPE_RECORD = 12,
	MODTYPE_IMPORT = 13,
	MODTYPE_BRANCH_0 = 14,
	MODTYPE_BRANCH_1 = 15,
	MODTYPE_BRANCH_2 = 16,
	MODTYPE_BRANCH_3 = 17,
	MODTYPE_BRANCH_4 = 18,
	MODTYPE_BRANCH_5 = 19,
	MODTYPE_BRANCH_6 = 20,
	MODTYPE_BRANCH_7 = 21,
	MODTYPE_BRANCH_8 = 22,
	MODTYPE_BRANCH_9 = 23,
	MODTYPE_BRANCH_MARKERS_0 = 24,
	MODTYPE_BRANCH_MARKERS_1 = 25,
	MODTYPE_BRANCH_MARKERS_2 = 26,
	MODTYPE_BRANCH_MARKERS_3 = 27,
	MODTYPE_BRANCH_MARKERS_4 = 28,
	MODTYPE_BRANCH_MARKERS_5 = 29,
	MODTYPE_BRANCH_MARKERS_6 = 30,
	MODTYPE_BRANCH_MARKERS_7 = 31,
	MODTYPE_BRANCH_MARKERS_8 = 32,
	MODTYPE_BRANCH_MARKERS_9 = 33,
	MODTYPE_MARKER_SET = 34,
	MODTYPE_MARKER_UNSET = 35,
	MODTYPE_MARKER_RENAME = 36,
	MODTYPE_LUA_MARKER_SET = 37,
	MODTYPE_LUA_MARKER_UNSET = 38,
	MODTYPE_LUA_MARKER_RENAME = 39,
	MODTYPE_LUA_CHANGE = 40,
};
#define HISTORY_NORMAL_COLOR 0x000000
#define HISTORY_INCOHERENT_COLOR 0x999999

#define HISTORY_ID_LEN 8

class INPUT_HISTORY
{
public:
	INPUT_HISTORY();
	void init();
	void free();
	void reset();
	void update();		// called every frame

	void save(EMUFILE *os, bool really_save = true);
	bool load(EMUFILE *is);

	int undo();
	int redo();
	int jump(int new_pos);

	void AddInputSnapshotToHistory(INPUT_SNAPSHOT &inp);

	int RegisterChanges(int mod_type, int start = 0, int end =-1);
	void RegisterMarkersChange(int mod_type, int start = 0, int end =-1);
	void RegisterBranching(int mod_type, int first_change, int slot);
	void RegisterRecording(int frame_of_change);
	void RegisterImport(MovieData& md, char* filename);

	INPUT_SNAPSHOT& GetCurrentSnapshot();
	INPUT_SNAPSHOT& GetNextToCurrentSnapshot();
	char* GetItemDesc(int pos);
	bool GetItemCoherence(int pos);
	int GetUndoHint();

	void GetDispInfo(NMLVDISPINFO* nmlvDispInfo);
	LONG CustomDraw(NMLVCUSTOMDRAW* msg);
	void Click(LPNMITEMACTIVATE info);

	void RedrawHistoryList();
	void UpdateHistoryList();

	HWND hwndHistoryList;

private:
	std::vector<INPUT_SNAPSHOT> input_snapshots;

	int history_cursor_pos;
	int history_start_pos;
	int history_total_items;
	int history_size;

	int undo_hint_pos, old_undo_hint_pos;
	int undo_hint_time;
	bool old_show_undo_hint, show_undo_hint;

};

