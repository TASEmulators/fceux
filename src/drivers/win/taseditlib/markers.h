//Specification file for Markers class

#define MARKER_FLAG_BIT 1

#define MARKERS_ID_LEN 8

class MARKERS
{
public:
	MARKERS();
	void init();
	void free();
	void update();

	void save(EMUFILE *os);
	bool load(EMUFILE *is);

	void ToggleMarker(int frame);
	void insertEmpty(int at, int frames);
	void truncateAt(int frame);

	std::vector<uint8> markers_array;

private:

};
