//Implementation file of TASEdit Project class

#include "../main.h"
#include "taseditproj.h"

extern MARKERS markers;
extern GREENZONE greenzone;
extern PLAYBACK playback;
extern INPUT_HISTORY history;

extern void FCEU_printf(char *format, ...);

TASEDIT_PROJECT::TASEDIT_PROJECT()	//Non parameterized constructor, loads project with default values
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
	old_changed = changed = false;

}
void TASEDIT_PROJECT::update()
{
	old_changed = changed;


}

bool TASEDIT_PROJECT::saveProject()
{
	std::string PFN = GetProjectFile();
	if (PFN.empty()) return false;
	const char* filename = PFN.c_str();
	EMUFILE_FILE* ofs = FCEUD_UTF8_fstream(filename,"wb");
	
	currMovieData.dump(ofs, true);
	markers.save(ofs);
	greenzone.save(ofs);
	history.save(ofs);

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
	}

	reset();
	playback.updateProgressbar();
	return true;
}
// -----------------------------------------------------------------
//All the get/set functions...
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

