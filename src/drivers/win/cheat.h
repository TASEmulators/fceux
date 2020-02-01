//--
//mbg merge 7/18/06 had to make these extern
#ifndef WIN_CHEAT_H
#define WIN_CHEAT_H
extern int CheatWindow,CheatStyle; //bbit edited: this line added
extern HWND hCheat;

HWND InitializeCheatList(HWND hwndDlg);
void RedoCheatsLB(HWND hwndDlg);

typedef unsigned int HWAddressType;


void ConfigCheats(HWND hParent);
void DoGGConv();
void SetGGConvFocus(int address, int compare);
void UpdateCheatList();
void UpdateCheatListGroupBoxUI();
void UpdateCheatsAdded();
void ToggleCheatInputMode(HWND hwndDlg, int modeId);
void GetUICheatInfo(HWND hwndDlg, char* name, uint32* a, uint8* v, int* c);
void GetUIGGInfo(HWND hwndDlg, uint32* a, uint8* v, int* c);
inline void GetCheatStr(char* buf, int a, int v, int c);
inline void GetCheatCodeStr(char* buf, int a, int v, int c);
static void SetCheatToolTip(HWND hwndDlg, UINT id);
char* GetCheatToolTipStr(HWND hwndDlg, UINT id);

extern unsigned int FrozenAddressCount;
//void ConfigAddCheat(HWND wnd); //bbit edited:commented out this line
extern struct CHEATF* cheats;
extern char* GameGenieLetters;

void DisableAllCheats();
bool ShowCheatFileBox(HWND hwnd, char* buf, bool save = false);
void AskSaveCheat();
void SaveCheatAs(HWND hwnd, bool flush = false);

void UpdateCheatRelatedWindow();
void SetupCheatFont(HWND hDlg);
void DeleteCheatFont();
extern POINT CalcSubWindowPos(HWND hDlg, POINT* conf);

void CreateCheatMap();
void ReleaseCheatMap();
extern int CheatMapUsers;

extern INT_PTR CALLBACK GGConvCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern LRESULT APIENTRY FilterEditCtrlProc(HWND hDlg, UINT msg, WPARAM wP, LPARAM lP);
extern WNDPROC DefaultEditCtrlProc;


// deselect the old one and select the new one
#define ListView_MoveSelectionMark(hwnd, prevIndex, newIndex) \
LVITEM lvi; \
SendMessage(hwnd, LVM_SETITEMSTATE, prevIndex, (LPARAM)&(lvi.mask = LVIF_STATE, lvi.stateMask = LVIS_SELECTED, lvi.state = 0, lvi)), \
SendMessage(hwnd, LVM_SETITEMSTATE, newIndex, (LPARAM)&(lvi.state = LVIS_SELECTED, lvi)), \
SendMessage(hwnd, LVM_SETSELECTIONMARK, 0, newIndex)

#define ClearCheatListText(hwnd) \
(SetDlgItemText(hwnd, IDC_CHEAT_ADDR, (LPCSTR)"") & \
SetDlgItemText(hwnd, IDC_CHEAT_VAL, (LPCSTR)"") & \
SetDlgItemText(hwnd, IDC_CHEAT_COM, (LPCSTR)"") & \
SetDlgItemText(hwnd, IDC_CHEAT_NAME, (LPCSTR)"") & \
SetDlgItemText(hwnd, IDC_CHEAT_TEXT, (LPCSTR)"") & \
SetDlgItemText(hwnd, IDC_CHEAT_GAME_GENIE_TEXT, (LPCSTR)""))

#endif