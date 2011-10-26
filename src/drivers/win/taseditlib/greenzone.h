//Specification file for Greenzone class

//#define LAG_FLAG_BIT 1
#define TIME_BETWEEN_CLEANINGS 10000	// in milliseconds

#define GREENZONE_ID_LEN 10

class GREENZONE
{
public:
	GREENZONE();
	void init();
	void reset();
	void update();

	void save(EMUFILE *os);
	bool load(EMUFILE *is);

	void clearGreenzone();
	void TryDumpIncremental(bool lagFlag = true);

	bool loadTasSavestate(int frame);
	void storeTasSavestate(int frame);

	void GreenzoneCleaning();
	void ClearSavestate(int index);

	void InvalidateAndCheck(int after);
	void Invalidate(int after);

	int FindBeginningOfGreenZone(int starting_index = 0);

	// data
	int greenZoneCount;
	std::vector<std::vector<uint8>> savestates;
	std::vector<uint8> lag_history;

private:
	int next_cleaning_time;

};
