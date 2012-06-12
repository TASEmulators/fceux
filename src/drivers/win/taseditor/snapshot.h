// Specification file for Snapshot class

enum Input_types
{
	INPUT_TYPE_1P,
	INPUT_TYPE_2P,
	INPUT_TYPE_FOURSCORE,

	NUM_SUPPORTED_INPUT_TYPES
};

#define BYTES_PER_JOYSTICK 1		// 1 byte per 1 joystick (8 buttons)
#define HOTCHANGE_BYTES_PER_JOY 4		// 4 bytes per 8 buttons

#define SNAPSHOT_DESC_MAX_LENGTH 100

class SNAPSHOT
{
public:
	SNAPSHOT();
	void init(MovieData& md, bool hotchanges, int force_input_type = -1);

	bool MarkersDifferFromCurrent(int end = -1);
	void copyToMarkers(int end = -1);
	MARKERS& GetMarkers();

	void toMovie(MovieData& md, int start = 0, int end = -1);

	void save(EMUFILE *os);
	bool load(EMUFILE *is);
	bool skipLoad(EMUFILE *is);

	void compress_data();
	bool Get_already_compressed();

	bool checkDiff(SNAPSHOT& snap);
	void fillJoypadsDiff(SNAPSHOT& snap, int frame);

	int findFirstChange(SNAPSHOT& snap, int start = 0, int end = -1);
	int findFirstChange(MovieData& md, int start = 0, int end = -1);

	int GetJoystickInfo(int frame, int joy);
	int GetCommandsInfo(int frame);

	void insertFrames(int at, int frames);
	void eraseFrame(int frame);

	void copyHotChanges(SNAPSHOT* source_of_hotchanges, int limit_frame_of_source = -1);
	void inheritHotChanges(SNAPSHOT* source_of_hotchanges);
	void inheritHotChanges_DeleteSelection(SNAPSHOT* source_of_hotchanges);
	void inheritHotChanges_InsertSelection(SNAPSHOT* source_of_hotchanges);
	void inheritHotChanges_DeleteNum(SNAPSHOT* source_of_hotchanges, int start, int frames);
	void inheritHotChanges_InsertNum(SNAPSHOT* source_of_hotchanges, int start, int frames);
	void inheritHotChanges_PasteInsert(SNAPSHOT* source_of_hotchanges, SelectionFrames& inserted_set);
	void fillHotChanges(SNAPSHOT& snap, int start = 0, int end = -1);

	void SetMaxHotChange_Bits(int frame, int joypad, uint8 joy_bits);
	void SetMaxHotChange(int frame, int absolute_button);

	void FadeHotChanges(int start_byte = 0, int end_byte = -1);

	int GetHotChangeInfo(int frame, int absolute_button);

	// saved data
	int size;							// in frames
	int input_type;						// theoretically TAS Editor can support any other input types
	int jump_frame;						// for jumping when making undo
	int start_frame;					// for consecutive Draws
	int end_frame;						// for consecutive Draws
	int consecutive_tag;				// for consecutive Recordings and Draws
	uint32 rec_joypad_diff_bits;		// for consecutive Recordings
	int mod_type;
	char description[SNAPSHOT_DESC_MAX_LENGTH];
	bool has_hot_changes;

	// not saved data
	std::vector<uint8> joysticks;		// Format: joy0-for-frame0, joy1-for-frame0, joy2-for-frame0, joy3-for-frame0, joy0-for-frame1, joy1-for-frame1, ...
	std::vector<uint8> commands;		// Format: commands-for-frame0, commands-for-frame1, ...
	std::vector<uint8> hot_changes;		// Format: buttons01joy0-for-frame0, buttons23joy0-for-frame0, buttons45joy0-for-frame0, buttons67joy0-for-frame0, buttons01joy1-for-frame0, ...

private:
	
	// also saved data
	MARKERS my_markers;
	std::vector<uint8> joysticks_compressed;
	std::vector<uint8> commands_compressed;
	std::vector<uint8> hot_changes_compressed;

	bool already_compressed;			// to compress only once
};

