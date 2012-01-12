enum
{
	MARKER_NOTE_EDIT_NONE, 
	MARKER_NOTE_EDIT_UPPER, 
	MARKER_NOTE_EDIT_LOWER
};

struct NewProjectParameters
{
	int input_type;
	bool copy_current_input;
	bool copy_current_markers;
	std::wstring author_name;
};

void SingleClick(LPNMITEMACTIVATE info);
void DoubleClick(LPNMITEMACTIVATE info);

void ToggleJoypadBit(int column_index, int row_index, UINT KeyFlags);
void ColumnSet(int column);
void InputColumnSet(int column);
void FrameColumnSet();

bool EnterTasEditor();
bool ExitTasEditor();
void UpdateTasEditor();

void NewProject();
void OpenProject();
bool LoadProject(char* fullname);
bool SaveProject();
bool SaveProjectAs();
void SaveCompact();
bool AskSaveProject();

void Import();
void Export();

int GetInputType(MovieData& md);
void SetInputType(MovieData& md, int new_input_type);

void SetTaseditInput();
void ClearTaseditInput();

void UpdateMarkerNote();

