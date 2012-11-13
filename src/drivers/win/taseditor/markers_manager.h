// Specification file for Markers_manager class

#include "markers.h"

#define MARKERS_ID_LEN 8
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

enum
{
	MARKER_NOTE_EDIT_NONE, 
	MARKER_NOTE_EDIT_UPPER, 
	MARKER_NOTE_EDIT_LOWER
};

class MARKERS_MANAGER
{
public:
	MARKERS_MANAGER();
	void init();
	void free();
	void reset();
	void update();

	void save(EMUFILE *os, bool really_save = true);
	bool load(EMUFILE *is, unsigned int offset);

	int GetMarkersSize();
	bool SetMarkersSize(int new_size);

	int GetMarker(int frame);
	int GetMarkerUp(int start_frame);
	int GetMarkerUp(MARKERS& target_markers, int start_frame);		// special version of the function
	int GetMarkerFrame(int marker_id);

	int SetMarker(int frame);
	void ClearMarker(int frame);
	void ToggleMarker(int frame);

	bool EraseMarker(int frame, int frames = 1);
	bool insertEmpty(int at, int frames);

	int GetNotesSize();
	std::string GetNote(int index);
	std::string GetNote(MARKERS& target_markers, int index);		// special version of the function
	void SetNote(int index, const char* new_text);

	void MakeCopyTo(MARKERS& destination);
	void RestoreFromCopy(MARKERS& source);

	bool checkMarkersDiff(MARKERS& their_markers);

	void FindSimilar();
	void FindNextSimilar();

	void UpdateMarkerNote();

	// not saved vars
	int marker_note_edit;
	char findnote_string[MAX_NOTE_LEN];
	int search_similar_marker;

private:
	// saved vars
	MARKERS markers;

};
