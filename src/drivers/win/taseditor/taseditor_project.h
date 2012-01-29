//Specification file for the TASEDITOR_PROJECT class
#include <time.h>
#include "movie.h"
#include "../common.h"
#include "taseditor_config.h"
#include "taseditor_window.h"
#include "taseditor_sel.h"
#include "markers_manager.h"
#include "snapshot.h"
#include "history.h"
#include "playback.h"
#include "recorder.h"
#include "greenzone.h"
#include "bookmarks.h"
#include "taseditor_list.h"
#include "taseditor_lua.h"
#include "splicer.h"
#include "popup_display.h"

#define AUTOSAVE_PERIOD_SCALE 60000		// = 1 minute in milliseconds

#define MARKERS_SAVED 1
#define BOOKMARKS_SAVED 2
#define GREENZONE_SAVED 4
#define HISTORY_SAVED 8
#define LIST_SAVED 16
#define SELECTION_SAVED 32
#define ALL_SAVED MARKERS_SAVED|BOOKMARKS_SAVED|GREENZONE_SAVED|HISTORY_SAVED|LIST_SAVED|SELECTION_SAVED

class TASEDITOR_PROJECT
{
public:
	TASEDITOR_PROJECT();
	void init();
	void reset();
	void update();

	bool save();
	bool save_compact(char* filename, bool save_binary, bool save_markers, bool save_bookmarks, bool save_greenzone, bool save_history, bool save_list, bool save_selection);
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
	std::string fm2FileName;	// same as projectName but with .fm2 extension instead of .fm3

};
