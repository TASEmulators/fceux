#ifndef RAMWATCH_H
#define RAMWATCH_H
bool ResetWatches();
void OpenRWRecentFile(int memwRFileNumber);
extern bool AutoRWLoad;
extern bool RWSaveWindowPos;
#define MAX_RECENT_WATCHES 5
extern char rw_recent_files[MAX_RECENT_WATCHES][1024];
extern bool AskSaveRamWatch();
extern int ramw_x;
extern int ramw_y;
extern bool RWfileChanged;

//Constants
#define AUTORWLOAD "RamWatchAutoLoad"
#define RWSAVEPOS "RamWatchSaveWindowPos"
#define RAMWX "RamwX"
#define RAMWY "RamwY"

// Cache all the things required for drawing separator
// to prevent creating and destroying them repeatedly when painting
struct SeparatorCache
{
	// these things are static, every items are currently all the same.
	static HPEN sepPen; // color for separator in normal state
	static HPEN sepPenSel; // color for separator in selected state
	static HFONT sepFon; // caption font
	static int iHeight; // item height
	static int sepOffY; // y coordinate offset of the separator
	static void Init(HWND hBox); // prepare all the static stuff
	static void DeInit(); // destroy all the static stuff

	// there are only 2 thing that identical
	int labelOffY; // y coordinate offset of the label
	int sepOffX; // x coordinate offset of the separator

	SeparatorCache() {};
	SeparatorCache(HWND hWnd, char* text);
};

#define MAX_WATCH_COUNT 256
extern int WatchCount; // number of valid items in rswatches

extern char Watch_Dir[1024];

extern HWND RamWatchHWnd;
extern HACCEL RamWatchAccels;

// AddressWatcher is self-contained now
struct AddressWatcher
{
	unsigned int Address; // hardware address
	unsigned int CurValue;
	char* comment = NULL; // NULL means no comment, non-NULL means allocated comment
	bool WrongEndian;
	char Size; //'d' = 4 bytes, 'w' = 2 bytes, 'b' = 1 byte, and 'S' means it's a separator.
	char Type;//'s' = signed integer, 'u' = unsigned, 'h' = hex, 'b' = binary, 'S' = separator
	short Cheats; // how many bytes are affected by cheat
};

bool InsertWatch(const AddressWatcher& Watch);
bool InsertWatch(const AddressWatcher& Watch, HWND parent); // asks user for comment
bool InsertWatches(const AddressWatcher* watches, HWND parent, const int count);
bool InsertWatch(int watchIndex, const AddressWatcher& watcher);
bool EditWatch(int watchIndex, const AddressWatcher& watcher);
bool RemoveWatch(int watchIndex);

void Update_RAM_Watch();
bool Load_Watches(bool clear, const char* filename);
void RWAddRecentFile(const char *filename);
void UpdateWatchCheats();

INT_PTR CALLBACK RamWatchProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK EditWatchProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

extern HWND RamWatchHWnd;

#define WatchSizeConv(watch) (watch.Type == 'S' ? 0 : watch.Size == 'd' ? 4 : watch.Size == 'w' ? 2 : watch.Size == 'b' ? 1 : 0)
#define SizeConvWatch(size) (size == 4 ? 'd' : size == 2 ? 'w' : size == 1 : 'b' : 0)

#define GetDlgStoreIndex(parent) \
	(parent == RamWatchHWnd ? 0 : \
	parent == RamSearchHWnd ? 1 : \
	parent == hCheat ? 2 : 3)

#endif
