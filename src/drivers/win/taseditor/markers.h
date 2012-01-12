//Specification file for Markers class
#define MARKERS_ID_LEN 8
#define MAX_NOTE_LEN 100

// constants for "Find Similar Note" algorithm (may need finetuning)
#define KEYWORD_MIN_LEN 2
#define MAX_NUM_KEYWORDS (MAX_NOTE_LEN / (KEYWORD_MIN_LEN+1)) + 1
#define KEYWORD_WEIGHT_BASE 2.0
#define KEYWORD_WEIGHT_FACTOR -1.0
#define KEYWORD_CASEINSENTITIVE_BONUS_PER_CHAR 1.0		// these two should be small, because they also work when keyword is inside another keyword, giving irrelevant results
#define KEYWORD_CASESENTITIVE_BONUS_PER_CHAR 1.0
#define KEYWORD_SEQUENCE_BONUS_PER_CHAR 5.0
#define KEYWORD_PENALTY_FOR_STRANGERS 0.2

#define KEYWORDS_LINE_MIN_SEQUENCE 1

#define MIN_PRIORITY_TRESHOLD 5.0

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

	int SetMarker(int frame);
	void ClearMarker(int frame);
	void ToggleMarker(int frame);

	void EraseMarker(int frame);
	void insertEmpty(int at, int frames);

	int GetNotesSize();
	std::string GetNote(int index);
	void SetNote(int index, const char* new_text);

	bool checkMarkersDiff(MARKERS& their_markers);
	bool checkMarkersDiff(MARKERS& their_markers, int end);

	void FindSimilar(int offset);

private:
	std::vector<int> markers_array;		// Format: 0th = marker num (id) for frame 0, 1st = marker num for frame 1, ...
	std::vector<std::string> notes;		// Format: 0th - note for intro (Marker 0), 1st - note for Marker1, 2nd - note for Marker2, ...

};
