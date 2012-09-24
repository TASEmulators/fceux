// Specification file for RECORDER class

enum
{
	MULTITRACK_RECORDING_ALL = 0,
	MULTITRACK_RECORDING_1P = 1,
	MULTITRACK_RECORDING_2P = 2,
	MULTITRACK_RECORDING_3P = 3,
	MULTITRACK_RECORDING_4P = 4,
};

enum
{
	SUPERIMPOSE_UNCHECKED = 0,
	SUPERIMPOSE_CHECKED = 1,
	SUPERIMPOSE_INDETERMINATE = 2,
};

class RECORDER
{
public:
	RECORDER();
	void init();
	void reset();
	void update();

	void UncheckRecordingRadioButtons();
	void RecheckRecordingRadioButtons();

	void InputChanged();

	const char* GetRecordingMode();
	const char* GetRecordingCaption();
	
	int multitrack_recording_joypad;
	int pattern_offset;
	std::vector<uint8> current_joy;
	bool state_was_loaded_in_readwrite_mode;

private:
	int old_multitrack_recording_joypad;
	int old_current_pattern, old_pattern_offset;
	bool must_increase_pattern_offset;
	bool old_movie_readonly;

	HWND hwndRecCheckbox, hwndRB_RecAll, hwndRB_Rec1P, hwndRB_Rec2P, hwndRB_Rec3P, hwndRB_Rec4P;

	// temps
	std::vector<uint8> old_joy;
	std::vector<uint8> new_joy;
};
