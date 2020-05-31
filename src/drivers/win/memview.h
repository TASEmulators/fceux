void DoMemView();
void KillMemView();
void UpdateMemoryView(int draw_all);
void UpdateColorTable();
void ChangeMemViewFocus(int newEditingMode, int StartOffset,int EndOffset);
void UpdateCaption();
/*
int saveBookmarks(HWND hwnd);
int loadBookmarks(HWND hwnd);
*/
void ApplyPatch(int addr,int size, uint8* data);
void UndoLastPatch();

void SetHexEditorAddress(int gotoaddress);

extern HWND hMemView, hMemFind;
extern int EditingMode;

extern char EditString[4][20];