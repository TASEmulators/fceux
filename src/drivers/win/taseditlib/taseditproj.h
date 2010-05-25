//Specification file for the TASEdit Project file object
//Written by Chris220

//Contains all the TASEDit project and all files/settings associated with it
//Also contains all methods for manipulating the project files/settings, and saving them to disk


//adelikat: How this needs to be structured conceptually
//A project is just all the settings and associated files
//No fm2 data shoudl be loaded here, instead all input logs that are unsaved should be stored in .ilog files which are stripped fm2 (no header info)
//When saving a project all ilog files are saved to disk if necessary, and the .tas project file is saved
//The "main branch" will be an ilog file too, but probably a special case from the others

//When loading, all settings are loaded into the project, all .ilog files are loaded, the main branch .log is loaded as the current fm2 file in the emulator
//The greenzone is loaded as well
//All notes are added to teh right columns
//All ilog files are listed in the input log list

#include <string>


//The notes feature, displays user notes in the notes column
struct TASENote
{
	std::string note;
	int frame;
};

//Movie header info
struct TASEHeader
{
	int version;				//Will always be 3 but might as well store it
	int emuVersion;				
	bool palFlag;
	std::string romFilename;
	std::string romChecksum;
	std::string guid;
	bool fourscore;				//note: TASEdit probably won't support 4 player input for quite some time
	bool microphone;
	bool port0;					//Always true
	bool port1;					//Should always be true for now at least
	bool port2;					//Will need to always be false until fourscore is supported
	bool FDS;
	bool NewPPU;				//Let's make this always false for now as to not support new PPU (until it is ready)
};

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
	std::string projectName;			//The TASEdit Project's name
	std::string fm2FileName;			//The main branch ilog file (todo rename more appropriately)
	std::string projectFile; 			//The TASEdit Project's filename (For saving purposes) //adelikat: TODO: why the hell is this different from project name??!
	std::vector<TASENote> notes;		//User notes
	std::vector<std::string> inputlogs;	//List of associated .ilog files
	TASEHeader header;
	std::vector<std::string> comments;
	std::vector<std::string> subtitles;

	bool changed;						// If there are unsaved changes. 
};
