#define MARKER_NOTE_EDIT_NONE 0
#define MARKER_NOTE_EDIT_UPPER 1
#define MARKER_NOTE_EDIT_LOWER 2

void SingleClick(LPNMITEMACTIVATE info);
void DoubleClick(LPNMITEMACTIVATE info);
bool EnterTasEdit();
void InitDialog();
bool ExitTasEdit();
void UpdateTasEdit();
void ToggleJoypadBit(int column_index, int row_index, UINT KeyFlags);
void OpenProject();
bool LoadProject(char* fullname);
bool SaveProject();
bool SaveProjectAs();
void SaveCompact();
bool AskSaveProject();
void Import();
void Export();
void CloneFrames();
void InsertFrames();
void InsertNumFrames();
void DeleteFrames();
void ClearFrames(SelectionFrames* current_selection = 0);
void Truncate();
void ColumnSet(int column);
void InputColumnSet(int column);
void FrameColumnSet();
bool Copy(SelectionFrames* current_selection = 0);
void Cut();
bool Paste();
bool PasteInsert();

void SetTaseditInput();
void ClearTaseditInput();

void UpdateMarkerNote();

