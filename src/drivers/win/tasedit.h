#define GREENZONE_CAPACITY_MIN 1
#define GREENZONE_CAPACITY_MAX 50000
#define GREENZONE_CAPACITY_DEFAULT 10000

#define UNDO_LEVELS_MIN 1
#define UNDO_LEVELS_MAX 999
#define UNDO_LEVELS_DEFAULT 99

#define AUTOSAVE_PERIOD_MIN 0			// 0 = no autosave
#define AUTOSAVE_PERIOD_MAX 60			// 1 hour
#define AUTOSAVE_PERIOD_DEFAULT 10		// in minutes

#define EXPORT_TYPE_1P 0
#define EXPORT_TYPE_2P 1
#define EXPORT_TYPE_FOURSCORE 2

enum ECONTEXTMENU
{
	CONTEXTMENU_STRAY = 0,
	CONTEXTMENU_SELECTED = 1,
};

bool EnterTasEdit();
void InitDialog();
bool ExitTasEdit();
void UpdateTasEdit();
void RedrawTasedit();
void RedrawWindowCaption();
void ToggleJoypadBit(int column_index, int row_index, UINT KeyFlags);
void OpenProject();
bool LoadProject(char* fullname);
void LoadRecentProject(int slot);
bool SaveProject();
bool SaveProjectAs();
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
void GotFocus();
void LostFocus();

void UpdateRecentProjectsMenu();
void UpdateRecentProjectsArray(const char* addString);
void RemoveRecentProject(unsigned int which);
