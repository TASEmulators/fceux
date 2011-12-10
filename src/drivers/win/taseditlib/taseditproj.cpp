//Implementation file of TASEdit Project class
#include "taseditproj.h"
#include "utils/xstring.h"

extern MARKERS markers;
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
	markers.save(ofs);
	bookmarks.save(ofs);
	greenzone.save(ofs);
	history.save(ofs);
	selection.save(ofs);
	tasedit_list.save(ofs);

	delete ofs;

	playback.updateProgressbar();
	this->reset();
	return true;
}
bool TASEDIT_PROJECT::save_compact(char* filename, bool save_binary, bool save_markers, bool save_bookmarks, bool save_greenzone, bool save_history, bool save_selection, bool save_list)
{
	EMUFILE_FILE* ofs = FCEUD_UTF8_fstream(filename,"wb");
	
	currMovieData.loadFrameCount = currMovieData.records.size();
	currMovieData.dump(ofs, save_binary);
	markers.save(ofs, save_markers);
	bookmarks.save(ofs, save_bookmarks);
	greenzone.save(ofs, save_greenzone);
	history.save(ofs, save_history);
	selection.save(ofs, save_selection);
	tasedit_list.save(ofs, save_list);

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
		LoadSubtitles(currMovieData);
	} else
	{
		FCEU_PrintError("Error loading movie data from %s!", fullname);
		return false;
	}
	markers.load(&ifs);
	bookmarks.load(&ifs);
	greenzone.load(&ifs);
	history.load(&ifs);
	selection.load(&ifs);
	tasedit_list.load(&ifs);

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
