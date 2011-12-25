//Implementation file of TASEdit Project class
#include "taseditproj.h"
#include "utils/xstring.h"
#include "version.h"

#define MARKERS_SAVED 1
#define BOOKMARKS_SAVED 2
#define GREENZONE_SAVED 4
#define HISTORY_SAVED 8
#define LIST_SAVED 16
#define SELECTION_SAVED 32
#define ALL_SAVED MARKERS_SAVED|BOOKMARKS_SAVED|GREENZONE_SAVED|HISTORY_SAVED|LIST_SAVED|SELECTION_SAVED

extern MARKERS current_markers;
extern BOOKMARKS bookmarks;
extern SCREENSHOT_DISPLAY screenshot_display;
extern GREENZONE greenzone;
extern PLAYBACK playback;
extern RECORDER recorder;
extern INPUT_HISTORY history;
extern TASEDIT_LIST tasedit_list;
extern TASEDIT_SELECTION selection;

extern void FCEU_printf(char *format, ...);
extern void FCEU_PrintError(char *format, ...);
extern bool SaveProject();
extern void RedrawWindowCaption();

extern int TASEdit_autosave_period;

TASEDIT_PROJECT::TASEDIT_PROJECT()
{
}

void TASEDIT_PROJECT::init()
{
	// init new project
	projectFile = "";
	projectName = "";
	fm2FileName = "";

	reset();
}
void TASEDIT_PROJECT::reset()
{
	changed = false;

}
void TASEDIT_PROJECT::update()
{
	// if it's time to autosave - save
	if (changed && TASEdit_autosave_period && clock() >= next_save_shedule)
	{
		SaveProject();
		// in case user pressed Cancel, postpone saving to next time
		SheduleNextAutosave();
	}
	
}

bool TASEDIT_PROJECT::save()
{
	std::string PFN = GetProjectFile();
	if (PFN.empty()) return false;
	const char* filename = PFN.c_str();
	EMUFILE_FILE* ofs = FCEUD_UTF8_fstream(filename,"wb");
	
	currMovieData.loadFrameCount = currMovieData.records.size();
	currMovieData.dump(ofs, true);

	// save all modules
	unsigned int saved_stuff = ALL_SAVED;
	write32le(saved_stuff, ofs);
	current_markers.save(ofs);
	bookmarks.save(ofs);
	greenzone.save(ofs);
	history.save(ofs);
	tasedit_list.save(ofs);
	selection.save(ofs);

	delete ofs;

	playback.updateProgressbar();
	this->reset();
	return true;
}
bool TASEDIT_PROJECT::save_compact(char* filename, bool save_binary, bool save_markers, bool save_bookmarks, bool save_greenzone, bool save_history, bool save_list, bool save_selection)
{
	EMUFILE_FILE* ofs = FCEUD_UTF8_fstream(filename,"wb");
	
	currMovieData.loadFrameCount = currMovieData.records.size();
	currMovieData.dump(ofs, save_binary);

	// save specified modules
	unsigned int saved_stuff = 0;
	if (save_markers) saved_stuff |= MARKERS_SAVED;
	if (save_bookmarks) saved_stuff |= BOOKMARKS_SAVED;
	if (save_greenzone) saved_stuff |= GREENZONE_SAVED;
	if (save_history) saved_stuff |= HISTORY_SAVED;
	if (save_list) saved_stuff |= LIST_SAVED;
	if (save_selection) saved_stuff |= SELECTION_SAVED;
	write32le(saved_stuff, ofs);
	current_markers.save(ofs, save_markers);
	bookmarks.save(ofs, save_bookmarks);
	greenzone.save(ofs, save_greenzone);
	history.save(ofs, save_history);
	tasedit_list.save(ofs, save_list);
	selection.save(ofs, save_selection);

	delete ofs;

	playback.updateProgressbar();
	return true;
}
bool TASEDIT_PROJECT::load(char* fullname)
{
	EMUFILE_FILE ifs(fullname, "rb");

	if(ifs.fail())
	{
		FCEU_PrintError("Error opening %s!", fullname);
		return false;
	}

	FCEU_printf("\nLoading TAS Editor project %s...\n", fullname);

	MovieData tempMovieData = MovieData();
	extern bool LoadFM2(MovieData& movieData, EMUFILE* fp, int size, bool stopAfterHeader);
	if (LoadFM2(tempMovieData, &ifs, ifs.size(), false))
	{
		currMovieData = tempMovieData;
		currMovieData.emuVersion = FCEU_VERSION_NUMERIC;
		LoadSubtitles(currMovieData);
	} else
	{
		FCEU_PrintError("Error loading movie data from %s!", fullname);
		// do not load the project
		return false;
	}

	// load modules
	unsigned int saved_stuff;
	read32le(&saved_stuff, &ifs);
	current_markers.load(&ifs);
	bookmarks.load(&ifs);
	greenzone.load(&ifs);
	history.load(&ifs);
	tasedit_list.load(&ifs);
	selection.load(&ifs);
	

	playback.reset();
	recorder.reset();
	screenshot_display.reset();
	reset();
	RenameProject(fullname);
	return true;
}

void TASEDIT_PROJECT::RenameProject(char* new_fullname)
{
	projectFile = new_fullname;
	char drv[512], dir[512], name[512], ext[512];		// For getting the filename
	splitpath(new_fullname, drv, dir, name, ext);
	projectName = name;
	std::string thisfm2name = name;
	thisfm2name.append(".fm2");
	fm2FileName = thisfm2name;
}
// -----------------------------------------------------------------
std::string TASEDIT_PROJECT::GetProjectFile()
{
	return projectFile;
}
std::string TASEDIT_PROJECT::GetProjectName()
{
	return projectName;
}
std::string TASEDIT_PROJECT::GetFM2Name()
{
	return fm2FileName;
}

void TASEDIT_PROJECT::SetProjectChanged()
{
	if (!changed)
	{
		changed = true;
		RedrawWindowCaption();
		SheduleNextAutosave();
	}
}
bool TASEDIT_PROJECT::GetProjectChanged()
{
	return changed;
}

void TASEDIT_PROJECT::SheduleNextAutosave()
{
	if (TASEdit_autosave_period)
		next_save_shedule = clock() + TASEdit_autosave_period * AUTOSAVE_PERIOD_SCALE;
}
