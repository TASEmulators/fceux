// Specification file for SELECTION class

#include <set>
typedef std::set<int> SelectionFrames;

#define SELECTION_ID_LEN 10

class SELECTION
{
public:
	SELECTION();
	void init();
	void free();
	void reset();
	void reset_vars();
	void update();

	void UpdateSelectionSize();

	void HistorySizeChanged();

	void RedrawMarker();

	void save(EMUFILE *os, bool really_save = true);
	bool load(EMUFILE *is, unsigned int offset);
	void saveSelection(SelectionFrames& selection, EMUFILE *os);
	bool loadSelection(SelectionFrames& selection, EMUFILE *is);
	bool skiploadSelection(EMUFILE *is);

	void ItemRangeChanged(NMLVODSTATECHANGE* info);
	void ItemChanged(NMLISTVIEW* info);

	void AddNewSelectionToHistory();
	void AddCurrentSelectionToHistory();

	void undo();
	void redo();

	bool GetRowSelection(int index);

	void ClearSelection();
	void ClearRowSelection(int index);
	void ClearRegionSelection(int start, int end);

	void SelectAll();
	void SetRowSelection(int index);
	void SetRegionSelection(int start, int end);

	void SetRegionSelectionPattern(int start, int end);
	void SelectBetweenMarkers();
	void ReselectClipboard();

	void Transpose(int shift);

	void JumpPrevMarker(int speed = 1);
	void JumpNextMarker(int speed = 1);
	void JumpToFrame(int frame);

	// getters
	int GetCurrentSelectionSize();
	int GetCurrentSelectionBeginning();
	int GetCurrentSelectionEnd();
	bool CheckFrameSelected(int frame);
	SelectionFrames* MakeStrobe();

	bool must_find_current_marker;
	int shown_marker;

	HWND hwndPrevMarker, hwndNextMarker;
	HWND hwndSelectionMarker, hwndSelectionMarkerEdit;

private:
	void JumpInTime(int new_pos);

	void EnforceSelectionToList();

	SelectionFrames& CurrentSelection();

	bool track_selection_changes;
	int last_selection_beginning;

	bool old_prev_marker_button_state, prev_marker_button_state;
	bool old_next_marker_button_state, next_marker_button_state;
	int button_hold_time;

	std::vector<SelectionFrames> selections_history;

	int history_cursor_pos;
	int history_start_pos;
	int history_total_items;
	int history_size;

	SelectionFrames temp_selection;

};
