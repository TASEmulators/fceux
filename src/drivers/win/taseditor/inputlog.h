// Specification file for InputLog class

enum Input_types
{
	INPUT_TYPE_1P,
	INPUT_TYPE_2P,
	INPUT_TYPE_FOURSCORE,

	NUM_SUPPORTED_INPUT_TYPES
};

#define BUTTONS_PER_JOYSTICK 8
#define BYTES_PER_JOYSTICK 1			// 1 byte per 1 joystick (8 buttons)

#define HOTCHANGE_BITS_PER_VALUE 4		// any HotChange value takes 4 bits
#define HOTCHANGE_BITMASK 0xF			// "1111"
#define HOTCHANGE_MAX_VALUE 0xF			// "1111" max
#define HOTCHANGE_VALUES_PER_BYTE 2		// hence 2 HotChange values fit into 1 byte
#define BYTE_VALUE_CONTAINING_MAX_HOTCHANGES ((HOTCHANGE_MAX_VALUE << HOTCHANGE_BITS_PER_VALUE) | HOTCHANGE_MAX_VALUE)	// "0xFF"
#define BYTE_VALUE_CONTAINING_MAX_HOTCHANGE_HI (HOTCHANGE_MAX_VALUE << HOTCHANGE_BITS_PER_VALUE)						// "0xF0"
#define BYTE_VALUE_CONTAINING_MAX_HOTCHANGE_LO HOTCHANGE_MAX_VALUE														// "0x0F"
#define HOTCHANGE_BYTES_PER_JOY (BYTES_PER_JOYSTICK * HOTCHANGE_BITS_PER_VALUE)	// 4 bytes per 8 buttons

class INPUTLOG
{
public:
	INPUTLOG();
	void init(MovieData& md, bool hotchanges, int force_input_type = -1);
	void toMovie(MovieData& md, int start = 0, int end = -1);

	void save(EMUFILE *os);
	bool load(EMUFILE *is);
	bool skipLoad(EMUFILE *is);

	void compress_data();
	bool Get_already_compressed();

	uint32 fillJoypadsDiff(INPUTLOG& their_log, int frame);
	int findFirstChange(INPUTLOG& their_log, int start = 0, int end = -1);
	int findFirstChange(MovieData& md, int start = 0, int end = -1);

	int GetJoystickInfo(int frame, int joy);
	int GetCommandsInfo(int frame);

	void insertFrames(int at, int frames);
	void eraseFrame(int frame);

	void Init_HotChanges();

	void copyHotChanges(INPUTLOG* source_of_hotchanges, int limit_frame_of_source = -1);
	void inheritHotChanges(INPUTLOG* source_of_hotchanges);
	void inheritHotChanges_DeleteSelection(INPUTLOG* source_of_hotchanges, SelectionFrames* frameset);
	void inheritHotChanges_InsertSelection(INPUTLOG* source_of_hotchanges, SelectionFrames* frameset);
	void inheritHotChanges_DeleteNum(INPUTLOG* source_of_hotchanges, int start, int frames, bool fade_old);
	void inheritHotChanges_InsertNum(INPUTLOG* source_of_hotchanges, int start, int frames, bool fade_old);
	void inheritHotChanges_PasteInsert(INPUTLOG* source_of_hotchanges, SelectionFrames* inserted_set);
	void fillHotChanges(INPUTLOG& their_log, int start = 0, int end = -1);

	void SetMaxHotChange_Bits(int frame, int joypad, uint8 joy_bits);
	void SetMaxHotChange(int frame, int absolute_button);

	void FadeHotChanges(int start_byte = 0, int end_byte = -1);

	int GetHotChangeInfo(int frame, int absolute_button);

	// saved data
	int size;						// in frames
	int input_type;						// theoretically TAS Editor can support any other Input types
	bool has_hot_changes;

	// not saved data
	std::vector<uint8> hot_changes;		// Format: buttons01joy0-for-frame0, buttons23joy0-for-frame0, buttons45joy0-for-frame0, buttons67joy0-for-frame0, buttons01joy1-for-frame0, ...
	std::vector<uint8> joysticks;		// Format: joy0-for-frame0, joy1-for-frame0, joy2-for-frame0, joy3-for-frame0, joy0-for-frame1, joy1-for-frame1, ...
	std::vector<uint8> commands;		// Format: commands-for-frame0, commands-for-frame1, ...

private:
	
	// also saved data
	std::vector<uint8> joysticks_compressed;
	std::vector<uint8> commands_compressed;
	std::vector<uint8> hot_changes_compressed;

	// not saved data
	bool already_compressed;			// to compress only once
};

