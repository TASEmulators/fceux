// Specification file for Greenzone class

#include "laglog.h"

#define GREENZONE_ID_LEN 10

#define TIME_BETWEEN_CLEANINGS 10000	// in milliseconds
// Greenzone cleaning masks
#define EVERY16TH 0xFFFFFFF0
#define EVERY8TH 0xFFFFFFF8
#define EVERY4TH 0xFFFFFFFC
#define EVERY2ND 0xFFFFFFFE

#define PROGRESSBAR_UPDATE_RATE 1000	// progressbar is updated after every 1000 savestates loaded from fm3

class GREENZONE
{
public:
	GREENZONE();
	void init();
	void reset();
	void free();
	void update();

	void save(EMUFILE *os, bool really_save = true);
	bool load(EMUFILE *is, unsigned int offset);

	bool LoadSavestate(unsigned int frame);

	void RunGreenzoneCleaning();

	void UnGreenzoneSelectedFrames();

	void InvalidateAndCheck(int after);
	void Invalidate(int after);

	int FindBeginningOfGreenZone(int starting_index = 0);

	int GetSize();
	std::vector<uint8>& GetSavestate(int frame);
	void WriteSavestate(int frame, std::vector<uint8>& savestate);
	bool SavestateIsEmpty(unsigned int frame);

	// saved data
	LAGLOG laglog;

private:
	void CollectCurrentState();
	bool ClearSavestate(unsigned int index);
	bool ClearSavestateAndFreeMemory(unsigned int index);

	void AdjustUp();
	void AdjustDown();

	// saved data
	int greenZoneCount;
	std::vector<std::vector<uint8>> savestates;

	// not saved data
	int next_cleaning_time;
	
};
