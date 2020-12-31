#ifndef HEADEREDITOR_H
#define HEADEREDITOR_H

struct iNES_HEADER;

extern HWND hHeadEditor;
void DoHeadEdit();

HWND InitHeaderEditDialog(HWND hwnd, iNES_HEADER* header);
bool ShowINESFileBox(HWND parent, char* buf = NULL, iNES_HEADER* header = NULL);
void ToggleINES20(HWND hwnd, bool ines20);
void ToggleExtendSystemList(HWND hwnd, bool enable);
void ToggleVSSystemGroup(HWND hwnd, bool enable);
void ToggleUnofficialPropertiesEnabled(HWND hwnd, bool ines20, bool check);
void ToggleUnofficialExtraRegionCode(HWND hwnd, bool ines20, bool unofficial_check, bool check);
void ToggleUnofficialPrgRamPresent(HWND hwnd, bool ines20, bool unofficial_check, bool check);
void SetHeaderData(HWND hwnd, iNES_HEADER* header);
bool LoadHeader(HWND parent, iNES_HEADER* header);
bool WriteHeaderData(HWND hwnd, iNES_HEADER* header = NULL);
int GetComboBoxByteSize(HWND hwnd, UINT id, int* value, iNES_HEADER* header);
bool SearchByString(HWND hwnd, UINT id, int* value, char* buf, iNES_HEADER* header = NULL);
bool GetComboBoxListItemData(HWND hwnd, UINT id, int* value, char* buf, iNES_HEADER* header = NULL);
bool SaveINESFile(HWND hwnd, char* path, iNES_HEADER* header);

INT_PTR CALLBACK HeaderEditorProc(HWND hDlg, UINT uMsg, WPARAM wP, LPARAM lP);
extern WNDPROC DefaultEditCtrlProc;
extern LRESULT APIENTRY FilterEditCtrlProc(HWND hwnd, UINT msg, WPARAM wP, LPARAM lP);

extern POINT CalcSubWindowPos(HWND hDlg, POINT* conf);
#endif
