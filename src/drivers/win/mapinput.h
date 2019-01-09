#ifndef WIN_MAPINPUT_h
#define WIN_MAPINPUT_h
char* GetKeyComboName(int c);
static int CALLBACK MapInputItemSortFunc(LPARAM lp1, LPARAM lp2, LPARAM lpSort);
void UpdateSortColumnIcon(HWND hwndListView, int sortColumn, bool sortAsc);
#endif
