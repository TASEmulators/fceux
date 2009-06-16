//Specification file for the TASEdit Project file object
//Written by Chris220

//Contains all the TASEDit project and all files/settings associated with it
//Also contains all methods for manipulating the project files/settings, and saving them to disk

//The project file struct

#include <string>

using namespace std;

class TASEDIT_PROJECT
{
public:
	//The TASEdit Project's name
	std::string projectName;
	//The FM2's file name
	std::string fm2FileName;
	//The TASEdit Project's filename (For saving purposes)
	std::string projectFile;

	std::string GetProjectName();
	void SetProjectName(std::string e);

	std::string GetFM2Name();
	void SetFM2Name(std::string e);

	std::string GetProjectFile();
	void SetProjectFile(std::string e);

	//Guess what this function is for...
	void SaveProject();
};