//Specification file for Greenzone class

//#define LAG_FLAG_BIT 1

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
	void ClearGreenzoneTail();
	void ClearSavestate(int index);

	void InvalidateGreenZone(int after);

	int FindBeginningOfGreenZone(int starting_index = 0);

	// data
	int greenZoneCount;
	std::vector<std::vector<uint8>> savestates;
	std::vector<uint8> lag_history;

private:


};
