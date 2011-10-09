//Specification file for Input History and Input Snapshot classes

#include <time.h>

#define HOTCHANGE_BYTES_PER_JOY 4
#define SNAPSHOT_DESC_MAX_LENGTH 50

#define MODTYPE_INIT 0
#define MODTYPE_CHANGE 1
#define MODTYPE_SET 2
#define MODTYPE_UNSET 3
#define MODTYPE_INSERT 4
#define MODTYPE_DELETE 5
#define MODTYPE_TRUNCATE 6
#define MODTYPE_CLEAR 7
#define MODTYPE_CUT 8
#define MODTYPE_PASTE 9
#define MODTYPE_PASTEINSERT 10
#define MODTYPE_CLONE 11
#define MODTYPE_RECORD 12
#define MODTYPE_BRANCH_0 13
#define MODTYPE_BRANCH_1 14
#define MODTYPE_BRANCH_2 15
#define MODTYPE_BRANCH_3 16
#define MODTYPE_BRANCH_4 17
#define MODTYPE_BRANCH_5 18
#define MODTYPE_BRANCH_6 19
#define MODTYPE_BRANCH_7 20
#define MODTYPE_BRANCH_8 21
#define MODTYPE_BRANCH_9 22

class INPUT_SNAPSHOT
{
public:
	INPUT_SNAPSHOT();
	void init(MovieData& md);
	void toMovie(MovieData& md, int start = 0);

	void save(EMUFILE *os);
	bool load(EMUFILE *is);
	bool skipLoad(EMUFILE *is);

	bool checkDiff(INPUT_SNAPSHOT& inp);
	bool checkJoypadDiff(INPUT_SNAPSHOT& inp, int frame, int joy);
	int findFirstChange(INPUT_SNAPSHOT& inp, int start = 0, int end = -1);
	int findFirstChange(MovieData& md);

	void SetMaxHotChange(int frame, int absolute_button);
	int GetHotChangeInfo(int frame, int absolute_button);

	int size;							// in frames
	bool fourscore;
	std::vector<uint8> joysticks;		// Format: joy0-for-frame0, joy1-for-frame0, joy2-for-frame0, joy3-for-frame0, joy0-for-frame1, joy1-for-frame1, ...
	std::vector<uint8> hot_changes;		// Format: buttons01joy0-for-frame0, buttons23joy0-for-frame0, buttons45joy0-for-frame0, buttons67joy0-for-frame0, buttons01joy1-for-frame0, ...

	bool coherent;						// indicates whether this state was made by inputchange of previous state
	int jump_frame;						// for jumping when making undo
	char description[SNAPSHOT_DESC_MAX_LENGTH];

private:

};

class INPUT_HISTORY
{
public:
	INPUT_HISTORY();
	void init(int new_size);
	void free();

	void save(EMUFILE *os);
	void load(EMUFILE *is);

	int undo();
	int redo();
	int jump(int new_pos);

	void AddInputSnapshotToHistory(INPUT_SNAPSHOT &inp);

	int RegisterInputChanges(int mod_type, int start = 0, int end =-1);

	int InputChanged(int start, int end);
	int InputInserted(int start);
	int InputDeleted(int start);

	INPUT_SNAPSHOT& GetCurrentSnapshot();
	INPUT_SNAPSHOT& GetNextToCurrentSnapshot();
	int GetCursorPos();
	int GetTotalItems();
	char* GetItemDesc(int pos);
	bool GetItemCoherence(int pos);

private:
	std::vector<INPUT_SNAPSHOT> input_snapshots;

	int history_cursor_pos;
	int history_start_pos;
	int history_total_items;
	int history_size;

};

