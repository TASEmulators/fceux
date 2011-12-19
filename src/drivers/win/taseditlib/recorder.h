//Specification file for RECORDER class

#define MULTITRACK_RECORDING_ALL 0
#define MULTITRACK_RECORDING_1P 1
#define MULTITRACK_RECORDING_2P 2
#define MULTITRACK_RECORDING_3P 3
#define MULTITRACK_RECORDING_4P 4

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
	
	int multitrack_recording_joypad;
	std::vector<uint8> current_joy;

private:
	int old_multitrack_recording_joypad;
	bool old_movie_readonly;

	HWND hwndRecCheckbox, hwndRB_RecAll, hwndRB_Rec1P, hwndRB_Rec2P, hwndRB_Rec3P, hwndRB_Rec4P;

	// temps
	std::vector<uint8> old_joy;
	std::vector<uint8> new_joy;
};
