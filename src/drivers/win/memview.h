#ifndef MEMVIEW_H
#define MEMVIEW_H

void DoMemView();
void KillMemView();
void UpdateMemoryView(int draw_all);
void UpdateColorTable();
void ChangeMemViewFocus(int newEditingMode, int StartOffset,int EndOffset);
void UpdateCaption();

void ApplyPatch(int addr,int size, uint8* data);
void UndoLastPatch();

void SetHexEditorAddress(int gotoaddress);

int GetMaxSize(int editingMode);

extern HWND hMemView, hMemFind;
extern int EditingMode;

extern char* EditString[4];

#endif