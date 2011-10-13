//Implementation file of TASEdit Project class

#include "../main.h"
#include "taseditproj.h"

extern INPUT_HISTORY history;
extern PLAYBACK playback;
extern GREENZONE greenzone;

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
	//ofs << GetProjectName() << std::endl;
	//ofs << GetFM2Name() << std::endl;
	
	currMovieData.dump(ofs, true);
	greenzone.dumpGreenzone(ofs);
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

	FCEU_printf("Loading project %s\n", filename);

	LoadFM2(currMovieData, &ifs, ifs.size(), false);
	LoadSubtitles(currMovieData);

	// try to load greenzone
	if (!greenzone.loadGreenzone(&ifs))
	{
		// there was some error while loading greenzone - reset playback to frame 0
		poweron(true);
		currFrameCounter = 0;
		// try to load history
		history.load(&ifs);
	}

	reset();
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

