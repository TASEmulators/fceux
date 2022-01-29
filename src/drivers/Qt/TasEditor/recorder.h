// Specification file for RECORDER class
#pragma once
#include <stdint.h>
#include <vector>

enum MULTITRACK_RECORDING_MODES
{
	MULTITRACK_RECORDING_ALL = 0,
	MULTITRACK_RECORDING_1P = 1,
	MULTITRACK_RECORDING_2P = 2,
	MULTITRACK_RECORDING_3P = 3,
	MULTITRACK_RECORDING_4P = 4,
};

enum SUPERIMPOSE_OPTIONS
{
	SUPERIMPOSE_UNCHECKED = 0,
	SUPERIMPOSE_CHECKED = 1,
	SUPERIMPOSE_INDETERMINATE = 2,
};

class RECORDER
{
public:
	RECORDER(void);
	void init(void);
	void reset(void);
	void update(void);

	void uncheckRecordingRadioButtons(void);
	void recheckRecordingRadioButtons(void);

	void recordInput(void);

	const char* getRecordingMode(void);
	const char* getRecordingCaption(void);
	
	int multitrackRecordingJoypadNumber;
	int patternOffset;
	std::vector<uint8_t> currentJoypadData;
	bool stateWasLoadedInReadWriteMode;

private:
	int oldMultitrackRecordingJoypadNumber;
	int oldCurrentPattern, oldPatternOffset;
	bool oldStateOfMovieReadonly;
	bool mustIncreasePatternOffset;

	//HWND hwndRecordingCheckbox, hwndRadioButtonRecordAll, hwndRadioButtonRecord1P, hwndRadioButtonRecord2P, hwndRadioButtonRecord3P, hwndRadioButtonRecord4P;

	// temps
	std::vector<uint8_t> oldJoyData;
	std::vector<uint8_t> newJoyData;
};
