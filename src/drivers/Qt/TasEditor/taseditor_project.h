// Specification file for the TASEDITOR_PROJECT class
#pragma once

#include <time.h>
#include "movie.h"
#include "Qt/TasEditor/taseditor_config.h"
#include "Qt/TasEditor/greenzone.h"
#include "Qt/TasEditor/selection.h"
#include "Qt/TasEditor/markers_manager.h"
#include "Qt/TasEditor/snapshot.h"
#include "Qt/TasEditor/bookmarks.h"
#include "Qt/TasEditor/branches.h"
#include "Qt/TasEditor/history.h"
#include "Qt/TasEditor/playback.h"
#include "Qt/TasEditor/recorder.h"
//#include "Qt/TasEditor/piano_roll.h"
#include "Qt/TasEditor/taseditor_lua.h"
#include "Qt/TasEditor/splicer.h"
//#include "Qt/TasEditor/editor.h"
//#include "Qt/TasEditor/popup_display.h"

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

#define NUM_JOYPAD_BUTTONS 8
#define MAX_NUM_JOYPADS 4

class TASEDITOR_PROJECT
{
public:
	TASEDITOR_PROJECT();
	void init();
	void reset();
	void update();

	bool save(const char* differentName = 0, bool inputInBinary = true, bool saveMarkers = true, bool saveBookmarks = true, int saveGreenzone = GREENZONE_SAVING_MODE_ALL, bool saveHistory = true, bool savePianoRoll = true, bool saveSelection = true);
	bool load(const char* fullName);

	void renameProject(const char* newFullName, bool filenameIsCorrect);

	std::string getProjectFile();
	std::string getProjectName();
	std::string getFM2Name();

	void setProjectChanged();
	bool getProjectChanged();

	void sheduleNextAutosave();

private:
	bool changed;
	int nextSaveShedule;

	std::string projectFile;	// full path
	std::string projectName;	// file name only
	std::string fm2FileName;	// same as projectName but with .fm2 extension instead of .fm3

};

int getInputType(MovieData& md);
void setInputType(MovieData& md, int new_input_type);
