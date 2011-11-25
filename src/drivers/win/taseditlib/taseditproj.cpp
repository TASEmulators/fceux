//Implementation file of TASEdit Project class
#include "taseditproj.h"

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
extern bool SaveProject();
extern void RedrawWindowCaption();

extern int TASEdit_autosave_period;

TASEDIT_PROJECT::TASEDIT_PROJECT()
{
}

void TASEDIT_PROJECT::init()
{
	// init new project
	projectName="";
	fm2FileName="";
	projectFile="";

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

bool TASEDIT_PROJECT::saveProject()
{
	std::string PFN = GetProjectFile();
	if (PFN.empty()) return false;
	const char* filename = PFN.c_str();
	EMUFILE_FILE* ofs = FCEUD_UTF8_fstream(filename,"wb");
	
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

extern bool LoadFM2(MovieData& movieData, EMUFILE* fp, int size, bool stopAfterHeader);


bool TASEDIT_PROJECT::LoadProject(std::string PFN)
{
	const char* filename = PFN.c_str();

	EMUFILE_FILE ifs(filename, "rb");

	FCEU_printf("\nLoading TASEdit project %s\n", filename);

	bool error;
	LoadFM2(currMovieData, &ifs, ifs.size(), false);
	LoadSubtitles(currMovieData);
	// try to load markers
	error = markers.load(&ifs);
	if (error)
	{
		FCEU_printf("Error loading markers\n");
		markers.init();
	} else
	{
		// try to load bookmarks
		error = bookmarks.load(&ifs);
	}
	if (error)
	{
		FCEU_printf("Error loading bookmarks\n");
		bookmarks.init();
	} else
	{
		// try to load greenzone
		error = greenzone.load(&ifs);
	}
	if (error)
	{
		FCEU_printf("Error loading greenzone\n");
		greenzone.init();
		playback.StartFromZero();		// reset playback to frame 0
	} else
	{
		// try to load history
		error = history.load(&ifs);
	}
	if (error)
	{
		FCEU_printf("Error loading history\n");
		history.init();
	} else
	{
		// try to load selection
		error = selection.load(&ifs);
	}
	if (error)
	{
		FCEU_printf("Error loading selection\n");
		selection.init();
	} else
	{
		// update and try to load list
		error = tasedit_list.load(&ifs);
	}
	if (error)
	{
		FCEU_printf("Error loading list\n");
	}

	playback.reset();
	recorder.reset();
	screenshot_display.reset();
	reset();
	return true;
}
// -----------------------------------------------------------------
std::string TASEDIT_PROJECT::GetProjectName()
{
	return projectName;
}
void TASEDIT_PROJECT::SetProjectName(std::string e)
{
	projectName = e;
}
std::string TASEDIT_PROJECT::GetFM2Name()
{
	return fm2FileName;
}
void TASEDIT_PROJECT::SetFM2Name(std::string e)
{
	fm2FileName = e;
}
std::string TASEDIT_PROJECT::GetProjectFile()
{
	return projectFile;
}
void TASEDIT_PROJECT::SetProjectFile(std::string e)
{
	projectFile = e;
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
