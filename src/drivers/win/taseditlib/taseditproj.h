//Specification file for the TASEdit Project file object
//Written by Chris220

//Contains all the TASEDit project and all files/settings associated with it
//Also contains all methods for manipulating the project files/settings, and saving them to disk

//The project file struct

#include <string>

class TASEDIT_PROJECT
{
public:
	void init();

	std::string GetProjectName();
	void SetProjectName(std::string e);

	std::string GetFM2Name();
	void SetFM2Name(std::string e);

	std::string GetProjectFile();
	void SetProjectFile(std::string e);

	//Guess what these functions are for...
	bool SaveProject();
	bool LoadProject(std::string PFN);

private:
	//The TASEdit Project's name
	std::string projectName;
	//The FM2's file name
	std::string fm2FileName;
	//The TASEdit Project's filename (For saving purposes)
	std::string projectFile;

	// If there are unsaved changes. 
	bool changed;
};
