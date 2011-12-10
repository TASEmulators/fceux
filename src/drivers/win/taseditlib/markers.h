//Specification file for Markers class

#define MARKER_FLAG_BIT 1

#define MARKERS_ID_LEN 8

class MARKERS
{
public:
	MARKERS();
	void init();
	void reset();
	void free();
	void update();

	void save(EMUFILE *os, bool really_save = true);
	bool load(EMUFILE *is);

	void MakeCopy(std::vector<uint8> &destination_array);
	void RestoreFromCopy(std::vector<uint8> &source_array, int until_frame = -1);

	bool GetMarker(int frame);
	int GetMarkersSize();

	void SetMarker(int frame);
	void ClearMarker(int frame);
	void EraseMarker(int frame);
	void ToggleMarker(int frame);

	void insertEmpty(int at, int frames);
	void truncateAt(int frame);

private:
	std::vector<uint8> markers_array;

};
