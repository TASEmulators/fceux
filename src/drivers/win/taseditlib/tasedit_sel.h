//Specification file for TASEDIT_SELECTION class

#define SELECTION_ID_LEN 10

class TASEDIT_SELECTION
{
public:
	TASEDIT_SELECTION();
	void init(int new_size = 0);
	void free();
	void update();

	void save(EMUFILE *os);
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

	// getters
	int GetCurrentSelectionSize();
	int GetCurrentSelectionBeginning();
	bool CheckFrameSelected(int frame);
	SelectionFrames* MakeStrobe();
	SelectionFrames& GetStrobedSelection();

private:
	SelectionFrames& CurrentSelection();

	bool track_selection_changes;

	std::vector<SelectionFrames> selections_history;
	SelectionFrames clipboard_selection;

	int history_cursor_pos;
	int history_start_pos;
	int history_total_items;
	int history_size;

	SelectionFrames temp_selection;

};
