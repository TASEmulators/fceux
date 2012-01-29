//Specification file for Markers class
#define MAX_NOTE_LEN 100

class MARKERS
{
public:
	MARKERS();

	void save(EMUFILE *os);
	bool load(EMUFILE *is);
	bool skipLoad(EMUFILE *is);

	std::vector<int> markers_array;		// Format: 0th = marker num (id) for frame 0, 1st = marker num for frame 1, ...
	std::vector<std::string> notes;		// Format: 0th - note for intro (Marker 0), 1st - note for Marker1, 2nd - note for Marker2, ...

private:

};
