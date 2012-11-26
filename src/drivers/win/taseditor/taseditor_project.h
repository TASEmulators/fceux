// Specification file for the TASEDITOR_PROJECT class

#include <time.h>
#include "movie.h"
#include "../common.h"
#include "taseditor_config.h"
#include "taseditor_window.h"
#include "greenzone.h"
#include "selection.h"
#include "markers_manager.h"
#include "snapshot.h"
#include "bookmarks.h"
#include "branches.h"
#include "history.h"
#include "playback.h"
#include "recorder.h"
#include "piano_roll.h"
#include "taseditor_lua.h"
#include "splicer.h"
#include "editor.h"
#include "popup_display.h"


//not available unless we #define _WIN32_WINNT >= 0x501 (XP) and we're trying very hard to keep 2000 support.
#ifndef LVS_EX_DOUBLEBUFFER
#define LVS_EX_DOUBLEBUFFER     0x00010000
#endif

#define AUTOSAVE_PERIOD_SCALE 60000		// = 1 minute in milliseconds

#define MARKERS_SAVED 1
#define BOOKMARKS_SAVED 2
#define GREENZONE_SAVED 4
#define HISTORY_SAVED 8
#define PIANO_ROLL_SAVED 16
#define SELECTION_SAVED 32

#define PROJECT_FILE_CURRENT_VERSION 3

#define PROJECT_FILE_OFFSET_OF_VERSION_NUMBER 0
#define PROJECT_FILE_OFFSET_OF_SAVED_MODULES_MAP (PROJECT_FILE_OFFSET_OF_VERSION_NUMBER + 4)
#define PROJECT_FILE_OFFSET_OF_NUMBER_OF_POINTERS (PROJECT_FILE_OFFSET_OF_SAVED_MODULES_MAP + 4)
#define DEFAULT_NUMBER_OF_POINTERS 6
#define PROJECT_FILE_OFFSET_OF_POINTERS_DATA (PROJECT_FILE_OFFSET_OF_NUMBER_OF_POINTERS + 4)

class TASEDITOR_PROJECT
{
public:
	TASEDITOR_PROJECT();
	void init();
	void reset();
	void update();

	bool save(const char* different_name = 0, bool save_binary = true, bool save_markers = true, bool save_bookmarks = true, bool save_greenzone = true, bool save_history = true, bool save_piano_roll = true, bool save_selection = true);
	bool load(const char* fullname);

	void RenameProject(const char* new_fullname, bool filename_is_correct);

	std::string GetProjectFile();
	std::string GetProjectName();
	std::string GetFM2Name();

	void SetProjectChanged();
	bool GetProjectChanged();

	void SheduleNextAutosave();

private:
	bool changed;
	int next_save_shedule;

	std::string projectFile;	// full path
	std::string projectName;	// file name only
	std::string fm2FileName;	// same as projectName but with .fm2 extension instead of .fm3

};
