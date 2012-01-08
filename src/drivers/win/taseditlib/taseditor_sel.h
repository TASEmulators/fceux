//Specification file for TASEDITOR_SELECTION class

#define SELECTION_ID_LEN 10

class TASEDITOR_SELECTION
{
public:
	TASEDITOR_SELECTION();
	void init();
	void free();
	void reset();
	void reset_vars();
	void update();

	void RedrawTextClipboard();
	void RedrawMarker();

	void save(EMUFILE *os, bool really_save = true);
	bool load(EMUFILE *is);
	void saveSelection(SelectionFrames& selection, EMUFILE *os);
	bool loadSelection(SelectionFrames& selection, EMUFILE *is);
	bool skiploadSelection(EMUFILE *is);

	void ItemRangeChanged(NMLVODSTATECHANGE* info);
	void ItemChanged(NMLISTVIEW* info);

	void AddNewSelectionToHistory();

	void undo();
	void redo();
	void jump(int new_pos);

	void MemorizeClipboardSelection();
	void ReselectClipboard();

	void ClearSelection();
	void ClearRowSelection(int index);

	void EnforceSelectionToList();

	void SelectAll();
	void SetRowSelection(int index);
	void SetRegionSelection(int start, int end);
	void SelectMidMarkers();

	void JumpPrevMarker();
	void JumpNextMarker();
	void JumpToFrame(int frame);

	// getters
	int GetCurrentSelectionSize();
	int GetCurrentSelectionBeginning();
	bool CheckFrameSelected(int frame);
	SelectionFrames* MakeStrobe();
	SelectionFrames& GetStrobedSelection();

	SelectionFrames& GetInsertedSet();

	bool must_find_current_marker;
	int shown_marker;

	HWND hwndPrevMarker, hwndNextMarker;
	HWND hwndTextSelection, hwndTextClipboard;
	HWND hwndSelectionMarker, hwndSelectionMarkerEdit;

private:
	SelectionFrames& CurrentSelection();
	void CheckClipboard();

	bool track_selection_changes;
	bool must_redraw_text;

	int last_selection_beginning;

	bool old_prev_marker_button_state, prev_marker_button_state;
	bool old_next_marker_button_state, next_marker_button_state;
	int button_hold_time;

	std::vector<SelectionFrames> selections_history;
	SelectionFrames clipboard_selection;

	SelectionFrames inserted_set;

	int history_cursor_pos;
	int history_start_pos;
	int history_total_items;
	int history_size;

	SelectionFrames temp_selection;

};
