//Specification file for Markers class
#define MARKERS_ID_LEN 8
#define MAX_NOTE_LEN 100

class MARKERS
{
public:
	MARKERS();
	void init();
	void free();
	void reset();
	void update();

	void save(EMUFILE *os, bool really_save = true);
	bool load(EMUFILE *is);
	bool skipLoad(EMUFILE *is);

	void MakeCopy(MARKERS& source);
	void CopyMarkersHere(std::vector<int>& array_for_markers, std::vector<std::string>& for_notes);
	void RestoreFromCopy(MARKERS& source, int until_frame = -1);

	int GetMarkersSize();
	void SetMarkersSize(int new_size);

	int GetMarker(int frame);
	int GetMarkerUp(int start_frame);
	int GetMarkerFrame(int marker_id);

	void SetMarker(int frame);
	void ClearMarker(int frame);
	void ToggleMarker(int frame);

	void EraseMarker(int frame);
	void insertEmpty(int at, int frames);

	int GetNotesSize();
	std::string GetNote(int index);
	void SetNote(int index, char* new_text);

	bool checkMarkersDiff(MARKERS& their_markers);
	bool checkMarkersDiff(MARKERS& their_markers, int end);

private:
	std::vector<int> markers_array;		// Format: 0th = marker num (id) for frame 0, 1st = marker num for frame 1, ...
	std::vector<std::string> notes;		// Format: 0th - note for intro (Marker 0), 1st - note for Marker1, 2nd - note for Marker2, ...

};
