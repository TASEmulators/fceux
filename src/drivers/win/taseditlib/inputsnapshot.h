//Specification file for Input Snapshot class

#include <time.h>

#define HOTCHANGE_BYTES_PER_JOY 4
#define SNAPSHOT_DESC_MAX_LENGTH 50

#define NUM_SUPPORTED_INPUT_TYPES 2
#define NORMAL_2JOYPADS 0
#define FOURSCORE 1

class INPUT_SNAPSHOT
{
public:
	INPUT_SNAPSHOT();
	void init(MovieData& md, bool hotchanges);

	void toMovie(MovieData& md, int start = 0, int end = -1);
	void toMarkers();
	void copyToMarkers(int end);

	void save(EMUFILE *os);
	bool load(EMUFILE *is);
	bool skipLoad(EMUFILE *is);

	bool checkDiff(INPUT_SNAPSHOT& inp);
	bool checkJoypadDiff(INPUT_SNAPSHOT& inp, int frame, int joy);

	bool checkMarkersDiff(INPUT_SNAPSHOT& inp);
	bool checkMarkersDiff();
	bool checkMarkersDiff(int end);

	int findFirstChange(INPUT_SNAPSHOT& inp, int start = 0, int end = -1);
	int findFirstChange(MovieData& md, int start = 0, int end = -1);

	void copyHotChanges(INPUT_SNAPSHOT* source_of_hotchanges, int limit_frame_of_source = -1);
	void inheritHotChanges(INPUT_SNAPSHOT* source_of_hotchanges);
	void inheritHotChanges_DeleteSelection(INPUT_SNAPSHOT* source_of_hotchanges);
	void inheritHotChanges_InsertSelection(INPUT_SNAPSHOT* source_of_hotchanges);
	void fillHotChanges(INPUT_SNAPSHOT& inp, int start = 0, int end = -1);

	void SetMaxHotChange_Bits(int frame, int joypad, uint8 joy_bits);
	void SetMaxHotChange(int frame, int absolute_button);

	void FadeHotChanges();

	int GetHotChangeInfo(int frame, int absolute_button);

	int size;							// in frames
	int input_type;						// 0=normal, 1=fourscore; theoretically TASEdit can support other input types, although some stuff may be unintentionally hardcoded
	std::vector<uint8> joysticks;		// Format: joy0-for-frame0, joy1-for-frame0, joy2-for-frame0, joy3-for-frame0, joy0-for-frame1, joy1-for-frame1, ...
	std::vector<uint8> commands;		// Format: commands-for-frame0, commands-for-frame1, ...
	std::vector<uint8> hot_changes;		// Format: buttons01joy0-for-frame0, buttons23joy0-for-frame0, buttons45joy0-for-frame0, buttons67joy0-for-frame0, buttons01joy1-for-frame0, ...
	std::vector<uint8> markers_array;	// just a copy of markers.markers_array

	bool coherent;						// indicates whether this state was made right after previous state
	int jump_frame;						// for jumping when making undo
	char description[SNAPSHOT_DESC_MAX_LENGTH];
	bool has_hot_changes;

private:
	void compress_data();

	bool already_compressed;			// to compress only once
	std::vector<uint8> joysticks_compressed;
	std::vector<uint8> commands_compressed;
	std::vector<uint8> hot_changes_compressed;
	std::vector<uint8> markers_array_compressed;

};

