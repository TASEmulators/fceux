//Specification file for Greenzone class


class GREENZONE
{
public:
	GREENZONE();
	void init();
	void reset();
	void update();

	void TryDumpIncremental(bool lagFlag = true);

	void clearGreenzone();
	int dumpGreenzone(EMUFILE *os);
	bool loadGreenzone(EMUFILE *is);

	bool loadTasSavestate(int frame);
	void storeTasSavestate(int frame);
	void ClearGreenzoneTail();
	void ClearSavestate(int index);

	void InvalidateGreenZone(int after);

	int FindBeginningOfGreenZone(int starting_index = 0);

	// data
	int greenZoneCount;
	std::vector<std::vector<uint8>> savestates;

private:


};
