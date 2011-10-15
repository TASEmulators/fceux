//Specification file for Markers class

#define MARKER_FLAG_BIT 1

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

	std::vector<uint8> markers_array;

private:

};
