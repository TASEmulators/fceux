// Specification file for Markers class

#define MAX_NOTE_LEN 100

class MARKERS
{
public:
	MARKERS();

	void save(EMUFILE *os);
	bool load(EMUFILE *is);
	bool skipLoad(EMUFILE *is);

	void compress_data();
	bool Get_already_compressed();
	void Reset_already_compressed();

	// saved data
	std::vector<std::string> notes;		// Format: 0th - note for intro (Marker 0), 1st - note for Marker1, 2nd - note for Marker2, ...
	// not saved data
	std::vector<int> markers_array;		// Format: 0th = Marker num (id) for frame 0, 1st = Marker num for frame 1, ...

private:
	// also saved data
	std::vector<uint8> markers_array_compressed;

	bool already_compressed;			// to compress only once
};
