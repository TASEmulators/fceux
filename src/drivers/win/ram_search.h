#ifndef RAM_SEARCH_H
#define RAM_SEARCH_H


extern char rs_type_size;
extern int ResultCount;

unsigned int sizeConv(unsigned int index,char size, char *prevSize = &rs_type_size, bool usePrev = false);
unsigned int GetRamValue(unsigned int Addr,char Size);
void prune(char Search, char Operater, char Type, int Value, int OperatorParameter);
void CompactAddrs();
void reset_address_info();
void signal_new_frame();
void signal_new_size();
void UpdateRamSearchTitleBar(int percent = 0);
void SetRamSearchUndoType(HWND hDlg, int type);
unsigned int ReadValueAtHardwareAddress(HWAddressType address, unsigned int size);
bool WriteValueAtHardwareAddress(HWAddressType address, unsigned int value, unsigned int size);
bool IsHardwareAddressValid(HWAddressType address);
extern int curr_ram_size;
extern bool noMisalign;

void ResetResults();
void CloseRamWindows(); //Close the Ram Search & Watch windows when rom closes
void ReopenRamWindows(); //Reopen them when a new Rom is loaded
void Update_RAM_Search(); //keeps RAM values up to date in the search and watch windows

void SetSearchType(int SearchType); //Set the search type
void DoRamSearchOperation(); //Perform a search

extern HWND RamSearchHWnd;
extern INT_PTR CALLBACK RamSearchProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern POINT CalcSubWindowPos(HWND hDlg, POINT* conf);

// Too much work to do for resorting the values, and finding the biggest number 
// by sorting in ram list doesn't help too much in usually use, so I gave it up. 
// static int CALLBACK RamSearchItemSortFunc(LPARAM lp1, LPARAM lp2, LPARAM lpSort);
// extern void UpdateSortColumnIcon(HWND hwndListView, int sortColumn, bool sortAsc);

#define CHEAT_1BYTE_BG RGB(216, 203, 253)
#define CHEAT_2BYTE_BG RGB(195, 186, 253)
#define CHEAT_3BYTE_BG RGB(176, 139, 252)
#define CHEAT_4BYTE_BG RGB(175, 94, 253)
#define CHEAT_4BYTE_TEXT RGB(255, 255, 255)

#endif
