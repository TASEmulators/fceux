//Implementation file of TASEdit Project file object

//Contains all the TASEDit project and all files/settings associated with it
//Also contains all methods for manipulating the project files/settings, and saving them to disk


#include <string>
#include <iostream>
#include <fstream>
#include "../main.h"
#include "taseditproj.h"
#include "movie.h"

TASEDIT_PROJECT::TASEDIT_PROJECT()	//Non parameterized constructor, loads project with default values
{

}

void TASEDIT_PROJECT::init()	//TODO: rip this out! This should be the class constructor instead
{
	projectName="";
	fm2FileName="";
	projectFile="";

	changed=false;
}

//TODO: a parameterized constructor that can serve as an import fm2 function

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

bool TASEDIT_PROJECT::SaveProject()
{
	std::string PFN = GetProjectFile();
	const char* filename = PFN.c_str();
	EMUFILE_FILE* ofs = FCEUD_UTF8_fstream(filename,"wb");
	//ofs << GetProjectName() << std::endl;
	//ofs << GetFM2Name() << std::endl;
	
	currMovieData.dump(ofs, true);
	ofs->fputc('\0'); // TODO: Add main branch name. 
	currMovieData.dumpGreenzone(ofs, true);

	delete ofs;

	changed=false;
	return true;
}

extern bool LoadFM2(MovieData& movieData, EMUFILE* fp, int size, bool stopAfterHeader);


bool TASEDIT_PROJECT::LoadProject(std::string PFN)
{
	const char* filename = PFN.c_str();

	SetProjectName(PFN);
	EMUFILE_FILE ifs(filename, "rb");

	LoadFM2(currMovieData, &ifs, ifs.size(), false);
	LoadSubtitles(currMovieData);

	char branchname;
	branchname = ifs.fgetc(); // TODO: Add main branch name. 
	currMovieData.loadGreenzone(&ifs, true);

	poweron(true);
	currFrameCounter = currMovieData.greenZoneCount;
	currMovieData.TryDumpIncremental();

	changed=false;
	return true;
}
