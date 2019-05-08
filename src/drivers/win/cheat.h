//--
//mbg merge 7/18/06 had to make these extern
extern int CheatWindow,CheatStyle; //bbit edited: this line added
extern HWND hCheat;

HWND InitializeCheatList(HWND hwndDlg);
void RedoCheatsLB(HWND hwndDlg);

typedef unsigned int HWAddressType;


void ConfigCheats(HWND hParent);
void DoGGConv();
void SetGGConvFocus(int address,int compare);
void UpdateCheatList();
void UpdateCheatListGroupBoxUI();
void UpdateCheatsAdded();

extern unsigned int FrozenAddressCount;
extern std::vector<uint16> FrozenAddresses;
//void ConfigAddCheat(HWND wnd); //bbit edited:commented out this line

void DisableAllCheats();

void UpdateCheatWindowRelatedWindow();

// deselect the old one and select the new one
#define ListView_MoveSelectionMark(hwnd, prevIndex, newIndex) \
LVITEM lvi; \
SendMessage(hwnd, LVM_SETITEMSTATE, prevIndex, (LPARAM)&(lvi.mask = LVIF_STATE, lvi.stateMask = LVIS_SELECTED, lvi.state = 0, lvi)), \
SendMessage(hwnd, LVM_SETITEMSTATE, newIndex, (LPARAM)&(lvi.state = LVIS_SELECTED, lvi)), \
SendMessage(hwnd, LVM_SETSELECTIONMARK, 0, newIndex)

#define ClearCheatListText(hwnd) \
(SetDlgItemText(hwnd, IDC_CHEAT_ADDR, (LPTSTR)"") & \
SetDlgItemText(hwnd, IDC_CHEAT_VAL, (LPTSTR)"") & \
SetDlgItemText(hwnd, IDC_CHEAT_COM, (LPTSTR)"") & \
SetDlgItemText(hwnd, IDC_CHEAT_NAME, (LPTSTR)""))

