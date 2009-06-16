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

void TASEDIT_PROJECT::SaveProject()
{
	std::string PFN = GetProjectFile();
	const char* filename = PFN.c_str();
	std::ofstream ofs;
	ofs.open(filename);
	ofs << GetProjectName() << std::endl;
	ofs << GetFM2Name() << std::endl;
	ofs.close();
}