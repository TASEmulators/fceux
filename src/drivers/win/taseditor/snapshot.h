// Specification file for Snapshot class

#include "inputlog.h"

#define SNAPSHOT_DESC_MAX_LENGTH 100

class SNAPSHOT
{
public:
	SNAPSHOT();
	void init(MovieData& md, bool hotchanges, int force_input_type = -1);

	bool MarkersDifferFromCurrent();
	void copyToMarkers();

	void compress_data();
	bool Get_already_compressed();

	void save(EMUFILE *os);
	bool load(EMUFILE *is);
	bool skipLoad(EMUFILE *is);

	// saved data
	INPUTLOG inputlog;
	LAGLOG laglog;
	MARKERS markers;
	int keyframe;						// for jumping when making undo
	int start_frame;					// for consecutive Draws and "Related items highlighting"
	int end_frame;						// for consecutive Draws and "Related items highlighting"
	int consecutive_tag;				// for consecutive Recordings and Draws
	uint32 rec_joypad_diff_bits;		// for consecutive Recordings
	int mod_type;
	char description[SNAPSHOT_DESC_MAX_LENGTH];

private:

};

