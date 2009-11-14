//Implementation file of TASEdit Project file object
//Written by Chris220

//Contains all the TASEDit project and all files/settings associated with it
//Also contains all methods for manipulating the project files/settings, and saving them to disk

/*
	//Private members
	//The TASEdit Project's name
	std::string projectName;
	//The FM2's file name
	std::string fm2FileName;
	//The TASEdit Project's filename (For saving purposes)
	std::string projectFile;
*/

#include <string>
#include <iostream>
#include <fstream>

#include "taseditproj.h"
#include "movie.h"

void TASEDIT_PROJECT::init()
{
	projectName="";
	fm2FileName="";
	projectFile="";

	changed=false;
}

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
	std::ofstream ofs;
	//ofs << GetProjectName() << std::endl;
	//ofs << GetFM2Name() << std::endl;
	ofs.open(filename);
	
	currMovieData.dump(&ofs, true);
	ofs.close();

	changed=false;
	return true;
}

extern bool LoadFM2(MovieData& movieData, std::istream* fp, int size, bool stopAfterHeader);


bool TASEDIT_PROJECT::LoadProject(std::string PFN)
{
	const char* filename = PFN.c_str();
	//char buf[4096];
	SetProjectName(PFN);
	std::ifstream ifs;
	ifs.open(filename);
	//ifs.getline(ifs, 4090);
	//ifs.getline(ifs, 4090);
	LoadFM2(currMovieData, &ifs, INT_MAX, false);
	LoadSubtitles(currMovieData);
	poweron(true);

	ifs.close();

	changed=false;
	return true;
}
