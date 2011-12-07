//Specification file for the TASEdit Project class
#include <set>
typedef std::set<int> SelectionFrames;

#include <time.h>
#include "movie.h"
#include "../common.h"
#include "inputsnapshot.h"
#include "inputhistory.h"
#include "playback.h"
#include "recorder.h"
#include "greenzone.h"
#include "markers.h"
#include "bookmarks.h"
#include "tasedit_list.h"
#include "tasedit_sel.h"
#include "screenshot_display.h"

#define AUTOSAVE_PERIOD_SCALE 60000		// = 1 minute in milliseconds

class TASEDIT_PROJECT
{
public:
	TASEDIT_PROJECT();
	void init();
	void reset();
	void update();

	bool save();
	bool load(char* fullname);

	void RenameProject(char* new_fullname);

	std::string GetProjectFile();
	std::string GetProjectName();
	std::string GetFM2Name();

	void SetProjectChanged();
	bool GetProjectChanged();

	void SheduleNextAutosave();

	// public vars

private:
	bool changed;
	int next_save_shedule;

	std::string projectFile;	// full path
	std::string projectName;	// file name only
	std::string fm2FileName;	// same as projectName but with .fm2 extension instead of .tas

};
