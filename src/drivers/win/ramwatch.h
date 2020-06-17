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
	char* comment; // NULL means no comment, non-NULL means allocated comment
	bool WrongEndian;
	char Size; //'d' = 4 bytes, 'w' = 2 bytes, 'b' = 1 byte, and 'S' means it's a separator.
	char Type;//'s' = signed integer, 'u' = unsigned, 'h' = hex, 'b' = binary, 'S' = separator
	short Cheats; // how many bytes are affected by cheat

	AddressWatcher(void)
	{
		Address = 0; CurValue = 0; comment = NULL; WrongEndian = false;
		Size = 'b'; Type = 's'; Cheats = 0;
	}
};

// the struct for communicating with add watch window
#define WATCHER_MSG_ADD 0
#define WATCHER_MSG_EDIT 1
#define WATCHER_MSG_DUP 2
struct WatcherMsg {
	int msg = WATCHER_MSG_ADD;
	int count = 0; // how many addresses are there
	unsigned int* Addresses = NULL; // Address list
	char* comment = NULL;
	bool WrongEndian;
	char Size;
	char Type;

	AddressWatcher* ToAddressWatches(int* _count = NULL);
	static WatcherMsg FromAddressWatches(const AddressWatcher* watches, int count = 1);
};



bool InsertWatch(const AddressWatcher& Watch);
bool InsertWatch(const AddressWatcher& Watch, HWND parent); // asks user for comment
bool InsertWatches(WatcherMsg* msg, HWND parent, int count);
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
