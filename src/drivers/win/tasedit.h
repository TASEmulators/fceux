#define GREENZONE_CAPACITY_DEFAULT 10000
#define GREENZONE_CAPACITY_MIN 1
#define GREENZONE_CAPACITY_MAX 50000

#define UNDO_LEVELS_MIN 1
#define UNDO_LEVELS_MAX 999
#define UNDO_LEVELS_DEFAULT 100

#define AUTOSAVE_PERIOD_DEFAULT 10		// in minutes
#define AUTOSAVE_PERIOD_MIN 0			// 0 = no autosave
#define AUTOSAVE_PERIOD_MAX 60			// 1 hour

enum ECONTEXTMENU
{
	CONTEXTMENU_STRAY = 0,
	CONTEXTMENU_SELECTED = 1,
};

void EnterTasEdit();
void InitDialog();
bool ExitTasEdit();
void UpdateTasEdit();
void RedrawWindowCaption();
void RedrawTasedit();
void RedrawListAndBookmarks();
void RedrawRowAndBookmark(int index);
void ToggleJoypadBit(int column_index, int row_index, UINT KeyFlags);
void OpenProject();
bool SaveProject();
bool SaveProjectAs();
bool AskSaveProject();
void CloneFrames();
void InsertFrames();
void InsertNumFrames();
void DeleteFrames();
void ClearFrames(SelectionFrames* current_selection = 0);
void Truncate();
void ColumnSet(int column);
bool Copy(SelectionFrames* current_selection = 0);
void Cut();
bool Paste();
void GotFocus();
void LostFocus();

