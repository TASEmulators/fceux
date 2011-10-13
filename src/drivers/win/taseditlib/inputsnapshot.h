//Specification file for Input Snapshot class

#include <time.h>

#define HOTCHANGE_BYTES_PER_JOY 4
#define SNAPSHOT_DESC_MAX_LENGTH 50

#define NUM_SUPPORTED_INPUT_TYPES 2
#define NORMAL_2JOYPADS 0
#define FOURSCORE 1

class INPUT_SNAPSHOT
{
public:
	INPUT_SNAPSHOT();
	void init(MovieData& md);
	void toMovie(MovieData& md, int start = 0);

	void save(EMUFILE *os);
	bool load(EMUFILE *is);
	bool skipLoad(EMUFILE *is);

	bool checkDiff(INPUT_SNAPSHOT& inp);
	bool checkJoypadDiff(INPUT_SNAPSHOT& inp, int frame, int joy);
	int findFirstChange(INPUT_SNAPSHOT& inp, int start = 0, int end = -1);
	int findFirstChange(MovieData& md);

	void SetMaxHotChange(int frame, int absolute_button);
	int GetHotChangeInfo(int frame, int absolute_button);

	int size;							// in frames
	int input_type;						// 0=normal, 1=fourscore, in future may support other input types
	std::vector<uint8> joysticks;		// Format: joy0-for-frame0, joy1-for-frame0, joy2-for-frame0, joy3-for-frame0, joy0-for-frame1, joy1-for-frame1, ...
	std::vector<uint8> hot_changes;		// Format: buttons01joy0-for-frame0, buttons23joy0-for-frame0, buttons45joy0-for-frame0, buttons67joy0-for-frame0, buttons01joy1-for-frame0, ...

	bool coherent;						// indicates whether this state was made by inputchange of previous state
	int jump_frame;						// for jumping when making undo
	char description[SNAPSHOT_DESC_MAX_LENGTH];

private:

};

