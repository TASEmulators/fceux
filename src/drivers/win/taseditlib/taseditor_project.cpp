//Implementation file of TASEDITOR_PROJECT class
#include "taseditor_project.h"
#include "utils/xstring.h"
#include "version.h"

extern TASEDITOR_CONFIG taseditor_config;
extern TASEDITOR_WINDOW taseditor_window;
extern MARKERS current_markers;
extern BOOKMARKS bookmarks;
extern POPUP_DISPLAY popup_display;
extern GREENZONE greenzone;
extern PLAYBACK playback;
extern RECORDER recorder;
extern INPUT_HISTORY history;
extern TASEDITOR_LIST list;
extern TASEDITOR_SELECTION selection;

extern void FCEU_PrintError(char *format, ...);
extern bool SaveProject();
extern bool SaveProjectAs();

TASEDITOR_PROJECT::TASEDITOR_PROJECT()
{
}

void TASEDITOR_PROJECT::init()
{
	// init new project
	projectFile = "";
	projectName = "";
	fm2FileName = "";

	reset();
}
void TASEDITOR_PROJECT::reset()
{
	changed = false;

}
void TASEDITOR_PROJECT::update()
{
	// if it's time to autosave - pop Save As dialog
	if (changed && taseditor_config.autosave_period && clock() >= next_save_shedule)
	{
		if (taseditor_config.silent_autosave)
			SaveProject();
		else
			SaveProjectAs();
		// in case user pressed Cancel, postpone saving to next time
		SheduleNextAutosave();
	}
	
}

bool TASEDITOR_PROJECT::save()
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
	list.save(ofs);
	selection.save(ofs);

	delete ofs;

	playback.updateProgressbar();
	this->reset();
	return true;
}
bool TASEDITOR_PROJECT::save_compact(char* filename, bool save_binary, bool save_markers, bool save_bookmarks, bool save_greenzone, bool save_history, bool save_list, bool save_selection)
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
	list.save(ofs, save_list);
	selection.save(ofs, save_selection);

	delete ofs;

	playback.updateProgressbar();
	return true;
}
bool TASEDITOR_PROJECT::load(char* fullname)
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
	list.load(&ifs);
	selection.load(&ifs);
	

	playback.reset();
	recorder.reset();
	popup_display.reset();
	reset();
	RenameProject(fullname);
	return true;
}

void TASEDITOR_PROJECT::RenameProject(char* new_fullname)
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
std::string TASEDITOR_PROJECT::GetProjectFile()
{
	return projectFile;
}
std::string TASEDITOR_PROJECT::GetProjectName()
{
	return projectName;
}
std::string TASEDITOR_PROJECT::GetFM2Name()
{
	return fm2FileName;
}

void TASEDITOR_PROJECT::SetProjectChanged()
{
	if (!changed)
	{
		changed = true;
		taseditor_window.RedrawCaption();
		SheduleNextAutosave();
	}
}
bool TASEDITOR_PROJECT::GetProjectChanged()
{
	return changed;
}

void TASEDITOR_PROJECT::SheduleNextAutosave()
{
	if (taseditor_config.autosave_period)
		next_save_shedule = clock() + taseditor_config.autosave_period * AUTOSAVE_PERIOD_SCALE;
}
